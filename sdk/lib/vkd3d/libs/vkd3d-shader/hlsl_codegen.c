/*
 * HLSL optimization and code generation
 *
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
#include <math.h>

/* TODO: remove when no longer needed, only used for new_offset_instr_from_deref() */
static struct hlsl_ir_node *new_offset_from_path_index(struct hlsl_ctx *ctx, struct hlsl_block *block,
        struct hlsl_type *type, struct hlsl_ir_node *base_offset, struct hlsl_ir_node *idx,
        enum hlsl_regset regset, unsigned int *offset_component, const struct vkd3d_shader_location *loc)
{
    struct hlsl_ir_node *idx_offset = NULL;
    struct hlsl_ir_node *c;

    switch (type->class)
    {
        case HLSL_CLASS_VECTOR:
            if (idx->type != HLSL_IR_CONSTANT)
            {
                hlsl_fixme(ctx, &idx->loc, "Non-constant vector addressing.");
                break;
            }
            *offset_component += hlsl_ir_constant(idx)->value.u[0].u;
            break;

        case HLSL_CLASS_MATRIX:
        {
            idx_offset = idx;
            break;
        }

        case HLSL_CLASS_ARRAY:
        {
            unsigned int size = hlsl_type_get_array_element_reg_size(type->e.array.type, regset);

            if (regset == HLSL_REGSET_NUMERIC)
            {
                VKD3D_ASSERT(size % 4 == 0);
                size /= 4;
            }

            if (!(c = hlsl_new_uint_constant(ctx, size, loc)))
                return NULL;
            hlsl_block_add_instr(block, c);

            if (!(idx_offset = hlsl_new_binary_expr(ctx, HLSL_OP2_MUL, c, idx)))
                return NULL;
            hlsl_block_add_instr(block, idx_offset);

            break;
        }

        case HLSL_CLASS_STRUCT:
        {
            unsigned int field_idx = hlsl_ir_constant(idx)->value.u[0].u;
            struct hlsl_struct_field *field = &type->e.record.fields[field_idx];
            unsigned int field_offset = field->reg_offset[regset];

            if (regset == HLSL_REGSET_NUMERIC)
            {
                VKD3D_ASSERT(*offset_component == 0);
                *offset_component = field_offset % 4;
                field_offset /= 4;
            }

            if (!(c = hlsl_new_uint_constant(ctx, field_offset, loc)))
                return NULL;
            hlsl_block_add_instr(block, c);

            idx_offset = c;

            break;
        }

        default:
            vkd3d_unreachable();
    }

    if (idx_offset)
    {
        if (!(base_offset = hlsl_new_binary_expr(ctx, HLSL_OP2_ADD, base_offset, idx_offset)))
            return NULL;
        hlsl_block_add_instr(block, base_offset);
    }

    return base_offset;
}

/* TODO: remove when no longer needed, only used for replace_deref_path_with_offset() */
static struct hlsl_ir_node *new_offset_instr_from_deref(struct hlsl_ctx *ctx, struct hlsl_block *block,
        const struct hlsl_deref *deref, unsigned int *offset_component, const struct vkd3d_shader_location *loc)
{
    enum hlsl_regset regset = hlsl_deref_get_regset(ctx, deref);
    struct hlsl_ir_node *offset;
    struct hlsl_type *type;
    unsigned int i;

    *offset_component = 0;

    hlsl_block_init(block);

    if (!(offset = hlsl_new_uint_constant(ctx, 0, loc)))
        return NULL;
    hlsl_block_add_instr(block, offset);

    VKD3D_ASSERT(deref->var);
    type = deref->var->data_type;

    for (i = 0; i < deref->path_len; ++i)
    {
        struct hlsl_block idx_block;

        hlsl_block_init(&idx_block);

        if (!(offset = new_offset_from_path_index(ctx, &idx_block, type, offset, deref->path[i].node,
                regset, offset_component, loc)))
        {
            hlsl_block_cleanup(&idx_block);
            return NULL;
        }

        hlsl_block_add_block(block, &idx_block);

        type = hlsl_get_element_type_from_path_index(ctx, type, deref->path[i].node);
    }

    return offset;
}

/* TODO: remove when no longer needed, only used for transform_deref_paths_into_offsets() */
static bool replace_deref_path_with_offset(struct hlsl_ctx *ctx, struct hlsl_deref *deref,
        struct hlsl_ir_node *instr)
{
    unsigned int offset_component;
    struct hlsl_ir_node *offset;
    struct hlsl_block block;
    struct hlsl_type *type;

    VKD3D_ASSERT(deref->var);
    VKD3D_ASSERT(!hlsl_deref_is_lowered(deref));

    type = hlsl_deref_get_type(ctx, deref);

    /* Instructions that directly refer to structs or arrays (instead of single-register components)
     * are removed later by dce. So it is not a problem to just cleanup their derefs. */
    if (type->class == HLSL_CLASS_STRUCT || type->class == HLSL_CLASS_ARRAY)
    {
        hlsl_cleanup_deref(deref);
        return true;
    }

    deref->data_type = type;

    if (!(offset = new_offset_instr_from_deref(ctx, &block, deref, &offset_component, &instr->loc)))
        return false;
    list_move_before(&instr->entry, &block.instrs);

    hlsl_cleanup_deref(deref);
    hlsl_src_from_node(&deref->rel_offset, offset);
    deref->const_offset = offset_component;

    return true;
}

static bool clean_constant_deref_offset_srcs(struct hlsl_ctx *ctx, struct hlsl_deref *deref,
        struct hlsl_ir_node *instr)
{
    if (deref->rel_offset.node && deref->rel_offset.node->type == HLSL_IR_CONSTANT)
    {
        enum hlsl_regset regset = hlsl_deref_get_regset(ctx, deref);

        if (regset == HLSL_REGSET_NUMERIC)
            deref->const_offset += 4 * hlsl_ir_constant(deref->rel_offset.node)->value.u[0].u;
        else
            deref->const_offset += hlsl_ir_constant(deref->rel_offset.node)->value.u[0].u;
        hlsl_src_remove(&deref->rel_offset);
        return true;
    }
    return false;
}


/* Split uniforms into two variables representing the constant and temp
 * registers, and copy the former to the latter, so that writes to uniforms
 * work. */
static void prepend_uniform_copy(struct hlsl_ctx *ctx, struct hlsl_block *block, struct hlsl_ir_var *temp)
{
    struct hlsl_ir_var *uniform;
    struct hlsl_ir_node *store;
    struct hlsl_ir_load *load;
    char *new_name;

    /* Use the synthetic name for the temp, rather than the uniform, so that we
     * can write the uniform name into the shader reflection data. */

    if (!(uniform = hlsl_new_var(ctx, temp->name, temp->data_type,
            &temp->loc, NULL, temp->storage_modifiers, &temp->reg_reservation)))
        return;
    list_add_before(&temp->scope_entry, &uniform->scope_entry);
    list_add_tail(&ctx->extern_vars, &uniform->extern_entry);
    uniform->is_uniform = 1;
    uniform->is_param = temp->is_param;
    uniform->buffer = temp->buffer;
    if (temp->default_values)
    {
        /* Transfer default values from the temp to the uniform. */
        VKD3D_ASSERT(!uniform->default_values);
        VKD3D_ASSERT(hlsl_type_component_count(temp->data_type) == hlsl_type_component_count(uniform->data_type));
        uniform->default_values = temp->default_values;
        temp->default_values = NULL;
    }

    if (!(new_name = hlsl_sprintf_alloc(ctx, "<temp-%s>", temp->name)))
        return;
    temp->name = new_name;

    if (!(load = hlsl_new_var_load(ctx, uniform, &temp->loc)))
        return;
    list_add_head(&block->instrs, &load->node.entry);

    if (!(store = hlsl_new_simple_store(ctx, temp, &load->node)))
        return;
    list_add_after(&load->node.entry, &store->entry);
}

static void validate_field_semantic(struct hlsl_ctx *ctx, struct hlsl_struct_field *field)
{
    if (!field->semantic.name && hlsl_is_numeric_type(hlsl_get_multiarray_element_type(field->type))
            && !field->semantic.reported_missing)
    {
        hlsl_error(ctx, &field->loc, VKD3D_SHADER_ERROR_HLSL_MISSING_SEMANTIC,
                "Field '%s' is missing a semantic.", field->name);
        field->semantic.reported_missing = true;
    }
}

static enum hlsl_base_type base_type_get_semantic_equivalent(enum hlsl_base_type base)
{
    if (base == HLSL_TYPE_BOOL)
        return HLSL_TYPE_UINT;
    if (base == HLSL_TYPE_INT)
        return HLSL_TYPE_UINT;
    if (base == HLSL_TYPE_HALF)
        return HLSL_TYPE_FLOAT;
    return base;
}

static bool types_are_semantic_equivalent(struct hlsl_ctx *ctx, const struct hlsl_type *type1,
        const struct hlsl_type *type2)
{
    if (ctx->profile->major_version < 4)
        return true;

    if (type1->dimx != type2->dimx)
        return false;

    return base_type_get_semantic_equivalent(type1->e.numeric.type)
            == base_type_get_semantic_equivalent(type2->e.numeric.type);
}

static struct hlsl_ir_var *add_semantic_var(struct hlsl_ctx *ctx, struct hlsl_ir_function_decl *func,
        struct hlsl_ir_var *var, struct hlsl_type *type, uint32_t modifiers, struct hlsl_semantic *semantic,
        uint32_t index, bool output, bool force_align, const struct vkd3d_shader_location *loc)
{
    struct hlsl_semantic new_semantic;
    struct hlsl_ir_var *ext_var;
    char *new_name;

    if (!(new_name = hlsl_sprintf_alloc(ctx, "<%s-%s%u>", output ? "output" : "input", semantic->name, index)))
        return NULL;

    LIST_FOR_EACH_ENTRY(ext_var, &func->extern_vars, struct hlsl_ir_var, extern_entry)
    {
        if (!ascii_strcasecmp(ext_var->name, new_name))
        {
            if (output)
            {
                if (index >= semantic->reported_duplicated_output_next_index)
                {
                    hlsl_error(ctx, loc, VKD3D_SHADER_ERROR_HLSL_INVALID_SEMANTIC,
                            "Output semantic \"%s%u\" is used multiple times.", semantic->name, index);
                    hlsl_note(ctx, &ext_var->loc, VKD3D_SHADER_LOG_ERROR,
                            "First use of \"%s%u\" is here.", semantic->name, index);
                    semantic->reported_duplicated_output_next_index = index + 1;
                }
            }
            else
            {
                if (index >= semantic->reported_duplicated_input_incompatible_next_index
                        && !types_are_semantic_equivalent(ctx, ext_var->data_type, type))
                {
                    hlsl_error(ctx, loc, VKD3D_SHADER_ERROR_HLSL_INVALID_SEMANTIC,
                            "Input semantic \"%s%u\" is used multiple times with incompatible types.",
                            semantic->name, index);
                    hlsl_note(ctx, &ext_var->loc, VKD3D_SHADER_LOG_ERROR,
                            "First declaration of \"%s%u\" is here.", semantic->name, index);
                    semantic->reported_duplicated_input_incompatible_next_index = index + 1;
                }
            }

            vkd3d_free(new_name);
            return ext_var;
        }
    }

    if (!(hlsl_clone_semantic(ctx, &new_semantic, semantic)))
    {
        vkd3d_free(new_name);
        return NULL;
    }
    new_semantic.index = index;
    if (!(ext_var = hlsl_new_var(ctx, new_name, type, loc, &new_semantic, modifiers, NULL)))
    {
        vkd3d_free(new_name);
        hlsl_cleanup_semantic(&new_semantic);
        return NULL;
    }
    if (output)
        ext_var->is_output_semantic = 1;
    else
        ext_var->is_input_semantic = 1;
    ext_var->is_param = var->is_param;
    ext_var->force_align = force_align;
    list_add_before(&var->scope_entry, &ext_var->scope_entry);
    list_add_tail(&func->extern_vars, &ext_var->extern_entry);

    return ext_var;
}

static uint32_t combine_field_storage_modifiers(uint32_t modifiers, uint32_t field_modifiers)
{
    field_modifiers |= modifiers;

    /* TODO: 'sample' modifier is not supported yet. */

    /* 'nointerpolation' always takes precedence, next the same is done for
     * 'sample', remaining modifiers are combined. */
    if (field_modifiers & HLSL_STORAGE_NOINTERPOLATION)
    {
        field_modifiers &= ~HLSL_INTERPOLATION_MODIFIERS_MASK;
        field_modifiers |= HLSL_STORAGE_NOINTERPOLATION;
    }

    return field_modifiers;
}

static void prepend_input_copy(struct hlsl_ctx *ctx, struct hlsl_ir_function_decl *func, struct hlsl_ir_load *lhs,
        uint32_t modifiers, struct hlsl_semantic *semantic, uint32_t semantic_index, bool force_align)
{
    struct hlsl_type *type = lhs->node.data_type, *vector_type_src, *vector_type_dst;
    struct vkd3d_shader_location *loc = &lhs->node.loc;
    struct hlsl_ir_var *var = lhs->src.var;
    struct hlsl_ir_node *c;
    unsigned int i;

    if (!hlsl_is_numeric_type(type))
    {
        struct vkd3d_string_buffer *string;
        if (!(string = hlsl_type_to_string(ctx, type)))
            return;
        hlsl_fixme(ctx, &var->loc, "Input semantics for type %s.", string->buffer);
        hlsl_release_string_buffer(ctx, string);
    }
    if (!semantic->name)
        return;

    vector_type_dst = hlsl_get_vector_type(ctx, type->e.numeric.type, hlsl_type_minor_size(type));
    vector_type_src = vector_type_dst;
    if (ctx->profile->major_version < 4 && ctx->profile->type == VKD3D_SHADER_TYPE_VERTEX)
        vector_type_src = hlsl_get_vector_type(ctx, type->e.numeric.type, 4);

    if (hlsl_type_major_size(type) > 1)
        force_align = true;

    for (i = 0; i < hlsl_type_major_size(type); ++i)
    {
        struct hlsl_ir_node *store, *cast;
        struct hlsl_ir_var *input;
        struct hlsl_ir_load *load;

        if (!(input = add_semantic_var(ctx, func, var, vector_type_src,
                modifiers, semantic, semantic_index + i, false, force_align, loc)))
            return;

        if (!(load = hlsl_new_var_load(ctx, input, &var->loc)))
            return;
        list_add_after(&lhs->node.entry, &load->node.entry);

        if (!(cast = hlsl_new_cast(ctx, &load->node, vector_type_dst, &var->loc)))
            return;
        list_add_after(&load->node.entry, &cast->entry);

        if (type->class == HLSL_CLASS_MATRIX)
        {
            if (!(c = hlsl_new_uint_constant(ctx, i, &var->loc)))
                return;
            list_add_after(&cast->entry, &c->entry);

            if (!(store = hlsl_new_store_index(ctx, &lhs->src, c, cast, 0, &var->loc)))
                return;
            list_add_after(&c->entry, &store->entry);
        }
        else
        {
            VKD3D_ASSERT(i == 0);

            if (!(store = hlsl_new_store_index(ctx, &lhs->src, NULL, cast, 0, &var->loc)))
                return;
            list_add_after(&cast->entry, &store->entry);
        }
    }
}

static void prepend_input_copy_recurse(struct hlsl_ctx *ctx,
        struct hlsl_ir_function_decl *func, struct hlsl_ir_load *lhs, uint32_t modifiers,
        struct hlsl_semantic *semantic, uint32_t semantic_index, bool force_align)
{
    struct vkd3d_shader_location *loc = &lhs->node.loc;
    struct hlsl_type *type = lhs->node.data_type;
    struct hlsl_ir_var *var = lhs->src.var;
    struct hlsl_ir_node *c;
    unsigned int i;

    if (type->class == HLSL_CLASS_ARRAY || type->class == HLSL_CLASS_STRUCT)
    {
        struct hlsl_ir_load *element_load;
        struct hlsl_struct_field *field;
        uint32_t elem_semantic_index;

        for (i = 0; i < hlsl_type_element_count(type); ++i)
        {
            uint32_t element_modifiers;

            if (type->class == HLSL_CLASS_ARRAY)
            {
                elem_semantic_index = semantic_index
                        + i * hlsl_type_get_array_element_reg_size(type->e.array.type, HLSL_REGSET_NUMERIC) / 4;
                element_modifiers = modifiers;
                force_align = true;
            }
            else
            {
                field = &type->e.record.fields[i];
                if (hlsl_type_is_resource(field->type))
                {
                    hlsl_fixme(ctx, &field->loc, "Prepend uniform copies for resource components within structs.");
                    continue;
                }
                validate_field_semantic(ctx, field);
                semantic = &field->semantic;
                elem_semantic_index = semantic->index;
                loc = &field->loc;
                element_modifiers = combine_field_storage_modifiers(modifiers, field->storage_modifiers);
                force_align = (i == 0);
            }

            if (!(c = hlsl_new_uint_constant(ctx, i, &var->loc)))
                return;
            list_add_after(&lhs->node.entry, &c->entry);

            /* This redundant load is expected to be deleted later by DCE. */
            if (!(element_load = hlsl_new_load_index(ctx, &lhs->src, c, loc)))
                return;
            list_add_after(&c->entry, &element_load->node.entry);

            prepend_input_copy_recurse(ctx, func, element_load, element_modifiers,
                    semantic, elem_semantic_index, force_align);
        }
    }
    else
    {
        prepend_input_copy(ctx, func, lhs, modifiers, semantic, semantic_index, force_align);
    }
}

/* Split inputs into two variables representing the semantic and temp registers,
 * and copy the former to the latter, so that writes to input variables work. */
static void prepend_input_var_copy(struct hlsl_ctx *ctx, struct hlsl_ir_function_decl *func, struct hlsl_ir_var *var)
{
    struct hlsl_ir_load *load;

    /* This redundant load is expected to be deleted later by DCE. */
    if (!(load = hlsl_new_var_load(ctx, var, &var->loc)))
        return;
    list_add_head(&func->body.instrs, &load->node.entry);

    prepend_input_copy_recurse(ctx, func, load, var->storage_modifiers, &var->semantic, var->semantic.index, false);
}

static void append_output_copy(struct hlsl_ctx *ctx, struct hlsl_ir_function_decl *func,
        struct hlsl_ir_load *rhs, uint32_t modifiers,
        struct hlsl_semantic *semantic, uint32_t semantic_index, bool force_align)
{
    struct hlsl_type *type = rhs->node.data_type, *vector_type;
    struct vkd3d_shader_location *loc = &rhs->node.loc;
    struct hlsl_ir_var *var = rhs->src.var;
    struct hlsl_ir_node *c;
    unsigned int i;

    if (!hlsl_is_numeric_type(type))
    {
        struct vkd3d_string_buffer *string;
        if (!(string = hlsl_type_to_string(ctx, type)))
            return;
        hlsl_fixme(ctx, &var->loc, "Output semantics for type %s.", string->buffer);
        hlsl_release_string_buffer(ctx, string);
    }
    if (!semantic->name)
        return;

    vector_type = hlsl_get_vector_type(ctx, type->e.numeric.type, hlsl_type_minor_size(type));

    if (hlsl_type_major_size(type) > 1)
        force_align = true;

    for (i = 0; i < hlsl_type_major_size(type); ++i)
    {
        struct hlsl_ir_node *store;
        struct hlsl_ir_var *output;
        struct hlsl_ir_load *load;

        if (!(output = add_semantic_var(ctx, func, var, vector_type,
                modifiers, semantic, semantic_index + i, true, force_align, loc)))
            return;

        if (type->class == HLSL_CLASS_MATRIX)
        {
            if (!(c = hlsl_new_uint_constant(ctx, i, &var->loc)))
                return;
            hlsl_block_add_instr(&func->body, c);

            if (!(load = hlsl_new_load_index(ctx, &rhs->src, c, &var->loc)))
                return;
            hlsl_block_add_instr(&func->body, &load->node);
        }
        else
        {
            VKD3D_ASSERT(i == 0);

            if (!(load = hlsl_new_load_index(ctx, &rhs->src, NULL, &var->loc)))
                return;
            hlsl_block_add_instr(&func->body, &load->node);
        }

        if (!(store = hlsl_new_simple_store(ctx, output, &load->node)))
            return;
        hlsl_block_add_instr(&func->body, store);
    }
}

static void append_output_copy_recurse(struct hlsl_ctx *ctx,
        struct hlsl_ir_function_decl *func, struct hlsl_ir_load *rhs, uint32_t modifiers,
        struct hlsl_semantic *semantic, uint32_t semantic_index, bool force_align)
{
    struct vkd3d_shader_location *loc = &rhs->node.loc;
    struct hlsl_type *type = rhs->node.data_type;
    struct hlsl_ir_var *var = rhs->src.var;
    struct hlsl_ir_node *c;
    unsigned int i;

    if (type->class == HLSL_CLASS_ARRAY || type->class == HLSL_CLASS_STRUCT)
    {
        struct hlsl_ir_load *element_load;
        struct hlsl_struct_field *field;
        uint32_t elem_semantic_index;

        for (i = 0; i < hlsl_type_element_count(type); ++i)
        {
            uint32_t element_modifiers;

            if (type->class == HLSL_CLASS_ARRAY)
            {
                elem_semantic_index = semantic_index
                        + i * hlsl_type_get_array_element_reg_size(type->e.array.type, HLSL_REGSET_NUMERIC) / 4;
                element_modifiers = modifiers;
                force_align = true;
            }
            else
            {
                field = &type->e.record.fields[i];
                if (hlsl_type_is_resource(field->type))
                    continue;
                validate_field_semantic(ctx, field);
                semantic = &field->semantic;
                elem_semantic_index = semantic->index;
                loc = &field->loc;
                element_modifiers = combine_field_storage_modifiers(modifiers, field->storage_modifiers);
                force_align = (i == 0);
            }

            if (!(c = hlsl_new_uint_constant(ctx, i, &var->loc)))
                return;
            hlsl_block_add_instr(&func->body, c);

            if (!(element_load = hlsl_new_load_index(ctx, &rhs->src, c, loc)))
                return;
            hlsl_block_add_instr(&func->body, &element_load->node);

            append_output_copy_recurse(ctx, func, element_load, element_modifiers,
                    semantic, elem_semantic_index, force_align);
        }
    }
    else
    {
        append_output_copy(ctx, func, rhs, modifiers, semantic, semantic_index, force_align);
    }
}

/* Split outputs into two variables representing the temp and semantic
 * registers, and copy the former to the latter, so that reads from output
 * variables work. */
static void append_output_var_copy(struct hlsl_ctx *ctx, struct hlsl_ir_function_decl *func, struct hlsl_ir_var *var)
{
    struct hlsl_ir_load *load;

    /* This redundant load is expected to be deleted later by DCE. */
    if (!(load = hlsl_new_var_load(ctx, var, &var->loc)))
        return;
    hlsl_block_add_instr(&func->body, &load->node);

    append_output_copy_recurse(ctx, func, load, var->storage_modifiers, &var->semantic, var->semantic.index, false);
}

bool hlsl_transform_ir(struct hlsl_ctx *ctx, bool (*func)(struct hlsl_ctx *ctx, struct hlsl_ir_node *, void *),
        struct hlsl_block *block, void *context)
{
    struct hlsl_ir_node *instr, *next;
    bool progress = false;

    LIST_FOR_EACH_ENTRY_SAFE(instr, next, &block->instrs, struct hlsl_ir_node, entry)
    {
        if (instr->type == HLSL_IR_IF)
        {
            struct hlsl_ir_if *iff = hlsl_ir_if(instr);

            progress |= hlsl_transform_ir(ctx, func, &iff->then_block, context);
            progress |= hlsl_transform_ir(ctx, func, &iff->else_block, context);
        }
        else if (instr->type == HLSL_IR_LOOP)
        {
            progress |= hlsl_transform_ir(ctx, func, &hlsl_ir_loop(instr)->body, context);
        }
        else if (instr->type == HLSL_IR_SWITCH)
        {
            struct hlsl_ir_switch *s = hlsl_ir_switch(instr);
            struct hlsl_ir_switch_case *c;

            LIST_FOR_EACH_ENTRY(c, &s->cases, struct hlsl_ir_switch_case, entry)
            {
                progress |= hlsl_transform_ir(ctx, func, &c->body, context);
            }
        }

        progress |= func(ctx, instr, context);
    }

    return progress;
}

typedef bool (*PFN_lower_func)(struct hlsl_ctx *, struct hlsl_ir_node *, struct hlsl_block *);

static bool call_lower_func(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, void *context)
{
    PFN_lower_func func = context;
    struct hlsl_block block;

    hlsl_block_init(&block);
    if (func(ctx, instr, &block))
    {
        struct hlsl_ir_node *replacement = LIST_ENTRY(list_tail(&block.instrs), struct hlsl_ir_node, entry);

        list_move_before(&instr->entry, &block.instrs);
        hlsl_replace_node(instr, replacement);
        return true;
    }
    else
    {
        hlsl_block_cleanup(&block);
        return false;
    }
}

/* Specific form of transform_ir() for passes which convert a single instruction
 * to a block of one or more instructions. This helper takes care of setting up
 * the block and calling hlsl_replace_node_with_block(). */
static bool lower_ir(struct hlsl_ctx *ctx, PFN_lower_func func, struct hlsl_block *block)
{
    return hlsl_transform_ir(ctx, call_lower_func, block, func);
}

static bool transform_instr_derefs(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, void *context)
{
    bool res;
    bool (*func)(struct hlsl_ctx *ctx, struct hlsl_deref *, struct hlsl_ir_node *) = context;

    switch(instr->type)
    {
        case HLSL_IR_LOAD:
            res = func(ctx, &hlsl_ir_load(instr)->src, instr);
            return res;

        case HLSL_IR_STORE:
            res = func(ctx, &hlsl_ir_store(instr)->lhs, instr);
            return res;

        case HLSL_IR_RESOURCE_LOAD:
            res = func(ctx, &hlsl_ir_resource_load(instr)->resource, instr);
            if (hlsl_ir_resource_load(instr)->sampler.var)
                res |= func(ctx, &hlsl_ir_resource_load(instr)->sampler, instr);
            return res;

        case HLSL_IR_RESOURCE_STORE:
            res = func(ctx, &hlsl_ir_resource_store(instr)->resource, instr);
            return res;

        default:
            return false;
    }
    return false;
}

static bool transform_derefs(struct hlsl_ctx *ctx,
        bool (*func)(struct hlsl_ctx *ctx, struct hlsl_deref *, struct hlsl_ir_node *),
        struct hlsl_block *block)
{
    return hlsl_transform_ir(ctx, transform_instr_derefs, block, func);
}

struct recursive_call_ctx
{
    const struct hlsl_ir_function_decl **backtrace;
    size_t count, capacity;
};

static bool find_recursive_calls(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, void *context)
{
    struct recursive_call_ctx *call_ctx = context;
    struct hlsl_ir_function_decl *decl;
    const struct hlsl_ir_call *call;
    size_t i;

    if (instr->type != HLSL_IR_CALL)
        return false;
    call = hlsl_ir_call(instr);
    decl = call->decl;

    for (i = 0; i < call_ctx->count; ++i)
    {
        if (call_ctx->backtrace[i] == decl)
        {
            hlsl_error(ctx, &call->node.loc, VKD3D_SHADER_ERROR_HLSL_RECURSIVE_CALL,
                    "Recursive call to \"%s\".", decl->func->name);
            /* Native returns E_NOTIMPL instead of E_FAIL here. */
            ctx->result = VKD3D_ERROR_NOT_IMPLEMENTED;
            return false;
        }
    }

    if (!hlsl_array_reserve(ctx, (void **)&call_ctx->backtrace, &call_ctx->capacity,
            call_ctx->count + 1, sizeof(*call_ctx->backtrace)))
        return false;
    call_ctx->backtrace[call_ctx->count++] = decl;

    hlsl_transform_ir(ctx, find_recursive_calls, &decl->body, call_ctx);

    --call_ctx->count;

    return false;
}

static void insert_early_return_break(struct hlsl_ctx *ctx,
        struct hlsl_ir_function_decl *func, struct hlsl_ir_node *cf_instr)
{
    struct hlsl_ir_node *iff, *jump;
    struct hlsl_block then_block;
    struct hlsl_ir_load *load;

    hlsl_block_init(&then_block);

    if (!(load = hlsl_new_var_load(ctx, func->early_return_var, &cf_instr->loc)))
        return;
    list_add_after(&cf_instr->entry, &load->node.entry);

    if (!(jump = hlsl_new_jump(ctx, HLSL_IR_JUMP_BREAK, NULL, &cf_instr->loc)))
        return;
    hlsl_block_add_instr(&then_block, jump);

    if (!(iff = hlsl_new_if(ctx, &load->node, &then_block, NULL, &cf_instr->loc)))
        return;
    list_add_after(&load->node.entry, &iff->entry);
}

/* Remove HLSL_IR_JUMP_RETURN calls by altering subsequent control flow. */
static bool lower_return(struct hlsl_ctx *ctx, struct hlsl_ir_function_decl *func,
        struct hlsl_block *block, bool in_loop)
{
    struct hlsl_ir_node *return_instr = NULL, *cf_instr = NULL;
    struct hlsl_ir_node *instr, *next;
    bool has_early_return = false;

    /* SM1 has no function calls. SM4 does, but native d3dcompiler inlines
     * everything anyway. We are safest following suit.
     *
     * The basic idea is to keep track of whether the function has executed an
     * early return in a synthesized boolean variable (func->early_return_var)
     * and guard all code after the return on that variable being false. In the
     * case of loops we also replace the return with a break.
     *
     * The following algorithm loops over instructions in a block, recursing
     * into inferior CF blocks, until it hits one of the following two things:
     *
     * - A return statement. In this case, we remove everything after the return
     *   statement in this block. We have to stop and do this in a separate
     *   loop, because instructions must be deleted in reverse order (due to
     *   def-use chains.)
     *
     *   If we're inside of a loop CF block, we can instead just turn the
     *   return into a break, which offers the right semantics—except that it
     *   won't break out of nested loops.
     *
     * - A CF block which contains a return statement. After calling
     *   lower_return() on the CF block body, we stop, pull out everything after
     *   the CF instruction, shove it into an if block, and then lower that if
     *   block.
     *
     *   (We could return a "did we make progress" boolean like hlsl_transform_ir()
     *   and run this pass multiple times, but we already know the only block
     *   that still needs to be addressed, so there's not much point.)
     *
     *   If we're inside of a loop CF block, we again do things differently. We
     *   already turned any returns into breaks. If the block we just processed
     *   was conditional, then "break" did our work for us. If it was a loop,
     *   we need to propagate that break to the outer loop.
     *
     * We return true if there was an early return anywhere in the block we just
     * processed (including CF contained inside that block).
     */

    LIST_FOR_EACH_ENTRY_SAFE(instr, next, &block->instrs, struct hlsl_ir_node, entry)
    {
        if (instr->type == HLSL_IR_CALL)
        {
            struct hlsl_ir_call *call = hlsl_ir_call(instr);

            lower_return(ctx, call->decl, &call->decl->body, false);
        }
        else if (instr->type == HLSL_IR_IF)
        {
            struct hlsl_ir_if *iff = hlsl_ir_if(instr);

            has_early_return |= lower_return(ctx, func, &iff->then_block, in_loop);
            has_early_return |= lower_return(ctx, func, &iff->else_block, in_loop);

            if (has_early_return)
            {
                /* If we're in a loop, we don't need to do anything here. We
                 * turned the return into a break, and that will already skip
                 * anything that comes after this "if" block. */
                if (!in_loop)
                {
                    cf_instr = instr;
                    break;
                }
            }
        }
        else if (instr->type == HLSL_IR_LOOP)
        {
            has_early_return |= lower_return(ctx, func, &hlsl_ir_loop(instr)->body, true);

            if (has_early_return)
            {
                if (in_loop)
                {
                    /* "instr" is a nested loop. "return" breaks out of all
                     * loops, so break out of this one too now. */
                    insert_early_return_break(ctx, func, instr);
                }
                else
                {
                    cf_instr = instr;
                    break;
                }
            }
        }
        else if (instr->type == HLSL_IR_JUMP)
        {
            struct hlsl_ir_jump *jump = hlsl_ir_jump(instr);
            struct hlsl_ir_node *constant, *store;

            if (jump->type == HLSL_IR_JUMP_RETURN)
            {
                if (!(constant = hlsl_new_bool_constant(ctx, true, &jump->node.loc)))
                    return false;
                list_add_before(&jump->node.entry, &constant->entry);

                if (!(store = hlsl_new_simple_store(ctx, func->early_return_var, constant)))
                    return false;
                list_add_after(&constant->entry, &store->entry);

                has_early_return = true;
                if (in_loop)
                {
                    jump->type = HLSL_IR_JUMP_BREAK;
                }
                else
                {
                    return_instr = instr;
                    break;
                }
            }
        }
        else if (instr->type == HLSL_IR_SWITCH)
        {
            struct hlsl_ir_switch *s = hlsl_ir_switch(instr);
            struct hlsl_ir_switch_case *c;

            LIST_FOR_EACH_ENTRY(c, &s->cases, struct hlsl_ir_switch_case, entry)
            {
                has_early_return |= lower_return(ctx, func, &c->body, true);
            }

            if (has_early_return)
            {
                if (in_loop)
                {
                    /* For a 'switch' nested in a loop append a break after the 'switch'. */
                    insert_early_return_break(ctx, func, instr);
                }
                else
                {
                    cf_instr = instr;
                    break;
                }
            }
        }
    }

    if (return_instr)
    {
        /* If we're in a loop, we should have used "break" instead. */
        VKD3D_ASSERT(!in_loop);

        /* Iterate in reverse, to avoid use-after-free when unlinking sources from
         * the "uses" list. */
        LIST_FOR_EACH_ENTRY_SAFE_REV(instr, next, &block->instrs, struct hlsl_ir_node, entry)
        {
            list_remove(&instr->entry);
            hlsl_free_instr(instr);

            /* Yes, we just freed it, but we're comparing pointers. */
            if (instr == return_instr)
                break;
        }
    }
    else if (cf_instr)
    {
        struct list *tail = list_tail(&block->instrs);
        struct hlsl_ir_node *not, *iff;
        struct hlsl_block then_block;
        struct hlsl_ir_load *load;

        /* If we're in a loop, we should have used "break" instead. */
        VKD3D_ASSERT(!in_loop);

        if (tail == &cf_instr->entry)
            return has_early_return;

        hlsl_block_init(&then_block);
        list_move_slice_tail(&then_block.instrs, list_next(&block->instrs, &cf_instr->entry), tail);
        lower_return(ctx, func, &then_block, in_loop);

        if (!(load = hlsl_new_var_load(ctx, func->early_return_var, &cf_instr->loc)))
            return false;
        hlsl_block_add_instr(block, &load->node);

        if (!(not = hlsl_new_unary_expr(ctx, HLSL_OP1_LOGIC_NOT, &load->node, &cf_instr->loc)))
            return false;
        hlsl_block_add_instr(block, not);

        if (!(iff = hlsl_new_if(ctx, not, &then_block, NULL, &cf_instr->loc)))
            return false;
        list_add_tail(&block->instrs, &iff->entry);
    }

    return has_early_return;
}

/* Remove HLSL_IR_CALL instructions by inlining them. */
static bool lower_calls(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, void *context)
{
    const struct hlsl_ir_function_decl *decl;
    struct hlsl_ir_call *call;
    struct hlsl_block block;

    if (instr->type != HLSL_IR_CALL)
        return false;
    call = hlsl_ir_call(instr);
    decl = call->decl;

    if (!decl->has_body)
        hlsl_error(ctx, &call->node.loc, VKD3D_SHADER_ERROR_HLSL_NOT_DEFINED,
                "Function \"%s\" is not defined.", decl->func->name);

    if (!hlsl_clone_block(ctx, &block, &decl->body))
        return false;
    list_move_before(&call->node.entry, &block.instrs);

    list_remove(&call->node.entry);
    hlsl_free_instr(&call->node);
    return true;
}

static struct hlsl_ir_node *add_zero_mipmap_level(struct hlsl_ctx *ctx, struct hlsl_ir_node *index,
        const struct vkd3d_shader_location *loc)
{
    unsigned int dim_count = index->data_type->dimx;
    struct hlsl_ir_node *store, *zero;
    struct hlsl_ir_load *coords_load;
    struct hlsl_deref coords_deref;
    struct hlsl_ir_var *coords;

    VKD3D_ASSERT(dim_count < 4);

    if (!(coords = hlsl_new_synthetic_var(ctx, "coords",
            hlsl_get_vector_type(ctx, HLSL_TYPE_UINT, dim_count + 1), loc)))
        return NULL;

    hlsl_init_simple_deref_from_var(&coords_deref, coords);
    if (!(store = hlsl_new_store_index(ctx, &coords_deref, NULL, index, (1u << dim_count) - 1, loc)))
        return NULL;
    list_add_after(&index->entry, &store->entry);

    if (!(zero = hlsl_new_uint_constant(ctx, 0, loc)))
        return NULL;
    list_add_after(&store->entry, &zero->entry);

    if (!(store = hlsl_new_store_index(ctx, &coords_deref, NULL, zero, 1u << dim_count, loc)))
        return NULL;
    list_add_after(&zero->entry, &store->entry);

    if (!(coords_load = hlsl_new_var_load(ctx, coords, loc)))
        return NULL;
    list_add_after(&store->entry, &coords_load->node.entry);

    return &coords_load->node;
}

/* hlsl_ir_swizzle nodes that directly point to a matrix value are only a parse-time construct that
 * represents matrix swizzles (e.g. mat._m01_m23) before we know if they will be used in the lhs of
 * an assignment or as a value made from different components of the matrix. The former cases should
 * have already been split into several separate assignments, but the latter are lowered by this
 * pass. */
static bool lower_matrix_swizzles(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, struct hlsl_block *block)
{
    struct hlsl_ir_swizzle *swizzle;
    struct hlsl_ir_load *var_load;
    struct hlsl_deref var_deref;
    struct hlsl_type *matrix_type;
    struct hlsl_ir_var *var;
    unsigned int x, y, k, i;

    if (instr->type != HLSL_IR_SWIZZLE)
        return false;
    swizzle = hlsl_ir_swizzle(instr);
    matrix_type = swizzle->val.node->data_type;
    if (matrix_type->class != HLSL_CLASS_MATRIX)
        return false;

    if (!(var = hlsl_new_synthetic_var(ctx, "matrix-swizzle", instr->data_type, &instr->loc)))
        return false;
    hlsl_init_simple_deref_from_var(&var_deref, var);

    for (i = 0; i < instr->data_type->dimx; ++i)
    {
        struct hlsl_block store_block;
        struct hlsl_ir_node *load;

        y = (swizzle->swizzle >> (8 * i + 4)) & 0xf;
        x = (swizzle->swizzle >> 8 * i) & 0xf;
        k = y * matrix_type->dimx + x;

        if (!(load = hlsl_add_load_component(ctx, block, swizzle->val.node, k, &instr->loc)))
            return false;

        if (!hlsl_new_store_component(ctx, &store_block, &var_deref, i, load))
            return false;
        hlsl_block_add_block(block, &store_block);
    }

    if (!(var_load = hlsl_new_var_load(ctx, var, &instr->loc)))
        return false;
    hlsl_block_add_instr(block, &var_load->node);

    return true;
}

/* hlsl_ir_index nodes are a parse-time construct used to represent array indexing and struct
 * record access before knowing if they will be used in the lhs of an assignment --in which case
 * they are lowered into a deref-- or as the load of an element within a larger value.
 * For the latter case, this pass takes care of lowering hlsl_ir_indexes into individual
 * hlsl_ir_loads, or individual hlsl_ir_resource_loads, in case the indexing is a
 * resource access. */
static bool lower_index_loads(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, struct hlsl_block *block)
{
    struct hlsl_ir_node *val, *store;
    struct hlsl_deref var_deref;
    struct hlsl_ir_index *index;
    struct hlsl_ir_load *load;
    struct hlsl_ir_var *var;

    if (instr->type != HLSL_IR_INDEX)
        return false;
    index = hlsl_ir_index(instr);
    val = index->val.node;

    if (hlsl_index_is_resource_access(index))
    {
        unsigned int dim_count = hlsl_sampler_dim_count(val->data_type->sampler_dim);
        struct hlsl_ir_node *coords = index->idx.node;
        struct hlsl_resource_load_params params = {0};
        struct hlsl_ir_node *resource_load;

        VKD3D_ASSERT(coords->data_type->class == HLSL_CLASS_VECTOR);
        VKD3D_ASSERT(coords->data_type->e.numeric.type == HLSL_TYPE_UINT);
        VKD3D_ASSERT(coords->data_type->dimx == dim_count);

        if (!(coords = add_zero_mipmap_level(ctx, coords, &instr->loc)))
            return false;

        params.type = HLSL_RESOURCE_LOAD;
        params.resource = val;
        params.coords = coords;
        params.format = val->data_type->e.resource.format;

        if (!(resource_load = hlsl_new_resource_load(ctx, &params, &instr->loc)))
            return false;
        hlsl_block_add_instr(block, resource_load);
        return true;
    }

    if (!(var = hlsl_new_synthetic_var(ctx, "index-val", val->data_type, &instr->loc)))
        return false;
    hlsl_init_simple_deref_from_var(&var_deref, var);

    if (!(store = hlsl_new_simple_store(ctx, var, val)))
        return false;
    hlsl_block_add_instr(block, store);

    if (hlsl_index_is_noncontiguous(index))
    {
        struct hlsl_ir_node *mat = index->val.node;
        struct hlsl_deref row_deref;
        unsigned int i;

        VKD3D_ASSERT(!hlsl_type_is_row_major(mat->data_type));

        if (!(var = hlsl_new_synthetic_var(ctx, "row", instr->data_type, &instr->loc)))
            return false;
        hlsl_init_simple_deref_from_var(&row_deref, var);

        for (i = 0; i < mat->data_type->dimx; ++i)
        {
            struct hlsl_ir_node *c;

            if (!(c = hlsl_new_uint_constant(ctx, i, &instr->loc)))
                return false;
            hlsl_block_add_instr(block, c);

            if (!(load = hlsl_new_load_index(ctx, &var_deref, c, &instr->loc)))
                return false;
            hlsl_block_add_instr(block, &load->node);

            if (!(load = hlsl_new_load_index(ctx, &load->src, index->idx.node, &instr->loc)))
                return false;
            hlsl_block_add_instr(block, &load->node);

            if (!(store = hlsl_new_store_index(ctx, &row_deref, c, &load->node, 0, &instr->loc)))
                return false;
            hlsl_block_add_instr(block, store);
        }

        if (!(load = hlsl_new_var_load(ctx, var, &instr->loc)))
            return false;
        hlsl_block_add_instr(block, &load->node);
    }
    else
    {
        if (!(load = hlsl_new_load_index(ctx, &var_deref, index->idx.node, &instr->loc)))
            return false;
        hlsl_block_add_instr(block, &load->node);
    }
    return true;
}

/* Lower casts from vec1 to vecN to swizzles. */
static bool lower_broadcasts(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, struct hlsl_block *block)
{
    const struct hlsl_type *src_type, *dst_type;
    struct hlsl_type *dst_scalar_type;
    struct hlsl_ir_expr *cast;

    if (instr->type != HLSL_IR_EXPR)
        return false;
    cast = hlsl_ir_expr(instr);
    if (cast->op != HLSL_OP1_CAST)
        return false;
    src_type = cast->operands[0].node->data_type;
    dst_type = cast->node.data_type;

    if (src_type->class <= HLSL_CLASS_VECTOR && dst_type->class <= HLSL_CLASS_VECTOR && src_type->dimx == 1)
    {
        struct hlsl_ir_node *new_cast, *swizzle;

        dst_scalar_type = hlsl_get_scalar_type(ctx, dst_type->e.numeric.type);
        /* We need to preserve the cast since it might be doing more than just
         * turning the scalar into a vector. */
        if (!(new_cast = hlsl_new_cast(ctx, cast->operands[0].node, dst_scalar_type, &cast->node.loc)))
            return false;
        hlsl_block_add_instr(block, new_cast);

        if (dst_type->dimx != 1)
        {
            if (!(swizzle = hlsl_new_swizzle(ctx, HLSL_SWIZZLE(X, X, X, X), dst_type->dimx, new_cast, &cast->node.loc)))
                return false;
            hlsl_block_add_instr(block, swizzle);
        }

        return true;
    }

    return false;
}

/* Allocate a unique, ordered index to each instruction, which will be used for
 * copy propagation and computing liveness ranges.
 * Index 0 means unused; index 1 means function entry, so start at 2. */
static unsigned int index_instructions(struct hlsl_block *block, unsigned int index)
{
    struct hlsl_ir_node *instr;

    LIST_FOR_EACH_ENTRY(instr, &block->instrs, struct hlsl_ir_node, entry)
    {
        instr->index = index++;

        if (instr->type == HLSL_IR_IF)
        {
            struct hlsl_ir_if *iff = hlsl_ir_if(instr);
            index = index_instructions(&iff->then_block, index);
            index = index_instructions(&iff->else_block, index);
        }
        else if (instr->type == HLSL_IR_LOOP)
        {
            index = index_instructions(&hlsl_ir_loop(instr)->body, index);
            hlsl_ir_loop(instr)->next_index = index;
        }
        else if (instr->type == HLSL_IR_SWITCH)
        {
            struct hlsl_ir_switch *s = hlsl_ir_switch(instr);
            struct hlsl_ir_switch_case *c;

            LIST_FOR_EACH_ENTRY(c, &s->cases, struct hlsl_ir_switch_case, entry)
            {
                index = index_instructions(&c->body, index);
            }
        }
    }

    return index;
}

/*
 * Copy propagation. The basic idea is to recognize instruction sequences of the
 * form:
 *
 *   2: <any instruction>
 *   3: v = @2
 *   4: load(v)
 *
 * and replace the load (@4) with the original instruction (@2).
 * This works for multiple components, even if they're written using separate
 * store instructions, as long as the rhs is the same in every case. This basic
 * detection is implemented by copy_propagation_replace_with_single_instr().
 *
 * In some cases, the load itself might not have a single source, but a
 * subsequent swizzle might; hence we also try to replace swizzles of loads.
 *
 * We use the same infrastructure to implement a more specialized
 * transformation. We recognize sequences of the form:
 *
 *   2: 123
 *   3: var.x = @2
 *   4: 345
 *   5: var.y = @4
 *   6: load(var.xy)
 *
 * where the load (@6) originates from different sources but that are constant,
 * and transform it into a single constant vector. This latter pass is done
 * by copy_propagation_replace_with_constant_vector().
 *
 * This is a specialized form of vectorization, and begs the question: why does
 * the load need to be involved? Can we just vectorize the stores into a single
 * instruction, and then use "normal" copy-prop to convert that into a single
 * vector?
 *
 * In general, the answer is yes, but there is a special case which necessitates
 * the use of this transformation: non-uniform control flow. Copy-prop can act
 * across some control flow, and in cases like the following:
 *
 *   2: 123
 *   3: var.x = @2
 *   4: if (...)
 *   5:    456
 *   6:    var.y = @5
 *   7:    load(var.xy)
 *
 * we can copy-prop the load (@7) into a constant vector {123, 456}, but we
 * cannot easily vectorize the stores @3 and @6.
 */

struct copy_propagation_value
{
    unsigned int timestamp;
    /* If node is NULL, the value was dynamically written and thus, it is unknown.*/
    struct hlsl_ir_node *node;
    unsigned int component;
};

struct copy_propagation_component_trace
{
    struct copy_propagation_value *records;
    size_t record_count, record_capacity;
};

struct copy_propagation_var_def
{
    struct rb_entry entry;
    struct hlsl_ir_var *var;
    struct copy_propagation_component_trace traces[];
};

struct copy_propagation_state
{
    struct rb_tree var_defs;
    struct copy_propagation_state *parent;
};

static int copy_propagation_var_def_compare(const void *key, const struct rb_entry *entry)
{
    struct copy_propagation_var_def *var_def = RB_ENTRY_VALUE(entry, struct copy_propagation_var_def, entry);
    uintptr_t key_int = (uintptr_t)key, entry_int = (uintptr_t)var_def->var;

    return (key_int > entry_int) - (key_int < entry_int);
}

static void copy_propagation_var_def_destroy(struct rb_entry *entry, void *context)
{
    struct copy_propagation_var_def *var_def = RB_ENTRY_VALUE(entry, struct copy_propagation_var_def, entry);
    unsigned int component_count = hlsl_type_component_count(var_def->var->data_type);
    unsigned int i;

    for (i = 0; i < component_count; ++i)
        vkd3d_free(var_def->traces[i].records);
    vkd3d_free(var_def);
}

static struct copy_propagation_value *copy_propagation_get_value_at_time(
        struct copy_propagation_component_trace *trace, unsigned int time)
{
    int r;

    for (r = trace->record_count - 1; r >= 0; --r)
    {
        if (trace->records[r].timestamp < time)
            return &trace->records[r];
    }

    return NULL;
}

static struct copy_propagation_value *copy_propagation_get_value(const struct copy_propagation_state *state,
        const struct hlsl_ir_var *var, unsigned int component, unsigned int time)
{
    for (; state; state = state->parent)
    {
        struct rb_entry *entry = rb_get(&state->var_defs, var);
        if (entry)
        {
            struct copy_propagation_var_def *var_def = RB_ENTRY_VALUE(entry, struct copy_propagation_var_def, entry);
            unsigned int component_count = hlsl_type_component_count(var->data_type);
            struct copy_propagation_value *value;

            VKD3D_ASSERT(component < component_count);
            value = copy_propagation_get_value_at_time(&var_def->traces[component], time);

            if (!value)
                continue;

            if (value->node)
                return value;
            else
                return NULL;
        }
    }

    return NULL;
}

static struct copy_propagation_var_def *copy_propagation_create_var_def(struct hlsl_ctx *ctx,
        struct copy_propagation_state *state, struct hlsl_ir_var *var)
{
    struct rb_entry *entry = rb_get(&state->var_defs, var);
    struct copy_propagation_var_def *var_def;
    unsigned int component_count = hlsl_type_component_count(var->data_type);
    int res;

    if (entry)
        return RB_ENTRY_VALUE(entry, struct copy_propagation_var_def, entry);

    if (!(var_def = hlsl_alloc(ctx, offsetof(struct copy_propagation_var_def, traces[component_count]))))
        return NULL;

    var_def->var = var;

    res = rb_put(&state->var_defs, var, &var_def->entry);
    VKD3D_ASSERT(!res);

    return var_def;
}

static void copy_propagation_trace_record_value(struct hlsl_ctx *ctx,
        struct copy_propagation_component_trace *trace, struct hlsl_ir_node *node,
        unsigned int component, unsigned int time)
{
    VKD3D_ASSERT(!trace->record_count || trace->records[trace->record_count - 1].timestamp < time);

    if (!hlsl_array_reserve(ctx, (void **)&trace->records, &trace->record_capacity,
            trace->record_count + 1, sizeof(trace->records[0])))
        return;

    trace->records[trace->record_count].timestamp = time;
    trace->records[trace->record_count].node = node;
    trace->records[trace->record_count].component = component;

    ++trace->record_count;
}

static void copy_propagation_invalidate_variable(struct hlsl_ctx *ctx, struct copy_propagation_var_def *var_def,
        unsigned int comp, unsigned char writemask, unsigned int time)
{
    unsigned i;

    TRACE("Invalidate variable %s[%u]%s.\n", var_def->var->name, comp, debug_hlsl_writemask(writemask));

    for (i = 0; i < 4; ++i)
    {
        if (writemask & (1u << i))
        {
            struct copy_propagation_component_trace *trace = &var_def->traces[comp + i];

            /* Don't add an invalidate record if it is already present. */
            if (trace->record_count && trace->records[trace->record_count - 1].timestamp == time)
            {
                VKD3D_ASSERT(!trace->records[trace->record_count - 1].node);
                continue;
            }

            copy_propagation_trace_record_value(ctx, trace, NULL, 0, time);
        }
    }
}

static void copy_propagation_invalidate_variable_from_deref_recurse(struct hlsl_ctx *ctx,
        struct copy_propagation_var_def *var_def, const struct hlsl_deref *deref,
        struct hlsl_type *type, unsigned int depth, unsigned int comp_start, unsigned char writemask,
        unsigned int time)
{
    unsigned int i, subtype_comp_count;
    struct hlsl_ir_node *path_node;
    struct hlsl_type *subtype;

    if (depth == deref->path_len)
    {
        copy_propagation_invalidate_variable(ctx, var_def, comp_start, writemask, time);
        return;
    }

    path_node = deref->path[depth].node;
    subtype = hlsl_get_element_type_from_path_index(ctx, type, path_node);

    if (type->class == HLSL_CLASS_STRUCT)
    {
        unsigned int idx = hlsl_ir_constant(path_node)->value.u[0].u;

        for (i = 0; i < idx; ++i)
            comp_start += hlsl_type_component_count(type->e.record.fields[i].type);

        copy_propagation_invalidate_variable_from_deref_recurse(ctx, var_def, deref, subtype,
                depth + 1, comp_start, writemask, time);
    }
    else
    {
        subtype_comp_count = hlsl_type_component_count(subtype);

        if (path_node->type == HLSL_IR_CONSTANT)
        {
            copy_propagation_invalidate_variable_from_deref_recurse(ctx, var_def, deref, subtype,
                    depth + 1, hlsl_ir_constant(path_node)->value.u[0].u * subtype_comp_count,
                    writemask, time);
        }
        else
        {
            for (i = 0; i < hlsl_type_element_count(type); ++i)
            {
                copy_propagation_invalidate_variable_from_deref_recurse(ctx, var_def, deref, subtype,
                        depth + 1, i * subtype_comp_count, writemask, time);
            }
        }
    }
}

static void copy_propagation_invalidate_variable_from_deref(struct hlsl_ctx *ctx,
        struct copy_propagation_var_def *var_def, const struct hlsl_deref *deref,
        unsigned char writemask, unsigned int time)
{
    copy_propagation_invalidate_variable_from_deref_recurse(ctx, var_def, deref, deref->var->data_type,
            0, 0, writemask, time);
}

static void copy_propagation_set_value(struct hlsl_ctx *ctx, struct copy_propagation_var_def *var_def,
        unsigned int comp, unsigned char writemask, struct hlsl_ir_node *instr, unsigned int time)
{
    unsigned int i, j = 0;

    for (i = 0; i < 4; ++i)
    {
        if (writemask & (1u << i))
        {
            struct copy_propagation_component_trace *trace = &var_def->traces[comp + i];

            TRACE("Variable %s[%u] is written by instruction %p%s.\n",
                    var_def->var->name, comp + i, instr, debug_hlsl_writemask(1u << i));

            copy_propagation_trace_record_value(ctx, trace, instr, j++, time);
        }
    }
}

static bool copy_propagation_replace_with_single_instr(struct hlsl_ctx *ctx,
        const struct copy_propagation_state *state, const struct hlsl_ir_load *load,
        uint32_t swizzle, struct hlsl_ir_node *instr)
{
    const unsigned int instr_component_count = hlsl_type_component_count(instr->data_type);
    const struct hlsl_deref *deref = &load->src;
    const struct hlsl_ir_var *var = deref->var;
    struct hlsl_ir_node *new_instr = NULL;
    unsigned int time = load->node.index;
    unsigned int start, count, i;
    uint32_t ret_swizzle = 0;

    if (!hlsl_component_index_range_from_deref(ctx, deref, &start, &count))
        return false;

    for (i = 0; i < instr_component_count; ++i)
    {
        struct copy_propagation_value *value;

        if (!(value = copy_propagation_get_value(state, var, start + hlsl_swizzle_get_component(swizzle, i),
                time)))
            return false;

        if (!new_instr)
        {
            new_instr = value->node;
        }
        else if (new_instr != value->node)
        {
            TRACE("No single source for propagating load from %s[%u-%u]%s\n",
                    var->name, start, start + count, debug_hlsl_swizzle(swizzle, instr_component_count));
            return false;
        }
        ret_swizzle |= value->component << HLSL_SWIZZLE_SHIFT(i);
    }

    TRACE("Load from %s[%u-%u]%s propagated as instruction %p%s.\n",
            var->name, start, start + count, debug_hlsl_swizzle(swizzle, instr_component_count),
            new_instr, debug_hlsl_swizzle(ret_swizzle, instr_component_count));

    if (new_instr->data_type->class == HLSL_CLASS_SCALAR || new_instr->data_type->class == HLSL_CLASS_VECTOR)
    {
        struct hlsl_ir_node *swizzle_node;

        if (!(swizzle_node = hlsl_new_swizzle(ctx, ret_swizzle, instr_component_count, new_instr, &instr->loc)))
            return false;
        list_add_before(&instr->entry, &swizzle_node->entry);
        new_instr = swizzle_node;
    }

    hlsl_replace_node(instr, new_instr);
    return true;
}

static bool copy_propagation_replace_with_constant_vector(struct hlsl_ctx *ctx,
        const struct copy_propagation_state *state, const struct hlsl_ir_load *load,
        uint32_t swizzle, struct hlsl_ir_node *instr)
{
    const unsigned int instr_component_count = hlsl_type_component_count(instr->data_type);
    const struct hlsl_deref *deref = &load->src;
    const struct hlsl_ir_var *var = deref->var;
    struct hlsl_constant_value values = {0};
    unsigned int time = load->node.index;
    unsigned int start, count, i;
    struct hlsl_ir_node *cons;

    if (!hlsl_component_index_range_from_deref(ctx, deref, &start, &count))
        return false;

    for (i = 0; i < instr_component_count; ++i)
    {
        struct copy_propagation_value *value;

        if (!(value = copy_propagation_get_value(state, var, start + hlsl_swizzle_get_component(swizzle, i),
                time)) || value->node->type != HLSL_IR_CONSTANT)
            return false;

        values.u[i] = hlsl_ir_constant(value->node)->value.u[value->component];
    }

    if (!(cons = hlsl_new_constant(ctx, instr->data_type, &values, &instr->loc)))
        return false;
    list_add_before(&instr->entry, &cons->entry);

    TRACE("Load from %s[%u-%u]%s turned into a constant %p.\n",
            var->name, start, start + count, debug_hlsl_swizzle(swizzle, instr_component_count), cons);

    hlsl_replace_node(instr, cons);
    return true;
}

static bool copy_propagation_transform_load(struct hlsl_ctx *ctx,
        struct hlsl_ir_load *load, struct copy_propagation_state *state)
{
    struct hlsl_type *type = load->node.data_type;

    switch (type->class)
    {
        case HLSL_CLASS_DEPTH_STENCIL_STATE:
        case HLSL_CLASS_SCALAR:
        case HLSL_CLASS_VECTOR:
        case HLSL_CLASS_PIXEL_SHADER:
        case HLSL_CLASS_RASTERIZER_STATE:
        case HLSL_CLASS_SAMPLER:
        case HLSL_CLASS_STRING:
        case HLSL_CLASS_TEXTURE:
        case HLSL_CLASS_UAV:
        case HLSL_CLASS_VERTEX_SHADER:
        case HLSL_CLASS_COMPUTE_SHADER:
        case HLSL_CLASS_DOMAIN_SHADER:
        case HLSL_CLASS_HULL_SHADER:
        case HLSL_CLASS_RENDER_TARGET_VIEW:
        case HLSL_CLASS_DEPTH_STENCIL_VIEW:
        case HLSL_CLASS_GEOMETRY_SHADER:
        case HLSL_CLASS_BLEND_STATE:
        case HLSL_CLASS_NULL:
            break;

        case HLSL_CLASS_MATRIX:
        case HLSL_CLASS_ARRAY:
        case HLSL_CLASS_STRUCT:
            /* We can't handle complex types here.
             * They should have been already split anyway by earlier passes,
             * but they may not have been deleted yet. We can't rely on DCE to
             * solve that problem for us, since we may be called on a partial
             * block, but DCE deletes dead stores, so it needs to be able to
             * see the whole program. */
        case HLSL_CLASS_ERROR:
            return false;

        case HLSL_CLASS_CONSTANT_BUFFER:
        case HLSL_CLASS_EFFECT_GROUP:
        case HLSL_CLASS_PASS:
        case HLSL_CLASS_TECHNIQUE:
        case HLSL_CLASS_VOID:
            vkd3d_unreachable();
    }

    if (copy_propagation_replace_with_constant_vector(ctx, state, load, HLSL_SWIZZLE(X, Y, Z, W), &load->node))
        return true;

    if (copy_propagation_replace_with_single_instr(ctx, state, load, HLSL_SWIZZLE(X, Y, Z, W), &load->node))
        return true;

    return false;
}

static bool copy_propagation_transform_swizzle(struct hlsl_ctx *ctx,
        struct hlsl_ir_swizzle *swizzle, struct copy_propagation_state *state)
{
    struct hlsl_ir_load *load;

    if (swizzle->val.node->type != HLSL_IR_LOAD)
        return false;
    load = hlsl_ir_load(swizzle->val.node);

    if (copy_propagation_replace_with_constant_vector(ctx, state, load, swizzle->swizzle, &swizzle->node))
        return true;

    if (copy_propagation_replace_with_single_instr(ctx, state, load, swizzle->swizzle, &swizzle->node))
        return true;

    return false;
}

static bool copy_propagation_transform_object_load(struct hlsl_ctx *ctx,
        struct hlsl_deref *deref, struct copy_propagation_state *state, unsigned int time)
{
    struct copy_propagation_value *value;
    struct hlsl_ir_load *load;
    unsigned int start, count;

    if (!hlsl_component_index_range_from_deref(ctx, deref, &start, &count))
        return false;
    VKD3D_ASSERT(count == 1);

    if (!(value = copy_propagation_get_value(state, deref->var, start, time)))
        return false;
    VKD3D_ASSERT(value->component == 0);

    /* Only HLSL_IR_LOAD can produce an object. */
    load = hlsl_ir_load(value->node);

    /* As we are replacing the instruction's deref (with the one in the hlsl_ir_load) and not the
     * instruction itself, we won't be able to rely on the value retrieved by
     * copy_propagation_get_value() for the new deref in subsequent iterations of copy propagation.
     * This is because another value may be written to that deref between the hlsl_ir_load and
     * this instruction.
     *
     * For this reason, we only replace the new deref when it corresponds to a uniform variable,
     * which cannot be written to.
     *
     * In a valid shader, all object references must resolve statically to a single uniform object.
     * If this is the case, we can expect copy propagation on regular store/loads and the other
     * compilation passes to replace all hlsl_ir_loads with loads to uniform objects, so this
     * implementation is complete, even with this restriction.
     */
    if (!load->src.var->is_uniform)
    {
        TRACE("Ignoring load from non-uniform object variable %s\n", load->src.var->name);
        return false;
    }

    hlsl_cleanup_deref(deref);
    hlsl_copy_deref(ctx, deref, &load->src);

    return true;
}

static bool copy_propagation_transform_resource_load(struct hlsl_ctx *ctx,
        struct hlsl_ir_resource_load *load, struct copy_propagation_state *state)
{
    bool progress = false;

    progress |= copy_propagation_transform_object_load(ctx, &load->resource, state, load->node.index);
    if (load->sampler.var)
        progress |= copy_propagation_transform_object_load(ctx, &load->sampler, state, load->node.index);
    return progress;
}

static bool copy_propagation_transform_resource_store(struct hlsl_ctx *ctx,
        struct hlsl_ir_resource_store *store, struct copy_propagation_state *state)
{
    bool progress = false;

    progress |= copy_propagation_transform_object_load(ctx, &store->resource, state, store->node.index);
    return progress;
}

static void copy_propagation_record_store(struct hlsl_ctx *ctx, struct hlsl_ir_store *store,
        struct copy_propagation_state *state)
{
    struct copy_propagation_var_def *var_def;
    struct hlsl_deref *lhs = &store->lhs;
    struct hlsl_ir_var *var = lhs->var;
    unsigned int start, count;

    if (!(var_def = copy_propagation_create_var_def(ctx, state, var)))
        return;

    if (hlsl_component_index_range_from_deref(ctx, lhs, &start, &count))
    {
        unsigned int writemask = store->writemask;

        if (!hlsl_is_numeric_type(store->rhs.node->data_type))
            writemask = VKD3DSP_WRITEMASK_0;
        copy_propagation_set_value(ctx, var_def, start, writemask, store->rhs.node, store->node.index);
    }
    else
    {
        copy_propagation_invalidate_variable_from_deref(ctx, var_def, lhs, store->writemask,
                store->node.index);
    }
}

static void copy_propagation_state_init(struct hlsl_ctx *ctx, struct copy_propagation_state *state,
        struct copy_propagation_state *parent)
{
    rb_init(&state->var_defs, copy_propagation_var_def_compare);
    state->parent = parent;
}

static void copy_propagation_state_destroy(struct copy_propagation_state *state)
{
    rb_destroy(&state->var_defs, copy_propagation_var_def_destroy, NULL);
}

static void copy_propagation_invalidate_from_block(struct hlsl_ctx *ctx, struct copy_propagation_state *state,
        struct hlsl_block *block, unsigned int time)
{
    struct hlsl_ir_node *instr;

    LIST_FOR_EACH_ENTRY(instr, &block->instrs, struct hlsl_ir_node, entry)
    {
        switch (instr->type)
        {
            case HLSL_IR_STORE:
            {
                struct hlsl_ir_store *store = hlsl_ir_store(instr);
                struct copy_propagation_var_def *var_def;
                struct hlsl_deref *lhs = &store->lhs;
                struct hlsl_ir_var *var = lhs->var;

                if (!(var_def = copy_propagation_create_var_def(ctx, state, var)))
                    continue;

                copy_propagation_invalidate_variable_from_deref(ctx, var_def, lhs, store->writemask, time);

                break;
            }

            case HLSL_IR_IF:
            {
                struct hlsl_ir_if *iff = hlsl_ir_if(instr);

                copy_propagation_invalidate_from_block(ctx, state, &iff->then_block, time);
                copy_propagation_invalidate_from_block(ctx, state, &iff->else_block, time);

                break;
            }

            case HLSL_IR_LOOP:
            {
                struct hlsl_ir_loop *loop = hlsl_ir_loop(instr);

                copy_propagation_invalidate_from_block(ctx, state, &loop->body, time);

                break;
            }

            case HLSL_IR_SWITCH:
            {
                struct hlsl_ir_switch *s = hlsl_ir_switch(instr);
                struct hlsl_ir_switch_case *c;

                LIST_FOR_EACH_ENTRY(c, &s->cases, struct hlsl_ir_switch_case, entry)
                {
                    copy_propagation_invalidate_from_block(ctx, state, &c->body, time);
                }

                break;
            }

            default:
                break;
        }
    }
}

static bool copy_propagation_transform_block(struct hlsl_ctx *ctx, struct hlsl_block *block,
        struct copy_propagation_state *state);

static bool copy_propagation_process_if(struct hlsl_ctx *ctx, struct hlsl_ir_if *iff,
        struct copy_propagation_state *state)
{
    struct copy_propagation_state inner_state;
    bool progress = false;

    copy_propagation_state_init(ctx, &inner_state, state);
    progress |= copy_propagation_transform_block(ctx, &iff->then_block, &inner_state);
    copy_propagation_state_destroy(&inner_state);

    copy_propagation_state_init(ctx, &inner_state, state);
    progress |= copy_propagation_transform_block(ctx, &iff->else_block, &inner_state);
    copy_propagation_state_destroy(&inner_state);

    /* Ideally we'd invalidate the outer state looking at what was
     * touched in the two inner states, but this doesn't work for
     * loops (because we need to know what is invalidated in advance),
     * so we need copy_propagation_invalidate_from_block() anyway. */
    copy_propagation_invalidate_from_block(ctx, state, &iff->then_block, iff->node.index);
    copy_propagation_invalidate_from_block(ctx, state, &iff->else_block, iff->node.index);

    return progress;
}

static bool copy_propagation_process_loop(struct hlsl_ctx *ctx, struct hlsl_ir_loop *loop,
        struct copy_propagation_state *state)
{
    struct copy_propagation_state inner_state;
    bool progress = false;

    copy_propagation_invalidate_from_block(ctx, state, &loop->body, loop->node.index);

    copy_propagation_state_init(ctx, &inner_state, state);
    progress |= copy_propagation_transform_block(ctx, &loop->body, &inner_state);
    copy_propagation_state_destroy(&inner_state);

    return progress;
}

static bool copy_propagation_process_switch(struct hlsl_ctx *ctx, struct hlsl_ir_switch *s,
        struct copy_propagation_state *state)
{
    struct copy_propagation_state inner_state;
    struct hlsl_ir_switch_case *c;
    bool progress = false;

    LIST_FOR_EACH_ENTRY(c, &s->cases, struct hlsl_ir_switch_case, entry)
    {
        copy_propagation_state_init(ctx, &inner_state, state);
        progress |= copy_propagation_transform_block(ctx, &c->body, &inner_state);
        copy_propagation_state_destroy(&inner_state);
    }

    LIST_FOR_EACH_ENTRY(c, &s->cases, struct hlsl_ir_switch_case, entry)
    {
        copy_propagation_invalidate_from_block(ctx, state, &c->body, s->node.index);
    }

    return progress;
}

static bool copy_propagation_transform_block(struct hlsl_ctx *ctx, struct hlsl_block *block,
        struct copy_propagation_state *state)
{
    struct hlsl_ir_node *instr, *next;
    bool progress = false;

    LIST_FOR_EACH_ENTRY_SAFE(instr, next, &block->instrs, struct hlsl_ir_node, entry)
    {
        switch (instr->type)
        {
            case HLSL_IR_LOAD:
                progress |= copy_propagation_transform_load(ctx, hlsl_ir_load(instr), state);
                break;

            case HLSL_IR_RESOURCE_LOAD:
                progress |= copy_propagation_transform_resource_load(ctx, hlsl_ir_resource_load(instr), state);
                break;

            case HLSL_IR_RESOURCE_STORE:
                progress |= copy_propagation_transform_resource_store(ctx, hlsl_ir_resource_store(instr), state);
                break;

            case HLSL_IR_STORE:
                copy_propagation_record_store(ctx, hlsl_ir_store(instr), state);
                break;

            case HLSL_IR_SWIZZLE:
                progress |= copy_propagation_transform_swizzle(ctx, hlsl_ir_swizzle(instr), state);
                break;

            case HLSL_IR_IF:
                progress |= copy_propagation_process_if(ctx, hlsl_ir_if(instr), state);
                break;

            case HLSL_IR_LOOP:
                progress |= copy_propagation_process_loop(ctx, hlsl_ir_loop(instr), state);
                break;

            case HLSL_IR_SWITCH:
                progress |= copy_propagation_process_switch(ctx, hlsl_ir_switch(instr), state);
                break;

            default:
                break;
        }
    }

    return progress;
}

bool hlsl_copy_propagation_execute(struct hlsl_ctx *ctx, struct hlsl_block *block)
{
    struct copy_propagation_state state;
    bool progress;

    index_instructions(block, 2);

    copy_propagation_state_init(ctx, &state, NULL);

    progress = copy_propagation_transform_block(ctx, block, &state);

    copy_propagation_state_destroy(&state);

    return progress;
}

enum validation_result
{
    DEREF_VALIDATION_OK,
    DEREF_VALIDATION_OUT_OF_BOUNDS,
    DEREF_VALIDATION_NOT_CONSTANT,
};

static enum validation_result validate_component_index_range_from_deref(struct hlsl_ctx *ctx,
        const struct hlsl_deref *deref)
{
    struct hlsl_type *type = deref->var->data_type;
    unsigned int i;

    for (i = 0; i < deref->path_len; ++i)
    {
        struct hlsl_ir_node *path_node = deref->path[i].node;
        unsigned int idx = 0;

        VKD3D_ASSERT(path_node);
        if (path_node->type != HLSL_IR_CONSTANT)
            return DEREF_VALIDATION_NOT_CONSTANT;

        /* We should always have generated a cast to UINT. */
        VKD3D_ASSERT(path_node->data_type->class == HLSL_CLASS_SCALAR
                && path_node->data_type->e.numeric.type == HLSL_TYPE_UINT);

        idx = hlsl_ir_constant(path_node)->value.u[0].u;

        switch (type->class)
        {
            case HLSL_CLASS_VECTOR:
                if (idx >= type->dimx)
                {
                    hlsl_error(ctx, &path_node->loc, VKD3D_SHADER_ERROR_HLSL_OFFSET_OUT_OF_BOUNDS,
                            "Vector index is out of bounds. %u/%u", idx, type->dimx);
                    return DEREF_VALIDATION_OUT_OF_BOUNDS;
                }
                break;

            case HLSL_CLASS_MATRIX:
                if (idx >= hlsl_type_major_size(type))
                {
                    hlsl_error(ctx, &path_node->loc, VKD3D_SHADER_ERROR_HLSL_OFFSET_OUT_OF_BOUNDS,
                            "Matrix index is out of bounds. %u/%u", idx, hlsl_type_major_size(type));
                    return DEREF_VALIDATION_OUT_OF_BOUNDS;
                }
                break;

            case HLSL_CLASS_ARRAY:
                if (idx >= type->e.array.elements_count)
                {
                    hlsl_error(ctx, &path_node->loc, VKD3D_SHADER_ERROR_HLSL_OFFSET_OUT_OF_BOUNDS,
                            "Array index is out of bounds. %u/%u", idx, type->e.array.elements_count);
                    return DEREF_VALIDATION_OUT_OF_BOUNDS;
                }
                break;

            case HLSL_CLASS_STRUCT:
                break;

            default:
                vkd3d_unreachable();
        }

        type = hlsl_get_element_type_from_path_index(ctx, type, path_node);
    }

    return DEREF_VALIDATION_OK;
}

static void note_non_static_deref_expressions(struct hlsl_ctx *ctx, const struct hlsl_deref *deref,
        const char *usage)
{
    unsigned int i;

    for (i = 0; i < deref->path_len; ++i)
    {
        struct hlsl_ir_node *path_node = deref->path[i].node;

        VKD3D_ASSERT(path_node);
        if (path_node->type != HLSL_IR_CONSTANT)
            hlsl_note(ctx, &path_node->loc, VKD3D_SHADER_LOG_ERROR,
                    "Expression for %s within \"%s\" cannot be resolved statically.",
                    usage, deref->var->name);
    }
}

static bool validate_dereferences(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr,
        void *context)
{
    switch (instr->type)
    {
        case HLSL_IR_RESOURCE_LOAD:
        {
            struct hlsl_ir_resource_load *load = hlsl_ir_resource_load(instr);

            if (!load->resource.var->is_uniform)
            {
                hlsl_error(ctx, &instr->loc, VKD3D_SHADER_ERROR_HLSL_NON_STATIC_OBJECT_REF,
                        "Loaded resource must have a single uniform source.");
            }
            else if (validate_component_index_range_from_deref(ctx, &load->resource) == DEREF_VALIDATION_NOT_CONSTANT)
            {
                hlsl_error(ctx, &instr->loc, VKD3D_SHADER_ERROR_HLSL_NON_STATIC_OBJECT_REF,
                        "Loaded resource from \"%s\" must be determinable at compile time.",
                        load->resource.var->name);
                note_non_static_deref_expressions(ctx, &load->resource, "loaded resource");
            }

            if (load->sampler.var)
            {
                if (!load->sampler.var->is_uniform)
                {
                    hlsl_error(ctx, &instr->loc, VKD3D_SHADER_ERROR_HLSL_NON_STATIC_OBJECT_REF,
                            "Resource load sampler must have a single uniform source.");
                }
                else if (validate_component_index_range_from_deref(ctx, &load->sampler) == DEREF_VALIDATION_NOT_CONSTANT)
                {
                    hlsl_error(ctx, &instr->loc, VKD3D_SHADER_ERROR_HLSL_NON_STATIC_OBJECT_REF,
                            "Resource load sampler from \"%s\" must be determinable at compile time.",
                            load->sampler.var->name);
                    note_non_static_deref_expressions(ctx, &load->sampler, "resource load sampler");
                }
            }
            break;
        }
        case HLSL_IR_RESOURCE_STORE:
        {
            struct hlsl_ir_resource_store *store = hlsl_ir_resource_store(instr);

            if (!store->resource.var->is_uniform)
            {
                hlsl_error(ctx, &instr->loc, VKD3D_SHADER_ERROR_HLSL_NON_STATIC_OBJECT_REF,
                        "Accessed resource must have a single uniform source.");
            }
            else if (validate_component_index_range_from_deref(ctx, &store->resource) == DEREF_VALIDATION_NOT_CONSTANT)
            {
                hlsl_error(ctx, &instr->loc, VKD3D_SHADER_ERROR_HLSL_NON_STATIC_OBJECT_REF,
                        "Accessed resource from \"%s\" must be determinable at compile time.",
                        store->resource.var->name);
                note_non_static_deref_expressions(ctx, &store->resource, "accessed resource");
            }
            break;
        }
        case HLSL_IR_LOAD:
        {
            struct hlsl_ir_load *load = hlsl_ir_load(instr);
            validate_component_index_range_from_deref(ctx, &load->src);
            break;
        }
        case HLSL_IR_STORE:
        {
            struct hlsl_ir_store *store = hlsl_ir_store(instr);
            validate_component_index_range_from_deref(ctx, &store->lhs);
            break;
        }
        default:
            break;
    }

    return false;
}

static bool is_vec1(const struct hlsl_type *type)
{
    return (type->class == HLSL_CLASS_SCALAR) || (type->class == HLSL_CLASS_VECTOR && type->dimx == 1);
}

static bool fold_redundant_casts(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, void *context)
{
    if (instr->type == HLSL_IR_EXPR)
    {
        struct hlsl_ir_expr *expr = hlsl_ir_expr(instr);
        const struct hlsl_type *dst_type = expr->node.data_type;
        const struct hlsl_type *src_type;

        if (expr->op != HLSL_OP1_CAST)
            return false;

        src_type = expr->operands[0].node->data_type;

        if (hlsl_types_are_equal(src_type, dst_type)
                || (src_type->e.numeric.type == dst_type->e.numeric.type && is_vec1(src_type) && is_vec1(dst_type)))
        {
            hlsl_replace_node(&expr->node, expr->operands[0].node);
            return true;
        }
    }

    return false;
}

/* Copy an element of a complex variable. Helper for
 * split_array_copies(), split_struct_copies() and
 * split_matrix_copies(). Inserts new instructions right before
 * "store". */
static bool split_copy(struct hlsl_ctx *ctx, struct hlsl_ir_store *store,
        const struct hlsl_ir_load *load, const unsigned int idx, struct hlsl_type *type)
{
    struct hlsl_ir_node *split_store, *c;
    struct hlsl_ir_load *split_load;

    if (!(c = hlsl_new_uint_constant(ctx, idx, &store->node.loc)))
        return false;
    list_add_before(&store->node.entry, &c->entry);

    if (!(split_load = hlsl_new_load_index(ctx, &load->src, c, &store->node.loc)))
        return false;
    list_add_before(&store->node.entry, &split_load->node.entry);

    if (!(split_store = hlsl_new_store_index(ctx, &store->lhs, c, &split_load->node, 0, &store->node.loc)))
        return false;
    list_add_before(&store->node.entry, &split_store->entry);

    return true;
}

static bool split_array_copies(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, void *context)
{
    const struct hlsl_ir_node *rhs;
    struct hlsl_type *element_type;
    const struct hlsl_type *type;
    struct hlsl_ir_store *store;
    unsigned int i;

    if (instr->type != HLSL_IR_STORE)
        return false;

    store = hlsl_ir_store(instr);
    rhs = store->rhs.node;
    type = rhs->data_type;
    if (type->class != HLSL_CLASS_ARRAY)
        return false;
    element_type = type->e.array.type;

    if (rhs->type != HLSL_IR_LOAD)
    {
        hlsl_fixme(ctx, &instr->loc, "Array store rhs is not HLSL_IR_LOAD. Broadcast may be missing.");
        return false;
    }

    for (i = 0; i < type->e.array.elements_count; ++i)
    {
        if (!split_copy(ctx, store, hlsl_ir_load(rhs), i, element_type))
            return false;
    }

    /* Remove the store instruction, so that we can split structs which contain
     * other structs. Although assignments produce a value, we don't allow
     * HLSL_IR_STORE to be used as a source. */
    list_remove(&store->node.entry);
    hlsl_free_instr(&store->node);
    return true;
}

static bool split_struct_copies(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, void *context)
{
    const struct hlsl_ir_node *rhs;
    const struct hlsl_type *type;
    struct hlsl_ir_store *store;
    size_t i;

    if (instr->type != HLSL_IR_STORE)
        return false;

    store = hlsl_ir_store(instr);
    rhs = store->rhs.node;
    type = rhs->data_type;
    if (type->class != HLSL_CLASS_STRUCT)
        return false;

    if (rhs->type != HLSL_IR_LOAD)
    {
        hlsl_fixme(ctx, &instr->loc, "Struct store rhs is not HLSL_IR_LOAD. Broadcast may be missing.");
        return false;
    }

    for (i = 0; i < type->e.record.field_count; ++i)
    {
        const struct hlsl_struct_field *field = &type->e.record.fields[i];

        if (!split_copy(ctx, store, hlsl_ir_load(rhs), i, field->type))
            return false;
    }

    /* Remove the store instruction, so that we can split structs which contain
     * other structs. Although assignments produce a value, we don't allow
     * HLSL_IR_STORE to be used as a source. */
    list_remove(&store->node.entry);
    hlsl_free_instr(&store->node);
    return true;
}

static bool split_matrix_copies(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, void *context)
{
    const struct hlsl_ir_node *rhs;
    struct hlsl_type *element_type;
    const struct hlsl_type *type;
    unsigned int i;
    struct hlsl_ir_store *store;

    if (instr->type != HLSL_IR_STORE)
        return false;

    store = hlsl_ir_store(instr);
    rhs = store->rhs.node;
    type = rhs->data_type;
    if (type->class != HLSL_CLASS_MATRIX)
        return false;
    element_type = hlsl_get_vector_type(ctx, type->e.numeric.type, hlsl_type_minor_size(type));

    if (rhs->type != HLSL_IR_LOAD)
    {
        hlsl_fixme(ctx, &instr->loc, "Copying from unsupported node type.");
        return false;
    }

    for (i = 0; i < hlsl_type_major_size(type); ++i)
    {
        if (!split_copy(ctx, store, hlsl_ir_load(rhs), i, element_type))
            return false;
    }

    list_remove(&store->node.entry);
    hlsl_free_instr(&store->node);
    return true;
}

static bool lower_narrowing_casts(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, struct hlsl_block *block)
{
    const struct hlsl_type *src_type, *dst_type;
    struct hlsl_type *dst_vector_type;
    struct hlsl_ir_expr *cast;

    if (instr->type != HLSL_IR_EXPR)
        return false;
    cast = hlsl_ir_expr(instr);
    if (cast->op != HLSL_OP1_CAST)
        return false;
    src_type = cast->operands[0].node->data_type;
    dst_type = cast->node.data_type;

    if (src_type->class <= HLSL_CLASS_VECTOR && dst_type->class <= HLSL_CLASS_VECTOR && dst_type->dimx < src_type->dimx)
    {
        struct hlsl_ir_node *new_cast, *swizzle;

        dst_vector_type = hlsl_get_vector_type(ctx, dst_type->e.numeric.type, src_type->dimx);
        /* We need to preserve the cast since it might be doing more than just
         * narrowing the vector. */
        if (!(new_cast = hlsl_new_cast(ctx, cast->operands[0].node, dst_vector_type, &cast->node.loc)))
            return false;
        hlsl_block_add_instr(block, new_cast);

        if (!(swizzle = hlsl_new_swizzle(ctx, HLSL_SWIZZLE(X, Y, Z, W), dst_type->dimx, new_cast, &cast->node.loc)))
            return false;
        hlsl_block_add_instr(block, swizzle);

        return true;
    }

    return false;
}

static bool fold_swizzle_chains(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, void *context)
{
    struct hlsl_ir_swizzle *swizzle;
    struct hlsl_ir_node *next_instr;

    if (instr->type != HLSL_IR_SWIZZLE)
        return false;
    swizzle = hlsl_ir_swizzle(instr);

    next_instr = swizzle->val.node;

    if (next_instr->type == HLSL_IR_SWIZZLE)
    {
        struct hlsl_ir_node *new_swizzle;
        uint32_t combined_swizzle;

        combined_swizzle = hlsl_combine_swizzles(hlsl_ir_swizzle(next_instr)->swizzle,
                swizzle->swizzle, instr->data_type->dimx);
        next_instr = hlsl_ir_swizzle(next_instr)->val.node;

        if (!(new_swizzle = hlsl_new_swizzle(ctx, combined_swizzle, instr->data_type->dimx, next_instr, &instr->loc)))
            return false;

        list_add_before(&instr->entry, &new_swizzle->entry);
        hlsl_replace_node(instr, new_swizzle);
        return true;
    }

    return false;
}

static bool remove_trivial_swizzles(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, void *context)
{
    struct hlsl_ir_swizzle *swizzle;
    unsigned int i;

    if (instr->type != HLSL_IR_SWIZZLE)
        return false;
    swizzle = hlsl_ir_swizzle(instr);

    if (instr->data_type->dimx != swizzle->val.node->data_type->dimx)
        return false;

    for (i = 0; i < instr->data_type->dimx; ++i)
        if (hlsl_swizzle_get_component(swizzle->swizzle, i) != i)
            return false;

    hlsl_replace_node(instr, swizzle->val.node);

    return true;
}

static bool remove_trivial_conditional_branches(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, void *context)
{
    struct hlsl_ir_constant *condition;
    struct hlsl_ir_if *iff;

    if (instr->type != HLSL_IR_IF)
        return false;
    iff = hlsl_ir_if(instr);
    if (iff->condition.node->type != HLSL_IR_CONSTANT)
        return false;
    condition = hlsl_ir_constant(iff->condition.node);

    list_move_before(&instr->entry, condition->value.u[0].u ? &iff->then_block.instrs : &iff->else_block.instrs);
    list_remove(&instr->entry);
    hlsl_free_instr(instr);

    return true;
}

static bool normalize_switch_cases(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, void *context)
{
    struct hlsl_ir_switch_case *c, *def = NULL;
    bool missing_terminal_break = false;
    struct hlsl_ir_node *node;
    struct hlsl_ir_switch *s;

    if (instr->type != HLSL_IR_SWITCH)
        return false;
    s = hlsl_ir_switch(instr);

    LIST_FOR_EACH_ENTRY(c, &s->cases, struct hlsl_ir_switch_case, entry)
    {
        bool terminal_break = false;

        if (list_empty(&c->body.instrs))
        {
            terminal_break = !!list_next(&s->cases, &c->entry);
        }
        else
        {
            node = LIST_ENTRY(list_tail(&c->body.instrs), struct hlsl_ir_node, entry);
            if (node->type == HLSL_IR_JUMP)
                terminal_break = (hlsl_ir_jump(node)->type == HLSL_IR_JUMP_BREAK);
        }

        missing_terminal_break |= !terminal_break;

        if (!terminal_break)
        {
            if (c->is_default)
            {
                hlsl_error(ctx, &c->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_SYNTAX,
                        "The 'default' case block is not terminated with 'break' or 'return'.");
            }
            else
            {
                hlsl_error(ctx, &c->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_SYNTAX,
                        "Switch case block '%u' is not terminated with 'break' or 'return'.", c->value);
            }
        }
    }

    if (missing_terminal_break)
        return true;

    LIST_FOR_EACH_ENTRY(c, &s->cases, struct hlsl_ir_switch_case, entry)
    {
        if (c->is_default)
        {
            def = c;

            /* Remove preceding empty cases. */
            while (list_prev(&s->cases, &def->entry))
            {
                c = LIST_ENTRY(list_prev(&s->cases, &def->entry), struct hlsl_ir_switch_case, entry);
                if (!list_empty(&c->body.instrs))
                    break;
                hlsl_free_ir_switch_case(c);
            }

            if (list_empty(&def->body.instrs))
            {
                /* Remove following empty cases. */
                while (list_next(&s->cases, &def->entry))
                {
                    c = LIST_ENTRY(list_next(&s->cases, &def->entry), struct hlsl_ir_switch_case, entry);
                    if (!list_empty(&c->body.instrs))
                        break;
                    hlsl_free_ir_switch_case(c);
                }

                /* Merge with the next case. */
                if (list_next(&s->cases, &def->entry))
                {
                    c = LIST_ENTRY(list_next(&s->cases, &def->entry), struct hlsl_ir_switch_case, entry);
                    c->is_default = true;
                    hlsl_free_ir_switch_case(def);
                    def = c;
                }
            }

            break;
        }
    }

    if (def)
    {
        list_remove(&def->entry);
    }
    else
    {
        struct hlsl_ir_node *jump;

        if (!(def = hlsl_new_switch_case(ctx, 0, true, NULL, &s->node.loc)))
            return true;
        if (!(jump = hlsl_new_jump(ctx, HLSL_IR_JUMP_BREAK, NULL, &s->node.loc)))
        {
            hlsl_free_ir_switch_case(def);
            return true;
        }
        hlsl_block_add_instr(&def->body, jump);
    }
    list_add_tail(&s->cases, &def->entry);

    return true;
}

static bool lower_nonconstant_vector_derefs(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, struct hlsl_block *block)
{
    struct hlsl_ir_node *idx;
    struct hlsl_deref *deref;
    struct hlsl_type *type;
    unsigned int i;

    if (instr->type != HLSL_IR_LOAD)
        return false;

    deref = &hlsl_ir_load(instr)->src;
    VKD3D_ASSERT(deref->var);

    if (deref->path_len == 0)
        return false;

    type = deref->var->data_type;
    for (i = 0; i < deref->path_len - 1; ++i)
        type = hlsl_get_element_type_from_path_index(ctx, type, deref->path[i].node);

    idx = deref->path[deref->path_len - 1].node;

    if (type->class == HLSL_CLASS_VECTOR && idx->type != HLSL_IR_CONSTANT)
    {
        struct hlsl_ir_node *eq, *swizzle, *dot, *c, *operands[HLSL_MAX_OPERANDS] = {0};
        struct hlsl_constant_value value;
        struct hlsl_ir_load *vector_load;
        enum hlsl_ir_expr_op op;

        if (!(vector_load = hlsl_new_load_parent(ctx, deref, &instr->loc)))
            return false;
        hlsl_block_add_instr(block, &vector_load->node);

        if (!(swizzle = hlsl_new_swizzle(ctx, HLSL_SWIZZLE(X, X, X, X), type->dimx, idx, &instr->loc)))
            return false;
        hlsl_block_add_instr(block, swizzle);

        value.u[0].u = 0;
        value.u[1].u = 1;
        value.u[2].u = 2;
        value.u[3].u = 3;
        if (!(c = hlsl_new_constant(ctx, hlsl_get_vector_type(ctx, HLSL_TYPE_UINT, type->dimx), &value, &instr->loc)))
            return false;
        hlsl_block_add_instr(block, c);

        operands[0] = swizzle;
        operands[1] = c;
        if (!(eq = hlsl_new_expr(ctx, HLSL_OP2_EQUAL, operands,
                hlsl_get_vector_type(ctx, HLSL_TYPE_BOOL, type->dimx), &instr->loc)))
            return false;
        hlsl_block_add_instr(block, eq);

        if (!(eq = hlsl_new_cast(ctx, eq, type, &instr->loc)))
            return false;
        hlsl_block_add_instr(block, eq);

        op = HLSL_OP2_DOT;
        if (type->dimx == 1)
            op = type->e.numeric.type == HLSL_TYPE_BOOL ? HLSL_OP2_LOGIC_AND : HLSL_OP2_MUL;

        /* Note: We may be creating a DOT for bool vectors here, which we need to lower to
         * LOGIC_OR + LOGIC_AND. */
        operands[0] = &vector_load->node;
        operands[1] = eq;
        if (!(dot = hlsl_new_expr(ctx, op, operands, instr->data_type, &instr->loc)))
            return false;
        hlsl_block_add_instr(block, dot);

        return true;
    }

    return false;
}

static bool validate_nonconstant_vector_store_derefs(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, struct hlsl_block *block)
{
    struct hlsl_ir_node *idx;
    struct hlsl_deref *deref;
    struct hlsl_type *type;
    unsigned int i;

    if (instr->type != HLSL_IR_STORE)
        return false;

    deref = &hlsl_ir_store(instr)->lhs;
    VKD3D_ASSERT(deref->var);

    if (deref->path_len == 0)
        return false;

    type = deref->var->data_type;
    for (i = 0; i < deref->path_len - 1; ++i)
        type = hlsl_get_element_type_from_path_index(ctx, type, deref->path[i].node);

    idx = deref->path[deref->path_len - 1].node;

    if (type->class == HLSL_CLASS_VECTOR && idx->type != HLSL_IR_CONSTANT)
    {
        /* We should turn this into an hlsl_error after we implement unrolling, because if we get
         * here after that, it means that the HLSL is invalid. */
        hlsl_fixme(ctx, &instr->loc, "Non-constant vector addressing on store. Unrolling may be missing.");
    }

    return false;
}

/* This pass flattens array (and row_major matrix) loads that include the indexing of a non-constant
 * index into multiple constant loads, where the value of only one of them ends up in the resulting
 * node.
 * This is achieved through a synthetic variable. The non-constant index is compared for equality
 * with every possible value it can have within the array bounds, and the ternary operator is used
 * to update the value of the synthetic var when the equality check passes. */
static bool lower_nonconstant_array_loads(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr,
        struct hlsl_block *block)
{
    struct hlsl_constant_value zero_value = {0};
    struct hlsl_ir_node *cut_index, *zero, *store;
    unsigned int i, i_cut, element_count;
    const struct hlsl_deref *deref;
    struct hlsl_type *cut_type;
    struct hlsl_ir_load *load;
    struct hlsl_ir_var *var;
    bool row_major;

    if (instr->type != HLSL_IR_LOAD)
        return false;
    load = hlsl_ir_load(instr);
    deref = &load->src;

    if (deref->path_len == 0)
        return false;

    for (i = deref->path_len - 1; ; --i)
    {
        if (deref->path[i].node->type != HLSL_IR_CONSTANT)
        {
            i_cut = i;
            break;
        }

        if (i == 0)
            return false;
    }

    cut_index = deref->path[i_cut].node;
    cut_type = deref->var->data_type;
    for (i = 0; i < i_cut; ++i)
        cut_type = hlsl_get_element_type_from_path_index(ctx, cut_type, deref->path[i].node);

    row_major = hlsl_type_is_row_major(cut_type);
    VKD3D_ASSERT(cut_type->class == HLSL_CLASS_ARRAY || row_major);

    if (!(var = hlsl_new_synthetic_var(ctx, row_major ? "row_major-load" : "array-load", instr->data_type, &instr->loc)))
        return false;

    if (!(zero = hlsl_new_constant(ctx, instr->data_type, &zero_value, &instr->loc)))
        return false;
    hlsl_block_add_instr(block, zero);

    if (!(store = hlsl_new_simple_store(ctx, var, zero)))
        return false;
    hlsl_block_add_instr(block, store);

    TRACE("Lowering non-constant %s load on variable '%s'.\n", row_major ? "row_major" : "array", deref->var->name);

    element_count = hlsl_type_element_count(cut_type);
    for (i = 0; i < element_count; ++i)
    {
        struct hlsl_type *btype = hlsl_get_scalar_type(ctx, HLSL_TYPE_BOOL);
        struct hlsl_ir_node *operands[HLSL_MAX_OPERANDS] = {0};
        struct hlsl_ir_node *const_i, *equals, *ternary, *var_store;
        struct hlsl_ir_load *var_load, *specific_load;
        struct hlsl_deref deref_copy = {0};

        if (!(const_i = hlsl_new_uint_constant(ctx, i, &cut_index->loc)))
            return false;
        hlsl_block_add_instr(block, const_i);

        operands[0] = cut_index;
        operands[1] = const_i;
        if (!(equals = hlsl_new_expr(ctx, HLSL_OP2_EQUAL, operands, btype, &cut_index->loc)))
            return false;
        hlsl_block_add_instr(block, equals);

        if (!(equals = hlsl_new_swizzle(ctx, HLSL_SWIZZLE(X, X, X, X), var->data_type->dimx, equals, &cut_index->loc)))
            return false;
        hlsl_block_add_instr(block, equals);

        if (!(var_load = hlsl_new_var_load(ctx, var, &cut_index->loc)))
            return false;
        hlsl_block_add_instr(block, &var_load->node);

        if (!hlsl_copy_deref(ctx, &deref_copy, deref))
            return false;
        hlsl_src_remove(&deref_copy.path[i_cut]);
        hlsl_src_from_node(&deref_copy.path[i_cut], const_i);

        if (!(specific_load = hlsl_new_load_index(ctx, &deref_copy, NULL, &cut_index->loc)))
        {
            hlsl_cleanup_deref(&deref_copy);
            return false;
        }
        hlsl_block_add_instr(block, &specific_load->node);

        hlsl_cleanup_deref(&deref_copy);

        operands[0] = equals;
        operands[1] = &specific_load->node;
        operands[2] = &var_load->node;
        if (!(ternary = hlsl_new_expr(ctx, HLSL_OP3_TERNARY, operands, instr->data_type, &cut_index->loc)))
            return false;
        hlsl_block_add_instr(block, ternary);

        if (!(var_store = hlsl_new_simple_store(ctx, var, ternary)))
            return false;
        hlsl_block_add_instr(block, var_store);
    }

    if (!(load = hlsl_new_var_load(ctx, var, &instr->loc)))
        return false;
    hlsl_block_add_instr(block, &load->node);

    return true;
}
/* Lower combined samples and sampler variables to synthesized separated textures and samplers.
 * That is, translate SM1-style samples in the source to SM4-style samples in the bytecode. */
static bool lower_combined_samples(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, void *context)
{
    struct hlsl_ir_resource_load *load;
    struct vkd3d_string_buffer *name;
    struct hlsl_ir_var *var;
    unsigned int i;

    if (instr->type != HLSL_IR_RESOURCE_LOAD)
        return false;
    load = hlsl_ir_resource_load(instr);

    switch (load->load_type)
    {
        case HLSL_RESOURCE_LOAD:
        case HLSL_RESOURCE_GATHER_RED:
        case HLSL_RESOURCE_GATHER_GREEN:
        case HLSL_RESOURCE_GATHER_BLUE:
        case HLSL_RESOURCE_GATHER_ALPHA:
        case HLSL_RESOURCE_RESINFO:
        case HLSL_RESOURCE_SAMPLE_CMP:
        case HLSL_RESOURCE_SAMPLE_CMP_LZ:
        case HLSL_RESOURCE_SAMPLE_INFO:
            return false;

        case HLSL_RESOURCE_SAMPLE:
        case HLSL_RESOURCE_SAMPLE_GRAD:
        case HLSL_RESOURCE_SAMPLE_LOD:
        case HLSL_RESOURCE_SAMPLE_LOD_BIAS:
        case HLSL_RESOURCE_SAMPLE_PROJ:
            break;
    }
    if (load->sampler.var)
        return false;

    if (!hlsl_type_is_resource(load->resource.var->data_type))
    {
        hlsl_fixme(ctx, &instr->loc, "Lower combined samplers within structs.");
        return false;
    }

    VKD3D_ASSERT(hlsl_deref_get_regset(ctx, &load->resource) == HLSL_REGSET_SAMPLERS);

    if (!(name = hlsl_get_string_buffer(ctx)))
        return false;
    vkd3d_string_buffer_printf(name, "<resource>%s", load->resource.var->name);

    TRACE("Lowering to separate resource %s.\n", debugstr_a(name->buffer));

    if (!(var = hlsl_get_var(ctx->globals, name->buffer)))
    {
        struct hlsl_type *texture_array_type = hlsl_new_texture_type(ctx, load->sampling_dim,
                hlsl_get_vector_type(ctx, HLSL_TYPE_FLOAT, 4), 0);

        /* Create (possibly multi-dimensional) texture array type with the same dims as the sampler array. */
        struct hlsl_type *arr_type = load->resource.var->data_type;
        for (i = 0; i < load->resource.path_len; ++i)
        {
            VKD3D_ASSERT(arr_type->class == HLSL_CLASS_ARRAY);
            texture_array_type = hlsl_new_array_type(ctx, texture_array_type, arr_type->e.array.elements_count);
            arr_type = arr_type->e.array.type;
        }

        if (!(var = hlsl_new_synthetic_var_named(ctx, name->buffer, texture_array_type, &instr->loc, false)))
        {
            hlsl_release_string_buffer(ctx, name);
            return false;
        }
        var->is_uniform = 1;
        var->is_separated_resource = true;

        list_add_tail(&ctx->extern_vars, &var->extern_entry);
    }
    hlsl_release_string_buffer(ctx, name);

    if (load->sampling_dim != var->data_type->sampler_dim)
    {
        hlsl_error(ctx, &load->node.loc, VKD3D_SHADER_ERROR_HLSL_INCONSISTENT_SAMPLER,
                "Cannot split combined samplers from \"%s\" if they have different usage dimensions.",
                load->resource.var->name);
        hlsl_note(ctx, &var->loc, VKD3D_SHADER_LOG_ERROR, "First use as combined sampler is here.");
        return false;

    }

    hlsl_copy_deref(ctx, &load->sampler, &load->resource);
    load->resource.var = var;
    VKD3D_ASSERT(hlsl_deref_get_type(ctx, &load->resource)->class == HLSL_CLASS_TEXTURE);
    VKD3D_ASSERT(hlsl_deref_get_type(ctx, &load->sampler)->class == HLSL_CLASS_SAMPLER);

    return true;
}

static void insert_ensuring_decreasing_bind_count(struct list *list, struct hlsl_ir_var *to_add,
        enum hlsl_regset regset)
{
    struct hlsl_ir_var *var;

    LIST_FOR_EACH_ENTRY(var, list, struct hlsl_ir_var, extern_entry)
    {
        if (var->bind_count[regset] < to_add->bind_count[regset])
        {
            list_add_before(&var->extern_entry, &to_add->extern_entry);
            return;
        }
    }

    list_add_tail(list, &to_add->extern_entry);
}

static bool sort_synthetic_separated_samplers_first(struct hlsl_ctx *ctx)
{
    struct list separated_resources;
    struct hlsl_ir_var *var, *next;

    list_init(&separated_resources);

    LIST_FOR_EACH_ENTRY_SAFE(var, next, &ctx->extern_vars, struct hlsl_ir_var, extern_entry)
    {
        if (var->is_separated_resource)
        {
            list_remove(&var->extern_entry);
            insert_ensuring_decreasing_bind_count(&separated_resources, var, HLSL_REGSET_TEXTURES);
        }
    }

    list_move_head(&ctx->extern_vars, &separated_resources);

    return false;
}

/* Turn CAST to int or uint into FLOOR + REINTERPRET (which is written as a mere MOV). */
static bool lower_casts_to_int(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, struct hlsl_block *block)
{
    struct hlsl_ir_node *operands[HLSL_MAX_OPERANDS] = { 0 };
    struct hlsl_ir_node *arg, *floor, *res;
    struct hlsl_ir_expr *expr;

    if (instr->type != HLSL_IR_EXPR)
        return false;
    expr = hlsl_ir_expr(instr);
    if (expr->op != HLSL_OP1_CAST)
        return false;

    arg = expr->operands[0].node;
    if (instr->data_type->e.numeric.type != HLSL_TYPE_INT && instr->data_type->e.numeric.type != HLSL_TYPE_UINT)
        return false;
    if (arg->data_type->e.numeric.type != HLSL_TYPE_FLOAT && arg->data_type->e.numeric.type != HLSL_TYPE_HALF)
        return false;

    if (!(floor = hlsl_new_unary_expr(ctx, HLSL_OP1_FLOOR, arg, &instr->loc)))
        return false;
    hlsl_block_add_instr(block, floor);

    memset(operands, 0, sizeof(operands));
    operands[0] = floor;
    if (!(res = hlsl_new_expr(ctx, HLSL_OP1_REINTERPRET, operands, instr->data_type, &instr->loc)))
        return false;
    hlsl_block_add_instr(block, res);

    return true;
}

/* Lower DIV to RCP + MUL. */
static bool lower_division(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, struct hlsl_block *block)
{
    struct hlsl_ir_node *rcp, *mul;
    struct hlsl_ir_expr *expr;

    if (instr->type != HLSL_IR_EXPR)
        return false;
    expr = hlsl_ir_expr(instr);
    if (expr->op != HLSL_OP2_DIV)
        return false;

    if (!(rcp = hlsl_new_unary_expr(ctx, HLSL_OP1_RCP, expr->operands[1].node, &instr->loc)))
        return false;
    hlsl_block_add_instr(block, rcp);

    if (!(mul = hlsl_new_binary_expr(ctx, HLSL_OP2_MUL, expr->operands[0].node, rcp)))
        return false;
    hlsl_block_add_instr(block, mul);

    return true;
}

/* Lower SQRT to RSQ + RCP. */
static bool lower_sqrt(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, struct hlsl_block *block)
{
    struct hlsl_ir_node *rsq, *rcp;
    struct hlsl_ir_expr *expr;

    if (instr->type != HLSL_IR_EXPR)
        return false;
    expr = hlsl_ir_expr(instr);
    if (expr->op != HLSL_OP1_SQRT)
        return false;

    if (!(rsq = hlsl_new_unary_expr(ctx, HLSL_OP1_RSQ, expr->operands[0].node, &instr->loc)))
        return false;
    hlsl_block_add_instr(block, rsq);

    if (!(rcp = hlsl_new_unary_expr(ctx, HLSL_OP1_RCP, rsq, &instr->loc)))
        return false;
    hlsl_block_add_instr(block, rcp);
    return true;
}

/* Lower DP2 to MUL + ADD */
static bool lower_dot(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, struct hlsl_block *block)
{
    struct hlsl_ir_node *arg1, *arg2, *mul, *replacement, *zero, *add_x, *add_y;
    struct hlsl_ir_expr *expr;

    if (instr->type != HLSL_IR_EXPR)
        return false;
    expr = hlsl_ir_expr(instr);
    arg1 = expr->operands[0].node;
    arg2 = expr->operands[1].node;
    if (expr->op != HLSL_OP2_DOT)
        return false;
    if (arg1->data_type->dimx != 2)
        return false;

    if (ctx->profile->type == VKD3D_SHADER_TYPE_PIXEL)
    {
        struct hlsl_ir_node *operands[HLSL_MAX_OPERANDS] = { 0 };

        if (!(zero = hlsl_new_float_constant(ctx, 0.0f, &expr->node.loc)))
            return false;
        hlsl_block_add_instr(block, zero);

        operands[0] = arg1;
        operands[1] = arg2;
        operands[2] = zero;

        if (!(replacement = hlsl_new_expr(ctx, HLSL_OP3_DP2ADD, operands, instr->data_type, &expr->node.loc)))
            return false;
    }
    else
    {
        if (!(mul = hlsl_new_binary_expr(ctx, HLSL_OP2_MUL, expr->operands[0].node, expr->operands[1].node)))
            return false;
        hlsl_block_add_instr(block, mul);

        if (!(add_x = hlsl_new_swizzle(ctx, HLSL_SWIZZLE(X, X, X, X), instr->data_type->dimx, mul, &expr->node.loc)))
            return false;
        hlsl_block_add_instr(block, add_x);

        if (!(add_y = hlsl_new_swizzle(ctx, HLSL_SWIZZLE(Y, Y, Y, Y), instr->data_type->dimx, mul, &expr->node.loc)))
            return false;
        hlsl_block_add_instr(block, add_y);

        if (!(replacement = hlsl_new_binary_expr(ctx, HLSL_OP2_ADD, add_x, add_y)))
            return false;
    }
    hlsl_block_add_instr(block, replacement);

    return true;
}

/* Lower ABS to MAX */
static bool lower_abs(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, struct hlsl_block *block)
{
    struct hlsl_ir_node *arg, *neg, *replacement;
    struct hlsl_ir_expr *expr;

    if (instr->type != HLSL_IR_EXPR)
        return false;
    expr = hlsl_ir_expr(instr);
    arg = expr->operands[0].node;
    if (expr->op != HLSL_OP1_ABS)
        return false;

    if (!(neg = hlsl_new_unary_expr(ctx, HLSL_OP1_NEG, arg, &instr->loc)))
        return false;
    hlsl_block_add_instr(block, neg);

    if (!(replacement = hlsl_new_binary_expr(ctx, HLSL_OP2_MAX, neg, arg)))
        return false;
    hlsl_block_add_instr(block, replacement);

    return true;
}

/* Lower ROUND using FRC, ROUND(x) -> ((x + 0.5) - FRC(x + 0.5)). */
static bool lower_round(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, struct hlsl_block *block)
{
    struct hlsl_ir_node *arg, *neg, *sum, *frc, *half, *replacement;
    struct hlsl_type *type = instr->data_type;
    struct hlsl_constant_value half_value;
    unsigned int i, component_count;
    struct hlsl_ir_expr *expr;

    if (instr->type != HLSL_IR_EXPR)
        return false;

    expr = hlsl_ir_expr(instr);
    arg = expr->operands[0].node;
    if (expr->op != HLSL_OP1_ROUND)
        return false;

    component_count = hlsl_type_component_count(type);
    for (i = 0; i < component_count; ++i)
        half_value.u[i].f = 0.5f;
    if (!(half = hlsl_new_constant(ctx, type, &half_value, &expr->node.loc)))
        return false;
    hlsl_block_add_instr(block, half);

    if (!(sum = hlsl_new_binary_expr(ctx, HLSL_OP2_ADD, arg, half)))
        return false;
    hlsl_block_add_instr(block, sum);

    if (!(frc = hlsl_new_unary_expr(ctx, HLSL_OP1_FRACT, sum, &instr->loc)))
        return false;
    hlsl_block_add_instr(block, frc);

    if (!(neg = hlsl_new_unary_expr(ctx, HLSL_OP1_NEG, frc, &instr->loc)))
        return false;
    hlsl_block_add_instr(block, neg);

    if (!(replacement = hlsl_new_binary_expr(ctx, HLSL_OP2_ADD, sum, neg)))
        return false;
    hlsl_block_add_instr(block, replacement);

    return true;
}

/* Lower CEIL to FRC */
static bool lower_ceil(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, struct hlsl_block *block)
{
    struct hlsl_ir_node *arg, *neg, *sum, *frc;
    struct hlsl_ir_expr *expr;

    if (instr->type != HLSL_IR_EXPR)
        return false;

    expr = hlsl_ir_expr(instr);
    arg = expr->operands[0].node;
    if (expr->op != HLSL_OP1_CEIL)
        return false;

    if (!(neg = hlsl_new_unary_expr(ctx, HLSL_OP1_NEG, arg, &instr->loc)))
        return false;
    hlsl_block_add_instr(block, neg);

    if (!(frc = hlsl_new_unary_expr(ctx, HLSL_OP1_FRACT, neg, &instr->loc)))
        return false;
    hlsl_block_add_instr(block, frc);

    if (!(sum = hlsl_new_binary_expr(ctx, HLSL_OP2_ADD, frc, arg)))
        return false;
    hlsl_block_add_instr(block, sum);

    return true;
}

/* Lower FLOOR to FRC */
static bool lower_floor(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, struct hlsl_block *block)
{
    struct hlsl_ir_node *arg, *neg, *sum, *frc;
    struct hlsl_ir_expr *expr;

    if (instr->type != HLSL_IR_EXPR)
        return false;

    expr = hlsl_ir_expr(instr);
    arg = expr->operands[0].node;
    if (expr->op != HLSL_OP1_FLOOR)
        return false;

    if (!(frc = hlsl_new_unary_expr(ctx, HLSL_OP1_FRACT, arg, &instr->loc)))
        return false;
    hlsl_block_add_instr(block, frc);

    if (!(neg = hlsl_new_unary_expr(ctx, HLSL_OP1_NEG, frc, &instr->loc)))
        return false;
    hlsl_block_add_instr(block, neg);

    if (!(sum = hlsl_new_binary_expr(ctx, HLSL_OP2_ADD, neg, arg)))
        return false;
    hlsl_block_add_instr(block, sum);

    return true;
}

/* Lower SIN/COS to SINCOS for SM1.  */
static bool lower_trig(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, struct hlsl_block *block)
{
    struct hlsl_ir_node *arg, *half, *two_pi, *reciprocal_two_pi, *neg_pi;
    struct hlsl_constant_value half_value, two_pi_value, reciprocal_two_pi_value, neg_pi_value;
    struct hlsl_ir_node *mad, *frc, *reduced;
    struct hlsl_type *type;
    struct hlsl_ir_expr *expr;
    enum hlsl_ir_expr_op op;
    struct hlsl_ir_node *sincos;
    int i;

    if (instr->type != HLSL_IR_EXPR)
        return false;
    expr = hlsl_ir_expr(instr);

    if (expr->op == HLSL_OP1_SIN)
        op = HLSL_OP1_SIN_REDUCED;
    else if (expr->op == HLSL_OP1_COS)
        op = HLSL_OP1_COS_REDUCED;
    else
        return false;

    arg = expr->operands[0].node;
    type = arg->data_type;

    /* Reduce the range of the input angles to [-pi, pi]. */
    for (i = 0; i < type->dimx; ++i)
    {
        half_value.u[i].f = 0.5;
        two_pi_value.u[i].f = 2.0 * M_PI;
        reciprocal_two_pi_value.u[i].f = 1.0 / (2.0 * M_PI);
        neg_pi_value.u[i].f = -M_PI;
    }

    if (!(half = hlsl_new_constant(ctx, type, &half_value, &instr->loc))
            || !(two_pi = hlsl_new_constant(ctx, type, &two_pi_value, &instr->loc))
            || !(reciprocal_two_pi = hlsl_new_constant(ctx, type, &reciprocal_two_pi_value, &instr->loc))
            || !(neg_pi = hlsl_new_constant(ctx, type, &neg_pi_value, &instr->loc)))
        return false;
    hlsl_block_add_instr(block, half);
    hlsl_block_add_instr(block, two_pi);
    hlsl_block_add_instr(block, reciprocal_two_pi);
    hlsl_block_add_instr(block, neg_pi);

    if (!(mad = hlsl_new_ternary_expr(ctx, HLSL_OP3_MAD, arg, reciprocal_two_pi, half)))
        return false;
    hlsl_block_add_instr(block, mad);
    if (!(frc = hlsl_new_unary_expr(ctx, HLSL_OP1_FRACT, mad, &instr->loc)))
        return false;
    hlsl_block_add_instr(block, frc);
    if (!(reduced = hlsl_new_ternary_expr(ctx, HLSL_OP3_MAD, frc, two_pi, neg_pi)))
        return false;
    hlsl_block_add_instr(block, reduced);

    if (type->dimx == 1)
    {
        if (!(sincos = hlsl_new_unary_expr(ctx, op, reduced, &instr->loc)))
            return false;
        hlsl_block_add_instr(block, sincos);
    }
    else
    {
        struct hlsl_ir_node *comps[4] = {0};
        struct hlsl_ir_var *var;
        struct hlsl_deref var_deref;
        struct hlsl_ir_load *var_load;

        for (i = 0; i < type->dimx; ++i)
        {
            uint32_t s = hlsl_swizzle_from_writemask(1 << i);

            if (!(comps[i] = hlsl_new_swizzle(ctx, s, 1, reduced, &instr->loc)))
                return false;
            hlsl_block_add_instr(block, comps[i]);
        }

        if (!(var = hlsl_new_synthetic_var(ctx, "sincos", type, &instr->loc)))
            return false;
        hlsl_init_simple_deref_from_var(&var_deref, var);

        for (i = 0; i < type->dimx; ++i)
        {
            struct hlsl_block store_block;

            if (!(sincos = hlsl_new_unary_expr(ctx, op, comps[i], &instr->loc)))
                return false;
            hlsl_block_add_instr(block, sincos);

            if (!hlsl_new_store_component(ctx, &store_block, &var_deref, i, sincos))
                return false;
            hlsl_block_add_block(block, &store_block);
        }

        if (!(var_load = hlsl_new_load_index(ctx, &var_deref, NULL, &instr->loc)))
            return false;
        hlsl_block_add_instr(block, &var_load->node);
    }

    return true;
}

static bool lower_logic_not(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, struct hlsl_block *block)
{
    struct hlsl_ir_node *operands[HLSL_MAX_OPERANDS];
    struct hlsl_ir_node *arg, *arg_cast, *neg, *one, *sub, *res;
    struct hlsl_constant_value one_value;
    struct hlsl_type *float_type;
    struct hlsl_ir_expr *expr;

    if (instr->type != HLSL_IR_EXPR)
        return false;
    expr = hlsl_ir_expr(instr);
    if (expr->op != HLSL_OP1_LOGIC_NOT)
        return false;

    arg = expr->operands[0].node;
    float_type = hlsl_get_vector_type(ctx, HLSL_TYPE_FLOAT, arg->data_type->dimx);

    /* If this is happens, it means we failed to cast the argument to boolean somewhere. */
    VKD3D_ASSERT(arg->data_type->e.numeric.type == HLSL_TYPE_BOOL);

    if (!(arg_cast = hlsl_new_cast(ctx, arg, float_type, &arg->loc)))
        return false;
    hlsl_block_add_instr(block, arg_cast);

    if (!(neg = hlsl_new_unary_expr(ctx, HLSL_OP1_NEG, arg_cast, &instr->loc)))
        return false;
    hlsl_block_add_instr(block, neg);

    one_value.u[0].f = 1.0;
    one_value.u[1].f = 1.0;
    one_value.u[2].f = 1.0;
    one_value.u[3].f = 1.0;
    if (!(one = hlsl_new_constant(ctx, float_type, &one_value, &instr->loc)))
        return false;
    hlsl_block_add_instr(block, one);

    if (!(sub = hlsl_new_binary_expr(ctx, HLSL_OP2_ADD, one, neg)))
        return false;
    hlsl_block_add_instr(block, sub);

    memset(operands, 0, sizeof(operands));
    operands[0] = sub;
    if (!(res = hlsl_new_expr(ctx, HLSL_OP1_REINTERPRET, operands, instr->data_type, &instr->loc)))
        return false;
    hlsl_block_add_instr(block, res);

    return true;
}

/* Lower TERNARY to CMP for SM1. */
static bool lower_ternary(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, struct hlsl_block *block)
{
    struct hlsl_ir_node *operands[HLSL_MAX_OPERANDS] = { 0 }, *replacement;
    struct hlsl_ir_node *cond, *first, *second, *float_cond, *neg;
    struct hlsl_ir_expr *expr;
    struct hlsl_type *type;

    if (instr->type != HLSL_IR_EXPR)
        return false;

    expr = hlsl_ir_expr(instr);
    if (expr->op != HLSL_OP3_TERNARY)
        return false;

    cond = expr->operands[0].node;
    first = expr->operands[1].node;
    second = expr->operands[2].node;

    if (cond->data_type->class > HLSL_CLASS_VECTOR || instr->data_type->class > HLSL_CLASS_VECTOR)
    {
        hlsl_fixme(ctx, &instr->loc, "Lower ternary of type other than scalar or vector.");
        return false;
    }

    VKD3D_ASSERT(cond->data_type->e.numeric.type == HLSL_TYPE_BOOL);

    type = hlsl_get_numeric_type(ctx, instr->data_type->class, HLSL_TYPE_FLOAT,
            instr->data_type->dimx, instr->data_type->dimy);

    if (!(float_cond = hlsl_new_cast(ctx, cond, type, &instr->loc)))
        return false;
    hlsl_block_add_instr(block, float_cond);

    if (!(neg = hlsl_new_unary_expr(ctx, HLSL_OP1_NEG, float_cond, &instr->loc)))
        return false;
    hlsl_block_add_instr(block, neg);

    memset(operands, 0, sizeof(operands));
    operands[0] = neg;
    operands[1] = second;
    operands[2] = first;
    if (!(replacement = hlsl_new_expr(ctx, HLSL_OP3_CMP, operands, first->data_type, &instr->loc)))
        return false;

    hlsl_block_add_instr(block, replacement);
    return true;
}

static bool lower_comparison_operators(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr,
        struct hlsl_block *block)
{
    struct hlsl_ir_node *arg1, *arg1_cast, *arg2, *arg2_cast, *slt, *res, *ret;
    struct hlsl_ir_node *operands[HLSL_MAX_OPERANDS];
    struct hlsl_type *float_type;
    struct hlsl_ir_expr *expr;
    bool negate = false;

    if (instr->type != HLSL_IR_EXPR)
        return false;
    expr = hlsl_ir_expr(instr);
    if (expr->op != HLSL_OP2_EQUAL && expr->op != HLSL_OP2_NEQUAL && expr->op != HLSL_OP2_LESS
            && expr->op != HLSL_OP2_GEQUAL)
        return false;

    arg1 = expr->operands[0].node;
    arg2 = expr->operands[1].node;
    float_type = hlsl_get_vector_type(ctx, HLSL_TYPE_FLOAT, instr->data_type->dimx);

    if (!(arg1_cast = hlsl_new_cast(ctx, arg1, float_type, &instr->loc)))
        return false;
    hlsl_block_add_instr(block, arg1_cast);

    if (!(arg2_cast = hlsl_new_cast(ctx, arg2, float_type, &instr->loc)))
        return false;
    hlsl_block_add_instr(block, arg2_cast);

    switch (expr->op)
    {
        case HLSL_OP2_EQUAL:
        case HLSL_OP2_NEQUAL:
        {
            struct hlsl_ir_node *neg, *sub, *abs, *abs_neg;

            if (!(neg = hlsl_new_unary_expr(ctx, HLSL_OP1_NEG, arg2_cast, &instr->loc)))
                return false;
            hlsl_block_add_instr(block, neg);

            if (!(sub = hlsl_new_binary_expr(ctx, HLSL_OP2_ADD, arg1_cast, neg)))
                return false;
            hlsl_block_add_instr(block, sub);

            if (ctx->profile->major_version >= 3)
            {
                if (!(abs = hlsl_new_unary_expr(ctx, HLSL_OP1_ABS, sub, &instr->loc)))
                    return false;
                hlsl_block_add_instr(block, abs);
            }
            else
            {
                /* Use MUL as a precarious ABS. */
                if (!(abs = hlsl_new_binary_expr(ctx, HLSL_OP2_MUL, sub, sub)))
                    return false;
                hlsl_block_add_instr(block, abs);
            }

            if (!(abs_neg = hlsl_new_unary_expr(ctx, HLSL_OP1_NEG, abs, &instr->loc)))
                return false;
            hlsl_block_add_instr(block, abs_neg);

            if (!(slt = hlsl_new_binary_expr(ctx, HLSL_OP2_SLT, abs_neg, abs)))
                return false;
            hlsl_block_add_instr(block, slt);

            negate = (expr->op == HLSL_OP2_EQUAL);
            break;
        }

        case HLSL_OP2_GEQUAL:
        case HLSL_OP2_LESS:
        {
            if (!(slt = hlsl_new_binary_expr(ctx, HLSL_OP2_SLT, arg1_cast, arg2_cast)))
                return false;
            hlsl_block_add_instr(block, slt);

            negate = (expr->op == HLSL_OP2_GEQUAL);
            break;
        }

        default:
            vkd3d_unreachable();
    }

    if (negate)
    {
        struct hlsl_constant_value one_value;
        struct hlsl_ir_node *one, *slt_neg;

        one_value.u[0].f = 1.0;
        one_value.u[1].f = 1.0;
        one_value.u[2].f = 1.0;
        one_value.u[3].f = 1.0;
        if (!(one = hlsl_new_constant(ctx, float_type, &one_value, &instr->loc)))
            return false;
        hlsl_block_add_instr(block, one);

        if (!(slt_neg = hlsl_new_unary_expr(ctx, HLSL_OP1_NEG, slt, &instr->loc)))
            return false;
        hlsl_block_add_instr(block, slt_neg);

        if (!(res = hlsl_new_binary_expr(ctx, HLSL_OP2_ADD, one, slt_neg)))
            return false;
        hlsl_block_add_instr(block, res);
    }
    else
    {
        res = slt;
    }

    /* We need a REINTERPRET so that the HLSL IR code is valid. SLT and its arguments must be FLOAT,
     * and casts to BOOL have already been lowered to "!= 0". */
    memset(operands, 0, sizeof(operands));
    operands[0] = res;
    if (!(ret = hlsl_new_expr(ctx, HLSL_OP1_REINTERPRET, operands, instr->data_type, &instr->loc)))
        return false;
    hlsl_block_add_instr(block, ret);

    return true;
}

/* Intended to be used for SM1-SM3, lowers SLT instructions (only available in vertex shaders) to
 * CMP instructions (only available in pixel shaders).
 * Based on the following equivalence:
 *     SLT(x, y)
 *     = (x < y) ? 1.0 : 0.0
 *     = ((x - y) >= 0) ? 0.0 : 1.0
 *     = CMP(x - y, 0.0, 1.0)
 */
static bool lower_slt(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, struct hlsl_block *block)
{
    struct hlsl_ir_node *arg1, *arg2, *arg1_cast, *arg2_cast, *neg, *sub, *zero, *one, *cmp;
    struct hlsl_constant_value zero_value, one_value;
    struct hlsl_type *float_type;
    struct hlsl_ir_expr *expr;

    if (instr->type != HLSL_IR_EXPR)
        return false;
    expr = hlsl_ir_expr(instr);
    if (expr->op != HLSL_OP2_SLT)
        return false;

    arg1 = expr->operands[0].node;
    arg2 = expr->operands[1].node;
    float_type = hlsl_get_vector_type(ctx, HLSL_TYPE_FLOAT, instr->data_type->dimx);

    if (!(arg1_cast = hlsl_new_cast(ctx, arg1, float_type, &instr->loc)))
        return false;
    hlsl_block_add_instr(block, arg1_cast);

    if (!(arg2_cast = hlsl_new_cast(ctx, arg2, float_type, &instr->loc)))
        return false;
    hlsl_block_add_instr(block, arg2_cast);

    if (!(neg = hlsl_new_unary_expr(ctx, HLSL_OP1_NEG, arg2_cast, &instr->loc)))
        return false;
    hlsl_block_add_instr(block, neg);

    if (!(sub = hlsl_new_binary_expr(ctx, HLSL_OP2_ADD, arg1_cast, neg)))
        return false;
    hlsl_block_add_instr(block, sub);

    memset(&zero_value, 0, sizeof(zero_value));
    if (!(zero = hlsl_new_constant(ctx, float_type, &zero_value, &instr->loc)))
        return false;
    hlsl_block_add_instr(block, zero);

    one_value.u[0].f = 1.0;
    one_value.u[1].f = 1.0;
    one_value.u[2].f = 1.0;
    one_value.u[3].f = 1.0;
    if (!(one = hlsl_new_constant(ctx, float_type, &one_value, &instr->loc)))
        return false;
    hlsl_block_add_instr(block, one);

    if (!(cmp = hlsl_new_ternary_expr(ctx, HLSL_OP3_CMP, sub, zero, one)))
        return false;
    hlsl_block_add_instr(block, cmp);

    return true;
}

/* Intended to be used for SM1-SM3, lowers CMP instructions (only available in pixel shaders) to
 * SLT instructions (only available in vertex shaders).
 * Based on the following equivalence:
 *     CMP(x, y, z)
 *     = (x >= 0) ? y : z
 *     = z * ((x < 0) ? 1.0 : 0.0) + y * ((x < 0) ? 0.0 : 1.0)
 *     = z * SLT(x, 0.0) + y * (1 - SLT(x, 0.0))
 */
static bool lower_cmp(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, struct hlsl_block *block)
{
    struct hlsl_ir_node *args[3], *args_cast[3], *slt, *neg_slt, *sub, *zero, *one, *mul1, *mul2, *add;
    struct hlsl_constant_value zero_value, one_value;
    struct hlsl_type *float_type;
    struct hlsl_ir_expr *expr;
    unsigned int i;

    if (instr->type != HLSL_IR_EXPR)
        return false;
    expr = hlsl_ir_expr(instr);
    if (expr->op != HLSL_OP3_CMP)
        return false;

    float_type = hlsl_get_vector_type(ctx, HLSL_TYPE_FLOAT, instr->data_type->dimx);

    for (i = 0; i < 3; ++i)
    {
        args[i] = expr->operands[i].node;

        if (!(args_cast[i] = hlsl_new_cast(ctx, args[i], float_type, &instr->loc)))
            return false;
        hlsl_block_add_instr(block, args_cast[i]);
    }

    memset(&zero_value, 0, sizeof(zero_value));
    if (!(zero = hlsl_new_constant(ctx, float_type, &zero_value, &instr->loc)))
        return false;
    hlsl_block_add_instr(block, zero);

    one_value.u[0].f = 1.0;
    one_value.u[1].f = 1.0;
    one_value.u[2].f = 1.0;
    one_value.u[3].f = 1.0;
    if (!(one = hlsl_new_constant(ctx, float_type, &one_value, &instr->loc)))
        return false;
    hlsl_block_add_instr(block, one);

    if (!(slt = hlsl_new_binary_expr(ctx, HLSL_OP2_SLT, args_cast[0], zero)))
        return false;
    hlsl_block_add_instr(block, slt);

    if (!(mul1 = hlsl_new_binary_expr(ctx, HLSL_OP2_MUL, args_cast[2], slt)))
        return false;
    hlsl_block_add_instr(block, mul1);

    if (!(neg_slt = hlsl_new_unary_expr(ctx, HLSL_OP1_NEG, slt, &instr->loc)))
        return false;
    hlsl_block_add_instr(block, neg_slt);

    if (!(sub = hlsl_new_binary_expr(ctx, HLSL_OP2_ADD, one, neg_slt)))
        return false;
    hlsl_block_add_instr(block, sub);

    if (!(mul2 = hlsl_new_binary_expr(ctx, HLSL_OP2_MUL, args_cast[1], sub)))
        return false;
    hlsl_block_add_instr(block, mul2);

    if (!(add = hlsl_new_binary_expr(ctx, HLSL_OP2_ADD, mul1, mul2)))
        return false;
    hlsl_block_add_instr(block, add);

    return true;
}

static bool lower_casts_to_bool(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, struct hlsl_block *block)
{
    struct hlsl_type *type = instr->data_type, *arg_type;
    static const struct hlsl_constant_value zero_value;
    struct hlsl_ir_node *zero, *neq;
    struct hlsl_ir_expr *expr;

    if (instr->type != HLSL_IR_EXPR)
        return false;
    expr = hlsl_ir_expr(instr);
    if (expr->op != HLSL_OP1_CAST)
        return false;
    arg_type = expr->operands[0].node->data_type;
    if (type->class > HLSL_CLASS_VECTOR || arg_type->class > HLSL_CLASS_VECTOR)
        return false;
    if (type->e.numeric.type != HLSL_TYPE_BOOL)
        return false;

    /* Narrowing casts should have already been lowered. */
    VKD3D_ASSERT(type->dimx == arg_type->dimx);

    zero = hlsl_new_constant(ctx, arg_type, &zero_value, &instr->loc);
    if (!zero)
        return false;
    hlsl_block_add_instr(block, zero);

    if (!(neq = hlsl_new_binary_expr(ctx, HLSL_OP2_NEQUAL, expr->operands[0].node, zero)))
        return false;
    neq->data_type = expr->node.data_type;
    hlsl_block_add_instr(block, neq);

    return true;
}

struct hlsl_ir_node *hlsl_add_conditional(struct hlsl_ctx *ctx, struct hlsl_block *instrs,
        struct hlsl_ir_node *condition, struct hlsl_ir_node *if_true, struct hlsl_ir_node *if_false)
{
    struct hlsl_type *cond_type = condition->data_type;
    struct hlsl_ir_node *operands[HLSL_MAX_OPERANDS];
    struct hlsl_ir_node *cond;

    VKD3D_ASSERT(hlsl_types_are_equal(if_true->data_type, if_false->data_type));

    if (cond_type->e.numeric.type != HLSL_TYPE_BOOL)
    {
        cond_type = hlsl_get_numeric_type(ctx, cond_type->class, HLSL_TYPE_BOOL, cond_type->dimx, cond_type->dimy);

        if (!(condition = hlsl_new_cast(ctx, condition, cond_type, &condition->loc)))
            return NULL;
        hlsl_block_add_instr(instrs, condition);
    }

    operands[0] = condition;
    operands[1] = if_true;
    operands[2] = if_false;
    if (!(cond = hlsl_new_expr(ctx, HLSL_OP3_TERNARY, operands, if_true->data_type, &condition->loc)))
        return false;
    hlsl_block_add_instr(instrs, cond);

    return cond;
}

static bool lower_int_division(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, struct hlsl_block *block)
{
    struct hlsl_ir_node *arg1, *arg2, *xor, *and, *abs1, *abs2, *div, *neg, *cast1, *cast2, *cast3, *high_bit;
    struct hlsl_type *type = instr->data_type, *utype;
    struct hlsl_constant_value high_bit_value;
    struct hlsl_ir_expr *expr;
    unsigned int i;

    if (instr->type != HLSL_IR_EXPR)
        return false;
    expr = hlsl_ir_expr(instr);
    arg1 = expr->operands[0].node;
    arg2 = expr->operands[1].node;
    if (expr->op != HLSL_OP2_DIV)
        return false;
    if (type->class != HLSL_CLASS_SCALAR && type->class != HLSL_CLASS_VECTOR)
        return false;
    if (type->e.numeric.type != HLSL_TYPE_INT)
        return false;
    utype = hlsl_get_numeric_type(ctx, type->class, HLSL_TYPE_UINT, type->dimx, type->dimy);

    if (!(xor = hlsl_new_binary_expr(ctx, HLSL_OP2_BIT_XOR, arg1, arg2)))
        return false;
    hlsl_block_add_instr(block, xor);

    for (i = 0; i < type->dimx; ++i)
        high_bit_value.u[i].u = 0x80000000;
    if (!(high_bit = hlsl_new_constant(ctx, type, &high_bit_value, &instr->loc)))
        return false;
    hlsl_block_add_instr(block, high_bit);

    if (!(and = hlsl_new_binary_expr(ctx, HLSL_OP2_BIT_AND, xor, high_bit)))
        return false;
    hlsl_block_add_instr(block, and);

    if (!(abs1 = hlsl_new_unary_expr(ctx, HLSL_OP1_ABS, arg1, &instr->loc)))
        return false;
    hlsl_block_add_instr(block, abs1);

    if (!(cast1 = hlsl_new_cast(ctx, abs1, utype, &instr->loc)))
        return false;
    hlsl_block_add_instr(block, cast1);

    if (!(abs2 = hlsl_new_unary_expr(ctx, HLSL_OP1_ABS, arg2, &instr->loc)))
        return false;
    hlsl_block_add_instr(block, abs2);

    if (!(cast2 = hlsl_new_cast(ctx, abs2, utype, &instr->loc)))
        return false;
    hlsl_block_add_instr(block, cast2);

    if (!(div = hlsl_new_binary_expr(ctx, HLSL_OP2_DIV, cast1, cast2)))
        return false;
    hlsl_block_add_instr(block, div);

    if (!(cast3 = hlsl_new_cast(ctx, div, type, &instr->loc)))
        return false;
    hlsl_block_add_instr(block, cast3);

    if (!(neg = hlsl_new_unary_expr(ctx, HLSL_OP1_NEG, cast3, &instr->loc)))
        return false;
    hlsl_block_add_instr(block, neg);

    return hlsl_add_conditional(ctx, block, and, neg, cast3);
}

static bool lower_int_modulus(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, struct hlsl_block *block)
{
    struct hlsl_ir_node *arg1, *arg2, *and, *abs1, *abs2, *div, *neg, *cast1, *cast2, *cast3, *high_bit;
    struct hlsl_type *type = instr->data_type, *utype;
    struct hlsl_constant_value high_bit_value;
    struct hlsl_ir_expr *expr;
    unsigned int i;

    if (instr->type != HLSL_IR_EXPR)
        return false;
    expr = hlsl_ir_expr(instr);
    arg1 = expr->operands[0].node;
    arg2 = expr->operands[1].node;
    if (expr->op != HLSL_OP2_MOD)
        return false;
    if (type->class != HLSL_CLASS_SCALAR && type->class != HLSL_CLASS_VECTOR)
        return false;
    if (type->e.numeric.type != HLSL_TYPE_INT)
        return false;
    utype = hlsl_get_numeric_type(ctx, type->class, HLSL_TYPE_UINT, type->dimx, type->dimy);

    for (i = 0; i < type->dimx; ++i)
        high_bit_value.u[i].u = 0x80000000;
    if (!(high_bit = hlsl_new_constant(ctx, type, &high_bit_value, &instr->loc)))
        return false;
    hlsl_block_add_instr(block, high_bit);

    if (!(and = hlsl_new_binary_expr(ctx, HLSL_OP2_BIT_AND, arg1, high_bit)))
        return false;
    hlsl_block_add_instr(block, and);

    if (!(abs1 = hlsl_new_unary_expr(ctx, HLSL_OP1_ABS, arg1, &instr->loc)))
        return false;
    hlsl_block_add_instr(block, abs1);

    if (!(cast1 = hlsl_new_cast(ctx, abs1, utype, &instr->loc)))
        return false;
    hlsl_block_add_instr(block, cast1);

    if (!(abs2 = hlsl_new_unary_expr(ctx, HLSL_OP1_ABS, arg2, &instr->loc)))
        return false;
    hlsl_block_add_instr(block, abs2);

    if (!(cast2 = hlsl_new_cast(ctx, abs2, utype, &instr->loc)))
        return false;
    hlsl_block_add_instr(block, cast2);

    if (!(div = hlsl_new_binary_expr(ctx, HLSL_OP2_MOD, cast1, cast2)))
        return false;
    hlsl_block_add_instr(block, div);

    if (!(cast3 = hlsl_new_cast(ctx, div, type, &instr->loc)))
        return false;
    hlsl_block_add_instr(block, cast3);

    if (!(neg = hlsl_new_unary_expr(ctx, HLSL_OP1_NEG, cast3, &instr->loc)))
        return false;
    hlsl_block_add_instr(block, neg);

    return hlsl_add_conditional(ctx, block, and, neg, cast3);
}

static bool lower_int_abs(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, struct hlsl_block *block)
{
    struct hlsl_type *type = instr->data_type;
    struct hlsl_ir_node *arg, *neg, *max;
    struct hlsl_ir_expr *expr;

    if (instr->type != HLSL_IR_EXPR)
        return false;
    expr = hlsl_ir_expr(instr);

    if (expr->op != HLSL_OP1_ABS)
        return false;
    if (type->class != HLSL_CLASS_SCALAR && type->class != HLSL_CLASS_VECTOR)
        return false;
    if (type->e.numeric.type != HLSL_TYPE_INT)
        return false;

    arg = expr->operands[0].node;

    if (!(neg = hlsl_new_unary_expr(ctx, HLSL_OP1_NEG, arg, &instr->loc)))
        return false;
    hlsl_block_add_instr(block, neg);

    if (!(max = hlsl_new_binary_expr(ctx, HLSL_OP2_MAX, arg, neg)))
        return false;
    hlsl_block_add_instr(block, max);

    return true;
}

static bool lower_int_dot(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, struct hlsl_block *block)
{
    struct hlsl_ir_node *arg1, *arg2, *mult, *comps[4] = {0}, *res;
    struct hlsl_type *type = instr->data_type;
    struct hlsl_ir_expr *expr;
    unsigned int i, dimx;
    bool is_bool;

    if (instr->type != HLSL_IR_EXPR)
        return false;
    expr = hlsl_ir_expr(instr);

    if (expr->op != HLSL_OP2_DOT)
        return false;

    if (type->e.numeric.type == HLSL_TYPE_INT || type->e.numeric.type == HLSL_TYPE_UINT
            || type->e.numeric.type == HLSL_TYPE_BOOL)
    {
        arg1 = expr->operands[0].node;
        arg2 = expr->operands[1].node;
        VKD3D_ASSERT(arg1->data_type->dimx == arg2->data_type->dimx);
        dimx = arg1->data_type->dimx;
        is_bool = type->e.numeric.type == HLSL_TYPE_BOOL;

        if (!(mult = hlsl_new_binary_expr(ctx, is_bool ? HLSL_OP2_LOGIC_AND : HLSL_OP2_MUL, arg1, arg2)))
            return false;
        hlsl_block_add_instr(block, mult);

        for (i = 0; i < dimx; ++i)
        {
            uint32_t s = hlsl_swizzle_from_writemask(1 << i);

            if (!(comps[i] = hlsl_new_swizzle(ctx, s, 1, mult, &instr->loc)))
                return false;
            hlsl_block_add_instr(block, comps[i]);
        }

        res = comps[0];
        for (i = 1; i < dimx; ++i)
        {
            if (!(res = hlsl_new_binary_expr(ctx, is_bool ? HLSL_OP2_LOGIC_OR : HLSL_OP2_ADD, res, comps[i])))
                return false;
            hlsl_block_add_instr(block, res);
        }

        return true;
    }

    return false;
}

static bool lower_float_modulus(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, struct hlsl_block *block)
{
    struct hlsl_ir_node *arg1, *arg2, *mul1, *neg1, *ge, *neg2, *div, *mul2, *frc, *cond, *one, *mul3;
    struct hlsl_type *type = instr->data_type, *btype;
    struct hlsl_constant_value one_value;
    struct hlsl_ir_expr *expr;
    unsigned int i;

    if (instr->type != HLSL_IR_EXPR)
        return false;
    expr = hlsl_ir_expr(instr);
    arg1 = expr->operands[0].node;
    arg2 = expr->operands[1].node;
    if (expr->op != HLSL_OP2_MOD)
        return false;
    if (type->class != HLSL_CLASS_SCALAR && type->class != HLSL_CLASS_VECTOR)
        return false;
    if (type->e.numeric.type != HLSL_TYPE_FLOAT)
        return false;
    btype = hlsl_get_numeric_type(ctx, type->class, HLSL_TYPE_BOOL, type->dimx, type->dimy);

    if (!(mul1 = hlsl_new_binary_expr(ctx, HLSL_OP2_MUL, arg2, arg1)))
        return false;
    hlsl_block_add_instr(block, mul1);

    if (!(neg1 = hlsl_new_unary_expr(ctx, HLSL_OP1_NEG, mul1, &instr->loc)))
        return false;
    hlsl_block_add_instr(block, neg1);

    if (!(ge = hlsl_new_binary_expr(ctx, HLSL_OP2_GEQUAL, mul1, neg1)))
        return false;
    ge->data_type = btype;
    hlsl_block_add_instr(block, ge);

    if (!(neg2 = hlsl_new_unary_expr(ctx, HLSL_OP1_NEG, arg2, &instr->loc)))
        return false;
    hlsl_block_add_instr(block, neg2);

    if (!(cond = hlsl_add_conditional(ctx, block, ge, arg2, neg2)))
        return false;

    for (i = 0; i < type->dimx; ++i)
        one_value.u[i].f = 1.0f;
    if (!(one = hlsl_new_constant(ctx, type, &one_value, &instr->loc)))
        return false;
    hlsl_block_add_instr(block, one);

    if (!(div = hlsl_new_binary_expr(ctx, HLSL_OP2_DIV, one, cond)))
        return false;
    hlsl_block_add_instr(block, div);

    if (!(mul2 = hlsl_new_binary_expr(ctx, HLSL_OP2_MUL, div, arg1)))
        return false;
    hlsl_block_add_instr(block, mul2);

    if (!(frc = hlsl_new_unary_expr(ctx, HLSL_OP1_FRACT, mul2, &instr->loc)))
        return false;
    hlsl_block_add_instr(block, frc);

    if (!(mul3 = hlsl_new_binary_expr(ctx, HLSL_OP2_MUL, frc, cond)))
        return false;
    hlsl_block_add_instr(block, mul3);

    return true;
}

static bool lower_nonfloat_exprs(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, struct hlsl_block *block)
{
    struct hlsl_ir_expr *expr;

    if (instr->type != HLSL_IR_EXPR)
        return false;
    expr = hlsl_ir_expr(instr);
    if (expr->op == HLSL_OP1_CAST || instr->data_type->e.numeric.type == HLSL_TYPE_FLOAT)
        return false;

    switch (expr->op)
    {
        case HLSL_OP1_ABS:
        case HLSL_OP1_NEG:
        case HLSL_OP2_ADD:
        case HLSL_OP2_DIV:
        case HLSL_OP2_LOGIC_AND:
        case HLSL_OP2_LOGIC_OR:
        case HLSL_OP2_MAX:
        case HLSL_OP2_MIN:
        case HLSL_OP2_MUL:
        {
            struct hlsl_ir_node *operands[HLSL_MAX_OPERANDS] = {0};
            struct hlsl_ir_node *arg, *arg_cast, *float_expr, *ret;
            struct hlsl_type *float_type;
            unsigned int i;

            for (i = 0; i < HLSL_MAX_OPERANDS; ++i)
            {
                arg = expr->operands[i].node;
                if (!arg)
                    continue;

                float_type = hlsl_get_vector_type(ctx, HLSL_TYPE_FLOAT, arg->data_type->dimx);
                if (!(arg_cast = hlsl_new_cast(ctx, arg, float_type, &instr->loc)))
                    return false;
                hlsl_block_add_instr(block, arg_cast);

                operands[i] = arg_cast;
            }

            float_type = hlsl_get_vector_type(ctx, HLSL_TYPE_FLOAT, instr->data_type->dimx);
            if (!(float_expr = hlsl_new_expr(ctx, expr->op, operands, float_type, &instr->loc)))
                return false;
            hlsl_block_add_instr(block, float_expr);

            if (!(ret = hlsl_new_cast(ctx, float_expr, instr->data_type, &instr->loc)))
                return false;
            hlsl_block_add_instr(block, ret);

            return true;
        }
        default:
            return false;
    }
}

static bool lower_discard_neg(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, void *context)
{
    struct hlsl_ir_node *zero, *bool_false, *or, *cmp, *load;
    static const struct hlsl_constant_value zero_value;
    struct hlsl_type *arg_type, *cmp_type;
    struct hlsl_ir_node *operands[HLSL_MAX_OPERANDS] = { 0 };
    struct hlsl_ir_jump *jump;
    struct hlsl_block block;
    unsigned int i, count;

    if (instr->type != HLSL_IR_JUMP)
        return false;
    jump = hlsl_ir_jump(instr);
    if (jump->type != HLSL_IR_JUMP_DISCARD_NEG)
        return false;

    hlsl_block_init(&block);

    arg_type = jump->condition.node->data_type;
    if (!(zero = hlsl_new_constant(ctx, arg_type, &zero_value, &instr->loc)))
        return false;
    hlsl_block_add_instr(&block, zero);

    operands[0] = jump->condition.node;
    operands[1] = zero;
    cmp_type = hlsl_get_numeric_type(ctx, arg_type->class, HLSL_TYPE_BOOL, arg_type->dimx, arg_type->dimy);
    if (!(cmp = hlsl_new_expr(ctx, HLSL_OP2_LESS, operands, cmp_type, &instr->loc)))
        return false;
    hlsl_block_add_instr(&block, cmp);

    if (!(bool_false = hlsl_new_constant(ctx, hlsl_get_scalar_type(ctx, HLSL_TYPE_BOOL), &zero_value, &instr->loc)))
        return false;
    hlsl_block_add_instr(&block, bool_false);

    or = bool_false;

    count = hlsl_type_component_count(cmp_type);
    for (i = 0; i < count; ++i)
    {
        if (!(load = hlsl_add_load_component(ctx, &block, cmp, i, &instr->loc)))
            return false;

        if (!(or = hlsl_new_binary_expr(ctx, HLSL_OP2_LOGIC_OR, or, load)))
                return NULL;
        hlsl_block_add_instr(&block, or);
    }

    list_move_tail(&instr->entry, &block.instrs);
    hlsl_src_remove(&jump->condition);
    hlsl_src_from_node(&jump->condition, or);
    jump->type = HLSL_IR_JUMP_DISCARD_NZ;

    return true;
}

static bool lower_discard_nz(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, void *context)
{
    struct hlsl_ir_node *cond, *cond_cast, *abs, *neg;
    struct hlsl_type *float_type;
    struct hlsl_ir_jump *jump;
    struct hlsl_block block;

    if (instr->type != HLSL_IR_JUMP)
        return false;
    jump = hlsl_ir_jump(instr);
    if (jump->type != HLSL_IR_JUMP_DISCARD_NZ)
        return false;

    cond = jump->condition.node;
    float_type = hlsl_get_vector_type(ctx, HLSL_TYPE_FLOAT, cond->data_type->dimx);

    hlsl_block_init(&block);

    if (!(cond_cast = hlsl_new_cast(ctx, cond, float_type, &instr->loc)))
        return false;
    hlsl_block_add_instr(&block, cond_cast);

    if (!(abs = hlsl_new_unary_expr(ctx, HLSL_OP1_ABS, cond_cast, &instr->loc)))
        return false;
    hlsl_block_add_instr(&block, abs);

    if (!(neg = hlsl_new_unary_expr(ctx, HLSL_OP1_NEG, abs, &instr->loc)))
        return false;
    hlsl_block_add_instr(&block, neg);

    list_move_tail(&instr->entry, &block.instrs);
    hlsl_src_remove(&jump->condition);
    hlsl_src_from_node(&jump->condition, neg);
    jump->type = HLSL_IR_JUMP_DISCARD_NEG;

    return true;
}

static bool dce(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, void *context)
{
    switch (instr->type)
    {
        case HLSL_IR_CONSTANT:
        case HLSL_IR_COMPILE:
        case HLSL_IR_EXPR:
        case HLSL_IR_INDEX:
        case HLSL_IR_LOAD:
        case HLSL_IR_RESOURCE_LOAD:
        case HLSL_IR_STRING_CONSTANT:
        case HLSL_IR_SWIZZLE:
        case HLSL_IR_SAMPLER_STATE:
            if (list_empty(&instr->uses))
            {
                list_remove(&instr->entry);
                hlsl_free_instr(instr);
                return true;
            }
            break;

        case HLSL_IR_STORE:
        {
            struct hlsl_ir_store *store = hlsl_ir_store(instr);
            struct hlsl_ir_var *var = store->lhs.var;

            if (var->last_read < instr->index)
            {
                list_remove(&instr->entry);
                hlsl_free_instr(instr);
                return true;
            }
            break;
        }

        case HLSL_IR_CALL:
        case HLSL_IR_IF:
        case HLSL_IR_JUMP:
        case HLSL_IR_LOOP:
        case HLSL_IR_RESOURCE_STORE:
        case HLSL_IR_SWITCH:
            break;
        case HLSL_IR_STATEBLOCK_CONSTANT:
            /* Stateblock constants should not appear in the shader program. */
            vkd3d_unreachable();
        case HLSL_IR_VSIR_INSTRUCTION_REF:
            /* HLSL IR nodes are not translated to hlsl_ir_vsir_instruction_ref at this point. */
            vkd3d_unreachable();
    }

    return false;
}

static void dump_function(struct rb_entry *entry, void *context)
{
    struct hlsl_ir_function *func = RB_ENTRY_VALUE(entry, struct hlsl_ir_function, entry);
    struct hlsl_ir_function_decl *decl;
    struct hlsl_ctx *ctx = context;

    LIST_FOR_EACH_ENTRY(decl, &func->overloads, struct hlsl_ir_function_decl, entry)
    {
        if (decl->has_body)
            hlsl_dump_function(ctx, decl);
    }
}

static bool mark_indexable_var(struct hlsl_ctx *ctx, struct hlsl_deref *deref,
        struct hlsl_ir_node *instr)
{
    if (!deref->rel_offset.node)
        return false;

    VKD3D_ASSERT(deref->var);
    VKD3D_ASSERT(deref->rel_offset.node->type != HLSL_IR_CONSTANT);
    deref->var->indexable = true;

    return true;
}

static void mark_indexable_vars(struct hlsl_ctx *ctx, struct hlsl_ir_function_decl *entry_func)
{
    struct hlsl_scope *scope;
    struct hlsl_ir_var *var;

    LIST_FOR_EACH_ENTRY(scope, &ctx->scopes, struct hlsl_scope, entry)
    {
        LIST_FOR_EACH_ENTRY(var, &scope->vars, struct hlsl_ir_var, scope_entry)
            var->indexable = false;
    }

    transform_derefs(ctx, mark_indexable_var, &entry_func->body);
}

static char get_regset_name(enum hlsl_regset regset)
{
    switch (regset)
    {
        case HLSL_REGSET_SAMPLERS:
            return 's';
        case HLSL_REGSET_TEXTURES:
            return 't';
        case HLSL_REGSET_UAVS:
            return 'u';
        case HLSL_REGSET_NUMERIC:
            vkd3d_unreachable();
    }
    vkd3d_unreachable();
}

static void allocate_register_reservations(struct hlsl_ctx *ctx, struct list *extern_vars)
{
    struct hlsl_ir_var *var;

    LIST_FOR_EACH_ENTRY(var, extern_vars, struct hlsl_ir_var, extern_entry)
    {
        const struct hlsl_reg_reservation *reservation = &var->reg_reservation;
        unsigned int r;

        if (reservation->reg_type)
        {
            for (r = 0; r <= HLSL_REGSET_LAST_OBJECT; ++r)
            {
                if (var->regs[r].allocation_size > 0)
                {
                    if (reservation->reg_type != get_regset_name(r))
                    {
                        struct vkd3d_string_buffer *type_string;

                        /* We can throw this error because resources can only span across a single
                         * regset, but we have to check for multiple regsets if we support register
                         * reservations for structs for SM5. */
                        type_string = hlsl_type_to_string(ctx, var->data_type);
                        hlsl_error(ctx, &var->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_RESERVATION,
                                "Object of type '%s' must be bound to register type '%c'.",
                                type_string->buffer, get_regset_name(r));
                        hlsl_release_string_buffer(ctx, type_string);
                    }
                    else
                    {
                        var->regs[r].allocated = true;
                        var->regs[r].space = reservation->reg_space;
                        var->regs[r].index = reservation->reg_index;
                    }
                }
            }
        }
    }
}

static void deref_mark_last_read(struct hlsl_deref *deref, unsigned int last_read)
{
    unsigned int i;

    if (hlsl_deref_is_lowered(deref))
    {
        if (deref->rel_offset.node)
            deref->rel_offset.node->last_read = last_read;
    }
    else
    {
        for (i = 0; i < deref->path_len; ++i)
            deref->path[i].node->last_read = last_read;
    }
}

/* Compute the earliest and latest liveness for each variable. In the case that
 * a variable is accessed inside of a loop, we promote its liveness to extend
 * to at least the range of the entire loop. We also do this for nodes, so that
 * nodes produced before the loop have their temp register protected from being
 * overridden after the last read within an iteration. */
static void compute_liveness_recurse(struct hlsl_block *block, unsigned int loop_first, unsigned int loop_last)
{
    struct hlsl_ir_node *instr;
    struct hlsl_ir_var *var;

    LIST_FOR_EACH_ENTRY(instr, &block->instrs, struct hlsl_ir_node, entry)
    {
        const unsigned int last_read = loop_last ? max(instr->index, loop_last) : instr->index;

        switch (instr->type)
        {
        case HLSL_IR_CALL:
            /* We should have inlined all calls before computing liveness. */
            vkd3d_unreachable();
        case HLSL_IR_STATEBLOCK_CONSTANT:
            /* Stateblock constants should not appear in the shader program. */
            vkd3d_unreachable();
        case HLSL_IR_VSIR_INSTRUCTION_REF:
            /* HLSL IR nodes are not translated to hlsl_ir_vsir_instruction_ref at this point. */
            vkd3d_unreachable();

        case HLSL_IR_STORE:
        {
            struct hlsl_ir_store *store = hlsl_ir_store(instr);

            var = store->lhs.var;
            if (!var->first_write)
                var->first_write = loop_first ? min(instr->index, loop_first) : instr->index;
            store->rhs.node->last_read = last_read;
            deref_mark_last_read(&store->lhs, last_read);
            break;
        }
        case HLSL_IR_EXPR:
        {
            struct hlsl_ir_expr *expr = hlsl_ir_expr(instr);
            unsigned int i;

            for (i = 0; i < ARRAY_SIZE(expr->operands) && expr->operands[i].node; ++i)
                expr->operands[i].node->last_read = last_read;
            break;
        }
        case HLSL_IR_IF:
        {
            struct hlsl_ir_if *iff = hlsl_ir_if(instr);

            compute_liveness_recurse(&iff->then_block, loop_first, loop_last);
            compute_liveness_recurse(&iff->else_block, loop_first, loop_last);
            iff->condition.node->last_read = last_read;
            break;
        }
        case HLSL_IR_LOAD:
        {
            struct hlsl_ir_load *load = hlsl_ir_load(instr);

            var = load->src.var;
            var->last_read = max(var->last_read, last_read);
            deref_mark_last_read(&load->src, last_read);
            break;
        }
        case HLSL_IR_LOOP:
        {
            struct hlsl_ir_loop *loop = hlsl_ir_loop(instr);

            compute_liveness_recurse(&loop->body, loop_first ? loop_first : instr->index,
                    loop_last ? loop_last : loop->next_index);
            break;
        }
        case HLSL_IR_RESOURCE_LOAD:
        {
            struct hlsl_ir_resource_load *load = hlsl_ir_resource_load(instr);

            var = load->resource.var;
            var->last_read = max(var->last_read, last_read);
            deref_mark_last_read(&load->resource, last_read);

            if ((var = load->sampler.var))
            {
                var->last_read = max(var->last_read, last_read);
                deref_mark_last_read(&load->sampler, last_read);
            }

            if (load->coords.node)
                load->coords.node->last_read = last_read;
            if (load->texel_offset.node)
                load->texel_offset.node->last_read = last_read;
            if (load->lod.node)
                load->lod.node->last_read = last_read;
            if (load->ddx.node)
                load->ddx.node->last_read = last_read;
            if (load->ddy.node)
                load->ddy.node->last_read = last_read;
            if (load->sample_index.node)
                load->sample_index.node->last_read = last_read;
            if (load->cmp.node)
                load->cmp.node->last_read = last_read;
            break;
        }
        case HLSL_IR_RESOURCE_STORE:
        {
            struct hlsl_ir_resource_store *store = hlsl_ir_resource_store(instr);

            var = store->resource.var;
            var->last_read = max(var->last_read, last_read);
            deref_mark_last_read(&store->resource, last_read);
            store->coords.node->last_read = last_read;
            store->value.node->last_read = last_read;
            break;
        }
        case HLSL_IR_SWIZZLE:
        {
            struct hlsl_ir_swizzle *swizzle = hlsl_ir_swizzle(instr);

            swizzle->val.node->last_read = last_read;
            break;
        }
        case HLSL_IR_INDEX:
        {
            struct hlsl_ir_index *index = hlsl_ir_index(instr);

            index->val.node->last_read = last_read;
            index->idx.node->last_read = last_read;
            break;
        }
        case HLSL_IR_JUMP:
        {
            struct hlsl_ir_jump *jump = hlsl_ir_jump(instr);

            if (jump->condition.node)
                jump->condition.node->last_read = last_read;
            break;
        }
        case HLSL_IR_SWITCH:
        {
            struct hlsl_ir_switch *s = hlsl_ir_switch(instr);
            struct hlsl_ir_switch_case *c;

            LIST_FOR_EACH_ENTRY(c, &s->cases, struct hlsl_ir_switch_case, entry)
                compute_liveness_recurse(&c->body, loop_first, loop_last);
            s->selector.node->last_read = last_read;
            break;
        }
        case HLSL_IR_CONSTANT:
        case HLSL_IR_STRING_CONSTANT:
            break;
        case HLSL_IR_COMPILE:
        case HLSL_IR_SAMPLER_STATE:
            /* These types are skipped as they are only relevant to effects. */
            break;
        }
    }
}

static void init_var_liveness(struct hlsl_ir_var *var)
{
    if (var->is_uniform || var->is_input_semantic)
        var->first_write = 1;
    else if (var->is_output_semantic)
        var->last_read = UINT_MAX;
}

static void compute_liveness(struct hlsl_ctx *ctx, struct hlsl_ir_function_decl *entry_func)
{
    struct hlsl_scope *scope;
    struct hlsl_ir_var *var;

    index_instructions(&entry_func->body, 2);

    LIST_FOR_EACH_ENTRY(scope, &ctx->scopes, struct hlsl_scope, entry)
    {
        LIST_FOR_EACH_ENTRY(var, &scope->vars, struct hlsl_ir_var, scope_entry)
            var->first_write = var->last_read = 0;
    }

    LIST_FOR_EACH_ENTRY(var, &ctx->extern_vars, struct hlsl_ir_var, extern_entry)
        init_var_liveness(var);

    LIST_FOR_EACH_ENTRY(var, &entry_func->extern_vars, struct hlsl_ir_var, extern_entry)
        init_var_liveness(var);

    compute_liveness_recurse(&entry_func->body, 0, 0);
}

static void mark_vars_usage(struct hlsl_ctx *ctx)
{
    struct hlsl_scope *scope;
    struct hlsl_ir_var *var;

    LIST_FOR_EACH_ENTRY(scope, &ctx->scopes, struct hlsl_scope, entry)
    {
        LIST_FOR_EACH_ENTRY(var, &scope->vars, struct hlsl_ir_var, scope_entry)
        {
            if (var->last_read)
                var->is_read = true;
        }
    }
}

struct register_allocator
{
    struct allocation
    {
        uint32_t reg;
        unsigned int writemask;
        unsigned int first_write, last_read;

        /* Two allocations with different mode can't share the same register. */
        int mode;
    } *allocations;
    size_t count, capacity;

    /* Indexable temps are allocated separately and always keep their index regardless of their
     * lifetime. */
    size_t indexable_count;

    /* Total number of registers allocated so far. Used to declare sm4 temp count. */
    uint32_t reg_count;

    /* Special flag so allocations that can share registers prioritize those
     * that will result in smaller writemasks.
     * For instance, a single-register allocation would prefer to share a register
     * whose .xy components are already allocated (becoming .z) instead of a
     * register whose .xyz components are already allocated (becoming .w). */
    bool prioritize_smaller_writemasks;
};

static unsigned int get_available_writemask(const struct register_allocator *allocator,
        unsigned int first_write, unsigned int last_read, uint32_t reg_idx, int mode)
{
    unsigned int writemask = VKD3DSP_WRITEMASK_ALL;
    size_t i;

    for (i = 0; i < allocator->count; ++i)
    {
        const struct allocation *allocation = &allocator->allocations[i];

        /* We do not overlap if first write == last read:
         * this is the case where we are allocating the result of that
         * expression, e.g. "add r0, r0, r1". */

        if (allocation->reg == reg_idx
                && first_write < allocation->last_read && last_read > allocation->first_write)
        {
            writemask &= ~allocation->writemask;
            if (allocation->mode != mode)
                writemask = 0;
        }

        if (!writemask)
            break;
    }

    return writemask;
}

static void record_allocation(struct hlsl_ctx *ctx, struct register_allocator *allocator, uint32_t reg_idx,
        unsigned int writemask, unsigned int first_write, unsigned int last_read, int mode)
{
    struct allocation *allocation;

    if (!hlsl_array_reserve(ctx, (void **)&allocator->allocations, &allocator->capacity,
            allocator->count + 1, sizeof(*allocator->allocations)))
        return;

    allocation = &allocator->allocations[allocator->count++];
    allocation->reg = reg_idx;
    allocation->writemask = writemask;
    allocation->first_write = first_write;
    allocation->last_read = last_read;
    allocation->mode = mode;

    allocator->reg_count = max(allocator->reg_count, reg_idx + 1);
}

/* reg_size is the number of register components to be reserved, while component_count is the number
 * of components for the register's writemask. In SM1, floats and vectors allocate the whole
 * register, even if they don't use it completely. */
static struct hlsl_reg allocate_register(struct hlsl_ctx *ctx, struct register_allocator *allocator,
        unsigned int first_write, unsigned int last_read, unsigned int reg_size,
        unsigned int component_count, int mode, bool force_align)
{
    struct hlsl_reg ret = {.allocation_size = 1, .allocated = true};
    unsigned int required_size = force_align ? 4 : reg_size;
    unsigned int pref;

    VKD3D_ASSERT(component_count <= reg_size);

    pref = allocator->prioritize_smaller_writemasks ? 4 : required_size;
    for (; pref >= required_size; --pref)
    {
        for (uint32_t reg_idx = 0; reg_idx < allocator->reg_count; ++reg_idx)
        {
            unsigned int available_writemask = get_available_writemask(allocator,
                    first_write, last_read, reg_idx, mode);

            if (vkd3d_popcount(available_writemask) >= pref)
            {
                unsigned int writemask = hlsl_combine_writemasks(available_writemask,
                        vkd3d_write_mask_from_component_count(reg_size));

                ret.id = reg_idx;
                ret.writemask = hlsl_combine_writemasks(writemask,
                        vkd3d_write_mask_from_component_count(component_count));
                record_allocation(ctx, allocator, reg_idx, writemask, first_write, last_read, mode);
                return ret;
            }
        }
    }

    ret.id = allocator->reg_count;
    ret.writemask = vkd3d_write_mask_from_component_count(component_count);
    record_allocation(ctx, allocator, allocator->reg_count,
            vkd3d_write_mask_from_component_count(reg_size), first_write, last_read, mode);
    return ret;
}

/* Allocate a register with writemask, while reserving reg_writemask. */
static struct hlsl_reg allocate_register_with_masks(struct hlsl_ctx *ctx, struct register_allocator *allocator,
        unsigned int first_write, unsigned int last_read, uint32_t reg_writemask, uint32_t writemask, int mode)
{
    struct hlsl_reg ret = {0};
    uint32_t reg_idx;

    VKD3D_ASSERT((reg_writemask & writemask) == writemask);

    for (reg_idx = 0;; ++reg_idx)
    {
        if ((get_available_writemask(allocator, first_write, last_read,
                reg_idx, mode) & reg_writemask) == reg_writemask)
            break;
    }

    record_allocation(ctx, allocator, reg_idx, reg_writemask, first_write, last_read, mode);

    ret.id = reg_idx;
    ret.allocation_size = 1;
    ret.writemask = writemask;
    ret.allocated = true;
    return ret;
}

static bool is_range_available(const struct register_allocator *allocator, unsigned int first_write,
        unsigned int last_read, uint32_t reg_idx, unsigned int reg_size, int mode)
{
    unsigned int last_reg_mask = (1u << (reg_size % 4)) - 1;
    unsigned int writemask;
    uint32_t i;

    for (i = 0; i < (reg_size / 4); ++i)
    {
        writemask = get_available_writemask(allocator, first_write, last_read, reg_idx + i, mode);
        if (writemask != VKD3DSP_WRITEMASK_ALL)
            return false;
    }
    writemask = get_available_writemask(allocator, first_write, last_read, reg_idx + (reg_size / 4), mode);
    if ((writemask & last_reg_mask) != last_reg_mask)
        return false;
    return true;
}

static struct hlsl_reg allocate_range(struct hlsl_ctx *ctx, struct register_allocator *allocator,
        unsigned int first_write, unsigned int last_read, unsigned int reg_size, int mode)
{
    struct hlsl_reg ret = {0};
    uint32_t reg_idx;
    unsigned int i;

    for (reg_idx = 0;; ++reg_idx)
    {
        if (is_range_available(allocator, first_write, last_read, reg_idx, reg_size, mode))
            break;
    }

    for (i = 0; i < reg_size / 4; ++i)
        record_allocation(ctx, allocator, reg_idx + i, VKD3DSP_WRITEMASK_ALL, first_write, last_read, mode);
    if (reg_size % 4)
        record_allocation(ctx, allocator, reg_idx + (reg_size / 4),
                (1u << (reg_size % 4)) - 1, first_write, last_read, mode);

    ret.id = reg_idx;
    ret.allocation_size = align(reg_size, 4) / 4;
    ret.allocated = true;
    return ret;
}

static struct hlsl_reg allocate_numeric_registers_for_type(struct hlsl_ctx *ctx, struct register_allocator *allocator,
        unsigned int first_write, unsigned int last_read, const struct hlsl_type *type)
{
    unsigned int reg_size = type->reg_size[HLSL_REGSET_NUMERIC];

    /* FIXME: We could potentially pack structs or arrays more efficiently... */

    if (type->class <= HLSL_CLASS_VECTOR)
        return allocate_register(ctx, allocator, first_write, last_read, type->dimx, type->dimx, 0, false);
    else
        return allocate_range(ctx, allocator, first_write, last_read, reg_size, 0);
}

static const char *debug_register(char class, struct hlsl_reg reg, const struct hlsl_type *type)
{
    static const char writemask_offset[] = {'w','x','y','z'};
    unsigned int reg_size = type->reg_size[HLSL_REGSET_NUMERIC];

    if (reg_size > 4)
    {
        if (reg_size & 3)
            return vkd3d_dbg_sprintf("%c%u-%c%u.%c", class, reg.id, class, reg.id + (reg_size / 4),
                    writemask_offset[reg_size & 3]);

        return vkd3d_dbg_sprintf("%c%u-%c%u", class, reg.id, class, reg.id + (reg_size / 4) - 1);
    }
    return vkd3d_dbg_sprintf("%c%u%s", class, reg.id, debug_hlsl_writemask(reg.writemask));
}

static bool track_object_components_sampler_dim(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, void *context)
{
    struct hlsl_ir_resource_load *load;
    struct hlsl_ir_var *var;
    enum hlsl_regset regset;
    unsigned int index;

    if (instr->type != HLSL_IR_RESOURCE_LOAD)
        return false;

    load = hlsl_ir_resource_load(instr);
    var = load->resource.var;

    regset = hlsl_deref_get_regset(ctx, &load->resource);
    if (!hlsl_regset_index_from_deref(ctx, &load->resource, regset, &index))
        return false;

    if (regset == HLSL_REGSET_SAMPLERS)
    {
        enum hlsl_sampler_dim dim;

        VKD3D_ASSERT(!load->sampler.var);

        dim = var->objects_usage[regset][index].sampler_dim;
        if (dim != load->sampling_dim)
        {
            if (dim == HLSL_SAMPLER_DIM_GENERIC)
            {
                var->objects_usage[regset][index].first_sampler_dim_loc = instr->loc;
            }
            else
            {
                hlsl_error(ctx, &instr->loc, VKD3D_SHADER_ERROR_HLSL_INCONSISTENT_SAMPLER,
                        "Inconsistent generic sampler usage dimension.");
                hlsl_note(ctx, &var->objects_usage[regset][index].first_sampler_dim_loc,
                        VKD3D_SHADER_LOG_ERROR, "First use is here.");
                return false;
            }
        }
    }
    var->objects_usage[regset][index].sampler_dim = load->sampling_dim;

    return false;
}

static void register_deref_usage(struct hlsl_ctx *ctx, struct hlsl_deref *deref)
{
    struct hlsl_ir_var *var = deref->var;
    enum hlsl_regset regset = hlsl_deref_get_regset(ctx, deref);
    uint32_t required_bind_count;
    struct hlsl_type *type;
    unsigned int index;

    if (!hlsl_regset_index_from_deref(ctx, deref, regset, &index))
        return;

    if (regset <= HLSL_REGSET_LAST_OBJECT)
    {
        var->objects_usage[regset][index].used = true;
        var->bind_count[regset] = max(var->bind_count[regset], index + 1);
    }
    else if (regset == HLSL_REGSET_NUMERIC)
    {
        type = hlsl_deref_get_type(ctx, deref);

        hlsl_regset_index_from_deref(ctx, deref, regset, &index);
        required_bind_count = align(index + type->reg_size[regset], 4) / 4;
        var->bind_count[regset] = max(var->bind_count[regset], required_bind_count);
    }
    else
    {
        vkd3d_unreachable();
    }
}

static bool track_components_usage(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, void *context)
{
    switch (instr->type)
    {
        case HLSL_IR_LOAD:
        {
            struct hlsl_ir_load *load = hlsl_ir_load(instr);

            if (!load->src.var->is_uniform)
                return false;

            /* These will are handled by validate_static_object_references(). */
            if (hlsl_deref_get_regset(ctx, &load->src) != HLSL_REGSET_NUMERIC)
                return false;

            register_deref_usage(ctx, &load->src);
            break;
        }

        case HLSL_IR_RESOURCE_LOAD:
            register_deref_usage(ctx, &hlsl_ir_resource_load(instr)->resource);
            if (hlsl_ir_resource_load(instr)->sampler.var)
                register_deref_usage(ctx, &hlsl_ir_resource_load(instr)->sampler);
            break;

        case HLSL_IR_RESOURCE_STORE:
            register_deref_usage(ctx, &hlsl_ir_resource_store(instr)->resource);
            break;

        default:
            break;
    }

    return false;
}

static void calculate_resource_register_counts(struct hlsl_ctx *ctx)
{
    struct hlsl_ir_var *var;
    struct hlsl_type *type;
    unsigned int k;

    LIST_FOR_EACH_ENTRY(var, &ctx->extern_vars, struct hlsl_ir_var, extern_entry)
    {
        type = var->data_type;

        for (k = 0; k <= HLSL_REGSET_LAST_OBJECT; ++k)
        {
            bool is_separated = var->is_separated_resource;

            if (var->bind_count[k] > 0)
                var->regs[k].allocation_size = (k == HLSL_REGSET_SAMPLERS || is_separated) ? var->bind_count[k] : type->reg_size[k];
        }
    }
}

static void allocate_instr_temp_register(struct hlsl_ctx *ctx,
        struct hlsl_ir_node *instr, struct register_allocator *allocator)
{
    unsigned int reg_writemask = 0, dst_writemask = 0;

    if (instr->reg.allocated || !instr->last_read)
        return;

    if (instr->type == HLSL_IR_EXPR)
    {
        switch (hlsl_ir_expr(instr)->op)
        {
            case HLSL_OP1_COS_REDUCED:
                dst_writemask = VKD3DSP_WRITEMASK_0;
                reg_writemask = ctx->profile->major_version < 3 ? (1 << 3) - 1 : VKD3DSP_WRITEMASK_0;
                break;

            case HLSL_OP1_SIN_REDUCED:
                dst_writemask = VKD3DSP_WRITEMASK_1;
                reg_writemask = ctx->profile->major_version < 3 ? (1 << 3) - 1 : VKD3DSP_WRITEMASK_1;
                break;

            default:
                break;
        }
    }

    if (reg_writemask)
        instr->reg = allocate_register_with_masks(ctx, allocator,
                instr->index, instr->last_read, reg_writemask, dst_writemask, 0);
    else
        instr->reg = allocate_numeric_registers_for_type(ctx, allocator,
                instr->index, instr->last_read, instr->data_type);

    TRACE("Allocated anonymous expression @%u to %s (liveness %u-%u).\n", instr->index,
            debug_register('r', instr->reg, instr->data_type), instr->index, instr->last_read);
}

static void allocate_variable_temp_register(struct hlsl_ctx *ctx,
        struct hlsl_ir_var *var, struct register_allocator *allocator)
{
    if (var->is_input_semantic || var->is_output_semantic || var->is_uniform)
        return;

    if (!var->regs[HLSL_REGSET_NUMERIC].allocated && var->last_read)
    {
        if (var->indexable)
        {
            var->regs[HLSL_REGSET_NUMERIC].id = allocator->indexable_count++;
            var->regs[HLSL_REGSET_NUMERIC].allocation_size = 1;
            var->regs[HLSL_REGSET_NUMERIC].writemask = 0;
            var->regs[HLSL_REGSET_NUMERIC].allocated = true;

            TRACE("Allocated %s to x%u[].\n", var->name, var->regs[HLSL_REGSET_NUMERIC].id);
        }
        else
        {
            var->regs[HLSL_REGSET_NUMERIC] = allocate_numeric_registers_for_type(ctx, allocator,
                    var->first_write, var->last_read, var->data_type);

            TRACE("Allocated %s to %s (liveness %u-%u).\n", var->name, debug_register('r',
                    var->regs[HLSL_REGSET_NUMERIC], var->data_type), var->first_write, var->last_read);
        }
    }
}

static void allocate_temp_registers_recurse(struct hlsl_ctx *ctx,
        struct hlsl_block *block, struct register_allocator *allocator)
{
    struct hlsl_ir_node *instr;

    LIST_FOR_EACH_ENTRY(instr, &block->instrs, struct hlsl_ir_node, entry)
    {
        /* In SM4 all constants are inlined. */
        if (ctx->profile->major_version >= 4 && instr->type == HLSL_IR_CONSTANT)
            continue;

        allocate_instr_temp_register(ctx, instr, allocator);

        switch (instr->type)
        {
            case HLSL_IR_IF:
            {
                struct hlsl_ir_if *iff = hlsl_ir_if(instr);
                allocate_temp_registers_recurse(ctx, &iff->then_block, allocator);
                allocate_temp_registers_recurse(ctx, &iff->else_block, allocator);
                break;
            }

            case HLSL_IR_LOAD:
            {
                struct hlsl_ir_load *load = hlsl_ir_load(instr);
                /* We need to at least allocate a variable for undefs.
                 * FIXME: We should probably find a way to remove them instead. */
                allocate_variable_temp_register(ctx, load->src.var, allocator);
                break;
            }

            case HLSL_IR_LOOP:
            {
                struct hlsl_ir_loop *loop = hlsl_ir_loop(instr);
                allocate_temp_registers_recurse(ctx, &loop->body, allocator);
                break;
            }

            case HLSL_IR_STORE:
            {
                struct hlsl_ir_store *store = hlsl_ir_store(instr);
                allocate_variable_temp_register(ctx, store->lhs.var, allocator);
                break;
            }

            case HLSL_IR_SWITCH:
            {
                struct hlsl_ir_switch *s = hlsl_ir_switch(instr);
                struct hlsl_ir_switch_case *c;

                LIST_FOR_EACH_ENTRY(c, &s->cases, struct hlsl_ir_switch_case, entry)
                {
                    allocate_temp_registers_recurse(ctx, &c->body, allocator);
                }
                break;
            }

            default:
                break;
        }
    }
}

static void record_constant(struct hlsl_ctx *ctx, unsigned int component_index, float f,
        const struct vkd3d_shader_location *loc)
{
    struct hlsl_constant_defs *defs = &ctx->constant_defs;
    struct hlsl_constant_register *reg;
    size_t i;

    for (i = 0; i < defs->count; ++i)
    {
        reg = &defs->regs[i];
        if (reg->index == (component_index / 4))
        {
            reg->value.f[component_index % 4] = f;
            return;
        }
    }

    if (!hlsl_array_reserve(ctx, (void **)&defs->regs, &defs->size, defs->count + 1, sizeof(*defs->regs)))
        return;
    reg = &defs->regs[defs->count++];
    memset(reg, 0, sizeof(*reg));
    reg->index = component_index / 4;
    reg->value.f[component_index % 4] = f;
    reg->loc = *loc;
}

static void allocate_const_registers_recurse(struct hlsl_ctx *ctx,
        struct hlsl_block *block, struct register_allocator *allocator)
{
    struct hlsl_ir_node *instr;

    LIST_FOR_EACH_ENTRY(instr, &block->instrs, struct hlsl_ir_node, entry)
    {
        switch (instr->type)
        {
            case HLSL_IR_CONSTANT:
            {
                struct hlsl_ir_constant *constant = hlsl_ir_constant(instr);
                const struct hlsl_type *type = instr->data_type;
                unsigned int x, i;

                constant->reg = allocate_numeric_registers_for_type(ctx, allocator, 1, UINT_MAX, type);
                TRACE("Allocated constant @%u to %s.\n", instr->index, debug_register('c', constant->reg, type));

                VKD3D_ASSERT(hlsl_is_numeric_type(type));
                VKD3D_ASSERT(type->dimy == 1);
                VKD3D_ASSERT(constant->reg.writemask);

                for (x = 0, i = 0; x < 4; ++x)
                {
                    const union hlsl_constant_value_component *value;
                    float f;

                    if (!(constant->reg.writemask & (1u << x)))
                        continue;
                    value = &constant->value.u[i++];

                    switch (type->e.numeric.type)
                    {
                        case HLSL_TYPE_BOOL:
                            f = !!value->u;
                            break;

                        case HLSL_TYPE_FLOAT:
                        case HLSL_TYPE_HALF:
                            f = value->f;
                            break;

                        case HLSL_TYPE_INT:
                            f = value->i;
                            break;

                        case HLSL_TYPE_UINT:
                            f = value->u;
                            break;

                        case HLSL_TYPE_DOUBLE:
                            FIXME("Double constant.\n");
                            return;

                        default:
                            vkd3d_unreachable();
                    }

                    record_constant(ctx, constant->reg.id * 4 + x, f, &constant->node.loc);
                }

                break;
            }

            case HLSL_IR_IF:
            {
                struct hlsl_ir_if *iff = hlsl_ir_if(instr);
                allocate_const_registers_recurse(ctx, &iff->then_block, allocator);
                allocate_const_registers_recurse(ctx, &iff->else_block, allocator);
                break;
            }

            case HLSL_IR_LOOP:
            {
                struct hlsl_ir_loop *loop = hlsl_ir_loop(instr);
                allocate_const_registers_recurse(ctx, &loop->body, allocator);
                break;
            }

            case HLSL_IR_SWITCH:
            {
                struct hlsl_ir_switch *s = hlsl_ir_switch(instr);
                struct hlsl_ir_switch_case *c;

                LIST_FOR_EACH_ENTRY(c, &s->cases, struct hlsl_ir_switch_case, entry)
                {
                    allocate_const_registers_recurse(ctx, &c->body, allocator);
                }
                break;
            }

            default:
                break;
        }
    }
}

static void sort_uniform_by_numeric_bind_count(struct list *sorted, struct hlsl_ir_var *to_sort)
{
    struct hlsl_ir_var *var;

    list_remove(&to_sort->extern_entry);

    LIST_FOR_EACH_ENTRY(var, sorted, struct hlsl_ir_var, extern_entry)
    {
        uint32_t to_sort_size = to_sort->bind_count[HLSL_REGSET_NUMERIC];
        uint32_t var_size = var->bind_count[HLSL_REGSET_NUMERIC];

        if (to_sort_size > var_size)
        {
            list_add_before(&var->extern_entry, &to_sort->extern_entry);
            return;
        }
    }

    list_add_tail(sorted, &to_sort->extern_entry);
}

static void sort_uniforms_by_numeric_bind_count(struct hlsl_ctx *ctx)
{
    struct list sorted = LIST_INIT(sorted);
    struct hlsl_ir_var *var, *next;

    LIST_FOR_EACH_ENTRY_SAFE(var, next, &ctx->extern_vars, struct hlsl_ir_var, extern_entry)
    {
        if (var->is_uniform)
            sort_uniform_by_numeric_bind_count(&sorted, var);
    }
    list_move_tail(&ctx->extern_vars, &sorted);
}

/* In SM2, 'sincos' expects specific constants as src1 and src2 arguments.
 * These have to be referenced directly, i.e. as 'c' not 'r'. */
static void allocate_sincos_const_registers(struct hlsl_ctx *ctx, struct hlsl_block *block,
        struct register_allocator *allocator)
{
    const struct hlsl_ir_node *instr;
    struct hlsl_type *type;

    if (ctx->profile->major_version >= 3)
        return;

    LIST_FOR_EACH_ENTRY(instr, &block->instrs, struct hlsl_ir_node, entry)
    {
        if (instr->type == HLSL_IR_EXPR && (hlsl_ir_expr(instr)->op == HLSL_OP1_SIN_REDUCED
                || hlsl_ir_expr(instr)->op == HLSL_OP1_COS_REDUCED))
        {
            type = hlsl_get_vector_type(ctx, HLSL_TYPE_FLOAT, 4);

            ctx->d3dsincosconst1 = allocate_numeric_registers_for_type(ctx, allocator, 1, UINT_MAX, type);
            TRACE("Allocated D3DSINCOSCONST1 to %s.\n", debug_register('c', ctx->d3dsincosconst1, type));
            record_constant(ctx, ctx->d3dsincosconst1.id * 4 + 0, -1.55009923e-06f, &instr->loc);
            record_constant(ctx, ctx->d3dsincosconst1.id * 4 + 1, -2.17013894e-05f, &instr->loc);
            record_constant(ctx, ctx->d3dsincosconst1.id * 4 + 2,  2.60416674e-03f, &instr->loc);
            record_constant(ctx, ctx->d3dsincosconst1.id * 4 + 3,  2.60416680e-04f, &instr->loc);

            ctx->d3dsincosconst2 = allocate_numeric_registers_for_type(ctx, allocator, 1, UINT_MAX, type);
            TRACE("Allocated D3DSINCOSCONST2 to %s.\n", debug_register('c', ctx->d3dsincosconst2, type));
            record_constant(ctx, ctx->d3dsincosconst2.id * 4 + 0, -2.08333340e-02f, &instr->loc);
            record_constant(ctx, ctx->d3dsincosconst2.id * 4 + 1, -1.25000000e-01f, &instr->loc);
            record_constant(ctx, ctx->d3dsincosconst2.id * 4 + 2,  1.00000000e+00f, &instr->loc);
            record_constant(ctx, ctx->d3dsincosconst2.id * 4 + 3,  5.00000000e-01f, &instr->loc);

            return;
        }
    }
}

static void allocate_const_registers(struct hlsl_ctx *ctx, struct hlsl_ir_function_decl *entry_func)
{
    struct register_allocator allocator_used = {0};
    struct register_allocator allocator = {0};
    struct hlsl_ir_var *var;

    sort_uniforms_by_numeric_bind_count(ctx);

    LIST_FOR_EACH_ENTRY(var, &ctx->extern_vars, struct hlsl_ir_var, extern_entry)
    {
        unsigned int reg_size = var->data_type->reg_size[HLSL_REGSET_NUMERIC];
        unsigned int bind_count = var->bind_count[HLSL_REGSET_NUMERIC];

        if (!var->is_uniform || reg_size == 0)
            continue;

        if (var->reg_reservation.reg_type == 'c')
        {
            unsigned int reg_idx = var->reg_reservation.reg_index;
            unsigned int i;

            VKD3D_ASSERT(reg_size % 4 == 0);
            for (i = 0; i < reg_size / 4; ++i)
            {
                if (i < bind_count)
                {
                    if (get_available_writemask(&allocator_used, 1, UINT_MAX, reg_idx + i, 0) != VKD3DSP_WRITEMASK_ALL)
                    {
                        hlsl_error(ctx, &var->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_RESERVATION,
                                "Overlapping register() reservations on 'c%u'.", reg_idx + i);
                    }
                    record_allocation(ctx, &allocator_used, reg_idx + i, VKD3DSP_WRITEMASK_ALL, 1, UINT_MAX, 0);
                }
                record_allocation(ctx, &allocator, reg_idx + i, VKD3DSP_WRITEMASK_ALL, 1, UINT_MAX, 0);
            }

            var->regs[HLSL_REGSET_NUMERIC].id = reg_idx;
            var->regs[HLSL_REGSET_NUMERIC].allocation_size = reg_size / 4;
            var->regs[HLSL_REGSET_NUMERIC].writemask = VKD3DSP_WRITEMASK_ALL;
            var->regs[HLSL_REGSET_NUMERIC].allocated = true;
            TRACE("Allocated reserved %s to %s.\n", var->name,
                    debug_register('c', var->regs[HLSL_REGSET_NUMERIC], var->data_type));
        }
    }

    vkd3d_free(allocator_used.allocations);

    LIST_FOR_EACH_ENTRY(var, &ctx->extern_vars, struct hlsl_ir_var, extern_entry)
    {
        unsigned int alloc_size = 4 * var->bind_count[HLSL_REGSET_NUMERIC];

        if (!var->is_uniform || alloc_size == 0)
            continue;

        if (!var->regs[HLSL_REGSET_NUMERIC].allocated)
        {
            var->regs[HLSL_REGSET_NUMERIC] = allocate_range(ctx, &allocator, 1, UINT_MAX, alloc_size, 0);
            TRACE("Allocated %s to %s.\n", var->name,
                    debug_register('c', var->regs[HLSL_REGSET_NUMERIC], var->data_type));
        }
    }

    allocate_const_registers_recurse(ctx, &entry_func->body, &allocator);

    allocate_sincos_const_registers(ctx, &entry_func->body, &allocator);

    vkd3d_free(allocator.allocations);
}

/* Simple greedy temporary register allocation pass that just assigns a unique
 * index to all (simultaneously live) variables or intermediate values. Agnostic
 * as to how many registers are actually available for the current backend, and
 * does not handle constants. */
static uint32_t allocate_temp_registers(struct hlsl_ctx *ctx, struct hlsl_ir_function_decl *entry_func)
{
    struct register_allocator allocator = {0};
    struct hlsl_scope *scope;
    struct hlsl_ir_var *var;

    /* Reset variable temp register allocations. */
    LIST_FOR_EACH_ENTRY(scope, &ctx->scopes, struct hlsl_scope, entry)
    {
        LIST_FOR_EACH_ENTRY(var, &scope->vars, struct hlsl_ir_var, scope_entry)
        {
            if (!(var->is_input_semantic || var->is_output_semantic || var->is_uniform))
                memset(var->regs, 0, sizeof(var->regs));
        }
    }

    /* ps_1_* outputs are special and go in temp register 0. */
    if (ctx->profile->major_version == 1 && ctx->profile->type == VKD3D_SHADER_TYPE_PIXEL)
    {
        size_t i;

        for (i = 0; i < entry_func->parameters.count; ++i)
        {
            var = entry_func->parameters.vars[i];
            if (var->is_output_semantic)
            {
                record_allocation(ctx, &allocator, 0, VKD3DSP_WRITEMASK_ALL, var->first_write, var->last_read, 0);
                break;
            }
        }
    }

    allocate_temp_registers_recurse(ctx, &entry_func->body, &allocator);
    vkd3d_free(allocator.allocations);

    return allocator.reg_count;
}

enum vkd3d_shader_interpolation_mode sm4_get_interpolation_mode(struct hlsl_type *type, unsigned int storage_modifiers)
{
    unsigned int i;

    static const struct
    {
        unsigned int modifiers;
        enum vkd3d_shader_interpolation_mode mode;
    }
    modes[] =
    {
        {HLSL_STORAGE_CENTROID | HLSL_STORAGE_NOPERSPECTIVE, VKD3DSIM_LINEAR_NOPERSPECTIVE_CENTROID},
        {HLSL_STORAGE_NOPERSPECTIVE, VKD3DSIM_LINEAR_NOPERSPECTIVE},
        {HLSL_STORAGE_CENTROID, VKD3DSIM_LINEAR_CENTROID},
        {HLSL_STORAGE_CENTROID | HLSL_STORAGE_LINEAR, VKD3DSIM_LINEAR_CENTROID},
    };

    if ((storage_modifiers & HLSL_STORAGE_NOINTERPOLATION)
            || base_type_get_semantic_equivalent(type->e.numeric.type) == HLSL_TYPE_UINT)
        return VKD3DSIM_CONSTANT;

    for (i = 0; i < ARRAY_SIZE(modes); ++i)
    {
        if ((storage_modifiers & modes[i].modifiers) == modes[i].modifiers)
            return modes[i].mode;
    }

    return VKD3DSIM_LINEAR;
}

static void allocate_semantic_register(struct hlsl_ctx *ctx, struct hlsl_ir_var *var,
        struct register_allocator *allocator, bool output, bool optimize, bool is_patch_constant_func)
{
    static const char *const shader_names[] =
    {
        [VKD3D_SHADER_TYPE_PIXEL] = "Pixel",
        [VKD3D_SHADER_TYPE_VERTEX] = "Vertex",
        [VKD3D_SHADER_TYPE_GEOMETRY] = "Geometry",
        [VKD3D_SHADER_TYPE_HULL] = "Hull",
        [VKD3D_SHADER_TYPE_DOMAIN] = "Domain",
        [VKD3D_SHADER_TYPE_COMPUTE] = "Compute",
    };

    enum vkd3d_shader_register_type type;
    struct vkd3d_shader_version version;
    uint32_t reg;
    bool builtin;

    VKD3D_ASSERT(var->semantic.name);

    version.major = ctx->profile->major_version;
    version.minor = ctx->profile->minor_version;
    version.type = ctx->profile->type;

    if (version.major < 4)
    {
        enum vkd3d_decl_usage usage;
        uint32_t usage_idx;

        /* ps_1_* outputs are special and go in temp register 0. */
        if (version.major == 1 && output && version.type == VKD3D_SHADER_TYPE_PIXEL)
            return;

        builtin = sm1_register_from_semantic_name(&version,
                var->semantic.name, var->semantic.index, output, &type, &reg);
        if (!builtin && !sm1_usage_from_semantic_name(var->semantic.name, var->semantic.index, &usage, &usage_idx))
        {
            hlsl_error(ctx, &var->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_SEMANTIC,
                    "Invalid semantic '%s'.", var->semantic.name);
            return;
        }

        if ((!output && !var->last_read) || (output && !var->first_write))
            return;
    }
    else
    {
        enum vkd3d_shader_sysval_semantic semantic;
        bool has_idx;

        if (!sm4_sysval_semantic_from_semantic_name(&semantic, &version, ctx->semantic_compat_mapping,
                ctx->domain, var->semantic.name, var->semantic.index, output, is_patch_constant_func))
        {
            hlsl_error(ctx, &var->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_SEMANTIC,
                    "Invalid semantic '%s'.", var->semantic.name);
            return;
        }

        if ((builtin = sm4_register_from_semantic_name(&version, var->semantic.name, output, &type, &has_idx)))
            reg = has_idx ? var->semantic.index : 0;

        if (semantic == VKD3D_SHADER_SV_TESS_FACTOR_TRIINT)
        {
            /* While SV_InsideTessFactor can be declared as 'float' for "tri"
             * domains, it is allocated as if it was 'float[1]'. */
            var->force_align = true;
        }
    }

    if (builtin)
    {
        TRACE("%s %s semantic %s[%u] matches predefined register %#x[%u].\n", shader_names[version.type],
                output ? "output" : "input", var->semantic.name, var->semantic.index, type, reg);
    }
    else
    {
        int mode = (ctx->profile->major_version < 4)
                ? 0 : sm4_get_interpolation_mode(var->data_type, var->storage_modifiers);
        unsigned int reg_size = optimize ? var->data_type->dimx : 4;

        var->regs[HLSL_REGSET_NUMERIC] = allocate_register(ctx, allocator, 1,
                UINT_MAX, reg_size, var->data_type->dimx, mode, var->force_align);

        TRACE("Allocated %s to %s (mode %d).\n", var->name, debug_register(output ? 'o' : 'v',
                var->regs[HLSL_REGSET_NUMERIC], var->data_type), mode);
    }
}

static void allocate_semantic_registers(struct hlsl_ctx *ctx, struct hlsl_ir_function_decl *entry_func)
{
    struct register_allocator input_allocator = {0}, output_allocator = {0};
    bool is_vertex_shader = ctx->profile->type == VKD3D_SHADER_TYPE_VERTEX;
    bool is_pixel_shader = ctx->profile->type == VKD3D_SHADER_TYPE_PIXEL;
    bool is_patch_constant_func = entry_func == ctx->patch_constant_func;
    struct hlsl_ir_var *var;

    input_allocator.prioritize_smaller_writemasks = true;
    output_allocator.prioritize_smaller_writemasks = true;

    LIST_FOR_EACH_ENTRY(var, &entry_func->extern_vars, struct hlsl_ir_var, extern_entry)
    {
        if (var->is_input_semantic)
            allocate_semantic_register(ctx, var, &input_allocator, false, !is_vertex_shader, is_patch_constant_func);
        if (var->is_output_semantic)
            allocate_semantic_register(ctx, var, &output_allocator, true, !is_pixel_shader, is_patch_constant_func);
    }

    vkd3d_free(input_allocator.allocations);
    vkd3d_free(output_allocator.allocations);
}

static const struct hlsl_buffer *get_reserved_buffer(struct hlsl_ctx *ctx,
        uint32_t space, uint32_t index, bool allocated_only)
{
    const struct hlsl_buffer *buffer;

    LIST_FOR_EACH_ENTRY(buffer, &ctx->buffers, const struct hlsl_buffer, entry)
    {
        if (buffer->reservation.reg_type == 'b'
                && buffer->reservation.reg_space == space && buffer->reservation.reg_index == index)
        {
            if (allocated_only && !buffer->reg.allocated)
                continue;

            return buffer;
        }
    }
    return NULL;
}

static void hlsl_calculate_buffer_offset(struct hlsl_ctx *ctx, struct hlsl_ir_var *var, bool register_reservation)
{
    unsigned int var_reg_size = var->data_type->reg_size[HLSL_REGSET_NUMERIC];
    enum hlsl_type_class var_class = var->data_type->class;
    struct hlsl_buffer *buffer = var->buffer;

    if (register_reservation)
    {
        var->buffer_offset = 4 * var->reg_reservation.reg_index;
        var->has_explicit_bind_point = 1;
    }
    else
    {
        if (var->reg_reservation.offset_type == 'c')
        {
            if (var->reg_reservation.offset_index % 4)
            {
                if (var_class == HLSL_CLASS_MATRIX)
                {
                    hlsl_error(ctx, &var->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_RESERVATION,
                            "packoffset() reservations with matrix types must be aligned with the beginning of a register.");
                }
                else if (var_class == HLSL_CLASS_ARRAY)
                {
                    hlsl_error(ctx, &var->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_RESERVATION,
                            "packoffset() reservations with array types must be aligned with the beginning of a register.");
                }
                else if (var_class == HLSL_CLASS_STRUCT)
                {
                    hlsl_error(ctx, &var->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_RESERVATION,
                            "packoffset() reservations with struct types must be aligned with the beginning of a register.");
                }
                else if (var_class == HLSL_CLASS_VECTOR)
                {
                    unsigned int aligned_offset = hlsl_type_get_sm4_offset(var->data_type, var->reg_reservation.offset_index);

                    if (var->reg_reservation.offset_index != aligned_offset)
                        hlsl_error(ctx, &var->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_RESERVATION,
                                "packoffset() reservations with vector types cannot span multiple registers.");
                }
            }
            var->buffer_offset = var->reg_reservation.offset_index;
            var->has_explicit_bind_point = 1;
        }
        else
        {
            var->buffer_offset = hlsl_type_get_sm4_offset(var->data_type, buffer->size);
        }
    }

    TRACE("Allocated buffer offset %u to %s.\n", var->buffer_offset, var->name);
    buffer->size = max(buffer->size, var->buffer_offset + var_reg_size);
    if (var->is_read)
        buffer->used_size = max(buffer->used_size, var->buffer_offset + var_reg_size);
}

static void validate_buffer_offsets(struct hlsl_ctx *ctx)
{
    struct hlsl_ir_var *var1, *var2;
    struct hlsl_buffer *buffer;

    LIST_FOR_EACH_ENTRY(var1, &ctx->extern_vars, struct hlsl_ir_var, extern_entry)
    {
        if (!var1->is_uniform || hlsl_type_is_resource(var1->data_type))
            continue;

        buffer = var1->buffer;
        if (!buffer->used_size)
            continue;

        LIST_FOR_EACH_ENTRY(var2, &ctx->extern_vars, struct hlsl_ir_var, extern_entry)
        {
            unsigned int var1_reg_size, var2_reg_size;

            if (!var2->is_uniform || hlsl_type_is_resource(var2->data_type))
                continue;

            if (var1 == var2 || var1->buffer != var2->buffer)
                continue;

            /* This is to avoid reporting the error twice for the same pair of overlapping variables. */
            if (strcmp(var1->name, var2->name) >= 0)
                continue;

            var1_reg_size = var1->data_type->reg_size[HLSL_REGSET_NUMERIC];
            var2_reg_size = var2->data_type->reg_size[HLSL_REGSET_NUMERIC];

            if (var1->buffer_offset < var2->buffer_offset + var2_reg_size
                    && var2->buffer_offset < var1->buffer_offset + var1_reg_size)
                hlsl_error(ctx, &buffer->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_RESERVATION,
                        "Invalid packoffset() reservation: Variables %s and %s overlap.",
                        var1->name, var2->name);
        }
    }

    LIST_FOR_EACH_ENTRY(var1, &ctx->extern_vars, struct hlsl_ir_var, extern_entry)
    {
        buffer = var1->buffer;
        if (!buffer || buffer == ctx->globals_buffer)
            continue;

        if (var1->reg_reservation.offset_type
                || var1->reg_reservation.reg_type == 's'
                || var1->reg_reservation.reg_type == 't'
                || var1->reg_reservation.reg_type == 'u')
            buffer->manually_packed_elements = true;
        else
            buffer->automatically_packed_elements = true;

        if (buffer->manually_packed_elements && buffer->automatically_packed_elements)
        {
            hlsl_error(ctx, &buffer->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_RESERVATION,
                    "packoffset() must be specified for all the buffer elements, or none of them.");
            break;
        }
    }
}

void hlsl_calculate_buffer_offsets(struct hlsl_ctx *ctx)
{
    struct hlsl_ir_var *var;

    LIST_FOR_EACH_ENTRY(var, &ctx->extern_vars, struct hlsl_ir_var, extern_entry)
    {
        if (!var->is_uniform || hlsl_type_is_resource(var->data_type))
            continue;

        if (hlsl_var_has_buffer_offset_register_reservation(ctx, var))
            hlsl_calculate_buffer_offset(ctx, var, true);
    }

    LIST_FOR_EACH_ENTRY(var, &ctx->extern_vars, struct hlsl_ir_var, extern_entry)
    {
        if (!var->is_uniform || hlsl_type_is_resource(var->data_type))
            continue;

        if (!hlsl_var_has_buffer_offset_register_reservation(ctx, var))
            hlsl_calculate_buffer_offset(ctx, var, false);
    }
}

static unsigned int get_max_cbuffer_reg_index(struct hlsl_ctx *ctx)
{
    if (hlsl_version_ge(ctx, 5, 1))
        return UINT_MAX;

    return 13;
}

static void allocate_buffers(struct hlsl_ctx *ctx)
{
    struct hlsl_buffer *buffer;
    uint32_t index = 0, id = 0;
    struct hlsl_ir_var *var;

    LIST_FOR_EACH_ENTRY(var, &ctx->extern_vars, struct hlsl_ir_var, extern_entry)
    {
        if (!var->is_uniform || hlsl_type_is_resource(var->data_type))
            continue;

        if (var->is_param)
            var->buffer = ctx->params_buffer;
    }

    hlsl_calculate_buffer_offsets(ctx);
    validate_buffer_offsets(ctx);

    LIST_FOR_EACH_ENTRY(buffer, &ctx->buffers, struct hlsl_buffer, entry)
    {
        if (!buffer->used_size)
            continue;

        if (buffer->type == HLSL_BUFFER_CONSTANT)
        {
            const struct hlsl_reg_reservation *reservation = &buffer->reservation;

            if (reservation->reg_type == 'b')
            {
                const struct hlsl_buffer *allocated_buffer = get_reserved_buffer(ctx,
                        reservation->reg_space, reservation->reg_index, true);
                unsigned int max_index = get_max_cbuffer_reg_index(ctx);

                if (buffer->reservation.reg_index > max_index)
                    hlsl_error(ctx, &buffer->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_RESERVATION,
                            "Buffer reservation cb%u exceeds target's maximum (cb%u).",
                            buffer->reservation.reg_index, max_index);

                if (allocated_buffer && allocated_buffer != buffer)
                {
                    hlsl_error(ctx, &buffer->loc, VKD3D_SHADER_ERROR_HLSL_OVERLAPPING_RESERVATIONS,
                            "Multiple buffers bound to space %u, index %u.",
                            reservation->reg_space, reservation->reg_index);
                    hlsl_note(ctx, &allocated_buffer->loc, VKD3D_SHADER_LOG_ERROR,
                            "Buffer %s is already bound to space %u, index %u.",
                            allocated_buffer->name, reservation->reg_space, reservation->reg_index);
                }

                buffer->reg.space = reservation->reg_space;
                buffer->reg.index = reservation->reg_index;
                if (hlsl_version_ge(ctx, 5, 1))
                    buffer->reg.id = id++;
                else
                    buffer->reg.id = buffer->reg.index;
                buffer->reg.allocation_size = 1;
                buffer->reg.allocated = true;
                TRACE("Allocated reserved %s to space %u, index %u, id %u.\n",
                        buffer->name, buffer->reg.space, buffer->reg.index, buffer->reg.id);
            }
            else if (!reservation->reg_type)
            {
                unsigned int max_index = get_max_cbuffer_reg_index(ctx);
                while (get_reserved_buffer(ctx, 0, index, false))
                    ++index;

                if (index > max_index)
                    hlsl_error(ctx, &buffer->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_RESERVATION,
                        "Too many buffers reserved, target's maximum is %u.", max_index);

                buffer->reg.space = 0;
                buffer->reg.index = index;
                if (hlsl_version_ge(ctx, 5, 1))
                    buffer->reg.id = id++;
                else
                    buffer->reg.id = buffer->reg.index;
                buffer->reg.allocation_size = 1;
                buffer->reg.allocated = true;
                TRACE("Allocated %s to space 0, index %u, id %u.\n", buffer->name, buffer->reg.index, buffer->reg.id);
                ++index;
            }
            else
            {
                hlsl_error(ctx, &buffer->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_RESERVATION,
                        "Constant buffers must be allocated to register type 'b'.");
            }
        }
        else
        {
            FIXME("Allocate registers for texture buffers.\n");
        }
    }
}

static const struct hlsl_ir_var *get_allocated_object(struct hlsl_ctx *ctx, enum hlsl_regset regset,
        uint32_t space, uint32_t index, bool allocated_only)
{
    const struct hlsl_ir_var *var;
    unsigned int start, count;

    LIST_FOR_EACH_ENTRY(var, &ctx->extern_vars, const struct hlsl_ir_var, extern_entry)
    {
        if (var->reg_reservation.reg_type == get_regset_name(regset)
                && var->data_type->reg_size[regset])
        {
            /* Vars with a reservation prevent non-reserved vars from being
             * bound there even if the reserved vars aren't used. */
            start = var->reg_reservation.reg_index;
            count = var->data_type->reg_size[regset];

            if (var->reg_reservation.reg_space != space)
                continue;

            if (!var->regs[regset].allocated && allocated_only)
                continue;
        }
        else if (var->regs[regset].allocated)
        {
            if (var->regs[regset].space != space)
                continue;

            start = var->regs[regset].index;
            count = var->regs[regset].allocation_size;
        }
        else
        {
            continue;
        }

        if (start <= index && index < start + count)
            return var;
    }
    return NULL;
}

static void allocate_objects(struct hlsl_ctx *ctx, struct hlsl_ir_function_decl *func, enum hlsl_regset regset)
{
    char regset_name = get_regset_name(regset);
    uint32_t min_index = 0, id = 0;
    struct hlsl_ir_var *var;

    if (regset == HLSL_REGSET_UAVS && ctx->profile->type == VKD3D_SHADER_TYPE_PIXEL)
    {
        LIST_FOR_EACH_ENTRY(var, &func->extern_vars, struct hlsl_ir_var, extern_entry)
        {
            if (var->semantic.name && (!ascii_strcasecmp(var->semantic.name, "color")
                    || !ascii_strcasecmp(var->semantic.name, "sv_target")))
                min_index = max(min_index, var->semantic.index + 1);
        }
    }

    LIST_FOR_EACH_ENTRY(var, &ctx->extern_vars, struct hlsl_ir_var, extern_entry)
    {
        unsigned int count = var->regs[regset].allocation_size;

        if (count == 0)
            continue;

        /* The variable was already allocated if it has a reservation. */
        if (var->regs[regset].allocated)
        {
            const struct hlsl_ir_var *reserved_object, *last_reported = NULL;
            unsigned int i;

            if (var->regs[regset].index < min_index)
            {
                VKD3D_ASSERT(regset == HLSL_REGSET_UAVS);
                hlsl_error(ctx, &var->loc, VKD3D_SHADER_ERROR_HLSL_OVERLAPPING_RESERVATIONS,
                        "UAV index (%u) must be higher than the maximum render target index (%u).",
                        var->regs[regset].index, min_index - 1);
                continue;
            }

            for (i = 0; i < count; ++i)
            {
                unsigned int space = var->regs[regset].space;
                unsigned int index = var->regs[regset].index + i;

                /* get_allocated_object() may return "var" itself, but we
                 * actually want that, otherwise we'll end up reporting the
                 * same conflict between the same two variables twice. */
                reserved_object = get_allocated_object(ctx, regset, space, index, true);
                if (reserved_object && reserved_object != var && reserved_object != last_reported)
                {
                    hlsl_error(ctx, &var->loc, VKD3D_SHADER_ERROR_HLSL_OVERLAPPING_RESERVATIONS,
                            "Multiple variables bound to space %u, %c%u.", regset_name, space, index);
                    hlsl_note(ctx, &reserved_object->loc, VKD3D_SHADER_LOG_ERROR,
                            "Variable '%s' is already bound to space %u, %c%u.",
                            reserved_object->name, regset_name, space, index);
                    last_reported = reserved_object;
                }
            }

            if (hlsl_version_ge(ctx, 5, 1))
                var->regs[regset].id = id++;
            else
                var->regs[regset].id = var->regs[regset].index;
            TRACE("Allocated reserved variable %s to space %u, indices %c%u-%c%u, id %u.\n",
                    var->name, var->regs[regset].space, regset_name, var->regs[regset].index,
                    regset_name, var->regs[regset].index + count, var->regs[regset].id);
        }
        else
        {
            unsigned int index = min_index;
            unsigned int available = 0;

            while (available < count)
            {
                if (get_allocated_object(ctx, regset, 0, index, false))
                    available = 0;
                else
                    ++available;
                ++index;
            }
            index -= count;

            var->regs[regset].space = 0;
            var->regs[regset].index = index;
            if (hlsl_version_ge(ctx, 5, 1))
                var->regs[regset].id = id++;
            else
                var->regs[regset].id = var->regs[regset].index;
            var->regs[regset].allocated = true;
            TRACE("Allocated variable %s to space 0, indices %c%u-%c%u, id %u.\n", var->name,
                    regset_name, index, regset_name, index + count, var->regs[regset].id);
            ++index;
        }
    }
}

bool hlsl_component_index_range_from_deref(struct hlsl_ctx *ctx, const struct hlsl_deref *deref,
        unsigned int *start, unsigned int *count)
{
    struct hlsl_type *type = deref->var->data_type;
    unsigned int i, k;

    *start = 0;
    *count = 0;

    for (i = 0; i < deref->path_len; ++i)
    {
        struct hlsl_ir_node *path_node = deref->path[i].node;
        unsigned int idx = 0;

        VKD3D_ASSERT(path_node);
        if (path_node->type != HLSL_IR_CONSTANT)
            return false;

        /* We should always have generated a cast to UINT. */
        VKD3D_ASSERT(path_node->data_type->class == HLSL_CLASS_SCALAR
                && path_node->data_type->e.numeric.type == HLSL_TYPE_UINT);

        idx = hlsl_ir_constant(path_node)->value.u[0].u;

        switch (type->class)
        {
            case HLSL_CLASS_VECTOR:
                if (idx >= type->dimx)
                    return false;
                *start += idx;
                break;

            case HLSL_CLASS_MATRIX:
                if (idx >= hlsl_type_major_size(type))
                    return false;
                if (hlsl_type_is_row_major(type))
                    *start += idx * type->dimx;
                else
                    *start += idx * type->dimy;
                break;

            case HLSL_CLASS_ARRAY:
                if (idx >= type->e.array.elements_count)
                    return false;
                *start += idx * hlsl_type_component_count(type->e.array.type);
                break;

            case HLSL_CLASS_STRUCT:
                for (k = 0; k < idx; ++k)
                    *start += hlsl_type_component_count(type->e.record.fields[k].type);
                break;

            default:
                vkd3d_unreachable();
        }

        type = hlsl_get_element_type_from_path_index(ctx, type, path_node);
    }

    *count = hlsl_type_component_count(type);
    return true;
}

/* Retrieves true if the index is constant, and false otherwise. In the latter case, the maximum
 * possible index is retrieved, assuming there is not out-of-bounds access. */
bool hlsl_regset_index_from_deref(struct hlsl_ctx *ctx, const struct hlsl_deref *deref,
        enum hlsl_regset regset, unsigned int *index)
{
    struct hlsl_type *type = deref->var->data_type;
    bool index_is_constant = true;
    unsigned int i;

    *index = 0;

    for (i = 0; i < deref->path_len; ++i)
    {
        struct hlsl_ir_node *path_node = deref->path[i].node;
        unsigned int idx = 0;

        VKD3D_ASSERT(path_node);
        if (path_node->type == HLSL_IR_CONSTANT)
        {
            /* We should always have generated a cast to UINT. */
            VKD3D_ASSERT(path_node->data_type->class == HLSL_CLASS_SCALAR
                    && path_node->data_type->e.numeric.type == HLSL_TYPE_UINT);

            idx = hlsl_ir_constant(path_node)->value.u[0].u;

            switch (type->class)
            {
                case HLSL_CLASS_ARRAY:
                    if (idx >= type->e.array.elements_count)
                        return false;

                    *index += idx * type->e.array.type->reg_size[regset];
                    break;

                case HLSL_CLASS_STRUCT:
                    *index += type->e.record.fields[idx].reg_offset[regset];
                    break;

                case HLSL_CLASS_MATRIX:
                    *index += 4 * idx;
                    break;

                default:
                    vkd3d_unreachable();
            }
        }
        else
        {
            index_is_constant = false;

            switch (type->class)
            {
                case HLSL_CLASS_ARRAY:
                    idx = type->e.array.elements_count - 1;
                    *index += idx * type->e.array.type->reg_size[regset];
                    break;

                case HLSL_CLASS_MATRIX:
                    idx = hlsl_type_major_size(type) - 1;
                    *index += idx * 4;
                    break;

                default:
                    vkd3d_unreachable();
            }
        }

        type = hlsl_get_element_type_from_path_index(ctx, type, path_node);
    }

    VKD3D_ASSERT(!(regset <= HLSL_REGSET_LAST_OBJECT) || (type->reg_size[regset] == 1));
    VKD3D_ASSERT(!(regset == HLSL_REGSET_NUMERIC) || type->reg_size[regset] <= 4);
    return index_is_constant;
}

bool hlsl_offset_from_deref(struct hlsl_ctx *ctx, const struct hlsl_deref *deref, unsigned int *offset)
{
    enum hlsl_regset regset = hlsl_deref_get_regset(ctx, deref);
    struct hlsl_ir_node *offset_node = deref->rel_offset.node;
    unsigned int size;

    *offset = deref->const_offset;

    if (offset_node)
    {
        /* We should always have generated a cast to UINT. */
        VKD3D_ASSERT(offset_node->data_type->class == HLSL_CLASS_SCALAR
                && offset_node->data_type->e.numeric.type == HLSL_TYPE_UINT);
        VKD3D_ASSERT(offset_node->type != HLSL_IR_CONSTANT);
        return false;
    }

    size = deref->var->data_type->reg_size[regset];
    if (*offset >= size)
    {
        /* FIXME: Report a more specific location for the constant deref. */
        hlsl_error(ctx, &deref->var->loc, VKD3D_SHADER_ERROR_HLSL_OFFSET_OUT_OF_BOUNDS,
                "Dereference is out of bounds. %u/%u", *offset, size);
        return false;
    }

    return true;
}

unsigned int hlsl_offset_from_deref_safe(struct hlsl_ctx *ctx, const struct hlsl_deref *deref)
{
    unsigned int offset;

    if (hlsl_offset_from_deref(ctx, deref, &offset))
        return offset;

    if (deref->rel_offset.node)
        hlsl_fixme(ctx, &deref->rel_offset.node->loc, "Dereference with non-constant offset of type %s.",
                hlsl_node_type_to_string(deref->rel_offset.node->type));

    return 0;
}

struct hlsl_reg hlsl_reg_from_deref(struct hlsl_ctx *ctx, const struct hlsl_deref *deref)
{
    const struct hlsl_ir_var *var = deref->var;
    struct hlsl_reg ret = var->regs[HLSL_REGSET_NUMERIC];
    unsigned int offset = hlsl_offset_from_deref_safe(ctx, deref);

    VKD3D_ASSERT(deref->data_type);
    VKD3D_ASSERT(hlsl_is_numeric_type(deref->data_type));

    ret.index += offset / 4;
    ret.id += offset / 4;

    ret.writemask = 0xf & (0xf << (offset % 4));
    if (var->regs[HLSL_REGSET_NUMERIC].writemask)
        ret.writemask = hlsl_combine_writemasks(var->regs[HLSL_REGSET_NUMERIC].writemask, ret.writemask);

    return ret;
}

static const char *get_string_argument_value(struct hlsl_ctx *ctx, const struct hlsl_attribute *attr, unsigned int i)
{
    const struct hlsl_ir_node *instr = attr->args[i].node;
    const struct hlsl_type *type = instr->data_type;

    if (type->class != HLSL_CLASS_STRING)
    {
        struct vkd3d_string_buffer *string;

        if ((string = hlsl_type_to_string(ctx, type)))
            hlsl_error(ctx, &instr->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_TYPE,
                    "Wrong type for the argument %u of [%s]: expected string, but got %s.",
                    i, attr->name, string->buffer);
        hlsl_release_string_buffer(ctx, string);
        return NULL;
    }

    return hlsl_ir_string_constant(instr)->string;
}

static void parse_numthreads_attribute(struct hlsl_ctx *ctx, const struct hlsl_attribute *attr)
{
    unsigned int i;

    ctx->found_numthreads = 1;

    if (attr->args_count != 3)
    {
        hlsl_error(ctx, &attr->loc, VKD3D_SHADER_ERROR_HLSL_WRONG_PARAMETER_COUNT,
                "Expected 3 parameters for [numthreads] attribute, but got %u.", attr->args_count);
        return;
    }

    for (i = 0; i < attr->args_count; ++i)
    {
        const struct hlsl_ir_node *instr = attr->args[i].node;
        const struct hlsl_type *type = instr->data_type;
        const struct hlsl_ir_constant *constant;

        if (type->class != HLSL_CLASS_SCALAR
                || (type->e.numeric.type != HLSL_TYPE_INT && type->e.numeric.type != HLSL_TYPE_UINT))
        {
            struct vkd3d_string_buffer *string;

            if ((string = hlsl_type_to_string(ctx, type)))
                hlsl_error(ctx, &instr->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_TYPE,
                        "Wrong type for argument %u of [numthreads]: expected int or uint, but got %s.",
                        i, string->buffer);
            hlsl_release_string_buffer(ctx, string);
            break;
        }

        if (instr->type != HLSL_IR_CONSTANT)
        {
            hlsl_fixme(ctx, &instr->loc, "Non-constant expression in [numthreads] initializer.");
            break;
        }
        constant = hlsl_ir_constant(instr);

        if ((type->e.numeric.type == HLSL_TYPE_INT && constant->value.u[0].i <= 0)
                || (type->e.numeric.type == HLSL_TYPE_UINT && !constant->value.u[0].u))
            hlsl_error(ctx, &instr->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_THREAD_COUNT,
                    "Thread count must be a positive integer.");

        ctx->thread_count[i] = constant->value.u[0].u;
    }
}

static void parse_domain_attribute(struct hlsl_ctx *ctx, const struct hlsl_attribute *attr)
{
    const char *value;

    if (attr->args_count != 1)
    {
        hlsl_error(ctx, &attr->loc, VKD3D_SHADER_ERROR_HLSL_WRONG_PARAMETER_COUNT,
                "Expected 1 parameter for [domain] attribute, but got %u.", attr->args_count);
        return;
    }

    if (!(value = get_string_argument_value(ctx, attr, 0)))
        return;

    if (!strcmp(value, "isoline"))
        ctx->domain = VKD3D_TESSELLATOR_DOMAIN_LINE;
    else if (!strcmp(value, "tri"))
        ctx->domain = VKD3D_TESSELLATOR_DOMAIN_TRIANGLE;
    else if (!strcmp(value, "quad"))
        ctx->domain = VKD3D_TESSELLATOR_DOMAIN_QUAD;
    else
        hlsl_error(ctx, &attr->args[0].node->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_DOMAIN,
                "Invalid tessellator domain \"%s\": expected \"isoline\", \"tri\", or \"quad\".",
                value);
}

static void parse_outputcontrolpoints_attribute(struct hlsl_ctx *ctx, const struct hlsl_attribute *attr)
{
    const struct hlsl_ir_node *instr;
    const struct hlsl_type *type;
    const struct hlsl_ir_constant *constant;

    if (attr->args_count != 1)
    {
        hlsl_error(ctx, &attr->loc, VKD3D_SHADER_ERROR_HLSL_WRONG_PARAMETER_COUNT,
                "Expected 1 parameter for [outputcontrolpoints] attribute, but got %u.", attr->args_count);
        return;
    }

    instr = attr->args[0].node;
    type = instr->data_type;

    if (type->class != HLSL_CLASS_SCALAR
            || (type->e.numeric.type != HLSL_TYPE_INT && type->e.numeric.type != HLSL_TYPE_UINT))
    {
        struct vkd3d_string_buffer *string;

        if ((string = hlsl_type_to_string(ctx, type)))
            hlsl_error(ctx, &instr->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_TYPE,
                    "Wrong type for argument 0 of [outputcontrolpoints]: expected int or uint, but got %s.",
                    string->buffer);
        hlsl_release_string_buffer(ctx, string);
        return;
    }

    if (instr->type != HLSL_IR_CONSTANT)
    {
        hlsl_fixme(ctx, &instr->loc, "Non-constant expression in [outputcontrolpoints] initializer.");
        return;
    }
    constant = hlsl_ir_constant(instr);

    if ((type->e.numeric.type == HLSL_TYPE_INT && constant->value.u[0].i < 0)
            || constant->value.u[0].u > 32)
        hlsl_error(ctx, &instr->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_CONTROL_POINT_COUNT,
                "Output control point count must be between 0 and 32.");

    ctx->output_control_point_count = constant->value.u[0].u;
}

static void parse_outputtopology_attribute(struct hlsl_ctx *ctx, const struct hlsl_attribute *attr)
{
    const char *value;

    if (attr->args_count != 1)
    {
        hlsl_error(ctx, &attr->loc, VKD3D_SHADER_ERROR_HLSL_WRONG_PARAMETER_COUNT,
                "Expected 1 parameter for [outputtopology] attribute, but got %u.", attr->args_count);
        return;
    }

    if (!(value = get_string_argument_value(ctx, attr, 0)))
        return;

    if (!strcmp(value, "point"))
        ctx->output_primitive = VKD3D_SHADER_TESSELLATOR_OUTPUT_POINT;
    else if (!strcmp(value, "line"))
        ctx->output_primitive = VKD3D_SHADER_TESSELLATOR_OUTPUT_LINE;
    else if (!strcmp(value, "triangle_cw"))
        ctx->output_primitive = VKD3D_SHADER_TESSELLATOR_OUTPUT_TRIANGLE_CW;
    else if (!strcmp(value, "triangle_ccw"))
        ctx->output_primitive = VKD3D_SHADER_TESSELLATOR_OUTPUT_TRIANGLE_CCW;
    else
        hlsl_error(ctx, &attr->args[0].node->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_OUTPUT_PRIMITIVE,
                "Invalid tessellator output topology \"%s\": "
                "expected \"point\", \"line\", \"triangle_cw\", or \"triangle_ccw\".", value);
}

static void parse_partitioning_attribute(struct hlsl_ctx *ctx, const struct hlsl_attribute *attr)
{
    const char *value;

    if (attr->args_count != 1)
    {
        hlsl_error(ctx, &attr->loc, VKD3D_SHADER_ERROR_HLSL_WRONG_PARAMETER_COUNT,
                "Expected 1 parameter for [partitioning] attribute, but got %u.", attr->args_count);
        return;
    }

    if (!(value = get_string_argument_value(ctx, attr, 0)))
        return;

    if (!strcmp(value, "integer"))
        ctx->partitioning = VKD3D_SHADER_TESSELLATOR_PARTITIONING_INTEGER;
    else if (!strcmp(value, "pow2"))
        ctx->partitioning = VKD3D_SHADER_TESSELLATOR_PARTITIONING_POW2;
    else if (!strcmp(value, "fractional_even"))
        ctx->partitioning = VKD3D_SHADER_TESSELLATOR_PARTITIONING_FRACTIONAL_EVEN;
    else if (!strcmp(value, "fractional_odd"))
        ctx->partitioning = VKD3D_SHADER_TESSELLATOR_PARTITIONING_FRACTIONAL_ODD;
    else
        hlsl_error(ctx, &attr->args[0].node->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_PARTITIONING,
                "Invalid tessellator partitioning \"%s\": "
                "expected \"integer\", \"pow2\", \"fractional_even\", or \"fractional_odd\".", value);
}

static void parse_patchconstantfunc_attribute(struct hlsl_ctx *ctx, const struct hlsl_attribute *attr)
{
    const char *name;
    struct hlsl_ir_function *func;
    struct hlsl_ir_function_decl *decl;

    if (attr->args_count != 1)
    {
        hlsl_error(ctx, &attr->loc, VKD3D_SHADER_ERROR_HLSL_WRONG_PARAMETER_COUNT,
                "Expected 1 parameter for [patchconstantfunc] attribute, but got %u.", attr->args_count);
        return;
    }

    if (!(name = get_string_argument_value(ctx, attr, 0)))
        return;

    ctx->patch_constant_func = NULL;
    if ((func = hlsl_get_function(ctx, name)))
    {
        /* Pick the last overload with a body. */
        LIST_FOR_EACH_ENTRY_REV(decl, &func->overloads, struct hlsl_ir_function_decl, entry)
        {
            if (decl->has_body)
            {
                ctx->patch_constant_func = decl;
                break;
            }
        }
    }

    if (!ctx->patch_constant_func)
        hlsl_error(ctx, &attr->loc, VKD3D_SHADER_ERROR_HLSL_NOT_DEFINED,
                "Patch constant function \"%s\" is not defined.", name);
}

static void parse_entry_function_attributes(struct hlsl_ctx *ctx, struct hlsl_ir_function_decl *entry_func)
{
    const struct hlsl_profile_info *profile = ctx->profile;
    unsigned int i;

    for (i = 0; i < entry_func->attr_count; ++i)
    {
        const struct hlsl_attribute *attr = entry_func->attrs[i];

        if (!strcmp(attr->name, "numthreads") && profile->type == VKD3D_SHADER_TYPE_COMPUTE)
            parse_numthreads_attribute(ctx, attr);
        else if (!strcmp(attr->name, "domain")
                    && (profile->type == VKD3D_SHADER_TYPE_HULL || profile->type == VKD3D_SHADER_TYPE_DOMAIN))
            parse_domain_attribute(ctx, attr);
        else if (!strcmp(attr->name, "outputcontrolpoints") && profile->type == VKD3D_SHADER_TYPE_HULL)
            parse_outputcontrolpoints_attribute(ctx, attr);
        else if (!strcmp(attr->name, "outputtopology") && profile->type == VKD3D_SHADER_TYPE_HULL)
            parse_outputtopology_attribute(ctx, attr);
        else if (!strcmp(attr->name, "partitioning") && profile->type == VKD3D_SHADER_TYPE_HULL)
            parse_partitioning_attribute(ctx, attr);
        else if (!strcmp(attr->name, "patchconstantfunc") && profile->type == VKD3D_SHADER_TYPE_HULL)
            parse_patchconstantfunc_attribute(ctx, attr);
        else if (!strcmp(attr->name, "earlydepthstencil") && profile->type == VKD3D_SHADER_TYPE_PIXEL)
            entry_func->early_depth_test = true;
        else
            hlsl_warning(ctx, &entry_func->attrs[i]->loc, VKD3D_SHADER_WARNING_HLSL_UNKNOWN_ATTRIBUTE,
                    "Ignoring unknown attribute \"%s\".", entry_func->attrs[i]->name);
    }
}

static void validate_hull_shader_attributes(struct hlsl_ctx *ctx, const struct hlsl_ir_function_decl *entry_func)
{
    if (ctx->domain == VKD3D_TESSELLATOR_DOMAIN_INVALID)
    {
        hlsl_error(ctx, &entry_func->loc, VKD3D_SHADER_ERROR_HLSL_MISSING_ATTRIBUTE,
                "Entry point \"%s\" is missing a [domain] attribute.", entry_func->func->name);
    }

    if (ctx->output_control_point_count == UINT_MAX)
    {
        hlsl_error(ctx, &entry_func->loc, VKD3D_SHADER_ERROR_HLSL_MISSING_ATTRIBUTE,
                "Entry point \"%s\" is missing a [outputcontrolpoints] attribute.", entry_func->func->name);
    }

    if (!ctx->output_primitive)
    {
        hlsl_error(ctx, &entry_func->loc, VKD3D_SHADER_ERROR_HLSL_MISSING_ATTRIBUTE,
                "Entry point \"%s\" is missing a [outputtopology] attribute.", entry_func->func->name);
    }

    if (!ctx->partitioning)
    {
        hlsl_error(ctx, &entry_func->loc, VKD3D_SHADER_ERROR_HLSL_MISSING_ATTRIBUTE,
                "Entry point \"%s\" is missing a [partitioning] attribute.", entry_func->func->name);
    }

    if (!ctx->patch_constant_func)
    {
        hlsl_error(ctx, &entry_func->loc, VKD3D_SHADER_ERROR_HLSL_MISSING_ATTRIBUTE,
                "Entry point \"%s\" is missing a [patchconstantfunc] attribute.", entry_func->func->name);
    }
    else if (ctx->patch_constant_func == entry_func)
    {
        hlsl_error(ctx, &entry_func->loc, VKD3D_SHADER_ERROR_HLSL_RECURSIVE_CALL,
                "Patch constant function cannot be the entry point function.");
        /* Native returns E_NOTIMPL instead of E_FAIL here. */
        ctx->result = VKD3D_ERROR_NOT_IMPLEMENTED;
        return;
    }

    switch (ctx->domain)
    {
        case VKD3D_TESSELLATOR_DOMAIN_LINE:
            if (ctx->output_primitive == VKD3D_SHADER_TESSELLATOR_OUTPUT_TRIANGLE_CW
                    || ctx->output_primitive == VKD3D_SHADER_TESSELLATOR_OUTPUT_TRIANGLE_CCW)
                hlsl_error(ctx, &entry_func->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_OUTPUT_PRIMITIVE,
                        "Triangle output topologies are not available for isoline domains.");
            break;

        case VKD3D_TESSELLATOR_DOMAIN_TRIANGLE:
            if (ctx->output_primitive == VKD3D_SHADER_TESSELLATOR_OUTPUT_LINE)
                hlsl_error(ctx, &entry_func->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_OUTPUT_PRIMITIVE,
                        "Line output topologies are not available for triangle domains.");
            break;

        case VKD3D_TESSELLATOR_DOMAIN_QUAD:
            if (ctx->output_primitive == VKD3D_SHADER_TESSELLATOR_OUTPUT_LINE)
                hlsl_error(ctx, &entry_func->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_OUTPUT_PRIMITIVE,
                        "Line output topologies are not available for quad domains.");
            break;

        default:
            break;
    }
}

static void remove_unreachable_code(struct hlsl_ctx *ctx, struct hlsl_block *body)
{
    struct hlsl_ir_node *instr, *next;
    struct hlsl_block block;
    struct list *start;

    LIST_FOR_EACH_ENTRY_SAFE(instr, next, &body->instrs, struct hlsl_ir_node, entry)
    {
        if (instr->type == HLSL_IR_IF)
        {
            struct hlsl_ir_if *iff = hlsl_ir_if(instr);

            remove_unreachable_code(ctx, &iff->then_block);
            remove_unreachable_code(ctx, &iff->else_block);
        }
        else if (instr->type == HLSL_IR_LOOP)
        {
            struct hlsl_ir_loop *loop = hlsl_ir_loop(instr);

            remove_unreachable_code(ctx, &loop->body);
        }
        else if (instr->type == HLSL_IR_SWITCH)
        {
            struct hlsl_ir_switch *s = hlsl_ir_switch(instr);
            struct hlsl_ir_switch_case *c;

            LIST_FOR_EACH_ENTRY(c, &s->cases, struct hlsl_ir_switch_case, entry)
            {
                remove_unreachable_code(ctx, &c->body);
            }
        }
    }

    /* Remove instructions past unconditional jumps. */
    LIST_FOR_EACH_ENTRY(instr, &body->instrs, struct hlsl_ir_node, entry)
    {
        struct hlsl_ir_jump *jump;

        if (instr->type != HLSL_IR_JUMP)
            continue;

        jump = hlsl_ir_jump(instr);
        if (jump->type != HLSL_IR_JUMP_BREAK && jump->type != HLSL_IR_JUMP_CONTINUE)
            continue;

        if (!(start = list_next(&body->instrs, &instr->entry)))
            break;

        hlsl_block_init(&block);
        list_move_slice_tail(&block.instrs, start, list_tail(&body->instrs));
        hlsl_block_cleanup(&block);

        break;
    }
}

void hlsl_lower_index_loads(struct hlsl_ctx *ctx, struct hlsl_block *body)
{
    lower_ir(ctx, lower_index_loads, body);
}

void hlsl_run_const_passes(struct hlsl_ctx *ctx, struct hlsl_block *body)
{
    bool progress;

    lower_ir(ctx, lower_matrix_swizzles, body);

    lower_ir(ctx, lower_broadcasts, body);
    while (hlsl_transform_ir(ctx, fold_redundant_casts, body, NULL));
    do
    {
        progress = hlsl_transform_ir(ctx, split_array_copies, body, NULL);
        progress |= hlsl_transform_ir(ctx, split_struct_copies, body, NULL);
    }
    while (progress);
    hlsl_transform_ir(ctx, split_matrix_copies, body, NULL);

    lower_ir(ctx, lower_narrowing_casts, body);
    lower_ir(ctx, lower_int_dot, body);
    lower_ir(ctx, lower_int_division, body);
    lower_ir(ctx, lower_int_modulus, body);
    lower_ir(ctx, lower_int_abs, body);
    lower_ir(ctx, lower_casts_to_bool, body);
    lower_ir(ctx, lower_float_modulus, body);
    hlsl_transform_ir(ctx, fold_redundant_casts, body, NULL);

    do
    {
        progress = hlsl_transform_ir(ctx, hlsl_fold_constant_exprs, body, NULL);
        progress |= hlsl_transform_ir(ctx, hlsl_fold_constant_identities, body, NULL);
        progress |= hlsl_transform_ir(ctx, hlsl_fold_constant_swizzles, body, NULL);
        progress |= hlsl_copy_propagation_execute(ctx, body);
        progress |= hlsl_transform_ir(ctx, fold_swizzle_chains, body, NULL);
        progress |= hlsl_transform_ir(ctx, remove_trivial_swizzles, body, NULL);
        progress |= hlsl_transform_ir(ctx, remove_trivial_conditional_branches, body, NULL);
    } while (progress);
}

static void generate_vsir_signature_entry(struct hlsl_ctx *ctx, struct vsir_program *program,
        struct shader_signature *signature, bool output, bool is_patch_constant_func, struct hlsl_ir_var *var)
{
    enum vkd3d_shader_sysval_semantic sysval = VKD3D_SHADER_SV_NONE;
    enum vkd3d_shader_component_type component_type;
    unsigned int register_index, mask, use_mask;
    const char *name = var->semantic.name;
    enum vkd3d_shader_register_type type;
    struct signature_element *element;

    if (hlsl_version_ge(ctx, 4, 0))
    {
        struct vkd3d_string_buffer *string;
        bool has_idx, ret;

        ret = sm4_sysval_semantic_from_semantic_name(&sysval, &program->shader_version, ctx->semantic_compat_mapping,
                ctx->domain, var->semantic.name, var->semantic.index, output, is_patch_constant_func);
        VKD3D_ASSERT(ret);
        if (sysval == ~0u)
            return;

        if (sm4_register_from_semantic_name(&program->shader_version, var->semantic.name, output, &type, &has_idx))
        {
            register_index = has_idx ? var->semantic.index : ~0u;
            mask = (1u << var->data_type->dimx) - 1;
        }
        else
        {
            VKD3D_ASSERT(var->regs[HLSL_REGSET_NUMERIC].allocated);
            register_index = var->regs[HLSL_REGSET_NUMERIC].id;
            mask = var->regs[HLSL_REGSET_NUMERIC].writemask;
        }

        use_mask = mask; /* FIXME: retrieve use mask accurately. */

        switch (var->data_type->e.numeric.type)
        {
            case HLSL_TYPE_FLOAT:
            case HLSL_TYPE_HALF:
                component_type = VKD3D_SHADER_COMPONENT_FLOAT;
                break;

            case HLSL_TYPE_INT:
                component_type = VKD3D_SHADER_COMPONENT_INT;
                break;

            case HLSL_TYPE_BOOL:
            case HLSL_TYPE_UINT:
                component_type = VKD3D_SHADER_COMPONENT_UINT;
                break;

            default:
                if ((string = hlsl_type_to_string(ctx, var->data_type)))
                    hlsl_error(ctx, &var->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_TYPE,
                            "Invalid data type %s for semantic variable %s.", string->buffer, var->name);
                hlsl_release_string_buffer(ctx, string);
                component_type = VKD3D_SHADER_COMPONENT_VOID;
                break;
        }

        if (sysval == VKD3D_SHADER_SV_TARGET && !ascii_strcasecmp(name, "color"))
            name = "SV_Target";
        else if (sysval == VKD3D_SHADER_SV_DEPTH && !ascii_strcasecmp(name, "depth"))
            name ="SV_Depth";
        else if (sysval == VKD3D_SHADER_SV_POSITION && !ascii_strcasecmp(name, "position"))
            name = "SV_Position";
    }
    else
    {
        if ((!output && !var->last_read) || (output && !var->first_write))
            return;

        if (!sm1_register_from_semantic_name(&program->shader_version,
                var->semantic.name, var->semantic.index, output, &type, &register_index))
        {
            enum vkd3d_decl_usage usage;
            unsigned int usage_idx;
            bool ret;

            register_index = var->regs[HLSL_REGSET_NUMERIC].id;

            ret = sm1_usage_from_semantic_name(var->semantic.name, var->semantic.index, &usage, &usage_idx);
            VKD3D_ASSERT(ret);
            /* With the exception of vertex POSITION output, none of these are
             * system values. Pixel POSITION input is not equivalent to
             * SV_Position; the closer equivalent is VPOS, which is not declared
             * as a semantic. */
            if (program->shader_version.type == VKD3D_SHADER_TYPE_VERTEX
                    && output && usage == VKD3D_DECL_USAGE_POSITION)
                sysval = VKD3D_SHADER_SV_POSITION;
        }

        mask = (1 << var->data_type->dimx) - 1;

        if (!ascii_strcasecmp(var->semantic.name, "PSIZE") && output
                && program->shader_version.type == VKD3D_SHADER_TYPE_VERTEX)
        {
            if (var->data_type->dimx > 1)
                hlsl_error(ctx, &var->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_SEMANTIC,
                        "PSIZE output must have only 1 component in this shader model.");
            /* For some reason the writemask has all components set. */
            mask = VKD3DSP_WRITEMASK_ALL;
        }
        if (!ascii_strcasecmp(var->semantic.name, "FOG") && output && program->shader_version.major < 3
                && program->shader_version.type == VKD3D_SHADER_TYPE_VERTEX && var->data_type->dimx > 1)
            hlsl_error(ctx, &var->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_SEMANTIC,
                    "FOG output must have only 1 component in this shader model.");

        use_mask = mask; /* FIXME: retrieve use mask accurately. */
        component_type = VKD3D_SHADER_COMPONENT_FLOAT;
    }

    if (!vkd3d_array_reserve((void **)&signature->elements, &signature->elements_capacity,
            signature->element_count + 1, sizeof(*signature->elements)))
    {
        ctx->result = VKD3D_ERROR_OUT_OF_MEMORY;
        return;
    }
    element = &signature->elements[signature->element_count++];
    memset(element, 0, sizeof(*element));

    if (!(element->semantic_name = vkd3d_strdup(name)))
    {
        --signature->element_count;
        ctx->result = VKD3D_ERROR_OUT_OF_MEMORY;
        return;
    }
    element->semantic_index = var->semantic.index;
    element->sysval_semantic = sysval;
    element->component_type = component_type;
    element->register_index = register_index;
    element->target_location = register_index;
    element->register_count = 1;
    element->mask = mask;
    element->used_mask = use_mask;
    if (program->shader_version.type == VKD3D_SHADER_TYPE_PIXEL && !output)
        element->interpolation_mode = VKD3DSIM_LINEAR;
}

static void generate_vsir_signature(struct hlsl_ctx *ctx,
        struct vsir_program *program, struct hlsl_ir_function_decl *func)
{
    bool is_domain = program->shader_version.type == VKD3D_SHADER_TYPE_DOMAIN;
    bool is_patch_constant_func = func == ctx->patch_constant_func;
    struct hlsl_ir_var *var;

    LIST_FOR_EACH_ENTRY(var, &func->extern_vars, struct hlsl_ir_var, extern_entry)
    {
        if (var->is_input_semantic)
        {
            if (is_patch_constant_func)
                generate_vsir_signature_entry(ctx, program, &program->patch_constant_signature, false, true, var);
            else if (is_domain)
                generate_vsir_signature_entry(ctx, program, &program->patch_constant_signature, false, false, var);
            else
                generate_vsir_signature_entry(ctx, program, &program->input_signature, false, false, var);
        }
        if (var->is_output_semantic)
        {
            if (is_patch_constant_func)
                generate_vsir_signature_entry(ctx, program, &program->patch_constant_signature, true, true, var);
            else
                generate_vsir_signature_entry(ctx, program, &program->output_signature, true, false, var);
        }
    }
}

static enum vkd3d_data_type vsir_data_type_from_hlsl_type(struct hlsl_ctx *ctx, const struct hlsl_type *type)
{
    if (hlsl_version_lt(ctx, 4, 0))
        return VKD3D_DATA_FLOAT;

    if (type->class == HLSL_CLASS_ARRAY)
        return vsir_data_type_from_hlsl_type(ctx, type->e.array.type);
    if (type->class == HLSL_CLASS_STRUCT)
        return VKD3D_DATA_MIXED;
    if (type->class <= HLSL_CLASS_LAST_NUMERIC)
    {
        switch (type->e.numeric.type)
        {
            case HLSL_TYPE_DOUBLE:
                return VKD3D_DATA_DOUBLE;
            case HLSL_TYPE_FLOAT:
                return VKD3D_DATA_FLOAT;
            case HLSL_TYPE_HALF:
                return VKD3D_DATA_HALF;
            case HLSL_TYPE_INT:
                return VKD3D_DATA_INT;
            case HLSL_TYPE_UINT:
            case HLSL_TYPE_BOOL:
                return VKD3D_DATA_UINT;
        }
    }

    vkd3d_unreachable();
}

static enum vkd3d_data_type vsir_data_type_from_hlsl_instruction(struct hlsl_ctx *ctx,
        const struct hlsl_ir_node *instr)
{
    return vsir_data_type_from_hlsl_type(ctx, instr->data_type);
}

static uint32_t generate_vsir_get_src_swizzle(uint32_t src_writemask, uint32_t dst_writemask)
{
    uint32_t swizzle;

    swizzle = hlsl_swizzle_from_writemask(src_writemask);
    swizzle = hlsl_map_swizzle(swizzle, dst_writemask);
    swizzle = vsir_swizzle_from_hlsl(swizzle);
    return swizzle;
}

static void sm1_generate_vsir_constant_defs(struct hlsl_ctx *ctx, struct vsir_program *program,
        struct hlsl_block *block)
{
    struct vkd3d_shader_instruction_array *instructions = &program->instructions;
    struct vkd3d_shader_dst_param *dst_param;
    struct vkd3d_shader_src_param *src_param;
    struct vkd3d_shader_instruction *ins;
    unsigned int i, x;

    for (i = 0; i < ctx->constant_defs.count; ++i)
    {
        const struct hlsl_constant_register *constant_reg = &ctx->constant_defs.regs[i];

        if (!shader_instruction_array_reserve(instructions, instructions->count + 1))
        {
            ctx->result = VKD3D_ERROR_OUT_OF_MEMORY;
            return;
        }

        ins = &instructions->elements[instructions->count];
        if (!vsir_instruction_init_with_params(program, ins, &constant_reg->loc, VKD3DSIH_DEF, 1, 1))
        {
            ctx->result = VKD3D_ERROR_OUT_OF_MEMORY;
            return;
        }
        ++instructions->count;

        dst_param = &ins->dst[0];
        vsir_register_init(&dst_param->reg, VKD3DSPR_CONST, VKD3D_DATA_FLOAT, 1);
        ins->dst[0].reg.dimension = VSIR_DIMENSION_VEC4;
        ins->dst[0].reg.idx[0].offset = constant_reg->index;
        ins->dst[0].write_mask = VKD3DSP_WRITEMASK_ALL;

        src_param = &ins->src[0];
        vsir_register_init(&src_param->reg, VKD3DSPR_IMMCONST, VKD3D_DATA_FLOAT, 0);
        src_param->reg.type = VKD3DSPR_IMMCONST;
        src_param->reg.precision = VKD3D_SHADER_REGISTER_PRECISION_DEFAULT;
        src_param->reg.non_uniform = false;
        src_param->reg.data_type = VKD3D_DATA_FLOAT;
        src_param->reg.dimension = VSIR_DIMENSION_VEC4;
        for (x = 0; x < 4; ++x)
            src_param->reg.u.immconst_f32[x] = constant_reg->value.f[x];
        src_param->swizzle = VKD3D_SHADER_NO_SWIZZLE;
    }
}

static void sm1_generate_vsir_sampler_dcls(struct hlsl_ctx *ctx,
        struct vsir_program *program, struct hlsl_block *block)
{
    struct vkd3d_shader_instruction_array *instructions = &program->instructions;
    enum vkd3d_shader_resource_type resource_type;
    struct vkd3d_shader_register_range *range;
    struct vkd3d_shader_dst_param *dst_param;
    struct vkd3d_shader_semantic *semantic;
    struct vkd3d_shader_instruction *ins;
    enum hlsl_sampler_dim sampler_dim;
    struct hlsl_ir_var *var;
    unsigned int i, count;

    LIST_FOR_EACH_ENTRY(var, &ctx->extern_vars, struct hlsl_ir_var, extern_entry)
    {
        if (!var->regs[HLSL_REGSET_SAMPLERS].allocated)
            continue;

        count = var->bind_count[HLSL_REGSET_SAMPLERS];
        for (i = 0; i < count; ++i)
        {
            if (var->objects_usage[HLSL_REGSET_SAMPLERS][i].used)
            {
                sampler_dim = var->objects_usage[HLSL_REGSET_SAMPLERS][i].sampler_dim;

                switch (sampler_dim)
                {
                    case HLSL_SAMPLER_DIM_2D:
                        resource_type = VKD3D_SHADER_RESOURCE_TEXTURE_2D;
                        break;

                    case HLSL_SAMPLER_DIM_CUBE:
                        resource_type = VKD3D_SHADER_RESOURCE_TEXTURE_CUBE;
                        break;

                    case HLSL_SAMPLER_DIM_3D:
                        resource_type = VKD3D_SHADER_RESOURCE_TEXTURE_3D;
                        break;

                    case HLSL_SAMPLER_DIM_GENERIC:
                        /* These can appear in sm4-style combined sample instructions. */
                        hlsl_fixme(ctx, &var->loc, "Generic samplers need to be lowered.");
                        continue;

                    default:
                        vkd3d_unreachable();
                        break;
                }

                if (!shader_instruction_array_reserve(instructions, instructions->count + 1))
                {
                    ctx->result = VKD3D_ERROR_OUT_OF_MEMORY;
                    return;
                }

                ins = &instructions->elements[instructions->count];
                if (!vsir_instruction_init_with_params(program, ins, &var->loc, VKD3DSIH_DCL, 0, 0))
                {
                    ctx->result = VKD3D_ERROR_OUT_OF_MEMORY;
                    return;
                }
                ++instructions->count;

                semantic = &ins->declaration.semantic;
                semantic->resource_type = resource_type;

                dst_param = &semantic->resource.reg;
                vsir_register_init(&dst_param->reg, VKD3DSPR_SAMPLER, VKD3D_DATA_FLOAT, 1);
                dst_param->reg.dimension = VSIR_DIMENSION_NONE;
                dst_param->reg.idx[0].offset = var->regs[HLSL_REGSET_SAMPLERS].index + i;
                dst_param->write_mask = 0;
                range = &semantic->resource.range;
                range->space = 0;
                range->first = range->last = dst_param->reg.idx[0].offset;
            }
        }
    }
}

static struct vkd3d_shader_instruction *generate_vsir_add_program_instruction(
        struct hlsl_ctx *ctx, struct vsir_program *program,
        const struct vkd3d_shader_location *loc, enum vkd3d_shader_opcode opcode,
        unsigned int dst_count, unsigned int src_count)
{
    struct vkd3d_shader_instruction_array *instructions = &program->instructions;
    struct vkd3d_shader_instruction *ins;

    if (!shader_instruction_array_reserve(instructions, instructions->count + 1))
    {
        ctx->result = VKD3D_ERROR_OUT_OF_MEMORY;
        return NULL;
    }
    ins = &instructions->elements[instructions->count];
    if (!vsir_instruction_init_with_params(program, ins, loc, opcode, dst_count, src_count))
    {
        ctx->result = VKD3D_ERROR_OUT_OF_MEMORY;
        return NULL;
    }
    ++instructions->count;
    return ins;
}

static void vsir_src_from_hlsl_constant_value(struct vkd3d_shader_src_param *src,
        struct hlsl_ctx *ctx, const struct hlsl_constant_value *value,
        enum vkd3d_data_type type, unsigned int width, unsigned int map_writemask)
{
    unsigned int i, j;

    vsir_src_param_init(src, VKD3DSPR_IMMCONST, type, 0);
    if (width == 1)
    {
        src->reg.u.immconst_u32[0] = value->u[0].u;
        return;
    }

    src->reg.dimension = VSIR_DIMENSION_VEC4;
    for (i = 0, j = 0; i < 4; ++i)
    {
        if ((map_writemask & (1u << i)) && (j < width))
            src->reg.u.immconst_u32[i] = value->u[j++].u;
        else
            src->reg.u.immconst_u32[i] = 0;
    }
}

static void vsir_src_from_hlsl_node(struct vkd3d_shader_src_param *src,
        struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, uint32_t map_writemask)
{
    struct hlsl_ir_constant *constant;

    if (hlsl_version_ge(ctx, 4, 0) && instr->type == HLSL_IR_CONSTANT)
    {
        /* In SM4 constants are inlined */
        constant = hlsl_ir_constant(instr);
        vsir_src_from_hlsl_constant_value(src, ctx, &constant->value,
                vsir_data_type_from_hlsl_instruction(ctx, instr), instr->data_type->dimx, map_writemask);
    }
    else
    {
        vsir_register_init(&src->reg, VKD3DSPR_TEMP, vsir_data_type_from_hlsl_instruction(ctx, instr), 1);
        src->reg.idx[0].offset = instr->reg.id;
        src->reg.dimension = VSIR_DIMENSION_VEC4;
        src->swizzle = generate_vsir_get_src_swizzle(instr->reg.writemask, map_writemask);
    }
}

static void vsir_dst_from_hlsl_node(struct vkd3d_shader_dst_param *dst,
        struct hlsl_ctx *ctx, const struct hlsl_ir_node *instr)
{
    VKD3D_ASSERT(instr->reg.allocated);
    vsir_dst_param_init(dst, VKD3DSPR_TEMP, vsir_data_type_from_hlsl_instruction(ctx, instr), 1);
    dst->reg.idx[0].offset = instr->reg.id;
    dst->reg.dimension = VSIR_DIMENSION_VEC4;
    dst->write_mask = instr->reg.writemask;
}

static void sm1_generate_vsir_instr_constant(struct hlsl_ctx *ctx,
        struct vsir_program *program, struct hlsl_ir_constant *constant)
{
    struct hlsl_ir_node *instr = &constant->node;
    struct vkd3d_shader_dst_param *dst_param;
    struct vkd3d_shader_src_param *src_param;
    struct vkd3d_shader_instruction *ins;

    VKD3D_ASSERT(instr->reg.allocated);
    VKD3D_ASSERT(constant->reg.allocated);

    if (!(ins = generate_vsir_add_program_instruction(ctx, program, &instr->loc, VKD3DSIH_MOV, 1, 1)))
        return;

    src_param = &ins->src[0];
    vsir_register_init(&src_param->reg, VKD3DSPR_CONST, VKD3D_DATA_FLOAT, 1);
    src_param->reg.idx[0].offset = constant->reg.id;
    src_param->swizzle = generate_vsir_get_src_swizzle(constant->reg.writemask, instr->reg.writemask);

    dst_param = &ins->dst[0];
    vsir_register_init(&dst_param->reg, VKD3DSPR_TEMP, VKD3D_DATA_FLOAT, 1);
    dst_param->reg.idx[0].offset = instr->reg.id;
    dst_param->write_mask = instr->reg.writemask;
}

static void sm4_generate_vsir_rasterizer_sample_count(struct hlsl_ctx *ctx,
        struct vsir_program *program, struct hlsl_ir_expr *expr)
{
    struct vkd3d_shader_src_param *src_param;
    struct hlsl_ir_node *instr = &expr->node;
    struct vkd3d_shader_instruction *ins;

    if (!(ins = generate_vsir_add_program_instruction(ctx, program, &instr->loc, VKD3DSIH_SAMPLE_INFO, 1, 1)))
        return;
    ins->flags = VKD3DSI_SAMPLE_INFO_UINT;

    vsir_dst_from_hlsl_node(&ins->dst[0], ctx, instr);

    src_param = &ins->src[0];
    vsir_src_param_init(src_param, VKD3DSPR_RASTERIZER, VKD3D_DATA_UNUSED, 0);
    src_param->reg.dimension = VSIR_DIMENSION_VEC4;
    src_param->swizzle = VKD3D_SHADER_SWIZZLE(X, X, X, X);
}

/* Translate ops that can be mapped to a single vsir instruction with only one dst register. */
static void generate_vsir_instr_expr_single_instr_op(struct hlsl_ctx *ctx,
        struct vsir_program *program, struct hlsl_ir_expr *expr, enum vkd3d_shader_opcode opcode,
        uint32_t src_mod, uint32_t dst_mod, bool map_src_swizzles)
{
    struct hlsl_ir_node *instr = &expr->node;
    struct vkd3d_shader_dst_param *dst_param;
    struct vkd3d_shader_src_param *src_param;
    struct vkd3d_shader_instruction *ins;
    unsigned int i, src_count = 0;

    VKD3D_ASSERT(instr->reg.allocated);

    for (i = 0; i < HLSL_MAX_OPERANDS; ++i)
    {
        if (expr->operands[i].node)
            src_count = i + 1;
    }
    VKD3D_ASSERT(!src_mod || src_count == 1);

    if (!(ins = generate_vsir_add_program_instruction(ctx, program, &instr->loc, opcode, 1, src_count)))
        return;

    dst_param = &ins->dst[0];
    vsir_dst_from_hlsl_node(dst_param, ctx, instr);
    dst_param->modifiers = dst_mod;

    for (i = 0; i < src_count; ++i)
    {
        struct hlsl_ir_node *operand = expr->operands[i].node;

        src_param = &ins->src[i];
        vsir_src_from_hlsl_node(src_param, ctx, operand,
                map_src_swizzles ? dst_param->write_mask : VKD3DSP_WRITEMASK_ALL);
        src_param->modifiers = src_mod;
    }
}

/* Translate ops that have 1 src and need one instruction for each component in
 * the d3dbc backend. */
static void sm1_generate_vsir_instr_expr_per_component_instr_op(struct hlsl_ctx *ctx,
        struct vsir_program *program, struct hlsl_ir_expr *expr, enum vkd3d_shader_opcode opcode)
{
    struct hlsl_ir_node *operand = expr->operands[0].node;
    struct hlsl_ir_node *instr = &expr->node;
    struct vkd3d_shader_dst_param *dst_param;
    struct vkd3d_shader_src_param *src_param;
    struct vkd3d_shader_instruction *ins;
    uint32_t src_swizzle;
    unsigned int i, c;

    VKD3D_ASSERT(instr->reg.allocated);
    VKD3D_ASSERT(operand);

    src_swizzle = generate_vsir_get_src_swizzle(operand->reg.writemask, instr->reg.writemask);
    for (i = 0; i < 4; ++i)
    {
        if (instr->reg.writemask & (1u << i))
        {
            if (!(ins = generate_vsir_add_program_instruction(ctx, program, &instr->loc, opcode, 1, 1)))
                return;

            dst_param = &ins->dst[0];
            vsir_register_init(&dst_param->reg, VKD3DSPR_TEMP, VKD3D_DATA_FLOAT, 1);
            dst_param->reg.idx[0].offset = instr->reg.id;
            dst_param->write_mask = 1u << i;

            src_param = &ins->src[0];
            vsir_register_init(&src_param->reg, VKD3DSPR_TEMP, VKD3D_DATA_FLOAT, 1);
            src_param->reg.idx[0].offset = operand->reg.id;
            c = vsir_swizzle_get_component(src_swizzle, i);
            src_param->swizzle = vsir_swizzle_from_writemask(1u << c);
        }
    }
}

static void sm1_generate_vsir_instr_expr_sincos(struct hlsl_ctx *ctx, struct vsir_program *program,
        struct hlsl_ir_expr *expr)
{
    struct hlsl_ir_node *operand = expr->operands[0].node;
    struct hlsl_ir_node *instr = &expr->node;
    struct vkd3d_shader_dst_param *dst_param;
    struct vkd3d_shader_src_param *src_param;
    struct vkd3d_shader_instruction *ins;
    unsigned int src_count = 0;

    VKD3D_ASSERT(instr->reg.allocated);
    src_count = (ctx->profile->major_version < 3) ? 3 : 1;

    if (!(ins = generate_vsir_add_program_instruction(ctx, program, &instr->loc, VKD3DSIH_SINCOS, 1, src_count)))
        return;

    dst_param = &ins->dst[0];
    vsir_register_init(&dst_param->reg, VKD3DSPR_TEMP, VKD3D_DATA_FLOAT, 1);
    dst_param->reg.idx[0].offset = instr->reg.id;
    dst_param->write_mask = instr->reg.writemask;

    src_param = &ins->src[0];
    vsir_register_init(&src_param->reg, VKD3DSPR_TEMP, VKD3D_DATA_FLOAT, 1);
    src_param->reg.idx[0].offset = operand->reg.id;
    src_param->swizzle = generate_vsir_get_src_swizzle(operand->reg.writemask, VKD3DSP_WRITEMASK_ALL);

    if (ctx->profile->major_version < 3)
    {
        src_param = &ins->src[1];
        vsir_register_init(&src_param->reg, VKD3DSPR_CONST, VKD3D_DATA_FLOAT, 1);
        src_param->reg.idx[0].offset = ctx->d3dsincosconst1.id;
        src_param->swizzle = VKD3D_SHADER_NO_SWIZZLE;

        src_param = &ins->src[1];
        vsir_register_init(&src_param->reg, VKD3DSPR_CONST, VKD3D_DATA_FLOAT, 1);
        src_param->reg.idx[0].offset = ctx->d3dsincosconst2.id;
        src_param->swizzle = VKD3D_SHADER_NO_SWIZZLE;
    }
}

static bool sm1_generate_vsir_instr_expr_cast(struct hlsl_ctx *ctx,
        struct vsir_program *program, struct hlsl_ir_expr *expr)
{
    const struct hlsl_type *src_type, *dst_type;
    const struct hlsl_ir_node *arg1, *instr;

    arg1 = expr->operands[0].node;
    src_type = arg1->data_type;
    instr = &expr->node;
    dst_type = instr->data_type;

    /* Narrowing casts were already lowered. */
    VKD3D_ASSERT(src_type->dimx == dst_type->dimx);

    switch (dst_type->e.numeric.type)
    {
        case HLSL_TYPE_HALF:
        case HLSL_TYPE_FLOAT:
            switch (src_type->e.numeric.type)
            {
                case HLSL_TYPE_INT:
                case HLSL_TYPE_UINT:
                case HLSL_TYPE_BOOL:
                    /* Integrals are internally represented as floats, so no change is necessary.*/
                case HLSL_TYPE_HALF:
                case HLSL_TYPE_FLOAT:
                    generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_MOV, 0, 0, true);
                    return true;

                case HLSL_TYPE_DOUBLE:
                    if (ctx->double_as_float_alias)
                    {
                        generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_MOV, 0, 0, true);
                        return true;
                    }
                    hlsl_error(ctx, &instr->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_TYPE,
                            "The 'double' type is not supported for the %s profile.", ctx->profile->name);
                    break;

                default:
                    vkd3d_unreachable();
            }
            break;

        case HLSL_TYPE_INT:
        case HLSL_TYPE_UINT:
            switch(src_type->e.numeric.type)
            {
                case HLSL_TYPE_HALF:
                case HLSL_TYPE_FLOAT:
                    /* A compilation pass turns these into FLOOR+REINTERPRET, so we should not
                     * reach this case unless we are missing something. */
                    hlsl_fixme(ctx, &instr->loc, "Unlowered SM1 cast from float to integer.");
                    break;

                case HLSL_TYPE_INT:
                case HLSL_TYPE_UINT:
                    generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_MOV, 0, 0, true);
                    return true;

                case HLSL_TYPE_BOOL:
                    hlsl_fixme(ctx, &instr->loc, "SM1 cast from bool to integer.");
                    break;

                case HLSL_TYPE_DOUBLE:
                    hlsl_fixme(ctx, &instr->loc, "SM1 cast from double to integer.");
                    break;

                default:
                    vkd3d_unreachable();
            }
            break;

        case HLSL_TYPE_DOUBLE:
            switch (src_type->e.numeric.type)
            {
                case HLSL_TYPE_FLOAT:
                    if (ctx->double_as_float_alias)
                    {
                        generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_MOV, 0, 0, true);
                        return true;
                    }
                    hlsl_error(ctx, &instr->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_TYPE,
                            "The 'double' type is not supported for the %s profile.", ctx->profile->name);
                    break;

                default:
                    hlsl_fixme(ctx, &instr->loc, "SM1 cast to double.");
                    break;
            }
            break;

        case HLSL_TYPE_BOOL:
            /* Casts to bool should have already been lowered. */
        default:
            hlsl_fixme(ctx, &expr->node.loc, "SM1 cast from %s to %s.",
                debug_hlsl_type(ctx, src_type), debug_hlsl_type(ctx, dst_type));
            break;
    }

    return false;
}

static bool sm1_generate_vsir_instr_expr(struct hlsl_ctx *ctx, struct vsir_program *program,
        struct hlsl_ir_expr *expr)
{
    struct hlsl_ir_node *instr = &expr->node;

    if (expr->op != HLSL_OP1_REINTERPRET && expr->op != HLSL_OP1_CAST
            && instr->data_type->e.numeric.type != HLSL_TYPE_FLOAT)
    {
        /* These need to be lowered. */
        hlsl_fixme(ctx, &instr->loc, "SM1 non-float expression.");
        return false;
    }

    switch (expr->op)
    {
        case HLSL_OP1_ABS:
            generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_ABS, 0, 0, true);
            break;

        case HLSL_OP1_CAST:
            return sm1_generate_vsir_instr_expr_cast(ctx, program, expr);

        case HLSL_OP1_COS_REDUCED:
            VKD3D_ASSERT(expr->node.reg.writemask == VKD3DSP_WRITEMASK_0);
            sm1_generate_vsir_instr_expr_sincos(ctx, program, expr);
            break;

        case HLSL_OP1_DSX:
            generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_DSX, 0, 0, true);
            break;

        case HLSL_OP1_DSY:
            generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_DSY, 0, 0, true);
            break;

        case HLSL_OP1_EXP2:
            sm1_generate_vsir_instr_expr_per_component_instr_op(ctx, program, expr, VKD3DSIH_EXP);
            break;

        case HLSL_OP1_LOG2:
            sm1_generate_vsir_instr_expr_per_component_instr_op(ctx, program, expr, VKD3DSIH_LOG);
            break;

        case HLSL_OP1_NEG:
            generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_MOV, VKD3DSPSM_NEG, 0, true);
            break;

        case HLSL_OP1_RCP:
            sm1_generate_vsir_instr_expr_per_component_instr_op(ctx, program, expr, VKD3DSIH_RCP);
            break;

        case HLSL_OP1_REINTERPRET:
            generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_MOV, 0, 0, true);
            break;

        case HLSL_OP1_RSQ:
            sm1_generate_vsir_instr_expr_per_component_instr_op(ctx, program, expr, VKD3DSIH_RSQ);
            break;

        case HLSL_OP1_SAT:
            generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_MOV, 0, VKD3DSPDM_SATURATE, true);
            break;

        case HLSL_OP1_SIN_REDUCED:
            VKD3D_ASSERT(expr->node.reg.writemask == VKD3DSP_WRITEMASK_1);
            sm1_generate_vsir_instr_expr_sincos(ctx, program, expr);
            break;

        case HLSL_OP2_ADD:
            generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_ADD, 0, 0, true);
            break;

        case HLSL_OP2_DOT:
            switch (expr->operands[0].node->data_type->dimx)
            {
                case 3:
                    generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_DP3, 0, 0, false);
                    break;

                case 4:
                    generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_DP4, 0, 0, false);
                    break;

                default:
                    vkd3d_unreachable();
                    return false;
            }
            break;

        case HLSL_OP2_MAX:
            generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_MAX, 0, 0, true);
            break;

        case HLSL_OP2_MIN:
            generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_MIN, 0, 0, true);
            break;

        case HLSL_OP2_MUL:
            generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_MUL, 0, 0, true);
            break;

        case HLSL_OP1_FRACT:
            generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_FRC, 0, 0, true);
            break;

        case HLSL_OP2_LOGIC_AND:
            generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_MIN, 0, 0, true);
            break;

        case HLSL_OP2_LOGIC_OR:
            generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_MAX, 0, 0, true);
            break;

        case HLSL_OP2_SLT:
            generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_SLT, 0, 0, true);
            break;

        case HLSL_OP3_CMP:
            generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_CMP, 0, 0, true);
            break;

        case HLSL_OP3_DP2ADD:
            generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_DP2ADD, 0, 0, false);
            break;

        case HLSL_OP3_MAD:
            generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_MAD, 0, 0, true);
            break;

        default:
            hlsl_fixme(ctx, &instr->loc, "SM1 \"%s\" expression.", debug_hlsl_expr_op(expr->op));
            return false;
    }

    return true;
}

static void sm1_generate_vsir_init_dst_param_from_deref(struct hlsl_ctx *ctx,
        struct vkd3d_shader_dst_param *dst_param, struct hlsl_deref *deref,
        const struct vkd3d_shader_location *loc, unsigned int writemask)
{
    enum vkd3d_shader_register_type type = VKD3DSPR_TEMP;
    struct vkd3d_shader_version version;
    uint32_t register_index;
    struct hlsl_reg reg;

    reg = hlsl_reg_from_deref(ctx, deref);
    register_index = reg.id;
    writemask = hlsl_combine_writemasks(reg.writemask, writemask);

    if (deref->var->is_output_semantic)
    {
        const char *semantic_name = deref->var->semantic.name;

        version.major = ctx->profile->major_version;
        version.minor = ctx->profile->minor_version;
        version.type = ctx->profile->type;

        if (version.type == VKD3D_SHADER_TYPE_PIXEL && version.major == 1)
        {
            type = VKD3DSPR_TEMP;
            register_index = 0;
        }
        else if (!sm1_register_from_semantic_name(&version, semantic_name,
                deref->var->semantic.index, true, &type, &register_index))
        {
            VKD3D_ASSERT(reg.allocated);
            type = VKD3DSPR_OUTPUT;
            register_index = reg.id;
        }
        else
            writemask = (1u << deref->var->data_type->dimx) - 1;

        if (version.type == VKD3D_SHADER_TYPE_PIXEL && (!ascii_strcasecmp(semantic_name, "PSIZE")
                || (!ascii_strcasecmp(semantic_name, "FOG") && version.major < 3)))
        {
            /* These are always 1-component, but for some reason are written
             * with a writemask containing all components. */
            writemask = VKD3DSP_WRITEMASK_ALL;
        }
    }
    else
        VKD3D_ASSERT(reg.allocated);

    vsir_register_init(&dst_param->reg, type, VKD3D_DATA_FLOAT, 1);
    dst_param->write_mask = writemask;
    dst_param->reg.idx[0].offset = register_index;

    if (deref->rel_offset.node)
        hlsl_fixme(ctx, loc, "Translate relative addressing on dst register for vsir.");
}

static void sm1_generate_vsir_init_src_param_from_deref(struct hlsl_ctx *ctx,
        struct vkd3d_shader_src_param *src_param, struct hlsl_deref *deref,
        unsigned int dst_writemask, const struct vkd3d_shader_location *loc)
{
    enum vkd3d_shader_register_type type = VKD3DSPR_TEMP;
    struct vkd3d_shader_version version;
    uint32_t register_index;
    unsigned int writemask;
    struct hlsl_reg reg;

    if (hlsl_type_is_resource(deref->var->data_type))
    {
        unsigned int sampler_offset;

        type = VKD3DSPR_COMBINED_SAMPLER;

        sampler_offset = hlsl_offset_from_deref_safe(ctx, deref);
        register_index = deref->var->regs[HLSL_REGSET_SAMPLERS].index + sampler_offset;
        writemask = VKD3DSP_WRITEMASK_ALL;
    }
    else if (deref->var->is_uniform)
    {
        type = VKD3DSPR_CONST;

        reg = hlsl_reg_from_deref(ctx, deref);
        register_index = reg.id;
        writemask = reg.writemask;
        VKD3D_ASSERT(reg.allocated);
    }
    else if (deref->var->is_input_semantic)
    {
        version.major = ctx->profile->major_version;
        version.minor = ctx->profile->minor_version;
        version.type = ctx->profile->type;
        if (sm1_register_from_semantic_name(&version, deref->var->semantic.name,
                deref->var->semantic.index, false, &type, &register_index))
        {
            writemask = (1 << deref->var->data_type->dimx) - 1;
        }
        else
        {
            type = VKD3DSPR_INPUT;

            reg = hlsl_reg_from_deref(ctx, deref);
            register_index = reg.id;
            writemask = reg.writemask;
            VKD3D_ASSERT(reg.allocated);
        }
    }
    else
    {
        type = VKD3DSPR_TEMP;

        reg = hlsl_reg_from_deref(ctx, deref);
        register_index = reg.id;
        writemask = reg.writemask;
    }

    vsir_register_init(&src_param->reg, type, VKD3D_DATA_FLOAT, 1);
    src_param->reg.idx[0].offset = register_index;
    src_param->swizzle = generate_vsir_get_src_swizzle(writemask, dst_writemask);

    if (deref->rel_offset.node)
        hlsl_fixme(ctx, loc, "Translate relative addressing on src register for vsir.");
}

static void sm1_generate_vsir_instr_load(struct hlsl_ctx *ctx, struct vsir_program *program,
        struct hlsl_ir_load *load)
{
    struct hlsl_ir_node *instr = &load->node;
    struct vkd3d_shader_dst_param *dst_param;
    struct vkd3d_shader_instruction *ins;

    VKD3D_ASSERT(instr->reg.allocated);

    if (!(ins = generate_vsir_add_program_instruction(ctx, program, &instr->loc, VKD3DSIH_MOV, 1, 1)))
        return;

    dst_param = &ins->dst[0];
    vsir_register_init(&dst_param->reg, VKD3DSPR_TEMP, VKD3D_DATA_FLOAT, 1);
    dst_param->reg.idx[0].offset = instr->reg.id;
    dst_param->write_mask = instr->reg.writemask;

    sm1_generate_vsir_init_src_param_from_deref(ctx, &ins->src[0], &load->src, dst_param->write_mask,
            &ins->location);
}

static void sm1_generate_vsir_instr_resource_load(struct hlsl_ctx *ctx,
        struct vsir_program *program, struct hlsl_ir_resource_load *load)
{
    struct hlsl_ir_node *coords = load->coords.node;
    struct hlsl_ir_node *ddx = load->ddx.node;
    struct hlsl_ir_node *ddy = load->ddy.node;
    struct hlsl_ir_node *instr = &load->node;
    struct vkd3d_shader_dst_param *dst_param;
    struct vkd3d_shader_src_param *src_param;
    struct vkd3d_shader_instruction *ins;
    enum vkd3d_shader_opcode opcode;
    unsigned int src_count = 2;
    uint32_t flags = 0;

    VKD3D_ASSERT(instr->reg.allocated);

    switch (load->load_type)
    {
        case HLSL_RESOURCE_SAMPLE:
            opcode = VKD3DSIH_TEX;
            break;

        case HLSL_RESOURCE_SAMPLE_PROJ:
            opcode = VKD3DSIH_TEX;
            flags |= VKD3DSI_TEXLD_PROJECT;
            break;

        case HLSL_RESOURCE_SAMPLE_LOD_BIAS:
            opcode = VKD3DSIH_TEX;
            flags |= VKD3DSI_TEXLD_BIAS;
            break;

        case HLSL_RESOURCE_SAMPLE_GRAD:
            opcode = VKD3DSIH_TEXLDD;
            src_count += 2;
            break;

        default:
            hlsl_fixme(ctx, &instr->loc, "Resource load type %u.", load->load_type);
            return;
    }

    if (!(ins = generate_vsir_add_program_instruction(ctx, program, &instr->loc, opcode, 1, src_count)))
        return;
    ins->flags = flags;

    dst_param = &ins->dst[0];
    vsir_register_init(&dst_param->reg, VKD3DSPR_TEMP, VKD3D_DATA_FLOAT, 1);
    dst_param->reg.idx[0].offset = instr->reg.id;
    dst_param->write_mask = instr->reg.writemask;

    src_param = &ins->src[0];
    vsir_src_from_hlsl_node(src_param, ctx, coords, VKD3DSP_WRITEMASK_ALL);

    sm1_generate_vsir_init_src_param_from_deref(ctx, &ins->src[1], &load->resource,
            VKD3DSP_WRITEMASK_ALL, &ins->location);

    if (load->load_type == HLSL_RESOURCE_SAMPLE_GRAD)
    {
        src_param = &ins->src[2];
        vsir_src_from_hlsl_node(src_param, ctx, ddx, VKD3DSP_WRITEMASK_ALL);

        src_param = &ins->src[3];
        vsir_src_from_hlsl_node(src_param, ctx, ddy, VKD3DSP_WRITEMASK_ALL);
    }
}

static void generate_vsir_instr_swizzle(struct hlsl_ctx *ctx,
        struct vsir_program *program, struct hlsl_ir_swizzle *swizzle_instr)
{
    struct hlsl_ir_node *instr = &swizzle_instr->node, *val = swizzle_instr->val.node;
    struct vkd3d_shader_dst_param *dst_param;
    struct vkd3d_shader_src_param *src_param;
    struct vkd3d_shader_instruction *ins;
    uint32_t swizzle;

    VKD3D_ASSERT(instr->reg.allocated);

    if (!(ins = generate_vsir_add_program_instruction(ctx, program, &instr->loc, VKD3DSIH_MOV, 1, 1)))
        return;

    dst_param = &ins->dst[0];
    vsir_register_init(&dst_param->reg, VKD3DSPR_TEMP, vsir_data_type_from_hlsl_instruction(ctx, instr), 1);
    dst_param->reg.idx[0].offset = instr->reg.id;
    dst_param->reg.dimension = VSIR_DIMENSION_VEC4;
    dst_param->write_mask = instr->reg.writemask;

    swizzle = hlsl_swizzle_from_writemask(val->reg.writemask);
    swizzle = hlsl_combine_swizzles(swizzle, swizzle_instr->swizzle, instr->data_type->dimx);
    swizzle = hlsl_map_swizzle(swizzle, ins->dst[0].write_mask);
    swizzle = vsir_swizzle_from_hlsl(swizzle);

    src_param = &ins->src[0];
    VKD3D_ASSERT(val->type != HLSL_IR_CONSTANT);
    vsir_register_init(&src_param->reg, VKD3DSPR_TEMP, vsir_data_type_from_hlsl_instruction(ctx, val), 1);
    src_param->reg.idx[0].offset = val->reg.id;
    src_param->reg.dimension = VSIR_DIMENSION_VEC4;
    src_param->swizzle = swizzle;
}

static void sm1_generate_vsir_instr_store(struct hlsl_ctx *ctx, struct vsir_program *program,
        struct hlsl_ir_store *store)
{
    struct hlsl_ir_node *rhs = store->rhs.node;
    struct hlsl_ir_node *instr = &store->node;
    struct vkd3d_shader_instruction *ins;
    struct vkd3d_shader_src_param *src_param;

    if (!(ins = generate_vsir_add_program_instruction(ctx, program, &instr->loc, VKD3DSIH_MOV, 1, 1)))
        return;

    sm1_generate_vsir_init_dst_param_from_deref(ctx, &ins->dst[0], &store->lhs, &ins->location, store->writemask);

    src_param = &ins->src[0];
    vsir_src_from_hlsl_node(src_param, ctx, rhs, ins->dst[0].write_mask);
}

static void sm1_generate_vsir_instr_jump(struct hlsl_ctx *ctx,
        struct vsir_program *program, struct hlsl_ir_jump *jump)
{
    struct hlsl_ir_node *condition = jump->condition.node;
    struct hlsl_ir_node *instr = &jump->node;
    struct vkd3d_shader_dst_param *dst_param;
    struct vkd3d_shader_instruction *ins;

    if (jump->type == HLSL_IR_JUMP_DISCARD_NEG)
    {
        if (!(ins = generate_vsir_add_program_instruction(ctx, program, &instr->loc, VKD3DSIH_TEXKILL, 1, 0)))
            return;

        dst_param = &ins->dst[0];
        vsir_register_init(&dst_param->reg, VKD3DSPR_TEMP, VKD3D_DATA_FLOAT, 1);
        dst_param->reg.idx[0].offset = condition->reg.id;
        dst_param->write_mask = condition->reg.writemask;
    }
    else
    {
        hlsl_fixme(ctx, &instr->loc, "Jump type %s.", hlsl_jump_type_to_string(jump->type));
    }
}

static void sm1_generate_vsir_block(struct hlsl_ctx *ctx, struct hlsl_block *block, struct vsir_program *program);

static void sm1_generate_vsir_instr_if(struct hlsl_ctx *ctx, struct vsir_program *program, struct hlsl_ir_if *iff)
{
    struct hlsl_ir_node *condition = iff->condition.node;
    struct vkd3d_shader_src_param *src_param;
    struct hlsl_ir_node *instr = &iff->node;
    struct vkd3d_shader_instruction *ins;

    if (hlsl_version_lt(ctx, 2, 1))
    {
        hlsl_fixme(ctx, &instr->loc, "Flatten \"if\" conditionals branches.");
        return;
    }
    VKD3D_ASSERT(condition->data_type->dimx == 1 && condition->data_type->dimy == 1);

    if (!(ins = generate_vsir_add_program_instruction(ctx, program, &instr->loc, VKD3DSIH_IFC, 0, 2)))
        return;
    ins->flags = VKD3D_SHADER_REL_OP_NE;

    src_param = &ins->src[0];
    vsir_src_from_hlsl_node(src_param, ctx, condition, VKD3DSP_WRITEMASK_ALL);
    src_param->modifiers = 0;

    src_param = &ins->src[1];
    vsir_src_from_hlsl_node(src_param, ctx, condition, VKD3DSP_WRITEMASK_ALL);
    src_param->modifiers = VKD3DSPSM_NEG;

    sm1_generate_vsir_block(ctx, &iff->then_block, program);

    if (!(ins = generate_vsir_add_program_instruction(ctx, program, &instr->loc, VKD3DSIH_ELSE, 0, 0)))
        return;

    sm1_generate_vsir_block(ctx, &iff->else_block, program);

    if (!(ins = generate_vsir_add_program_instruction(ctx, program, &instr->loc, VKD3DSIH_ENDIF, 0, 0)))
        return;
}

static void sm1_generate_vsir_block(struct hlsl_ctx *ctx, struct hlsl_block *block, struct vsir_program *program)
{
    struct hlsl_ir_node *instr, *next;

    LIST_FOR_EACH_ENTRY_SAFE(instr, next, &block->instrs, struct hlsl_ir_node, entry)
    {
        if (instr->data_type)
        {
            if (instr->data_type->class != HLSL_CLASS_SCALAR && instr->data_type->class != HLSL_CLASS_VECTOR)
            {
                hlsl_fixme(ctx, &instr->loc, "Class %#x should have been lowered or removed.", instr->data_type->class);
                break;
            }
        }

        switch (instr->type)
        {
            case HLSL_IR_CALL:
                vkd3d_unreachable();

            case HLSL_IR_CONSTANT:
                sm1_generate_vsir_instr_constant(ctx, program, hlsl_ir_constant(instr));
                break;

            case HLSL_IR_EXPR:
                sm1_generate_vsir_instr_expr(ctx, program, hlsl_ir_expr(instr));
                break;

            case HLSL_IR_IF:
                sm1_generate_vsir_instr_if(ctx, program, hlsl_ir_if(instr));
                break;

            case HLSL_IR_JUMP:
                sm1_generate_vsir_instr_jump(ctx, program, hlsl_ir_jump(instr));
                break;

            case HLSL_IR_LOAD:
                sm1_generate_vsir_instr_load(ctx, program, hlsl_ir_load(instr));
                break;

            case HLSL_IR_RESOURCE_LOAD:
                sm1_generate_vsir_instr_resource_load(ctx, program, hlsl_ir_resource_load(instr));
                break;

            case HLSL_IR_STORE:
                sm1_generate_vsir_instr_store(ctx, program, hlsl_ir_store(instr));
                break;

            case HLSL_IR_SWIZZLE:
                generate_vsir_instr_swizzle(ctx, program, hlsl_ir_swizzle(instr));
                break;

            default:
                hlsl_fixme(ctx, &instr->loc, "Instruction type %s.", hlsl_node_type_to_string(instr->type));
                break;
        }
    }
}

static void sm1_generate_vsir(struct hlsl_ctx *ctx, struct hlsl_ir_function_decl *entry_func,
        uint64_t config_flags, struct vsir_program *program, struct vkd3d_shader_code *ctab)
{
    struct vkd3d_shader_version version = {0};
    struct vkd3d_bytecode_buffer buffer = {0};
    struct hlsl_block block;

    version.major = ctx->profile->major_version;
    version.minor = ctx->profile->minor_version;
    version.type = ctx->profile->type;
    if (!vsir_program_init(program, NULL, &version, 0, VSIR_CF_STRUCTURED, VSIR_NOT_NORMALISED))
    {
        ctx->result = VKD3D_ERROR_OUT_OF_MEMORY;
        return;
    }

    write_sm1_uniforms(ctx, &buffer);
    if (buffer.status)
    {
        vkd3d_free(buffer.data);
        ctx->result = buffer.status;
        return;
    }
    ctab->code = buffer.data;
    ctab->size = buffer.size;

    generate_vsir_signature(ctx, program, entry_func);

    hlsl_block_init(&block);
    sm1_generate_vsir_constant_defs(ctx, program, &block);
    sm1_generate_vsir_sampler_dcls(ctx, program, &block);
    list_move_head(&entry_func->body.instrs, &block.instrs);

    sm1_generate_vsir_block(ctx, &entry_func->body, program);
}

static void add_last_vsir_instr_to_block(struct hlsl_ctx *ctx, struct vsir_program *program, struct hlsl_block *block)
{
    struct vkd3d_shader_location *loc;
    struct hlsl_ir_node *vsir_instr;

    loc = &program->instructions.elements[program->instructions.count - 1].location;

    if (!(vsir_instr = hlsl_new_vsir_instruction_ref(ctx, program->instructions.count - 1, NULL, NULL, loc)))
    {
        ctx->result = VKD3D_ERROR_OUT_OF_MEMORY;
        return;
    }
    hlsl_block_add_instr(block, vsir_instr);
}

static void replace_instr_with_last_vsir_instr(struct hlsl_ctx *ctx,
        struct vsir_program *program, struct hlsl_ir_node *instr)
{
    struct vkd3d_shader_location *loc;
    struct hlsl_ir_node *vsir_instr;

    loc = &program->instructions.elements[program->instructions.count - 1].location;

    if (!(vsir_instr = hlsl_new_vsir_instruction_ref(ctx,
            program->instructions.count - 1, instr->data_type, &instr->reg, loc)))
    {
        ctx->result = VKD3D_ERROR_OUT_OF_MEMORY;
        return;
    }

    list_add_before(&instr->entry, &vsir_instr->entry);
    hlsl_replace_node(instr, vsir_instr);
}

static void sm4_generate_vsir_instr_dcl_semantic(struct hlsl_ctx *ctx, struct vsir_program *program,
        const struct hlsl_ir_var *var, bool is_patch_constant_func, struct hlsl_block *block,
        const struct vkd3d_shader_location *loc)
{
    const struct vkd3d_shader_version *version = &program->shader_version;
    const bool output = var->is_output_semantic;
    enum vkd3d_shader_sysval_semantic semantic;
    struct vkd3d_shader_dst_param *dst_param;
    struct vkd3d_shader_instruction *ins;
    enum vkd3d_shader_register_type type;
    enum vkd3d_shader_opcode opcode;
    unsigned int idx = 0;
    uint32_t write_mask;
    bool has_idx;

    sm4_sysval_semantic_from_semantic_name(&semantic, version, ctx->semantic_compat_mapping,
            ctx->domain, var->semantic.name, var->semantic.index, output, is_patch_constant_func);
    if (semantic == ~0u)
        semantic = VKD3D_SHADER_SV_NONE;

    if (var->is_input_semantic)
    {
        switch (semantic)
        {
            case VKD3D_SHADER_SV_NONE:
                opcode = (version->type == VKD3D_SHADER_TYPE_PIXEL)
                        ? VKD3DSIH_DCL_INPUT_PS : VKD3DSIH_DCL_INPUT;
                break;

            case VKD3D_SHADER_SV_INSTANCE_ID:
            case VKD3D_SHADER_SV_IS_FRONT_FACE:
            case VKD3D_SHADER_SV_PRIMITIVE_ID:
            case VKD3D_SHADER_SV_SAMPLE_INDEX:
            case VKD3D_SHADER_SV_VERTEX_ID:
                opcode = (version->type == VKD3D_SHADER_TYPE_PIXEL)
                        ? VKD3DSIH_DCL_INPUT_PS_SGV : VKD3DSIH_DCL_INPUT_SGV;
                break;

            default:
                opcode = (version->type == VKD3D_SHADER_TYPE_PIXEL)
                        ? VKD3DSIH_DCL_INPUT_PS_SIV : VKD3DSIH_DCL_INPUT_SIV;
                break;
        }
    }
    else
    {
        if (semantic == VKD3D_SHADER_SV_NONE || version->type == VKD3D_SHADER_TYPE_PIXEL)
            opcode = VKD3DSIH_DCL_OUTPUT;
        else
            opcode = VKD3DSIH_DCL_OUTPUT_SIV;
    }

    if (sm4_register_from_semantic_name(version, var->semantic.name, output, &type, &has_idx))
    {
        if (has_idx)
            idx = var->semantic.index;
        write_mask = (1u << var->data_type->dimx) - 1;
    }
    else
    {
        if (output)
            type = VKD3DSPR_OUTPUT;
        else if (version->type == VKD3D_SHADER_TYPE_DOMAIN)
            type = VKD3DSPR_PATCHCONST;
        else
            type = VKD3DSPR_INPUT;

        has_idx = true;
        idx = var->regs[HLSL_REGSET_NUMERIC].id;
        write_mask = var->regs[HLSL_REGSET_NUMERIC].writemask;
    }

    if (!(ins = generate_vsir_add_program_instruction(ctx, program, loc, opcode, 0, 0)))
        return;

    if (opcode == VKD3DSIH_DCL_OUTPUT)
    {
        VKD3D_ASSERT(semantic == VKD3D_SHADER_SV_NONE
                || semantic == VKD3D_SHADER_SV_TARGET || type != VKD3DSPR_OUTPUT);
        dst_param = &ins->declaration.dst;
    }
    else if (opcode == VKD3DSIH_DCL_INPUT || opcode == VKD3DSIH_DCL_INPUT_PS)
    {
        VKD3D_ASSERT(semantic == VKD3D_SHADER_SV_NONE);
        dst_param = &ins->declaration.dst;
    }
    else
    {
        VKD3D_ASSERT(semantic != VKD3D_SHADER_SV_NONE);
        ins->declaration.register_semantic.sysval_semantic = vkd3d_siv_from_sysval_indexed(semantic,
                var->semantic.index);
        dst_param = &ins->declaration.register_semantic.reg;
    }

    if (has_idx)
    {
        vsir_register_init(&dst_param->reg, type, VKD3D_DATA_FLOAT, 1);
        dst_param->reg.idx[0].offset = idx;
    }
    else
    {
        vsir_register_init(&dst_param->reg, type, VKD3D_DATA_FLOAT, 0);
    }

    if (shader_sm4_is_scalar_register(&dst_param->reg))
        dst_param->reg.dimension = VSIR_DIMENSION_SCALAR;
    else
        dst_param->reg.dimension = VSIR_DIMENSION_VEC4;

    dst_param->write_mask = write_mask;

    if (var->is_input_semantic && version->type == VKD3D_SHADER_TYPE_PIXEL)
        ins->flags = sm4_get_interpolation_mode(var->data_type, var->storage_modifiers);

    add_last_vsir_instr_to_block(ctx, program, block);
}

static void sm4_generate_vsir_instr_dcl_temps(struct hlsl_ctx *ctx, struct vsir_program *program,
        uint32_t temp_count, struct hlsl_block *block, const struct vkd3d_shader_location *loc)
{
    struct vkd3d_shader_instruction *ins;

    if (!(ins = generate_vsir_add_program_instruction(ctx, program, loc, VKD3DSIH_DCL_TEMPS, 0, 0)))
        return;

    ins->declaration.count = temp_count;

    add_last_vsir_instr_to_block(ctx, program, block);
}

static void sm4_generate_vsir_instr_dcl_indexable_temp(struct hlsl_ctx *ctx,
        struct vsir_program *program, struct hlsl_block *block, uint32_t idx,
        uint32_t size, uint32_t comp_count, const struct vkd3d_shader_location *loc)
{
    struct vkd3d_shader_instruction *ins;

    if (!(ins = generate_vsir_add_program_instruction(ctx, program, loc, VKD3DSIH_DCL_INDEXABLE_TEMP, 0, 0)))
        return;

    ins->declaration.indexable_temp.register_idx = idx;
    ins->declaration.indexable_temp.register_size = size;
    ins->declaration.indexable_temp.alignment = 0;
    ins->declaration.indexable_temp.data_type = VKD3D_DATA_FLOAT;
    ins->declaration.indexable_temp.component_count = comp_count;
    ins->declaration.indexable_temp.has_function_scope = false;

    add_last_vsir_instr_to_block(ctx, program, block);
}

static bool type_is_float(const struct hlsl_type *type)
{
    return type->e.numeric.type == HLSL_TYPE_FLOAT || type->e.numeric.type == HLSL_TYPE_HALF;
}

static bool type_is_integer(const struct hlsl_type *type)
{
    return type->e.numeric.type == HLSL_TYPE_BOOL
            || type->e.numeric.type == HLSL_TYPE_INT
            || type->e.numeric.type == HLSL_TYPE_UINT;
}

static void sm4_generate_vsir_cast_from_bool(struct hlsl_ctx *ctx, struct vsir_program *program,
        const struct hlsl_ir_expr *expr, uint32_t bits)
{
    struct hlsl_ir_node *operand = expr->operands[0].node;
    const struct hlsl_ir_node *instr = &expr->node;
    struct vkd3d_shader_dst_param *dst_param;
    struct hlsl_constant_value value = {0};
    struct vkd3d_shader_instruction *ins;

    VKD3D_ASSERT(instr->reg.allocated);

    if (!(ins = generate_vsir_add_program_instruction(ctx, program, &instr->loc, VKD3DSIH_AND, 1, 2)))
        return;

    dst_param = &ins->dst[0];
    vsir_dst_from_hlsl_node(dst_param, ctx, instr);

    vsir_src_from_hlsl_node(&ins->src[0], ctx, operand, dst_param->write_mask);

    value.u[0].u = bits;
    vsir_src_from_hlsl_constant_value(&ins->src[1], ctx, &value, VKD3D_DATA_UINT, 1, 0);
}

static bool sm4_generate_vsir_instr_expr_cast(struct hlsl_ctx *ctx,
        struct vsir_program *program, struct hlsl_ir_expr *expr)
{
    const struct hlsl_ir_node *arg1 = expr->operands[0].node;
    const struct hlsl_type *dst_type = expr->node.data_type;
    const struct hlsl_type *src_type = arg1->data_type;

    static const union
    {
        uint32_t u;
        float f;
    } one = { .f = 1.0 };

    /* Narrowing casts were already lowered. */
    VKD3D_ASSERT(src_type->dimx == dst_type->dimx);

    switch (dst_type->e.numeric.type)
    {
        case HLSL_TYPE_HALF:
        case HLSL_TYPE_FLOAT:
            switch (src_type->e.numeric.type)
            {
                case HLSL_TYPE_HALF:
                case HLSL_TYPE_FLOAT:
                    generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_MOV, 0, 0, true);
                    return true;

                case HLSL_TYPE_INT:
                    generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_ITOF, 0, 0, true);
                    return true;

                case HLSL_TYPE_UINT:
                    generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_UTOF, 0, 0, true);
                    return true;

                case HLSL_TYPE_BOOL:
                    sm4_generate_vsir_cast_from_bool(ctx, program, expr, one.u);
                    return true;

                case HLSL_TYPE_DOUBLE:
                    hlsl_fixme(ctx, &expr->node.loc, "SM4 cast from double to float.");
                    return false;

                default:
                    vkd3d_unreachable();
            }
            break;

        case HLSL_TYPE_INT:
            switch (src_type->e.numeric.type)
            {
                case HLSL_TYPE_HALF:
                case HLSL_TYPE_FLOAT:
                    generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_FTOI, 0, 0, true);
                    return true;

                case HLSL_TYPE_INT:
                case HLSL_TYPE_UINT:
                    generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_MOV, 0, 0, true);
                    return true;

                case HLSL_TYPE_BOOL:
                    sm4_generate_vsir_cast_from_bool(ctx, program, expr, 1u);
                    return true;

                case HLSL_TYPE_DOUBLE:
                    hlsl_fixme(ctx, &expr->node.loc, "SM4 cast from double to int.");
                    return false;

                default:
                    vkd3d_unreachable();
            }
            break;

        case HLSL_TYPE_UINT:
            switch (src_type->e.numeric.type)
            {
                case HLSL_TYPE_HALF:
                case HLSL_TYPE_FLOAT:
                    generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_FTOU, 0, 0, true);
                    return true;

                case HLSL_TYPE_INT:
                case HLSL_TYPE_UINT:
                    generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_MOV, 0, 0, true);
                    return true;

                case HLSL_TYPE_BOOL:
                    sm4_generate_vsir_cast_from_bool(ctx, program, expr, 1u);
                    return true;

                case HLSL_TYPE_DOUBLE:
                    hlsl_fixme(ctx, &expr->node.loc, "SM4 cast from double to uint.");
                    return false;

                default:
                    vkd3d_unreachable();
            }
            break;

        case HLSL_TYPE_DOUBLE:
            hlsl_fixme(ctx, &expr->node.loc, "SM4 cast to double.");
            return false;

        case HLSL_TYPE_BOOL:
            /* Casts to bool should have already been lowered. */
        default:
            vkd3d_unreachable();
    }
}

static void sm4_generate_vsir_expr_with_two_destinations(struct hlsl_ctx *ctx, struct vsir_program *program,
        enum vkd3d_shader_opcode opcode, const struct hlsl_ir_expr *expr, unsigned int dst_idx)
{
    struct vkd3d_shader_dst_param *dst_param, *null_param;
    const struct hlsl_ir_node *instr = &expr->node;
    struct vkd3d_shader_instruction *ins;
    unsigned int i, src_count;

    VKD3D_ASSERT(instr->reg.allocated);

    for (i = 0; i < HLSL_MAX_OPERANDS; ++i)
    {
        if (expr->operands[i].node)
            src_count = i + 1;
    }

    if (!(ins = generate_vsir_add_program_instruction(ctx, program, &instr->loc, opcode, 2, src_count)))
        return;

    dst_param = &ins->dst[dst_idx];
    vsir_dst_from_hlsl_node(dst_param, ctx, instr);

    null_param = &ins->dst[1 - dst_idx];
    vsir_dst_param_init(null_param, VKD3DSPR_NULL, VKD3D_DATA_FLOAT, 0);
    null_param->reg.dimension = VSIR_DIMENSION_NONE;

    for (i = 0; i < src_count; ++i)
        vsir_src_from_hlsl_node(&ins->src[i], ctx, expr->operands[i].node, dst_param->write_mask);
}

static void sm4_generate_vsir_rcp_using_div(struct hlsl_ctx *ctx,
        struct vsir_program *program, const struct hlsl_ir_expr *expr)
{
    struct hlsl_ir_node *operand = expr->operands[0].node;
    const struct hlsl_ir_node *instr = &expr->node;
    struct vkd3d_shader_dst_param *dst_param;
    struct hlsl_constant_value value = {0};
    struct vkd3d_shader_instruction *ins;

    VKD3D_ASSERT(type_is_float(expr->node.data_type));

    if (!(ins = generate_vsir_add_program_instruction(ctx, program, &instr->loc, VKD3DSIH_DIV, 1, 2)))
        return;

    dst_param = &ins->dst[0];
    vsir_dst_from_hlsl_node(dst_param, ctx, instr);

    value.u[0].f = 1.0f;
    value.u[1].f = 1.0f;
    value.u[2].f = 1.0f;
    value.u[3].f = 1.0f;
    vsir_src_from_hlsl_constant_value(&ins->src[0], ctx, &value,
            VKD3D_DATA_FLOAT, instr->data_type->dimx, dst_param->write_mask);

    vsir_src_from_hlsl_node(&ins->src[1], ctx, operand, dst_param->write_mask);
}

static bool sm4_generate_vsir_instr_expr(struct hlsl_ctx *ctx,
        struct vsir_program *program, struct hlsl_ir_expr *expr, const char *dst_type_name)
{
    const struct hlsl_type *dst_type = expr->node.data_type;
    const struct hlsl_type *src_type = NULL;

    VKD3D_ASSERT(expr->node.reg.allocated);
    if (expr->operands[0].node)
        src_type = expr->operands[0].node->data_type;

    switch (expr->op)
    {
        case HLSL_OP0_RASTERIZER_SAMPLE_COUNT:
            sm4_generate_vsir_rasterizer_sample_count(ctx, program, expr);
            return true;

        case HLSL_OP1_ABS:
            VKD3D_ASSERT(type_is_float(dst_type));
            generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_MOV, VKD3DSPSM_ABS, 0, true);
            return true;

        case HLSL_OP1_BIT_NOT:
            VKD3D_ASSERT(type_is_integer(dst_type));
            generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_NOT, 0, 0, true);
            return true;

        case HLSL_OP1_CAST:
            return sm4_generate_vsir_instr_expr_cast(ctx, program, expr);

        case HLSL_OP1_CEIL:
            VKD3D_ASSERT(type_is_float(dst_type));
            generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_ROUND_PI, 0, 0, true);
            return true;

        case HLSL_OP1_COS:
            VKD3D_ASSERT(type_is_float(dst_type));
            sm4_generate_vsir_expr_with_two_destinations(ctx, program, VKD3DSIH_SINCOS, expr, 1);
            return true;

        case HLSL_OP1_DSX:
            VKD3D_ASSERT(type_is_float(dst_type));
            generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_DSX, 0, 0, true);
            return true;

        case HLSL_OP1_DSX_COARSE:
            VKD3D_ASSERT(type_is_float(dst_type));
            generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_DSX_COARSE, 0, 0, true);
            return true;

        case HLSL_OP1_DSX_FINE:
            VKD3D_ASSERT(type_is_float(dst_type));
            generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_DSX_FINE, 0, 0, true);
            return true;

        case HLSL_OP1_DSY:
            VKD3D_ASSERT(type_is_float(dst_type));
            generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_DSY, 0, 0, true);
            return true;

        case HLSL_OP1_DSY_COARSE:
            VKD3D_ASSERT(type_is_float(dst_type));
            generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_DSY_COARSE, 0, 0, true);
            return true;

        case HLSL_OP1_DSY_FINE:
            VKD3D_ASSERT(type_is_float(dst_type));
            generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_DSY_FINE, 0, 0, true);
            return true;

        case HLSL_OP1_EXP2:
            VKD3D_ASSERT(type_is_float(dst_type));
            generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_EXP, 0, 0, true);
            return true;

        case HLSL_OP1_F16TOF32:
            VKD3D_ASSERT(type_is_float(dst_type));
            VKD3D_ASSERT(hlsl_version_ge(ctx, 5, 0));
            generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_F16TOF32, 0, 0, true);
            return true;

        case HLSL_OP1_F32TOF16:
            VKD3D_ASSERT(dst_type->e.numeric.type == HLSL_TYPE_UINT);
            VKD3D_ASSERT(hlsl_version_ge(ctx, 5, 0));
            generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_F32TOF16, 0, 0, true);
            return true;

        case HLSL_OP1_FLOOR:
            VKD3D_ASSERT(type_is_float(dst_type));
            generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_ROUND_NI, 0, 0, true);
            return true;

        case HLSL_OP1_FRACT:
            VKD3D_ASSERT(type_is_float(dst_type));
            generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_FRC, 0, 0, true);
            return true;

        case HLSL_OP1_LOG2:
            VKD3D_ASSERT(type_is_float(dst_type));
            generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_LOG, 0, 0, true);
            return true;

        case HLSL_OP1_LOGIC_NOT:
            VKD3D_ASSERT(dst_type->e.numeric.type == HLSL_TYPE_BOOL);
            generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_NOT, 0, 0, true);
            return true;

        case HLSL_OP1_NEG:
            switch (dst_type->e.numeric.type)
            {
                case HLSL_TYPE_FLOAT:
                    generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_MOV, VKD3DSPSM_NEG, 0, true);
                    return true;

                case HLSL_TYPE_INT:
                case HLSL_TYPE_UINT:
                    generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_INEG, 0, 0, true);
                    return true;

                default:
                    hlsl_fixme(ctx, &expr->node.loc, "SM4 %s negation expression.", dst_type_name);
                    return false;
            }

        case HLSL_OP1_RCP:
            switch (dst_type->e.numeric.type)
            {
                case HLSL_TYPE_FLOAT:
                    /* SM5 comes with a RCP opcode */
                    if (hlsl_version_ge(ctx, 5, 0))
                        generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_RCP, 0, 0, true);
                    else
                        sm4_generate_vsir_rcp_using_div(ctx, program, expr);
                    return true;

                default:
                    hlsl_fixme(ctx, &expr->node.loc, "SM4 %s rcp expression.", dst_type_name);
                    return false;
            }

        case HLSL_OP1_REINTERPRET:
            generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_MOV, 0, 0, true);
            return true;

        case HLSL_OP1_ROUND:
            VKD3D_ASSERT(type_is_float(dst_type));
            generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_ROUND_NE, 0, 0, true);
            return true;

        case HLSL_OP1_RSQ:
            VKD3D_ASSERT(type_is_float(dst_type));
            generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_RSQ, 0, 0, true);
            return true;

        case HLSL_OP1_SAT:
            VKD3D_ASSERT(type_is_float(dst_type));
            generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_MOV, 0, VKD3DSPDM_SATURATE, true);
            return true;

        case HLSL_OP1_SIN:
            VKD3D_ASSERT(type_is_float(dst_type));
            sm4_generate_vsir_expr_with_two_destinations(ctx, program, VKD3DSIH_SINCOS, expr, 0);
            return true;

        case HLSL_OP1_SQRT:
            VKD3D_ASSERT(type_is_float(dst_type));
            generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_SQRT, 0, 0, true);
            return true;

        case HLSL_OP1_TRUNC:
            VKD3D_ASSERT(type_is_float(dst_type));
            generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_ROUND_Z, 0, 0, true);
            return true;

        case HLSL_OP2_ADD:
            switch (dst_type->e.numeric.type)
            {
                case HLSL_TYPE_FLOAT:
                    generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_ADD, 0, 0, true);
                    return true;

                case HLSL_TYPE_INT:
                case HLSL_TYPE_UINT:
                    generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_IADD, 0, 0, true);
                    return true;

                default:
                    hlsl_fixme(ctx, &expr->node.loc, "SM4 %s addition expression.", dst_type_name);
                    return false;
            }

        case HLSL_OP2_BIT_AND:
            VKD3D_ASSERT(type_is_integer(dst_type));
            generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_AND, 0, 0, true);
            return true;

        case HLSL_OP2_BIT_OR:
            VKD3D_ASSERT(type_is_integer(dst_type));
            generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_OR, 0, 0, true);
            return true;

        case HLSL_OP2_BIT_XOR:
            VKD3D_ASSERT(type_is_integer(dst_type));
            generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_XOR, 0, 0, true);
            return true;

        case HLSL_OP2_DIV:
            switch (dst_type->e.numeric.type)
            {
                case HLSL_TYPE_FLOAT:
                    generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_DIV, 0, 0, true);
                    return true;

                case HLSL_TYPE_UINT:
                    sm4_generate_vsir_expr_with_two_destinations(ctx, program, VKD3DSIH_UDIV, expr, 0);
                    return true;

                default:
                    hlsl_fixme(ctx, &expr->node.loc, "SM4 %s division expression.", dst_type_name);
                    return false;
            }

        case HLSL_OP2_DOT:
            switch (dst_type->e.numeric.type)
            {
                case HLSL_TYPE_FLOAT:
                    switch (expr->operands[0].node->data_type->dimx)
                    {
                        case 4:
                            generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_DP4, 0, 0, false);
                            return true;

                        case 3:
                            generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_DP3, 0, 0, false);
                            return true;

                        case 2:
                            generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_DP2, 0, 0, false);
                            return true;

                        case 1:
                        default:
                            vkd3d_unreachable();
                    }

                default:
                    hlsl_fixme(ctx, &expr->node.loc, "SM4 %s dot expression.", dst_type_name);
                    return false;
            }

        case HLSL_OP2_EQUAL:
            VKD3D_ASSERT(dst_type->e.numeric.type == HLSL_TYPE_BOOL);

            switch (src_type->e.numeric.type)
            {
                case HLSL_TYPE_FLOAT:
                    generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_EQO, 0, 0, true);
                    return true;

                case HLSL_TYPE_BOOL:
                case HLSL_TYPE_INT:
                case HLSL_TYPE_UINT:
                    generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_IEQ, 0, 0, true);
                    return true;

                default:
                    hlsl_fixme(ctx, &expr->node.loc, "SM4 equality between \"%s\" operands.",
                            debug_hlsl_type(ctx, src_type));
                    return false;
            }

        case HLSL_OP2_GEQUAL:
            VKD3D_ASSERT(dst_type->e.numeric.type == HLSL_TYPE_BOOL);

            switch (src_type->e.numeric.type)
            {
                case HLSL_TYPE_FLOAT:
                    generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_GEO, 0, 0, true);
                    return true;

                case HLSL_TYPE_INT:
                    generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_IGE, 0, 0, true);
                    return true;

                case HLSL_TYPE_BOOL:
                case HLSL_TYPE_UINT:
                    generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_UGE, 0, 0, true);
                    return true;

                default:
                    hlsl_fixme(ctx, &expr->node.loc, "SM4 greater-than-or-equal between \"%s\" operands.",
                            debug_hlsl_type(ctx, src_type));
                    return false;
            }

        case HLSL_OP2_LESS:
            VKD3D_ASSERT(dst_type->e.numeric.type == HLSL_TYPE_BOOL);

            switch (src_type->e.numeric.type)
            {
                case HLSL_TYPE_FLOAT:
                    generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_LTO, 0, 0, true);
                    return true;

                case HLSL_TYPE_INT:
                    generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_ILT, 0, 0, true);
                    return true;

                case HLSL_TYPE_BOOL:
                case HLSL_TYPE_UINT:
                    generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_ULT, 0, 0, true);
                    return true;

                default:
                    hlsl_fixme(ctx, &expr->node.loc, "SM4 less-than between \"%s\" operands.",
                            debug_hlsl_type(ctx, src_type));
                    return false;
            }

        case HLSL_OP2_LOGIC_AND:
            VKD3D_ASSERT(dst_type->e.numeric.type == HLSL_TYPE_BOOL);
            generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_AND, 0, 0, true);
            return true;

        case HLSL_OP2_LOGIC_OR:
            VKD3D_ASSERT(dst_type->e.numeric.type == HLSL_TYPE_BOOL);
            generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_OR, 0, 0, true);
            return true;

        case HLSL_OP2_LSHIFT:
            VKD3D_ASSERT(type_is_integer(dst_type));
            VKD3D_ASSERT(dst_type->e.numeric.type != HLSL_TYPE_BOOL);
            generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_ISHL, 0, 0, true);
            return true;

        case HLSL_OP3_MAD:
            switch (dst_type->e.numeric.type)
            {
                case HLSL_TYPE_FLOAT:
                    generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_MAD, 0, 0, true);
                    return true;

                case HLSL_TYPE_INT:
                case HLSL_TYPE_UINT:
                    generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_IMAD, 0, 0, true);
                    return true;

                default:
                    hlsl_fixme(ctx, &expr->node.loc, "SM4 %s MAD expression.", dst_type_name);
                    return false;
            }

        case HLSL_OP2_MAX:
            switch (dst_type->e.numeric.type)
            {
                case HLSL_TYPE_FLOAT:
                    generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_MAX, 0, 0, true);
                    return true;

                case HLSL_TYPE_INT:
                    generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_IMAX, 0, 0, true);
                    return true;

                case HLSL_TYPE_UINT:
                    generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_UMAX, 0, 0, true);
                    return true;

                default:
                    hlsl_fixme(ctx, &expr->node.loc, "SM4 %s maximum expression.", dst_type_name);
                    return false;
            }

        case HLSL_OP2_MIN:
            switch (dst_type->e.numeric.type)
            {
                case HLSL_TYPE_FLOAT:
                    generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_MIN, 0, 0, true);
                    return true;

                case HLSL_TYPE_INT:
                    generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_IMIN, 0, 0, true);
                    return true;

                case HLSL_TYPE_UINT:
                    generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_UMIN, 0, 0, true);
                    return true;

                default:
                    hlsl_fixme(ctx, &expr->node.loc, "SM4 %s minimum expression.", dst_type_name);
                    return false;
            }

        case HLSL_OP2_MOD:
            switch (dst_type->e.numeric.type)
            {
                case HLSL_TYPE_UINT:
                    sm4_generate_vsir_expr_with_two_destinations(ctx, program, VKD3DSIH_UDIV, expr, 1);
                    return true;

                default:
                    hlsl_fixme(ctx, &expr->node.loc, "SM4 %s modulus expression.", dst_type_name);
                    return false;
            }

        case HLSL_OP2_MUL:
            switch (dst_type->e.numeric.type)
            {
                case HLSL_TYPE_FLOAT:
                    generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_MUL, 0, 0, true);
                    return true;

                case HLSL_TYPE_INT:
                case HLSL_TYPE_UINT:
                    /* Using IMUL instead of UMUL because we're taking the low
                     * bits, and the native compiler generates IMUL. */
                    sm4_generate_vsir_expr_with_two_destinations(ctx, program, VKD3DSIH_IMUL, expr, 1);
                    return true;

                default:
                    hlsl_fixme(ctx, &expr->node.loc, "SM4 %s multiplication expression.", dst_type_name);
                    return false;
            }

        case HLSL_OP2_NEQUAL:
            VKD3D_ASSERT(dst_type->e.numeric.type == HLSL_TYPE_BOOL);

            switch (src_type->e.numeric.type)
            {
                case HLSL_TYPE_FLOAT:
                    generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_NEU, 0, 0, true);
                    return true;

                case HLSL_TYPE_BOOL:
                case HLSL_TYPE_INT:
                case HLSL_TYPE_UINT:
                    generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_INE, 0, 0, true);
                    return true;

                default:
                    hlsl_fixme(ctx, &expr->node.loc, "SM4 inequality between \"%s\" operands.",
                            debug_hlsl_type(ctx, src_type));
                    return false;
            }

        case HLSL_OP2_RSHIFT:
            VKD3D_ASSERT(type_is_integer(dst_type));
            VKD3D_ASSERT(dst_type->e.numeric.type != HLSL_TYPE_BOOL);
            generate_vsir_instr_expr_single_instr_op(ctx, program, expr,
                    dst_type->e.numeric.type == HLSL_TYPE_INT ? VKD3DSIH_ISHR : VKD3DSIH_USHR, 0, 0, true);
            return true;

        case HLSL_OP3_TERNARY:
            generate_vsir_instr_expr_single_instr_op(ctx, program, expr, VKD3DSIH_MOVC, 0, 0, true);
            return true;

        default:
            hlsl_fixme(ctx, &expr->node.loc, "SM4 %s expression.", debug_hlsl_expr_op(expr->op));
            return false;
    }
}

static void sm4_generate_vsir_block(struct hlsl_ctx *ctx, struct hlsl_block *block, struct vsir_program *program)
{
    struct vkd3d_string_buffer *dst_type_string;
    struct hlsl_ir_node *instr, *next;
    struct hlsl_ir_switch_case *c;

    LIST_FOR_EACH_ENTRY_SAFE(instr, next, &block->instrs, struct hlsl_ir_node, entry)
    {
        if (instr->data_type)
        {
            if (instr->data_type->class != HLSL_CLASS_SCALAR && instr->data_type->class != HLSL_CLASS_VECTOR)
            {
                hlsl_fixme(ctx, &instr->loc, "Class %#x should have been lowered or removed.", instr->data_type->class);
                break;
            }
        }

        switch (instr->type)
        {
            case HLSL_IR_CALL:
                vkd3d_unreachable();

            case HLSL_IR_CONSTANT:
                /* In SM4 all constants are inlined. */
                break;

            case HLSL_IR_EXPR:
                if (!(dst_type_string = hlsl_type_to_string(ctx, instr->data_type)))
                    break;

                if (sm4_generate_vsir_instr_expr(ctx, program, hlsl_ir_expr(instr), dst_type_string->buffer))
                    replace_instr_with_last_vsir_instr(ctx, program, instr);

                hlsl_release_string_buffer(ctx, dst_type_string);
                break;

            case HLSL_IR_IF:
                sm4_generate_vsir_block(ctx, &hlsl_ir_if(instr)->then_block, program);
                sm4_generate_vsir_block(ctx, &hlsl_ir_if(instr)->else_block, program);
                break;

            case HLSL_IR_LOOP:
                sm4_generate_vsir_block(ctx, &hlsl_ir_loop(instr)->body, program);
                break;

            case HLSL_IR_SWITCH:
                LIST_FOR_EACH_ENTRY(c, &hlsl_ir_switch(instr)->cases, struct hlsl_ir_switch_case, entry)
                    sm4_generate_vsir_block(ctx, &c->body, program);
                break;

            case HLSL_IR_SWIZZLE:
                generate_vsir_instr_swizzle(ctx, program, hlsl_ir_swizzle(instr));
                replace_instr_with_last_vsir_instr(ctx, program, instr);
                break;

            default:
                break;
        }
    }
}

static void sm4_generate_vsir_add_function(struct hlsl_ctx *ctx,
        struct hlsl_ir_function_decl *func, uint64_t config_flags, struct vsir_program *program)
{
    bool is_patch_constant_func = func == ctx->patch_constant_func;
    struct hlsl_block block = {0};
    struct hlsl_scope *scope;
    struct hlsl_ir_var *var;
    uint32_t temp_count;

    compute_liveness(ctx, func);
    mark_indexable_vars(ctx, func);
    temp_count = allocate_temp_registers(ctx, func);
    if (ctx->result)
        return;
    program->temp_count = max(program->temp_count, temp_count);

    hlsl_block_init(&block);

    LIST_FOR_EACH_ENTRY(var, &func->extern_vars, struct hlsl_ir_var, extern_entry)
    {
        if ((var->is_input_semantic && var->last_read)
                || (var->is_output_semantic && var->first_write))
            sm4_generate_vsir_instr_dcl_semantic(ctx, program, var, is_patch_constant_func, &block, &var->loc);
    }

    if (temp_count)
        sm4_generate_vsir_instr_dcl_temps(ctx, program, temp_count, &block, &func->loc);

    LIST_FOR_EACH_ENTRY(scope, &ctx->scopes, struct hlsl_scope, entry)
    {
        LIST_FOR_EACH_ENTRY(var, &scope->vars, struct hlsl_ir_var, scope_entry)
        {
            if (var->is_uniform || var->is_input_semantic || var->is_output_semantic)
                continue;
            if (!var->regs[HLSL_REGSET_NUMERIC].allocated)
                continue;

            if (var->indexable)
            {
                unsigned int id = var->regs[HLSL_REGSET_NUMERIC].id;
                unsigned int size = align(var->data_type->reg_size[HLSL_REGSET_NUMERIC], 4) / 4;

                sm4_generate_vsir_instr_dcl_indexable_temp(ctx, program, &block, id, size, 4, &var->loc);
            }
        }
    }

    list_move_head(&func->body.instrs, &block.instrs);

    hlsl_block_cleanup(&block);

    sm4_generate_vsir_block(ctx, &func->body, program);
}

/* OBJECTIVE: Translate all the information from ctx and entry_func to the
 * vsir_program, so it can be used as input to tpf_compile() without relying
 * on ctx and entry_func. */
static void sm4_generate_vsir(struct hlsl_ctx *ctx, struct hlsl_ir_function_decl *func,
        uint64_t config_flags, struct vsir_program *program)
{
    struct vkd3d_shader_version version = {0};

    version.major = ctx->profile->major_version;
    version.minor = ctx->profile->minor_version;
    version.type = ctx->profile->type;

    if (!vsir_program_init(program, NULL, &version, 0, VSIR_CF_STRUCTURED, VSIR_NOT_NORMALISED))
    {
        ctx->result = VKD3D_ERROR_OUT_OF_MEMORY;
        return;
    }

    generate_vsir_signature(ctx, program, func);
    if (version.type == VKD3D_SHADER_TYPE_HULL)
        generate_vsir_signature(ctx, program, ctx->patch_constant_func);

    if (version.type == VKD3D_SHADER_TYPE_COMPUTE)
    {
        program->thread_group_size.x = ctx->thread_count[0];
        program->thread_group_size.y = ctx->thread_count[1];
        program->thread_group_size.z = ctx->thread_count[2];
    }

    sm4_generate_vsir_add_function(ctx, func, config_flags, program);
    if (version.type == VKD3D_SHADER_TYPE_HULL)
        sm4_generate_vsir_add_function(ctx, ctx->patch_constant_func, config_flags, program);
}

static struct hlsl_ir_jump *loop_unrolling_find_jump(struct hlsl_block *block, struct hlsl_ir_node *stop_point,
        struct hlsl_block **found_block)
{
    struct hlsl_ir_node *node;

    LIST_FOR_EACH_ENTRY(node, &block->instrs, struct hlsl_ir_node, entry)
    {
        if (node == stop_point)
            return NULL;

        if (node->type == HLSL_IR_IF)
        {
            struct hlsl_ir_if *iff = hlsl_ir_if(node);
            struct hlsl_ir_jump *jump = NULL;

            if ((jump = loop_unrolling_find_jump(&iff->then_block, stop_point, found_block)))
                return jump;
            if ((jump = loop_unrolling_find_jump(&iff->else_block, stop_point, found_block)))
                return jump;
        }
        else if (node->type == HLSL_IR_JUMP)
        {
            struct hlsl_ir_jump *jump = hlsl_ir_jump(node);

            if (jump->type == HLSL_IR_JUMP_BREAK || jump->type == HLSL_IR_JUMP_CONTINUE)
            {
                *found_block = block;
                return jump;
            }
        }
    }

    return NULL;
}

static unsigned int loop_unrolling_get_max_iterations(struct hlsl_ctx *ctx, struct hlsl_ir_loop *loop)
{
    /* Always use the explicit limit if it has been passed. */
    if (loop->unroll_limit)
        return loop->unroll_limit;

    /* All SMs will default to 1024 if [unroll] has been specified without an explicit limit. */
    if (loop->unroll_type == HLSL_IR_LOOP_FORCE_UNROLL)
        return 1024;

    /* SM4 limits implicit unrolling to 254 iterations. */
    if (hlsl_version_ge(ctx, 4, 0))
        return 254;

    /* SM<3 implicitly unrolls up to 1024 iterations. */
    return 1024;
}

static bool loop_unrolling_unroll_loop(struct hlsl_ctx *ctx, struct hlsl_block *block,
        struct hlsl_block *loop_parent, struct hlsl_ir_loop *loop)
{
    unsigned int max_iterations, i;

    max_iterations = loop_unrolling_get_max_iterations(ctx, loop);

    for (i = 0; i < max_iterations; ++i)
    {
        struct hlsl_block tmp_dst, *jump_block;
        struct hlsl_ir_jump *jump = NULL;

        if (!hlsl_clone_block(ctx, &tmp_dst, &loop->body))
            return false;
        list_move_before(&loop->node.entry, &tmp_dst.instrs);
        hlsl_block_cleanup(&tmp_dst);

        hlsl_run_const_passes(ctx, block);

        if ((jump = loop_unrolling_find_jump(loop_parent, &loop->node, &jump_block)))
        {
            enum hlsl_ir_jump_type type = jump->type;

            if (jump_block != loop_parent)
            {
                if (loop->unroll_type == HLSL_IR_LOOP_FORCE_UNROLL)
                    hlsl_error(ctx, &jump->node.loc, VKD3D_SHADER_ERROR_HLSL_FAILED_FORCED_UNROLL,
                        "Unable to unroll loop, unrolling loops with conditional jumps is currently not supported.");
                return false;
            }

            list_move_slice_tail(&tmp_dst.instrs, &jump->node.entry, list_prev(&loop_parent->instrs, &loop->node.entry));
            hlsl_block_cleanup(&tmp_dst);

            if (type == HLSL_IR_JUMP_BREAK)
                break;
        }
    }

    /* Native will not emit an error if max_iterations has been reached with an
     * explicit limit. It also will not insert a loop if there are iterations left
     * i.e [unroll(4)] for (i = 0; i < 8; ++i)) */
    if (!loop->unroll_limit && i == max_iterations)
    {
        if (loop->unroll_type == HLSL_IR_LOOP_FORCE_UNROLL)
            hlsl_error(ctx, &loop->node.loc, VKD3D_SHADER_ERROR_HLSL_FAILED_FORCED_UNROLL,
                "Unable to unroll loop, maximum iterations reached (%u).", max_iterations);
        return false;
    }

    list_remove(&loop->node.entry);
    hlsl_free_instr(&loop->node);

    return true;
}

/*
 * loop_unrolling_find_unrollable_loop() is not the normal way to do things;
 * normal passes simply iterate over the whole block and apply a transformation
 * to every relevant instruction. However, loop unrolling can fail, and we want
 * to leave the loop in its previous state in that case. That isn't a problem by
 * itself, except that loop unrolling needs copy-prop in order to work properly,
 * and copy-prop state at the time of the loop depends on the rest of the program
 * up to that point. This means we need to clone the whole program, and at that
 * point we have to search it again anyway to find the clone of the loop we were
 * going to unroll.
 *
 * FIXME: Ideally we wouldn't clone the whole program; instead we would run copyprop
 * up until the loop instruction, clone just that loop, then use copyprop again
 * with the saved state after unrolling. However, copyprop currently isn't built
 * for that yet [notably, it still relies on indices]. Note also this still doesn't
 * really let us use transform_ir() anyway [since we don't have a good way to say
 * "copyprop from the beginning of the program up to the instruction we're
 * currently processing" from the callback]; we'd have to use a dedicated
 * recursive function instead. */
static struct hlsl_ir_loop *loop_unrolling_find_unrollable_loop(struct hlsl_ctx *ctx, struct hlsl_block *block,
        struct hlsl_block **containing_block)
{
    struct hlsl_ir_node *instr;

    LIST_FOR_EACH_ENTRY(instr, &block->instrs, struct hlsl_ir_node, entry)
    {
        switch (instr->type)
        {
            case HLSL_IR_LOOP:
            {
                struct hlsl_ir_loop *nested_loop;
                struct hlsl_ir_loop *loop = hlsl_ir_loop(instr);

                if ((nested_loop = loop_unrolling_find_unrollable_loop(ctx, &loop->body, containing_block)))
                    return nested_loop;

                if (loop->unroll_type == HLSL_IR_LOOP_UNROLL || loop->unroll_type == HLSL_IR_LOOP_FORCE_UNROLL)
                {
                    *containing_block = block;
                    return loop;
                }

                break;
            }
            case HLSL_IR_IF:
            {
                struct hlsl_ir_loop *loop;
                struct hlsl_ir_if *iff = hlsl_ir_if(instr);

                if ((loop = loop_unrolling_find_unrollable_loop(ctx, &iff->then_block, containing_block)))
                    return loop;
                if ((loop = loop_unrolling_find_unrollable_loop(ctx, &iff->else_block, containing_block)))
                    return loop;

                break;
            }
            case HLSL_IR_SWITCH:
            {
                struct hlsl_ir_switch *s = hlsl_ir_switch(instr);
                struct hlsl_ir_switch_case *c;
                struct hlsl_ir_loop *loop;

                LIST_FOR_EACH_ENTRY(c, &s->cases, struct hlsl_ir_switch_case, entry)
                {
                    if ((loop = loop_unrolling_find_unrollable_loop(ctx, &c->body, containing_block)))
                        return loop;
                }

                break;
            }
            default:
                break;
        }
    }

    return NULL;
}

static void transform_unroll_loops(struct hlsl_ctx *ctx, struct hlsl_block *block)
{
    while (true)
    {
        struct hlsl_block clone, *containing_block;
        struct hlsl_ir_loop *loop, *cloned_loop;

        if (!(loop = loop_unrolling_find_unrollable_loop(ctx, block, &containing_block)))
            return;

        if (!hlsl_clone_block(ctx, &clone, block))
            return;

        cloned_loop = loop_unrolling_find_unrollable_loop(ctx, &clone, &containing_block);
        VKD3D_ASSERT(cloned_loop);

        if (!loop_unrolling_unroll_loop(ctx, &clone, containing_block, cloned_loop))
        {
            hlsl_block_cleanup(&clone);
            loop->unroll_type = HLSL_IR_LOOP_FORCE_LOOP;
            continue;
        }

        hlsl_block_cleanup(block);
        hlsl_block_init(block);
        hlsl_block_add_block(block, &clone);
    }
}

static bool lower_f16tof32(struct hlsl_ctx *ctx, struct hlsl_ir_node *node, struct hlsl_block *block)
{
    struct hlsl_ir_node *call, *rhs, *store;
    struct hlsl_ir_function_decl *func;
    unsigned int component_count;
    struct hlsl_ir_load *load;
    struct hlsl_ir_expr *expr;
    struct hlsl_ir_var *lhs;
    char *body;

    static const char template[] =
    "typedef uint%u uintX;\n"
    "float%u soft_f16tof32(uintX x)\n"
    "{\n"
    "    uintX mantissa = x & 0x3ff;\n"
    "    uintX high2 = mantissa >> 8;\n"
    "    uintX high2_check = high2 ? high2 : mantissa;\n"
    "    uintX high6 = high2_check >> 4;\n"
    "    uintX high6_check = high6 ? high6 : high2_check;\n"
    "\n"
    "    uintX high8 = high6_check >> 2;\n"
    "    uintX high8_check = (high8 ? high8 : high6_check) >> 1;\n"
    "    uintX shift = high6 ? (high2 ? 12 : 4) : (high2 ? 8 : 0);\n"
    "    shift = high8 ? shift + 2 : shift;\n"
    "    shift = high8_check ? shift + 1 : shift;\n"
    "    shift = -shift + 10;\n"
    "    shift = mantissa ? shift : 11;\n"
    "    uintX subnormal_mantissa = ((mantissa << shift) << 23) & 0x7fe000;\n"
    "    uintX subnormal_exp = -(shift << 23) + 0x38800000;\n"
    "    uintX subnormal_val = subnormal_exp + subnormal_mantissa;\n"
    "    uintX subnormal_or_zero = mantissa ? subnormal_val : 0;\n"
    "\n"
    "    uintX exponent = (((x >> 10) << 23) & 0xf800000) + 0x38000000;\n"
    "\n"
    "    uintX low_3 = (x << 13) & 0x7fe000;\n"
    "    uintX normalized_val = exponent + low_3;\n"
    "    uintX inf_nan_val = low_3 + 0x7f800000;\n"
    "\n"
    "    uintX exp_mask = 0x7c00;\n"
    "    uintX is_inf_nan = (x & exp_mask) == exp_mask;\n"
    "    uintX is_normalized = x & exp_mask;\n"
    "\n"
    "    uintX check = is_inf_nan ? inf_nan_val : normalized_val;\n"
    "    uintX exp_mantissa = (is_normalized ? check : subnormal_or_zero) & 0x7fffe000;\n"
    "    uintX sign_bit = (x << 16) & 0x80000000;\n"
    "\n"
    "    return asfloat(exp_mantissa + sign_bit);\n"
    "}\n";


    if (node->type != HLSL_IR_EXPR)
        return false;

    expr = hlsl_ir_expr(node);

    if (expr->op != HLSL_OP1_F16TOF32)
        return false;

    rhs = expr->operands[0].node;
    component_count = hlsl_type_component_count(rhs->data_type);

    if (!(body = hlsl_sprintf_alloc(ctx, template, component_count, component_count)))
        return false;

    if (!(func = hlsl_compile_internal_function(ctx, "soft_f16tof32", body)))
        return false;

    lhs = func->parameters.vars[0];

    if (!(store = hlsl_new_simple_store(ctx, lhs, rhs)))
        return false;
    hlsl_block_add_instr(block, store);

    if (!(call = hlsl_new_call(ctx, func, &node->loc)))
        return false;
    hlsl_block_add_instr(block, call);

    if (!(load = hlsl_new_var_load(ctx, func->return_var, &node->loc)))
        return false;
    hlsl_block_add_instr(block, &load->node);

    return true;
}

static bool lower_f32tof16(struct hlsl_ctx *ctx, struct hlsl_ir_node *node, struct hlsl_block *block)
{
    struct hlsl_ir_node *call, *rhs, *store;
    struct hlsl_ir_function_decl *func;
    unsigned int component_count;
    struct hlsl_ir_load *load;
    struct hlsl_ir_expr *expr;
    struct hlsl_ir_var *lhs;
    char *body;

    static const char template[] =
    "typedef uint%u uintX;\n"
    "uintX soft_f32tof16(float%u x)\n"
    "{\n"
    "    uintX v = asuint(x);\n"
    "    uintX v_abs = v & 0x7fffffff;\n"
    "    uintX sign_bit = (v >> 16) & 0x8000;\n"
    "    uintX exp = (v >> 23) & 0xff;\n"
    "    uintX mantissa = v & 0x7fffff;\n"
    "    uintX nan16;\n"
    "    uintX nan = (v & 0x7f800000) == 0x7f800000;\n"
    "    uintX val;\n"
    "\n"
    "    val = 113 - exp;\n"
    "    val = (mantissa + 0x800000) >> val;\n"
    "    val >>= 13;\n"
    "\n"
    "    val = (exp - 127) < -38 ? 0 : val;\n"
    "\n"
    "    val = v_abs < 0x38800000 ? val : (v_abs + 0xc8000000) >> 13;\n"
    "    val = v_abs > 0x47ffe000 ? 0x7bff : val;\n"
    "\n"
    "    nan16 = (((v >> 13) | (v >> 3) | v) & 0x3ff) + 0x7c00;\n"
    "    val = nan ? nan16 : val;\n"
    "\n"
    "    return (val & 0x7fff) + sign_bit;\n"
    "}\n";

    if (node->type != HLSL_IR_EXPR)
        return false;

    expr = hlsl_ir_expr(node);

    if (expr->op != HLSL_OP1_F32TOF16)
        return false;

    rhs = expr->operands[0].node;
    component_count = hlsl_type_component_count(rhs->data_type);

    if (!(body = hlsl_sprintf_alloc(ctx, template, component_count, component_count)))
        return false;

    if (!(func = hlsl_compile_internal_function(ctx, "soft_f32tof16", body)))
        return false;

    lhs = func->parameters.vars[0];

    if (!(store = hlsl_new_simple_store(ctx, lhs, rhs)))
        return false;
    hlsl_block_add_instr(block, store);

    if (!(call = hlsl_new_call(ctx, func, &node->loc)))
        return false;
    hlsl_block_add_instr(block, call);

    if (!(load = hlsl_new_var_load(ctx, func->return_var, &node->loc)))
        return false;
    hlsl_block_add_instr(block, &load->node);

    return true;
}

static void process_entry_function(struct hlsl_ctx *ctx,
        const struct hlsl_block *global_uniform_block, struct hlsl_ir_function_decl *entry_func)
{
    const struct hlsl_profile_info *profile = ctx->profile;
    struct hlsl_block static_initializers, global_uniforms;
    struct hlsl_block *const body = &entry_func->body;
    struct recursive_call_ctx recursive_call_ctx;
    struct hlsl_ir_var *var;
    unsigned int i;

    if (!hlsl_clone_block(ctx, &static_initializers, &ctx->static_initializers))
        return;
    list_move_head(&body->instrs, &static_initializers.instrs);

    if (!hlsl_clone_block(ctx, &global_uniforms, global_uniform_block))
        return;
    list_move_head(&body->instrs, &global_uniforms.instrs);

    memset(&recursive_call_ctx, 0, sizeof(recursive_call_ctx));
    hlsl_transform_ir(ctx, find_recursive_calls, body, &recursive_call_ctx);
    vkd3d_free(recursive_call_ctx.backtrace);

    /* Avoid going into an infinite loop when processing call instructions.
     * lower_return() recurses into inferior calls. */
    if (ctx->result)
        return;

    if (hlsl_version_ge(ctx, 4, 0) && hlsl_version_lt(ctx, 5, 0))
    {
        lower_ir(ctx, lower_f16tof32, body);
        lower_ir(ctx, lower_f32tof16, body);
    }

    lower_return(ctx, entry_func, body, false);

    while (hlsl_transform_ir(ctx, lower_calls, body, NULL));

    lower_ir(ctx, lower_matrix_swizzles, body);
    lower_ir(ctx, lower_index_loads, body);

    for (i = 0; i < entry_func->parameters.count; ++i)
    {
        var = entry_func->parameters.vars[i];

        if (hlsl_type_is_resource(var->data_type))
        {
            prepend_uniform_copy(ctx, body, var);
        }
        else if ((var->storage_modifiers & HLSL_STORAGE_UNIFORM))
        {
            if (ctx->profile->type == VKD3D_SHADER_TYPE_HULL && entry_func == ctx->patch_constant_func)
                hlsl_error(ctx, &var->loc, VKD3D_SHADER_ERROR_HLSL_INVALID_MODIFIER,
                        "Patch constant function parameter \"%s\" cannot be uniform.", var->name);
            else
                prepend_uniform_copy(ctx, body, var);
        }
        else
        {
            if (hlsl_get_multiarray_element_type(var->data_type)->class != HLSL_CLASS_STRUCT
                    && !var->semantic.name)
            {
                hlsl_error(ctx, &var->loc, VKD3D_SHADER_ERROR_HLSL_MISSING_SEMANTIC,
                        "Parameter \"%s\" is missing a semantic.", var->name);
                var->semantic.reported_missing = true;
            }

            if (var->storage_modifiers & HLSL_STORAGE_IN)
                prepend_input_var_copy(ctx, entry_func, var);
            if (var->storage_modifiers & HLSL_STORAGE_OUT)
                append_output_var_copy(ctx, entry_func, var);
        }
    }
    if (entry_func->return_var)
    {
        if (entry_func->return_var->data_type->class != HLSL_CLASS_STRUCT && !entry_func->return_var->semantic.name)
            hlsl_error(ctx, &entry_func->loc, VKD3D_SHADER_ERROR_HLSL_MISSING_SEMANTIC,
                    "Entry point \"%s\" is missing a return value semantic.", entry_func->func->name);

        append_output_var_copy(ctx, entry_func, entry_func->return_var);
    }

    if (profile->major_version >= 4)
    {
        hlsl_transform_ir(ctx, lower_discard_neg, body, NULL);
    }
    else
    {
        hlsl_transform_ir(ctx, lower_discard_nz, body, NULL);
    }

    transform_unroll_loops(ctx, body);
    hlsl_run_const_passes(ctx, body);

    remove_unreachable_code(ctx, body);
    hlsl_transform_ir(ctx, normalize_switch_cases, body, NULL);

    lower_ir(ctx, lower_nonconstant_vector_derefs, body);
    lower_ir(ctx, lower_casts_to_bool, body);
    lower_ir(ctx, lower_int_dot, body);

    hlsl_transform_ir(ctx, validate_dereferences, body, NULL);
    hlsl_transform_ir(ctx, track_object_components_sampler_dim, body, NULL);
    if (profile->major_version >= 4)
        hlsl_transform_ir(ctx, lower_combined_samples, body, NULL);

    do
        compute_liveness(ctx, entry_func);
    while (hlsl_transform_ir(ctx, dce, body, NULL));

    hlsl_transform_ir(ctx, track_components_usage, body, NULL);
    sort_synthetic_separated_samplers_first(ctx);

    if (profile->major_version < 4)
    {
        while (lower_ir(ctx, lower_nonconstant_array_loads, body));

        lower_ir(ctx, lower_ternary, body);

        lower_ir(ctx, lower_nonfloat_exprs, body);
        /* Constants casted to float must be folded, and new casts to bool also need to be lowered. */
        hlsl_transform_ir(ctx, hlsl_fold_constant_exprs, body, NULL);
        lower_ir(ctx, lower_casts_to_bool, body);

        lower_ir(ctx, lower_casts_to_int, body);
        lower_ir(ctx, lower_division, body);
        lower_ir(ctx, lower_sqrt, body);
        lower_ir(ctx, lower_dot, body);
        lower_ir(ctx, lower_round, body);
        lower_ir(ctx, lower_ceil, body);
        lower_ir(ctx, lower_floor, body);
        lower_ir(ctx, lower_trig, body);
        lower_ir(ctx, lower_comparison_operators, body);
        lower_ir(ctx, lower_logic_not, body);
        if (ctx->profile->type == VKD3D_SHADER_TYPE_PIXEL)
            lower_ir(ctx, lower_slt, body);
        else
            lower_ir(ctx, lower_cmp, body);
    }

    if (profile->major_version < 2)
    {
        lower_ir(ctx, lower_abs, body);
    }

    lower_ir(ctx, validate_nonconstant_vector_store_derefs, body);

    do
        compute_liveness(ctx, entry_func);
    while (hlsl_transform_ir(ctx, dce, body, NULL));

    /* TODO: move forward, remove when no longer needed */
    transform_derefs(ctx, replace_deref_path_with_offset, body);
    while (hlsl_transform_ir(ctx, hlsl_fold_constant_exprs, body, NULL));
    transform_derefs(ctx, clean_constant_deref_offset_srcs, body);

    do
        compute_liveness(ctx, entry_func);
    while (hlsl_transform_ir(ctx, dce, body, NULL));

    compute_liveness(ctx, entry_func);
    mark_vars_usage(ctx);

    calculate_resource_register_counts(ctx);

    allocate_register_reservations(ctx, &ctx->extern_vars);
    allocate_register_reservations(ctx, &entry_func->extern_vars);
    allocate_semantic_registers(ctx, entry_func);
}

int hlsl_emit_bytecode(struct hlsl_ctx *ctx, struct hlsl_ir_function_decl *entry_func,
        enum vkd3d_shader_target_type target_type, struct vkd3d_shader_code *out)
{
    const struct hlsl_profile_info *profile = ctx->profile;
    struct hlsl_block global_uniform_block;
    struct hlsl_ir_var *var;

    parse_entry_function_attributes(ctx, entry_func);
    if (ctx->result)
        return ctx->result;

    if (profile->type == VKD3D_SHADER_TYPE_HULL)
        validate_hull_shader_attributes(ctx, entry_func);
    else if (profile->type == VKD3D_SHADER_TYPE_COMPUTE && !ctx->found_numthreads)
        hlsl_error(ctx, &entry_func->loc, VKD3D_SHADER_ERROR_HLSL_MISSING_ATTRIBUTE,
                "Entry point \"%s\" is missing a [numthreads] attribute.", entry_func->func->name);
    else if (profile->type == VKD3D_SHADER_TYPE_DOMAIN && ctx->domain == VKD3D_TESSELLATOR_DOMAIN_INVALID)
        hlsl_error(ctx, &entry_func->loc, VKD3D_SHADER_ERROR_HLSL_MISSING_ATTRIBUTE,
                "Entry point \"%s\" is missing a [domain] attribute.", entry_func->func->name);

    hlsl_block_init(&global_uniform_block);

    LIST_FOR_EACH_ENTRY(var, &ctx->globals->vars, struct hlsl_ir_var, scope_entry)
    {
        if (var->storage_modifiers & HLSL_STORAGE_UNIFORM)
            prepend_uniform_copy(ctx, &global_uniform_block, var);
    }

    process_entry_function(ctx, &global_uniform_block, entry_func);
    if (ctx->result)
        return ctx->result;

    if (profile->type == VKD3D_SHADER_TYPE_HULL)
    {
        process_entry_function(ctx, &global_uniform_block, ctx->patch_constant_func);
        if (ctx->result)
            return ctx->result;
    }

    hlsl_block_cleanup(&global_uniform_block);

    if (profile->major_version < 4)
    {
        mark_indexable_vars(ctx, entry_func);
        allocate_temp_registers(ctx, entry_func);
        allocate_const_registers(ctx, entry_func);
    }
    else
    {
        allocate_buffers(ctx);
        allocate_objects(ctx, entry_func, HLSL_REGSET_TEXTURES);
        allocate_objects(ctx, entry_func, HLSL_REGSET_UAVS);
    }
    allocate_objects(ctx, entry_func, HLSL_REGSET_SAMPLERS);

    if (TRACE_ON())
        rb_for_each_entry(&ctx->functions, dump_function, ctx);

    if (ctx->result)
        return ctx->result;

    switch (target_type)
    {
        case VKD3D_SHADER_TARGET_D3D_BYTECODE:
        {
            uint32_t config_flags = vkd3d_shader_init_config_flags();
            struct vkd3d_shader_code ctab = {0};
            struct vsir_program program;
            int result;

            sm1_generate_vsir(ctx, entry_func, config_flags, &program, &ctab);
            if (ctx->result)
            {
                vsir_program_cleanup(&program);
                vkd3d_shader_free_shader_code(&ctab);
                return ctx->result;
            }

            result = d3dbc_compile(&program, config_flags, NULL, &ctab, out, ctx->message_context);
            vsir_program_cleanup(&program);
            vkd3d_shader_free_shader_code(&ctab);
            return result;
        }

        case VKD3D_SHADER_TARGET_DXBC_TPF:
        {
            uint32_t config_flags = vkd3d_shader_init_config_flags();
            struct vsir_program program;
            int result;

            sm4_generate_vsir(ctx, entry_func, config_flags, &program);
            if (ctx->result)
            {
                vsir_program_cleanup(&program);
                return ctx->result;
            }

            result = tpf_compile(&program, config_flags, out, ctx->message_context, ctx, entry_func);
            vsir_program_cleanup(&program);
            return result;
        }

        default:
            ERR("Unsupported shader target type %#x.\n", target_type);
            return VKD3D_ERROR_INVALID_ARGUMENT;
    }
}

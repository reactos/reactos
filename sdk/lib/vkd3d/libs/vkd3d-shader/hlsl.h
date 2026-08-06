/*
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

#ifndef __VKD3D_SHADER_HLSL_H
#define __VKD3D_SHADER_HLSL_H

#include "vkd3d_shader_private.h"
#include "wine/rbtree.h"
#include "d3dcommon.h"
#include "d3dx9shader.h"

/* The general IR structure is inspired by Mesa GLSL hir, even though the code
 * ends up being quite different in practice. Anyway, here comes the relevant
 * licensing information.
 *
 * Copyright © 2010 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#define HLSL_SWIZZLE_X (0u)
#define HLSL_SWIZZLE_Y (1u)
#define HLSL_SWIZZLE_Z (2u)
#define HLSL_SWIZZLE_W (3u)

#define HLSL_SWIZZLE(x, y, z, w) \
        (((HLSL_SWIZZLE_ ## x) << 0) \
        | ((HLSL_SWIZZLE_ ## y) << 2) \
        | ((HLSL_SWIZZLE_ ## z) << 4) \
        | ((HLSL_SWIZZLE_ ## w) << 6))

#define HLSL_SWIZZLE_MASK (0x3u)
#define HLSL_SWIZZLE_SHIFT(idx) (2u * (idx))

static inline unsigned int hlsl_swizzle_get_component(uint32_t swizzle, unsigned int idx)
{
    return (swizzle >> HLSL_SWIZZLE_SHIFT(idx)) & HLSL_SWIZZLE_MASK;
}

static inline uint32_t vsir_swizzle_from_hlsl(uint32_t swizzle)
{
    return vkd3d_shader_create_swizzle(hlsl_swizzle_get_component(swizzle, 0),
            hlsl_swizzle_get_component(swizzle, 1),
            hlsl_swizzle_get_component(swizzle, 2),
            hlsl_swizzle_get_component(swizzle, 3));
}

enum hlsl_type_class
{
    HLSL_CLASS_SCALAR,
    HLSL_CLASS_VECTOR,
    HLSL_CLASS_MATRIX,
    HLSL_CLASS_LAST_NUMERIC = HLSL_CLASS_MATRIX,
    HLSL_CLASS_STRUCT,
    HLSL_CLASS_ARRAY,
    HLSL_CLASS_DEPTH_STENCIL_STATE,
    HLSL_CLASS_DEPTH_STENCIL_VIEW,
    HLSL_CLASS_EFFECT_GROUP,
    HLSL_CLASS_PASS,
    HLSL_CLASS_PIXEL_SHADER,
    HLSL_CLASS_RASTERIZER_STATE,
    HLSL_CLASS_RENDER_TARGET_VIEW,
    HLSL_CLASS_SAMPLER,
    HLSL_CLASS_STRING,
    HLSL_CLASS_TECHNIQUE,
    HLSL_CLASS_TEXTURE,
    HLSL_CLASS_UAV,
    HLSL_CLASS_VERTEX_SHADER,
    HLSL_CLASS_COMPUTE_SHADER,
    HLSL_CLASS_DOMAIN_SHADER,
    HLSL_CLASS_HULL_SHADER,
    HLSL_CLASS_GEOMETRY_SHADER,
    HLSL_CLASS_CONSTANT_BUFFER,
    HLSL_CLASS_BLEND_STATE,
    HLSL_CLASS_VOID,
    HLSL_CLASS_NULL,
    HLSL_CLASS_ERROR,
};

enum hlsl_base_type
{
    HLSL_TYPE_FLOAT,
    HLSL_TYPE_HALF,
    HLSL_TYPE_DOUBLE,
    HLSL_TYPE_INT,
    HLSL_TYPE_UINT,
    HLSL_TYPE_BOOL,
    HLSL_TYPE_LAST_SCALAR = HLSL_TYPE_BOOL,
};

enum hlsl_sampler_dim
{
    HLSL_SAMPLER_DIM_GENERIC = 0,
    HLSL_SAMPLER_DIM_COMPARISON,
    HLSL_SAMPLER_DIM_1D,
    HLSL_SAMPLER_DIM_2D,
    HLSL_SAMPLER_DIM_3D,
    HLSL_SAMPLER_DIM_CUBE,
    HLSL_SAMPLER_DIM_LAST_SAMPLER = HLSL_SAMPLER_DIM_CUBE,
    HLSL_SAMPLER_DIM_1DARRAY,
    HLSL_SAMPLER_DIM_2DARRAY,
    HLSL_SAMPLER_DIM_2DMS,
    HLSL_SAMPLER_DIM_2DMSARRAY,
    HLSL_SAMPLER_DIM_CUBEARRAY,
    HLSL_SAMPLER_DIM_BUFFER,
    HLSL_SAMPLER_DIM_STRUCTURED_BUFFER,
    HLSL_SAMPLER_DIM_RAW_BUFFER,
    HLSL_SAMPLER_DIM_MAX = HLSL_SAMPLER_DIM_RAW_BUFFER,
    /* NOTE: Remember to update object_methods[] in hlsl.y if this enum is modified. */
};

enum hlsl_regset
{
    HLSL_REGSET_SAMPLERS,
    HLSL_REGSET_TEXTURES,
    HLSL_REGSET_UAVS,
    HLSL_REGSET_LAST_OBJECT = HLSL_REGSET_UAVS,
    HLSL_REGSET_NUMERIC,
    HLSL_REGSET_LAST = HLSL_REGSET_NUMERIC,
};

/* An HLSL source-level data type, including anonymous structs and typedefs. */
struct hlsl_type
{
    /* Item entry in hlsl_ctx->types. */
    struct list entry;
    /* Item entry in hlsl_scope->types. hlsl_type->name is used as key (if not NULL). */
    struct rb_entry scope_entry;

    enum hlsl_type_class class;

    /* If class is HLSL_CLASS_SAMPLER, then sampler_dim is <= HLSL_SAMPLER_DIM_LAST_SAMPLER.
     * If class is HLSL_CLASS_TEXTURE, then sampler_dim can be any value of the enum except
     *   HLSL_SAMPLER_DIM_GENERIC and HLSL_SAMPLER_DIM_COMPARISON.
     * If class is HLSL_CLASS_UAV, then sampler_dim must be one of HLSL_SAMPLER_DIM_1D,
     *   HLSL_SAMPLER_DIM_2D, HLSL_SAMPLER_DIM_3D, HLSL_SAMPLER_DIM_1DARRAY, HLSL_SAMPLER_DIM_2DARRAY,
     *   HLSL_SAMPLER_DIM_BUFFER, or HLSL_SAMPLER_DIM_STRUCTURED_BUFFER.
     * Otherwise, sampler_dim is not used */
    enum hlsl_sampler_dim sampler_dim;
    /* Name, in case the type is a named struct or a typedef. */
    const char *name;
    /* Bitfield for storing type modifiers, subset of HLSL_TYPE_MODIFIERS_MASK.
     * Modifiers that don't fall inside this mask are to be stored in the variable in
     *   hlsl_ir_var.modifiers, or in the struct field in hlsl_ir_field.modifiers. */
    uint32_t modifiers;
    /* Size of the type values on each dimension. For non-numeric types, they are set for the
     *   convenience of the sm1/sm4 backends.
     * If type is HLSL_CLASS_SCALAR, then both dimx = 1 and dimy = 1.
     * If type is HLSL_CLASS_VECTOR, then dimx is the size of the vector, and dimy = 1.
     * If type is HLSL_CLASS_MATRIX, then dimx is the number of columns, and dimy the number of rows.
     * If type is HLSL_CLASS_ARRAY, then dimx and dimy have the same value as in the type of the array elements.
     * If type is HLSL_CLASS_STRUCT, then dimx is the sum of (dimx * dimy) of every component, and dimy = 1.
     */
    unsigned int dimx;
    unsigned int dimy;
    /* Sample count for HLSL_SAMPLER_DIM_2DMS or HLSL_SAMPLER_DIM_2DMSARRAY. */
    unsigned int sample_count;

    union
    {
        /* Additional information if type is numeric. */
        struct
        {
            enum hlsl_base_type type;
        } numeric;
        /* Additional information if type is HLSL_CLASS_STRUCT. */
        struct
        {
            struct hlsl_struct_field *fields;
            size_t field_count;
        } record;
        /* Additional information if type is HLSL_CLASS_ARRAY. */
        struct
        {
            struct hlsl_type *type;
            /* Array length, or HLSL_ARRAY_ELEMENTS_COUNT_IMPLICIT if it is not known yet at parse time. */
            unsigned int elements_count;
        } array;
        /* Additional information if the class is HLSL_CLASS_TEXTURE or
         * HLSL_CLASS_UAV. */
        struct
        {
            /* Format of the data contained within the type. */
            struct hlsl_type *format;
            /* The type is a rasteriser-ordered view. */
            bool rasteriser_ordered;
        } resource;
        /* Additional field to distinguish object types. Currently used only for technique types. */
        unsigned int version;
    } e;

    /* Number of numeric register components used by one value of this type, for each regset.
     * For HLSL_REGSET_NUMERIC, 4 components make 1 register, while for other regsets 1 component makes
     *   1 register.
     * If type is HLSL_CLASS_STRUCT or HLSL_CLASS_ARRAY, the reg_size of their elements and padding
     *   (which varies according to the backend) is also included. */
    unsigned int reg_size[HLSL_REGSET_LAST + 1];
    /* Offset where the type's description starts in the output bytecode, in bytes. */
    size_t bytecode_offset;

    uint32_t is_minimum_precision : 1;
};

/* In HLSL, a semantic is a string linked to a variable (or a field) to be recognized across
 *   different shader stages in the graphics pipeline. */
struct hlsl_semantic
{
    const char *name;
    uint32_t index;

    /* Name exactly as it appears in the sources. */
    const char *raw_name;
    /* If the variable or field that stores this hlsl_semantic has already reported that it is missing. */
    bool reported_missing;
    /* In case the variable or field that stores this semantic has already reported to use a
     *   duplicated output semantic, this value stores the last reported index + 1. Otherwise it is 0. */
    uint32_t reported_duplicated_output_next_index;
    /* In case the variable or field that stores this semantic has already reported to use a
     *   duplicated input semantic with incompatible values, this value stores the last reported
     *   index + 1. Otherwise it is 0. */
    uint32_t reported_duplicated_input_incompatible_next_index;
};

/* A field within a struct type declaration, used in hlsl_type.e.fields. */
struct hlsl_struct_field
{
    struct vkd3d_shader_location loc;
    struct hlsl_type *type;
    const char *name;
    struct hlsl_semantic semantic;

    /* Bitfield for storing modifiers that are not in HLSL_TYPE_MODIFIERS_MASK (these are stored in
     *   type->modifiers instead) and that also are specific to the field and not the whole variable.
     *   In particular, interpolation modifiers. */
    uint32_t storage_modifiers;
    /* Offset of the field within the type it belongs to, in register components, for each regset. */
    unsigned int reg_offset[HLSL_REGSET_LAST + 1];

    /* Offset where the field name starts in the output bytecode, in bytes. */
    size_t name_bytecode_offset;
};

/* Information of the register(s) allocated for an instruction node or variable.
 * These values are initialized at the end of hlsl_emit_bytecode(), after the compilation passes,
 *   just before writing the bytecode.
 * The type of register (register class) is implied from its use, so it is not stored in this
 *   struct. */
struct hlsl_reg
{
    /* Register number of the first register allocated. */
    uint32_t id;
    /* For descriptors (buffer, texture, sampler, UAV) this is the base binding
     * index of the descriptor.
     * For 5.1 and above descriptors have space and may be arrayed, in which
     * case the array shares a single register ID but has a range of register
     * indices, and "id" and "index" are as a rule not equal.
     * For versions below 5.1, the register number for descriptors is the same
     * as its external binding index, so only "index" is used, and "id" is
     * ignored.
     * For numeric registers "index" is not used. */
    uint32_t index;
    /* Register space of a descriptor. Not used for numeric registers. */
    uint32_t space;
    /* Number of registers to be allocated.
     * Unlike the variable's type's regsize, it is not expressed in register components, but rather
     *  in whole registers, and may depend on which components are used within the shader. */
    uint32_t allocation_size;
    /* For numeric registers, a writemask can be provided to indicate the reservation of only some
     *   of the 4 components. */
    unsigned int writemask;
    /* Whether the register has been allocated. */
    bool allocated;
};

/* Types of instruction nodes for the IR.
 * Each type of instruction node is associated to a struct with the same name in lower case.
 *   e.g. for HLSL_IR_CONSTANT there exists struct hlsl_ir_constant.
 * Each one of these structs start with a struct hlsl_ir_node field, so pointers to values of these
 *   types can be casted seamlessly to (struct hlsl_ir_node *) and vice-versa. */
enum hlsl_ir_node_type
{
    HLSL_IR_CALL,
    HLSL_IR_CONSTANT,
    HLSL_IR_EXPR,
    HLSL_IR_IF,
    HLSL_IR_INDEX,
    HLSL_IR_LOAD,
    HLSL_IR_LOOP,
    HLSL_IR_JUMP,
    HLSL_IR_RESOURCE_LOAD,
    HLSL_IR_RESOURCE_STORE,
    HLSL_IR_STRING_CONSTANT,
    HLSL_IR_STORE,
    HLSL_IR_SWIZZLE,
    HLSL_IR_SWITCH,

    HLSL_IR_COMPILE,
    HLSL_IR_SAMPLER_STATE,
    HLSL_IR_STATEBLOCK_CONSTANT,

    HLSL_IR_VSIR_INSTRUCTION_REF,
};

/* Common data for every type of IR instruction node. */
struct hlsl_ir_node
{
    /* Item entry for storing the instruction in a list of instructions. */
    struct list entry;

    /* Type of node, which means that a pointer to this struct hlsl_ir_node can be casted to a
     *   pointer to the struct with the same name. */
    enum hlsl_ir_node_type type;
    /* HLSL data type of the node, when used by other nodes as a source (through an hlsl_src).
     * HLSL_IR_CONSTANT, HLSL_IR_EXPR, HLSL_IR_LOAD, HLSL_IR_RESOURCE_LOAD, and HLSL_IR_SWIZZLE
     *   have a data type and can be used through an hlsl_src; other types of node don't. */
    struct hlsl_type *data_type;

    /* List containing all the struct hlsl_src·s that point to this node; linked by the
     *   hlsl_src.entry fields. */
    struct list uses;

    struct vkd3d_shader_location loc;

    /* Liveness ranges. "index" is the index of this instruction. Since this is
     * essentially an SSA value, the earliest live point is the index. This is
     * true even for loops, since currently we can't have a reference to a
     * value generated in an earlier iteration of the loop. */
    unsigned int index, last_read;
    /* Temp. register allocated to store the result of this instruction (if any). */
    struct hlsl_reg reg;
};

struct hlsl_block
{
    /* List containing instruction nodes; linked by the hlsl_ir_node.entry fields. */
    struct list instrs;
    /* Instruction representing the "value" of this block, if applicable.
     * This may point to an instruction outside of this block! */
    struct hlsl_ir_node *value;
};

/* A reference to an instruction node (struct hlsl_ir_node), usable as a field in other structs.
 *   struct hlsl_src is more powerful than a mere pointer to an hlsl_ir_node because it also
 *   contains a linked list item entry, which is used by the referenced instruction node to keep
 *   track of all the hlsl_src·s that reference it.
 * This allows replacing any hlsl_ir_node with any other in all the places it is used, or checking
 *   that a node has no uses before it is removed. */
struct hlsl_src
{
    struct hlsl_ir_node *node;
    /* Item entry for node->uses. */
    struct list entry;
};

struct hlsl_attribute
{
    const char *name;
    struct hlsl_block instrs;
    struct vkd3d_shader_location loc;
    unsigned int args_count;
    struct hlsl_src args[];
};

#define HLSL_STORAGE_EXTERN              0x00000001
#define HLSL_STORAGE_NOINTERPOLATION     0x00000002
#define HLSL_MODIFIER_PRECISE            0x00000004
#define HLSL_STORAGE_SHARED              0x00000008
#define HLSL_STORAGE_GROUPSHARED         0x00000010
#define HLSL_STORAGE_STATIC              0x00000020
#define HLSL_STORAGE_UNIFORM             0x00000040
#define HLSL_MODIFIER_VOLATILE           0x00000080
#define HLSL_MODIFIER_CONST              0x00000100
#define HLSL_MODIFIER_ROW_MAJOR          0x00000200
#define HLSL_MODIFIER_COLUMN_MAJOR       0x00000400
#define HLSL_STORAGE_IN                  0x00000800
#define HLSL_STORAGE_OUT                 0x00001000
#define HLSL_MODIFIER_INLINE             0x00002000
#define HLSL_STORAGE_CENTROID            0x00004000
#define HLSL_STORAGE_NOPERSPECTIVE       0x00008000
#define HLSL_STORAGE_LINEAR              0x00010000
#define HLSL_MODIFIER_SINGLE             0x00020000
#define HLSL_MODIFIER_EXPORT             0x00040000
#define HLSL_STORAGE_ANNOTATION          0x00080000
#define HLSL_MODIFIER_UNORM              0x00100000
#define HLSL_MODIFIER_SNORM              0x00200000

#define HLSL_TYPE_MODIFIERS_MASK     (HLSL_MODIFIER_PRECISE | HLSL_MODIFIER_VOLATILE | \
                                      HLSL_MODIFIER_CONST | HLSL_MODIFIER_ROW_MAJOR | \
                                      HLSL_MODIFIER_COLUMN_MAJOR | HLSL_MODIFIER_UNORM | HLSL_MODIFIER_SNORM)

#define HLSL_INTERPOLATION_MODIFIERS_MASK (HLSL_STORAGE_NOINTERPOLATION | HLSL_STORAGE_CENTROID | \
                                           HLSL_STORAGE_NOPERSPECTIVE | HLSL_STORAGE_LINEAR)

#define HLSL_MODIFIERS_MAJORITY_MASK (HLSL_MODIFIER_ROW_MAJOR | HLSL_MODIFIER_COLUMN_MAJOR)

#define HLSL_ARRAY_ELEMENTS_COUNT_IMPLICIT 0

/* Reservation of a register and/or an offset for objects inside constant buffers, to be used as a
 *   starting point of their allocation. They are available through the register(·) and the
 *   packoffset(·) syntaxes, respectively.
 * The constant buffer offset is measured register components. */
struct hlsl_reg_reservation
{
    char reg_type;
    unsigned int reg_space, reg_index;

    char offset_type;
    unsigned int offset_index;
};

union hlsl_constant_value_component
{
    uint32_t u;
    int32_t i;
    float f;
    double d;
};

struct hlsl_ir_var
{
    struct hlsl_type *data_type;
    struct vkd3d_shader_location loc;
    const char *name;
    struct hlsl_semantic semantic;
    /* Buffer where the variable's value is stored, in case it is uniform. */
    struct hlsl_buffer *buffer;
    /* Bitfield for storage modifiers (type modifiers are stored in data_type->modifiers). */
    uint32_t storage_modifiers;
    /* Optional reservations of registers and/or offsets for variables within constant buffers. */
    struct hlsl_reg_reservation reg_reservation;

    /* Item entry in hlsl_scope.vars. Specifically hlsl_ctx.globals.vars if the variable is global. */
    struct list scope_entry;
    /* Item entry in hlsl_ctx.extern_vars, if the variable is extern. */
    struct list extern_entry;
    /* Scope that variable itself defines, used to provide a container for techniques and passes. */
    struct hlsl_scope *scope;
    /* Scope that contains annotations for this variable. */
    struct hlsl_scope *annotations;

    /* Array of default values the variable was initialized with, one for each component.
     * Only for variables that need it, such as uniforms and variables inside constant buffers.
     * This pointer is NULL for others. */
    struct hlsl_default_value
    {
        /* Default value, in case the component is a string, otherwise it is NULL. */
        const char *string;
        /* Default value, in case the component is a numeric value. */
        union hlsl_constant_value_component number;
    } *default_values;

    /* A dynamic array containing the state block on the variable's declaration, if any.
     * An array variable may contain multiple state blocks.
     * A technique pass will always contain one.
     * These are only really used for effect profiles. */
    struct hlsl_state_block **state_blocks;
    unsigned int state_block_count;
    size_t state_block_capacity;

    /* Indexes of the IR instructions where the variable is first written and last read (liveness
     *   range). The IR instructions are numerated starting from 2, because 0 means unused, and 1
     *   means function entry. */
    unsigned int first_write, last_read;
    /* Whether the variable is read in any entry function. */
    bool is_read;
    /* Offset where the variable's value is stored within its buffer in numeric register components.
     * This in case the variable is uniform. */
    unsigned int buffer_offset;
    /* Register to which the variable is allocated during its lifetime, for each register set.
     * In case that the variable spans multiple registers in one regset, this is set to the
     *   start of the register range.
     *   Builtin semantics don't use the field.
     *   In SM4, uniforms don't use the field because they are located using the buffer's hlsl_reg
     *     and the buffer_offset instead. */
    struct hlsl_reg regs[HLSL_REGSET_LAST + 1];

    struct
    {
        bool used;
        enum hlsl_sampler_dim sampler_dim;
        struct vkd3d_shader_location first_sampler_dim_loc;
    } *objects_usage[HLSL_REGSET_LAST_OBJECT + 1];
    /* Minimum number of binds required to include all components actually used in the shader.
     * It may be less than the allocation size, e.g. for texture arrays.
     * The bind_count for HLSL_REGSET_NUMERIC is only used in uniforms for now. */
    unsigned int bind_count[HLSL_REGSET_LAST + 1];

    /* Whether the shader performs dereferences with non-constant offsets in the variable. */
    bool indexable;
    /* Whether this is a semantic variable that was split from an array, or is the first
     * element of a struct, and thus needs to be aligned when packed in the signature. */
    bool force_align;

    uint32_t is_input_semantic : 1;
    uint32_t is_output_semantic : 1;
    uint32_t is_uniform : 1;
    uint32_t is_param : 1;
    uint32_t is_separated_resource : 1;
    uint32_t is_synthetic : 1;
    uint32_t has_explicit_bind_point : 1;
};

/* This struct is used to represent assignments in state block entries:
 *     name = {args[0], args[1], ...};
 *       - or -
 *     name = args[0]
 *       - or -
 *     name[lhs_index] = args[0]
 *       - or -
 *     name[lhs_index] = {args[0], args[1], ...};
 *
 * This struct also represents function call syntax:
 *     name(args[0], args[1], ...)
 */
struct hlsl_state_block_entry
{
    /* Whether this entry is a function call. */
    bool is_function_call;

    /* For assignments, the name in the lhs.
     * For functions, the name of the function. */
    char *name;
    /* Resolved format-specific property identifier. */
    unsigned int name_id;

    /* For assignments, whether the lhs of an assignment is indexed and, in
     * that case, its index. */
    bool lhs_has_index;
    unsigned int lhs_index;

    /* Instructions present in the rhs or the function arguments. */
    struct hlsl_block *instrs;

    /* For assignments, arguments of the rhs initializer.
     * For function calls, the arguments themselves. */
    struct hlsl_src *args;
    unsigned int args_count;
};

struct hlsl_state_block
{
    struct hlsl_state_block_entry **entries;
    size_t count, capacity;
};

/* Sized array of variables representing a function's parameters. */
struct hlsl_func_parameters
{
    struct hlsl_ir_var **vars;
    size_t count, capacity;
};

struct hlsl_ir_function
{
    /* Item entry in hlsl_ctx.functions */
    struct rb_entry entry;

    const char *name;
    /* Tree containing function definitions, stored as hlsl_ir_function_decl structures, which would
     *   be more than one in case of function overloading. */
    struct list overloads;
};

struct hlsl_ir_function_decl
{
    struct hlsl_type *return_type;
    /* Synthetic variable used to store the return value of the function. */
    struct hlsl_ir_var *return_var;

    struct vkd3d_shader_location loc;
    /* Item entry in hlsl_ir_function.overloads. */
    struct list entry;

    /* Function to which this declaration corresponds. */
    struct hlsl_ir_function *func;

    struct hlsl_func_parameters parameters;

    struct hlsl_block body;
    bool has_body;
    /* Array of attributes (like numthreads) specified just before the function declaration.
     * Not to be confused with the function parameters! */
    unsigned int attr_count;
    const struct hlsl_attribute *const *attrs;

    bool early_depth_test;

    /* Synthetic boolean variable marking whether a return statement has been
     * executed. Needed to deal with return statements in non-uniform control
     * flow, since some backends can't handle them. */
    struct hlsl_ir_var *early_return_var;

    /* List of all the extern semantic variables; linked by the
     * hlsl_ir_var.extern_entry fields. This exists as a convenience because
     * it is often necessary to iterate all extern variables and these can be
     * declared in as function parameters, or as the function return value. */
    struct list extern_vars;
};

struct hlsl_ir_call
{
    struct hlsl_ir_node node;
    struct hlsl_ir_function_decl *decl;
};

struct hlsl_ir_if
{
    struct hlsl_ir_node node;
    struct hlsl_src condition;
    struct hlsl_block then_block;
    struct hlsl_block else_block;
};

enum hlsl_ir_loop_unroll_type
{
    HLSL_IR_LOOP_UNROLL,
    HLSL_IR_LOOP_FORCE_UNROLL,
    HLSL_IR_LOOP_FORCE_LOOP
};

struct hlsl_ir_loop
{
    struct hlsl_ir_node node;
    /* loop condition is stored in the body (as "if (!condition) break;") */
    struct hlsl_block body;
    unsigned int next_index; /* liveness index of the end of the loop */
    unsigned int unroll_limit;
    enum hlsl_ir_loop_unroll_type unroll_type;
};

struct hlsl_ir_switch_case
{
    unsigned int value;
    bool is_default;
    struct hlsl_block body;
    struct list entry;
    struct vkd3d_shader_location loc;
};

struct hlsl_ir_switch
{
    struct hlsl_ir_node node;
    struct hlsl_src selector;
    struct list cases;
};

enum hlsl_ir_expr_op
{
    HLSL_OP0_ERROR,
    HLSL_OP0_VOID,
    HLSL_OP0_RASTERIZER_SAMPLE_COUNT,

    HLSL_OP1_ABS,
    HLSL_OP1_BIT_NOT,
    HLSL_OP1_CAST,
    HLSL_OP1_CEIL,
    HLSL_OP1_COS,
    HLSL_OP1_COS_REDUCED,    /* Reduced range [-pi, pi], writes to .x */
    HLSL_OP1_DSX,
    HLSL_OP1_DSX_COARSE,
    HLSL_OP1_DSX_FINE,
    HLSL_OP1_DSY,
    HLSL_OP1_DSY_COARSE,
    HLSL_OP1_DSY_FINE,
    HLSL_OP1_EXP2,
    HLSL_OP1_F16TOF32,
    HLSL_OP1_F32TOF16,
    HLSL_OP1_FLOOR,
    HLSL_OP1_FRACT,
    HLSL_OP1_LOG2,
    HLSL_OP1_LOGIC_NOT,
    HLSL_OP1_NEG,
    HLSL_OP1_NRM,
    HLSL_OP1_RCP,
    HLSL_OP1_REINTERPRET,
    HLSL_OP1_ROUND,
    HLSL_OP1_RSQ,
    HLSL_OP1_SAT,
    HLSL_OP1_SIGN,
    HLSL_OP1_SIN,
    HLSL_OP1_SIN_REDUCED,    /* Reduced range [-pi, pi], writes to .y */
    HLSL_OP1_SQRT,
    HLSL_OP1_TRUNC,

    HLSL_OP2_ADD,
    HLSL_OP2_BIT_AND,
    HLSL_OP2_BIT_OR,
    HLSL_OP2_BIT_XOR,
    HLSL_OP2_CRS,
    HLSL_OP2_DIV,
    HLSL_OP2_DOT,
    HLSL_OP2_EQUAL,
    HLSL_OP2_GEQUAL,
    HLSL_OP2_LESS,
    HLSL_OP2_LOGIC_AND,
    HLSL_OP2_LOGIC_OR,
    HLSL_OP2_LSHIFT,
    HLSL_OP2_MAX,
    HLSL_OP2_MIN,
    HLSL_OP2_MOD,
    HLSL_OP2_MUL,
    HLSL_OP2_NEQUAL,
    HLSL_OP2_RSHIFT,
    /* SLT(a, b) retrieves 1.0 if (a < b), else 0.0. Only used for SM1-SM3 target vertex shaders. */
    HLSL_OP2_SLT,

    /* DP2ADD(a, b, c) computes the scalar product of a.xy and b.xy,
     * then adds c, where c must have dimx=1. */
    HLSL_OP3_DP2ADD,
    /* TERNARY(a, b, c) returns 'b' if 'a' is true and 'c' otherwise. 'a' must always be boolean.
     * CMP(a, b, c) returns 'b' if 'a' >= 0, and 'c' otherwise. It's used only for SM1-SM3 targets. */
    HLSL_OP3_CMP,
    HLSL_OP3_TERNARY,
    HLSL_OP3_MAD,
};

#define HLSL_MAX_OPERANDS 3

struct hlsl_ir_expr
{
    struct hlsl_ir_node node;
    enum hlsl_ir_expr_op op;
    struct hlsl_src operands[HLSL_MAX_OPERANDS];
};

enum hlsl_ir_jump_type
{
    HLSL_IR_JUMP_BREAK,
    HLSL_IR_JUMP_CONTINUE,
    HLSL_IR_JUMP_DISCARD_NEG,
    HLSL_IR_JUMP_DISCARD_NZ,
    HLSL_IR_JUMP_RETURN,
    /* UNRESOLVED_CONTINUE type is used by the parser when 'continue' statement is found,
       it never reaches code generation, and is resolved to CONTINUE type once iteration
       and loop exit logic was properly applied. */
    HLSL_IR_JUMP_UNRESOLVED_CONTINUE,
};

struct hlsl_ir_jump
{
    struct hlsl_ir_node node;
    enum hlsl_ir_jump_type type;
    /* Argument used for HLSL_IR_JUMP_DISCARD_NZ and HLSL_IR_JUMP_DISCARD_NEG. */
    struct hlsl_src condition;
};

struct hlsl_ir_swizzle
{
    struct hlsl_ir_node node;
    struct hlsl_src val;
    uint32_t swizzle;
};

struct hlsl_ir_index
{
    struct hlsl_ir_node node;
    struct hlsl_src val, idx;
};

/* Reference to a variable, or a part of it (e.g. a vector within a matrix within a struct). */
struct hlsl_deref
{
    struct hlsl_ir_var *var;

    /* An array of references to instruction nodes, of data type uint, that are used to reach the
     *   desired part of the variable.
     * If path_len is 0, then this is a reference to the whole variable.
     * The value of each instruction node in the path corresponds to the index of the element/field
     *   that has to be selected on each nesting level to reach this part.
     * The path shall not contain additional values once a type that cannot be subdivided
     *   (a.k.a. "component") is reached. */
    unsigned int path_len;
    struct hlsl_src *path;

    /* Before writing the bytecode, deref paths are lowered into an offset (within the pertaining
     *   regset) from the start of the variable, to the part of the variable that is referenced.
     * This offset is stored using two fields, one for a variable part and other for a constant
     *   part, which are added together:
     *   - rel_offset: An offset given by an instruction node, in whole registers.
     *   - const_offset: A constant number of register components.
     * Since the type information cannot longer be retrieved from the offset alone, the type is
     *   stored in the data_type field, which remains NULL if the deref hasn't been lowered yet. */
    struct hlsl_src rel_offset;
    unsigned int const_offset;
    struct hlsl_type *data_type;
};

/* Whether the path has been lowered to an offset or not. */
static inline bool hlsl_deref_is_lowered(const struct hlsl_deref *deref)
{
    return !!deref->data_type;
}

struct hlsl_ir_load
{
    struct hlsl_ir_node node;
    struct hlsl_deref src;
};

enum hlsl_resource_load_type
{
    HLSL_RESOURCE_LOAD,
    HLSL_RESOURCE_SAMPLE,
    HLSL_RESOURCE_SAMPLE_CMP,
    HLSL_RESOURCE_SAMPLE_CMP_LZ,
    HLSL_RESOURCE_SAMPLE_LOD,
    HLSL_RESOURCE_SAMPLE_LOD_BIAS,
    HLSL_RESOURCE_SAMPLE_GRAD,
    HLSL_RESOURCE_SAMPLE_PROJ,
    HLSL_RESOURCE_GATHER_RED,
    HLSL_RESOURCE_GATHER_GREEN,
    HLSL_RESOURCE_GATHER_BLUE,
    HLSL_RESOURCE_GATHER_ALPHA,
    HLSL_RESOURCE_SAMPLE_INFO,
    HLSL_RESOURCE_RESINFO,
};

struct hlsl_ir_resource_load
{
    struct hlsl_ir_node node;
    enum hlsl_resource_load_type load_type;
    struct hlsl_deref resource, sampler;
    struct hlsl_src coords, lod, ddx, ddy, cmp, sample_index, texel_offset;
    enum hlsl_sampler_dim sampling_dim;
};

struct hlsl_ir_resource_store
{
    struct hlsl_ir_node node;
    struct hlsl_deref resource;
    struct hlsl_src coords, value;
};

struct hlsl_ir_store
{
    struct hlsl_ir_node node;
    struct hlsl_deref lhs;
    struct hlsl_src rhs;
    unsigned char writemask;
};

struct hlsl_ir_constant
{
    struct hlsl_ir_node node;
    struct hlsl_constant_value
    {
        union hlsl_constant_value_component u[4];
    } value;
    /* Constant register of type 'c' where the constant value is stored for SM1. */
    struct hlsl_reg reg;
};

struct hlsl_ir_string_constant
{
    struct hlsl_ir_node node;
    char *string;
};

/* Represents shader compilation call for effects, such as "CompileShader()".
 *
 * Unlike hlsl_ir_call, it is not flattened, thus, it keeps track of its
 * arguments and maintains its own instruction block. */
struct hlsl_ir_compile
{
    struct hlsl_ir_node node;

    enum hlsl_compile_type
    {
        /* A shader compilation through the CompileShader() function or the "compile" syntax. */
        HLSL_COMPILE_TYPE_COMPILE,
        /* A call to ConstructGSWithSO(), which receives a geometry shader and retrieves one as well. */
        HLSL_COMPILE_TYPE_CONSTRUCTGSWITHSO,
    } compile_type;

    /* Special field to store the profile argument for HLSL_COMPILE_TYPE_COMPILE. */
    const struct hlsl_profile_info *profile;

    /* Block containing the instructions required by the arguments of the
     * compilation call. */
    struct hlsl_block instrs;

    /* Arguments to the compilation call. For HLSL_COMPILE_TYPE_COMPILE
     * args[0] is an hlsl_ir_call to the specified function. */
    struct hlsl_src *args;
    unsigned int args_count;
};

/* Represents a state block initialized with the "sampler_state" keyword. */
struct hlsl_ir_sampler_state
{
    struct hlsl_ir_node node;

    struct hlsl_state_block *state_block;
};

/* Stateblock constants are undeclared values found on state blocks or technique passes descriptions,
 *   that do not concern regular pixel, vertex, or compute shaders, except for parsing. */
struct hlsl_ir_stateblock_constant
{
    struct hlsl_ir_node node;
    char *name;
};

/* A vkd3d_shader_instruction that can be inserted in a hlsl_block.
 * Only used for the HLSL IR to vsir translation, might be removed once this translation is complete. */
struct hlsl_ir_vsir_instruction_ref
{
    struct hlsl_ir_node node;

    /* Index to a vkd3d_shader_instruction within a vkd3d_shader_instruction_array in a vsir_program. */
    unsigned int vsir_instr_idx;
};

struct hlsl_scope
{
    /* Item entry for hlsl_ctx.scopes. */
    struct list entry;

    /* List containing the variables declared in this scope; linked by hlsl_ir_var->scope_entry. */
    struct list vars;
    /* Tree map containing the types declared in this scope, using hlsl_tree.name as key.
     * The types are attached through the hlsl_type.scope_entry fields. */
    struct rb_tree types;
    /* Scope containing this scope. This value is NULL for the global scope. */
    struct hlsl_scope *upper;
    /* The scope was created for the loop statement. */
    bool loop;
    /* The scope was created for the switch statement. */
    bool _switch;
    /* The scope contains annotation variables. */
    bool annotations;
};

struct hlsl_profile_info
{
    const char *name;
    enum vkd3d_shader_type type;
    unsigned int major_version;
    unsigned int minor_version;
    unsigned int major_level;
    unsigned int minor_level;
    bool software;
};

struct hlsl_vec4
{
    float f[4];
};

enum hlsl_buffer_type
{
    HLSL_BUFFER_CONSTANT,
    HLSL_BUFFER_TEXTURE,
};

/* In SM4, uniform variables are organized in different buffers. Besides buffers defined in the
 *   source code, there is also the implicit $Globals buffer and the implicit $Params buffer,
 *   to which uniform globals and parameters belong by default. */
struct hlsl_buffer
{
    struct vkd3d_shader_location loc;
    enum hlsl_buffer_type type;
    const char *name;
    uint32_t modifiers;
    /* Register reserved for this buffer, if any.
     * If provided, it should be of type 'b' if type is HLSL_BUFFER_CONSTANT and 't' if type is
     *   HLSL_BUFFER_TEXTURE. */
    struct hlsl_reg_reservation reservation;
    /* Scope that contains annotations for this buffer. */
    struct hlsl_scope *annotations;
    /* Item entry for hlsl_ctx.buffers */
    struct list entry;

    /* The size of the buffer (in register components), and the size of the buffer as determined
     *   by its last variable that's actually used. */
    unsigned size, used_size;
    /* Register of type 'b' on which the buffer is allocated. */
    struct hlsl_reg reg;

    bool manually_packed_elements;
    bool automatically_packed_elements;
};

struct hlsl_ctx
{
    const struct hlsl_profile_info *profile;

    const char **source_files;
    unsigned int source_files_count;
    /* Current location being read in the HLSL source, updated while parsing. */
    struct vkd3d_shader_location location;
    /* Stores the logging messages and logging configuration. */
    struct vkd3d_shader_message_context *message_context;
    /* Cache for temporary string allocations. */
    struct vkd3d_string_buffer_cache string_buffers;
    /* A value from enum vkd3d_result with the current success/failure result of the whole
     *   compilation.
     * It is initialized to VKD3D_OK and set to an error code in case a call to hlsl_fixme() or
     *   hlsl_error() is triggered, or in case of a memory allocation error.
     * The value of this field is checked between compilation stages to stop execution in case of
     *   failure. */
    int result;

    /* Pointer to an opaque data structure managed by FLEX (during lexing), that encapsulates the
     *   current state of the scanner. This pointer is required by all FLEX API functions when the
     *   scanner is declared as reentrant, which is the case. */
    void *scanner;

    /* Pointer to the current scope; changes as the parser reads the code. */
    struct hlsl_scope *cur_scope;
    /* Scope of global variables. */
    struct hlsl_scope *globals;
    /* Dummy scope for variables which should never be looked up by name. */
    struct hlsl_scope *dummy_scope;
    /* List of all the scopes in the program; linked by the hlsl_scope.entry fields. */
    struct list scopes;

    /* List of all the extern variables, excluding semantic variables; linked
     * by the hlsl_ir_var.extern_entry fields. This exists as a convenience
     * because it is often necessary to iterate all extern variables declared
     * in the global scope or as function parameters. */
    struct list extern_vars;

    /* List containing both the built-in HLSL buffers ($Globals and $Params) and the ones declared
     *   in the shader; linked by the hlsl_buffer.entry fields. */
    struct list buffers;
    /* Current buffer (changes as the parser reads the code), $Globals buffer, and $Params buffer,
     *   respectively. */
    struct hlsl_buffer *cur_buffer, *globals_buffer, *params_buffer;
    /* List containing all created hlsl_types, except builtin_types; linked by the hlsl_type.entry
     *   fields. */
    struct list types;
    /* Tree map for the declared functions, using hlsl_ir_function.name as key.
     * The functions are attached through the hlsl_ir_function.entry fields. */
    struct rb_tree functions;
    /* Pointer to the current function; changes as the parser reads the code. */
    const struct hlsl_ir_function_decl *cur_function;

    /* Counter for generating unique internal variable names. */
    unsigned int internal_name_counter;

    /* Default matrix majority for matrix types. Can be set by a pragma within the HLSL source. */
    unsigned int matrix_majority;

    /* Basic data types stored for convenience. */
    struct
    {
        struct hlsl_type *scalar[HLSL_TYPE_LAST_SCALAR + 1];
        struct hlsl_type *vector[HLSL_TYPE_LAST_SCALAR + 1][4];
        /* matrix[HLSL_TYPE_FLOAT][1][3] is a float4x2, i.e. dimx = 2, dimy = 4 */
        struct hlsl_type *matrix[HLSL_TYPE_LAST_SCALAR + 1][4][4];
        struct hlsl_type *sampler[HLSL_SAMPLER_DIM_LAST_SAMPLER + 1];
        struct hlsl_type *string;
        struct hlsl_type *Void;
        struct hlsl_type *null;
        struct hlsl_type *error;
    } builtin_types;

    /* Pre-allocated "error" expression. */
    struct hlsl_ir_node *error_instr;

    /* List of the instruction nodes for initializing static variables. */
    struct hlsl_block static_initializers;

    /* Dynamic array of constant values that appear in the shader, associated to the 'c' registers.
     * Only used for SM1 profiles. */
    struct hlsl_constant_defs
    {
        struct hlsl_constant_register
        {
            uint32_t index;
            struct hlsl_vec4 value;
            struct vkd3d_shader_location loc;
        } *regs;
        size_t count, size;
    } constant_defs;
    /* 'c' registers where the constants expected by SM2 sincos are stored. */
    struct hlsl_reg d3dsincosconst1, d3dsincosconst2;

    /* Number of threads to be executed (on the X, Y, and Z dimensions) in a single thread group in
     *   compute shader profiles. It is set using the numthreads() attribute in the entry point. */
    uint32_t thread_count[3];

    enum vkd3d_tessellator_domain domain;
    unsigned int output_control_point_count;
    enum vkd3d_shader_tessellator_output_primitive output_primitive;
    enum vkd3d_shader_tessellator_partitioning partitioning;
    struct hlsl_ir_function_decl *patch_constant_func;

    /* In some cases we generate opcodes by parsing an HLSL function and then
     * invoking it. If not NULL, this field is the name of the function that we
     * are currently parsing, "mangled" with an internal prefix to avoid
     * polluting the user namespace. */
    const char *internal_func_name;

    /* Whether the parser is inside a state block (effects' metadata) inside a variable declaration. */
    uint32_t in_state_block : 1;
    /* Whether the numthreads() attribute has been provided in the entry-point function. */
    uint32_t found_numthreads : 1;

    bool semantic_compat_mapping;
    bool child_effect;
    bool include_empty_buffers;
    bool warn_implicit_truncation;
    bool double_as_float_alias;
};

static inline bool hlsl_version_ge(const struct hlsl_ctx *ctx, unsigned int major, unsigned int minor)
{
    return ctx->profile->major_version > major || (ctx->profile->major_version == major && ctx->profile->minor_version >= minor);
}

static inline bool hlsl_version_lt(const struct hlsl_ctx *ctx, unsigned int major, unsigned int minor)
{
    return !hlsl_version_ge(ctx, major, minor);
}

struct hlsl_resource_load_params
{
    struct hlsl_type *format;
    enum hlsl_resource_load_type type;
    struct hlsl_ir_node *resource, *sampler;
    struct hlsl_ir_node *coords, *lod, *ddx, *ddy, *cmp, *sample_index, *texel_offset;
    enum hlsl_sampler_dim sampling_dim;
};

static inline struct hlsl_ir_call *hlsl_ir_call(const struct hlsl_ir_node *node)
{
    VKD3D_ASSERT(node->type == HLSL_IR_CALL);
    return CONTAINING_RECORD(node, struct hlsl_ir_call, node);
}

static inline struct hlsl_ir_constant *hlsl_ir_constant(const struct hlsl_ir_node *node)
{
    VKD3D_ASSERT(node->type == HLSL_IR_CONSTANT);
    return CONTAINING_RECORD(node, struct hlsl_ir_constant, node);
}

static inline struct hlsl_ir_string_constant *hlsl_ir_string_constant(const struct hlsl_ir_node *node)
{
    VKD3D_ASSERT(node->type == HLSL_IR_STRING_CONSTANT);
    return CONTAINING_RECORD(node, struct hlsl_ir_string_constant, node);
}

static inline struct hlsl_ir_expr *hlsl_ir_expr(const struct hlsl_ir_node *node)
{
    VKD3D_ASSERT(node->type == HLSL_IR_EXPR);
    return CONTAINING_RECORD(node, struct hlsl_ir_expr, node);
}

static inline struct hlsl_ir_if *hlsl_ir_if(const struct hlsl_ir_node *node)
{
    VKD3D_ASSERT(node->type == HLSL_IR_IF);
    return CONTAINING_RECORD(node, struct hlsl_ir_if, node);
}

static inline struct hlsl_ir_jump *hlsl_ir_jump(const struct hlsl_ir_node *node)
{
    VKD3D_ASSERT(node->type == HLSL_IR_JUMP);
    return CONTAINING_RECORD(node, struct hlsl_ir_jump, node);
}

static inline struct hlsl_ir_load *hlsl_ir_load(const struct hlsl_ir_node *node)
{
    VKD3D_ASSERT(node->type == HLSL_IR_LOAD);
    return CONTAINING_RECORD(node, struct hlsl_ir_load, node);
}

static inline struct hlsl_ir_loop *hlsl_ir_loop(const struct hlsl_ir_node *node)
{
    VKD3D_ASSERT(node->type == HLSL_IR_LOOP);
    return CONTAINING_RECORD(node, struct hlsl_ir_loop, node);
}

static inline struct hlsl_ir_resource_load *hlsl_ir_resource_load(const struct hlsl_ir_node *node)
{
    VKD3D_ASSERT(node->type == HLSL_IR_RESOURCE_LOAD);
    return CONTAINING_RECORD(node, struct hlsl_ir_resource_load, node);
}

static inline struct hlsl_ir_resource_store *hlsl_ir_resource_store(const struct hlsl_ir_node *node)
{
    VKD3D_ASSERT(node->type == HLSL_IR_RESOURCE_STORE);
    return CONTAINING_RECORD(node, struct hlsl_ir_resource_store, node);
}

static inline struct hlsl_ir_store *hlsl_ir_store(const struct hlsl_ir_node *node)
{
    VKD3D_ASSERT(node->type == HLSL_IR_STORE);
    return CONTAINING_RECORD(node, struct hlsl_ir_store, node);
}

static inline struct hlsl_ir_swizzle *hlsl_ir_swizzle(const struct hlsl_ir_node *node)
{
    VKD3D_ASSERT(node->type == HLSL_IR_SWIZZLE);
    return CONTAINING_RECORD(node, struct hlsl_ir_swizzle, node);
}

static inline struct hlsl_ir_index *hlsl_ir_index(const struct hlsl_ir_node *node)
{
    VKD3D_ASSERT(node->type == HLSL_IR_INDEX);
    return CONTAINING_RECORD(node, struct hlsl_ir_index, node);
}

static inline struct hlsl_ir_switch *hlsl_ir_switch(const struct hlsl_ir_node *node)
{
    VKD3D_ASSERT(node->type == HLSL_IR_SWITCH);
    return CONTAINING_RECORD(node, struct hlsl_ir_switch, node);
}

static inline struct hlsl_ir_compile *hlsl_ir_compile(const struct hlsl_ir_node *node)
{
    VKD3D_ASSERT(node->type == HLSL_IR_COMPILE);
    return CONTAINING_RECORD(node, struct hlsl_ir_compile, node);
}

static inline struct hlsl_ir_sampler_state *hlsl_ir_sampler_state(const struct hlsl_ir_node *node)
{
    VKD3D_ASSERT(node->type == HLSL_IR_SAMPLER_STATE);
    return CONTAINING_RECORD(node, struct hlsl_ir_sampler_state, node);
};

static inline struct hlsl_ir_stateblock_constant *hlsl_ir_stateblock_constant(const struct hlsl_ir_node *node)
{
    VKD3D_ASSERT(node->type == HLSL_IR_STATEBLOCK_CONSTANT);
    return CONTAINING_RECORD(node, struct hlsl_ir_stateblock_constant, node);
}

static inline struct hlsl_ir_vsir_instruction_ref *hlsl_ir_vsir_instruction_ref(const struct hlsl_ir_node *node)
{
    VKD3D_ASSERT(node->type == HLSL_IR_VSIR_INSTRUCTION_REF);
    return CONTAINING_RECORD(node, struct hlsl_ir_vsir_instruction_ref, node);
}

static inline void hlsl_block_init(struct hlsl_block *block)
{
    list_init(&block->instrs);
    block->value = NULL;
}

static inline void hlsl_block_add_instr(struct hlsl_block *block, struct hlsl_ir_node *instr)
{
    list_add_tail(&block->instrs, &instr->entry);
    block->value = (instr->data_type ? instr : NULL);
}

static inline void hlsl_block_add_block(struct hlsl_block *block, struct hlsl_block *add)
{
    list_move_tail(&block->instrs, &add->instrs);
    block->value = add->value;
}

static inline void hlsl_src_from_node(struct hlsl_src *src, struct hlsl_ir_node *node)
{
    src->node = node;
    if (node)
        list_add_tail(&node->uses, &src->entry);
}

static inline void hlsl_src_remove(struct hlsl_src *src)
{
    if (src->node)
        list_remove(&src->entry);
    src->node = NULL;
}

static inline void *hlsl_alloc(struct hlsl_ctx *ctx, size_t size)
{
    void *ptr = vkd3d_calloc(1, size);

    if (!ptr)
        ctx->result = VKD3D_ERROR_OUT_OF_MEMORY;
    return ptr;
}

static inline void *hlsl_calloc(struct hlsl_ctx *ctx, size_t count, size_t size)
{
    void *ptr = vkd3d_calloc(count, size);

    if (!ptr)
        ctx->result = VKD3D_ERROR_OUT_OF_MEMORY;
    return ptr;
}

static inline void *hlsl_realloc(struct hlsl_ctx *ctx, void *ptr, size_t size)
{
    void *ret = vkd3d_realloc(ptr, size);

    if (!ret)
        ctx->result = VKD3D_ERROR_OUT_OF_MEMORY;
    return ret;
}

static inline char *hlsl_strdup(struct hlsl_ctx *ctx, const char *string)
{
    char *ptr = vkd3d_strdup(string);

    if (!ptr)
        ctx->result = VKD3D_ERROR_OUT_OF_MEMORY;
    return ptr;
}

static inline bool hlsl_array_reserve(struct hlsl_ctx *ctx, void **elements,
        size_t *capacity, size_t element_count, size_t element_size)
{
    bool ret = vkd3d_array_reserve(elements, capacity, element_count, element_size);

    if (!ret)
        ctx->result = VKD3D_ERROR_OUT_OF_MEMORY;
    return ret;
}

static inline struct vkd3d_string_buffer *hlsl_get_string_buffer(struct hlsl_ctx *ctx)
{
    struct vkd3d_string_buffer *ret = vkd3d_string_buffer_get(&ctx->string_buffers);

    if (!ret)
        ctx->result = VKD3D_ERROR_OUT_OF_MEMORY;
    return ret;
}

static inline void hlsl_release_string_buffer(struct hlsl_ctx *ctx, struct vkd3d_string_buffer *buffer)
{
    vkd3d_string_buffer_release(&ctx->string_buffers, buffer);
}

static inline struct hlsl_type *hlsl_get_scalar_type(const struct hlsl_ctx *ctx, enum hlsl_base_type base_type)
{
    return ctx->builtin_types.scalar[base_type];
}

static inline struct hlsl_type *hlsl_get_vector_type(const struct hlsl_ctx *ctx, enum hlsl_base_type base_type,
        unsigned int dimx)
{
    return ctx->builtin_types.vector[base_type][dimx - 1];
}

static inline struct hlsl_type *hlsl_get_matrix_type(const struct hlsl_ctx *ctx, enum hlsl_base_type base_type,
        unsigned int dimx, unsigned int dimy)
{
    return ctx->builtin_types.matrix[base_type][dimx - 1][dimy - 1];
}

static inline struct hlsl_type *hlsl_get_numeric_type(const struct hlsl_ctx *ctx, enum hlsl_type_class type,
        enum hlsl_base_type base_type, unsigned int dimx, unsigned int dimy)
{
    if (type == HLSL_CLASS_SCALAR)
        return hlsl_get_scalar_type(ctx, base_type);
    else if (type == HLSL_CLASS_VECTOR)
        return hlsl_get_vector_type(ctx, base_type, dimx);
    else
        return hlsl_get_matrix_type(ctx, base_type, dimx, dimy);
}

static inline bool hlsl_is_numeric_type(const struct hlsl_type *type)
{
    return type->class <= HLSL_CLASS_LAST_NUMERIC;
}

static inline unsigned int hlsl_sampler_dim_count(enum hlsl_sampler_dim dim)
{
    switch (dim)
    {
        case HLSL_SAMPLER_DIM_1D:
        case HLSL_SAMPLER_DIM_BUFFER:
        case HLSL_SAMPLER_DIM_RAW_BUFFER:
        case HLSL_SAMPLER_DIM_STRUCTURED_BUFFER:
            return 1;
        case HLSL_SAMPLER_DIM_1DARRAY:
        case HLSL_SAMPLER_DIM_2D:
        case HLSL_SAMPLER_DIM_2DMS:
            return 2;
        case HLSL_SAMPLER_DIM_2DARRAY:
        case HLSL_SAMPLER_DIM_2DMSARRAY:
        case HLSL_SAMPLER_DIM_3D:
        case HLSL_SAMPLER_DIM_CUBE:
            return 3;
        case HLSL_SAMPLER_DIM_CUBEARRAY:
            return 4;
        default:
            vkd3d_unreachable();
    }
}

static inline bool hlsl_var_has_buffer_offset_register_reservation(struct hlsl_ctx *ctx, const struct hlsl_ir_var *var)
{
    return var->reg_reservation.reg_type == 'c' && var->buffer == ctx->globals_buffer;
}

char *hlsl_sprintf_alloc(struct hlsl_ctx *ctx, const char *fmt, ...) VKD3D_PRINTF_FUNC(2, 3);

const char *debug_hlsl_expr_op(enum hlsl_ir_expr_op op);
const char *debug_hlsl_type(struct hlsl_ctx *ctx, const struct hlsl_type *type);
const char *debug_hlsl_writemask(unsigned int writemask);
const char *debug_hlsl_swizzle(unsigned int swizzle, unsigned int count);

struct vkd3d_string_buffer *hlsl_type_to_string(struct hlsl_ctx *ctx, const struct hlsl_type *type);
struct vkd3d_string_buffer *hlsl_component_to_string(struct hlsl_ctx *ctx, const struct hlsl_ir_var *var,
        unsigned int index);
struct vkd3d_string_buffer *hlsl_modifiers_to_string(struct hlsl_ctx *ctx, unsigned int modifiers);
const char *hlsl_node_type_to_string(enum hlsl_ir_node_type type);

struct hlsl_ir_node *hlsl_add_conditional(struct hlsl_ctx *ctx, struct hlsl_block *block,
        struct hlsl_ir_node *condition, struct hlsl_ir_node *if_true, struct hlsl_ir_node *if_false);
void hlsl_add_function(struct hlsl_ctx *ctx, char *name, struct hlsl_ir_function_decl *decl);
bool hlsl_add_var(struct hlsl_ctx *ctx, struct hlsl_ir_var *decl, bool local_var);

void hlsl_block_cleanup(struct hlsl_block *block);
bool hlsl_clone_block(struct hlsl_ctx *ctx, struct hlsl_block *dst_block, const struct hlsl_block *src_block);

void hlsl_dump_function(struct hlsl_ctx *ctx, const struct hlsl_ir_function_decl *func);
void hlsl_dump_var_default_values(const struct hlsl_ir_var *var);

bool hlsl_state_block_add_entry(struct hlsl_state_block *state_block,
        struct hlsl_state_block_entry *entry);
bool hlsl_validate_state_block_entry(struct hlsl_ctx *ctx, struct hlsl_state_block_entry *entry,
        const struct vkd3d_shader_location *loc);
struct hlsl_state_block_entry *clone_stateblock_entry(struct hlsl_ctx *ctx,
        const struct hlsl_state_block_entry *src, const char *name, bool lhs_has_index,
        unsigned int lhs_index, bool single_arg, unsigned int arg_index);

void hlsl_lower_index_loads(struct hlsl_ctx *ctx, struct hlsl_block *body);
void hlsl_run_const_passes(struct hlsl_ctx *ctx, struct hlsl_block *body);
int hlsl_emit_bytecode(struct hlsl_ctx *ctx, struct hlsl_ir_function_decl *entry_func,
        enum vkd3d_shader_target_type target_type, struct vkd3d_shader_code *out);
int hlsl_emit_effect_binary(struct hlsl_ctx *ctx, struct vkd3d_shader_code *out);

bool hlsl_init_deref_from_index_chain(struct hlsl_ctx *ctx, struct hlsl_deref *deref, struct hlsl_ir_node *chain);
bool hlsl_copy_deref(struct hlsl_ctx *ctx, struct hlsl_deref *deref, const struct hlsl_deref *other);

void hlsl_cleanup_deref(struct hlsl_deref *deref);

void hlsl_cleanup_semantic(struct hlsl_semantic *semantic);
bool hlsl_clone_semantic(struct hlsl_ctx *ctx, struct hlsl_semantic *dst, const struct hlsl_semantic *src);

void hlsl_cleanup_ir_switch_cases(struct list *cases);
void hlsl_free_ir_switch_case(struct hlsl_ir_switch_case *c);

void hlsl_replace_node(struct hlsl_ir_node *old, struct hlsl_ir_node *new);

void hlsl_free_attribute(struct hlsl_attribute *attr);
void hlsl_free_instr(struct hlsl_ir_node *node);
void hlsl_free_instr_list(struct list *list);
void hlsl_free_state_block(struct hlsl_state_block *state_block);
void hlsl_free_state_block_entry(struct hlsl_state_block_entry *state_block_entry);
void hlsl_free_type(struct hlsl_type *type);
void hlsl_free_var(struct hlsl_ir_var *decl);

struct hlsl_ir_function *hlsl_get_function(struct hlsl_ctx *ctx, const char *name);
struct hlsl_ir_function_decl *hlsl_get_first_func_decl(struct hlsl_ctx *ctx, const char *name);
struct hlsl_ir_function_decl *hlsl_get_func_decl(struct hlsl_ctx *ctx, const char *name,
        const struct hlsl_func_parameters *parameters);
const struct hlsl_profile_info *hlsl_get_target_info(const char *target);
struct hlsl_type *hlsl_get_type(struct hlsl_scope *scope, const char *name, bool recursive, bool case_insensitive);
struct hlsl_ir_var *hlsl_get_var(struct hlsl_scope *scope, const char *name);

struct hlsl_type *hlsl_get_element_type_from_path_index(struct hlsl_ctx *ctx, const struct hlsl_type *type,
        struct hlsl_ir_node *idx);

const char *hlsl_jump_type_to_string(enum hlsl_ir_jump_type type);

struct hlsl_type *hlsl_new_array_type(struct hlsl_ctx *ctx, struct hlsl_type *basic_type, unsigned int array_size);
struct hlsl_ir_node *hlsl_new_binary_expr(struct hlsl_ctx *ctx, enum hlsl_ir_expr_op op, struct hlsl_ir_node *arg1,
        struct hlsl_ir_node *arg2);
struct hlsl_ir_node *hlsl_new_bool_constant(struct hlsl_ctx *ctx, bool b, const struct vkd3d_shader_location *loc);
struct hlsl_buffer *hlsl_new_buffer(struct hlsl_ctx *ctx, enum hlsl_buffer_type type, const char *name,
        uint32_t modifiers, const struct hlsl_reg_reservation *reservation, struct hlsl_scope *annotations,
        const struct vkd3d_shader_location *loc);
struct hlsl_ir_node *hlsl_new_call(struct hlsl_ctx *ctx, struct hlsl_ir_function_decl *decl,
        const struct vkd3d_shader_location *loc);
struct hlsl_ir_node *hlsl_new_cast(struct hlsl_ctx *ctx, struct hlsl_ir_node *node, struct hlsl_type *type,
        const struct vkd3d_shader_location *loc);
struct hlsl_ir_node *hlsl_new_constant(struct hlsl_ctx *ctx, struct hlsl_type *type,
        const struct hlsl_constant_value *value, const struct vkd3d_shader_location *loc);
struct hlsl_ir_node *hlsl_new_copy(struct hlsl_ctx *ctx, struct hlsl_ir_node *node);
struct hlsl_ir_node *hlsl_new_expr(struct hlsl_ctx *ctx, enum hlsl_ir_expr_op op,
        struct hlsl_ir_node *operands[HLSL_MAX_OPERANDS],
        struct hlsl_type *data_type, const struct vkd3d_shader_location *loc);
struct hlsl_ir_node *hlsl_new_float_constant(struct hlsl_ctx *ctx,
        float f, const struct vkd3d_shader_location *loc);
struct hlsl_ir_function_decl *hlsl_new_func_decl(struct hlsl_ctx *ctx,
        struct hlsl_type *return_type, const struct hlsl_func_parameters *parameters,
        const struct hlsl_semantic *semantic, const struct vkd3d_shader_location *loc);
struct hlsl_ir_node *hlsl_new_if(struct hlsl_ctx *ctx, struct hlsl_ir_node *condition,
        struct hlsl_block *then_block, struct hlsl_block *else_block, const struct vkd3d_shader_location *loc);
struct hlsl_ir_node *hlsl_new_int_constant(struct hlsl_ctx *ctx, int32_t n, const struct vkd3d_shader_location *loc);
struct hlsl_ir_node *hlsl_new_jump(struct hlsl_ctx *ctx,
        enum hlsl_ir_jump_type type, struct hlsl_ir_node *condition, const struct vkd3d_shader_location *loc);
struct hlsl_ir_node *hlsl_new_ternary_expr(struct hlsl_ctx *ctx, enum hlsl_ir_expr_op op,
        struct hlsl_ir_node *arg1, struct hlsl_ir_node *arg2, struct hlsl_ir_node *arg3);

void hlsl_init_simple_deref_from_var(struct hlsl_deref *deref, struct hlsl_ir_var *var);

struct hlsl_ir_load *hlsl_new_var_load(struct hlsl_ctx *ctx, struct hlsl_ir_var *var,
        const struct vkd3d_shader_location *loc);
struct hlsl_ir_load *hlsl_new_load_index(struct hlsl_ctx *ctx, const struct hlsl_deref *deref,
        struct hlsl_ir_node *idx, const struct vkd3d_shader_location *loc);
struct hlsl_ir_load *hlsl_new_load_parent(struct hlsl_ctx *ctx, const struct hlsl_deref *deref,
        const struct vkd3d_shader_location *loc);
struct hlsl_ir_node *hlsl_new_load_component(struct hlsl_ctx *ctx, struct hlsl_block *block,
        const struct hlsl_deref *deref, unsigned int comp, const struct vkd3d_shader_location *loc);
struct hlsl_ir_node *hlsl_add_load_component(struct hlsl_ctx *ctx, struct hlsl_block *block,
        struct hlsl_ir_node *var_instr, unsigned int comp, const struct vkd3d_shader_location *loc);

struct hlsl_ir_node *hlsl_new_simple_store(struct hlsl_ctx *ctx, struct hlsl_ir_var *lhs, struct hlsl_ir_node *rhs);
struct hlsl_ir_node *hlsl_new_store_index(struct hlsl_ctx *ctx, const struct hlsl_deref *lhs,
        struct hlsl_ir_node *idx, struct hlsl_ir_node *rhs, unsigned int writemask, const struct vkd3d_shader_location *loc);
bool hlsl_new_store_component(struct hlsl_ctx *ctx, struct hlsl_block *block,
        const struct hlsl_deref *lhs, unsigned int comp, struct hlsl_ir_node *rhs);

bool hlsl_index_is_noncontiguous(struct hlsl_ir_index *index);
bool hlsl_index_is_resource_access(struct hlsl_ir_index *index);
bool hlsl_index_chain_has_resource_access(struct hlsl_ir_index *index);

struct hlsl_ir_node *hlsl_new_compile(struct hlsl_ctx *ctx, enum hlsl_compile_type compile_type,
        const char *profile_name, struct hlsl_ir_node **args, unsigned int args_count,
        struct hlsl_block *args_instrs, const struct vkd3d_shader_location *loc);
struct hlsl_ir_node *hlsl_new_index(struct hlsl_ctx *ctx, struct hlsl_ir_node *val,
        struct hlsl_ir_node *idx, const struct vkd3d_shader_location *loc);
struct hlsl_ir_node *hlsl_new_loop(struct hlsl_ctx *ctx,
        struct hlsl_block *block, enum hlsl_ir_loop_unroll_type unroll_type, unsigned int unroll_limit, const struct vkd3d_shader_location *loc);
struct hlsl_ir_node *hlsl_new_resource_load(struct hlsl_ctx *ctx,
        const struct hlsl_resource_load_params *params, const struct vkd3d_shader_location *loc);
struct hlsl_ir_node *hlsl_new_resource_store(struct hlsl_ctx *ctx, const struct hlsl_deref *resource,
        struct hlsl_ir_node *coords, struct hlsl_ir_node *value, const struct vkd3d_shader_location *loc);
struct hlsl_type *hlsl_new_struct_type(struct hlsl_ctx *ctx, const char *name,
        struct hlsl_struct_field *fields, size_t field_count);
struct hlsl_ir_node *hlsl_new_swizzle(struct hlsl_ctx *ctx, uint32_t s, unsigned int components,
        struct hlsl_ir_node *val, const struct vkd3d_shader_location *loc);
struct hlsl_ir_node *hlsl_new_sampler_state(struct hlsl_ctx *ctx,
        const struct hlsl_state_block *state_block, struct vkd3d_shader_location *loc);
struct hlsl_ir_node *hlsl_new_stateblock_constant(struct hlsl_ctx *ctx, const char *name,
        struct vkd3d_shader_location *loc);
struct hlsl_ir_node *hlsl_new_string_constant(struct hlsl_ctx *ctx, const char *str,
        const struct vkd3d_shader_location *loc);
struct hlsl_ir_var *hlsl_new_synthetic_var(struct hlsl_ctx *ctx, const char *template,
        struct hlsl_type *type, const struct vkd3d_shader_location *loc);
struct hlsl_ir_var *hlsl_new_synthetic_var_named(struct hlsl_ctx *ctx, const char *name,
    struct hlsl_type *type, const struct vkd3d_shader_location *loc, bool dummy_scope);
struct hlsl_type *hlsl_new_texture_type(struct hlsl_ctx *ctx, enum hlsl_sampler_dim dim, struct hlsl_type *format,
        unsigned int sample_count);
struct hlsl_type *hlsl_new_uav_type(struct hlsl_ctx *ctx, enum hlsl_sampler_dim dim,
        struct hlsl_type *format, bool rasteriser_ordered);
struct hlsl_type *hlsl_new_cb_type(struct hlsl_ctx *ctx, struct hlsl_type *format);
struct hlsl_ir_node *hlsl_new_uint_constant(struct hlsl_ctx *ctx, unsigned int n,
        const struct vkd3d_shader_location *loc);
struct hlsl_ir_node *hlsl_new_null_constant(struct hlsl_ctx *ctx, const struct vkd3d_shader_location *loc);
struct hlsl_ir_node *hlsl_new_unary_expr(struct hlsl_ctx *ctx, enum hlsl_ir_expr_op op, struct hlsl_ir_node *arg,
        const struct vkd3d_shader_location *loc);
struct hlsl_ir_var *hlsl_new_var(struct hlsl_ctx *ctx, const char *name, struct hlsl_type *type,
        const struct vkd3d_shader_location *loc, const struct hlsl_semantic *semantic, uint32_t modifiers,
        const struct hlsl_reg_reservation *reg_reservation);
struct hlsl_ir_switch_case *hlsl_new_switch_case(struct hlsl_ctx *ctx, unsigned int value, bool is_default,
        struct hlsl_block *body, const struct vkd3d_shader_location *loc);
struct hlsl_ir_node *hlsl_new_switch(struct hlsl_ctx *ctx, struct hlsl_ir_node *selector,
        struct list *cases, const struct vkd3d_shader_location *loc);

struct hlsl_ir_node *hlsl_new_vsir_instruction_ref(struct hlsl_ctx *ctx, unsigned int vsir_instr_idx,
        struct hlsl_type *type, const struct hlsl_reg *reg, const struct vkd3d_shader_location *loc);

void hlsl_error(struct hlsl_ctx *ctx, const struct vkd3d_shader_location *loc,
        enum vkd3d_shader_error error, const char *fmt, ...) VKD3D_PRINTF_FUNC(4, 5);
void hlsl_fixme(struct hlsl_ctx *ctx, const struct vkd3d_shader_location *loc,
        const char *fmt, ...) VKD3D_PRINTF_FUNC(3, 4);
void hlsl_warning(struct hlsl_ctx *ctx, const struct vkd3d_shader_location *loc,
        enum vkd3d_shader_error error, const char *fmt, ...) VKD3D_PRINTF_FUNC(4, 5);
void hlsl_note(struct hlsl_ctx *ctx, const struct vkd3d_shader_location *loc,
        enum vkd3d_shader_log_level level, const char *fmt, ...) VKD3D_PRINTF_FUNC(4, 5);

void hlsl_push_scope(struct hlsl_ctx *ctx);
void hlsl_pop_scope(struct hlsl_ctx *ctx);

bool hlsl_scope_add_type(struct hlsl_scope *scope, struct hlsl_type *type);

struct hlsl_type *hlsl_type_clone(struct hlsl_ctx *ctx, struct hlsl_type *old,
        unsigned int default_majority, uint32_t modifiers);
unsigned int hlsl_type_component_count(const struct hlsl_type *type);
unsigned int hlsl_type_get_array_element_reg_size(const struct hlsl_type *type, enum hlsl_regset regset);
struct hlsl_type *hlsl_type_get_component_type(struct hlsl_ctx *ctx, struct hlsl_type *type,
        unsigned int index);
unsigned int hlsl_type_get_component_offset(struct hlsl_ctx *ctx, struct hlsl_type *type,
        unsigned int index, enum hlsl_regset *regset);
bool hlsl_type_is_row_major(const struct hlsl_type *type);
unsigned int hlsl_type_minor_size(const struct hlsl_type *type);
unsigned int hlsl_type_major_size(const struct hlsl_type *type);
unsigned int hlsl_type_element_count(const struct hlsl_type *type);
bool hlsl_type_is_resource(const struct hlsl_type *type);
bool hlsl_type_is_shader(const struct hlsl_type *type);
unsigned int hlsl_type_get_sm4_offset(const struct hlsl_type *type, unsigned int offset);
bool hlsl_types_are_equal(const struct hlsl_type *t1, const struct hlsl_type *t2);

void hlsl_calculate_buffer_offsets(struct hlsl_ctx *ctx);

const struct hlsl_type *hlsl_get_multiarray_element_type(const struct hlsl_type *type);
unsigned int hlsl_get_multiarray_size(const struct hlsl_type *type);

uint32_t hlsl_combine_swizzles(uint32_t first, uint32_t second, unsigned int dim);
unsigned int hlsl_combine_writemasks(unsigned int first, unsigned int second);
uint32_t hlsl_map_swizzle(uint32_t swizzle, unsigned int writemask);
uint32_t hlsl_swizzle_from_writemask(unsigned int writemask);

struct hlsl_type *hlsl_deref_get_type(struct hlsl_ctx *ctx, const struct hlsl_deref *deref);
enum hlsl_regset hlsl_deref_get_regset(struct hlsl_ctx *ctx, const struct hlsl_deref *deref);
bool hlsl_component_index_range_from_deref(struct hlsl_ctx *ctx, const struct hlsl_deref *deref,
        unsigned int *start, unsigned int *count);
bool hlsl_regset_index_from_deref(struct hlsl_ctx *ctx, const struct hlsl_deref *deref,
        enum hlsl_regset regset, unsigned int *index);
bool hlsl_offset_from_deref(struct hlsl_ctx *ctx, const struct hlsl_deref *deref, unsigned int *offset);
unsigned int hlsl_offset_from_deref_safe(struct hlsl_ctx *ctx, const struct hlsl_deref *deref);
struct hlsl_reg hlsl_reg_from_deref(struct hlsl_ctx *ctx, const struct hlsl_deref *deref);

bool hlsl_copy_propagation_execute(struct hlsl_ctx *ctx, struct hlsl_block *block);
bool hlsl_fold_constant_exprs(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, void *context);
bool hlsl_fold_constant_identities(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, void *context);
bool hlsl_fold_constant_swizzles(struct hlsl_ctx *ctx, struct hlsl_ir_node *instr, void *context);
bool hlsl_transform_ir(struct hlsl_ctx *ctx, bool (*func)(struct hlsl_ctx *ctx, struct hlsl_ir_node *, void *),
        struct hlsl_block *block, void *context);

D3DXPARAMETER_CLASS hlsl_sm1_class(const struct hlsl_type *type);
D3DXPARAMETER_TYPE hlsl_sm1_base_type(const struct hlsl_type *type);

void write_sm1_uniforms(struct hlsl_ctx *ctx, struct vkd3d_bytecode_buffer *buffer);
int d3dbc_compile(struct vsir_program *program, uint64_t config_flags,
        const struct vkd3d_shader_compile_info *compile_info, const struct vkd3d_shader_code *ctab,
        struct vkd3d_shader_code *out, struct vkd3d_shader_message_context *message_context);

int tpf_compile(struct vsir_program *program, uint64_t config_flags,
        struct vkd3d_shader_code *out, struct vkd3d_shader_message_context *message_context,
        struct hlsl_ctx *ctx, struct hlsl_ir_function_decl *entry_func);

enum vkd3d_shader_interpolation_mode sm4_get_interpolation_mode(struct hlsl_type *type,
        unsigned int storage_modifiers);

struct hlsl_ir_function_decl *hlsl_compile_internal_function(struct hlsl_ctx *ctx, const char *name, const char *hlsl);

int hlsl_lexer_compile(struct hlsl_ctx *ctx, const struct vkd3d_shader_code *hlsl);

#endif

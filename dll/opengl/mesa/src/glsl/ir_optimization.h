/*
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


/**
 * \file ir_optimization.h
 *
 * Prototypes for optimization passes to be called by the compiler and drivers.
 */

/* Operations for lower_instructions() */
#define SUB_TO_ADD_NEG     0x01
#define DIV_TO_MUL_RCP     0x02
#define EXP_TO_EXP2        0x04
#define POW_TO_EXP2        0x08
#define LOG_TO_LOG2        0x10
#define MOD_TO_FRACT       0x20
#define INT_DIV_TO_MUL_RCP 0x40

bool do_common_optimization(exec_list *ir, bool linked,
			    bool uniform_locations_assigned,
			    unsigned max_unroll_iterations);

bool do_algebraic(exec_list *instructions);
bool do_constant_folding(exec_list *instructions);
bool do_constant_variable(exec_list *instructions);
bool do_constant_variable_unlinked(exec_list *instructions);
bool do_copy_propagation(exec_list *instructions);
bool do_copy_propagation_elements(exec_list *instructions);
bool do_constant_propagation(exec_list *instructions);
bool do_dead_code(exec_list *instructions, bool uniform_locations_assigned);
bool do_dead_code_local(exec_list *instructions);
bool do_dead_code_unlinked(exec_list *instructions);
bool do_dead_functions(exec_list *instructions);
bool do_function_inlining(exec_list *instructions);
bool do_lower_jumps(exec_list *instructions, bool pull_out_jumps = true, bool lower_sub_return = true, bool lower_main_return = false, bool lower_continue = false, bool lower_break = false);
bool do_lower_texture_projection(exec_list *instructions);
bool do_if_simplification(exec_list *instructions);
bool do_discard_simplification(exec_list *instructions);
bool lower_if_to_cond_assign(exec_list *instructions, unsigned max_depth = 0);
bool do_mat_op_to_vec(exec_list *instructions);
bool do_noop_swizzle(exec_list *instructions);
bool do_structure_splitting(exec_list *instructions);
bool do_swizzle_swizzle(exec_list *instructions);
bool do_tree_grafting(exec_list *instructions);
bool do_vec_index_to_cond_assign(exec_list *instructions);
bool do_vec_index_to_swizzle(exec_list *instructions);
bool lower_discard(exec_list *instructions);
bool lower_instructions(exec_list *instructions, unsigned what_to_lower);
bool lower_noise(exec_list *instructions);
bool lower_variable_index_to_cond_assign(exec_list *instructions,
    bool lower_input, bool lower_output, bool lower_temp, bool lower_uniform);
bool lower_quadop_vector(exec_list *instructions, bool dont_lower_swz);
bool lower_clip_distance(exec_list *instructions);
void lower_output_reads(exec_list *instructions);
bool optimize_redundant_jumps(exec_list *instructions);

ir_rvalue *
compare_index_block(exec_list *instructions, ir_variable *index,
		    unsigned base, unsigned components, void *mem_ctx);

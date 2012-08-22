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
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * Authors:
 *    Eric Anholt <eric@anholt.net>
 *
 */

struct ra_class;
struct ra_regs;

/* @{
 * Register set setup.
 *
 * This should be done once at backend initializaion, as
 * ra_set_finalize is O(r^2*c^2).  The registers may be virtual
 * registers, such as aligned register pairs that conflict with the
 * two real registers from which they are composed.
 */
struct ra_regs *ra_alloc_reg_set(void *mem_ctx, unsigned int count);
unsigned int ra_alloc_reg_class(struct ra_regs *regs);
void ra_add_reg_conflict(struct ra_regs *regs,
			 unsigned int r1, unsigned int r2);
void ra_add_transitive_reg_conflict(struct ra_regs *regs,
				    unsigned int base_reg, unsigned int reg);
void ra_class_add_reg(struct ra_regs *regs, unsigned int c, unsigned int reg);
void ra_set_finalize(struct ra_regs *regs);
/** @} */

/** @{ Interference graph setup.
 *
 * Each interference graph node is a virtual variable in the IL.  It
 * is up to the user to ra_set_node_class() for the virtual variable,
 * and compute live ranges and ra_node_interfere() between conflicting
 * live ranges.
 */
struct ra_graph *ra_alloc_interference_graph(struct ra_regs *regs,
					     unsigned int count);
void ra_set_node_class(struct ra_graph *g, unsigned int n, unsigned int c);
void ra_add_node_interference(struct ra_graph *g,
			      unsigned int n1, unsigned int n2);
/** @} */

/** @{ Graph-coloring register allocation */
GLboolean ra_simplify(struct ra_graph *g);
void ra_optimistic_color(struct ra_graph *g);
GLboolean ra_select(struct ra_graph *g);
GLboolean ra_allocate_no_spills(struct ra_graph *g);

unsigned int ra_get_node_reg(struct ra_graph *g, unsigned int n);
void ra_set_node_reg(struct ra_graph * g, unsigned int n, unsigned int reg);
void ra_set_node_spill_cost(struct ra_graph *g, unsigned int n, float cost);
int ra_get_best_spill_node(struct ra_graph *g);
/** @} */


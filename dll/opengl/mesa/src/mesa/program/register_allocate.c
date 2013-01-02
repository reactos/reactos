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

/** @file register_allocate.c
 *
 * Graph-coloring register allocator.
 *
 * The basic idea of graph coloring is to make a node in a graph for
 * every thing that needs a register (color) number assigned, and make
 * edges in the graph between nodes that interfere (can't be allocated
 * to the same register at the same time).
 *
 * During the "simplify" process, any any node with fewer edges than
 * there are registers means that that edge can get assigned a
 * register regardless of what its neighbors choose, so that node is
 * pushed on a stack and removed (with its edges) from the graph.
 * That likely causes other nodes to become trivially colorable as well.
 *
 * Then during the "select" process, nodes are popped off of that
 * stack, their edges restored, and assigned a color different from
 * their neighbors.  Because they were pushed on the stack only when
 * they were trivially colorable, any color chosen won't interfere
 * with the registers to be popped later.
 *
 * The downside to most graph coloring is that real hardware often has
 * limitations, like registers that need to be allocated to a node in
 * pairs, or aligned on some boundary.  This implementation follows
 * the paper "Retargetable Graph-Coloring Register Allocation for
 * Irregular Architectures" by Johan Runeson and Sven-Olof Nyström.
 *
 * In this system, there are register classes each containing various
 * registers, and registers may interfere with other registers.  For
 * example, one might have a class of base registers, and a class of
 * aligned register pairs that would each interfere with their pair of
 * the base registers.  Each node has a register class it needs to be
 * assigned to.  Define p(B) to be the size of register class B, and
 * q(B,C) to be the number of registers in B that the worst choice
 * register in C could conflict with.  Then, this system replaces the
 * basic graph coloring test of "fewer edges from this node than there
 * are registers" with "For this node of class B, the sum of q(B,C)
 * for each neighbor node of class C is less than pB".
 *
 * A nice feature of the pq test is that q(B,C) can be computed once
 * up front and stored in a 2-dimensional array, so that the cost of
 * coloring a node is constant with the number of registers.  We do
 * this during ra_set_finalize().
 */

#include <ralloc.h>

#include "main/imports.h"
#include "main/macros.h"
#include "main/mtypes.h"
#include "register_allocate.h"

#define NO_REG ~0

struct ra_reg {
   GLboolean *conflicts;
   unsigned int *conflict_list;
   unsigned int conflict_list_size;
   unsigned int num_conflicts;
};

struct ra_regs {
   struct ra_reg *regs;
   unsigned int count;

   struct ra_class **classes;
   unsigned int class_count;
};

struct ra_class {
   GLboolean *regs;

   /**
    * p(B) in Runeson/Nyström paper.
    *
    * This is "how many regs are in the set."
    */
   unsigned int p;

   /**
    * q(B,C) (indexed by C, B is this register class) in
    * Runeson/Nyström paper.  This is "how many registers of B could
    * the worst choice register from C conflict with".
    */
   unsigned int *q;
};

struct ra_node {
   /** @{
    *
    * List of which nodes this node interferes with.  This should be
    * symmetric with the other node.
    */
   GLboolean *adjacency;
   unsigned int *adjacency_list;
   unsigned int adjacency_count;
   /** @} */

   unsigned int class;

   /* Register, if assigned, or NO_REG. */
   unsigned int reg;

   /**
    * Set when the node is in the trivially colorable stack.  When
    * set, the adjacency to this node is ignored, to implement the
    * "remove the edge from the graph" in simplification without
    * having to actually modify the adjacency_list.
    */
   GLboolean in_stack;

   /* For an implementation that needs register spilling, this is the
    * approximate cost of spilling this node.
    */
   float spill_cost;
};

struct ra_graph {
   struct ra_regs *regs;
   /**
    * the variables that need register allocation.
    */
   struct ra_node *nodes;
   unsigned int count; /**< count of nodes. */

   unsigned int *stack;
   unsigned int stack_count;
};

/**
 * Creates a set of registers for the allocator.
 *
 * mem_ctx is a ralloc context for the allocator.  The reg set may be freed
 * using ralloc_free().
 */
struct ra_regs *
ra_alloc_reg_set(void *mem_ctx, unsigned int count)
{
   unsigned int i;
   struct ra_regs *regs;

   regs = rzalloc(mem_ctx, struct ra_regs);
   regs->count = count;
   regs->regs = rzalloc_array(regs, struct ra_reg, count);

   for (i = 0; i < count; i++) {
      regs->regs[i].conflicts = rzalloc_array(regs->regs, GLboolean, count);
      regs->regs[i].conflicts[i] = GL_TRUE;

      regs->regs[i].conflict_list = ralloc_array(regs->regs, unsigned int, 4);
      regs->regs[i].conflict_list_size = 4;
      regs->regs[i].conflict_list[0] = i;
      regs->regs[i].num_conflicts = 1;
   }

   return regs;
}

static void
ra_add_conflict_list(struct ra_regs *regs, unsigned int r1, unsigned int r2)
{
   struct ra_reg *reg1 = &regs->regs[r1];

   if (reg1->conflict_list_size == reg1->num_conflicts) {
      reg1->conflict_list_size *= 2;
      reg1->conflict_list = reralloc(regs->regs, reg1->conflict_list,
				     unsigned int, reg1->conflict_list_size);
   }
   reg1->conflict_list[reg1->num_conflicts++] = r2;
   reg1->conflicts[r2] = GL_TRUE;
}

void
ra_add_reg_conflict(struct ra_regs *regs, unsigned int r1, unsigned int r2)
{
   if (!regs->regs[r1].conflicts[r2]) {
      ra_add_conflict_list(regs, r1, r2);
      ra_add_conflict_list(regs, r2, r1);
   }
}

/**
 * Adds a conflict between base_reg and reg, and also between reg and
 * anything that base_reg conflicts with.
 *
 * This can simplify code for setting up multiple register classes
 * which are aggregates of some base hardware registers, compared to
 * explicitly using ra_add_reg_conflict.
 */
void
ra_add_transitive_reg_conflict(struct ra_regs *regs,
			       unsigned int base_reg, unsigned int reg)
{
   int i;

   ra_add_reg_conflict(regs, reg, base_reg);

   for (i = 0; i < regs->regs[base_reg].num_conflicts; i++) {
      ra_add_reg_conflict(regs, reg, regs->regs[base_reg].conflict_list[i]);
   }
}

unsigned int
ra_alloc_reg_class(struct ra_regs *regs)
{
   struct ra_class *class;

   regs->classes = reralloc(regs->regs, regs->classes, struct ra_class *,
			    regs->class_count + 1);

   class = rzalloc(regs, struct ra_class);
   regs->classes[regs->class_count] = class;

   class->regs = rzalloc_array(class, GLboolean, regs->count);

   return regs->class_count++;
}

void
ra_class_add_reg(struct ra_regs *regs, unsigned int c, unsigned int r)
{
   struct ra_class *class = regs->classes[c];

   class->regs[r] = GL_TRUE;
   class->p++;
}

/**
 * Must be called after all conflicts and register classes have been
 * set up and before the register set is used for allocation.
 */
void
ra_set_finalize(struct ra_regs *regs)
{
   unsigned int b, c;

   for (b = 0; b < regs->class_count; b++) {
      regs->classes[b]->q = ralloc_array(regs, unsigned int, regs->class_count);
   }

   /* Compute, for each class B and C, how many regs of B an
    * allocation to C could conflict with.
    */
   for (b = 0; b < regs->class_count; b++) {
      for (c = 0; c < regs->class_count; c++) {
	 unsigned int rc;
	 int max_conflicts = 0;

	 for (rc = 0; rc < regs->count; rc++) {
	    int conflicts = 0;
	    int i;

	    if (!regs->classes[c]->regs[rc])
	       continue;

	    for (i = 0; i < regs->regs[rc].num_conflicts; i++) {
	       unsigned int rb = regs->regs[rc].conflict_list[i];
	       if (regs->classes[b]->regs[rb])
		  conflicts++;
	    }
	    max_conflicts = MAX2(max_conflicts, conflicts);
	 }
	 regs->classes[b]->q[c] = max_conflicts;
      }
   }
}

static void
ra_add_node_adjacency(struct ra_graph *g, unsigned int n1, unsigned int n2)
{
   g->nodes[n1].adjacency[n2] = GL_TRUE;
   g->nodes[n1].adjacency_list[g->nodes[n1].adjacency_count] = n2;
   g->nodes[n1].adjacency_count++;
}

struct ra_graph *
ra_alloc_interference_graph(struct ra_regs *regs, unsigned int count)
{
   struct ra_graph *g;
   unsigned int i;

   g = rzalloc(regs, struct ra_graph);
   g->regs = regs;
   g->nodes = rzalloc_array(g, struct ra_node, count);
   g->count = count;

   g->stack = rzalloc_array(g, unsigned int, count);

   for (i = 0; i < count; i++) {
      g->nodes[i].adjacency = rzalloc_array(g, GLboolean, count);
      g->nodes[i].adjacency_list = ralloc_array(g, unsigned int, count);
      g->nodes[i].adjacency_count = 0;
      ra_add_node_adjacency(g, i, i);
      g->nodes[i].reg = NO_REG;
   }

   return g;
}

void
ra_set_node_class(struct ra_graph *g,
		  unsigned int n, unsigned int class)
{
   g->nodes[n].class = class;
}

void
ra_add_node_interference(struct ra_graph *g,
			 unsigned int n1, unsigned int n2)
{
   if (!g->nodes[n1].adjacency[n2]) {
      ra_add_node_adjacency(g, n1, n2);
      ra_add_node_adjacency(g, n2, n1);
   }
}

static GLboolean pq_test(struct ra_graph *g, unsigned int n)
{
   unsigned int j;
   unsigned int q = 0;
   int n_class = g->nodes[n].class;

   for (j = 0; j < g->nodes[n].adjacency_count; j++) {
      unsigned int n2 = g->nodes[n].adjacency_list[j];
      unsigned int n2_class = g->nodes[n2].class;

      if (n != n2 && !g->nodes[n2].in_stack) {
	 q += g->regs->classes[n_class]->q[n2_class];
      }
   }

   return q < g->regs->classes[n_class]->p;
}

/**
 * Simplifies the interference graph by pushing all
 * trivially-colorable nodes into a stack of nodes to be colored,
 * removing them from the graph, and rinsing and repeating.
 *
 * Returns GL_TRUE if all nodes were removed from the graph.  GL_FALSE
 * means that either spilling will be required, or optimistic coloring
 * should be applied.
 */
GLboolean
ra_simplify(struct ra_graph *g)
{
   GLboolean progress = GL_TRUE;
   int i;

   while (progress) {
      progress = GL_FALSE;

      for (i = g->count - 1; i >= 0; i--) {
	 if (g->nodes[i].in_stack || g->nodes[i].reg != NO_REG)
	    continue;

	 if (pq_test(g, i)) {
	    g->stack[g->stack_count] = i;
	    g->stack_count++;
	    g->nodes[i].in_stack = GL_TRUE;
	    progress = GL_TRUE;
	 }
      }
   }

   for (i = 0; i < g->count; i++) {
      if (!g->nodes[i].in_stack)
	 return GL_FALSE;
   }

   return GL_TRUE;
}

/**
 * Pops nodes from the stack back into the graph, coloring them with
 * registers as they go.
 *
 * If all nodes were trivially colorable, then this must succeed.  If
 * not (optimistic coloring), then it may return GL_FALSE;
 */
GLboolean
ra_select(struct ra_graph *g)
{
   int i;

   while (g->stack_count != 0) {
      unsigned int r;
      int n = g->stack[g->stack_count - 1];
      struct ra_class *c = g->regs->classes[g->nodes[n].class];

      /* Find the lowest-numbered reg which is not used by a member
       * of the graph adjacent to us.
       */
      for (r = 0; r < g->regs->count; r++) {
	 if (!c->regs[r])
	    continue;

	 /* Check if any of our neighbors conflict with this register choice. */
	 for (i = 0; i < g->nodes[n].adjacency_count; i++) {
	    unsigned int n2 = g->nodes[n].adjacency_list[i];

	    if (!g->nodes[n2].in_stack &&
		g->regs->regs[r].conflicts[g->nodes[n2].reg]) {
	       break;
	    }
	 }
	 if (i == g->nodes[n].adjacency_count)
	    break;
      }
      if (r == g->regs->count)
	 return GL_FALSE;

      g->nodes[n].reg = r;
      g->nodes[n].in_stack = GL_FALSE;
      g->stack_count--;
   }

   return GL_TRUE;
}

/**
 * Optimistic register coloring: Just push the remaining nodes
 * on the stack.  They'll be colored first in ra_select(), and
 * if they succeed then the locally-colorable nodes are still
 * locally-colorable and the rest of the register allocation
 * will succeed.
 */
void
ra_optimistic_color(struct ra_graph *g)
{
   unsigned int i;

   for (i = 0; i < g->count; i++) {
      if (g->nodes[i].in_stack || g->nodes[i].reg != NO_REG)
	 continue;

      g->stack[g->stack_count] = i;
      g->stack_count++;
      g->nodes[i].in_stack = GL_TRUE;
   }
}

GLboolean
ra_allocate_no_spills(struct ra_graph *g)
{
   if (!ra_simplify(g)) {
      ra_optimistic_color(g);
   }
   return ra_select(g);
}

unsigned int
ra_get_node_reg(struct ra_graph *g, unsigned int n)
{
   return g->nodes[n].reg;
}

/**
 * Forces a node to a specific register.  This can be used to avoid
 * creating a register class containing one node when handling data
 * that must live in a fixed location and is known to not conflict
 * with other forced register assignment (as is common with shader
 * input data).  These nodes do not end up in the stack during
 * ra_simplify(), and thus at ra_select() time it is as if they were
 * the first popped off the stack and assigned their fixed locations.
 *
 * Must be called before ra_simplify().
 */
void
ra_set_node_reg(struct ra_graph *g, unsigned int n, unsigned int reg)
{
   g->nodes[n].reg = reg;
   g->nodes[n].in_stack = GL_FALSE;
}

static float
ra_get_spill_benefit(struct ra_graph *g, unsigned int n)
{
   int j;
   float benefit = 0;
   int n_class = g->nodes[n].class;

   /* Define the benefit of eliminating an interference between n, n2
    * through spilling as q(C, B) / p(C).  This is similar to the
    * "count number of edges" approach of traditional graph coloring,
    * but takes classes into account.
    */
   for (j = 0; j < g->nodes[n].adjacency_count; j++) {
      unsigned int n2 = g->nodes[n].adjacency_list[j];
      if (n != n2) {
	 unsigned int n2_class = g->nodes[n2].class;
	 benefit += ((float)g->regs->classes[n_class]->q[n2_class] /
		     g->regs->classes[n_class]->p);
      }
   }

   return benefit;
}

/**
 * Returns a node number to be spilled according to the cost/benefit using
 * the pq test, or -1 if there are no spillable nodes.
 */
int
ra_get_best_spill_node(struct ra_graph *g)
{
   unsigned int best_node = -1;
   unsigned int best_benefit = 0.0;
   unsigned int n;

   for (n = 0; n < g->count; n++) {
      float cost = g->nodes[n].spill_cost;
      float benefit;

      if (cost <= 0.0)
	 continue;

      benefit = ra_get_spill_benefit(g, n);

      if (benefit / cost > best_benefit) {
	 best_benefit = benefit / cost;
	 best_node = n;
      }
   }

   return best_node;
}

/**
 * Only nodes with a spill cost set (cost != 0.0) will be considered
 * for register spilling.
 */
void
ra_set_node_spill_cost(struct ra_graph *g, unsigned int n, float cost)
{
   g->nodes[n].spill_cost = cost;
}

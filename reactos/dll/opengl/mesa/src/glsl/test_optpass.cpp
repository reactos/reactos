/*
 * Copyright © 2011 Intel Corporation
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
 * \file test_optpass.cpp
 *
 * Standalone test for optimization passes.
 *
 * This file provides the "optpass" command for the standalone
 * glsl_test app.  It accepts either GLSL or high-level IR as input,
 * and performs the optimiation passes specified on the command line.
 * It outputs the IR, both before and after optimiations.
 */

#include <string>
#include <iostream>
#include <sstream>
#include <getopt.h>

#include "ast.h"
#include "ir_optimization.h"
#include "ir_print_visitor.h"
#include "program.h"
#include "ir_reader.h"
#include "standalone_scaffolding.h"

using namespace std;

static string read_stdin_to_eof()
{
   stringbuf sb;
   cin.get(sb, '\0');
   return sb.str();
}

static GLboolean
do_optimization(struct exec_list *ir, const char *optimization)
{
   int int_0;
   int int_1;
   int int_2;
   int int_3;
   int int_4;

   if (sscanf(optimization, "do_common_optimization ( %d , %d ) ",
              &int_0, &int_1) == 2) {
      return do_common_optimization(ir, int_0 != 0, false, int_1);
   } else if (strcmp(optimization, "do_algebraic") == 0) {
      return do_algebraic(ir);
   } else if (strcmp(optimization, "do_constant_folding") == 0) {
      return do_constant_folding(ir);
   } else if (strcmp(optimization, "do_constant_variable") == 0) {
      return do_constant_variable(ir);
   } else if (strcmp(optimization, "do_constant_variable_unlinked") == 0) {
      return do_constant_variable_unlinked(ir);
   } else if (strcmp(optimization, "do_copy_propagation") == 0) {
      return do_copy_propagation(ir);
   } else if (strcmp(optimization, "do_copy_propagation_elements") == 0) {
      return do_copy_propagation_elements(ir);
   } else if (strcmp(optimization, "do_constant_propagation") == 0) {
      return do_constant_propagation(ir);
   } else if (strcmp(optimization, "do_dead_code") == 0) {
      return do_dead_code(ir, false);
   } else if (strcmp(optimization, "do_dead_code_local") == 0) {
      return do_dead_code_local(ir);
   } else if (strcmp(optimization, "do_dead_code_unlinked") == 0) {
      return do_dead_code_unlinked(ir);
   } else if (strcmp(optimization, "do_dead_functions") == 0) {
      return do_dead_functions(ir);
   } else if (strcmp(optimization, "do_function_inlining") == 0) {
      return do_function_inlining(ir);
   } else if (sscanf(optimization,
                     "do_lower_jumps ( %d , %d , %d , %d , %d ) ",
                     &int_0, &int_1, &int_2, &int_3, &int_4) == 5) {
      return do_lower_jumps(ir, int_0 != 0, int_1 != 0, int_2 != 0,
                            int_3 != 0, int_4 != 0);
   } else if (strcmp(optimization, "do_lower_texture_projection") == 0) {
      return do_lower_texture_projection(ir);
   } else if (strcmp(optimization, "do_if_simplification") == 0) {
      return do_if_simplification(ir);
   } else if (strcmp(optimization, "do_discard_simplification") == 0) {
      return do_discard_simplification(ir);
   } else if (sscanf(optimization, "lower_if_to_cond_assign ( %d ) ",
                     &int_0) == 1) {
      return lower_if_to_cond_assign(ir, int_0);
   } else if (strcmp(optimization, "do_mat_op_to_vec") == 0) {
      return do_mat_op_to_vec(ir);
   } else if (strcmp(optimization, "do_noop_swizzle") == 0) {
      return do_noop_swizzle(ir);
   } else if (strcmp(optimization, "do_structure_splitting") == 0) {
      return do_structure_splitting(ir);
   } else if (strcmp(optimization, "do_swizzle_swizzle") == 0) {
      return do_swizzle_swizzle(ir);
   } else if (strcmp(optimization, "do_tree_grafting") == 0) {
      return do_tree_grafting(ir);
   } else if (strcmp(optimization, "do_vec_index_to_cond_assign") == 0) {
      return do_vec_index_to_cond_assign(ir);
   } else if (strcmp(optimization, "do_vec_index_to_swizzle") == 0) {
      return do_vec_index_to_swizzle(ir);
   } else if (strcmp(optimization, "lower_discard") == 0) {
      return lower_discard(ir);
   } else if (sscanf(optimization, "lower_instructions ( %d ) ",
                     &int_0) == 1) {
      return lower_instructions(ir, int_0);
   } else if (strcmp(optimization, "lower_noise") == 0) {
      return lower_noise(ir);
   } else if (sscanf(optimization, "lower_variable_index_to_cond_assign "
                     "( %d , %d , %d , %d ) ", &int_0, &int_1, &int_2,
                     &int_3) == 4) {
      return lower_variable_index_to_cond_assign(ir, int_0 != 0, int_1 != 0,
                                                 int_2 != 0, int_3 != 0);
   } else if (sscanf(optimization, "lower_quadop_vector ( %d ) ",
                     &int_0) == 1) {
      return lower_quadop_vector(ir, int_0 != 0);
   } else if (strcmp(optimization, "optimize_redundant_jumps") == 0) {
      return optimize_redundant_jumps(ir);
   } else {
      printf("Unrecognized optimization %s\n", optimization);
      exit(EXIT_FAILURE);
      return false;
   }
}

static GLboolean
do_optimization_passes(struct exec_list *ir, char **optimizations,
                       int num_optimizations, bool quiet)
{
   GLboolean overall_progress = false;

   for (int i = 0; i < num_optimizations; ++i) {
      const char *optimization = optimizations[i];
      if (!quiet) {
         printf("*** Running optimization %s...", optimization);
      }
      GLboolean progress = do_optimization(ir, optimization);
      if (!quiet) {
         printf("%s\n", progress ? "progress" : "no progress");
      }
      validate_ir_tree(ir);

      overall_progress = overall_progress || progress;
   }

   return overall_progress;
}

int test_optpass(int argc, char **argv)
{
   int input_format_ir = 0; /* 0=glsl, 1=ir */
   int loop = 0;
   int shader_type = GL_VERTEX_SHADER;
   int quiet = 0;

   const struct option optpass_opts[] = {
      { "input-ir", no_argument, &input_format_ir, 1 },
      { "input-glsl", no_argument, &input_format_ir, 0 },
      { "loop", no_argument, &loop, 1 },
      { "vertex-shader", no_argument, &shader_type, GL_VERTEX_SHADER },
      { "fragment-shader", no_argument, &shader_type, GL_FRAGMENT_SHADER },
      { "quiet", no_argument, &quiet, 1 },
      { NULL, 0, NULL, 0 }
   };

   int idx = 0;
   int c;
   while ((c = getopt_long(argc, argv, "", optpass_opts, &idx)) != -1) {
      if (c != 0) {
         printf("*** usage: %s optpass <optimizations> <options>\n", argv[0]);
         printf("\n");
         printf("Possible options are:\n");
         printf("  --input-ir: input format is IR\n");
         printf("  --input-glsl: input format is GLSL (the default)\n");
         printf("  --loop: run optimizations repeatedly until no progress\n");
         printf("  --vertex-shader: test with a vertex shader (the default)\n");
         printf("  --fragment-shader: test with a fragment shader\n");
         exit(EXIT_FAILURE);
      }
   }

   struct gl_context local_ctx;
   struct gl_context *ctx = &local_ctx;
   initialize_context_to_defaults(ctx, API_OPENGL);

   ctx->Driver.NewShader = _mesa_new_shader;

   struct gl_shader *shader = rzalloc(NULL, struct gl_shader);
   shader->Type = shader_type;

   string input = read_stdin_to_eof();

   struct _mesa_glsl_parse_state *state
      = new(shader) _mesa_glsl_parse_state(ctx, shader->Type, shader);

   if (input_format_ir) {
      shader->ir = new(shader) exec_list;
      _mesa_glsl_initialize_types(state);
      _mesa_glsl_read_ir(state, shader->ir, input.c_str(), true);
   } else {
      shader->Source = input.c_str();
      const char *source = shader->Source;
      state->error = preprocess(state, &source, &state->info_log,
                                state->extensions, ctx->API) != 0;

      if (!state->error) {
         _mesa_glsl_lexer_ctor(state, source);
         _mesa_glsl_parse(state);
         _mesa_glsl_lexer_dtor(state);
      }

      shader->ir = new(shader) exec_list;
      if (!state->error && !state->translation_unit.is_empty())
         _mesa_ast_to_hir(shader->ir, state);
   }

   /* Print out the initial IR */
   if (!state->error && !quiet) {
      printf("*** pre-optimization IR:\n");
      _mesa_print_ir(shader->ir, state);
      printf("\n--\n");
   }

   /* Optimization passes */
   if (!state->error) {
      GLboolean progress;
      do {
         progress = do_optimization_passes(shader->ir, &argv[optind],
                                           argc - optind, quiet != 0);
      } while (loop && progress);
   }

   /* Print out the resulting IR */
   if (!state->error) {
      if (!quiet) {
         printf("*** resulting IR:\n");
      }
      _mesa_print_ir(shader->ir, state);
      if (!quiet) {
         printf("\n--\n");
      }
   }

   if (state->error) {
      printf("*** error(s) occurred:\n");
      printf("%s\n", state->info_log);
      printf("--\n");
   }

   ralloc_free(state);
   ralloc_free(shader);

   return state->error;
}


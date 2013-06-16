/*
 * Copyright © 2008, 2009 Intel Corporation
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
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>

extern "C" {
#include "main/core.h" /* for struct gl_context */
}

#include "ralloc.h"
#include "ast.h"
#include "glsl_parser_extras.h"
#include "glsl_parser.h"
#include "ir_optimization.h"
#include "loop_analysis.h"

_mesa_glsl_parse_state::_mesa_glsl_parse_state(struct gl_context *ctx,
					       GLenum target, void *mem_ctx)
{
   switch (target) {
   case GL_VERTEX_SHADER:   this->target = vertex_shader; break;
   case GL_FRAGMENT_SHADER: this->target = fragment_shader; break;
   case GL_GEOMETRY_SHADER: this->target = geometry_shader; break;
   }

   this->scanner = NULL;
   this->translation_unit.make_empty();
   this->symbols = new(mem_ctx) glsl_symbol_table;
   this->info_log = ralloc_strdup(mem_ctx, "");
   this->error = false;
   this->loop_nesting_ast = NULL;
   this->switch_state.switch_nesting_ast = NULL;

   this->num_builtins_to_link = 0;

   /* Set default language version and extensions */
   this->language_version = 110;
   this->es_shader = false;
   this->ARB_texture_rectangle_enable = true;

   /* OpenGL ES 2.0 has different defaults from desktop GL. */
   if (ctx->API == API_OPENGLES2) {
      this->language_version = 100;
      this->es_shader = true;
      this->ARB_texture_rectangle_enable = false;
   }

   this->extensions = &ctx->Extensions;

   this->Const.MaxLights = ctx->Const.MaxLights;
   this->Const.MaxClipPlanes = ctx->Const.MaxClipPlanes;
   this->Const.MaxTextureUnits = ctx->Const.MaxTextureUnits;
   this->Const.MaxTextureCoords = ctx->Const.MaxTextureCoordUnits;
   this->Const.MaxVertexAttribs = ctx->Const.VertexProgram.MaxAttribs;
   this->Const.MaxVertexUniformComponents = ctx->Const.VertexProgram.MaxUniformComponents;
   this->Const.MaxVaryingFloats = ctx->Const.MaxVarying * 4;
   this->Const.MaxVertexTextureImageUnits = ctx->Const.MaxVertexTextureImageUnits;
   this->Const.MaxCombinedTextureImageUnits = ctx->Const.MaxCombinedTextureImageUnits;
   this->Const.MaxTextureImageUnits = ctx->Const.MaxTextureImageUnits;
   this->Const.MaxFragmentUniformComponents = ctx->Const.FragmentProgram.MaxUniformComponents;

   this->Const.MaxDrawBuffers = ctx->Const.MaxDrawBuffers;

   /* Note: Once the OpenGL 3.0 'forward compatible' context or the OpenGL 3.2
    * Core context is supported, this logic will need change.  Older versions of
    * GLSL are no longer supported outside the compatibility contexts of 3.x.
    */
   this->Const.GLSL_100ES = (ctx->API == API_OPENGLES2)
      || ctx->Extensions.ARB_ES2_compatibility;
   this->Const.GLSL_110 = (ctx->API == API_OPENGL);
   this->Const.GLSL_120 = (ctx->API == API_OPENGL)
      && (ctx->Const.GLSLVersion >= 120);
   this->Const.GLSL_130 = (ctx->API == API_OPENGL)
      && (ctx->Const.GLSLVersion >= 130);

   const unsigned lowest_version =
      (ctx->API == API_OPENGLES2) || ctx->Extensions.ARB_ES2_compatibility
      ? 100 : 110;
   const unsigned highest_version =
      (ctx->API == API_OPENGL) ? ctx->Const.GLSLVersion : 100;
   char *supported = ralloc_strdup(this, "");

   for (unsigned ver = lowest_version; ver <= highest_version; ver += 10) {
      const char *const prefix = (ver == lowest_version)
	 ? ""
	 : ((ver == highest_version) ? ", and " : ", ");

      ralloc_asprintf_append(& supported, "%s%d.%02d%s",
			     prefix,
			     ver / 100, ver % 100,
			     (ver == 100) ? " ES" : "");
   }

   this->supported_version_string = supported;

   if (ctx->Const.ForceGLSLExtensionsWarn)
      _mesa_glsl_process_extension("all", NULL, "warn", NULL, this);
}

const char *
_mesa_glsl_shader_target_name(enum _mesa_glsl_parser_targets target)
{
   switch (target) {
   case vertex_shader:   return "vertex";
   case fragment_shader: return "fragment";
   case geometry_shader: return "geometry";
   }

   assert(!"Should not get here.");
   return "unknown";
}


void
_mesa_glsl_error(YYLTYPE *locp, _mesa_glsl_parse_state *state,
		 const char *fmt, ...)
{
   va_list ap;

   state->error = true;

   assert(state->info_log != NULL);
   ralloc_asprintf_append(&state->info_log, "%u:%u(%u): error: ",
					    locp->source,
					    locp->first_line,
					    locp->first_column);
   va_start(ap, fmt);
   ralloc_vasprintf_append(&state->info_log, fmt, ap);
   va_end(ap);
   ralloc_strcat(&state->info_log, "\n");
}


void
_mesa_glsl_warning(const YYLTYPE *locp, _mesa_glsl_parse_state *state,
		   const char *fmt, ...)
{
   va_list ap;

   assert(state->info_log != NULL);
   ralloc_asprintf_append(&state->info_log, "%u:%u(%u): warning: ",
					    locp->source,
					    locp->first_line,
					    locp->first_column);
   va_start(ap, fmt);
   ralloc_vasprintf_append(&state->info_log, fmt, ap);
   va_end(ap);
   ralloc_strcat(&state->info_log, "\n");
}


/**
 * Enum representing the possible behaviors that can be specified in
 * an #extension directive.
 */
enum ext_behavior {
   extension_disable,
   extension_enable,
   extension_require,
   extension_warn
};

/**
 * Element type for _mesa_glsl_supported_extensions
 */
struct _mesa_glsl_extension {
   /**
    * Name of the extension when referred to in a GLSL extension
    * statement
    */
   const char *name;

   /** True if this extension is available to vertex shaders */
   bool avail_in_VS;

   /** True if this extension is available to geometry shaders */
   bool avail_in_GS;

   /** True if this extension is available to fragment shaders */
   bool avail_in_FS;

   /** True if this extension is available to desktop GL shaders */
   bool avail_in_GL;

   /** True if this extension is available to GLES shaders */
   bool avail_in_ES;

   /**
    * Flag in the gl_extensions struct indicating whether this
    * extension is supported by the driver, or
    * &gl_extensions::dummy_true if supported by all drivers.
    *
    * Note: the type (GLboolean gl_extensions::*) is a "pointer to
    * member" type, the type-safe alternative to the "offsetof" macro.
    * In a nutshell:
    *
    * - foo bar::* p declares p to be an "offset" to a field of type
    *   foo that exists within struct bar
    * - &bar::baz computes the "offset" of field baz within struct bar
    * - x.*p accesses the field of x that exists at "offset" p
    * - x->*p is equivalent to (*x).*p
    */
   const GLboolean gl_extensions::* supported_flag;

   /**
    * Flag in the _mesa_glsl_parse_state struct that should be set
    * when this extension is enabled.
    *
    * See note in _mesa_glsl_extension::supported_flag about "pointer
    * to member" types.
    */
   bool _mesa_glsl_parse_state::* enable_flag;

   /**
    * Flag in the _mesa_glsl_parse_state struct that should be set
    * when the shader requests "warn" behavior for this extension.
    *
    * See note in _mesa_glsl_extension::supported_flag about "pointer
    * to member" types.
    */
   bool _mesa_glsl_parse_state::* warn_flag;


   bool compatible_with_state(const _mesa_glsl_parse_state *state) const;
   void set_flags(_mesa_glsl_parse_state *state, ext_behavior behavior) const;
};

#define EXT(NAME, VS, GS, FS, GL, ES, SUPPORTED_FLAG)                   \
   { "GL_" #NAME, VS, GS, FS, GL, ES, &gl_extensions::SUPPORTED_FLAG,   \
         &_mesa_glsl_parse_state::NAME##_enable,                        \
         &_mesa_glsl_parse_state::NAME##_warn }

/**
 * Table of extensions that can be enabled/disabled within a shader,
 * and the conditions under which they are supported.
 */
static const _mesa_glsl_extension _mesa_glsl_supported_extensions[] = {
   /*                                  target availability  API availability */
   /* name                             VS     GS     FS     GL     ES         supported flag */
   EXT(ARB_conservative_depth,         false, false, true,  true,  false,     ARB_conservative_depth),
   EXT(ARB_draw_buffers,               false, false, true,  true,  false,     dummy_true),
   EXT(ARB_draw_instanced,             true,  false, false, true,  false,     ARB_draw_instanced),
   EXT(ARB_explicit_attrib_location,   true,  false, true,  true,  false,     ARB_explicit_attrib_location),
   EXT(ARB_fragment_coord_conventions, true,  false, true,  true,  false,     ARB_fragment_coord_conventions),
   EXT(ARB_texture_rectangle,          true,  false, true,  true,  false,     dummy_true),
   EXT(EXT_texture_array,              true,  false, true,  true,  false,     EXT_texture_array),
   EXT(ARB_shader_texture_lod,         true,  false, true,  true,  false,     ARB_shader_texture_lod),
   EXT(ARB_shader_stencil_export,      false, false, true,  true,  false,     ARB_shader_stencil_export),
   EXT(AMD_conservative_depth,         false, false, true,  true,  false,     ARB_conservative_depth),
   EXT(AMD_shader_stencil_export,      false, false, true,  true,  false,     ARB_shader_stencil_export),
   EXT(OES_texture_3D,                 true,  false, true,  false, true,      EXT_texture3D),
   EXT(OES_EGL_image_external,         true,  false, true,  false, true,      OES_EGL_image_external),
};

#undef EXT


/**
 * Determine whether a given extension is compatible with the target,
 * API, and extension information in the current parser state.
 */
bool _mesa_glsl_extension::compatible_with_state(const _mesa_glsl_parse_state *
                                                 state) const
{
   /* Check that this extension matches the type of shader we are
    * compiling to.
    */
   switch (state->target) {
   case vertex_shader:
      if (!this->avail_in_VS) {
         return false;
      }
      break;
   case geometry_shader:
      if (!this->avail_in_GS) {
         return false;
      }
      break;
   case fragment_shader:
      if (!this->avail_in_FS) {
         return false;
      }
      break;
   default:
      assert (!"Unrecognized shader target");
      return false;
   }

   /* Check that this extension matches whether we are compiling
    * for desktop GL or GLES.
    */
   if (state->es_shader) {
      if (!this->avail_in_ES) return false;
   } else {
      if (!this->avail_in_GL) return false;
   }

   /* Check that this extension is supported by the OpenGL
    * implementation.
    *
    * Note: the ->* operator indexes into state->extensions by the
    * offset this->supported_flag.  See
    * _mesa_glsl_extension::supported_flag for more info.
    */
   return state->extensions->*(this->supported_flag);
}

/**
 * Set the appropriate flags in the parser state to establish the
 * given behavior for this extension.
 */
void _mesa_glsl_extension::set_flags(_mesa_glsl_parse_state *state,
                                     ext_behavior behavior) const
{
   /* Note: the ->* operator indexes into state by the
    * offsets this->enable_flag and this->warn_flag.  See
    * _mesa_glsl_extension::supported_flag for more info.
    */
   state->*(this->enable_flag) = (behavior != extension_disable);
   state->*(this->warn_flag)   = (behavior == extension_warn);
}

/**
 * Find an extension by name in _mesa_glsl_supported_extensions.  If
 * the name is not found, return NULL.
 */
static const _mesa_glsl_extension *find_extension(const char *name)
{
   for (unsigned i = 0; i < Elements(_mesa_glsl_supported_extensions); ++i) {
      if (strcmp(name, _mesa_glsl_supported_extensions[i].name) == 0) {
         return &_mesa_glsl_supported_extensions[i];
      }
   }
   return NULL;
}


bool
_mesa_glsl_process_extension(const char *name, YYLTYPE *name_locp,
			     const char *behavior_string, YYLTYPE *behavior_locp,
			     _mesa_glsl_parse_state *state)
{
   ext_behavior behavior;
   if (strcmp(behavior_string, "warn") == 0) {
      behavior = extension_warn;
   } else if (strcmp(behavior_string, "require") == 0) {
      behavior = extension_require;
   } else if (strcmp(behavior_string, "enable") == 0) {
      behavior = extension_enable;
   } else if (strcmp(behavior_string, "disable") == 0) {
      behavior = extension_disable;
   } else {
      _mesa_glsl_error(behavior_locp, state,
		       "Unknown extension behavior `%s'",
		       behavior_string);
      return false;
   }

   if (strcmp(name, "all") == 0) {
      if ((behavior == extension_enable) || (behavior == extension_require)) {
	 _mesa_glsl_error(name_locp, state, "Cannot %s all extensions",
			  (behavior == extension_enable)
			  ? "enable" : "require");
	 return false;
      } else {
         for (unsigned i = 0;
              i < Elements(_mesa_glsl_supported_extensions); ++i) {
            const _mesa_glsl_extension *extension
               = &_mesa_glsl_supported_extensions[i];
            if (extension->compatible_with_state(state)) {
               _mesa_glsl_supported_extensions[i].set_flags(state, behavior);
            }
         }
      }
   } else {
      const _mesa_glsl_extension *extension = find_extension(name);
      if (extension && extension->compatible_with_state(state)) {
         extension->set_flags(state, behavior);
      } else {
         static const char *const fmt = "extension `%s' unsupported in %s shader";

         if (behavior == extension_require) {
            _mesa_glsl_error(name_locp, state, fmt,
                             name, _mesa_glsl_shader_target_name(state->target));
            return false;
         } else {
            _mesa_glsl_warning(name_locp, state, fmt,
                               name, _mesa_glsl_shader_target_name(state->target));
         }
      }
   }

   return true;
}

void
_mesa_ast_type_qualifier_print(const struct ast_type_qualifier *q)
{
   if (q->flags.q.constant)
      printf("const ");

   if (q->flags.q.invariant)
      printf("invariant ");

   if (q->flags.q.attribute)
      printf("attribute ");

   if (q->flags.q.varying)
      printf("varying ");

   if (q->flags.q.in && q->flags.q.out)
      printf("inout ");
   else {
      if (q->flags.q.in)
	 printf("in ");

      if (q->flags.q.out)
	 printf("out ");
   }

   if (q->flags.q.centroid)
      printf("centroid ");
   if (q->flags.q.uniform)
      printf("uniform ");
   if (q->flags.q.smooth)
      printf("smooth ");
   if (q->flags.q.flat)
      printf("flat ");
   if (q->flags.q.noperspective)
      printf("noperspective ");
}


void
ast_node::print(void) const
{
   printf("unhandled node ");
}


ast_node::ast_node(void)
{
   this->location.source = 0;
   this->location.line = 0;
   this->location.column = 0;
}


static void
ast_opt_array_size_print(bool is_array, const ast_expression *array_size)
{
   if (is_array) {
      printf("[ ");

      if (array_size)
	 array_size->print();

      printf("] ");
   }
}


void
ast_compound_statement::print(void) const
{
   printf("{\n");
   
   foreach_list_const(n, &this->statements) {
      ast_node *ast = exec_node_data(ast_node, n, link);
      ast->print();
   }

   printf("}\n");
}


ast_compound_statement::ast_compound_statement(int new_scope,
					       ast_node *statements)
{
   this->new_scope = new_scope;

   if (statements != NULL) {
      this->statements.push_degenerate_list_at_head(&statements->link);
   }
}


void
ast_expression::print(void) const
{
   switch (oper) {
   case ast_assign:
   case ast_mul_assign:
   case ast_div_assign:
   case ast_mod_assign:
   case ast_add_assign:
   case ast_sub_assign:
   case ast_ls_assign:
   case ast_rs_assign:
   case ast_and_assign:
   case ast_xor_assign:
   case ast_or_assign:
      subexpressions[0]->print();
      printf("%s ", operator_string(oper));
      subexpressions[1]->print();
      break;

   case ast_field_selection:
      subexpressions[0]->print();
      printf(". %s ", primary_expression.identifier);
      break;

   case ast_plus:
   case ast_neg:
   case ast_bit_not:
   case ast_logic_not:
   case ast_pre_inc:
   case ast_pre_dec:
      printf("%s ", operator_string(oper));
      subexpressions[0]->print();
      break;

   case ast_post_inc:
   case ast_post_dec:
      subexpressions[0]->print();
      printf("%s ", operator_string(oper));
      break;

   case ast_conditional:
      subexpressions[0]->print();
      printf("? ");
      subexpressions[1]->print();
      printf(": ");
      subexpressions[2]->print();
      break;

   case ast_array_index:
      subexpressions[0]->print();
      printf("[ ");
      subexpressions[1]->print();
      printf("] ");
      break;

   case ast_function_call: {
      subexpressions[0]->print();
      printf("( ");

      foreach_list_const (n, &this->expressions) {
	 if (n != this->expressions.get_head())
	    printf(", ");

	 ast_node *ast = exec_node_data(ast_node, n, link);
	 ast->print();
      }

      printf(") ");
      break;
   }

   case ast_identifier:
      printf("%s ", primary_expression.identifier);
      break;

   case ast_int_constant:
      printf("%d ", primary_expression.int_constant);
      break;

   case ast_uint_constant:
      printf("%u ", primary_expression.uint_constant);
      break;

   case ast_float_constant:
      printf("%f ", primary_expression.float_constant);
      break;

   case ast_bool_constant:
      printf("%s ",
	     primary_expression.bool_constant
	     ? "true" : "false");
      break;

   case ast_sequence: {
      printf("( ");
      foreach_list_const(n, & this->expressions) {
	 if (n != this->expressions.get_head())
	    printf(", ");

	 ast_node *ast = exec_node_data(ast_node, n, link);
	 ast->print();
      }
      printf(") ");
      break;
   }

   default:
      assert(0);
      break;
   }
}

ast_expression::ast_expression(int oper,
			       ast_expression *ex0,
			       ast_expression *ex1,
			       ast_expression *ex2)
{
   this->oper = ast_operators(oper);
   this->subexpressions[0] = ex0;
   this->subexpressions[1] = ex1;
   this->subexpressions[2] = ex2;
   this->non_lvalue_description = NULL;
}


void
ast_expression_statement::print(void) const
{
   if (expression)
      expression->print();

   printf("; ");
}


ast_expression_statement::ast_expression_statement(ast_expression *ex) :
   expression(ex)
{
   /* empty */
}


void
ast_function::print(void) const
{
   return_type->print();
   printf(" %s (", identifier);

   foreach_list_const(n, & this->parameters) {
      ast_node *ast = exec_node_data(ast_node, n, link);
      ast->print();
   }

   printf(")");
}


ast_function::ast_function(void)
   : is_definition(false), signature(NULL)
{
   /* empty */
}


void
ast_fully_specified_type::print(void) const
{
   _mesa_ast_type_qualifier_print(& qualifier);
   specifier->print();
}


void
ast_parameter_declarator::print(void) const
{
   type->print();
   if (identifier)
      printf("%s ", identifier);
   ast_opt_array_size_print(is_array, array_size);
}


void
ast_function_definition::print(void) const
{
   prototype->print();
   body->print();
}


void
ast_declaration::print(void) const
{
   printf("%s ", identifier);
   ast_opt_array_size_print(is_array, array_size);

   if (initializer) {
      printf("= ");
      initializer->print();
   }
}


ast_declaration::ast_declaration(char *identifier, int is_array,
				 ast_expression *array_size,
				 ast_expression *initializer)
{
   this->identifier = identifier;
   this->is_array = is_array;
   this->array_size = array_size;
   this->initializer = initializer;
}


void
ast_declarator_list::print(void) const
{
   assert(type || invariant);

   if (type)
      type->print();
   else
      printf("invariant ");

   foreach_list_const (ptr, & this->declarations) {
      if (ptr != this->declarations.get_head())
	 printf(", ");

      ast_node *ast = exec_node_data(ast_node, ptr, link);
      ast->print();
   }

   printf("; ");
}


ast_declarator_list::ast_declarator_list(ast_fully_specified_type *type)
{
   this->type = type;
   this->invariant = false;
}

void
ast_jump_statement::print(void) const
{
   switch (mode) {
   case ast_continue:
      printf("continue; ");
      break;
   case ast_break:
      printf("break; ");
      break;
   case ast_return:
      printf("return ");
      if (opt_return_value)
	 opt_return_value->print();

      printf("; ");
      break;
   case ast_discard:
      printf("discard; ");
      break;
   }
}


ast_jump_statement::ast_jump_statement(int mode, ast_expression *return_value)
{
   this->mode = ast_jump_modes(mode);

   if (mode == ast_return)
      opt_return_value = return_value;
}


void
ast_selection_statement::print(void) const
{
   printf("if ( ");
   condition->print();
   printf(") ");

   then_statement->print();

   if (else_statement) {
      printf("else ");
      else_statement->print();
   }
   
}


ast_selection_statement::ast_selection_statement(ast_expression *condition,
						 ast_node *then_statement,
						 ast_node *else_statement)
{
   this->condition = condition;
   this->then_statement = then_statement;
   this->else_statement = else_statement;
}


void
ast_switch_statement::print(void) const
{
   printf("switch ( ");
   test_expression->print();
   printf(") ");

   body->print();
}


ast_switch_statement::ast_switch_statement(ast_expression *test_expression,
					   ast_node *body)
{
   this->test_expression = test_expression;
   this->body = body;
}


void
ast_switch_body::print(void) const
{
   printf("{\n");
   if (stmts != NULL) {
      stmts->print();
   }
   printf("}\n");
}


ast_switch_body::ast_switch_body(ast_case_statement_list *stmts)
{
   this->stmts = stmts;
}


void ast_case_label::print(void) const
{
   if (test_value != NULL) {
      printf("case ");
      test_value->print();
      printf(": ");
   } else {
      printf("default: ");
   }
}


ast_case_label::ast_case_label(ast_expression *test_value)
{
   this->test_value = test_value;
}


void ast_case_label_list::print(void) const
{
   foreach_list_const(n, & this->labels) {
      ast_node *ast = exec_node_data(ast_node, n, link);
      ast->print();
   }
   printf("\n");
}


ast_case_label_list::ast_case_label_list(void)
{
}


void ast_case_statement::print(void) const
{
   labels->print();
   foreach_list_const(n, & this->stmts) {
      ast_node *ast = exec_node_data(ast_node, n, link);
      ast->print();
      printf("\n");
   }
}


ast_case_statement::ast_case_statement(ast_case_label_list *labels)
{
   this->labels = labels;
}


void ast_case_statement_list::print(void) const
{
   foreach_list_const(n, & this->cases) {
      ast_node *ast = exec_node_data(ast_node, n, link);
      ast->print();
   }
}


ast_case_statement_list::ast_case_statement_list(void)
{
}


void
ast_iteration_statement::print(void) const
{
   switch (mode) {
   case ast_for:
      printf("for( ");
      if (init_statement)
	 init_statement->print();
      printf("; ");

      if (condition)
	 condition->print();
      printf("; ");

      if (rest_expression)
	 rest_expression->print();
      printf(") ");

      body->print();
      break;

   case ast_while:
      printf("while ( ");
      if (condition)
	 condition->print();
      printf(") ");
      body->print();
      break;

   case ast_do_while:
      printf("do ");
      body->print();
      printf("while ( ");
      if (condition)
	 condition->print();
      printf("); ");
      break;
   }
}


ast_iteration_statement::ast_iteration_statement(int mode,
						 ast_node *init,
						 ast_node *condition,
						 ast_expression *rest_expression,
						 ast_node *body)
{
   this->mode = ast_iteration_modes(mode);
   this->init_statement = init;
   this->condition = condition;
   this->rest_expression = rest_expression;
   this->body = body;
}


void
ast_struct_specifier::print(void) const
{
   printf("struct %s { ", name);
   foreach_list_const(n, &this->declarations) {
      ast_node *ast = exec_node_data(ast_node, n, link);
      ast->print();
   }
   printf("} ");
}


ast_struct_specifier::ast_struct_specifier(char *identifier,
					   ast_node *declarator_list)
{
   if (identifier == NULL) {
      static unsigned anon_count = 1;
      identifier = ralloc_asprintf(this, "#anon_struct_%04x", anon_count);
      anon_count++;
   }
   name = identifier;
   this->declarations.push_degenerate_list_at_head(&declarator_list->link);
}

/**
 * Do the set of common optimizations passes
 *
 * \param ir                          List of instructions to be optimized
 * \param linked                      Is the shader linked?  This enables
 *                                    optimizations passes that remove code at
 *                                    global scope and could cause linking to
 *                                    fail.
 * \param uniform_locations_assigned  Have locations already been assigned for
 *                                    uniforms?  This prevents the declarations
 *                                    of unused uniforms from being removed.
 *                                    The setting of this flag only matters if
 *                                    \c linked is \c true.
 * \param max_unroll_iterations       Maximum number of loop iterations to be
 *                                    unrolled.  Setting to 0 forces all loops
 *                                    to be unrolled.
 */
bool
do_common_optimization(exec_list *ir, bool linked,
		       bool uniform_locations_assigned,
		       unsigned max_unroll_iterations)
{
   GLboolean progress = GL_FALSE;

   progress = lower_instructions(ir, SUB_TO_ADD_NEG) || progress;

   if (linked) {
      progress = do_function_inlining(ir) || progress;
      progress = do_dead_functions(ir) || progress;
      progress = do_structure_splitting(ir) || progress;
   }
   progress = do_if_simplification(ir) || progress;
   progress = do_discard_simplification(ir) || progress;
   progress = do_copy_propagation(ir) || progress;
   progress = do_copy_propagation_elements(ir) || progress;
   if (linked)
      progress = do_dead_code(ir, uniform_locations_assigned) || progress;
   else
      progress = do_dead_code_unlinked(ir) || progress;
   progress = do_dead_code_local(ir) || progress;
   progress = do_tree_grafting(ir) || progress;
   progress = do_constant_propagation(ir) || progress;
   if (linked)
      progress = do_constant_variable(ir) || progress;
   else
      progress = do_constant_variable_unlinked(ir) || progress;
   progress = do_constant_folding(ir) || progress;
   progress = do_algebraic(ir) || progress;
   progress = do_lower_jumps(ir) || progress;
   progress = do_vec_index_to_swizzle(ir) || progress;
   progress = do_swizzle_swizzle(ir) || progress;
   progress = do_noop_swizzle(ir) || progress;

   progress = optimize_redundant_jumps(ir) || progress;

   loop_state *ls = analyze_loop_variables(ir);
   if (ls->loop_found) {
      progress = set_loop_controls(ir, ls) || progress;
      progress = unroll_loops(ir, ls, max_unroll_iterations) || progress;
   }
   delete ls;

   return progress;
}

extern "C" {

/**
 * To be called at GL teardown time, this frees compiler datastructures.
 *
 * After calling this, any previously compiled shaders and shader
 * programs would be invalid.  So this should happen at approximately
 * program exit.
 */
void
_mesa_destroy_shader_compiler(void)
{
   _mesa_destroy_shader_compiler_caches();

   _mesa_glsl_release_types();
}

/**
 * Releases compiler caches to trade off performance for memory.
 *
 * Intended to be used with glReleaseShaderCompiler().
 */
void
_mesa_destroy_shader_compiler_caches(void)
{
   _mesa_glsl_release_functions();
}

}

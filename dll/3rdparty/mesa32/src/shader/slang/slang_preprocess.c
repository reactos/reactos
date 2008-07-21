/*
 * Mesa 3-D graphics library
 * Version:  6.5.2
 *
 * Copyright (C) 2005-2006  Brian Paul   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/**
 * \file slang_preprocess.c
 * slang preprocessor
 * \author Michal Krol
 */

#include "imports.h"
#include "grammar_mesa.h"
#include "slang_preprocess.h"

LONGSTRING static const char *slang_pp_directives_syn =
#include "library/slang_pp_directives_syn.h"
;

LONGSTRING static const char *slang_pp_expression_syn =
#include "library/slang_pp_expression_syn.h"
;

LONGSTRING static const char *slang_pp_version_syn =
#include "library/slang_pp_version_syn.h"
;

static GLvoid
grammar_error_to_log (slang_info_log *log)
{
   char buf[1024];
   GLint pos;

   grammar_get_last_error ((byte *) (buf), sizeof (buf), &pos);
   slang_info_log_error (log, buf);
}

GLboolean
_slang_preprocess_version (const char *text, GLuint *version, GLuint *eaten, slang_info_log *log)
{
   grammar id;
   byte *prod, *I;
   unsigned int size;

   id = grammar_load_from_text ((const byte *) (slang_pp_version_syn));
   if (id == 0) {
      grammar_error_to_log (log);
      return GL_FALSE;
   }

   if (!grammar_fast_check (id, (const byte *) (text), &prod, &size, 8)) {
      grammar_error_to_log (log);
      grammar_destroy (id);
      return GL_FALSE;
   }

   /* there can be multiple #version directives - grab the last one */
   I = &prod[size - 6];
   *version = (GLuint) (I[0]) + (GLuint) (I[1]) * 100;
   *eaten = (GLuint) (I[2]) + ((GLuint) (I[3]) << 8) + ((GLuint) (I[4]) << 16) + ((GLuint) (I[5]) << 24);

   grammar_destroy (id);
   grammar_alloc_free (prod);
   return GL_TRUE;
}

/*
 * The preprocessor does the following work.
 * 1. Remove comments. Each comment block is replaced with a single space and if the
 *    block contains new-lines, they are preserved. This ensures that line numbers
 *    stay the same and if a comment block delimits two tokens, the are delitmited
 *    by the space after comment removal.
 * 2. Remove preprocessor directives from the source string, checking their syntax and
 *    executing them if appropriate. Again, new-lines are preserved.
 * 3. Expand macros.
 * 4. Tokenize the source string by ensuring there is at least one space between every
 *    two adjacent tokens.
 */

#define PP_ANNOTATE 0

static GLvoid
pp_annotate (slang_string *output, const char *fmt, ...)
{
#if PP_ANNOTATE
   va_list va;
   char buffer[1024];

   va_start (va, fmt);
   _mesa_vsprintf (buffer, fmt, va);
   va_end (va);
   slang_string_pushs (output, buffer, _mesa_strlen (buffer));
#else
   (GLvoid) (output);
   (GLvoid) (fmt);
#endif
}

 /*
 * The expression is executed on a fixed-sized stack. The PUSH macro makes a runtime
 * check if the stack is not overflown by too complex expressions. In that situation the
 * GLSL preprocessor should report internal compiler error.
 * The BINARYDIV makes a runtime check if the divider is not 0. If it is, it reports
 * compilation error.
 */

#define EXECUTION_STACK_SIZE 1024

#define PUSH(x)\
   do {\
      if (sp == 0) {\
         slang_info_log_error (elog, "internal compiler error: preprocessor execution stack overflow.");\
         return GL_FALSE;\
      }\
      stack[--sp] = x;\
   } while (GL_FALSE)

#define POP(x)\
   do {\
      assert (sp < EXECUTION_STACK_SIZE);\
      x = stack[sp++];\
   } while (GL_FALSE)

#define BINARY(op)\
   do {\
      GLint a, b;\
      POP(b);\
      POP(a);\
      PUSH(a op b);\
   } while (GL_FALSE)

#define BINARYDIV(op)\
   do {\
      GLint a, b;\
      POP(b);\
      POP(a);\
      if (b == 0) {\
         slang_info_log_error (elog, "division by zero in preprocessor expression.");\
         return GL_FALSE;\
      }\
      PUSH(a op b);\
   } while (GL_FALSE)

#define UNARY(op)\
   do {\
      GLint a;\
      POP(a);\
      PUSH(op a);\
   } while (GL_FALSE)

#define OP_END          0
#define OP_PUSHINT      1
#define OP_LOGICALOR    2
#define OP_LOGICALAND   3
#define OP_OR           4
#define OP_XOR          5
#define OP_AND          6
#define OP_EQUAL        7
#define OP_NOTEQUAL     8
#define OP_LESSEQUAL    9
#define OP_GREATEREQUAL 10
#define OP_LESS         11
#define OP_GREATER      12
#define OP_LEFTSHIFT    13
#define OP_RIGHTSHIFT   14
#define OP_ADD          15
#define OP_SUBTRACT     16
#define OP_MULTIPLY     17
#define OP_DIVIDE       18
#define OP_MODULUS      19
#define OP_PLUS         20
#define OP_MINUS        21
#define OP_NEGATE       22
#define OP_COMPLEMENT   23

static GLboolean
execute_expression (slang_string *output, const byte *code, GLuint *pi, GLint *result,
                    slang_info_log *elog)
{
   GLuint i = *pi;
   GLint stack[EXECUTION_STACK_SIZE];
   GLuint sp = EXECUTION_STACK_SIZE;

   while (code[i] != OP_END) {
      switch (code[i++]) {
         case OP_PUSHINT:
            i++;
            PUSH(_mesa_atoi ((const char *) (&code[i])));
            i += _mesa_strlen ((const char *) (&code[i])) + 1;
            break;
         case OP_LOGICALOR:
            BINARY(||);
            break;
         case OP_LOGICALAND:
            BINARY(&&);
            break;
         case OP_OR:
            BINARY(|);
            break;
         case OP_XOR:
            BINARY(^);
            break;
         case OP_AND:
            BINARY(&);
            break;
         case OP_EQUAL:
            BINARY(==);
            break;
         case OP_NOTEQUAL:
            BINARY(!=);
            break;
         case OP_LESSEQUAL:
            BINARY(<=);
            break;
         case OP_GREATEREQUAL:
            BINARY(>=);
            break;
         case OP_LESS:
            BINARY(<);
            break;
         case OP_GREATER:
            BINARY(>);
            break;
         case OP_LEFTSHIFT:
            BINARY(<<);
            break;
         case OP_RIGHTSHIFT:
            BINARY(>>);
            break;
         case OP_ADD:
            BINARY(+);
            break;
         case OP_SUBTRACT:
            BINARY(-);
            break;
         case OP_MULTIPLY:
            BINARY(*);
            break;
         case OP_DIVIDE:
            BINARYDIV(/);
            break;
         case OP_MODULUS:
            BINARYDIV(%);
            break;
         case OP_PLUS:
            UNARY(+);
            break;
         case OP_MINUS:
            UNARY(-);
            break;
         case OP_NEGATE:
            UNARY(!);
            break;
         case OP_COMPLEMENT:
            UNARY(~);
            break;
         default:
            assert (0);
      }
   }

   /* Write-back the index skipping the OP_END. */
   *pi = i + 1;

   /* There should be exactly one value left on the stack. This is our result. */
   POP(*result);
   pp_annotate (output, "%d ", *result);
   assert (sp == EXECUTION_STACK_SIZE);
   return GL_TRUE;
}

/*
 * Function execute_expressions() executes up to 2 expressions. The second expression is there
 * for the #line directive which takes 1 or 2 expressions that indicate line and file numbers.
 * If it fails, it returns 0. If it succeeds, it returns the number of executed expressions.
 */

#define EXP_END        0
#define EXP_EXPRESSION 1

static GLuint
execute_expressions (slang_string *output, grammar eid, const byte *expr, GLint results[2],
                     slang_info_log *elog)
{
   GLint success;
   byte *code;
   GLuint size, count = 0;

   success = grammar_fast_check (eid, expr, &code, &size, 64);
   if (success) {
      GLuint i = 0;

      while (code[i++] == EXP_EXPRESSION) {
         assert (count < 2);

         if (!execute_expression (output, code, &i, &results[count], elog)) {
            count = 0;
            break;
         }
         count++;
      }
      grammar_alloc_free (code);
   }
   else {
      slang_info_log_error (elog, "syntax error in preprocessor expression.");\
   }
   return count;
}

/*
 * The pp_symbol structure is used to hold macro definitions and macro formal parameters. The
 * pp_symbols strcture is a collection of pp_symbol. It is used both for storing macro formal
 * parameters and all global macro definitions. Making this unification wastes some memory,
 * becuse macro formal parameters don't need further lists of symbols. We lose 8 bytes per
 * formal parameter here, but making this we can use the same code to substitute macro parameters
 * as well as macros in the source string.
 */

typedef struct
{
   struct pp_symbol_ *symbols;
   GLuint count;
} pp_symbols;

static GLvoid
pp_symbols_init (pp_symbols *self)
{
   self->symbols = NULL;
   self->count = 0;
}

static GLvoid
pp_symbols_free (pp_symbols *);

typedef struct pp_symbol_
{
   slang_string name;
   slang_string replacement;
   pp_symbols parameters;
} pp_symbol;

static GLvoid
pp_symbol_init (pp_symbol *self)
{
   slang_string_init (&self->name);
   slang_string_init (&self->replacement);
   pp_symbols_init (&self->parameters);
}

static GLvoid
pp_symbol_free (pp_symbol *self)
{
   slang_string_free (&self->name);
   slang_string_free (&self->replacement);
   pp_symbols_free (&self->parameters);
}

static GLvoid
pp_symbol_reset (pp_symbol *self)
{
   /* Leave symbol name intact. */
   slang_string_reset (&self->replacement);
   pp_symbols_free (&self->parameters);
   pp_symbols_init (&self->parameters);
}

static GLvoid
pp_symbols_free (pp_symbols *self)
{
   GLuint i;

   for (i = 0; i < self->count; i++)
      pp_symbol_free (&self->symbols[i]);
   _mesa_free (self->symbols);
}

static pp_symbol *
pp_symbols_push (pp_symbols *self)
{
   self->symbols = (pp_symbol *) (_mesa_realloc (self->symbols, self->count * sizeof (pp_symbol),
                                                 (self->count + 1) * sizeof (pp_symbol)));
   if (self->symbols == NULL)
      return NULL;
   pp_symbol_init (&self->symbols[self->count]);
   return &self->symbols[self->count++];
}

static GLboolean
pp_symbols_erase (pp_symbols *self, pp_symbol *symbol)
{
   assert (symbol >= self->symbols && symbol < self->symbols + self->count);

   self->count--;
   pp_symbol_free (symbol);
   if (symbol < self->symbols + self->count)
      _mesa_memcpy (symbol, symbol + 1, sizeof (pp_symbol) * (self->symbols + self->count - symbol));
   self->symbols = (pp_symbol *) (_mesa_realloc (self->symbols, (self->count + 1) * sizeof (pp_symbol),
                                                 self->count * sizeof (pp_symbol)));
   return self->symbols != NULL;
}

static pp_symbol *
pp_symbols_find (pp_symbols *self, const char *name)
{
   GLuint i;

   for (i = 0; i < self->count; i++)
      if (_mesa_strcmp (name, slang_string_cstr (&self->symbols[i].name)) == 0)
         return &self->symbols[i];
   return NULL;
}

/*
 * The condition context of a single #if/#else/#endif level. Those can be nested, so there
 * is a stack of condition contexts.
 * There is a special global context on the bottom of the stack. It is there to simplify
 * context handling.
 */

typedef struct
{
   GLboolean current;         /* The condition value of this level. */
   GLboolean effective;       /* The effective product of current condition, outer level conditions
                               * and position within #if-#else-#endif sections. */
   GLboolean else_allowed;    /* TRUE if in #if-#else section, FALSE if in #else-#endif section
                               * and for global context. */
   GLboolean endif_required;  /* FALSE for global context only. */
} pp_cond_ctx;

/* Should be enuff. */
#define CONDITION_STACK_SIZE 64

typedef struct
{
   pp_cond_ctx stack[CONDITION_STACK_SIZE];
   pp_cond_ctx *top;
} pp_cond_stack;

static GLboolean
pp_cond_stack_push (pp_cond_stack *self, slang_info_log *elog)
{
   if (self->top == self->stack) {
      slang_info_log_error (elog, "internal compiler error: preprocessor condition stack overflow.");
      return GL_FALSE;
   }
   self->top--;
   return GL_TRUE;
}

static GLvoid
pp_cond_stack_reevaluate (pp_cond_stack *self)
{
   /* There must be at least 2 conditions on the stack - one global and one being evaluated. */
   assert (self->top <= &self->stack[CONDITION_STACK_SIZE - 2]);

   self->top->effective = self->top->current && self->top[1].effective;
}

/*
 * Extension enables through #extension directive.
 * NOTE: Currently, only enable/disable state is stored.
 */

typedef struct
{
   GLboolean MESA_shader_debug;        /* GL_MESA_shader_debug enable */
} pp_ext;

/*
 * Disable all extensions. Called at startup and on #extension all: disable.
 */
static GLvoid
pp_ext_disable_all (pp_ext *self)
{
   self->MESA_shader_debug = GL_FALSE;
}

static GLvoid
pp_ext_init (pp_ext *self)
{
   pp_ext_disable_all (self);
   /* Other initialization code goes here. */
}

static GLboolean
pp_ext_set (pp_ext *self, const char *name, GLboolean enable)
{
   if (_mesa_strcmp (name, "MESA_shader_debug") == 0)
      self->MESA_shader_debug = enable;
   /* Next extension name tests go here. */
   else
      return GL_FALSE;
   return GL_TRUE;
}

/*
 * The state of preprocessor: current line, file and version number, list of all defined macros
 * and the #if/#endif context.
 */

typedef struct
{
   GLint line;
   GLint file;
   GLint version;
   pp_symbols symbols;
   pp_ext ext;
   slang_info_log *elog;
   pp_cond_stack cond;
} pp_state;

static GLvoid
pp_state_init (pp_state *self, slang_info_log *elog)
{
   self->line = 0;
   self->file = 1;
   self->version = 110;
   pp_symbols_init (&self->symbols);
   pp_ext_init (&self->ext);
   self->elog = elog;

   /* Initialize condition stack and create the global context. */
   self->cond.top = &self->cond.stack[CONDITION_STACK_SIZE - 1];
   self->cond.top->current = GL_TRUE;
   self->cond.top->effective = GL_TRUE;
   self->cond.top->else_allowed = GL_FALSE;
   self->cond.top->endif_required = GL_FALSE;
}

static GLvoid
pp_state_free (pp_state *self)
{
   pp_symbols_free (&self->symbols);
}

#define IS_FIRST_ID_CHAR(x) (((x) >= 'a' && (x) <= 'z') || ((x) >= 'A' && (x) <= 'Z') || (x) == '_')
#define IS_NEXT_ID_CHAR(x) (IS_FIRST_ID_CHAR(x) || ((x) >= '0' && (x) <= '9'))
#define IS_WHITE(x) ((x) == ' ' || (x) == '\n')
#define IS_NULL(x) ((x) == '\0')

#define SKIP_WHITE(x) do { while (IS_WHITE(*(x))) (x)++; } while (GL_FALSE)

typedef struct
{
   slang_string *output;
   const char *input;
   pp_state *state;
} expand_state;

static GLboolean
expand_defined (expand_state *e, slang_string *buffer)
{
   GLboolean in_paren = GL_FALSE;
   const char *id;

   /* Parse the optional opening parenthesis. */
   SKIP_WHITE(e->input);
   if (*e->input == '(') {
      e->input++;
      in_paren = GL_TRUE;
      SKIP_WHITE(e->input);
   }

   /* Parse operand. */
   if (!IS_FIRST_ID_CHAR(*e->input)) {
      slang_info_log_error (e->state->elog,
                            "preprocess error: identifier expected after operator 'defined'.");
      return GL_FALSE;
   }
   slang_string_reset (buffer);
   slang_string_pushc (buffer, *e->input++);
   while (IS_NEXT_ID_CHAR(*e->input))
      slang_string_pushc (buffer, *e->input++);
   id = slang_string_cstr (buffer);

   /* Check if the operand is defined. Output 1 if it is defined, output 0 if not. */
   if (pp_symbols_find (&e->state->symbols, id) == NULL)
      slang_string_pushs (e->output, " 0 ", 3);
   else
      slang_string_pushs (e->output, " 1 ", 3);

   /* Parse the closing parentehesis if the opening one was there. */
   if (in_paren) {
      SKIP_WHITE(e->input);
      if (*e->input != ')') {
         slang_info_log_error (e->state->elog, "preprocess error: ')' expected.");
         return GL_FALSE;
      }
      e->input++;
      SKIP_WHITE(e->input);
   }
   return GL_TRUE;
}

static GLboolean
expand (expand_state *, pp_symbols *);

static GLboolean
expand_symbol (expand_state *e, pp_symbol *symbol)
{
   expand_state es;

   /* If the macro has some parameters, we need to parse them. */
   if (symbol->parameters.count != 0) {
      GLuint i;

      /* Parse the opening parenthesis. */
      SKIP_WHITE(e->input);
      if (*e->input != '(') {
         slang_info_log_error (e->state->elog, "preprocess error: '(' expected.");
         return GL_FALSE;
      }
      e->input++;
      SKIP_WHITE(e->input);

      /* Parse macro actual parameters. This can be anything, separated by a colon.
       * TODO: What about nested/grouped parameters by parenthesis? */
      for (i = 0; i < symbol->parameters.count; i++) {
         if (*e->input == ')') {
            slang_info_log_error (e->state->elog, "preprocess error: unexpected ')'.");
            return GL_FALSE;
         }

         /* Eat all characters up to the comma or closing parentheses. */
         pp_symbol_reset (&symbol->parameters.symbols[i]);
         while (!IS_NULL(*e->input) && *e->input != ',' && *e->input != ')')
            slang_string_pushc (&symbol->parameters.symbols[i].replacement, *e->input++);

         /* If it was not the last paremeter, skip the comma. Otherwise, skip the
          * closing parentheses. */
         if (i + 1 == symbol->parameters.count) {
            /* This is the last paremeter - skip the closing parentheses. */
            if (*e->input != ')') {
               slang_info_log_error (e->state->elog, "preprocess error: ')' expected.");
               return GL_FALSE;
            }
            e->input++;
            SKIP_WHITE(e->input);
         }
         else {
            /* Skip the separating comma. */
            if (*e->input != ',') {
               slang_info_log_error (e->state->elog, "preprocess error: ',' expected.");
               return GL_FALSE;
            }
            e->input++;
            SKIP_WHITE(e->input);
         }
      }
   }

   /* Expand the macro. Use its parameters as a priority symbol list to expand
    * macro parameters correctly. */
   es.output = e->output;
   es.input = slang_string_cstr (&symbol->replacement);
   es.state = e->state;
   slang_string_pushc (e->output, ' ');
   if (!expand (&es, &symbol->parameters))
      return GL_FALSE;
   slang_string_pushc (e->output, ' ');
   return GL_TRUE;
}

/*
 * Function expand() expands source text from <input> to <output>. The expansion is made using
 * the list passed in <symbols> parameter. It allows us to expand macro formal parameters with
 * actual parameters. The global list of symbols from pp state is used when doing a recursive
 * call of expand().
 */

static GLboolean
expand (expand_state *e, pp_symbols *symbols)
{
   while (!IS_NULL(*e->input)) {
      if (IS_FIRST_ID_CHAR(*e->input)) {
         slang_string buffer;
         const char *id;

         /* Parse the identifier. */
         slang_string_init (&buffer);
         slang_string_pushc (&buffer, *e->input++);
         while (IS_NEXT_ID_CHAR(*e->input))
            slang_string_pushc (&buffer, *e->input++);
         id = slang_string_cstr (&buffer);

         /* Now check if the identifier is special in some way. The "defined" identifier is
          * actually an operator that we must handle here and expand it either to " 0 " or " 1 ".
          * The other identifiers start with "__" and we expand it to appropriate values
          * taken from the preprocessor state. */
         if (_mesa_strcmp (id, "defined") == 0) {
            if (!expand_defined (e, &buffer))
               return GL_FALSE;
         }
         else if (_mesa_strcmp (id, "__LINE__") == 0) {
            slang_string_pushc (e->output, ' ');
            slang_string_pushi (e->output, e->state->line);
            slang_string_pushc (e->output, ' ');
         }
         else if (_mesa_strcmp (id, "__FILE__") == 0) {
            slang_string_pushc (e->output, ' ');
            slang_string_pushi (e->output, e->state->file);
            slang_string_pushc (e->output, ' ');
         }
         else if (_mesa_strcmp (id, "__VERSION__") == 0) {
            slang_string_pushc (e->output, ' ');
            slang_string_pushi (e->output, e->state->version);
            slang_string_pushc (e->output, ' ');
         }
         else {
            pp_symbol *symbol;

            /* The list of symbols from <symbols> take precedence over the list from <state>.
             * Note that in some cases this is the same list so avoid double look-up. */
            symbol = pp_symbols_find (symbols, id);
            if (symbol == NULL && symbols != &e->state->symbols)
               symbol = pp_symbols_find (&e->state->symbols, id);

            /* If the symbol was found, recursively expand its definition. */
            if (symbol != NULL) {
               if (!expand_symbol (e, symbol)) {
                  slang_string_free (&buffer);
                  return GL_FALSE;
               }
            }
            else {
               slang_string_push (e->output, &buffer);
            }
         }
         slang_string_free (&buffer);
      }
      else if (IS_WHITE(*e->input)) {
         slang_string_pushc (e->output, *e->input++);
      }
      else {
         while (!IS_WHITE(*e->input) && !IS_NULL(*e->input) && !IS_FIRST_ID_CHAR(*e->input))
            slang_string_pushc (e->output, *e->input++);
      }
   }
   return GL_TRUE;
}

static GLboolean
parse_if (slang_string *output, const byte *prod, GLuint *pi, GLint *result, pp_state *state,
          grammar eid)
{
   const char *text;
   GLuint len;

   text = (const char *) (&prod[*pi]);
   len = _mesa_strlen (text);

   if (state->cond.top->effective) {
      slang_string expr;
      GLuint count;
      GLint results[2];
      expand_state es;

      /* Expand the expression. */
      slang_string_init (&expr);
      es.output = &expr;
      es.input = text;
      es.state = state;
      if (!expand (&es, &state->symbols))
         return GL_FALSE;

      /* Execute the expression. */
      count = execute_expressions (output, eid, (const byte *) (slang_string_cstr (&expr)),
                                   results, state->elog);
      slang_string_free (&expr);
      if (count != 1)
         return GL_FALSE;
      *result = results[0];
   }
   else {
      /* The directive is dead. */
      *result = 0;
   }

   *pi += len + 1;
   return GL_TRUE;
}

#define ESCAPE_TOKEN 0

#define TOKEN_END       0
#define TOKEN_DEFINE    1
#define TOKEN_UNDEF     2
#define TOKEN_IF        3
#define TOKEN_ELSE      4
#define TOKEN_ELIF      5
#define TOKEN_ENDIF     6
#define TOKEN_ERROR     7
#define TOKEN_PRAGMA    8
#define TOKEN_EXTENSION 9
#define TOKEN_LINE      10

#define PARAM_END       0
#define PARAM_PARAMETER 1

#define BEHAVIOR_REQUIRE 1
#define BEHAVIOR_ENABLE  2
#define BEHAVIOR_WARN    3
#define BEHAVIOR_DISABLE 4

static GLboolean
preprocess_source (slang_string *output, const char *source, grammar pid, grammar eid,
                   slang_info_log *elog)
{
   byte *prod;
   GLuint size, i;
   pp_state state;

   if (!grammar_fast_check (pid, (const byte *) (source), &prod, &size, 65536)) {
      grammar_error_to_log (elog);
      return GL_FALSE;
   }

   pp_state_init (&state, elog);

   i = 0;
   while (i < size) {
      if (prod[i] != ESCAPE_TOKEN) {
         if (state.cond.top->effective) {
            slang_string input;
            expand_state es;

            /* Eat only one line of source code to expand it.
             * FIXME: This approach has one drawback. If a macro with parameters spans across
             *        multiple lines, the preprocessor will raise an error. */
            slang_string_init (&input);
            while (prod[i] != '\0' && prod[i] != '\n')
               slang_string_pushc (&input, prod[i++]);
            if (prod[i] != '\0')
               slang_string_pushc (&input, prod[i++]);

            /* Increment line number. */
            state.line++;

            es.output = output;
            es.input = slang_string_cstr (&input);
            es.state = &state;
            if (!expand (&es, &state.symbols))
               goto error;

            slang_string_free (&input);
         }
         else {
            /* Condition stack is disabled - keep track on line numbers and output only newlines. */
            if (prod[i] == '\n') {
               state.line++;
               /*pp_annotate (output, "%c", prod[i]);*/
            }
            else {
               /*pp_annotate (output, "%c", prod[i]);*/
            }
            i++;
         }
      }
      else {
         const char *id;
         GLuint idlen;

         i++;
         switch (prod[i++]) {

         case TOKEN_END:
            /* End of source string.
               * Check if all #ifs have been terminated by matching #endifs.
               * On condition stack there should be only the global condition context. */
            if (state.cond.top->endif_required) {
               slang_info_log_error (elog, "end of source without matching #endif.");
               return GL_FALSE;
            }
            break;

         case TOKEN_DEFINE:
            {
               pp_symbol *symbol = NULL;

               /* Parse macro name. */
               id = (const char *) (&prod[i]);
               idlen = _mesa_strlen (id);
               if (state.cond.top->effective) {
                  pp_annotate (output, "// #define %s(", id);

                  /* If the symbol is already defined, override it. */
                  symbol = pp_symbols_find (&state.symbols, id);
                  if (symbol == NULL) {
                     symbol = pp_symbols_push (&state.symbols);
                     if (symbol == NULL)
                        goto error;
                     slang_string_pushs (&symbol->name, id, idlen);
                  }
                  else {
                     pp_symbol_reset (symbol);
                  }
               }
               i += idlen + 1;

               /* Parse optional macro parameters. */
               while (prod[i++] != PARAM_END) {
                  if (state.cond.top->effective) {
                     pp_symbol *param;

                     id = (const char *) (&prod[i]);
                     idlen = _mesa_strlen (id);
                     pp_annotate (output, "%s, ", id);
                     param = pp_symbols_push (&symbol->parameters);
                     if (param == NULL)
                        goto error;
                     slang_string_pushs (&param->name, id, idlen);
                  }
                  i += idlen + 1;
               }

               /* Parse macro replacement. */
               id = (const char *) (&prod[i]);
               idlen = _mesa_strlen (id);
               if (state.cond.top->effective) {
                  pp_annotate (output, ") %s", id);
                  slang_string_pushs (&symbol->replacement, id, idlen);
               }
               i += idlen + 1;
            }
            break;

         case TOKEN_UNDEF:
            id = (const char *) (&prod[i]);
            i += _mesa_strlen (id) + 1;
            if (state.cond.top->effective) {
               pp_symbol *symbol;

               pp_annotate (output, "// #undef %s", id);
               /* Try to find symbol with given name and remove it. */
               symbol = pp_symbols_find (&state.symbols, id);
               if (symbol != NULL)
                  if (!pp_symbols_erase (&state.symbols, symbol))
                     goto error;
            }
            break;

         case TOKEN_IF:
            {
               GLint result;

               /* Parse #if expression end execute it. */
               pp_annotate (output, "// #if ");
               if (!parse_if (output, prod, &i, &result, &state, eid))
                  goto error;

               /* Push new condition on the stack. */
               if (!pp_cond_stack_push (&state.cond, state.elog))
                  goto error;
               state.cond.top->current = result ? GL_TRUE : GL_FALSE;
               state.cond.top->else_allowed = GL_TRUE;
               state.cond.top->endif_required = GL_TRUE;
               pp_cond_stack_reevaluate (&state.cond);
            }
            break;

         case TOKEN_ELSE:
            /* Check if #else is alloved here. */
            if (!state.cond.top->else_allowed) {
               slang_info_log_error (elog, "#else without matching #if.");
               goto error;
            }

            /* Negate current condition and reevaluate it. */
            state.cond.top->current = !state.cond.top->current;
            state.cond.top->else_allowed = GL_FALSE;
            pp_cond_stack_reevaluate (&state.cond);
            if (state.cond.top->effective)
               pp_annotate (output, "// #else");
            break;

         case TOKEN_ELIF:
            /* Check if #elif is alloved here. */
            if (!state.cond.top->else_allowed) {
               slang_info_log_error (elog, "#elif without matching #if.");
               goto error;
            }

            /* Negate current condition and reevaluate it. */
            state.cond.top->current = !state.cond.top->current;
            pp_cond_stack_reevaluate (&state.cond);

            if (state.cond.top->effective)
               pp_annotate (output, "// #elif ");

            {
               GLint result;

               /* Parse #elif expression end execute it. */
               if (!parse_if (output, prod, &i, &result, &state, eid))
                  goto error;

               /* Update current condition and reevaluate it. */
               state.cond.top->current = result ? GL_TRUE : GL_FALSE;
               pp_cond_stack_reevaluate (&state.cond);
            }
            break;

         case TOKEN_ENDIF:
            /* Check if #endif is alloved here. */
            if (!state.cond.top->endif_required) {
               slang_info_log_error (elog, "#endif without matching #if.");
               goto error;
            }

            /* Pop the condition off the stack. */
            state.cond.top++;
            if (state.cond.top->effective)
               pp_annotate (output, "// #endif");
            break;

         case TOKEN_EXTENSION:
            /* Parse the extension name. */
            id = (const char *) (&prod[i]);
            i += _mesa_strlen (id) + 1;
            if (state.cond.top->effective)
               pp_annotate (output, "// #extension %s: ", id);

            /* Parse and apply extension behavior. */
            if (state.cond.top->effective) {
               switch (prod[i++]) {

               case BEHAVIOR_REQUIRE:
                  pp_annotate (output, "require");
                  if (!pp_ext_set (&state.ext, id, GL_TRUE)) {
                     if (_mesa_strcmp (id, "all") == 0) {
                        slang_info_log_error (elog, "require: bad behavior for #extension all.");
                        goto error;
                     }
                     else {
                        slang_info_log_error (elog, "%s: required extension is not supported.", id);
                        goto error;
                     }
                  }
                  break;

               case BEHAVIOR_ENABLE:
                  pp_annotate (output, "enable");
                  if (!pp_ext_set (&state.ext, id, GL_TRUE)) {
                     if (_mesa_strcmp (id, "all") == 0) {
                        slang_info_log_error (elog, "enable: bad behavior for #extension all.");
                        goto error;
                     }
                     else {
                        slang_info_log_warning (elog, "%s: enabled extension is not supported.", id);
                     }
                  }
                  break;

               case BEHAVIOR_WARN:
                  pp_annotate (output, "warn");
                  if (!pp_ext_set (&state.ext, id, GL_TRUE)) {
                     if (_mesa_strcmp (id, "all") != 0) {
                        slang_info_log_warning (elog, "%s: enabled extension is not supported.", id);
                     }
                  }
                  break;

               case BEHAVIOR_DISABLE:
                  pp_annotate (output, "disable");
                  if (!pp_ext_set (&state.ext, id, GL_FALSE)) {
                     if (_mesa_strcmp (id, "all") == 0) {
                        pp_ext_disable_all (&state.ext);
                     }
                     else {
                        slang_info_log_warning (elog, "%s: disabled extension is not supported.", id);
                     }
                  }
                  break;

               default:
                  assert (0);
               }
            }
            break;

         case TOKEN_LINE:
            id = (const char *) (&prod[i]);
            i += _mesa_strlen (id) + 1;

            if (state.cond.top->effective) {
               slang_string buffer;
               GLuint count;
               GLint results[2];
               expand_state es;

               slang_string_init (&buffer);
               state.line++;
               es.output = &buffer;
               es.input = id;
               es.state = &state;
               if (!expand (&es, &state.symbols))
                  goto error;

               pp_annotate (output, "// #line ");
               count = execute_expressions (output, eid,
                                             (const byte *) (slang_string_cstr (&buffer)),
                                             results, state.elog);
               slang_string_free (&buffer);
               if (count == 0)
                  goto error;

               state.line = results[0] - 1;
               if (count == 2)
                  state.file = results[1];
            }
            break;
         }
      }
   }

   /* Check for missing #endifs. */
   if (state.cond.top->endif_required) {
      slang_info_log_error (elog, "#endif expected but end of source found.");
      goto error;
   }

   grammar_alloc_free(prod);
   pp_state_free (&state);
   return GL_TRUE;

error:
   grammar_alloc_free(prod);
   pp_state_free (&state);
   return GL_FALSE;
}

GLboolean
_slang_preprocess_directives (slang_string *output, const char *input, slang_info_log *elog)
{
   grammar pid, eid;
   GLboolean success;

   pid = grammar_load_from_text ((const byte *) (slang_pp_directives_syn));
   if (pid == 0) {
      grammar_error_to_log (elog);
      return GL_FALSE;
   }
   eid = grammar_load_from_text ((const byte *) (slang_pp_expression_syn));
   if (eid == 0) {
      grammar_error_to_log (elog);
      grammar_destroy (pid);
      return GL_FALSE;
   }
   success = preprocess_source (output, input, pid, eid, elog);
   grammar_destroy (eid);
   grammar_destroy (pid);
   return success;
}


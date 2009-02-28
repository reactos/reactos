/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 2005-2006  Brian Paul   All Rights Reserved.
 * Copyright (C) 2008 VMware, Inc.  All Rights Reserved.
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
 * \file slang_compile.c
 * slang front-end compiler
 * \author Michal Krol
 */

#include "main/imports.h"
#include "main/context.h"
#include "shader/program.h"
#include "shader/programopt.h"
#include "shader/prog_print.h"
#include "shader/prog_parameter.h"
#include "shader/grammar/grammar_mesa.h"
#include "slang_codegen.h"
#include "slang_compile.h"
#include "slang_preprocess.h"
#include "slang_storage.h"
#include "slang_emit.h"
#include "slang_log.h"
#include "slang_mem.h"
#include "slang_vartable.h"
#include "slang_simplify.h"

#include "slang_print.h"

/*
 * This is a straightforward implementation of the slang front-end
 * compiler.  Lots of error-checking functionality is missing but
 * every well-formed shader source should compile successfully and
 * execute as expected. However, some semantically ill-formed shaders
 * may be accepted resulting in undefined behaviour.
 */


/** re-defined below, should be the same though */
#define TYPE_SPECIFIER_COUNT 32


/**
 * Check if the given identifier is legal.
 */
static GLboolean
legal_identifier(slang_atom name)
{
   /* "gl_" is a reserved prefix */
   if (_mesa_strncmp((char *) name, "gl_", 3) == 0) {
      return GL_FALSE;
   }
   return GL_TRUE;
}


/*
 * slang_code_unit
 */

GLvoid
_slang_code_unit_ctr(slang_code_unit * self,
                     struct slang_code_object_ * object)
{
   _slang_variable_scope_ctr(&self->vars);
   _slang_function_scope_ctr(&self->funs);
   _slang_struct_scope_ctr(&self->structs);
   self->object = object;
}

GLvoid
_slang_code_unit_dtr(slang_code_unit * self)
{
   slang_variable_scope_destruct(&self->vars);
   slang_function_scope_destruct(&self->funs);
   slang_struct_scope_destruct(&self->structs);
}

/*
 * slang_code_object
 */

GLvoid
_slang_code_object_ctr(slang_code_object * self)
{
   GLuint i;

   for (i = 0; i < SLANG_BUILTIN_TOTAL; i++)
      _slang_code_unit_ctr(&self->builtin[i], self);
   _slang_code_unit_ctr(&self->unit, self);
   slang_atom_pool_construct(&self->atompool);
}

GLvoid
_slang_code_object_dtr(slang_code_object * self)
{
   GLuint i;

   for (i = 0; i < SLANG_BUILTIN_TOTAL; i++)
      _slang_code_unit_dtr(&self->builtin[i]);
   _slang_code_unit_dtr(&self->unit);
   slang_atom_pool_destruct(&self->atompool);
}


/* slang_parse_ctx */

typedef struct slang_parse_ctx_
{
   const byte *I;
   slang_info_log *L;
   int parsing_builtin;
   GLboolean global_scope;   /**< Is object being declared a global? */
   slang_atom_pool *atoms;
   slang_unit_type type;     /**< Vertex vs. Fragment */
   GLuint version;           /**< user-specified (or default) #version */
} slang_parse_ctx;

/* slang_output_ctx */

typedef struct slang_output_ctx_
{
   slang_variable_scope *vars;
   slang_function_scope *funs;
   slang_struct_scope *structs;
   struct gl_program *program;
   struct gl_sl_pragmas *pragmas;
   slang_var_table *vartable;
   GLuint default_precision[TYPE_SPECIFIER_COUNT];
   GLboolean allow_precision;
   GLboolean allow_invariant;
   GLboolean allow_centroid;
   GLboolean allow_array_types;  /* float[] syntax */
} slang_output_ctx;

/* _slang_compile() */


/* Debugging aid, print file/line where parsing error is detected */
#define RETURN0 \
   do { \
      if (0) \
         printf("slang error at %s:%d\n", __FILE__, __LINE__); \
      return 0; \
   } while (0)


static void
parse_identifier_str(slang_parse_ctx * C, char **id)
{
   *id = (char *) C->I;
   C->I += _mesa_strlen(*id) + 1;
}

static slang_atom
parse_identifier(slang_parse_ctx * C)
{
   const char *id;

   id = (const char *) C->I;
   C->I += _mesa_strlen(id) + 1;
   return slang_atom_pool_atom(C->atoms, id);
}

static int
parse_number(slang_parse_ctx * C, int *number)
{
   const int radix = (int) (*C->I++);
   *number = 0;
   while (*C->I != '\0') {
      int digit;
      if (*C->I >= '0' && *C->I <= '9')
         digit = (int) (*C->I - '0');
      else if (*C->I >= 'A' && *C->I <= 'Z')
         digit = (int) (*C->I - 'A') + 10;
      else
         digit = (int) (*C->I - 'a') + 10;
      *number = *number * radix + digit;
      C->I++;
   }
   C->I++;
   if (*number > 65535)
      slang_info_log_warning(C->L, "%d: literal integer overflow.", *number);
   return 1;
}

static int
parse_float(slang_parse_ctx * C, float *number)
{
   char *integral = NULL;
   char *fractional = NULL;
   char *exponent = NULL;
   char *whole = NULL;

   parse_identifier_str(C, &integral);
   parse_identifier_str(C, &fractional);
   parse_identifier_str(C, &exponent);

   whole = (char *) _slang_alloc((_mesa_strlen(integral) +
                                  _mesa_strlen(fractional) +
                                  _mesa_strlen(exponent) + 3) * sizeof(char));
   if (whole == NULL) {
      slang_info_log_memory(C->L);
      RETURN0;
   }

   slang_string_copy(whole, integral);
   slang_string_concat(whole, ".");
   slang_string_concat(whole, fractional);
   slang_string_concat(whole, "E");
   slang_string_concat(whole, exponent);

   *number = (float) (_mesa_strtod(whole, (char **) NULL));

   _slang_free(whole);

   return 1;
}

/* revision number - increment after each change affecting emitted output */
#define REVISION 5

static int
check_revision(slang_parse_ctx * C)
{
   if (*C->I != REVISION) {
      slang_info_log_error(C->L, "Internal compiler error.");
      RETURN0;
   }
   C->I++;
   return 1;
}

static int parse_statement(slang_parse_ctx *, slang_output_ctx *,
                           slang_operation *);
static int parse_expression(slang_parse_ctx *, slang_output_ctx *,
                            slang_operation *);
static int parse_type_specifier(slang_parse_ctx *, slang_output_ctx *,
                                slang_type_specifier *);
static int
parse_type_array_size(slang_parse_ctx *C,
                      slang_output_ctx *O,
                      GLint *array_len);

static GLboolean
parse_array_len(slang_parse_ctx * C, slang_output_ctx * O, GLuint * len)
{
   slang_operation array_size;
   slang_name_space space;
   GLboolean result;

   if (!slang_operation_construct(&array_size))
      return GL_FALSE;
   if (!parse_expression(C, O, &array_size)) {
      slang_operation_destruct(&array_size);
      return GL_FALSE;
   }

   space.funcs = O->funs;
   space.structs = O->structs;
   space.vars = O->vars;

   /* evaluate compile-time expression which is array size */
   _slang_simplify(&array_size, &space, C->atoms);

   if (array_size.type == SLANG_OPER_LITERAL_INT) {
      result = GL_TRUE;
      *len = (GLint) array_size.literal[0];
   } else if (array_size.type == SLANG_OPER_IDENTIFIER) {
      slang_variable *var = _slang_variable_locate(array_size.locals, array_size.a_id, GL_TRUE);
      if (!var) {
         slang_info_log_error(C->L, "undefined variable '%s'",
                              (char *) array_size.a_id);
         result = GL_FALSE;
      } else if (var->type.qualifier == SLANG_QUAL_CONST &&
                 var->type.specifier.type == SLANG_SPEC_INT) {
         if (var->initializer &&
             var->initializer->type == SLANG_OPER_LITERAL_INT) {
            *len = (GLint) var->initializer->literal[0];
            result = GL_TRUE;
         } else {
            slang_info_log_error(C->L, "unable to parse array size declaration");
            result = GL_FALSE;
         }
      } else {
         slang_info_log_error(C->L, "unable to parse array size declaration");
         result = GL_FALSE;
      }
   } else {
      result = GL_FALSE;
   }

   slang_operation_destruct(&array_size);
   return result;
}

static GLboolean
calculate_var_size(slang_parse_ctx * C, slang_output_ctx * O,
                   slang_variable * var)
{
   slang_storage_aggregate agg;

   if (!slang_storage_aggregate_construct(&agg))
      return GL_FALSE;
   if (!_slang_aggregate_variable(&agg, &var->type.specifier, var->array_len,
                                  O->funs, O->structs, O->vars, C->atoms)) {
      slang_storage_aggregate_destruct(&agg);
      return GL_FALSE;
   }
   var->size = _slang_sizeof_aggregate(&agg);
   slang_storage_aggregate_destruct(&agg);
   return GL_TRUE;
}

static void
promote_type_to_array(slang_parse_ctx *C,
                      slang_fully_specified_type *type,
                      GLint array_len)
{
   slang_type_specifier *baseType =
      slang_type_specifier_new(type->specifier.type, NULL, NULL);

   type->specifier.type = SLANG_SPEC_ARRAY;
   type->specifier._array = baseType;
   type->array_len = array_len;
}


static GLboolean
convert_to_array(slang_parse_ctx * C, slang_variable * var,
                 const slang_type_specifier * sp)
{
   /* sized array - mark it as array, copy the specifier to the array element
    * and parse the expression */
   var->type.specifier.type = SLANG_SPEC_ARRAY;
   var->type.specifier._array = (slang_type_specifier *)
      _slang_alloc(sizeof(slang_type_specifier));
   if (var->type.specifier._array == NULL) {
      slang_info_log_memory(C->L);
      return GL_FALSE;
   }
   slang_type_specifier_ctr(var->type.specifier._array);
   return slang_type_specifier_copy(var->type.specifier._array, sp);
}

/* structure field */
#define FIELD_NONE 0
#define FIELD_NEXT 1
#define FIELD_ARRAY 2

static GLboolean
parse_struct_field_var(slang_parse_ctx * C, slang_output_ctx * O,
                       slang_variable * var, slang_atom a_name,
                       const slang_type_specifier * sp,
                       GLuint array_len)
{
   var->a_name = a_name;
   if (var->a_name == SLANG_ATOM_NULL)
      return GL_FALSE;

   switch (*C->I++) {
   case FIELD_NONE:
      if (array_len != -1) {
         if (!convert_to_array(C, var, sp))
            return GL_FALSE;
         var->array_len = array_len;
      }
      else {
         if (!slang_type_specifier_copy(&var->type.specifier, sp))
            return GL_FALSE;
      }
      break;
   case FIELD_ARRAY:
      if (array_len != -1)
         return GL_FALSE;
      if (!convert_to_array(C, var, sp))
         return GL_FALSE;
      if (!parse_array_len(C, O, &var->array_len))
         return GL_FALSE;
      break;
   default:
      return GL_FALSE;
   }

   return calculate_var_size(C, O, var);
}

static int
parse_struct_field(slang_parse_ctx * C, slang_output_ctx * O,
                   slang_struct * st, slang_type_specifier * sp)
{
   slang_output_ctx o = *O;
   GLint array_len;

   o.structs = st->structs;
   if (!parse_type_specifier(C, &o, sp))
      RETURN0;
   if (!parse_type_array_size(C, &o, &array_len))
      RETURN0;

   do {
      slang_atom a_name;
      slang_variable *var = slang_variable_scope_grow(st->fields);
      if (!var) {
         slang_info_log_memory(C->L);
         RETURN0;
      }
      a_name = parse_identifier(C);
      if (_slang_variable_locate(st->fields, a_name, GL_FALSE)) {
         slang_info_log_error(C->L, "duplicate field '%s'", (char *) a_name);
         RETURN0;
      }

      if (!parse_struct_field_var(C, &o, var, a_name, sp, array_len))
         RETURN0;
   }
   while (*C->I++ != FIELD_NONE);

   return 1;
}

static int
parse_struct(slang_parse_ctx * C, slang_output_ctx * O, slang_struct ** st)
{
   slang_atom a_name;
   const char *name;

   /* parse struct name (if any) and make sure it is unique in current scope */
   a_name = parse_identifier(C);
   if (a_name == SLANG_ATOM_NULL)
      RETURN0;

   name = slang_atom_pool_id(C->atoms, a_name);
   if (name[0] != '\0'
       && slang_struct_scope_find(O->structs, a_name, 0) != NULL) {
      slang_info_log_error(C->L, "%s: duplicate type name.", name);
      RETURN0;
   }

   /* set-up a new struct */
   *st = (slang_struct *) _slang_alloc(sizeof(slang_struct));
   if (*st == NULL) {
      slang_info_log_memory(C->L);
      RETURN0;
   }
   if (!slang_struct_construct(*st)) {
      _slang_free(*st);
      *st = NULL;
      slang_info_log_memory(C->L);
      RETURN0;
   }
   (**st).a_name = a_name;
   (**st).structs->outer_scope = O->structs;

   /* parse individual struct fields */
   do {
      slang_type_specifier sp;

      slang_type_specifier_ctr(&sp);
      if (!parse_struct_field(C, O, *st, &sp)) {
         slang_type_specifier_dtr(&sp);
         RETURN0;
      }
      slang_type_specifier_dtr(&sp);
   }
   while (*C->I++ != FIELD_NONE);

   /* if named struct, copy it to current scope */
   if (name[0] != '\0') {
      slang_struct *s;

      O->structs->structs =
         (slang_struct *) _slang_realloc(O->structs->structs,
                                         O->structs->num_structs
                                         * sizeof(slang_struct),
                                         (O->structs->num_structs + 1)
                                         * sizeof(slang_struct));
      if (O->structs->structs == NULL) {
         slang_info_log_memory(C->L);
         RETURN0;
      }
      s = &O->structs->structs[O->structs->num_structs];
      if (!slang_struct_construct(s))
         RETURN0;
      O->structs->num_structs++;
      if (!slang_struct_copy(s, *st))
         RETURN0;
   }

   return 1;
}


/* invariant qualifer */
#define TYPE_VARIANT    90
#define TYPE_INVARIANT  91

static int
parse_type_variant(slang_parse_ctx * C, slang_type_variant *variant)
{
   GLuint invariant = *C->I++;
   switch (invariant) {
   case TYPE_VARIANT:
      *variant = SLANG_VARIANT;
      return 1;
   case TYPE_INVARIANT:
      *variant = SLANG_INVARIANT;
      return 1;
   default:
      RETURN0;
   }
}


/* centroid qualifer */
#define TYPE_CENTER    95
#define TYPE_CENTROID  96

static int
parse_type_centroid(slang_parse_ctx * C, slang_type_centroid *centroid)
{
   GLuint c = *C->I++;
   switch (c) {
   case TYPE_CENTER:
      *centroid = SLANG_CENTER;
      return 1;
   case TYPE_CENTROID:
      *centroid = SLANG_CENTROID;
      return 1;
   default:
      RETURN0;
   }
}


/* type qualifier */
#define TYPE_QUALIFIER_NONE 0
#define TYPE_QUALIFIER_CONST 1
#define TYPE_QUALIFIER_ATTRIBUTE 2
#define TYPE_QUALIFIER_VARYING 3
#define TYPE_QUALIFIER_UNIFORM 4
#define TYPE_QUALIFIER_FIXEDOUTPUT 5
#define TYPE_QUALIFIER_FIXEDINPUT 6

static int
parse_type_qualifier(slang_parse_ctx * C, slang_type_qualifier * qual)
{
   GLuint qualifier = *C->I++;
   switch (qualifier) {
   case TYPE_QUALIFIER_NONE:
      *qual = SLANG_QUAL_NONE;
      break;
   case TYPE_QUALIFIER_CONST:
      *qual = SLANG_QUAL_CONST;
      break;
   case TYPE_QUALIFIER_ATTRIBUTE:
      *qual = SLANG_QUAL_ATTRIBUTE;
      break;
   case TYPE_QUALIFIER_VARYING:
      *qual = SLANG_QUAL_VARYING;
      break;
   case TYPE_QUALIFIER_UNIFORM:
      *qual = SLANG_QUAL_UNIFORM;
      break;
   case TYPE_QUALIFIER_FIXEDOUTPUT:
      *qual = SLANG_QUAL_FIXEDOUTPUT;
      break;
   case TYPE_QUALIFIER_FIXEDINPUT:
      *qual = SLANG_QUAL_FIXEDINPUT;
      break;
   default:
      RETURN0;
   }
   return 1;
}

/* type specifier */
#define TYPE_SPECIFIER_VOID 0
#define TYPE_SPECIFIER_BOOL 1
#define TYPE_SPECIFIER_BVEC2 2
#define TYPE_SPECIFIER_BVEC3 3
#define TYPE_SPECIFIER_BVEC4 4
#define TYPE_SPECIFIER_INT 5
#define TYPE_SPECIFIER_IVEC2 6
#define TYPE_SPECIFIER_IVEC3 7
#define TYPE_SPECIFIER_IVEC4 8
#define TYPE_SPECIFIER_FLOAT 9
#define TYPE_SPECIFIER_VEC2 10
#define TYPE_SPECIFIER_VEC3 11
#define TYPE_SPECIFIER_VEC4 12
#define TYPE_SPECIFIER_MAT2 13
#define TYPE_SPECIFIER_MAT3 14
#define TYPE_SPECIFIER_MAT4 15
#define TYPE_SPECIFIER_SAMPLER1D 16
#define TYPE_SPECIFIER_SAMPLER2D 17
#define TYPE_SPECIFIER_SAMPLER3D 18
#define TYPE_SPECIFIER_SAMPLERCUBE 19
#define TYPE_SPECIFIER_SAMPLER1DSHADOW 20
#define TYPE_SPECIFIER_SAMPLER2DSHADOW 21
#define TYPE_SPECIFIER_SAMPLER2DRECT 22
#define TYPE_SPECIFIER_SAMPLER2DRECTSHADOW 23
#define TYPE_SPECIFIER_STRUCT 24
#define TYPE_SPECIFIER_TYPENAME 25
#define TYPE_SPECIFIER_MAT23 26
#define TYPE_SPECIFIER_MAT32 27
#define TYPE_SPECIFIER_MAT24 28
#define TYPE_SPECIFIER_MAT42 29
#define TYPE_SPECIFIER_MAT34 30
#define TYPE_SPECIFIER_MAT43 31
#define TYPE_SPECIFIER_COUNT 32

static int
parse_type_specifier(slang_parse_ctx * C, slang_output_ctx * O,
                     slang_type_specifier * spec)
{
   switch (*C->I++) {
   case TYPE_SPECIFIER_VOID:
      spec->type = SLANG_SPEC_VOID;
      break;
   case TYPE_SPECIFIER_BOOL:
      spec->type = SLANG_SPEC_BOOL;
      break;
   case TYPE_SPECIFIER_BVEC2:
      spec->type = SLANG_SPEC_BVEC2;
      break;
   case TYPE_SPECIFIER_BVEC3:
      spec->type = SLANG_SPEC_BVEC3;
      break;
   case TYPE_SPECIFIER_BVEC4:
      spec->type = SLANG_SPEC_BVEC4;
      break;
   case TYPE_SPECIFIER_INT:
      spec->type = SLANG_SPEC_INT;
      break;
   case TYPE_SPECIFIER_IVEC2:
      spec->type = SLANG_SPEC_IVEC2;
      break;
   case TYPE_SPECIFIER_IVEC3:
      spec->type = SLANG_SPEC_IVEC3;
      break;
   case TYPE_SPECIFIER_IVEC4:
      spec->type = SLANG_SPEC_IVEC4;
      break;
   case TYPE_SPECIFIER_FLOAT:
      spec->type = SLANG_SPEC_FLOAT;
      break;
   case TYPE_SPECIFIER_VEC2:
      spec->type = SLANG_SPEC_VEC2;
      break;
   case TYPE_SPECIFIER_VEC3:
      spec->type = SLANG_SPEC_VEC3;
      break;
   case TYPE_SPECIFIER_VEC4:
      spec->type = SLANG_SPEC_VEC4;
      break;
   case TYPE_SPECIFIER_MAT2:
      spec->type = SLANG_SPEC_MAT2;
      break;
   case TYPE_SPECIFIER_MAT3:
      spec->type = SLANG_SPEC_MAT3;
      break;
   case TYPE_SPECIFIER_MAT4:
      spec->type = SLANG_SPEC_MAT4;
      break;
   case TYPE_SPECIFIER_MAT23:
      spec->type = SLANG_SPEC_MAT23;
      break;
   case TYPE_SPECIFIER_MAT32:
      spec->type = SLANG_SPEC_MAT32;
      break;
   case TYPE_SPECIFIER_MAT24:
      spec->type = SLANG_SPEC_MAT24;
      break;
   case TYPE_SPECIFIER_MAT42:
      spec->type = SLANG_SPEC_MAT42;
      break;
   case TYPE_SPECIFIER_MAT34:
      spec->type = SLANG_SPEC_MAT34;
      break;
   case TYPE_SPECIFIER_MAT43:
      spec->type = SLANG_SPEC_MAT43;
      break;
   case TYPE_SPECIFIER_SAMPLER1D:
      spec->type = SLANG_SPEC_SAMPLER1D;
      break;
   case TYPE_SPECIFIER_SAMPLER2D:
      spec->type = SLANG_SPEC_SAMPLER2D;
      break;
   case TYPE_SPECIFIER_SAMPLER3D:
      spec->type = SLANG_SPEC_SAMPLER3D;
      break;
   case TYPE_SPECIFIER_SAMPLERCUBE:
      spec->type = SLANG_SPEC_SAMPLERCUBE;
      break;
   case TYPE_SPECIFIER_SAMPLER2DRECT:
      spec->type = SLANG_SPEC_SAMPLER2DRECT;
      break;
   case TYPE_SPECIFIER_SAMPLER1DSHADOW:
      spec->type = SLANG_SPEC_SAMPLER1DSHADOW;
      break;
   case TYPE_SPECIFIER_SAMPLER2DSHADOW:
      spec->type = SLANG_SPEC_SAMPLER2DSHADOW;
      break;
   case TYPE_SPECIFIER_SAMPLER2DRECTSHADOW:
      spec->type = SLANG_SPEC_SAMPLER2DRECTSHADOW;
      break;
   case TYPE_SPECIFIER_STRUCT:
      spec->type = SLANG_SPEC_STRUCT;
      if (!parse_struct(C, O, &spec->_struct))
         RETURN0;
      break;
   case TYPE_SPECIFIER_TYPENAME:
      spec->type = SLANG_SPEC_STRUCT;
      {
         slang_atom a_name;
         slang_struct *stru;

         a_name = parse_identifier(C);
         if (a_name == NULL)
            RETURN0;

         stru = slang_struct_scope_find(O->structs, a_name, 1);
         if (stru == NULL) {
            slang_info_log_error(C->L, "undeclared type name '%s'",
                                 slang_atom_pool_id(C->atoms, a_name));
            RETURN0;
         }

         spec->_struct = (slang_struct *) _slang_alloc(sizeof(slang_struct));
         if (spec->_struct == NULL) {
            slang_info_log_memory(C->L);
            RETURN0;
         }
         if (!slang_struct_construct(spec->_struct)) {
            _slang_free(spec->_struct);
            spec->_struct = NULL;
            RETURN0;
         }
         if (!slang_struct_copy(spec->_struct, stru))
            RETURN0;
      }
      break;
   default:
      RETURN0;
   }
   return 1;
}

#define TYPE_SPECIFIER_NONARRAY 0
#define TYPE_SPECIFIER_ARRAY    1

static int
parse_type_array_size(slang_parse_ctx *C,
                      slang_output_ctx *O,
                      GLint *array_len)
{
   GLuint size;

   switch (*C->I++) {
   case TYPE_SPECIFIER_NONARRAY:
      *array_len = -1; /* -1 = not an array */
      break;
   case TYPE_SPECIFIER_ARRAY:
      if (!parse_array_len(C, O, &size))
         RETURN0;
      *array_len = (GLint) size;
      break;
   default:
      assert(0);
      RETURN0;
   }
   return 1;
}

#define PRECISION_DEFAULT 0
#define PRECISION_LOW     1
#define PRECISION_MEDIUM  2
#define PRECISION_HIGH    3

static int
parse_type_precision(slang_parse_ctx *C,
                     slang_type_precision *precision)
{
   GLint prec = *C->I++;
   switch (prec) {
   case PRECISION_DEFAULT:
      *precision = SLANG_PREC_DEFAULT;
      return 1;
   case PRECISION_LOW:
      *precision = SLANG_PREC_LOW;
      return 1;
   case PRECISION_MEDIUM:
      *precision = SLANG_PREC_MEDIUM;
      return 1;
   case PRECISION_HIGH:
      *precision = SLANG_PREC_HIGH;
      return 1;
   default:
      RETURN0;
   }
}

static int
parse_fully_specified_type(slang_parse_ctx * C, slang_output_ctx * O,
                           slang_fully_specified_type * type)
{
   if (!parse_type_variant(C, &type->variant))
      RETURN0;
  
   if (!parse_type_centroid(C, &type->centroid))
      RETURN0;

   if (!parse_type_qualifier(C, &type->qualifier))
      RETURN0;

   if (!parse_type_precision(C, &type->precision))
      RETURN0;

   if (!parse_type_specifier(C, O, &type->specifier))
      RETURN0;

   if (!parse_type_array_size(C, O, &type->array_len))
      RETURN0;

   if (!O->allow_invariant && type->variant == SLANG_INVARIANT) {
      slang_info_log_error(C->L,
         "'invariant' keyword not allowed (perhaps set #version 120)");
      RETURN0;
   }

   if (!O->allow_centroid && type->centroid == SLANG_CENTROID) {
      slang_info_log_error(C->L,
         "'centroid' keyword not allowed (perhaps set #version 120)");
      RETURN0;
   }
   else if (type->centroid == SLANG_CENTROID &&
            type->qualifier != SLANG_QUAL_VARYING) {
      slang_info_log_error(C->L,
         "'centroid' keyword only allowed for varying vars");
      RETURN0;
   }


   /* need this?
   if (type->qualifier != SLANG_QUAL_VARYING &&
       type->variant == SLANG_INVARIANT) {
      slang_info_log_error(C->L,
                           "invariant qualifer only allowed for varying vars");
      RETURN0;
   }
   */

   if (O->allow_precision) {
      if (type->precision == SLANG_PREC_DEFAULT) {
         assert(type->specifier.type < TYPE_SPECIFIER_COUNT);
         /* use the default precision for this datatype */
         type->precision = O->default_precision[type->specifier.type];
      }
   }
   else {
      /* only default is allowed */
      if (type->precision != SLANG_PREC_DEFAULT) {
         slang_info_log_error(C->L, "precision qualifiers not allowed");
         RETURN0;
      }
   }

   if (!O->allow_array_types && type->array_len >= 0) {
      slang_info_log_error(C->L, "first-class array types not allowed");
      RETURN0;
   }

   if (type->array_len >= 0) {
      /* convert type to array type (ex: convert "int" to "array of int" */
      promote_type_to_array(C, type, type->array_len);
   }

   return 1;
}

/* operation */
#define OP_END 0
#define OP_BLOCK_BEGIN_NO_NEW_SCOPE 1
#define OP_BLOCK_BEGIN_NEW_SCOPE 2
#define OP_DECLARE 3
#define OP_ASM 4
#define OP_BREAK 5
#define OP_CONTINUE 6
#define OP_DISCARD 7
#define OP_RETURN 8
#define OP_EXPRESSION 9
#define OP_IF 10
#define OP_WHILE 11
#define OP_DO 12
#define OP_FOR 13
#define OP_PUSH_VOID 14
#define OP_PUSH_BOOL 15
#define OP_PUSH_INT 16
#define OP_PUSH_FLOAT 17
#define OP_PUSH_IDENTIFIER 18
#define OP_SEQUENCE 19
#define OP_ASSIGN 20
#define OP_ADDASSIGN 21
#define OP_SUBASSIGN 22
#define OP_MULASSIGN 23
#define OP_DIVASSIGN 24
/*#define OP_MODASSIGN 25*/
/*#define OP_LSHASSIGN 26*/
/*#define OP_RSHASSIGN 27*/
/*#define OP_ORASSIGN 28*/
/*#define OP_XORASSIGN 29*/
/*#define OP_ANDASSIGN 30*/
#define OP_SELECT 31
#define OP_LOGICALOR 32
#define OP_LOGICALXOR 33
#define OP_LOGICALAND 34
/*#define OP_BITOR 35*/
/*#define OP_BITXOR 36*/
/*#define OP_BITAND 37*/
#define OP_EQUAL 38
#define OP_NOTEQUAL 39
#define OP_LESS 40
#define OP_GREATER 41
#define OP_LESSEQUAL 42
#define OP_GREATEREQUAL 43
/*#define OP_LSHIFT 44*/
/*#define OP_RSHIFT 45*/
#define OP_ADD 46
#define OP_SUBTRACT 47
#define OP_MULTIPLY 48
#define OP_DIVIDE 49
/*#define OP_MODULUS 50*/
#define OP_PREINCREMENT 51
#define OP_PREDECREMENT 52
#define OP_PLUS 53
#define OP_MINUS 54
/*#define OP_COMPLEMENT 55*/
#define OP_NOT 56
#define OP_SUBSCRIPT 57
#define OP_CALL 58
#define OP_FIELD 59
#define OP_POSTINCREMENT 60
#define OP_POSTDECREMENT 61
#define OP_PRECISION 62
#define OP_METHOD 63


/**
 * When parsing a compound production, this function is used to parse the
 * children.
 * For example, a while-loop compound will have two children, the
 * while condition expression and the loop body.  So, this function will
 * be called twice to parse those two sub-expressions.
 * \param C  the parsing context
 * \param O  the output context
 * \param oper  the operation we're parsing
 * \param statement  indicates whether parsing a statement, or expression
 * \return 1 if success, 0 if error
 */
static int
parse_child_operation(slang_parse_ctx * C, slang_output_ctx * O,
                      slang_operation * oper, GLboolean statement)
{
   slang_operation *ch;

   /* grow child array */
   ch = slang_operation_grow(&oper->num_children, &oper->children);
   if (statement)
      return parse_statement(C, O, ch);
   return parse_expression(C, O, ch);
}

static int parse_declaration(slang_parse_ctx * C, slang_output_ctx * O);

static int
parse_statement(slang_parse_ctx * C, slang_output_ctx * O,
                slang_operation * oper)
{
   int op;

   oper->locals->outer_scope = O->vars;

   op = *C->I++;
   switch (op) {
   case OP_BLOCK_BEGIN_NO_NEW_SCOPE:
      /* parse child statements, do not create new variable scope */
      oper->type = SLANG_OPER_BLOCK_NO_NEW_SCOPE;
      while (*C->I != OP_END)
         if (!parse_child_operation(C, O, oper, GL_TRUE))
            RETURN0;
      C->I++;
      break;
   case OP_BLOCK_BEGIN_NEW_SCOPE:
      /* parse child statements, create new variable scope */
      {
         slang_output_ctx o = *O;

         oper->type = SLANG_OPER_BLOCK_NEW_SCOPE;
         o.vars = oper->locals;
         while (*C->I != OP_END)
            if (!parse_child_operation(C, &o, oper, GL_TRUE))
               RETURN0;
         C->I++;
      }
      break;
   case OP_DECLARE:
      /* local variable declaration, individual declarators are stored as
       * children identifiers
       */
      oper->type = SLANG_OPER_BLOCK_NO_NEW_SCOPE;
      {
         const unsigned int first_var = O->vars->num_variables;

         /* parse the declaration, note that there can be zero or more
          * than one declarators
          */
         if (!parse_declaration(C, O))
            RETURN0;
         if (first_var < O->vars->num_variables) {
            const unsigned int num_vars = O->vars->num_variables - first_var;
            unsigned int i;
            assert(oper->num_children == 0);
            oper->num_children = num_vars;
            oper->children = slang_operation_new(num_vars);
            if (oper->children == NULL) {
               slang_info_log_memory(C->L);
               RETURN0;
            }
            for (i = first_var; i < O->vars->num_variables; i++) {
               slang_operation *o = &oper->children[i - first_var];
               slang_variable *var = O->vars->variables[i];
               o->type = SLANG_OPER_VARIABLE_DECL;
               o->locals->outer_scope = O->vars;
               o->a_id = var->a_name;

               /* new/someday...
               calculate_var_size(C, O, var);
               */

               if (!legal_identifier(o->a_id)) {
                  slang_info_log_error(C->L, "illegal variable name '%s'",
                                       (char *) o->a_id);
                  RETURN0;
               }
            }
         }
      }
      break;
   case OP_ASM:
      /* the __asm statement, parse the mnemonic and all its arguments
       * as expressions
       */
      oper->type = SLANG_OPER_ASM;
      oper->a_id = parse_identifier(C);
      if (oper->a_id == SLANG_ATOM_NULL)
         RETURN0;
      while (*C->I != OP_END) {
         if (!parse_child_operation(C, O, oper, GL_FALSE))
            RETURN0;
      }
      C->I++;
      break;
   case OP_BREAK:
      oper->type = SLANG_OPER_BREAK;
      break;
   case OP_CONTINUE:
      oper->type = SLANG_OPER_CONTINUE;
      break;
   case OP_DISCARD:
      oper->type = SLANG_OPER_DISCARD;
      break;
   case OP_RETURN:
      oper->type = SLANG_OPER_RETURN;
      if (!parse_child_operation(C, O, oper, GL_FALSE))
         RETURN0;
      break;
   case OP_EXPRESSION:
      oper->type = SLANG_OPER_EXPRESSION;
      if (!parse_child_operation(C, O, oper, GL_FALSE))
         RETURN0;
      break;
   case OP_IF:
      oper->type = SLANG_OPER_IF;
      if (!parse_child_operation(C, O, oper, GL_FALSE))
         RETURN0;
      if (!parse_child_operation(C, O, oper, GL_TRUE))
         RETURN0;
      if (!parse_child_operation(C, O, oper, GL_TRUE))
         RETURN0;
      break;
   case OP_WHILE:
      {
         slang_output_ctx o = *O;

         oper->type = SLANG_OPER_WHILE;
         o.vars = oper->locals;
         if (!parse_child_operation(C, &o, oper, GL_TRUE))
            RETURN0;
         if (!parse_child_operation(C, &o, oper, GL_TRUE))
            RETURN0;
      }
      break;
   case OP_DO:
      oper->type = SLANG_OPER_DO;
      if (!parse_child_operation(C, O, oper, GL_TRUE))
         RETURN0;
      if (!parse_child_operation(C, O, oper, GL_FALSE))
         RETURN0;
      break;
   case OP_FOR:
      {
         slang_output_ctx o = *O;

         oper->type = SLANG_OPER_FOR;
         o.vars = oper->locals;
         if (!parse_child_operation(C, &o, oper, GL_TRUE))
            RETURN0;
         if (!parse_child_operation(C, &o, oper, GL_TRUE))
            RETURN0;
         if (!parse_child_operation(C, &o, oper, GL_FALSE))
            RETURN0;
         if (!parse_child_operation(C, &o, oper, GL_TRUE))
            RETURN0;
      }
      break;
   case OP_PRECISION:
      {
         /* set default precision for a type in this scope */
         /* ignored at this time */
         int prec_qual = *C->I++;
         int datatype = *C->I++;
         (void) prec_qual;
         (void) datatype;
      }
      break;
   default:
      /*printf("Unexpected operation %d\n", op);*/
      RETURN0;
   }
   return 1;
}

static int
handle_nary_expression(slang_parse_ctx * C, slang_operation * op,
                       slang_operation ** ops, unsigned int *total_ops,
                       unsigned int n)
{
   unsigned int i;

   op->children = slang_operation_new(n);
   if (op->children == NULL) {
      slang_info_log_memory(C->L);
      RETURN0;
   }
   op->num_children = n;

   for (i = 0; i < n; i++) {
      slang_operation_destruct(&op->children[i]);
      op->children[i] = (*ops)[*total_ops - (n + 1 - i)];
   }

   (*ops)[*total_ops - (n + 1)] = (*ops)[*total_ops - 1];
   *total_ops -= n;

   *ops = (slang_operation *)
      _slang_realloc(*ops,
                     (*total_ops + n) * sizeof(slang_operation),
                     *total_ops * sizeof(slang_operation));
   if (*ops == NULL) {
      slang_info_log_memory(C->L);
      RETURN0;
   }
   return 1;
}

static int
is_constructor_name(const char *name, slang_atom a_name,
                    slang_struct_scope * structs)
{
   if (slang_type_specifier_type_from_string(name) != SLANG_SPEC_VOID)
      return 1;
   return slang_struct_scope_find(structs, a_name, 1) != NULL;
}

#define FUNCTION_CALL_NONARRAY 0
#define FUNCTION_CALL_ARRAY    1

static int
parse_expression(slang_parse_ctx * C, slang_output_ctx * O,
                 slang_operation * oper)
{
   slang_operation *ops = NULL;
   unsigned int num_ops = 0;
   int number;

   while (*C->I != OP_END) {
      slang_operation *op;
      const unsigned int op_code = *C->I++;

      /* allocate default operation, becomes a no-op if not used  */
      ops = (slang_operation *)
         _slang_realloc(ops,
                        num_ops * sizeof(slang_operation),
                        (num_ops + 1) * sizeof(slang_operation));
      if (ops == NULL) {
         slang_info_log_memory(C->L);
         RETURN0;
      }
      op = &ops[num_ops];
      if (!slang_operation_construct(op)) {
         slang_info_log_memory(C->L);
         RETURN0;
      }
      num_ops++;
      op->locals->outer_scope = O->vars;

      switch (op_code) {
      case OP_PUSH_VOID:
         op->type = SLANG_OPER_VOID;
         break;
      case OP_PUSH_BOOL:
         op->type = SLANG_OPER_LITERAL_BOOL;
         if (!parse_number(C, &number))
            RETURN0;
         op->literal[0] =
         op->literal[1] =
         op->literal[2] =
         op->literal[3] = (GLfloat) number;
         op->literal_size = 1;
         break;
      case OP_PUSH_INT:
         op->type = SLANG_OPER_LITERAL_INT;
         if (!parse_number(C, &number))
            RETURN0;
         op->literal[0] =
         op->literal[1] =
         op->literal[2] =
         op->literal[3] = (GLfloat) number;
         op->literal_size = 1;
         break;
      case OP_PUSH_FLOAT:
         op->type = SLANG_OPER_LITERAL_FLOAT;
         if (!parse_float(C, &op->literal[0]))
            RETURN0;
         op->literal[1] =
         op->literal[2] =
         op->literal[3] = op->literal[0];
         op->literal_size = 1;
         break;
      case OP_PUSH_IDENTIFIER:
         op->type = SLANG_OPER_IDENTIFIER;
         op->a_id = parse_identifier(C);
         if (op->a_id == SLANG_ATOM_NULL)
            RETURN0;
         break;
      case OP_SEQUENCE:
         op->type = SLANG_OPER_SEQUENCE;
         if (!handle_nary_expression(C, op, &ops, &num_ops, 2))
            RETURN0;
         break;
      case OP_ASSIGN:
         op->type = SLANG_OPER_ASSIGN;
         if (!handle_nary_expression(C, op, &ops, &num_ops, 2))
            RETURN0;
         break;
      case OP_ADDASSIGN:
         op->type = SLANG_OPER_ADDASSIGN;
         if (!handle_nary_expression(C, op, &ops, &num_ops, 2))
            RETURN0;
         break;
      case OP_SUBASSIGN:
         op->type = SLANG_OPER_SUBASSIGN;
         if (!handle_nary_expression(C, op, &ops, &num_ops, 2))
            RETURN0;
         break;
      case OP_MULASSIGN:
         op->type = SLANG_OPER_MULASSIGN;
         if (!handle_nary_expression(C, op, &ops, &num_ops, 2))
            RETURN0;
         break;
      case OP_DIVASSIGN:
         op->type = SLANG_OPER_DIVASSIGN;
         if (!handle_nary_expression(C, op, &ops, &num_ops, 2))
            RETURN0;
         break;
         /*case OP_MODASSIGN: */
         /*case OP_LSHASSIGN: */
         /*case OP_RSHASSIGN: */
         /*case OP_ORASSIGN: */
         /*case OP_XORASSIGN: */
         /*case OP_ANDASSIGN: */
      case OP_SELECT:
         op->type = SLANG_OPER_SELECT;
         if (!handle_nary_expression(C, op, &ops, &num_ops, 3))
            RETURN0;
         break;
      case OP_LOGICALOR:
         op->type = SLANG_OPER_LOGICALOR;
         if (!handle_nary_expression(C, op, &ops, &num_ops, 2))
            RETURN0;
         break;
      case OP_LOGICALXOR:
         op->type = SLANG_OPER_LOGICALXOR;
         if (!handle_nary_expression(C, op, &ops, &num_ops, 2))
            RETURN0;
         break;
      case OP_LOGICALAND:
         op->type = SLANG_OPER_LOGICALAND;
         if (!handle_nary_expression(C, op, &ops, &num_ops, 2))
            RETURN0;
         break;
         /*case OP_BITOR: */
         /*case OP_BITXOR: */
         /*case OP_BITAND: */
      case OP_EQUAL:
         op->type = SLANG_OPER_EQUAL;
         if (!handle_nary_expression(C, op, &ops, &num_ops, 2))
            RETURN0;
         break;
      case OP_NOTEQUAL:
         op->type = SLANG_OPER_NOTEQUAL;
         if (!handle_nary_expression(C, op, &ops, &num_ops, 2))
            RETURN0;
         break;
      case OP_LESS:
         op->type = SLANG_OPER_LESS;
         if (!handle_nary_expression(C, op, &ops, &num_ops, 2))
            RETURN0;
         break;
      case OP_GREATER:
         op->type = SLANG_OPER_GREATER;
         if (!handle_nary_expression(C, op, &ops, &num_ops, 2))
            RETURN0;
         break;
      case OP_LESSEQUAL:
         op->type = SLANG_OPER_LESSEQUAL;
         if (!handle_nary_expression(C, op, &ops, &num_ops, 2))
            RETURN0;
         break;
      case OP_GREATEREQUAL:
         op->type = SLANG_OPER_GREATEREQUAL;
         if (!handle_nary_expression(C, op, &ops, &num_ops, 2))
            RETURN0;
         break;
         /*case OP_LSHIFT: */
         /*case OP_RSHIFT: */
      case OP_ADD:
         op->type = SLANG_OPER_ADD;
         if (!handle_nary_expression(C, op, &ops, &num_ops, 2))
            RETURN0;
         break;
      case OP_SUBTRACT:
         op->type = SLANG_OPER_SUBTRACT;
         if (!handle_nary_expression(C, op, &ops, &num_ops, 2))
            RETURN0;
         break;
      case OP_MULTIPLY:
         op->type = SLANG_OPER_MULTIPLY;
         if (!handle_nary_expression(C, op, &ops, &num_ops, 2))
            RETURN0;
         break;
      case OP_DIVIDE:
         op->type = SLANG_OPER_DIVIDE;
         if (!handle_nary_expression(C, op, &ops, &num_ops, 2))
            RETURN0;
         break;
         /*case OP_MODULUS: */
      case OP_PREINCREMENT:
         op->type = SLANG_OPER_PREINCREMENT;
         if (!handle_nary_expression(C, op, &ops, &num_ops, 1))
            RETURN0;
         break;
      case OP_PREDECREMENT:
         op->type = SLANG_OPER_PREDECREMENT;
         if (!handle_nary_expression(C, op, &ops, &num_ops, 1))
            RETURN0;
         break;
      case OP_PLUS:
         op->type = SLANG_OPER_PLUS;
         if (!handle_nary_expression(C, op, &ops, &num_ops, 1))
            RETURN0;
         break;
      case OP_MINUS:
         op->type = SLANG_OPER_MINUS;
         if (!handle_nary_expression(C, op, &ops, &num_ops, 1))
            RETURN0;
         break;
      case OP_NOT:
         op->type = SLANG_OPER_NOT;
         if (!handle_nary_expression(C, op, &ops, &num_ops, 1))
            RETURN0;
         break;
         /*case OP_COMPLEMENT: */
      case OP_SUBSCRIPT:
         op->type = SLANG_OPER_SUBSCRIPT;
         if (!handle_nary_expression(C, op, &ops, &num_ops, 2))
            RETURN0;
         break;
      case OP_METHOD:
         op->type = SLANG_OPER_METHOD;
         op->a_obj = parse_identifier(C);
         if (op->a_obj == SLANG_ATOM_NULL)
            RETURN0;

         op->a_id = parse_identifier(C);
         if (op->a_id == SLANG_ATOM_NULL)
            RETURN0;

         assert(*C->I == OP_END);
         C->I++;

         while (*C->I != OP_END)
            if (!parse_child_operation(C, O, op, GL_FALSE))
               RETURN0;
         C->I++;
#if 0
         /* don't lookup the method (not yet anyway) */
         if (!C->parsing_builtin
             && !slang_function_scope_find_by_name(O->funs, op->a_id, 1)) {
            const char *id;

            id = slang_atom_pool_id(C->atoms, op->a_id);
            if (!is_constructor_name(id, op->a_id, O->structs)) {
               slang_info_log_error(C->L, "%s: undeclared function name.", id);
               RETURN0;
            }
         }
#endif
         break;
      case OP_CALL:
         {
            GLboolean array_constructor = GL_FALSE;
            GLint array_constructor_size;

            op->type = SLANG_OPER_CALL;
            op->a_id = parse_identifier(C);
            if (op->a_id == SLANG_ATOM_NULL)
               RETURN0;
            switch (*C->I++) {
            case FUNCTION_CALL_NONARRAY:
               /* Nothing to do. */
               break;
            case FUNCTION_CALL_ARRAY:
               /* Calling an array constructor. For example:
                *   float[3](1.1, 2.2, 3.3);
                */
               if (!O->allow_array_types) {
                  slang_info_log_error(C->L,
                                       "array constructors not allowed "
                                       "in this GLSL version");
                  RETURN0;
               }
               else {
                  /* parse the array constructor size */
                  slang_operation array_size;
                  array_constructor = GL_TRUE;
                  slang_operation_construct(&array_size);
                  if (!parse_expression(C, O, &array_size)) {
                     slang_operation_destruct(&array_size);
                     return GL_FALSE;
                  }
                  if (array_size.type != SLANG_OPER_LITERAL_INT) {
                     slang_info_log_error(C->L,
                        "constructor array size is not an integer");
                     slang_operation_destruct(&array_size);
                     RETURN0;
                  }
                  array_constructor_size = (int) array_size.literal[0];
                  op->array_constructor = GL_TRUE;
                  slang_operation_destruct(&array_size);
               }
               break;
            default:
               assert(0);
               RETURN0;
            }
            while (*C->I != OP_END)
               if (!parse_child_operation(C, O, op, GL_FALSE))
                  RETURN0;
            C->I++;

            if (array_constructor &&
                array_constructor_size != op->num_children) {
               slang_info_log_error(C->L, "number of parameters to array"
                                    " constructor does not match array size");
               RETURN0;
            }

            if (!C->parsing_builtin
                && !slang_function_scope_find_by_name(O->funs, op->a_id, 1)) {
               const char *id;

               id = slang_atom_pool_id(C->atoms, op->a_id);
               if (!is_constructor_name(id, op->a_id, O->structs)) {
                  slang_info_log_error(C->L, "%s: undeclared function name.", id);
                  RETURN0;
               }
            }
         }
         break;
      case OP_FIELD:
         op->type = SLANG_OPER_FIELD;
         op->a_id = parse_identifier(C);
         if (op->a_id == SLANG_ATOM_NULL)
            RETURN0;
         if (!handle_nary_expression(C, op, &ops, &num_ops, 1))
            RETURN0;
         break;
      case OP_POSTINCREMENT:
         op->type = SLANG_OPER_POSTINCREMENT;
         if (!handle_nary_expression(C, op, &ops, &num_ops, 1))
            RETURN0;
         break;
      case OP_POSTDECREMENT:
         op->type = SLANG_OPER_POSTDECREMENT;
         if (!handle_nary_expression(C, op, &ops, &num_ops, 1))
            RETURN0;
         break;
      default:
         RETURN0;
      }
   }
   C->I++;

   slang_operation_destruct(oper);
   *oper = *ops; /* struct copy */
   _slang_free(ops);

   return 1;
}

/* parameter qualifier */
#define PARAM_QUALIFIER_IN 0
#define PARAM_QUALIFIER_OUT 1
#define PARAM_QUALIFIER_INOUT 2

/* function parameter array presence */
#define PARAMETER_ARRAY_NOT_PRESENT 0
#define PARAMETER_ARRAY_PRESENT 1

static int
parse_parameter_declaration(slang_parse_ctx * C, slang_output_ctx * O,
                            slang_variable * param)
{
   int param_qual, precision_qual;

   /* parse and validate the parameter's type qualifiers (there can be
    * two at most) because not all combinations are valid
    */
   if (!parse_type_qualifier(C, &param->type.qualifier))
      RETURN0;

   param_qual = *C->I++;
   switch (param_qual) {
   case PARAM_QUALIFIER_IN:
      if (param->type.qualifier != SLANG_QUAL_CONST
          && param->type.qualifier != SLANG_QUAL_NONE) {
         slang_info_log_error(C->L, "Invalid type qualifier.");
         RETURN0;
      }
      break;
   case PARAM_QUALIFIER_OUT:
      if (param->type.qualifier == SLANG_QUAL_NONE)
         param->type.qualifier = SLANG_QUAL_OUT;
      else {
         slang_info_log_error(C->L, "Invalid type qualifier.");
         RETURN0;
      }
      break;
   case PARAM_QUALIFIER_INOUT:
      if (param->type.qualifier == SLANG_QUAL_NONE)
         param->type.qualifier = SLANG_QUAL_INOUT;
      else {
         slang_info_log_error(C->L, "Invalid type qualifier.");
         RETURN0;
      }
      break;
   default:
      RETURN0;
   }

   /* parse precision qualifier (lowp, mediump, highp */
   precision_qual = *C->I++;
   /* ignored at this time */
   (void) precision_qual;

   /* parse parameter's type specifier and name */
   if (!parse_type_specifier(C, O, &param->type.specifier))
      RETURN0;
   if (!parse_type_array_size(C, O, &param->type.array_len))
      RETURN0;
   param->a_name = parse_identifier(C);
   if (param->a_name == SLANG_ATOM_NULL)
      RETURN0;

   /* first-class array
    */
   if (param->type.array_len >= 0) {
      slang_type_specifier p;

      slang_type_specifier_ctr(&p);
      if (!slang_type_specifier_copy(&p, &param->type.specifier)) {
         slang_type_specifier_dtr(&p);
         RETURN0;
      }
      if (!convert_to_array(C, param, &p)) {
         slang_type_specifier_dtr(&p);
         RETURN0;
      }
      slang_type_specifier_dtr(&p);
      param->array_len = param->type.array_len;
   }

   /* if the parameter is an array, parse its size (the size must be
    * explicitly defined
    */
   if (*C->I++ == PARAMETER_ARRAY_PRESENT) {
      slang_type_specifier p;

      if (param->type.array_len >= 0) {
         slang_info_log_error(C->L, "multi-dimensional arrays not allowed");
         RETURN0;
      }
      slang_type_specifier_ctr(&p);
      if (!slang_type_specifier_copy(&p, &param->type.specifier)) {
         slang_type_specifier_dtr(&p);
         RETURN0;
      }
      if (!convert_to_array(C, param, &p)) {
         slang_type_specifier_dtr(&p);
         RETURN0;
      }
      slang_type_specifier_dtr(&p);
      if (!parse_array_len(C, O, &param->array_len))
         RETURN0;
   }

#if 0
   /* calculate the parameter size */
   if (!calculate_var_size(C, O, param))
      RETURN0;
#endif
   /* TODO: allocate the local address here? */
   return 1;
}

/* function type */
#define FUNCTION_ORDINARY 0
#define FUNCTION_CONSTRUCTOR 1
#define FUNCTION_OPERATOR 2

/* function parameter */
#define PARAMETER_NONE 0
#define PARAMETER_NEXT 1

/* operator type */
#define OPERATOR_ADDASSIGN 1
#define OPERATOR_SUBASSIGN 2
#define OPERATOR_MULASSIGN 3
#define OPERATOR_DIVASSIGN 4
/*#define OPERATOR_MODASSIGN 5*/
/*#define OPERATOR_LSHASSIGN 6*/
/*#define OPERATOR_RSHASSIGN 7*/
/*#define OPERATOR_ANDASSIGN 8*/
/*#define OPERATOR_XORASSIGN 9*/
/*#define OPERATOR_ORASSIGN 10*/
#define OPERATOR_LOGICALXOR 11
/*#define OPERATOR_BITOR 12*/
/*#define OPERATOR_BITXOR 13*/
/*#define OPERATOR_BITAND 14*/
#define OPERATOR_LESS 15
#define OPERATOR_GREATER 16
#define OPERATOR_LESSEQUAL 17
#define OPERATOR_GREATEREQUAL 18
/*#define OPERATOR_LSHIFT 19*/
/*#define OPERATOR_RSHIFT 20*/
#define OPERATOR_MULTIPLY 21
#define OPERATOR_DIVIDE 22
/*#define OPERATOR_MODULUS 23*/
#define OPERATOR_INCREMENT 24
#define OPERATOR_DECREMENT 25
#define OPERATOR_PLUS 26
#define OPERATOR_MINUS 27
/*#define OPERATOR_COMPLEMENT 28*/
#define OPERATOR_NOT 29

static const struct
{
   unsigned int o_code;
   const char *o_name;
} operator_names[] = {
   {OPERATOR_INCREMENT, "++"},
   {OPERATOR_ADDASSIGN, "+="},
   {OPERATOR_PLUS, "+"},
   {OPERATOR_DECREMENT, "--"},
   {OPERATOR_SUBASSIGN, "-="},
   {OPERATOR_MINUS, "-"},
   {OPERATOR_NOT, "!"},
   {OPERATOR_MULASSIGN, "*="},
   {OPERATOR_MULTIPLY, "*"},
   {OPERATOR_DIVASSIGN, "/="},
   {OPERATOR_DIVIDE, "/"},
   {OPERATOR_LESSEQUAL, "<="},
   /*{ OPERATOR_LSHASSIGN, "<<=" }, */
   /*{ OPERATOR_LSHIFT, "<<" }, */
   {OPERATOR_LESS, "<"},
   {OPERATOR_GREATEREQUAL, ">="},
   /*{ OPERATOR_RSHASSIGN, ">>=" }, */
   /*{ OPERATOR_RSHIFT, ">>" }, */
   {OPERATOR_GREATER, ">"},
   /*{ OPERATOR_MODASSIGN, "%=" }, */
   /*{ OPERATOR_MODULUS, "%" }, */
   /*{ OPERATOR_ANDASSIGN, "&=" }, */
   /*{ OPERATOR_BITAND, "&" }, */
   /*{ OPERATOR_ORASSIGN, "|=" }, */
   /*{ OPERATOR_BITOR, "|" }, */
   /*{ OPERATOR_COMPLEMENT, "~" }, */
   /*{ OPERATOR_XORASSIGN, "^=" }, */
   {OPERATOR_LOGICALXOR, "^^"},
   /*{ OPERATOR_BITXOR, "^" } */
};

static slang_atom
parse_operator_name(slang_parse_ctx * C)
{
   unsigned int i;

   for (i = 0; i < sizeof(operator_names) / sizeof(*operator_names); i++) {
      if (operator_names[i].o_code == (unsigned int) (*C->I)) {
         slang_atom atom =
            slang_atom_pool_atom(C->atoms, operator_names[i].o_name);
         if (atom == SLANG_ATOM_NULL) {
            slang_info_log_memory(C->L);
            RETURN0;
         }
         C->I++;
         return atom;
      }
   }
   RETURN0;
}


static int
parse_function_prototype(slang_parse_ctx * C, slang_output_ctx * O,
                         slang_function * func)
{
   GLuint functype;
   /* parse function type and name */
   if (!parse_fully_specified_type(C, O, &func->header.type))
      RETURN0;

   functype = *C->I++;
   switch (functype) {
   case FUNCTION_ORDINARY:
      func->kind = SLANG_FUNC_ORDINARY;
      func->header.a_name = parse_identifier(C);
      if (func->header.a_name == SLANG_ATOM_NULL)
         RETURN0;
      break;
   case FUNCTION_CONSTRUCTOR:
      func->kind = SLANG_FUNC_CONSTRUCTOR;
      if (func->header.type.specifier.type == SLANG_SPEC_STRUCT)
         RETURN0;
      func->header.a_name =
         slang_atom_pool_atom(C->atoms,
                              slang_type_specifier_type_to_string
                              (func->header.type.specifier.type));
      if (func->header.a_name == SLANG_ATOM_NULL) {
         slang_info_log_memory(C->L);
         RETURN0;
      }
      break;
   case FUNCTION_OPERATOR:
      func->kind = SLANG_FUNC_OPERATOR;
      func->header.a_name = parse_operator_name(C);
      if (func->header.a_name == SLANG_ATOM_NULL)
         RETURN0;
      break;
   default:
      RETURN0;
   }

   if (!legal_identifier(func->header.a_name)) {
      slang_info_log_error(C->L, "illegal function name '%s'",
                           (char *) func->header.a_name);
      RETURN0;
   }

   /* parse function parameters */
   while (*C->I++ == PARAMETER_NEXT) {
      slang_variable *p = slang_variable_scope_grow(func->parameters);
      if (!p) {
         slang_info_log_memory(C->L);
         RETURN0;
      }
      if (!parse_parameter_declaration(C, O, p))
         RETURN0;
   }

   /* if the function returns a value, append a hidden __retVal 'out'
    * parameter that corresponds to the return value.
    */
   if (_slang_function_has_return_value(func)) {
      slang_variable *p = slang_variable_scope_grow(func->parameters);
      slang_atom a_retVal = slang_atom_pool_atom(C->atoms, "__retVal");
      assert(a_retVal);
      p->a_name = a_retVal;
      p->type = func->header.type;
      p->type.qualifier = SLANG_QUAL_OUT;
   }

   /* function formal parameters and local variables share the same
    * scope, so save the information about param count in a seperate
    * place also link the scope to the global variable scope so when a
    * given identifier is not found here, the search process continues
    * in the global space
    */
   func->param_count = func->parameters->num_variables;
   func->parameters->outer_scope = O->vars;

   return 1;
}

static int
parse_function_definition(slang_parse_ctx * C, slang_output_ctx * O,
                          slang_function * func)
{
   slang_output_ctx o = *O;

   if (!parse_function_prototype(C, O, func))
      RETURN0;

   /* create function's body operation */
   func->body = (slang_operation *) _slang_alloc(sizeof(slang_operation));
   if (func->body == NULL) {
      slang_info_log_memory(C->L);
      RETURN0;
   }
   if (!slang_operation_construct(func->body)) {
      _slang_free(func->body);
      func->body = NULL;
      slang_info_log_memory(C->L);
      RETURN0;
   }

   /* to parse the body the parse context is modified in order to
    * capture parsed variables into function's local variable scope
    */
   C->global_scope = GL_FALSE;
   o.vars = func->parameters;
   if (!parse_statement(C, &o, func->body))
      RETURN0;

   C->global_scope = GL_TRUE;
   return 1;
}

static GLboolean
initialize_global(slang_assemble_ctx * A, slang_variable * var)
{
   slang_operation op_id, op_assign;
   GLboolean result;

   /* construct the left side of assignment */
   if (!slang_operation_construct(&op_id))
      return GL_FALSE;
   op_id.type = SLANG_OPER_IDENTIFIER;
   op_id.a_id = var->a_name;

   /* put the variable into operation's scope */
   op_id.locals->variables =
      (slang_variable **) _slang_alloc(sizeof(slang_variable *));
   if (op_id.locals->variables == NULL) {
      slang_operation_destruct(&op_id);
      return GL_FALSE;
   }
   op_id.locals->num_variables = 1;
   op_id.locals->variables[0] = var;

   /* construct the assignment expression */
   if (!slang_operation_construct(&op_assign)) {
      op_id.locals->num_variables = 0;
      slang_operation_destruct(&op_id);
      return GL_FALSE;
   }
   op_assign.type = SLANG_OPER_ASSIGN;
   op_assign.children =
      (slang_operation *) _slang_alloc(2 * sizeof(slang_operation));
   if (op_assign.children == NULL) {
      slang_operation_destruct(&op_assign);
      op_id.locals->num_variables = 0;
      slang_operation_destruct(&op_id);
      return GL_FALSE;
   }
   op_assign.num_children = 2;
   op_assign.children[0] = op_id;
   op_assign.children[1] = *var->initializer;

   result = 1;

   /* carefully destroy the operations */
   op_assign.num_children = 0;
   _slang_free(op_assign.children);
   op_assign.children = NULL;
   slang_operation_destruct(&op_assign);
   op_id.locals->num_variables = 0;
   slang_operation_destruct(&op_id);

   if (!result)
      return GL_FALSE;

   return GL_TRUE;
}

/* init declarator list */
#define DECLARATOR_NONE 0
#define DECLARATOR_NEXT 1

/* variable declaration */
#define VARIABLE_NONE 0
#define VARIABLE_IDENTIFIER 1
#define VARIABLE_INITIALIZER 2
#define VARIABLE_ARRAY_EXPLICIT 3
#define VARIABLE_ARRAY_UNKNOWN 4


/**
 * Parse the initializer for a variable declaration.
 */
static int
parse_init_declarator(slang_parse_ctx * C, slang_output_ctx * O,
                      const slang_fully_specified_type * type)
{
   slang_variable *var;
   slang_atom a_name;

   /* empty init declatator (without name, e.g. "float ;") */
   if (*C->I++ == VARIABLE_NONE)
      return 1;

   a_name = parse_identifier(C);

   /* check if name is already in this scope */
   if (_slang_variable_locate(O->vars, a_name, GL_FALSE)) {
      slang_info_log_error(C->L,
                   "declaration of '%s' conflicts with previous declaration",
                   (char *) a_name);
      RETURN0;
   }

   /* make room for the new variable and initialize it */
   var = slang_variable_scope_grow(O->vars);
   if (!var) {
      slang_info_log_memory(C->L);
      RETURN0;
   }

   /* copy the declarator type qualifier/etc info, parse the identifier */
   var->type.qualifier = type->qualifier;
   var->type.centroid = type->centroid;
   var->type.precision = type->precision;
   var->type.variant = type->variant;
   var->type.array_len = type->array_len;
   var->a_name = a_name;
   if (var->a_name == SLANG_ATOM_NULL)
      RETURN0;

   switch (*C->I++) {
   case VARIABLE_NONE:
      /* simple variable declarator - just copy the specifier */
      if (!slang_type_specifier_copy(&var->type.specifier, &type->specifier))
         RETURN0;
      break;
   case VARIABLE_INITIALIZER:
      /* initialized variable - copy the specifier and parse the expression */
      if (0 && type->array_len >= 0) {
         /* The type was something like "float[4]" */
         convert_to_array(C, var, &type->specifier);
         var->array_len = type->array_len;
      }
      else {
         if (!slang_type_specifier_copy(&var->type.specifier, &type->specifier))
            RETURN0;
      }
      var->initializer =
         (slang_operation *) _slang_alloc(sizeof(slang_operation));
      if (var->initializer == NULL) {
         slang_info_log_memory(C->L);
         RETURN0;
      }
      if (!slang_operation_construct(var->initializer)) {
         _slang_free(var->initializer);
         var->initializer = NULL;
         slang_info_log_memory(C->L);
         RETURN0;
      }
      if (!parse_expression(C, O, var->initializer))
         RETURN0;
      break;
   case VARIABLE_ARRAY_UNKNOWN:
      /* unsized array - mark it as array and copy the specifier to
       * the array element
       */
      if (type->array_len >= 0) {
         slang_info_log_error(C->L, "multi-dimensional arrays not allowed");
         RETURN0;
      }
      if (!convert_to_array(C, var, &type->specifier))
         return GL_FALSE;
      break;
   case VARIABLE_ARRAY_EXPLICIT:
      if (type->array_len >= 0) {
         /* the user is trying to do something like: float[2] x[3]; */
         slang_info_log_error(C->L, "multi-dimensional arrays not allowed");
         RETURN0;
      }
      if (!convert_to_array(C, var, &type->specifier))
         return GL_FALSE;
      if (!parse_array_len(C, O, &var->array_len))
         return GL_FALSE;
      break;
   default:
      RETURN0;
   }

   /* allocate global address space for a variable with a known size */
   if (C->global_scope
       && !(var->type.specifier.type == SLANG_SPEC_ARRAY
            && var->array_len == 0)) {
      if (!calculate_var_size(C, O, var))
         return GL_FALSE;
   }

   /* emit code for global var decl */
   if (C->global_scope) {
      slang_assemble_ctx A;
      A.atoms = C->atoms;
      A.space.funcs = O->funs;
      A.space.structs = O->structs;
      A.space.vars = O->vars;
      A.program = O->program;
      A.pragmas = O->pragmas;
      A.vartable = O->vartable;
      A.log = C->L;
      A.curFuncEndLabel = NULL;
      if (!_slang_codegen_global_variable(&A, var, C->type))
         RETURN0;
   }

   /* initialize global variable */
   if (C->global_scope) {
      if (var->initializer != NULL) {
         slang_assemble_ctx A;

         A.atoms = C->atoms;
         A.space.funcs = O->funs;
         A.space.structs = O->structs;
         A.space.vars = O->vars;
         if (!initialize_global(&A, var))
            RETURN0;
      }
   }
   return 1;
}

/**
 * Parse a list of variable declarations.  Each variable may have an
 * initializer.
 */
static int
parse_init_declarator_list(slang_parse_ctx * C, slang_output_ctx * O)
{
   slang_fully_specified_type type;

   /* parse the fully specified type, common to all declarators */
   if (!slang_fully_specified_type_construct(&type))
      RETURN0;
   if (!parse_fully_specified_type(C, O, &type)) {
      slang_fully_specified_type_destruct(&type);
      RETURN0;
   }

   /* parse declarators, pass-in the parsed type */
   do {
      if (!parse_init_declarator(C, O, &type)) {
         slang_fully_specified_type_destruct(&type);
         RETURN0;
      }
   }
   while (*C->I++ == DECLARATOR_NEXT);

   slang_fully_specified_type_destruct(&type);
   return 1;
}


/**
 * Parse a function definition or declaration.
 * \param C  parsing context
 * \param O  output context
 * \param definition if non-zero expect a definition, else a declaration
 * \param parsed_func_ret  returns the parsed function
 * \return GL_TRUE if success, GL_FALSE if failure
 */
static GLboolean
parse_function(slang_parse_ctx * C, slang_output_ctx * O, int definition,
               slang_function ** parsed_func_ret)
{
   slang_function parsed_func, *found_func;

   /* parse function definition/declaration */
   if (!slang_function_construct(&parsed_func))
      return GL_FALSE;
   if (definition) {
      if (!parse_function_definition(C, O, &parsed_func)) {
         slang_function_destruct(&parsed_func);
         return GL_FALSE;
      }
   }
   else {
      if (!parse_function_prototype(C, O, &parsed_func)) {
         slang_function_destruct(&parsed_func);
         return GL_FALSE;
      }
   }

   /* find a function with a prototype matching the parsed one - only
    * the current scope is being searched to allow built-in function
    * overriding
    */
   found_func = slang_function_scope_find(O->funs, &parsed_func, 0);
   if (found_func == NULL) {
      /* New function, add it to the function list */
      O->funs->functions =
         (slang_function *) _slang_realloc(O->funs->functions,
                                           O->funs->num_functions
                                           * sizeof(slang_function),
                                           (O->funs->num_functions + 1)
                                           * sizeof(slang_function));
      if (O->funs->functions == NULL) {
         slang_info_log_memory(C->L);
         slang_function_destruct(&parsed_func);
         return GL_FALSE;
      }
      O->funs->functions[O->funs->num_functions] = parsed_func;
      O->funs->num_functions++;

      /* return the newly parsed function */
      *parsed_func_ret = &O->funs->functions[O->funs->num_functions - 1];
   }
   else {
      /* previously defined or declared */
      /* TODO: check function return type qualifiers and specifiers */
      if (definition) {
         if (found_func->body != NULL) {
            slang_info_log_error(C->L, "%s: function already has a body.",
                                 slang_atom_pool_id(C->atoms,
                                                    parsed_func.header.
                                                    a_name));
            slang_function_destruct(&parsed_func);
            return GL_FALSE;
         }

         /* destroy the existing function declaration and replace it
          * with the new one
          */
         slang_function_destruct(found_func);
         *found_func = parsed_func;
      }
      else {
         /* another declaration of the same function prototype - ignore it */
         slang_function_destruct(&parsed_func);
      }

      /* return the found function */
      *parsed_func_ret = found_func;
   }

   return GL_TRUE;
}

/* declaration */
#define DECLARATION_FUNCTION_PROTOTYPE 1
#define DECLARATION_INIT_DECLARATOR_LIST 2

static int
parse_declaration(slang_parse_ctx * C, slang_output_ctx * O)
{
   switch (*C->I++) {
   case DECLARATION_INIT_DECLARATOR_LIST:
      if (!parse_init_declarator_list(C, O))
         RETURN0;
      break;
   case DECLARATION_FUNCTION_PROTOTYPE:
      {
         slang_function *dummy_func;

         if (!parse_function(C, O, 0, &dummy_func))
            RETURN0;
      }
      break;
   default:
      RETURN0;
   }
   return 1;
}

static int
parse_default_precision(slang_parse_ctx * C, slang_output_ctx * O)
{
   int precision, type;

   if (!O->allow_precision) {
      slang_info_log_error(C->L, "syntax error at \"precision\"");
      RETURN0;
   }

   precision = *C->I++;
   switch (precision) {
   case PRECISION_LOW:
   case PRECISION_MEDIUM:
   case PRECISION_HIGH:
      /* OK */
      break;
   default:
      _mesa_problem(NULL, "unexpected precision %d at %s:%d\n",
                    precision, __FILE__, __LINE__);
      RETURN0;
   }

   type = *C->I++;
   switch (type) {
   case TYPE_SPECIFIER_FLOAT:
   case TYPE_SPECIFIER_INT:
   case TYPE_SPECIFIER_SAMPLER1D:
   case TYPE_SPECIFIER_SAMPLER2D:
   case TYPE_SPECIFIER_SAMPLER3D:
   case TYPE_SPECIFIER_SAMPLERCUBE:
   case TYPE_SPECIFIER_SAMPLER1DSHADOW:
   case TYPE_SPECIFIER_SAMPLER2DSHADOW:
   case TYPE_SPECIFIER_SAMPLER2DRECT:
   case TYPE_SPECIFIER_SAMPLER2DRECTSHADOW:
      /* OK */
      break;
   default:
      _mesa_problem(NULL, "unexpected type %d at %s:%d\n",
                    type, __FILE__, __LINE__);
      RETURN0;
   }

   assert(type < TYPE_SPECIFIER_COUNT);
   O->default_precision[type] = precision;

   return 1;
}


/**
 * Initialize the default precision for all types.
 * XXX this info isn't used yet.
 */
static void
init_default_precision(slang_output_ctx *O, slang_unit_type type)
{
   GLuint i;
   for (i = 0; i < TYPE_SPECIFIER_COUNT; i++) {
#if FEATURE_es2_glsl
      O->default_precision[i] = PRECISION_LOW;
#else
      O->default_precision[i] = PRECISION_HIGH;
#endif
   }

   if (type == SLANG_UNIT_VERTEX_SHADER) {
      O->default_precision[TYPE_SPECIFIER_FLOAT] = PRECISION_HIGH;
      O->default_precision[TYPE_SPECIFIER_INT] = PRECISION_HIGH;
   }
   else {
      O->default_precision[TYPE_SPECIFIER_INT] = PRECISION_MEDIUM;
   }
}


static int
parse_invariant(slang_parse_ctx * C, slang_output_ctx * O)
{
   if (O->allow_invariant) {
      slang_atom *a = parse_identifier(C);
      /* XXX not doing anything with this var yet */
      /*printf("ID: %s\n", (char*) a);*/
      return a ? 1 : 0;
   }
   else {
      slang_info_log_error(C->L, "syntax error at \"invariant\"");
      RETURN0;
   }
}
      

/* external declaration or default precision specifier */
#define EXTERNAL_NULL 0
#define EXTERNAL_FUNCTION_DEFINITION 1
#define EXTERNAL_DECLARATION 2
#define DEFAULT_PRECISION 3
#define INVARIANT_STMT 4


static GLboolean
parse_code_unit(slang_parse_ctx * C, slang_code_unit * unit,
                struct gl_shader *shader)
{
   GET_CURRENT_CONTEXT(ctx);
   slang_output_ctx o;
   GLboolean success;
   GLuint maxRegs;
   slang_function *mainFunc = NULL;

   if (unit->type == SLANG_UNIT_FRAGMENT_BUILTIN ||
       unit->type == SLANG_UNIT_FRAGMENT_SHADER) {
      maxRegs = ctx->Const.FragmentProgram.MaxTemps;
   }
   else {
      assert(unit->type == SLANG_UNIT_VERTEX_BUILTIN ||
             unit->type == SLANG_UNIT_VERTEX_SHADER);
      maxRegs = ctx->Const.VertexProgram.MaxTemps;
   }

   /* setup output context */
   o.funs = &unit->funs;
   o.structs = &unit->structs;
   o.vars = &unit->vars;
   o.program = shader ? shader->Program : NULL;
   o.pragmas = shader ? &shader->Pragmas : NULL;
   o.vartable = _slang_new_var_table(maxRegs);
   _slang_push_var_table(o.vartable);

   /* allow 'invariant' keyword? */
#if FEATURE_es2_glsl
   o.allow_invariant = GL_TRUE;
#else
   o.allow_invariant = (C->version >= 120) ? GL_TRUE : GL_FALSE;
#endif

   /* allow 'centroid' keyword? */
   o.allow_centroid = (C->version >= 120) ? GL_TRUE : GL_FALSE;

   /* allow 'lowp/mediump/highp' keywords? */
#if FEATURE_es2_glsl
   o.allow_precision = GL_TRUE;
#else
   o.allow_precision = (C->version >= 120) ? GL_TRUE : GL_FALSE;
#endif
   init_default_precision(&o, unit->type);

   /* allow 'float[]' keyword? */
   o.allow_array_types = (C->version >= 120) ? GL_TRUE : GL_FALSE;

   /* parse individual functions and declarations */
   while (*C->I != EXTERNAL_NULL) {
      switch (*C->I++) {
      case EXTERNAL_FUNCTION_DEFINITION:
         {
            slang_function *func;
            success = parse_function(C, &o, 1, &func);
            if (success &&
                _mesa_strcmp((char *) func->header.a_name, "main") == 0) {
               /* found main() */
               mainFunc = func;
            }
         }
         break;
      case EXTERNAL_DECLARATION:
         success = parse_declaration(C, &o);
         break;
      case DEFAULT_PRECISION:
         success = parse_default_precision(C, &o);
         break;
      case INVARIANT_STMT:
         success = parse_invariant(C, &o);
         break;
      default:
         success = GL_FALSE;
      }

      if (!success) {
         /* xxx free codegen */
         _slang_pop_var_table(o.vartable);
         return GL_FALSE;
      }
   }
   C->I++;

   if (mainFunc) {
      /* assemble (generate code) for main() */
      slang_assemble_ctx A;

      A.atoms = C->atoms;
      A.space.funcs = o.funs;
      A.space.structs = o.structs;
      A.space.vars = o.vars;
      A.program = o.program;
      A.pragmas = &shader->Pragmas;
      A.vartable = o.vartable;
      A.log = C->L;

      /* main() takes no parameters */
      if (mainFunc->param_count > 0) {
         slang_info_log_error(A.log, "main() takes no arguments");
         return GL_FALSE;
      }

      _slang_codegen_function(&A, mainFunc);

      shader->Main = GL_TRUE; /* this shader defines main() */
   }

   _slang_pop_var_table(o.vartable);
   _slang_delete_var_table(o.vartable);

   return GL_TRUE;
}

static GLboolean
compile_binary(const byte * prod, slang_code_unit * unit,
               GLuint version,
               slang_unit_type type, slang_info_log * infolog,
               slang_code_unit * builtin, slang_code_unit * downlink,
               struct gl_shader *shader)
{
   slang_parse_ctx C;

   unit->type = type;

   /* setup parse context */
   C.I = prod;
   C.L = infolog;
   C.parsing_builtin = (builtin == NULL);
   C.global_scope = GL_TRUE;
   C.atoms = &unit->object->atompool;
   C.type = type;
   C.version = version;

   if (!check_revision(&C))
      return GL_FALSE;

   if (downlink != NULL) {
      unit->vars.outer_scope = &downlink->vars;
      unit->funs.outer_scope = &downlink->funs;
      unit->structs.outer_scope = &downlink->structs;
   }

   /* parse translation unit */
   return parse_code_unit(&C, unit, shader);
}

static GLboolean
compile_with_grammar(grammar id, const char *source, slang_code_unit * unit,
                     slang_unit_type type, slang_info_log * infolog,
                     slang_code_unit * builtin,
                     struct gl_shader *shader,
                     const struct gl_extensions *extensions,
                     struct gl_sl_pragmas *pragmas)
{
   byte *prod;
   GLuint size, start, version;
   slang_string preprocessed;
   GLuint maxVersion;

#if FEATURE_ARB_shading_language_120
   maxVersion = 120;
#elif FEATURE_es2_glsl
   maxVersion = 100;
#else
   maxVersion = 110;
#endif

   /* First retrieve the version number. */
   if (!_slang_preprocess_version(source, &version, &start, infolog))
      return GL_FALSE;

   if (version > maxVersion) {
      slang_info_log_error(infolog,
                           "language version %.2f is not supported.",
                           version * 0.01);
      return GL_FALSE;
   }

   /* Now preprocess the source string. */
   slang_string_init(&preprocessed);
   if (!_slang_preprocess_directives(&preprocessed, &source[start],
                                     infolog, extensions, pragmas)) {
      slang_string_free(&preprocessed);
      slang_info_log_error(infolog, "failed to preprocess the source.");
      return GL_FALSE;
   }

   /* Finally check the syntax and generate its binary representation. */
   if (!grammar_fast_check(id,
                           (const byte *) (slang_string_cstr(&preprocessed)),
                           &prod, &size, 65536)) {
      char buf[1024];
      GLint pos;

      slang_string_free(&preprocessed);
      grammar_get_last_error((byte *) (buf), sizeof(buf), &pos);
      slang_info_log_error(infolog, buf);
      /* syntax error (possibly in library code) */
#if 0
      {
         int line, col;
         char *s;
         s = (char *) _mesa_find_line_column((const GLubyte *) source,
                                             (const GLubyte *) source + pos,
                                             &line, &col);
         printf("Error on line %d, col %d: %s\n", line, col, s);
      }
#endif
      return GL_FALSE;
   }
   slang_string_free(&preprocessed);

   /* Syntax is okay - translate it to internal representation. */
   if (!compile_binary(prod, unit, version, type, infolog, builtin,
                       &builtin[SLANG_BUILTIN_TOTAL - 1],
                       shader)) {
      grammar_alloc_free(prod);
      return GL_FALSE;
   }
   grammar_alloc_free(prod);
   return GL_TRUE;
}

LONGSTRING static const char *slang_shader_syn =
#include "library/slang_shader_syn.h"
   ;

static const byte slang_core_gc[] = {
#include "library/slang_core_gc.h"
};

static const byte slang_120_core_gc[] = {
#include "library/slang_120_core_gc.h"
};

static const byte slang_120_fragment_gc[] = {
#include "library/slang_builtin_120_fragment_gc.h"
};

static const byte slang_common_builtin_gc[] = {
#include "library/slang_common_builtin_gc.h"
};

static const byte slang_fragment_builtin_gc[] = {
#include "library/slang_fragment_builtin_gc.h"
};

static const byte slang_vertex_builtin_gc[] = {
#include "library/slang_vertex_builtin_gc.h"
};

static GLboolean
compile_object(grammar * id, const char *source, slang_code_object * object,
               slang_unit_type type, slang_info_log * infolog,
               struct gl_shader *shader,
               const struct gl_extensions *extensions,
               struct gl_sl_pragmas *pragmas)
{
   slang_code_unit *builtins = NULL;
   GLuint base_version = 110;

   /* load GLSL grammar */
   *id = grammar_load_from_text((const byte *) (slang_shader_syn));
   if (*id == 0) {
      byte buf[1024];
      int pos;

      grammar_get_last_error(buf, 1024, &pos);
      slang_info_log_error(infolog, (const char *) (buf));
      return GL_FALSE;
   }

   /* set shader type - the syntax is slightly different for different shaders */
   if (type == SLANG_UNIT_FRAGMENT_SHADER
       || type == SLANG_UNIT_FRAGMENT_BUILTIN)
      grammar_set_reg8(*id, (const byte *) "shader_type", 1);
   else
      grammar_set_reg8(*id, (const byte *) "shader_type", 2);

   /* enable language extensions */
   grammar_set_reg8(*id, (const byte *) "parsing_builtin", 1);

   /* if parsing user-specified shader, load built-in library */
   if (type == SLANG_UNIT_FRAGMENT_SHADER || type == SLANG_UNIT_VERTEX_SHADER) {
      /* compile core functionality first */
      if (!compile_binary(slang_core_gc,
                          &object->builtin[SLANG_BUILTIN_CORE],
                          base_version,
                          SLANG_UNIT_FRAGMENT_BUILTIN, infolog,
                          NULL, NULL, NULL))
         return GL_FALSE;

#if FEATURE_ARB_shading_language_120
      if (!compile_binary(slang_120_core_gc,
                          &object->builtin[SLANG_BUILTIN_120_CORE],
                          120,
                          SLANG_UNIT_FRAGMENT_BUILTIN, infolog,
                          NULL, &object->builtin[SLANG_BUILTIN_CORE], NULL))
         return GL_FALSE;
#endif

      /* compile common functions and variables, link to core */
      if (!compile_binary(slang_common_builtin_gc,
                          &object->builtin[SLANG_BUILTIN_COMMON],
#if FEATURE_ARB_shading_language_120
                          120,
#else
                          base_version,
#endif
                          SLANG_UNIT_FRAGMENT_BUILTIN, infolog, NULL,
#if FEATURE_ARB_shading_language_120
                          &object->builtin[SLANG_BUILTIN_120_CORE],
#else
                          &object->builtin[SLANG_BUILTIN_CORE],
#endif
                          NULL))
         return GL_FALSE;

      /* compile target-specific functions and variables, link to common */
      if (type == SLANG_UNIT_FRAGMENT_SHADER) {
         if (!compile_binary(slang_fragment_builtin_gc,
                             &object->builtin[SLANG_BUILTIN_TARGET],
                             base_version,
                             SLANG_UNIT_FRAGMENT_BUILTIN, infolog, NULL,
                             &object->builtin[SLANG_BUILTIN_COMMON], NULL))
            return GL_FALSE;
#if FEATURE_ARB_shading_language_120
         if (!compile_binary(slang_120_fragment_gc,
                             &object->builtin[SLANG_BUILTIN_TARGET],
                             120,
                             SLANG_UNIT_FRAGMENT_BUILTIN, infolog, NULL,
                             &object->builtin[SLANG_BUILTIN_COMMON], NULL))
            return GL_FALSE;
#endif
      }
      else if (type == SLANG_UNIT_VERTEX_SHADER) {
         if (!compile_binary(slang_vertex_builtin_gc,
                             &object->builtin[SLANG_BUILTIN_TARGET],
                             base_version,
                             SLANG_UNIT_VERTEX_BUILTIN, infolog, NULL,
                             &object->builtin[SLANG_BUILTIN_COMMON], NULL))
            return GL_FALSE;
      }

      /* disable language extensions */
#if NEW_SLANG /* allow-built-ins */
      grammar_set_reg8(*id, (const byte *) "parsing_builtin", 1);
#else
      grammar_set_reg8(*id, (const byte *) "parsing_builtin", 0);
#endif
      builtins = object->builtin;
   }

   /* compile the actual shader - pass-in built-in library for external shader */
   return compile_with_grammar(*id, source, &object->unit, type, infolog,
                               builtins, shader, extensions, pragmas);
}


static GLboolean
compile_shader(GLcontext *ctx, slang_code_object * object,
               slang_unit_type type, slang_info_log * infolog,
               struct gl_shader *shader)
{
   GLboolean success;
   grammar id = 0;

#if 0 /* for debug */
   _mesa_printf("********* COMPILE SHADER ***********\n");
   _mesa_printf("%s\n", shader->Source);
   _mesa_printf("************************************\n");
#endif

   assert(shader->Program);

   _slang_code_object_dtr(object);
   _slang_code_object_ctr(object);

   success = compile_object(&id, shader->Source, object, type, infolog, shader,
                            &ctx->Extensions, &shader->Pragmas);
   if (id != 0)
      grammar_destroy(id);
   if (!success)
      return GL_FALSE;

   return GL_TRUE;
}



GLboolean
_slang_compile(GLcontext *ctx, struct gl_shader *shader)
{
   GLboolean success;
   slang_info_log info_log;
   slang_code_object obj;
   slang_unit_type type;

   if (shader->Type == GL_VERTEX_SHADER) {
      type = SLANG_UNIT_VERTEX_SHADER;
   }
   else {
      assert(shader->Type == GL_FRAGMENT_SHADER);
      type = SLANG_UNIT_FRAGMENT_SHADER;
   }

   if (!shader->Source)
      return GL_FALSE;

   ctx->Shader.MemPool = _slang_new_mempool(1024*1024);

   shader->Main = GL_FALSE;

   if (!shader->Program) {
      GLenum progTarget;
      if (shader->Type == GL_VERTEX_SHADER)
         progTarget = GL_VERTEX_PROGRAM_ARB;
      else
         progTarget = GL_FRAGMENT_PROGRAM_ARB;
      shader->Program = ctx->Driver.NewProgram(ctx, progTarget, 1);
      shader->Program->Parameters = _mesa_new_parameter_list();
      shader->Program->Varying = _mesa_new_parameter_list();
      shader->Program->Attributes = _mesa_new_parameter_list();
   }

   slang_info_log_construct(&info_log);
   _slang_code_object_ctr(&obj);

   success = compile_shader(ctx, &obj, type, &info_log, shader);

   /* free shader's prev info log */
   if (shader->InfoLog) {
      _mesa_free(shader->InfoLog);
      shader->InfoLog = NULL;
   }

   if (info_log.text) {
      /* copy info-log string to shader object */
      shader->InfoLog = _mesa_strdup(info_log.text);
   }

   if (info_log.error_flag) {
      success = GL_FALSE;
   }

   slang_info_log_destruct(&info_log);
   _slang_code_object_dtr(&obj);

   _slang_delete_mempool((slang_mempool *) ctx->Shader.MemPool);
   ctx->Shader.MemPool = NULL;

   /* remove any reads of output registers */
#if 0
   printf("Pre-remove output reads:\n");
   _mesa_print_program(shader->Program);
#endif
   _mesa_remove_output_reads(shader->Program, PROGRAM_OUTPUT);
   if (shader->Type == GL_VERTEX_SHADER) {
      /* and remove writes to varying vars in vertex programs */
      _mesa_remove_output_reads(shader->Program, PROGRAM_VARYING);
   }
#if 0
   printf("Post-remove output reads:\n");
   _mesa_print_program(shader->Program);
#endif

   return success;
}


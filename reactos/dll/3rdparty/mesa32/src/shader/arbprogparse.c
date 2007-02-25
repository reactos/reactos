/*
 * Mesa 3-D graphics library
 * Version:  6.4
 *
 * Copyright (C) 1999-2005  Brian Paul   All Rights Reserved.
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

#define DEBUG_PARSING 0

/**
 * \file arbprogparse.c
 * ARB_*_program parser core
 * \author Karl Rasche
 */

#include "mtypes.h"
#include "glheader.h"
#include "context.h"
#include "hash.h"
#include "imports.h"
#include "macros.h"
#include "program.h"
#include "nvvertprog.h"
#include "nvfragprog.h"
#include "arbprogparse.h"
#include "grammar_mesa.h"

#include "dispatch.h"

#ifndef __extension__
#if !defined(__GNUC__) || (__GNUC__ < 2) || \
    ((__GNUC__ == 2) && (__GNUC_MINOR__ <= 7))
# define __extension__
#endif
#endif

/* TODO:
 *    Fragment Program Stuff:
 *    -----------------------------------------------------
 *
 *    - things from Michal's email
 *       + overflow on atoi
 *       + not-overflowing floats (don't use parse_integer..)
 *       + can remove range checking in arbparse.c
 *
 *    - check all limits of number of various variables
 *      + parameters
 *
 *    - test! test! test!
 *
 *    Vertex Program Stuff:
 *    -----------------------------------------------------
 *    - Optimize param array usage and count limits correctly, see spec,
 *         section 2.14.3.7
 *       + Record if an array is reference absolutly or relatively (or both)
 *       + For absolute arrays, store a bitmap of accesses
 *       + For single parameters, store an access flag
 *       + After parsing, make a parameter cleanup and merging pass, where
 *           relative arrays are layed out first, followed by abs arrays, and
 *           finally single state.
 *       + Remap offsets for param src and dst registers
 *       + Now we can properly count parameter usage
 *
 *    - Multiple state binding errors in param arrays (see spec, just before
 *         section 2.14.3.3)
 *    - grep for XXX
 *
 *    Mesa Stuff
 *    -----------------------------------------------------
 *    - User clipping planes vs. PositionInvariant
 *    - Is it sufficient to just multiply by the mvp to transform in the
 *        PositionInvariant case? Or do we need something more involved?
 *
 *    - vp_src swizzle is GLubyte, fp_src swizzle is GLuint
 *    - fetch state listed in program_parameters list
 *       + WTF should this go???
 *       + currently in nvvertexec.c and s_nvfragprog.c
 *
 *    - allow for multiple address registers (and fetch address regs properly)
 *
 *    Cosmetic Stuff
 *    -----------------------------------------------------
 * 	- remove any leftover unused grammer.c stuff (dict_ ?)
 * 	- fix grammer.c error handling so its not static
 * 	- #ifdef around stuff pertaining to extentions
 *
 *    Outstanding Questions:
 *    -----------------------------------------------------
 *    - ARB_matrix_palette / ARB_vertex_blend -- not supported
 *      what gets hacked off because of this:
 *       + VERTEX_ATTRIB_MATRIXINDEX
 *       + VERTEX_ATTRIB_WEIGHT
 *       + MATRIX_MODELVIEW
 *       + MATRIX_PALETTE
 *
 *    - When can we fetch env/local params from their own register files, and
 *      when to we have to fetch them into the main state register file?
 *      (think arrays)
 *
 *    Grammar Changes:
 *    -----------------------------------------------------
 */

/* Changes since moving the file to shader directory

2004-III-4 ------------------------------------------------------------
- added #include "grammar_mesa.h"
- removed grammar specific code part (it resides now in grammar.c)
- added GL_ARB_fragment_program_shadow tokens
- modified #include "arbparse_syn.h"
- major changes inside _mesa_parse_arb_program()
- check the program string for '\0' characters
- copy the program string to a one-byte-longer location to have
  it null-terminated
- position invariance test (not writing to result.position) moved
  to syntax part
*/

typedef GLubyte *production;

/**
 * This is the text describing the rules to parse the grammar
 */
__extension__ static char arb_grammar_text[] =
#include "arbprogram_syn.h"
;

/**
 * These should match up with the values defined in arbprogram.syn
 */

/*
    Changes:
    - changed and merged V_* and F_* opcode values to OP_*.
    - added GL_ARB_fragment_program_shadow specific tokens (michal)
*/
#define  REVISION                                   0x09

/* program type */
#define  FRAGMENT_PROGRAM                           0x01
#define  VERTEX_PROGRAM                             0x02

/* program section */
#define  OPTION                                     0x01
#define  INSTRUCTION                                0x02
#define  DECLARATION                                0x03
#define  END                                        0x04

/* GL_ARB_fragment_program option */
#define  ARB_PRECISION_HINT_FASTEST                 0x00
#define  ARB_PRECISION_HINT_NICEST                  0x01
#define  ARB_FOG_EXP                                0x02
#define  ARB_FOG_EXP2                               0x03
#define  ARB_FOG_LINEAR                             0x04

/* GL_ARB_vertex_program option */
#define  ARB_POSITION_INVARIANT                     0x05

/* GL_ARB_fragment_program_shadow option */
#define  ARB_FRAGMENT_PROGRAM_SHADOW                0x06

/* GL_ARB_draw_buffers option */
#define  ARB_DRAW_BUFFERS                           0x07

/* GL_ARB_fragment_program instruction class */
#define  OP_ALU_INST                                0x00
#define  OP_TEX_INST                                0x01

/* GL_ARB_vertex_program instruction class */
/*       OP_ALU_INST */

/* GL_ARB_fragment_program instruction type */
#define  OP_ALU_VECTOR                               0x00
#define  OP_ALU_SCALAR                               0x01
#define  OP_ALU_BINSC                                0x02
#define  OP_ALU_BIN                                  0x03
#define  OP_ALU_TRI                                  0x04
#define  OP_ALU_SWZ                                  0x05
#define  OP_TEX_SAMPLE                               0x06
#define  OP_TEX_KIL                                  0x07

/* GL_ARB_vertex_program instruction type */
#define  OP_ALU_ARL                                  0x08
/*       OP_ALU_VECTOR */
/*       OP_ALU_SCALAR */
/*       OP_ALU_BINSC */
/*       OP_ALU_BIN */
/*       OP_ALU_TRI */
/*       OP_ALU_SWZ */

/* GL_ARB_fragment_program instruction code */
#define  OP_ABS                                     0x00
#define  OP_ABS_SAT                                 0x1B
#define  OP_FLR                                     0x09
#define  OP_FLR_SAT                                 0x26
#define  OP_FRC                                     0x0A
#define  OP_FRC_SAT                                 0x27
#define  OP_LIT                                     0x0C
#define  OP_LIT_SAT                                 0x2A
#define  OP_MOV                                     0x11
#define  OP_MOV_SAT                                 0x30
#define  OP_COS                                     0x1F
#define  OP_COS_SAT                                 0x20
#define  OP_EX2                                     0x07
#define  OP_EX2_SAT                                 0x25
#define  OP_LG2                                     0x0B
#define  OP_LG2_SAT                                 0x29
#define  OP_RCP                                     0x14
#define  OP_RCP_SAT                                 0x33
#define  OP_RSQ                                     0x15
#define  OP_RSQ_SAT                                 0x34
#define  OP_SIN                                     0x38
#define  OP_SIN_SAT                                 0x39
#define  OP_SCS                                     0x35
#define  OP_SCS_SAT                                 0x36
#define  OP_POW                                     0x13
#define  OP_POW_SAT                                 0x32
#define  OP_ADD                                     0x01
#define  OP_ADD_SAT                                 0x1C
#define  OP_DP3                                     0x03
#define  OP_DP3_SAT                                 0x21
#define  OP_DP4                                     0x04
#define  OP_DP4_SAT                                 0x22
#define  OP_DPH                                     0x05
#define  OP_DPH_SAT                                 0x23
#define  OP_DST                                     0x06
#define  OP_DST_SAT                                 0x24
#define  OP_MAX                                     0x0F
#define  OP_MAX_SAT                                 0x2E
#define  OP_MIN                                     0x10
#define  OP_MIN_SAT                                 0x2F
#define  OP_MUL                                     0x12
#define  OP_MUL_SAT                                 0x31
#define  OP_SGE                                     0x16
#define  OP_SGE_SAT                                 0x37
#define  OP_SLT                                     0x17
#define  OP_SLT_SAT                                 0x3A
#define  OP_SUB                                     0x18
#define  OP_SUB_SAT                                 0x3B
#define  OP_XPD                                     0x1A
#define  OP_XPD_SAT                                 0x43
#define  OP_CMP                                     0x1D
#define  OP_CMP_SAT                                 0x1E
#define  OP_LRP                                     0x2B
#define  OP_LRP_SAT                                 0x2C
#define  OP_MAD                                     0x0E
#define  OP_MAD_SAT                                 0x2D
#define  OP_SWZ                                     0x19
#define  OP_SWZ_SAT                                 0x3C
#define  OP_TEX                                     0x3D
#define  OP_TEX_SAT                                 0x3E
#define  OP_TXB                                     0x3F
#define  OP_TXB_SAT                                 0x40
#define  OP_TXP                                     0x41
#define  OP_TXP_SAT                                 0x42
#define  OP_KIL                                     0x28

/* GL_ARB_vertex_program instruction code */
#define  OP_ARL                                     0x02
/*       OP_ABS */
/*       OP_FLR */
/*       OP_FRC */
/*       OP_LIT */
/*       OP_MOV */
/*       OP_EX2 */
#define  OP_EXP                                     0x08
/*       OP_LG2 */
#define  OP_LOG                                     0x0D
/*       OP_RCP */
/*       OP_RSQ */
/*       OP_POW */
/*       OP_ADD */
/*       OP_DP3 */
/*       OP_DP4 */
/*       OP_DPH */
/*       OP_DST */
/*       OP_MAX */
/*       OP_MIN */
/*       OP_MUL */
/*       OP_SGE */
/*       OP_SLT */
/*       OP_SUB */
/*       OP_XPD */
/*       OP_MAD */
/*       OP_SWZ */

/* fragment attribute binding */
#define  FRAGMENT_ATTRIB_COLOR                      0x01
#define  FRAGMENT_ATTRIB_TEXCOORD                   0x02
#define  FRAGMENT_ATTRIB_FOGCOORD                   0x03
#define  FRAGMENT_ATTRIB_POSITION                   0x04

/* vertex attribute binding */
#define  VERTEX_ATTRIB_POSITION                     0x01
#define  VERTEX_ATTRIB_WEIGHT                       0x02
#define  VERTEX_ATTRIB_NORMAL                       0x03
#define  VERTEX_ATTRIB_COLOR                        0x04
#define  VERTEX_ATTRIB_FOGCOORD                     0x05
#define  VERTEX_ATTRIB_TEXCOORD                     0x06
#define  VERTEX_ATTRIB_MATRIXINDEX                  0x07
#define  VERTEX_ATTRIB_GENERIC                      0x08

/* fragment result binding */
#define  FRAGMENT_RESULT_COLOR                      0x01
#define  FRAGMENT_RESULT_DEPTH                      0x02

/* vertex result binding */
#define  VERTEX_RESULT_POSITION                     0x01
#define  VERTEX_RESULT_COLOR                        0x02
#define  VERTEX_RESULT_FOGCOORD                     0x03
#define  VERTEX_RESULT_POINTSIZE                    0x04
#define  VERTEX_RESULT_TEXCOORD                     0x05

/* texture target */
#define  TEXTARGET_1D                               0x01
#define  TEXTARGET_2D                               0x02
#define  TEXTARGET_3D                               0x03
#define  TEXTARGET_RECT                             0x04
#define  TEXTARGET_CUBE                             0x05
/* GL_ARB_fragment_program_shadow */
#define  TEXTARGET_SHADOW1D                         0x06
#define  TEXTARGET_SHADOW2D                         0x07
#define  TEXTARGET_SHADOWRECT                       0x08

/* face type */
#define  FACE_FRONT                                 0x00
#define  FACE_BACK                                  0x01

/* color type */
#define  COLOR_PRIMARY                              0x00
#define  COLOR_SECONDARY                            0x01

/* component */
#define  COMPONENT_X                                0x00
#define  COMPONENT_Y                                0x01
#define  COMPONENT_Z                                0x02
#define  COMPONENT_W                                0x03
#define  COMPONENT_0                                0x04
#define  COMPONENT_1                                0x05

/* array index type */
#define  ARRAY_INDEX_ABSOLUTE                       0x00
#define  ARRAY_INDEX_RELATIVE                       0x01

/* matrix name */
#define  MATRIX_MODELVIEW                           0x01
#define  MATRIX_PROJECTION                          0x02
#define  MATRIX_MVP                                 0x03
#define  MATRIX_TEXTURE                             0x04
#define  MATRIX_PALETTE                             0x05
#define  MATRIX_PROGRAM                             0x06

/* matrix modifier */
#define  MATRIX_MODIFIER_IDENTITY                   0x00
#define  MATRIX_MODIFIER_INVERSE                    0x01
#define  MATRIX_MODIFIER_TRANSPOSE                  0x02
#define  MATRIX_MODIFIER_INVTRANS                   0x03

/* constant type */
#define  CONSTANT_SCALAR                            0x01
#define  CONSTANT_VECTOR                            0x02

/* program param type */
#define  PROGRAM_PARAM_ENV                          0x01
#define  PROGRAM_PARAM_LOCAL                        0x02

/* register type */
#define  REGISTER_ATTRIB                            0x01
#define  REGISTER_PARAM                             0x02
#define  REGISTER_RESULT                            0x03
#define  REGISTER_ESTABLISHED_NAME                  0x04

/* param binding */
#define  PARAM_NULL                                 0x00
#define  PARAM_ARRAY_ELEMENT                        0x01
#define  PARAM_STATE_ELEMENT                        0x02
#define  PARAM_PROGRAM_ELEMENT                      0x03
#define  PARAM_PROGRAM_ELEMENTS                     0x04
#define  PARAM_CONSTANT                             0x05

/* param state property */
#define  STATE_MATERIAL_PARSER                      0x01
#define  STATE_LIGHT_PARSER                         0x02
#define  STATE_LIGHT_MODEL                          0x03
#define  STATE_LIGHT_PROD                           0x04
#define  STATE_FOG                                  0x05
#define  STATE_MATRIX_ROWS                          0x06
/* GL_ARB_fragment_program */
#define  STATE_TEX_ENV                              0x07
#define  STATE_DEPTH                                0x08
/* GL_ARB_vertex_program */
#define  STATE_TEX_GEN                              0x09
#define  STATE_CLIP_PLANE                           0x0A
#define  STATE_POINT                                0x0B

/* state material property */
#define  MATERIAL_AMBIENT                           0x01
#define  MATERIAL_DIFFUSE                           0x02
#define  MATERIAL_SPECULAR                          0x03
#define  MATERIAL_EMISSION                          0x04
#define  MATERIAL_SHININESS                         0x05

/* state light property */
#define  LIGHT_AMBIENT                              0x01
#define  LIGHT_DIFFUSE                              0x02
#define  LIGHT_SPECULAR                             0x03
#define  LIGHT_POSITION                             0x04
#define  LIGHT_ATTENUATION                          0x05
#define  LIGHT_HALF                                 0x06
#define  LIGHT_SPOT_DIRECTION                       0x07

/* state light model property */
#define  LIGHT_MODEL_AMBIENT                        0x01
#define  LIGHT_MODEL_SCENECOLOR                     0x02

/* state light product property */
#define  LIGHT_PROD_AMBIENT                         0x01
#define  LIGHT_PROD_DIFFUSE                         0x02
#define  LIGHT_PROD_SPECULAR                        0x03

/* state texture environment property */
#define  TEX_ENV_COLOR                              0x01

/* state texture generation coord property */
#define  TEX_GEN_EYE                                0x01
#define  TEX_GEN_OBJECT                             0x02

/* state fog property */
#define  FOG_COLOR                                  0x01
#define  FOG_PARAMS                                 0x02

/* state depth property */
#define  DEPTH_RANGE                                0x01

/* state point parameters property */
#define  POINT_SIZE                                 0x01
#define  POINT_ATTENUATION                          0x02

/* declaration */
#define  ATTRIB                                     0x01
#define  PARAM                                      0x02
#define  TEMP                                       0x03
#define  OUTPUT                                     0x04
#define  ALIAS                                      0x05
/* GL_ARB_vertex_program */
#define  ADDRESS                                    0x06

/*-----------------------------------------------------------------------
 * From here on down is the semantic checking portion
 *
 */

/**
 * Variable Table Handling functions
 */
typedef enum
{
   vt_none,
   vt_address,
   vt_attrib,
   vt_param,
   vt_temp,
   vt_output,
   vt_alias
} var_type;


/*
 * Setting an explicit field for each of the binding properties is a bit wasteful
 * of space, but it should be much more clear when reading later on..
 */
struct var_cache
{
   GLubyte *name;
   var_type type;
   GLuint address_binding;      /* The index of the address register we should
                                 * be using                                        */
   GLuint attrib_binding;       /* For type vt_attrib, see nvfragprog.h for values */
   GLuint attrib_binding_idx;   /* The index into the attrib register file corresponding
                                 * to the state in attrib_binding                  */
   GLuint attrib_is_generic;    /* If the attrib was specified through a generic
                                 * vertex attrib                                   */
   GLuint temp_binding;         /* The index of the temp register we are to use    */
   GLuint output_binding;       /* For type vt_output, see nvfragprog.h for values */
   GLuint output_binding_idx;   /* This is the index into the result register file
                                 * corresponding to the bound result state         */
   struct var_cache *alias_binding;     /* For type vt_alias, points to the var_cache entry
                                         * that this is aliased to                         */
   GLuint param_binding_type;   /* {PROGRAM_STATE_VAR, PROGRAM_LOCAL_PARAM,
                                 *    PROGRAM_ENV_PARAM}                           */
   GLuint param_binding_begin;  /* This is the offset into the program_parameter_list where
                                 * the tokens representing our bound state (or constants)
                                 * start */
   GLuint param_binding_length; /* This is how many entries in the the program_parameter_list
                                 * we take up with our state tokens or constants. Note that
                                 * this is _not_ the same as the number of param registers
                                 * we eventually use */
   struct var_cache *next;
};

static GLvoid
var_cache_create (struct var_cache **va)
{
   *va = (struct var_cache *) _mesa_malloc (sizeof (struct var_cache));
   if (*va) {
      (**va).name = NULL;
      (**va).type = vt_none;
      (**va).attrib_binding = ~0;
      (**va).attrib_is_generic = 0;
      (**va).temp_binding = ~0;
      (**va).output_binding = ~0;
      (**va).output_binding_idx = ~0;
      (**va).param_binding_type = ~0;
      (**va).param_binding_begin = ~0;
      (**va).param_binding_length = ~0;
      (**va).alias_binding = NULL;
      (**va).next = NULL;
   }
}

static GLvoid
var_cache_destroy (struct var_cache **va)
{
   if (*va) {
      var_cache_destroy (&(**va).next);
      _mesa_free (*va);
      *va = NULL;
   }
}

static GLvoid
var_cache_append (struct var_cache **va, struct var_cache *nv)
{
   if (*va)
      var_cache_append (&(**va).next, nv);
   else
      *va = nv;
}

static struct var_cache *
var_cache_find (struct var_cache *va, GLubyte * name)
{
   /*struct var_cache *first = va;*/

   while (va) {
      if (!strcmp ( (const char*) name, (const char*) va->name)) {
         if (va->type == vt_alias)
            return va->alias_binding;
         return va;
      }

      va = va->next;
   }

   return NULL;
}

/**
 * constructs an integer from 4 GLubytes in LE format
 */
static GLuint
parse_position (GLubyte ** inst)
{
   GLuint value;

   value =  (GLuint) (*(*inst)++);
   value += (GLuint) (*(*inst)++) * 0x100;
   value += (GLuint) (*(*inst)++) * 0x10000;
   value += (GLuint) (*(*inst)++) * 0x1000000;

   return value;
}

/**
 * This will, given a string, lookup the string as a variable name in the
 * var cache. If the name is found, the var cache node corresponding to the
 * var name is returned. If it is not found, a new entry is allocated
 *
 * \param  I     Points into the binary array where the string identifier begins
 * \param  found 1 if the string was found in the var_cache, 0 if it was allocated
 * \return       The location on the var_cache corresponding the the string starting at I
 */
static struct var_cache *
parse_string (GLubyte ** inst, struct var_cache **vc_head,
              struct arb_program *Program, GLuint * found)
{
   GLubyte *i = *inst;
   struct var_cache *va = NULL;
   (void) Program;

   *inst += _mesa_strlen ((char *) i) + 1;

   va = var_cache_find (*vc_head, i);

   if (va) {
      *found = 1;
      return va;
   }

   *found = 0;
   var_cache_create (&va);
   va->name = i;

   var_cache_append (vc_head, va);

   return va;
}

static char *
parse_string_without_adding (GLubyte ** inst, struct arb_program *Program)
{
   GLubyte *i = *inst;
   (void) Program;
   
   *inst += _mesa_strlen ((char *) i) + 1;

   return (char *) i;
}

/**
 * \return -1 if we parse '-', return 1 otherwise
 */
static GLint
parse_sign (GLubyte ** inst)
{
   /*return *(*inst)++ != '+'; */

   if (**inst == '-') {
      (*inst)++;
      return -1;
   }
   else if (**inst == '+') {
      (*inst)++;
      return 1;
   }

   return 1;
}

/**
 * parses and returns signed integer
 */
static GLint
parse_integer (GLubyte ** inst, struct arb_program *Program)
{
   GLint sign;
   GLint value;

   /* check if *inst points to '+' or '-'
    * if yes, grab the sign and increment *inst
    */
   sign = parse_sign (inst);

   /* now check if *inst points to 0
    * if yes, increment the *inst and return the default value
    */
   if (**inst == 0) {
      (*inst)++;
      return 0;
   }

   /* parse the integer as you normally would do it */
   value = _mesa_atoi (parse_string_without_adding (inst, Program));

   /* now, after terminating 0 there is a position
    * to parse it - parse_position()
    */
   Program->Position = parse_position (inst);

   return value * sign;
}

/**
  Accumulate this string of digits, and return them as 
  a large integer represented in floating point (for range).
  If scale is not NULL, also accumulates a power-of-ten
  integer scale factor that represents the number of digits 
  in the string.
*/
static GLdouble
parse_float_string(GLubyte ** inst, struct arb_program *Program, GLdouble *scale)
{
   GLdouble value = 0.0;
   GLdouble oscale = 1.0;

   if (**inst == 0) { /* this string of digits is empty-- do nothing */
      (*inst)++;
   }
   else { /* nonempty string-- parse out the digits */
      while (**inst >= '0' && **inst <= '9') {
         GLubyte digit = *((*inst)++);
         value = value * 10.0 + (GLint) (digit - '0');
         oscale *= 10.0;
      }
      assert(**inst == 0); /* integer string should end with 0 */
      (*inst)++; /* skip over terminating 0 */
      Program->Position = parse_position(inst); /* skip position (from integer) */
   }
   if (scale)
      *scale = oscale;
   return value;
}

/**
  Parse an unsigned floating-point number from this stream of tokenized
  characters.  Example floating-point formats supported:
     12.34
     12
     0.34
     .34
     12.34e-4
 */
static GLfloat
parse_float (GLubyte ** inst, struct arb_program *Program)
{
   GLint exponent;
   GLdouble whole, fraction, fracScale = 1.0;

   whole = parse_float_string(inst, Program, 0);
   fraction = parse_float_string(inst, Program, &fracScale);
   
   /* Parse signed exponent */
   exponent = parse_integer(inst, Program);   /* This is the exponent */

   /* Assemble parts of floating-point number: */
   return (GLfloat) ((whole + fraction / fracScale) *
                     _mesa_pow(10.0, (GLfloat) exponent));
}


/**
 */
static GLfloat
parse_signed_float (GLubyte ** inst, struct arb_program *Program)
{
   GLint sign = parse_sign (inst);
   GLfloat value = parse_float (inst, Program);
   return value * sign;
}

/**
 * This picks out a constant value from the parsed array. The constant vector is r
 * returned in the *values array, which should be of length 4.
 *
 * \param values - The 4 component vector with the constant value in it
 */
static GLvoid
parse_constant (GLubyte ** inst, GLfloat *values, struct arb_program *Program,
                GLboolean use)
{
   GLuint components, i;


   switch (*(*inst)++) {
      case CONSTANT_SCALAR:
         if (use == GL_TRUE) {
            values[0] =
               values[1] =
               values[2] = values[3] = parse_float (inst, Program);
         }
         else {
            values[0] =
               values[1] =
               values[2] = values[3] = parse_signed_float (inst, Program);
         }

         break;
      case CONSTANT_VECTOR:
         values[0] = values[1] = values[2] = 0;
         values[3] = 1;
         components = *(*inst)++;
         for (i = 0; i < components; i++) {
            values[i] = parse_signed_float (inst, Program);
         }
         break;
   }
}

/**
 * \param offset The offset from the address register that we should
 *                address
 *
 * \return 0 on sucess, 1 on error
 */
static GLuint
parse_relative_offset (GLcontext *ctx, GLubyte **inst, struct arb_program *Program,
                        GLint *offset)
{
   *offset = parse_integer(inst, Program);
   return 0;
}

/**
 * \param  color 0 if color type is primary, 1 if color type is secondary
 * \return 0 on sucess, 1 on error
 */
static GLuint
parse_color_type (GLcontext * ctx, GLubyte ** inst, struct arb_program *Program,
                  GLint * color)
{
   (void) ctx; (void) Program;
   *color = *(*inst)++ != COLOR_PRIMARY;
   return 0;
}

/**
 * Get an integer corresponding to a generic vertex attribute.
 *
 * \return 0 on sucess, 1 on error
 */
static GLuint
parse_generic_attrib_num(GLcontext *ctx, GLubyte ** inst,
                       struct arb_program *Program, GLuint *attrib)
{
   GLint i = parse_integer(inst, Program);

   if ((i < 0) || (i > MAX_VERTEX_PROGRAM_ATTRIBS))
   {
      _mesa_set_program_error (ctx, Program->Position,
                               "Invalid generic vertex attribute index");
      _mesa_error (ctx, GL_INVALID_OPERATION, "Invalid generic vertex attribute index");

      return 1;
   }

   *attrib = (GLuint) i;

   return 0;
}


/**
 * \param color The index of the color buffer to write into
 * \return 0 on sucess, 1 on error
 */
static GLuint
parse_output_color_num (GLcontext * ctx, GLubyte ** inst,
                    struct arb_program *Program, GLuint * color)
{
   GLint i = parse_integer (inst, Program);

   if ((i < 0) || (i >= (int)ctx->Const.MaxDrawBuffers)) {
      _mesa_set_program_error (ctx, Program->Position,
                               "Invalid draw buffer index");
      _mesa_error (ctx, GL_INVALID_OPERATION, "Invalid draw buffer index");
      return 1;
   }

   *color = (GLuint) i;
   return 0;
}


/**
 * \param coord The texture unit index
 * \return 0 on sucess, 1 on error
 */
static GLuint
parse_texcoord_num (GLcontext * ctx, GLubyte ** inst,
                    struct arb_program *Program, GLuint * coord)
{
   GLint i = parse_integer (inst, Program);

   if ((i < 0) || (i >= (int)ctx->Const.MaxTextureUnits)) {
      _mesa_set_program_error (ctx, Program->Position,
                               "Invalid texture unit index");
      _mesa_error (ctx, GL_INVALID_OPERATION, "Invalid texture unit index");
      return 1;
   }

   *coord = (GLuint) i;
   return 0;
}

/**
 * \param coord The weight index
 * \return 0 on sucess, 1 on error
 */
static GLuint
parse_weight_num (GLcontext * ctx, GLubyte ** inst, struct arb_program *Program,
                  GLint * coord)
{
   *coord = parse_integer (inst, Program);

   if ((*coord < 0) || (*coord >= 1)) {
      _mesa_set_program_error (ctx, Program->Position,
                               "Invalid weight index");
      _mesa_error (ctx, GL_INVALID_OPERATION, "Invalid weight index");
      return 1;
   }

   return 0;
}

/**
 * \param coord The clip plane index
 * \return 0 on sucess, 1 on error
 */
static GLuint
parse_clipplane_num (GLcontext * ctx, GLubyte ** inst,
                     struct arb_program *Program, GLint * coord)
{
   *coord = parse_integer (inst, Program);

   if ((*coord < 0) || (*coord >= (GLint) ctx->Const.MaxClipPlanes)) {
      _mesa_set_program_error (ctx, Program->Position,
                               "Invalid clip plane index");
      _mesa_error (ctx, GL_INVALID_OPERATION, "Invalid clip plane index");
      return 1;
   }

   return 0;
}


/**
 * \return 0 on front face, 1 on back face
 */
static GLuint
parse_face_type (GLubyte ** inst)
{
   switch (*(*inst)++) {
      case FACE_FRONT:
         return 0;

      case FACE_BACK:
         return 1;
   }
   return 0;
}


/**
 * Given a matrix and a modifier token on the binary array, return tokens
 * that _mesa_fetch_state() [program.c] can understand.
 *
 * \param matrix - the matrix we are talking about
 * \param matrix_idx - the index of the matrix we have (for texture & program matricies)
 * \param matrix_modifier - the matrix modifier (trans, inv, etc)
 * \return 0 on sucess, 1 on failure
 */
static GLuint
parse_matrix (GLcontext * ctx, GLubyte ** inst, struct arb_program *Program,
              GLint * matrix, GLint * matrix_idx, GLint * matrix_modifier)
{
   GLubyte mat = *(*inst)++;

   *matrix_idx = 0;

   switch (mat) {
      case MATRIX_MODELVIEW:
         *matrix = STATE_MODELVIEW;
         *matrix_idx = parse_integer (inst, Program);
         if (*matrix_idx > 0) {
            _mesa_set_program_error (ctx, Program->Position,
               "ARB_vertex_blend not supported\n");
            _mesa_error (ctx, GL_INVALID_OPERATION,
               "ARB_vertex_blend not supported\n");
            return 1;
         }
         break;

      case MATRIX_PROJECTION:
         *matrix = STATE_PROJECTION;
         break;

      case MATRIX_MVP:
         *matrix = STATE_MVP;
         break;

      case MATRIX_TEXTURE:
         *matrix = STATE_TEXTURE;
         *matrix_idx = parse_integer (inst, Program);
         if (*matrix_idx >= (GLint) ctx->Const.MaxTextureUnits) {
            _mesa_set_program_error (ctx, Program->Position,
                                     "Invalid Texture Unit");
            _mesa_error (ctx, GL_INVALID_OPERATION,
                         "Invalid Texture Unit: %d", *matrix_idx);
            return 1;
         }
         break;

         /* This is not currently supported (ARB_matrix_palette) */
      case MATRIX_PALETTE:
         *matrix_idx = parse_integer (inst, Program);
         _mesa_set_program_error (ctx, Program->Position,
              "ARB_matrix_palette not supported\n");
         _mesa_error (ctx, GL_INVALID_OPERATION,
              "ARB_matrix_palette not supported\n");
         return 1;
         break;

      case MATRIX_PROGRAM:
         *matrix = STATE_PROGRAM;
         *matrix_idx = parse_integer (inst, Program);
         if (*matrix_idx >= (GLint) ctx->Const.MaxProgramMatrices) {
            _mesa_set_program_error (ctx, Program->Position,
                                     "Invalid Program Matrix");
            _mesa_error (ctx, GL_INVALID_OPERATION,
                         "Invalid Program Matrix: %d", *matrix_idx);
            return 1;
         }
         break;
   }

   switch (*(*inst)++) {
      case MATRIX_MODIFIER_IDENTITY:
         *matrix_modifier = 0;
         break;
      case MATRIX_MODIFIER_INVERSE:
         *matrix_modifier = STATE_MATRIX_INVERSE;
         break;
      case MATRIX_MODIFIER_TRANSPOSE:
         *matrix_modifier = STATE_MATRIX_TRANSPOSE;
         break;
      case MATRIX_MODIFIER_INVTRANS:
         *matrix_modifier = STATE_MATRIX_INVTRANS;
         break;
   }

   return 0;
}


/**
 * This parses a state string (rather, the binary version of it) into
 * a 6-token sequence as described in _mesa_fetch_state() [program.c]
 *
 * \param inst         - the start in the binary arry to start working from
 * \param state_tokens - the storage for the 6-token state description
 * \return             - 0 on sucess, 1 on error
 */
static GLuint
parse_state_single_item (GLcontext * ctx, GLubyte ** inst,
                         struct arb_program *Program, GLint * state_tokens)
{
   switch (*(*inst)++) {
      case STATE_MATERIAL_PARSER:
         state_tokens[0] = STATE_MATERIAL;
         state_tokens[1] = parse_face_type (inst);
         switch (*(*inst)++) {
            case MATERIAL_AMBIENT:
               state_tokens[2] = STATE_AMBIENT;
               break;
            case MATERIAL_DIFFUSE:
               state_tokens[2] = STATE_DIFFUSE;
               break;
            case MATERIAL_SPECULAR:
               state_tokens[2] = STATE_SPECULAR;
               break;
            case MATERIAL_EMISSION:
               state_tokens[2] = STATE_EMISSION;
	       break;
            case MATERIAL_SHININESS:
               state_tokens[2] = STATE_SHININESS;
               break;
         }
         break;

      case STATE_LIGHT_PARSER:
         state_tokens[0] = STATE_LIGHT;
         state_tokens[1] = parse_integer (inst, Program);

         /* Check the value of state_tokens[1] against the # of lights */
         if (state_tokens[1] >= (GLint) ctx->Const.MaxLights) {
            _mesa_set_program_error (ctx, Program->Position,
                                     "Invalid Light Number");
            _mesa_error (ctx, GL_INVALID_OPERATION,
                         "Invalid Light Number: %d", state_tokens[1]);
            return 1;
         }

         switch (*(*inst)++) {
            case LIGHT_AMBIENT:
               state_tokens[2] = STATE_AMBIENT;
               break;
            case LIGHT_DIFFUSE:
               state_tokens[2] = STATE_DIFFUSE;
               break;
            case LIGHT_SPECULAR:
               state_tokens[2] = STATE_SPECULAR;
               break;
            case LIGHT_POSITION:
               state_tokens[2] = STATE_POSITION;
               break;
            case LIGHT_ATTENUATION:
               state_tokens[2] = STATE_ATTENUATION;
               break;
            case LIGHT_HALF:
               state_tokens[2] = STATE_HALF;
               break;
            case LIGHT_SPOT_DIRECTION:
               state_tokens[2] = STATE_SPOT_DIRECTION;
               break;
         }
         break;

      case STATE_LIGHT_MODEL:
         switch (*(*inst)++) {
            case LIGHT_MODEL_AMBIENT:
               state_tokens[0] = STATE_LIGHTMODEL_AMBIENT;
               break;
            case LIGHT_MODEL_SCENECOLOR:
               state_tokens[0] = STATE_LIGHTMODEL_SCENECOLOR;
               state_tokens[1] = parse_face_type (inst);
               break;
         }
         break;

      case STATE_LIGHT_PROD:
         state_tokens[0] = STATE_LIGHTPROD;
         state_tokens[1] = parse_integer (inst, Program);

         /* Check the value of state_tokens[1] against the # of lights */
         if (state_tokens[1] >= (GLint) ctx->Const.MaxLights) {
            _mesa_set_program_error (ctx, Program->Position,
                                     "Invalid Light Number");
            _mesa_error (ctx, GL_INVALID_OPERATION,
                         "Invalid Light Number: %d", state_tokens[1]);
            return 1;
         }

         state_tokens[2] = parse_face_type (inst);
         switch (*(*inst)++) {
            case LIGHT_PROD_AMBIENT:
               state_tokens[3] = STATE_AMBIENT;
               break;
            case LIGHT_PROD_DIFFUSE:
               state_tokens[3] = STATE_DIFFUSE;
               break;
            case LIGHT_PROD_SPECULAR:
               state_tokens[3] = STATE_SPECULAR;
               break;
         }
         break;


      case STATE_FOG:
         switch (*(*inst)++) {
            case FOG_COLOR:
               state_tokens[0] = STATE_FOG_COLOR;
               break;
            case FOG_PARAMS:
               state_tokens[0] = STATE_FOG_PARAMS;
               break;
         }
         break;

      case STATE_TEX_ENV:
         state_tokens[1] = parse_integer (inst, Program);
         switch (*(*inst)++) {
            case TEX_ENV_COLOR:
               state_tokens[0] = STATE_TEXENV_COLOR;
               break;
         }
         break;

      case STATE_TEX_GEN:
         {
            GLuint type, coord;

            state_tokens[0] = STATE_TEXGEN;
            /*state_tokens[1] = parse_integer (inst, Program);*/    /* Texture Unit */

            if (parse_texcoord_num (ctx, inst, Program, &coord))
               return 1;
	    state_tokens[1] = coord;

            /* EYE or OBJECT */
            type = *(*inst++);

            /* 0 - s, 1 - t, 2 - r, 3 - q */
            coord = *(*inst++);

            if (type == TEX_GEN_EYE) {
               switch (coord) {
                  case COMPONENT_X:
                     state_tokens[2] = STATE_TEXGEN_EYE_S;
                     break;
                  case COMPONENT_Y:
                     state_tokens[2] = STATE_TEXGEN_EYE_T;
                     break;
                  case COMPONENT_Z:
                     state_tokens[2] = STATE_TEXGEN_EYE_R;
                     break;
                  case COMPONENT_W:
                     state_tokens[2] = STATE_TEXGEN_EYE_Q;
                     break;
               }
            }
            else {
               switch (coord) {
                  case COMPONENT_X:
                     state_tokens[2] = STATE_TEXGEN_OBJECT_S;
                     break;
                  case COMPONENT_Y:
                     state_tokens[2] = STATE_TEXGEN_OBJECT_T;
                     break;
                  case COMPONENT_Z:
                     state_tokens[2] = STATE_TEXGEN_OBJECT_R;
                     break;
                  case COMPONENT_W:
                     state_tokens[2] = STATE_TEXGEN_OBJECT_Q;
                     break;
               }
            }
         }
         break;

      case STATE_DEPTH:
         switch (*(*inst)++) {
            case DEPTH_RANGE:
               state_tokens[0] = STATE_DEPTH_RANGE;
               break;
         }
         break;

      case STATE_CLIP_PLANE:
         state_tokens[0] = STATE_CLIPPLANE;
         state_tokens[1] = parse_integer (inst, Program);
         if (parse_clipplane_num (ctx, inst, Program, &state_tokens[1]))
            return 1;
         break;

      case STATE_POINT:
         switch (*(*inst++)) {
            case POINT_SIZE:
               state_tokens[0] = STATE_POINT_SIZE;
               break;

            case POINT_ATTENUATION:
               state_tokens[0] = STATE_POINT_ATTENUATION;
               break;
         }
         break;

         /* XXX: I think this is the correct format for a matrix row */
      case STATE_MATRIX_ROWS:
         state_tokens[0] = STATE_MATRIX;
         if (parse_matrix
             (ctx, inst, Program, &state_tokens[1], &state_tokens[2],
              &state_tokens[5]))
            return 1;

         state_tokens[3] = parse_integer (inst, Program);       /* The first row to grab */

         if ((**inst) != 0) {                                   /* Either the last row, 0 */
            state_tokens[4] = parse_integer (inst, Program);
            if (state_tokens[4] < state_tokens[3]) {
               _mesa_set_program_error (ctx, Program->Position,
                     "Second matrix index less than the first");
               _mesa_error (ctx, GL_INVALID_OPERATION,
                     "Second matrix index (%d) less than the first (%d)",
                     state_tokens[4], state_tokens[3]);
               return 1;
            }
         }
         else {
            state_tokens[4] = state_tokens[3];
            (*inst)++;
         }
         break;
   }

   return 0;
}

/**
 * This parses a state string (rather, the binary version of it) into
 * a 6-token similar for the state fetching code in program.c
 *
 * One might ask, why fetch these parameters into just like  you fetch
 * state when they are already stored in other places?
 *
 * Because of array offsets -> We can stick env/local parameters in the
 * middle of a parameter array and then index someplace into the array
 * when we execute.
 *
 * One optimization might be to only do this for the cases where the
 * env/local parameters end up inside of an array, and leave the
 * single parameters (or arrays of pure env/local pareameters) in their
 * respective register files.
 *
 * For ENV parameters, the format is:
 *    state_tokens[0] = STATE_FRAGMENT_PROGRAM / STATE_VERTEX_PROGRAM
 *    state_tokens[1] = STATE_ENV
 *    state_tokens[2] = the parameter index
 *
 * for LOCAL parameters, the format is:
 *    state_tokens[0] = STATE_FRAGMENT_PROGRAM / STATE_VERTEX_PROGRAM
 *    state_tokens[1] = STATE_LOCAL
 *    state_tokens[2] = the parameter index
 *
 * \param inst         - the start in the binary arry to start working from
 * \param state_tokens - the storage for the 6-token state description
 * \return             - 0 on sucess, 1 on failure
 */
static GLuint
parse_program_single_item (GLcontext * ctx, GLubyte ** inst,
                           struct arb_program *Program, GLint * state_tokens)
{
   if (Program->Base.Target == GL_FRAGMENT_PROGRAM_ARB)
      state_tokens[0] = STATE_FRAGMENT_PROGRAM;
   else
      state_tokens[0] = STATE_VERTEX_PROGRAM;


   switch (*(*inst)++) {
      case PROGRAM_PARAM_ENV:
         state_tokens[1] = STATE_ENV;
         state_tokens[2] = parse_integer (inst, Program);

         /* Check state_tokens[2] against the number of ENV parameters available */
         if (((Program->Base.Target == GL_FRAGMENT_PROGRAM_ARB) &&
              (state_tokens[2] >= (GLint) ctx->Const.MaxFragmentProgramEnvParams))
             ||
             ((Program->Base.Target == GL_VERTEX_PROGRAM_ARB) &&
              (state_tokens[2] >= (GLint) ctx->Const.MaxVertexProgramEnvParams))) {
            _mesa_set_program_error (ctx, Program->Position,
                                     "Invalid Program Env Parameter");
            _mesa_error (ctx, GL_INVALID_OPERATION,
                         "Invalid Program Env Parameter: %d",
                         state_tokens[2]);
            return 1;
         }

         break;

      case PROGRAM_PARAM_LOCAL:
         state_tokens[1] = STATE_LOCAL;
         state_tokens[2] = parse_integer (inst, Program);

         /* Check state_tokens[2] against the number of LOCAL parameters available */
         if (((Program->Base.Target == GL_FRAGMENT_PROGRAM_ARB) &&
              (state_tokens[2] >= (GLint) ctx->Const.MaxFragmentProgramLocalParams))
             ||
             ((Program->Base.Target == GL_VERTEX_PROGRAM_ARB) &&
              (state_tokens[2] >= (GLint) ctx->Const.MaxVertexProgramLocalParams))) {
            _mesa_set_program_error (ctx, Program->Position,
                                     "Invalid Program Local Parameter");
            _mesa_error (ctx, GL_INVALID_OPERATION,
                         "Invalid Program Local Parameter: %d",
                         state_tokens[2]);
            return 1;
         }
         break;
   }

   return 0;
}

/**
 * For ARB_vertex_program, programs are not allowed to use both an explicit
 * vertex attribute and a generic vertex attribute corresponding to the same
 * state. See section 2.14.3.1 of the GL_ARB_vertex_program spec.
 *
 * This will walk our var_cache and make sure that nobody does anything fishy.
 *
 * \return 0 on sucess, 1 on error
 */
static GLuint
generic_attrib_check(struct var_cache *vc_head)
{
   int a;
   struct var_cache *curr;
   GLboolean explicitAttrib[MAX_VERTEX_PROGRAM_ATTRIBS],
      genericAttrib[MAX_VERTEX_PROGRAM_ATTRIBS];

   for (a=0; a<MAX_VERTEX_PROGRAM_ATTRIBS; a++) {
      explicitAttrib[a] = GL_FALSE;
      genericAttrib[a] = GL_FALSE;
   }

   curr = vc_head;
   while (curr) {
      if (curr->type == vt_attrib) {
         if (curr->attrib_is_generic)
            genericAttrib[ curr->attrib_binding_idx ] = GL_TRUE;
         else
            explicitAttrib[ curr->attrib_binding_idx ] = GL_TRUE;
      }

      curr = curr->next;
   }

   for (a=0; a<MAX_VERTEX_PROGRAM_ATTRIBS; a++) {
      if ((explicitAttrib[a]) && (genericAttrib[a]))
         return 1;
   }

   return 0;
}

/**
 * This will handle the binding side of an ATTRIB var declaration
 *
 * \param binding     - the fragment input register state, defined in nvfragprog.h
 * \param binding_idx - the index in the attrib register file that binding is associated with
 * \return returns 0 on sucess, 1 on error
 *
 * See nvfragparse.c for attrib register file layout
 */
static GLuint
parse_attrib_binding (GLcontext * ctx, GLubyte ** inst,
                      struct arb_program *Program, GLuint * binding,
                      GLuint * binding_idx, GLuint *is_generic)
{
   GLuint texcoord;
   GLint coord;
   GLint err = 0;

   *is_generic = 0;
   if (Program->Base.Target == GL_FRAGMENT_PROGRAM_ARB) {
      switch (*(*inst)++) {
         case FRAGMENT_ATTRIB_COLOR:
            err = parse_color_type (ctx, inst, Program, &coord);
            *binding = FRAG_ATTRIB_COL0 + coord;
            *binding_idx = 1 + coord;
            break;

         case FRAGMENT_ATTRIB_TEXCOORD:
            err = parse_texcoord_num (ctx, inst, Program, &texcoord);
            *binding = FRAG_ATTRIB_TEX0 + texcoord;
            *binding_idx = 4 + texcoord;
            break;

         case FRAGMENT_ATTRIB_FOGCOORD:
            *binding = FRAG_ATTRIB_FOGC;
            *binding_idx = 3;
            break;

         case FRAGMENT_ATTRIB_POSITION:
            *binding = FRAG_ATTRIB_WPOS;
            *binding_idx = 0;
            break;

         default:
            err = 1;
            break;
      }
   }
   else {
      switch (*(*inst)++) {
         case VERTEX_ATTRIB_POSITION:
            *binding = VERT_ATTRIB_POS;
            *binding_idx = 0;
            break;

         case VERTEX_ATTRIB_WEIGHT:
            {
               GLint weight;

               err = parse_weight_num (ctx, inst, Program, &weight);
               *binding = VERT_ATTRIB_WEIGHT;
               *binding_idx = 1;
            }
            _mesa_set_program_error (ctx, Program->Position,
                 "ARB_vertex_blend not supported\n");
            _mesa_error (ctx, GL_INVALID_OPERATION,
                 "ARB_vertex_blend not supported\n");
            return 1;
            break;

         case VERTEX_ATTRIB_NORMAL:
            *binding = VERT_ATTRIB_NORMAL;
            *binding_idx = 2;
            break;

         case VERTEX_ATTRIB_COLOR:
            {
               GLint color;

               err = parse_color_type (ctx, inst, Program, &color);
               if (color) {
                  *binding = VERT_ATTRIB_COLOR1;
                  *binding_idx = 4;
               }
               else {
                  *binding = VERT_ATTRIB_COLOR0;
                  *binding_idx = 3;
               }
            }
            break;

         case VERTEX_ATTRIB_FOGCOORD:
            *binding = VERT_ATTRIB_FOG;
            *binding_idx = 5;
            break;

         case VERTEX_ATTRIB_TEXCOORD:
            {
               GLuint unit;

               err = parse_texcoord_num (ctx, inst, Program, &unit);
               *binding = VERT_ATTRIB_TEX0 + unit;
               *binding_idx = 8 + unit;
            }
            break;

            /* It looks like we don't support this at all, atm */
         case VERTEX_ATTRIB_MATRIXINDEX:
            parse_integer (inst, Program);
            _mesa_set_program_error (ctx, Program->Position,
                  "ARB_palette_matrix not supported");
            _mesa_error (ctx, GL_INVALID_OPERATION,
                  "ARB_palette_matrix not supported");
            return 1;
            break;

         case VERTEX_ATTRIB_GENERIC:
            {
               GLuint attrib;

               if (!parse_generic_attrib_num(ctx, inst, Program, &attrib)) {
                  *is_generic = 1;
                  switch (attrib) {
                     case 0:
                        *binding = VERT_ATTRIB_POS;
                        break;
                     case 1:
                        *binding = VERT_ATTRIB_WEIGHT;
                        break;
                     case 2:
                        *binding = VERT_ATTRIB_NORMAL;
                        break;
                     case 3:
                        *binding = VERT_ATTRIB_COLOR0;
                        break;
                     case 4:
                        *binding = VERT_ATTRIB_COLOR1;
                        break;
                     case 5:
                        *binding = VERT_ATTRIB_FOG;
                        break;
                     case 6:
                        break;
                     case 7:
                        break;
                     default:
                        *binding = VERT_ATTRIB_TEX0 + (attrib-8);
                        break;
                  }
                  *binding_idx = attrib;
               }
            }
            break;

         default:
            err = 1;
            break;
      }
   }

   /* Can this even happen? */
   if (err) {
      _mesa_set_program_error (ctx, Program->Position,
                               "Bad attribute binding");
      _mesa_error (ctx, GL_INVALID_OPERATION, "Bad attribute binding");
   }

   Program->InputsRead |= (1 << *binding_idx);

   return err;
}

/**
 * This translates between a binary token for an output variable type
 * and the mesa token for the same thing.
 *
 *
 * XXX: What is the 'name' for vertex program state? -> do we need it?
 *         I don't think we do;
 *
 * See nvfragprog.h for definitions
 *
 * \param inst        - The parsed tokens
 * \param binding     - The name of the state we are binding too
 * \param binding_idx - The index into the result register file that this is bound too
 *
 * See nvfragparse.c for the register file layout for fragment programs
 * See nvvertparse.c for the register file layout for vertex programs
 */
static GLuint
parse_result_binding (GLcontext * ctx, GLubyte ** inst, GLuint * binding,
                      GLuint * binding_idx, struct arb_program *Program)
{
   GLuint b, out_color;

   switch (*(*inst)++) {
      case FRAGMENT_RESULT_COLOR:
         /* for frag programs, this is FRAGMENT_RESULT_COLOR */
         if (Program->Base.Target == GL_FRAGMENT_PROGRAM_ARB) {
            /* This gets result of the color buffer we're supposed to 
             * draw into
             */
            parse_output_color_num(ctx, inst, Program, &out_color);

            *binding = FRAG_OUTPUT_COLR;

				/* XXX: We're ignoring the color buffer for now. */
            *binding_idx = 0;
         }
         /* for vtx programs, this is VERTEX_RESULT_POSITION */
         else {
            *binding_idx = 0;
         }
         break;

      case FRAGMENT_RESULT_DEPTH:
         /* for frag programs, this is FRAGMENT_RESULT_DEPTH */
         if (Program->Base.Target == GL_FRAGMENT_PROGRAM_ARB) {
            *binding = FRAG_OUTPUT_DEPR;
            *binding_idx = 2;
         }
         /* for vtx programs, this is VERTEX_RESULT_COLOR */
         else {
            GLint color_type;
            GLuint face_type = parse_face_type(inst);
	    GLint color_type_ret = parse_color_type(ctx, inst, Program, &color_type);

            /* back face */
            if (face_type) {
               if (color_type_ret) return 1;

               /* secondary color */
               if (color_type) {
                  *binding_idx = 4;
               }
               /*  primary color */
               else {
                  *binding_idx = 3;
               }
            }
            /* front face */
            else {
               /* secondary color */
               if (color_type) {
                  *binding_idx = 2;
               }
               /* primary color */
               else {
                  *binding_idx = 1;
               }
            }
         }
         break;

      case VERTEX_RESULT_FOGCOORD:
         *binding_idx = 5;
         break;

      case VERTEX_RESULT_POINTSIZE:
         *binding_idx = 6;
         break;

      case VERTEX_RESULT_TEXCOORD:
         if (parse_texcoord_num (ctx, inst, Program, &b))
            return 1;
         *binding_idx = 7 + b;
         break;
   }

   Program->OutputsWritten |= (1 << *binding_idx);

   return 0;
}

/**
 * This handles the declaration of ATTRIB variables
 *
 * XXX: Still needs
 *      parse_vert_attrib_binding(), or something like that
 *
 * \return 0 on sucess, 1 on error
 */
static GLint
parse_attrib (GLcontext * ctx, GLubyte ** inst, struct var_cache **vc_head,
              struct arb_program *Program)
{
   GLuint found;
   char *error_msg;
   struct var_cache *attrib_var;

   attrib_var = parse_string (inst, vc_head, Program, &found);
   Program->Position = parse_position (inst);
   if (found) {
      error_msg = (char *)
         _mesa_malloc (_mesa_strlen ((char *) attrib_var->name) + 40);
      _mesa_sprintf (error_msg, "Duplicate Varible Declaration: %s",
                     attrib_var->name);

      _mesa_set_program_error (ctx, Program->Position, error_msg);
      _mesa_error (ctx, GL_INVALID_OPERATION, error_msg);

      _mesa_free (error_msg);
      return 1;
   }

   attrib_var->type = vt_attrib;

   /* I think this is ok now - karl */
   /* XXX: */
   /*if (Program->type == GL_FRAGMENT_PROGRAM_ARB) */
   {
      if (parse_attrib_binding
          (ctx, inst, Program, &attrib_var->attrib_binding,
           &attrib_var->attrib_binding_idx, &attrib_var->attrib_is_generic))
         return 1;
      if (generic_attrib_check(*vc_head)) {
         _mesa_set_program_error (ctx, Program->Position,
   "Cannot use both a generic vertex attribute and a specific attribute of the same type");
         _mesa_error (ctx, GL_INVALID_OPERATION,
   "Cannot use both a generic vertex attribute and a specific attribute of the same type");
         return 1;
      }

   }

   Program->Base.NumAttributes++;
   return 0;
}

/**
 * \param use -- TRUE if we're called when declaring implicit parameters,
 *               FALSE if we're declaraing variables. This has to do with
 *               if we get a signed or unsigned float for scalar constants
 */
static GLuint
parse_param_elements (GLcontext * ctx, GLubyte ** inst,
                      struct var_cache *param_var,
                      struct arb_program *Program, GLboolean use)
{
   GLint idx;
   GLuint err;
   GLint state_tokens[6];
   GLfloat const_values[4];

   err = 0;

   switch (*(*inst)++) {
      case PARAM_STATE_ELEMENT:

         if (parse_state_single_item (ctx, inst, Program, state_tokens))
            return 1;

         /* If we adding STATE_MATRIX that has multiple rows, we need to
          * unroll it and call _mesa_add_state_reference() for each row
          */
         if ((state_tokens[0] == STATE_MATRIX)
             && (state_tokens[3] != state_tokens[4])) {
            GLint row;
            GLint first_row = state_tokens[3];
            GLint last_row = state_tokens[4];

            for (row = first_row; row <= last_row; row++) {
               state_tokens[3] = state_tokens[4] = row;

               idx =
                  _mesa_add_state_reference (Program->Parameters,
                                             state_tokens);
               if (param_var->param_binding_begin == ~0U)
                  param_var->param_binding_begin = idx;
               param_var->param_binding_length++;
               Program->Base.NumParameters++;
            }
         }
         else {
            idx =
               _mesa_add_state_reference (Program->Parameters, state_tokens);
            if (param_var->param_binding_begin == ~0U)
               param_var->param_binding_begin = idx;
            param_var->param_binding_length++;
            Program->Base.NumParameters++;
         }
         break;

      case PARAM_PROGRAM_ELEMENT:

         if (parse_program_single_item (ctx, inst, Program, state_tokens))
            return 1;
         idx = _mesa_add_state_reference (Program->Parameters, state_tokens);
         if (param_var->param_binding_begin == ~0U)
            param_var->param_binding_begin = idx;
         param_var->param_binding_length++;
         Program->Base.NumParameters++;

         /* Check if there is more: 0 -> we're done, else its an integer */
         if (**inst) {
            GLuint out_of_range, new_idx;
            GLuint start_idx = state_tokens[2] + 1;
            GLuint end_idx = parse_integer (inst, Program);

            out_of_range = 0;
            if (Program->Base.Target == GL_FRAGMENT_PROGRAM_ARB) {
               if (((state_tokens[1] == STATE_ENV)
                    && (end_idx >= ctx->Const.MaxFragmentProgramEnvParams))
                   || ((state_tokens[1] == STATE_LOCAL)
                       && (end_idx >=
                           ctx->Const.MaxFragmentProgramLocalParams)))
                  out_of_range = 1;
            }
            else {
               if (((state_tokens[1] == STATE_ENV)
                    && (end_idx >= ctx->Const.MaxVertexProgramEnvParams))
                   || ((state_tokens[1] == STATE_LOCAL)
                       && (end_idx >=
                           ctx->Const.MaxVertexProgramLocalParams)))
                  out_of_range = 1;
            }
            if (out_of_range) {
               _mesa_set_program_error (ctx, Program->Position,
                                        "Invalid Program Parameter");
               _mesa_error (ctx, GL_INVALID_OPERATION,
                            "Invalid Program Parameter: %d", end_idx);
               return 1;
            }

            for (new_idx = start_idx; new_idx <= end_idx; new_idx++) {
               state_tokens[2] = new_idx;
               idx =
                  _mesa_add_state_reference (Program->Parameters,
                                             state_tokens);
               param_var->param_binding_length++;
               Program->Base.NumParameters++;
            }
         }
			else
			{
				(*inst)++;
			}
         break;

      case PARAM_CONSTANT:
         parse_constant (inst, const_values, Program, use);
         idx =
            _mesa_add_named_constant (Program->Parameters,
                                      (char *) param_var->name, const_values);
         if (param_var->param_binding_begin == ~0U)
            param_var->param_binding_begin = idx;
         param_var->param_binding_length++;
         Program->Base.NumParameters++;
         break;

      default:
         _mesa_set_program_error (ctx, Program->Position,
                                  "Unexpected token in parse_param_elements()");
         _mesa_error (ctx, GL_INVALID_OPERATION,
                      "Unexpected token in parse_param_elements()");
         return 1;
   }

   /* Make sure we haven't blown past our parameter limits */
   if (((Program->Base.Target == GL_VERTEX_PROGRAM_ARB) &&
        (Program->Base.NumParameters >=
         ctx->Const.MaxVertexProgramLocalParams))
       || ((Program->Base.Target == GL_FRAGMENT_PROGRAM_ARB)
           && (Program->Base.NumParameters >=
               ctx->Const.MaxFragmentProgramLocalParams))) {
      _mesa_set_program_error (ctx, Program->Position,
                               "Too many parameter variables");
      _mesa_error (ctx, GL_INVALID_OPERATION, "Too many parameter variables");
      return 1;
   }

   return err;
}

/**
 * This picks out PARAM program parameter bindings.
 *
 * XXX: This needs to be stressed & tested
 *
 * \return 0 on sucess, 1 on error
 */
static GLuint
parse_param (GLcontext * ctx, GLubyte ** inst, struct var_cache **vc_head,
             struct arb_program *Program)
{
   GLuint found, err;
   GLint specified_length;
   char *error_msg;
   struct var_cache *param_var;

   err = 0;
   param_var = parse_string (inst, vc_head, Program, &found);
   Program->Position = parse_position (inst);

   if (found) {
      error_msg = (char *) _mesa_malloc (_mesa_strlen ((char *) param_var->name) + 40);
      _mesa_sprintf (error_msg, "Duplicate Varible Declaration: %s",
                     param_var->name);

      _mesa_set_program_error (ctx, Program->Position, error_msg);
      _mesa_error (ctx, GL_INVALID_OPERATION, error_msg);

      _mesa_free (error_msg);
      return 1;
   }

   specified_length = parse_integer (inst, Program);

   if (specified_length < 0) {
      _mesa_set_program_error (ctx, Program->Position,
                               "Negative parameter array length");
      _mesa_error (ctx, GL_INVALID_OPERATION,
                   "Negative parameter array length: %d", specified_length);
      return 1;
   }

   param_var->type = vt_param;
   param_var->param_binding_length = 0;

   /* Right now, everything is shoved into the main state register file.
    *
    * In the future, it would be nice to leave things ENV/LOCAL params
    * in their respective register files, if possible
    */
   param_var->param_binding_type = PROGRAM_STATE_VAR;

   /* Remember to:
    * *   - add each guy to the parameter list
    * *   - increment the param_var->param_binding_len
    * *   - store the param_var->param_binding_begin for the first one
    * *   - compare the actual len to the specified len at the end
    */
   while (**inst != PARAM_NULL) {
      if (parse_param_elements (ctx, inst, param_var, Program, GL_FALSE))
         return 1;
   }

   /* Test array length here! */
   if (specified_length) {
      if (specified_length != (int)param_var->param_binding_length) {
         _mesa_set_program_error (ctx, Program->Position,
                                  "Declared parameter array lenght does not match parameter list");
         _mesa_error (ctx, GL_INVALID_OPERATION,
                      "Declared parameter array lenght does not match parameter list");
      }
   }

   (*inst)++;

   return 0;
}

/**
 *
 */
static GLuint
parse_param_use (GLcontext * ctx, GLubyte ** inst, struct var_cache **vc_head,
                 struct arb_program *Program, struct var_cache **new_var)
{
   struct var_cache *param_var;

   /* First, insert a dummy entry into the var_cache */
   var_cache_create (&param_var);
   param_var->name = (GLubyte *) _mesa_strdup (" ");
   param_var->type = vt_param;

   param_var->param_binding_length = 0;
   /* Don't fill in binding_begin; We use the default value of -1
    * to tell if its already initialized, elsewhere.
    *
    * param_var->param_binding_begin  = 0;
    */
   param_var->param_binding_type = PROGRAM_STATE_VAR;

   var_cache_append (vc_head, param_var);

   /* Then fill it with juicy parameter goodness */
   if (parse_param_elements (ctx, inst, param_var, Program, GL_TRUE))
      return 1;

   *new_var = param_var;

   return 0;
}


/**
 * This handles the declaration of TEMP variables
 *
 * \return 0 on sucess, 1 on error
 */
static GLuint
parse_temp (GLcontext * ctx, GLubyte ** inst, struct var_cache **vc_head,
            struct arb_program *Program)
{
   GLuint found;
   struct var_cache *temp_var;
   char *error_msg;

   while (**inst != 0) {
      temp_var = parse_string (inst, vc_head, Program, &found);
      Program->Position = parse_position (inst);
      if (found) {
         error_msg = (char *)
            _mesa_malloc (_mesa_strlen ((char *) temp_var->name) + 40);
         _mesa_sprintf (error_msg, "Duplicate Varible Declaration: %s",
                        temp_var->name);

         _mesa_set_program_error (ctx, Program->Position, error_msg);
         _mesa_error (ctx, GL_INVALID_OPERATION, error_msg);

         _mesa_free (error_msg);
         return 1;
      }

      temp_var->type = vt_temp;

      if (((Program->Base.Target == GL_FRAGMENT_PROGRAM_ARB) &&
           (Program->Base.NumTemporaries >=
            ctx->Const.MaxFragmentProgramTemps))
          || ((Program->Base.Target == GL_VERTEX_PROGRAM_ARB)
              && (Program->Base.NumTemporaries >=
                  ctx->Const.MaxVertexProgramTemps))) {
         _mesa_set_program_error (ctx, Program->Position,
                                  "Too many TEMP variables declared");
         _mesa_error (ctx, GL_INVALID_OPERATION,
                      "Too many TEMP variables declared");
         return 1;
      }

      temp_var->temp_binding = Program->Base.NumTemporaries;
      Program->Base.NumTemporaries++;
   }
   (*inst)++;

   return 0;
}

/**
 * This handles variables of the OUTPUT variety
 *
 * \return 0 on sucess, 1 on error
 */
static GLuint
parse_output (GLcontext * ctx, GLubyte ** inst, struct var_cache **vc_head,
              struct arb_program *Program)
{
   GLuint found;
   struct var_cache *output_var;

   output_var = parse_string (inst, vc_head, Program, &found);
   Program->Position = parse_position (inst);
   if (found) {
      char *error_msg;
      error_msg = (char *)
         _mesa_malloc (_mesa_strlen ((char *) output_var->name) + 40);
      _mesa_sprintf (error_msg, "Duplicate Varible Declaration: %s",
                     output_var->name);

      _mesa_set_program_error (ctx, Program->Position, error_msg);
      _mesa_error (ctx, GL_INVALID_OPERATION, error_msg);

      _mesa_free (error_msg);
      return 1;
   }

   output_var->type = vt_output;
   return parse_result_binding (ctx, inst, &output_var->output_binding,
                                &output_var->output_binding_idx, Program);
}

/**
 * This handles variables of the ALIAS kind
 *
 * \return 0 on sucess, 1 on error
 */
static GLuint
parse_alias (GLcontext * ctx, GLubyte ** inst, struct var_cache **vc_head,
             struct arb_program *Program)
{
   GLuint found;
   struct var_cache *temp_var;
   char *error_msg;


   temp_var = parse_string (inst, vc_head, Program, &found);
   Program->Position = parse_position (inst);

   if (found) {
      error_msg = (char *)
         _mesa_malloc (_mesa_strlen ((char *) temp_var->name) + 40);
      _mesa_sprintf (error_msg, "Duplicate Varible Declaration: %s",
                     temp_var->name);

      _mesa_set_program_error (ctx, Program->Position, error_msg);
      _mesa_error (ctx, GL_INVALID_OPERATION, error_msg);

      _mesa_free (error_msg);
      return 1;
   }

   temp_var->type = vt_alias;
   temp_var->alias_binding =  parse_string (inst, vc_head, Program, &found);
   Program->Position = parse_position (inst);

   if (!found)
   {
      error_msg = (char *)
         _mesa_malloc (_mesa_strlen ((char *) temp_var->name) + 40);
      _mesa_sprintf (error_msg, "Alias value %s is not defined",
                     temp_var->alias_binding->name);

      _mesa_set_program_error (ctx, Program->Position, error_msg);
      _mesa_error (ctx, GL_INVALID_OPERATION, error_msg);

      _mesa_free (error_msg);
      return 1;
   }

   return 0;
}

/**
 * This handles variables of the ADDRESS kind
 *
 * \return 0 on sucess, 1 on error
 */
static GLuint
parse_address (GLcontext * ctx, GLubyte ** inst, struct var_cache **vc_head,
               struct arb_program *Program)
{
   GLuint found;
   struct var_cache *temp_var;
   char *error_msg;

   while (**inst != 0) {
      temp_var = parse_string (inst, vc_head, Program, &found);
      Program->Position = parse_position (inst);
      if (found) {
         error_msg = (char *)
            _mesa_malloc (_mesa_strlen ((char *) temp_var->name) + 40);
         _mesa_sprintf (error_msg, "Duplicate Varible Declaration: %s",
                        temp_var->name);

         _mesa_set_program_error (ctx, Program->Position, error_msg);
         _mesa_error (ctx, GL_INVALID_OPERATION, error_msg);

         _mesa_free (error_msg);
         return 1;
      }

      temp_var->type = vt_address;

      if (Program->Base.NumAddressRegs >=
          ctx->Const.MaxVertexProgramAddressRegs) {
         _mesa_set_program_error (ctx, Program->Position,
                                  "Too many ADDRESS variables declared");
         _mesa_error (ctx, GL_INVALID_OPERATION,
                      "Too many ADDRESS variables declared");
         return 1;
      }

      temp_var->address_binding = Program->Base.NumAddressRegs;
      Program->Base.NumAddressRegs++;
   }
   (*inst)++;

   return 0;
}

/**
 * Parse a program declaration
 *
 * \return 0 on sucess, 1 on error
 */
static GLint
parse_declaration (GLcontext * ctx, GLubyte ** inst, struct var_cache **vc_head,
                   struct arb_program *Program)
{
   GLint err = 0;

   switch (*(*inst)++) {
      case ADDRESS:
         err = parse_address (ctx, inst, vc_head, Program);
         break;

      case ALIAS:
         err = parse_alias (ctx, inst, vc_head, Program);
         break;

      case ATTRIB:
         err = parse_attrib (ctx, inst, vc_head, Program);
         break;

      case OUTPUT:
         err = parse_output (ctx, inst, vc_head, Program);
         break;

      case PARAM:
         err = parse_param (ctx, inst, vc_head, Program);
         break;

      case TEMP:
         err = parse_temp (ctx, inst, vc_head, Program);
         break;
   }

   return err;
}

/**
 * Handle the parsing out of a masked destination register
 *
 * If we are a vertex program, make sure we don't write to
 * result.position of we have specified that the program is
 * position invariant
 *
 * \param File      - The register file we write to
 * \param Index     - The register index we write to
 * \param WriteMask - The mask controlling which components we write (1->write)
 *
 * \return 0 on sucess, 1 on error
 */
static GLuint
parse_masked_dst_reg (GLcontext * ctx, GLubyte ** inst,
                      struct var_cache **vc_head, struct arb_program *Program,
                      GLint * File, GLint * Index, GLint *WriteMask)
{
   GLuint result, tmp;
   struct var_cache *dst;

   /* We either have a result register specified, or a
    * variable that may or may not be writable
    */
   switch (*(*inst)++) {
      case REGISTER_RESULT:
         if (parse_result_binding
             (ctx, inst, &result, (GLuint *) Index, Program))
            return 1;
         *File = PROGRAM_OUTPUT;
         break;

      case REGISTER_ESTABLISHED_NAME:
         dst = parse_string (inst, vc_head, Program, &result);
         Program->Position = parse_position (inst);

         /* If the name has never been added to our symbol table, we're hosed */
         if (!result) {
            _mesa_set_program_error (ctx, Program->Position,
                                     "0: Undefined variable");
            _mesa_error (ctx, GL_INVALID_OPERATION, "0: Undefined variable: %s",
                         dst->name);
            return 1;
         }

         switch (dst->type) {
            case vt_output:
               *File = PROGRAM_OUTPUT;
               *Index = dst->output_binding_idx;
               break;

            case vt_temp:
               *File = PROGRAM_TEMPORARY;
               *Index = dst->temp_binding;
               break;

               /* If the var type is not vt_output or vt_temp, no go */
            default:
               _mesa_set_program_error (ctx, Program->Position,
                                        "Destination register is read only");
               _mesa_error (ctx, GL_INVALID_OPERATION,
                            "Destination register is read only: %s",
                            dst->name);
               return 1;
         }
         break;

      default:
         _mesa_set_program_error (ctx, Program->Position,
                                  "Unexpected opcode in parse_masked_dst_reg()");
         _mesa_error (ctx, GL_INVALID_OPERATION,
                      "Unexpected opcode in parse_masked_dst_reg()");
         return 1;
   }


   /* Position invariance test */
   /* This test is done now in syntax portion - when position invariance OPTION
      is specified, "result.position" rule is disabled so there is no way
      to write the position
   */
   /*if ((Program->HintPositionInvariant) && (*File == PROGRAM_OUTPUT) &&
      (*Index == 0))   {
      _mesa_set_program_error (ctx, Program->Position,
                  "Vertex program specified position invariance and wrote vertex position");
      _mesa_error (ctx, GL_INVALID_OPERATION,
                  "Vertex program specified position invariance and wrote vertex position");
   }*/

   /* And then the mask.
    *  w,a -> bit 0
    *  z,b -> bit 1
    *  y,g -> bit 2
    *  x,r -> bit 3
    *
    * ==> Need to reverse the order of bits for this!
    */
   tmp =  (GLint) *(*inst)++;
   *WriteMask = (((tmp>>3) & 0x1) |
		 ((tmp>>1) & 0x2) |
		 ((tmp<<1) & 0x4) |
		 ((tmp<<3) & 0x8));

   return 0;
}


/**
 * Handle the parsing of a address register
 *
 * \param Index     - The register index we write to
 *
 * \return 0 on sucess, 1 on error
 */
static GLuint
parse_address_reg (GLcontext * ctx, GLubyte ** inst,
                          struct var_cache **vc_head,
                          struct arb_program *Program, GLint * Index)
{
   struct var_cache *dst;
   GLuint result;
   (void) Index;

   dst = parse_string (inst, vc_head, Program, &result);
   Program->Position = parse_position (inst);

   /* If the name has never been added to our symbol table, we're hosed */
   if (!result) {
      _mesa_set_program_error (ctx, Program->Position, "Undefined variable");
      _mesa_error (ctx, GL_INVALID_OPERATION, "Undefined variable: %s",
                   dst->name);
      return 1;
   }

   if (dst->type != vt_address) {
      _mesa_set_program_error (ctx, Program->Position,
                               "Variable is not of type ADDRESS");
      _mesa_error (ctx, GL_INVALID_OPERATION,
                   "Variable: %s is not of type ADDRESS", dst->name);
      return 1;
   }

   return 0;
}

#if 0 /* unused */
/**
 * Handle the parsing out of a masked address register
 *
 * \param Index     - The register index we write to
 * \param WriteMask - The mask controlling which components we write (1->write)
 *
 * \return 0 on sucess, 1 on error
 */
static GLuint
parse_masked_address_reg (GLcontext * ctx, GLubyte ** inst,
                          struct var_cache **vc_head,
                          struct arb_program *Program, GLint * Index,
                          GLboolean * WriteMask)
{
   if (parse_address_reg (ctx, inst, vc_head, Program, Index))
      return 1;

   /* This should be 0x8 */
   (*inst)++;

   /* Writemask of .x is implied */
   WriteMask[0] = 1;
   WriteMask[1] = WriteMask[2] = WriteMask[3] = 0;

   return 0;
}
#endif

/**
 * Parse out a swizzle mask.
 *
 * The values in the input stream are:
 *   COMPONENT_X -> x/r
 *   COMPONENT_Y -> y/g
 *   COMPONENT_Z-> z/b
 *   COMPONENT_W-> w/a
 *
 * The values in the output mask are:
 *   0 -> x/r
 *   1 -> y/g
 *   2 -> z/b
 *   3 -> w/a
 *
 * The len parameter allows us to grab 4 components for a vector
 * swizzle, or just 1 component for a scalar src register selection
 */
static GLuint
parse_swizzle_mask (GLubyte ** inst, GLubyte * mask, GLint len)
{
   GLint a;

   for (a = 0; a < 4; a++)
      mask[a] = a;

   for (a = 0; a < len; a++) {
      switch (*(*inst)++) {
         case COMPONENT_X:
            mask[a] = 0;
            break;

         case COMPONENT_Y:
            mask[a] = 1;
            break;

         case COMPONENT_Z:
            mask[a] = 2;
            break;

         case COMPONENT_W:
            mask[a] = 3;
            break;
      }
   }

   return 0;
}

/**
 */
static GLuint
parse_extended_swizzle_mask(GLubyte **inst, GLubyte *mask, GLubyte *negate)
{
   GLint a;
   GLubyte swz;

   *negate = 0x0;
   for (a = 0; a < 4; a++) {
      if (parse_sign (inst) == -1)
         *negate |= (1 << a);

      swz = *(*inst)++;

      switch (swz) {
         case COMPONENT_0:
            mask[a] = SWIZZLE_ZERO;
            break;
         case COMPONENT_1:
            mask[a] = SWIZZLE_ONE;
            break;
         case COMPONENT_X:
            mask[a] = SWIZZLE_X;
            break;
         case COMPONENT_Y:
            mask[a] = SWIZZLE_Y;
            break;
         case COMPONENT_Z:
            mask[a] = SWIZZLE_Z;
            break;
         case COMPONENT_W:
            mask[a] = SWIZZLE_W;
            break;

      }
#if 0
      if (swz == 0)
         mask[a] = SWIZZLE_ZERO;
      else if (swz == 1)
         mask[a] = SWIZZLE_ONE;
      else
         mask[a] = swz - 2;
#endif

   }

   return 0;
}


static GLuint
parse_src_reg (GLcontext * ctx, GLubyte ** inst, struct var_cache **vc_head,
               struct arb_program *Program, GLint * File, GLint * Index,
               GLboolean *IsRelOffset )
{
   struct var_cache *src;
   GLuint binding_state, binding_idx, is_generic, found;
   GLint offset;

   *IsRelOffset = 0;

   /* And the binding for the src */
   switch (*(*inst)++) {
      case REGISTER_ATTRIB:
         if (parse_attrib_binding
             (ctx, inst, Program, &binding_state, &binding_idx, &is_generic))
            return 1;
         *File = PROGRAM_INPUT;
         *Index = binding_idx;

         /* We need to insert a dummy variable into the var_cache so we can
          * catch generic vertex attrib aliasing errors
          */
         var_cache_create(&src);
         src->type = vt_attrib;
         src->name = (GLubyte *)_mesa_strdup("Dummy Attrib Variable");
         src->attrib_binding     = binding_state;
         src->attrib_binding_idx = binding_idx;
         src->attrib_is_generic  = is_generic;
         var_cache_append(vc_head, src);
         if (generic_attrib_check(*vc_head)) {
            _mesa_set_program_error (ctx, Program->Position,
   "Cannot use both a generic vertex attribute and a specific attribute of the same type");
            _mesa_error (ctx, GL_INVALID_OPERATION,
   "Cannot use both a generic vertex attribute and a specific attribute of the same type");
            return 1;
         }
         break;

      case REGISTER_PARAM:
         switch (**inst) {
            case PARAM_ARRAY_ELEMENT:
               (*inst)++;
               src = parse_string (inst, vc_head, Program, &found);
               Program->Position = parse_position (inst);

               if (!found) {
                  _mesa_set_program_error (ctx, Program->Position,
                                           "2: Undefined variable");
                  _mesa_error (ctx, GL_INVALID_OPERATION,
                               "2: Undefined variable: %s", src->name);
                  return 1;
               }

               *File = src->param_binding_type;

               switch (*(*inst)++) {
                  case ARRAY_INDEX_ABSOLUTE:
                     offset = parse_integer (inst, Program);

                     if ((offset < 0)
                         || (offset >= (int)src->param_binding_length)) {
                        _mesa_set_program_error (ctx, Program->Position,
                                                 "Index out of range");
                        _mesa_error (ctx, GL_INVALID_OPERATION,
                                     "Index %d out of range for %s", offset,
                                     src->name);
                        return 1;
                     }

                     *Index = src->param_binding_begin + offset;
                     break;

                  case ARRAY_INDEX_RELATIVE:
                     {
                        GLint addr_reg_idx, rel_off;

                        /* First, grab the address regiseter */
                        if (parse_address_reg (ctx, inst, vc_head, Program, &addr_reg_idx))
                           return 1;

                        /* And the .x */
                        ((*inst)++);
                        ((*inst)++);
                        ((*inst)++);
                        ((*inst)++);

                        /* Then the relative offset */
                        if (parse_relative_offset(ctx, inst, Program, &rel_off)) return 1;

                        /* And store it properly */
                        *Index = src->param_binding_begin + rel_off;
                        *IsRelOffset = 1;
                     }
                     break;
               }
               break;

            default:

               if (parse_param_use (ctx, inst, vc_head, Program, &src))
                  return 1;

               *File = src->param_binding_type;
               *Index = src->param_binding_begin;
               break;
         }
         break;

      case REGISTER_ESTABLISHED_NAME:

         src = parse_string (inst, vc_head, Program, &found);
         Program->Position = parse_position (inst);

         /* If the name has never been added to our symbol table, we're hosed */
         if (!found) {
            _mesa_set_program_error (ctx, Program->Position,
                                     "3: Undefined variable");
            _mesa_error (ctx, GL_INVALID_OPERATION, "3: Undefined variable: %s",
                         src->name);
            return 1;
         }

         switch (src->type) {
            case vt_attrib:
               *File = PROGRAM_INPUT;
               *Index = src->attrib_binding_idx;
               break;

               /* XXX: We have to handle offsets someplace in here!  -- or are those above? */
            case vt_param:
               *File = src->param_binding_type;
               *Index = src->param_binding_begin;
               break;

            case vt_temp:
               *File = PROGRAM_TEMPORARY;
               *Index = src->temp_binding;
               break;

               /* If the var type is vt_output no go */
            default:
               _mesa_set_program_error (ctx, Program->Position,
                                        "destination register is read only");
               _mesa_error (ctx, GL_INVALID_OPERATION,
                            "destination register is read only: %s",
                            src->name);
               return 1;
         }
         break;

      default:
         _mesa_set_program_error (ctx, Program->Position,
                                  "Unknown token in parse_src_reg");
         _mesa_error (ctx, GL_INVALID_OPERATION,
                      "Unknown token in parse_src_reg");
         return 1;
   }

   return 0;
}

/**
 */
static GLuint
parse_fp_vector_src_reg (GLcontext * ctx, GLubyte ** inst,
			 struct var_cache **vc_head, struct arb_program *Program,
			 struct fp_src_register *reg )
{

   GLint File;
   GLint Index;
   GLboolean Negate;
   GLubyte Swizzle[4];
   GLboolean IsRelOffset;

   /* Grab the sign */
   Negate = (parse_sign (inst) == -1) ? 0xf : 0x0;

   /* And the src reg */
   if (parse_src_reg (ctx, inst, vc_head, Program, &File, &Index, &IsRelOffset))
      return 1;

   /* finally, the swizzle */
   parse_swizzle_mask (inst, Swizzle, 4);

   reg->File = File;
   reg->Index = Index;
   reg->Abs = 0;		/* NV only */
   reg->NegateAbs = 0;		/* NV only */
   reg->NegateBase = Negate;
   reg->Swizzle = (Swizzle[0] << 0 |
		   Swizzle[1] << 3 |
		   Swizzle[2] << 6 |
		   Swizzle[3] << 9);

   return 0;
}


static GLuint 
parse_fp_dst_reg(GLcontext * ctx, GLubyte ** inst,
		 struct var_cache **vc_head, struct arb_program *Program,
		 struct fp_dst_register *reg )
{
   GLint file, idx, mask;
   
   if (parse_masked_dst_reg (ctx, inst, vc_head, Program, &file, &idx, &mask))
      return 1;

   reg->CondMask = 0;		/* NV only */
   reg->CondSwizzle = 0;	/* NV only */
   reg->File = file;
   reg->Index = idx;
   reg->WriteMask = mask;
   return 0;
}



static GLuint
parse_fp_scalar_src_reg (GLcontext * ctx, GLubyte ** inst,
			 struct var_cache **vc_head, struct arb_program *Program,
			 struct fp_src_register *reg )
{

   GLint File;
   GLint Index;
   GLboolean Negate;
   GLubyte Swizzle[4];
   GLboolean IsRelOffset;

   /* Grab the sign */
   Negate = (parse_sign (inst) == -1) ? 0x1 : 0x0;

   /* And the src reg */
   if (parse_src_reg (ctx, inst, vc_head, Program, &File, &Index, &IsRelOffset))
      return 1;

   /* finally, the swizzle */
   parse_swizzle_mask (inst, Swizzle, 1);

   reg->File = File;
   reg->Index = Index;
   reg->Abs = 0;		/* NV only */
   reg->NegateAbs = 0;		/* NV only */
   reg->NegateBase = Negate;
   reg->Swizzle = (Swizzle[0] << 0);

   return 0;
}


/**
 * This is a big mother that handles getting opcodes into the instruction
 * and handling the src & dst registers for fragment program instructions
 */
static GLuint
parse_fp_instruction (GLcontext * ctx, GLubyte ** inst,
                      struct var_cache **vc_head, struct arb_program *Program,
                      struct fp_instruction *fp)
{
   GLint a;
   GLuint texcoord;
   GLubyte instClass, type, code;
   GLboolean rel;

   /* No condition codes in ARB_fp */
   fp->UpdateCondRegister = 0;

   /* Record the position in the program string for debugging */
   fp->StringPos = Program->Position;

   fp->Data = NULL;

   fp->DstReg.File = 0xf;	/* mark as undef */
   fp->SrcReg[0].File = 0xf;	/* mark as undef */
   fp->SrcReg[1].File = 0xf;	/* mark as undef */
   fp->SrcReg[2].File = 0xf;	/* mark as undef */

   /* OP_ALU_INST or OP_TEX_INST */
   instClass = *(*inst)++;

   /* OP_ALU_{VECTOR, SCALAR, BINSC, BIN, TRI, SWZ},
    * OP_TEX_{SAMPLE, KIL}
    */
   type = *(*inst)++;

   /* The actual opcode name */
   code = *(*inst)++;

   /* Increment the correct count */
   switch (instClass) {
      case OP_ALU_INST:
         Program->NumAluInstructions++;
         break;
      case OP_TEX_INST:
         Program->NumTexInstructions++;
         break;
   }

   fp->Saturate = 0;
   fp->Precision = FLOAT32;

   fp->DstReg.CondMask = COND_TR;

   switch (type) {
      case OP_ALU_VECTOR:
         switch (code) {
            case OP_ABS_SAT:
               fp->Saturate = 1;
            case OP_ABS:
               fp->Opcode = FP_OPCODE_ABS;
               break;

            case OP_FLR_SAT:
               fp->Saturate = 1;
            case OP_FLR:
               fp->Opcode = FP_OPCODE_FLR;
               break;

            case OP_FRC_SAT:
               fp->Saturate = 1;
            case OP_FRC:
               fp->Opcode = FP_OPCODE_FRC;
               break;

            case OP_LIT_SAT:
               fp->Saturate = 1;
            case OP_LIT:
               fp->Opcode = FP_OPCODE_LIT;
               break;

            case OP_MOV_SAT:
               fp->Saturate = 1;
            case OP_MOV:
               fp->Opcode = FP_OPCODE_MOV;
               break;
         }

         if (parse_fp_dst_reg (ctx, inst, vc_head, Program, &fp->DstReg))
            return 1;

         if (parse_fp_vector_src_reg(ctx, inst, vc_head, Program, &fp->SrcReg[0]))
            return 1;
         break;

      case OP_ALU_SCALAR:
         switch (code) {
            case OP_COS_SAT:
               fp->Saturate = 1;
            case OP_COS:
               fp->Opcode = FP_OPCODE_COS;
               break;

            case OP_EX2_SAT:
               fp->Saturate = 1;
            case OP_EX2:
               fp->Opcode = FP_OPCODE_EX2;
               break;

            case OP_LG2_SAT:
               fp->Saturate = 1;
            case OP_LG2:
               fp->Opcode = FP_OPCODE_LG2;
               break;

            case OP_RCP_SAT:
               fp->Saturate = 1;
            case OP_RCP:
               fp->Opcode = FP_OPCODE_RCP;
               break;

            case OP_RSQ_SAT:
               fp->Saturate = 1;
            case OP_RSQ:
               fp->Opcode = FP_OPCODE_RSQ;
               break;

            case OP_SIN_SAT:
               fp->Saturate = 1;
            case OP_SIN:
               fp->Opcode = FP_OPCODE_SIN;
               break;

            case OP_SCS_SAT:
               fp->Saturate = 1;
            case OP_SCS:

               fp->Opcode = FP_OPCODE_SCS;
               break;
         }

         if (parse_fp_dst_reg (ctx, inst, vc_head, Program, &fp->DstReg))
            return 1;

         if (parse_fp_scalar_src_reg(ctx, inst, vc_head, Program, &fp->SrcReg[0]))
            return 1;
         break;

      case OP_ALU_BINSC:
         switch (code) {
            case OP_POW_SAT:
               fp->Saturate = 1;
            case OP_POW:
               fp->Opcode = FP_OPCODE_POW;
               break;
         }

         if (parse_fp_dst_reg(ctx, inst, vc_head, Program, &fp->DstReg))
            return 1;

         for (a = 0; a < 2; a++) {
	    if (parse_fp_scalar_src_reg(ctx, inst, vc_head, Program, &fp->SrcReg[a]))
               return 1;
         }
         break;


      case OP_ALU_BIN:
         switch (code) {
            case OP_ADD_SAT:
               fp->Saturate = 1;
            case OP_ADD:
               fp->Opcode = FP_OPCODE_ADD;
               break;

            case OP_DP3_SAT:
               fp->Saturate = 1;
            case OP_DP3:
               fp->Opcode = FP_OPCODE_DP3;
               break;

            case OP_DP4_SAT:
               fp->Saturate = 1;
            case OP_DP4:
               fp->Opcode = FP_OPCODE_DP4;
               break;

            case OP_DPH_SAT:
               fp->Saturate = 1;
            case OP_DPH:
               fp->Opcode = FP_OPCODE_DPH;
               break;

            case OP_DST_SAT:
               fp->Saturate = 1;
            case OP_DST:
               fp->Opcode = FP_OPCODE_DST;
               break;

            case OP_MAX_SAT:
               fp->Saturate = 1;
            case OP_MAX:
               fp->Opcode = FP_OPCODE_MAX;
               break;

            case OP_MIN_SAT:
               fp->Saturate = 1;
            case OP_MIN:
               fp->Opcode = FP_OPCODE_MIN;
               break;

            case OP_MUL_SAT:
               fp->Saturate = 1;
            case OP_MUL:
               fp->Opcode = FP_OPCODE_MUL;
               break;

            case OP_SGE_SAT:
               fp->Saturate = 1;
            case OP_SGE:
               fp->Opcode = FP_OPCODE_SGE;
               break;

            case OP_SLT_SAT:
               fp->Saturate = 1;
            case OP_SLT:
               fp->Opcode = FP_OPCODE_SLT;
               break;

            case OP_SUB_SAT:
               fp->Saturate = 1;
            case OP_SUB:
               fp->Opcode = FP_OPCODE_SUB;
               break;

            case OP_XPD_SAT:
               fp->Saturate = 1;
            case OP_XPD:
               fp->Opcode = FP_OPCODE_XPD;
               break;
         }

         if (parse_fp_dst_reg (ctx, inst, vc_head, Program, &fp->DstReg))
            return 1;
         for (a = 0; a < 2; a++) {
	    if (parse_fp_vector_src_reg(ctx, inst, vc_head, Program, &fp->SrcReg[a]))
	       return 1;
         }
         break;

      case OP_ALU_TRI:
         switch (code) {
            case OP_CMP_SAT:
               fp->Saturate = 1;
            case OP_CMP:
               fp->Opcode = FP_OPCODE_CMP;
               break;

            case OP_LRP_SAT:
               fp->Saturate = 1;
            case OP_LRP:
               fp->Opcode = FP_OPCODE_LRP;
               break;

            case OP_MAD_SAT:
               fp->Saturate = 1;
            case OP_MAD:
               fp->Opcode = FP_OPCODE_MAD;
               break;
         }

         if (parse_fp_dst_reg (ctx, inst, vc_head, Program, &fp->DstReg))
            return 1;

         for (a = 0; a < 3; a++) {
	    if (parse_fp_vector_src_reg(ctx, inst, vc_head, Program, &fp->SrcReg[a]))
	       return 1;
         }
         break;

      case OP_ALU_SWZ:
         switch (code) {
            case OP_SWZ_SAT:
               fp->Saturate = 1;
            case OP_SWZ:
               fp->Opcode = FP_OPCODE_SWZ;
               break;
         }
         if (parse_fp_dst_reg (ctx, inst, vc_head, Program, &fp->DstReg))
            return 1;

	 {
	    GLubyte Swizzle[4]; /* FP's swizzle mask is a GLubyte, while VP's is GLuint */
	    GLubyte negateMask;
	    GLint File, Index;

	    if (parse_src_reg(ctx, inst, vc_head, Program, &File, &Index, &rel))
	       return 1;
	    parse_extended_swizzle_mask (inst, Swizzle, &negateMask);
	    fp->SrcReg[0].File = File;
	    fp->SrcReg[0].Index = Index;
	    fp->SrcReg[0].NegateBase = negateMask;
	    fp->SrcReg[0].Swizzle = (Swizzle[0] << 0 |
				     Swizzle[1] << 3 |
				     Swizzle[2] << 6 |
				     Swizzle[3] << 9);
	 }
         break;

      case OP_TEX_SAMPLE:
         switch (code) {
            case OP_TEX_SAT:
               fp->Saturate = 1;
            case OP_TEX:
               fp->Opcode = FP_OPCODE_TEX;
               break;

            case OP_TXP_SAT:
               fp->Saturate = 1;
            case OP_TXP:
               fp->Opcode = FP_OPCODE_TXP;
               break;

            case OP_TXB_SAT:
               fp->Saturate = 1;
            case OP_TXB:
               fp->Opcode = FP_OPCODE_TXB;
               break;
         }

         if (parse_fp_dst_reg (ctx, inst, vc_head, Program, &fp->DstReg))
            return 1;

	 if (parse_fp_vector_src_reg(ctx, inst, vc_head, Program, &fp->SrcReg[0]))
            return 1;

         /* texImageUnit */
         if (parse_texcoord_num (ctx, inst, Program, &texcoord))
            return 1;
         fp->TexSrcUnit = texcoord;

         /* texTarget */
         switch (*(*inst)++) {
            case TEXTARGET_1D:
               fp->TexSrcIdx = TEXTURE_1D_INDEX;
               break;
            case TEXTARGET_2D:
               fp->TexSrcIdx = TEXTURE_2D_INDEX;
               break;
            case TEXTARGET_3D:
               fp->TexSrcIdx = TEXTURE_3D_INDEX;
               break;
            case TEXTARGET_RECT:
               fp->TexSrcIdx = TEXTURE_RECT_INDEX;
               break;
            case TEXTARGET_CUBE:
               fp->TexSrcIdx = TEXTURE_CUBE_INDEX;
               break;
	    case TEXTARGET_SHADOW1D:
	    case TEXTARGET_SHADOW2D:
	    case TEXTARGET_SHADOWRECT:
	       /* TODO ARB_fragment_program_shadow code */
	       break;
         }
         Program->TexturesUsed[texcoord] |= (1<<fp->TexSrcIdx);
         break;

      case OP_TEX_KIL:
	 if (parse_fp_vector_src_reg(ctx, inst, vc_head, Program, &fp->SrcReg[0]))
            return 1;
         fp->Opcode = FP_OPCODE_KIL;
         break;
   }

   return 0;
}

static GLuint 
parse_vp_dst_reg(GLcontext * ctx, GLubyte ** inst,
		 struct var_cache **vc_head, struct arb_program *Program,
		 struct vp_dst_register *reg )
{
   GLint file, idx, mask;

   if (parse_masked_dst_reg(ctx, inst, vc_head, Program, &file, &idx, &mask))
      return 1;

   reg->File = file;
   reg->Index = idx;
   reg->WriteMask = mask;
   return 0;
}

/**
 * Handle the parsing out of a masked address register
 *
 * \param Index     - The register index we write to
 * \param WriteMask - The mask controlling which components we write (1->write)
 *
 * \return 0 on sucess, 1 on error
 */
static GLuint
parse_vp_address_reg (GLcontext * ctx, GLubyte ** inst,
		      struct var_cache **vc_head,
		      struct arb_program *Program,
		      struct vp_dst_register *reg)
{
   GLint idx;

   if (parse_address_reg (ctx, inst, vc_head, Program, &idx))
      return 1;

   /* This should be 0x8 */
   (*inst)++;

   reg->File = PROGRAM_ADDRESS;
   reg->Index = idx;

   /* Writemask of .x is implied */
   reg->WriteMask = 0x1;
   return 0;
}

/**
 */
static GLuint
parse_vp_vector_src_reg (GLcontext * ctx, GLubyte ** inst,
			 struct var_cache **vc_head, struct arb_program *Program,
			 struct vp_src_register *reg )
{

   GLint File;
   GLint Index;
   GLboolean Negate;
   GLubyte Swizzle[4];
   GLboolean IsRelOffset;

   /* Grab the sign */
   Negate = (parse_sign (inst) == -1) ? 0xf : 0x0;

   /* And the src reg */
   if (parse_src_reg (ctx, inst, vc_head, Program, &File, &Index, &IsRelOffset))
      return 1;

   /* finally, the swizzle */
   parse_swizzle_mask (inst, Swizzle, 4);

   reg->File = File;
   reg->Index = Index;
   reg->Swizzle = ((Swizzle[0] << 0) |
		   (Swizzle[1] << 3) |
		   (Swizzle[2] << 6) |
		   (Swizzle[3] << 9));
   reg->Negate = Negate;
   reg->RelAddr = IsRelOffset;
   return 0;
}


static GLuint
parse_vp_scalar_src_reg (GLcontext * ctx, GLubyte ** inst,
			 struct var_cache **vc_head, struct arb_program *Program,
			 struct vp_src_register *reg )
{

   GLint File;
   GLint Index;
   GLboolean Negate;
   GLubyte Swizzle[4];
   GLboolean IsRelOffset;

   /* Grab the sign */
   Negate = (parse_sign (inst) == -1) ? 0x1 : 0x0;

   /* And the src reg */
   if (parse_src_reg (ctx, inst, vc_head, Program, &File, &Index, &IsRelOffset))
      return 1;

   /* finally, the swizzle */
   parse_swizzle_mask (inst, Swizzle, 1);

   reg->File = File;
   reg->Index = Index;
   reg->Swizzle = (Swizzle[0] << 0);
   reg->Negate = Negate;
   reg->RelAddr = IsRelOffset;
   return 0;
}


/**
 * This is a big mother that handles getting opcodes into the instruction
 * and handling the src & dst registers for vertex program instructions
 */
static GLuint
parse_vp_instruction (GLcontext * ctx, GLubyte ** inst,
                      struct var_cache **vc_head, struct arb_program *Program,
                      struct vp_instruction *vp)
{
   GLint a;
   GLubyte type, code;

   /* OP_ALU_{ARL, VECTOR, SCALAR, BINSC, BIN, TRI, SWZ} */
   type = *(*inst)++;

   /* The actual opcode name */
   code = *(*inst)++;

   /* Record the position in the program string for debugging */
   vp->StringPos = Program->Position;
   vp->Data = NULL;
   vp->SrcReg[0].RelAddr = vp->SrcReg[1].RelAddr = vp->SrcReg[2].RelAddr = 0;
   vp->SrcReg[0].Swizzle = SWIZZLE_NOOP;
   vp->SrcReg[1].Swizzle = SWIZZLE_NOOP;
   vp->SrcReg[2].Swizzle = SWIZZLE_NOOP;
   vp->SrcReg[3].Swizzle = SWIZZLE_NOOP;
   vp->DstReg.WriteMask = 0xf;

   switch (type) {
         /* XXX: */
      case OP_ALU_ARL:
         vp->Opcode = VP_OPCODE_ARL;

         /* Remember to set SrcReg.RelAddr; */

         /* Get the masked address register [dst] */
         if (parse_vp_address_reg(ctx, inst, vc_head, Program, &vp->DstReg))
            return 1;

         vp->DstReg.File = PROGRAM_ADDRESS;

         /* Get a scalar src register */
	 if (parse_vp_scalar_src_reg(ctx, inst, vc_head, Program, &vp->SrcReg[0]))
            return 1;

         break;

      case OP_ALU_VECTOR:
         switch (code) {
            case OP_ABS:
               vp->Opcode = VP_OPCODE_ABS;
               break;
            case OP_FLR:
               vp->Opcode = VP_OPCODE_FLR;
               break;
            case OP_FRC:
               vp->Opcode = VP_OPCODE_FRC;
               break;
            case OP_LIT:
               vp->Opcode = VP_OPCODE_LIT;
               break;
            case OP_MOV:
               vp->Opcode = VP_OPCODE_MOV;
               break;
         }

         if (parse_vp_dst_reg(ctx, inst, vc_head, Program, &vp->DstReg))
            return 1;

         if (parse_vp_vector_src_reg(ctx, inst, vc_head, Program, &vp->SrcReg[0]))
            return 1;
         break;

      case OP_ALU_SCALAR:
         switch (code) {
            case OP_EX2:
               vp->Opcode = VP_OPCODE_EX2;
               break;
            case OP_EXP:
               vp->Opcode = VP_OPCODE_EXP;
               break;
            case OP_LG2:
               vp->Opcode = VP_OPCODE_LG2;
               break;
            case OP_LOG:
               vp->Opcode = VP_OPCODE_LOG;
               break;
            case OP_RCP:
               vp->Opcode = VP_OPCODE_RCP;
               break;
            case OP_RSQ:
               vp->Opcode = VP_OPCODE_RSQ;
               break;
         }
         if (parse_vp_dst_reg(ctx, inst, vc_head, Program, &vp->DstReg))
            return 1;

	 if (parse_vp_scalar_src_reg(ctx, inst, vc_head, Program, &vp->SrcReg[0]))
            return 1;
         break;

      case OP_ALU_BINSC:
         switch (code) {
            case OP_POW:
               vp->Opcode = VP_OPCODE_POW;
               break;
         }
         if (parse_vp_dst_reg(ctx, inst, vc_head, Program, &vp->DstReg))
            return 1;

         for (a = 0; a < 2; a++) {
	    if (parse_vp_scalar_src_reg(ctx, inst, vc_head, Program, &vp->SrcReg[a]))
               return 1;
         }
         break;

      case OP_ALU_BIN:
         switch (code) {
            case OP_ADD:
               vp->Opcode = VP_OPCODE_ADD;
               break;
            case OP_DP3:
               vp->Opcode = VP_OPCODE_DP3;
               break;
            case OP_DP4:
               vp->Opcode = VP_OPCODE_DP4;
               break;
            case OP_DPH:
               vp->Opcode = VP_OPCODE_DPH;
               break;
            case OP_DST:
               vp->Opcode = VP_OPCODE_DST;
               break;
            case OP_MAX:
               vp->Opcode = VP_OPCODE_MAX;
               break;
            case OP_MIN:
               vp->Opcode = VP_OPCODE_MIN;
               break;
            case OP_MUL:
               vp->Opcode = VP_OPCODE_MUL;
               break;
            case OP_SGE:
               vp->Opcode = VP_OPCODE_SGE;
               break;
            case OP_SLT:
               vp->Opcode = VP_OPCODE_SLT;
               break;
            case OP_SUB:
               vp->Opcode = VP_OPCODE_SUB;
               break;
            case OP_XPD:
               vp->Opcode = VP_OPCODE_XPD;
               break;
         }
         if (parse_vp_dst_reg(ctx, inst, vc_head, Program, &vp->DstReg))
            return 1;

         for (a = 0; a < 2; a++) {
	    if (parse_vp_vector_src_reg(ctx, inst, vc_head, Program, &vp->SrcReg[a]))
               return 1;
         }
         break;

      case OP_ALU_TRI:
         switch (code) {
            case OP_MAD:
               vp->Opcode = VP_OPCODE_MAD;
               break;
         }

         if (parse_vp_dst_reg(ctx, inst, vc_head, Program, &vp->DstReg))
            return 1;

         for (a = 0; a < 3; a++) {
	    if (parse_vp_vector_src_reg(ctx, inst, vc_head, Program, &vp->SrcReg[a]))
               return 1;
         }
         break;

      case OP_ALU_SWZ:
         switch (code) {
            case OP_SWZ:
               vp->Opcode = VP_OPCODE_SWZ;
               break;
         }
	 {
	    GLubyte Swizzle[4]; /* FP's swizzle mask is a GLubyte, while VP's is GLuint */
	    GLubyte Negate[4];
	    GLboolean RelAddr;
	    GLint File, Index;

	    if (parse_vp_dst_reg(ctx, inst, vc_head, Program, &vp->DstReg))
	       return 1;

	    if (parse_src_reg(ctx, inst, vc_head, Program, &File, &Index, &RelAddr))
	       return 1;
	    parse_extended_swizzle_mask (inst, Swizzle, Negate);
	    vp->SrcReg[0].File = File;
	    vp->SrcReg[0].Index = Index;
	    vp->SrcReg[0].Negate = (Negate[0] << 0 |
				    Negate[1] << 1 |
				    Negate[2] << 2 |
				    Negate[3] << 3);
	    vp->SrcReg[0].Swizzle = (Swizzle[0] << 0 |
				     Swizzle[1] << 3 |
				     Swizzle[2] << 6 |
				     Swizzle[3] << 9);
	    vp->SrcReg[0].RelAddr = RelAddr;
	 }
         break;
   }
   return 0;
}

#if DEBUG_PARSING

static GLvoid
print_state_token (GLint token)
{
   switch (token) {
      case STATE_MATERIAL:
         fprintf (stderr, "STATE_MATERIAL ");
         break;
      case STATE_LIGHT:
         fprintf (stderr, "STATE_LIGHT ");
         break;

      case STATE_LIGHTMODEL_AMBIENT:
         fprintf (stderr, "STATE_AMBIENT ");
         break;

      case STATE_LIGHTMODEL_SCENECOLOR:
         fprintf (stderr, "STATE_SCENECOLOR ");
         break;

      case STATE_LIGHTPROD:
         fprintf (stderr, "STATE_LIGHTPROD ");
         break;

      case STATE_TEXGEN:
         fprintf (stderr, "STATE_TEXGEN ");
         break;

      case STATE_FOG_COLOR:
         fprintf (stderr, "STATE_FOG_COLOR ");
         break;

      case STATE_FOG_PARAMS:
         fprintf (stderr, "STATE_FOG_PARAMS ");
         break;

      case STATE_CLIPPLANE:
         fprintf (stderr, "STATE_CLIPPLANE ");
         break;

      case STATE_POINT_SIZE:
         fprintf (stderr, "STATE_POINT_SIZE ");
         break;

      case STATE_POINT_ATTENUATION:
         fprintf (stderr, "STATE_ATTENUATION ");
         break;

      case STATE_MATRIX:
         fprintf (stderr, "STATE_MATRIX ");
         break;

      case STATE_MODELVIEW:
         fprintf (stderr, "STATE_MODELVIEW ");
         break;

      case STATE_PROJECTION:
         fprintf (stderr, "STATE_PROJECTION ");
         break;

      case STATE_MVP:
         fprintf (stderr, "STATE_MVP ");
         break;

      case STATE_TEXTURE:
         fprintf (stderr, "STATE_TEXTURE ");
         break;

      case STATE_PROGRAM:
         fprintf (stderr, "STATE_PROGRAM ");
         break;

      case STATE_MATRIX_INVERSE:
         fprintf (stderr, "STATE_INVERSE ");
         break;

      case STATE_MATRIX_TRANSPOSE:
         fprintf (stderr, "STATE_TRANSPOSE ");
         break;

      case STATE_MATRIX_INVTRANS:
         fprintf (stderr, "STATE_INVTRANS ");
         break;

      case STATE_AMBIENT:
         fprintf (stderr, "STATE_AMBIENT ");
         break;

      case STATE_DIFFUSE:
         fprintf (stderr, "STATE_DIFFUSE ");
         break;

      case STATE_SPECULAR:
         fprintf (stderr, "STATE_SPECULAR ");
         break;

      case STATE_EMISSION:
         fprintf (stderr, "STATE_EMISSION ");
         break;

      case STATE_SHININESS:
         fprintf (stderr, "STATE_SHININESS ");
         break;

      case STATE_HALF:
         fprintf (stderr, "STATE_HALF ");
         break;

      case STATE_POSITION:
         fprintf (stderr, "STATE_POSITION ");
         break;

      case STATE_ATTENUATION:
         fprintf (stderr, "STATE_ATTENUATION ");
         break;

      case STATE_SPOT_DIRECTION:
         fprintf (stderr, "STATE_DIRECTION ");
         break;

      case STATE_TEXGEN_EYE_S:
         fprintf (stderr, "STATE_TEXGEN_EYE_S ");
         break;

      case STATE_TEXGEN_EYE_T:
         fprintf (stderr, "STATE_TEXGEN_EYE_T ");
         break;

      case STATE_TEXGEN_EYE_R:
         fprintf (stderr, "STATE_TEXGEN_EYE_R ");
         break;

      case STATE_TEXGEN_EYE_Q:
         fprintf (stderr, "STATE_TEXGEN_EYE_Q ");
         break;

      case STATE_TEXGEN_OBJECT_S:
         fprintf (stderr, "STATE_TEXGEN_EYE_S ");
         break;

      case STATE_TEXGEN_OBJECT_T:
         fprintf (stderr, "STATE_TEXGEN_OBJECT_T ");
         break;

      case STATE_TEXGEN_OBJECT_R:
         fprintf (stderr, "STATE_TEXGEN_OBJECT_R ");
         break;

      case STATE_TEXGEN_OBJECT_Q:
         fprintf (stderr, "STATE_TEXGEN_OBJECT_Q ");
         break;

      case STATE_TEXENV_COLOR:
         fprintf (stderr, "STATE_TEXENV_COLOR ");
         break;

      case STATE_DEPTH_RANGE:
         fprintf (stderr, "STATE_DEPTH_RANGE ");
         break;

      case STATE_VERTEX_PROGRAM:
         fprintf (stderr, "STATE_VERTEX_PROGRAM ");
         break;

      case STATE_FRAGMENT_PROGRAM:
         fprintf (stderr, "STATE_FRAGMENT_PROGRAM ");
         break;

      case STATE_ENV:
         fprintf (stderr, "STATE_ENV ");
         break;

      case STATE_LOCAL:
         fprintf (stderr, "STATE_LOCAL ");
         break;

   }
   fprintf (stderr, "[%d] ", token);
}


static GLvoid
debug_variables (GLcontext * ctx, struct var_cache *vc_head,
                 struct arb_program *Program)
{
   struct var_cache *vc;
   GLint a, b;

   fprintf (stderr, "debug_variables, vc_head: %x\n", vc_head);

   /* First of all, print out the contents of the var_cache */
   vc = vc_head;
   while (vc) {
      fprintf (stderr, "[%x]\n", vc);
      switch (vc->type) {
         case vt_none:
            fprintf (stderr, "UNDEFINED %s\n", vc->name);
            break;
         case vt_attrib:
            fprintf (stderr, "ATTRIB    %s\n", vc->name);
            fprintf (stderr, "          binding: 0x%x\n", vc->attrib_binding);
            break;
         case vt_param:
            fprintf (stderr, "PARAM     %s  begin: %d len: %d\n", vc->name,
                     vc->param_binding_begin, vc->param_binding_length);
            b = vc->param_binding_begin;
            for (a = 0; a < vc->param_binding_length; a++) {
               fprintf (stderr, "%s\n",
                        Program->Parameters->Parameters[a + b].Name);
               if (Program->Parameters->Parameters[a + b].Type == STATE) {
                  print_state_token (Program->Parameters->Parameters[a + b].
                                     StateIndexes[0]);
                  print_state_token (Program->Parameters->Parameters[a + b].
                                     StateIndexes[1]);
                  print_state_token (Program->Parameters->Parameters[a + b].
                                     StateIndexes[2]);
                  print_state_token (Program->Parameters->Parameters[a + b].
                                     StateIndexes[3]);
                  print_state_token (Program->Parameters->Parameters[a + b].
                                     StateIndexes[4]);
                  print_state_token (Program->Parameters->Parameters[a + b].
                                     StateIndexes[5]);
               }
               else
                  fprintf (stderr, "%f %f %f %f\n",
                           Program->Parameters->Parameters[a + b].Values[0],
                           Program->Parameters->Parameters[a + b].Values[1],
                           Program->Parameters->Parameters[a + b].Values[2],
                           Program->Parameters->Parameters[a + b].Values[3]);
            }
            break;
         case vt_temp:
            fprintf (stderr, "TEMP      %s\n", vc->name);
            fprintf (stderr, "          binding: 0x%x\n", vc->temp_binding);
            break;
         case vt_output:
            fprintf (stderr, "OUTPUT    %s\n", vc->name);
            fprintf (stderr, "          binding: 0x%x\n", vc->output_binding);
            break;
         case vt_alias:
            fprintf (stderr, "ALIAS     %s\n", vc->name);
            fprintf (stderr, "          binding: 0x%x (%s)\n",
                     vc->alias_binding, vc->alias_binding->name);
            break;
      }
      vc = vc->next;
   }
}

#endif


/**
 * The main loop for parsing a fragment or vertex program
 *
 * \return 0 on sucess, 1 on error
 */
static GLint
parse_arb_program (GLcontext * ctx, GLubyte * inst, struct var_cache **vc_head,
                   struct arb_program *Program)
{
   GLint err = 0;

   Program->MajorVersion = (GLuint) * inst++;
   Program->MinorVersion = (GLuint) * inst++;

   while (*inst != END) {
      switch (*inst++) {

         case OPTION:
            switch (*inst++) {
               case ARB_PRECISION_HINT_FASTEST:
                  Program->PrecisionOption = GL_FASTEST;
                  break;

               case ARB_PRECISION_HINT_NICEST:
                  Program->PrecisionOption = GL_NICEST;
                  break;

               case ARB_FOG_EXP:
                  Program->FogOption = GL_EXP;
                  break;

               case ARB_FOG_EXP2:
                  Program->FogOption = GL_EXP2;
                  break;

               case ARB_FOG_LINEAR:
                  Program->FogOption = GL_LINEAR;
                  break;

               case ARB_POSITION_INVARIANT:
                  if (Program->Base.Target == GL_VERTEX_PROGRAM_ARB)
                     Program->HintPositionInvariant = 1;
                  break;

               case ARB_FRAGMENT_PROGRAM_SHADOW:
	          if (Program->Base.Target == GL_FRAGMENT_PROGRAM_ARB) {
	             /* TODO ARB_fragment_program_shadow code */
		  }
		  break;

               case ARB_DRAW_BUFFERS:
	          if (Program->Base.Target == GL_FRAGMENT_PROGRAM_ARB) {
                     /* do nothing for now */
                  }
                  break;
            }
            break;

         case INSTRUCTION:
            Program->Position = parse_position (&inst);

            if (Program->Base.Target == GL_FRAGMENT_PROGRAM_ARB) {

               /* Check the instruction count
                * XXX: Does END count as an instruction?
                */
               if (Program->Base.NumInstructions+1 == MAX_NV_FRAGMENT_PROGRAM_INSTRUCTIONS) {
                  _mesa_set_program_error (ctx, Program->Position,
                      "Max instruction count exceeded!");
                  _mesa_error (ctx, GL_INVALID_OPERATION,
                      "Max instruction count exceeded!");
               }

               /* Realloc Program->FPInstructions */
               Program->FPInstructions =
                  (struct fp_instruction *) _mesa_realloc (Program->FPInstructions,
                                                           Program->Base.NumInstructions*sizeof(struct fp_instruction),
                                                           (Program->Base.NumInstructions+1)*sizeof (struct fp_instruction));

               /* parse the current instruction   */
               err = parse_fp_instruction (ctx, &inst, vc_head, Program,
                                           &Program->FPInstructions[Program->Base.NumInstructions]);

            }
            else {
               /* Check the instruction count
                * XXX: Does END count as an instruction?
                */
               if (Program->Base.NumInstructions+1 == MAX_NV_VERTEX_PROGRAM_INSTRUCTIONS) {
                  _mesa_set_program_error (ctx, Program->Position,
                      "Max instruction count exceeded!");
                  _mesa_error (ctx, GL_INVALID_OPERATION,
                      "Max instruction count exceeded!");
               }

               /* Realloc Program->VPInstructions */
               Program->VPInstructions =
                  (struct vp_instruction *) _mesa_realloc (Program->VPInstructions,
                                                           Program->Base.NumInstructions*sizeof(struct vp_instruction),
                                                           (Program->Base.NumInstructions +1)*sizeof(struct vp_instruction));

               /* parse the current instruction   */
               err = parse_vp_instruction (ctx, &inst, vc_head, Program,
                                           &Program->VPInstructions[Program->Base.NumInstructions]);
            }

            /* increment Program->Base.NumInstructions */
            Program->Base.NumInstructions++;
            break;

         case DECLARATION:
            err = parse_declaration (ctx, &inst, vc_head, Program);
            break;

         default:
            break;
      }

      if (err)
         break;
   }

   /* Finally, tag on an OPCODE_END instruction */
   if (Program->Base.Target == GL_FRAGMENT_PROGRAM_ARB) {
      Program->FPInstructions =
         (struct fp_instruction *) _mesa_realloc (Program->FPInstructions,
						  Program->Base.NumInstructions*sizeof(struct fp_instruction),
                                                  (Program->Base.NumInstructions+1)*sizeof(struct fp_instruction));

      Program->FPInstructions[Program->Base.NumInstructions].Opcode = FP_OPCODE_END;
      Program->FPInstructions[Program->Base.NumInstructions].Saturate = 0;
      Program->FPInstructions[Program->Base.NumInstructions].DstReg.File = 0xf;
      Program->FPInstructions[Program->Base.NumInstructions].SrcReg[0].File = 0xf;
      Program->FPInstructions[Program->Base.NumInstructions].SrcReg[1].File = 0xf;
      Program->FPInstructions[Program->Base.NumInstructions].SrcReg[2].File = 0xf;
      /* YYY Wrong Position in program, whatever, at least not random -> crash
	 Program->Position = parse_position (&inst);
      */
      Program->FPInstructions[Program->Base.NumInstructions].StringPos = Program->Position;
      Program->FPInstructions[Program->Base.NumInstructions].Data = NULL;
   }
   else {
      Program->VPInstructions =
         (struct vp_instruction *) _mesa_realloc (Program->VPInstructions,
                                                  Program->Base.NumInstructions*sizeof(struct vp_instruction),
                                                  (Program->Base.NumInstructions+1)*sizeof(struct vp_instruction));

      Program->VPInstructions[Program->Base.NumInstructions].Opcode = VP_OPCODE_END;
      /* YYY Wrong Position in program, whatever, at least not random -> crash
	 Program->Position = parse_position (&inst);
      */
      Program->VPInstructions[Program->Base.NumInstructions].StringPos = Program->Position;
      Program->VPInstructions[Program->Base.NumInstructions].Data = NULL;
   }

   /* increment Program->Base.NumInstructions */
   Program->Base.NumInstructions++;

   return err;
}

/* XXX temporary */
__extension__ static char core_grammar_text[] =
#include "grammar_syn.h"
;

static int set_reg8 (GLcontext *ctx, grammar id, const byte *name, byte value)
{
   char error_msg[300];
   GLint error_pos;

   if (grammar_set_reg8 (id, name, value))
      return 0;

   grammar_get_last_error ((byte *) error_msg, 300, &error_pos);
   _mesa_set_program_error (ctx, error_pos, error_msg);
   _mesa_error (ctx, GL_INVALID_OPERATION, "Grammar Register Error");
   return 1;
}

static int extension_is_supported (const GLubyte *ext)
{
   const GLubyte *extensions = CALL_GetString(GET_DISPATCH(), (GL_EXTENSIONS));
   const GLubyte *end = extensions + _mesa_strlen ((const char *) extensions);
   const GLint ext_len = (GLint)_mesa_strlen ((const char *) ext);

   while (extensions < end)
   {
      const GLubyte *name_end = (const GLubyte *) strchr ((const char *) extensions, ' ');
      if (name_end == NULL)
         name_end = end;
      if (name_end - extensions == ext_len && _mesa_strncmp ((const char *) ext,
         (const char *) extensions, ext_len) == 0)
         return 1;
      extensions = name_end + 1;
   }

   return 0;
}

static int enable_ext (GLcontext *ctx, grammar id, const byte *name, const byte *extname)
{
   if (extension_is_supported (extname))
      if (set_reg8 (ctx, id, name, 0x01))
         return 1;
   return 0;
}

/**
 * This kicks everything off.
 *
 * \param ctx - The GL Context
 * \param str - The program string
 * \param len - The program string length
 * \param Program - The arb_program struct to return all the parsed info in
 * \return 0 on sucess, 1 on error
 */
GLuint
_mesa_parse_arb_program (GLcontext * ctx, const GLubyte * str, GLsizei len,
                         struct arb_program * program)
{
   GLint a, err, error_pos;
   char error_msg[300];
   GLuint parsed_len;
   struct var_cache *vc_head;
   grammar arbprogram_syn_id;
   GLubyte *parsed, *inst;
   GLubyte *strz = NULL;
   static int arbprogram_syn_is_ok = 0;		/* XXX temporary */

   /* Reset error state */
   _mesa_set_program_error(ctx, -1, NULL);

#if DEBUG_PARSING
   fprintf (stderr, "Loading grammar text!\n");
#endif

   /* check if the arb_grammar_text (arbprogram.syn) is syntactically correct */
   if (!arbprogram_syn_is_ok) {
      grammar grammar_syn_id;
      GLint err;
      GLuint parsed_len;
      byte *parsed;

      grammar_syn_id = grammar_load_from_text ((byte *) core_grammar_text);
      if (grammar_syn_id == 0) {
         grammar_get_last_error ((byte *) error_msg, 300, &error_pos);
         _mesa_set_program_error (ctx, error_pos, error_msg);
         _mesa_error (ctx, GL_INVALID_OPERATION,
                      "Error loading grammar rule set");
         return 1;
      }

      err = grammar_check (grammar_syn_id, (byte *) arb_grammar_text, &parsed, &parsed_len);

      /* NOTE: we cant destroy grammar_syn_id right here because grammar_destroy() can
         reset the last error
      */

      if (err == 0) {
         grammar_get_last_error ((byte *) error_msg, 300, &error_pos);
         _mesa_set_program_error (ctx, error_pos, error_msg);
         _mesa_error (ctx, GL_INVALID_OPERATION, "Error loading grammar rule set");

         grammar_destroy (grammar_syn_id);
         return 1;
      }

      grammar_destroy (grammar_syn_id);

      arbprogram_syn_is_ok = 1;
   }

   /* create the grammar object */
   arbprogram_syn_id = grammar_load_from_text ((byte *) arb_grammar_text);
   if (arbprogram_syn_id == 0) {
      grammar_get_last_error ((GLubyte *) error_msg, 300, &error_pos);
      _mesa_set_program_error (ctx, error_pos, error_msg);
      _mesa_error (ctx, GL_INVALID_OPERATION,
                   "Error loading grammer rule set");
      return 1;
   }

   /* Set program_target register value */
   if (set_reg8 (ctx, arbprogram_syn_id, (byte *) "program_target",
      program->Base.Target == GL_FRAGMENT_PROGRAM_ARB ? 0x10 : 0x20)) {
      grammar_destroy (arbprogram_syn_id);
      return 1;
   }

   /* Enable all active extensions */
   if (enable_ext (ctx, arbprogram_syn_id,
          (byte *) "vertex_blend", (byte *) "GL_ARB_vertex_blend") ||
       enable_ext (ctx, arbprogram_syn_id,
          (byte *) "vertex_blend", (byte *) "GL_EXT_vertex_weighting") ||
       enable_ext (ctx, arbprogram_syn_id,
          (byte *) "matrix_palette", (byte *) "GL_ARB_matrix_palette") ||
       enable_ext (ctx, arbprogram_syn_id,
          (byte *) "point_parameters", (byte *) "GL_ARB_point_parameters") ||
       enable_ext (ctx, arbprogram_syn_id,
          (byte *) "point_parameters", (byte *) "GL_EXT_point_parameters") ||
       enable_ext (ctx, arbprogram_syn_id,
          (byte *) "secondary_color", (byte *) "GL_EXT_secondary_color") ||
       enable_ext (ctx, arbprogram_syn_id,
          (byte *) "fog_coord", (byte *) "GL_EXT_fog_coord") ||
       enable_ext (ctx, arbprogram_syn_id,
          (byte *) "texture_rectangle", (byte *) "GL_ARB_texture_rectangle") ||
       enable_ext (ctx, arbprogram_syn_id,
          (byte *) "texture_rectangle", (byte *) "GL_EXT_texture_rectangle") ||
       enable_ext (ctx, arbprogram_syn_id,
          (byte *) "texture_rectangle", (byte *) "GL_NV_texture_rectangle") ||
       enable_ext (ctx, arbprogram_syn_id,
          (byte *) "fragment_program_shadow", (byte *) "GL_ARB_fragment_program_shadow") ||
       enable_ext (ctx, arbprogram_syn_id,
          (byte *) "draw_buffers", (byte *) "GL_ARB_draw_buffers")) {
      grammar_destroy (arbprogram_syn_id);
      return 1;
   }

   /* check for NULL character occurences */
   {
      int i;
      for (i = 0; i < len; i++)
         if (str[i] == '\0') {
            _mesa_set_program_error (ctx, i, "invalid character");
            _mesa_error (ctx, GL_INVALID_OPERATION, "Lexical Error");

            grammar_destroy (arbprogram_syn_id);
            return 1;
         }
   }

   /* copy the program string to a null-terminated string */
   /* XXX should I check for NULL from malloc()? */
   strz = (GLubyte *) _mesa_malloc (len + 1);
   _mesa_memcpy (strz, str, len);
   strz[len] = '\0';

#if DEBUG_PARSING
   printf ("Checking Grammar!\n");
#endif
   /* do a fast check on program string - initial production buffer is 4K */
   err = grammar_fast_check (arbprogram_syn_id, strz, &parsed, &parsed_len, 0x1000);

   /* Syntax parse error */
   if (err == 0) {
      _mesa_free (strz);
      grammar_get_last_error ((GLubyte *) error_msg, 300, &error_pos);
      _mesa_set_program_error (ctx, error_pos, error_msg);
      _mesa_error (ctx, GL_INVALID_OPERATION, "glProgramStringARB(syntax error)");

      /* useful for debugging */
      if (0) {
         int line, col;
         char *s;
         printf("Program: %s\n", (char *) strz);
         printf("Error Pos: %d\n", ctx->Program.ErrorPos);
         s = (char *) _mesa_find_line_column(strz, strz+ctx->Program.ErrorPos, &line, &col);
         printf("line %d col %d: %s\n", line, col, s);
      }

      grammar_destroy (arbprogram_syn_id);
      return 1;
   }

#if DEBUG_PARSING
   printf ("Destroying grammer dict [parse retval: %d]\n", err);
#endif
   grammar_destroy (arbprogram_syn_id);

   /* Initialize the arb_program struct */
   program->Base.String = strz;
   program->Base.NumInstructions =
   program->Base.NumTemporaries =
   program->Base.NumParameters =
   program->Base.NumAttributes = program->Base.NumAddressRegs = 0;
   program->Parameters = _mesa_new_parameter_list ();
   program->InputsRead = 0;
   program->OutputsWritten = 0;
   program->Position = 0;
   program->MajorVersion = program->MinorVersion = 0;
   program->PrecisionOption = GL_DONT_CARE;
   program->FogOption = GL_NONE;
   program->HintPositionInvariant = GL_FALSE;
   for (a = 0; a < MAX_TEXTURE_IMAGE_UNITS; a++)
      program->TexturesUsed[a] = 0;
   program->NumAluInstructions =
   program->NumTexInstructions =
   program->NumTexIndirections = 0;

   program->FPInstructions = NULL;
   program->VPInstructions = NULL;

   vc_head = NULL;
   err = 0;

   /* Start examining the tokens in the array */
   inst = parsed;

   /* Check the grammer rev */
   if (*inst++ != REVISION) {
      _mesa_set_program_error (ctx, 0, "Grammar version mismatch");
      _mesa_error (ctx, GL_INVALID_OPERATION, "glProgramStringARB(Grammar verison mismatch)");
      err = 1;
   }
   else {
      /* ignore program target */
      inst++;

      err = parse_arb_program (ctx, inst, &vc_head, program);
#if DEBUG_PARSING
      fprintf (stderr, "Symantic analysis returns %d [1 is bad!]\n", err);
#endif
   }

   /*debug_variables(ctx, vc_head, program); */

   /* We're done with the parsed binary array */
   var_cache_destroy (&vc_head);

   _mesa_free (parsed);
#if DEBUG_PARSING
   printf ("_mesa_parse_arb_program() done\n");
#endif
   return err;
}

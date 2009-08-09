/*
 * Mesa 3-D graphics library
 * Version:  7.1
 *
 * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
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

#include "main/glheader.h"
#include "main/imports.h"
#include "main/context.h"
#include "main/macros.h"
#include "main/mtypes.h"
#include "shader/grammar/grammar_mesa.h"
#include "arbprogparse.h"
#include "program.h"
#include "prog_parameter.h"
#include "prog_statevars.h"
#include "prog_instruction.h"


/* For ARB programs, use the NV instruction limits */
#define MAX_INSTRUCTIONS MAX2(MAX_NV_FRAGMENT_PROGRAM_INSTRUCTIONS, \
                              MAX_NV_VERTEX_PROGRAM_INSTRUCTIONS)


/**
 * This is basically a union of the vertex_program and fragment_program
 * structs that we can use to parse the program into
 *
 * XXX we can probably get rid of this entirely someday.
 */
struct arb_program
{
   struct gl_program Base;

   GLuint Position;       /* Just used for error reporting while parsing */
   GLuint MajorVersion;
   GLuint MinorVersion;

   /* ARB_vertex_progmra options */
   GLboolean HintPositionInvariant;

   /* ARB_fragment_progmra options */
   GLenum PrecisionOption; /* GL_DONT_CARE, GL_NICEST or GL_FASTEST */
   GLenum FogOption;       /* GL_NONE, GL_LINEAR, GL_EXP or GL_EXP2 */

   /* ARB_fragment_program specifics */
   GLbitfield TexturesUsed[MAX_TEXTURE_IMAGE_UNITS]; 
   GLbitfield ShadowSamplers;
   GLuint NumAluInstructions; 
   GLuint NumTexInstructions;
   GLuint NumTexIndirections;

   GLboolean UsesKill;
};



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
LONGSTRING static char arb_grammar_text[] =
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
#define  REVISION                                   0x0a

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

/* GL_MESA_texture_array option */
#define  MESA_TEXTURE_ARRAY                        0x08

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
/* GL_MESA_texture_array */
#define  TEXTARGET_1D_ARRAY                         0x09
#define  TEXTARGET_2D_ARRAY                         0x0a
#define  TEXTARGET_SHADOW1D_ARRAY                   0x0b
#define  TEXTARGET_SHADOW2D_ARRAY                   0x0c

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


/**
 * Setting an explicit field for each of the binding properties is a bit
 * wasteful of space, but it should be much more clear when reading later on..
 */
struct var_cache
{
   const GLubyte *name;         /* don't free() - no need */
   var_type type;
   GLuint address_binding;      /* The index of the address register we should
                                 * be using                                        */
   GLuint attrib_binding;       /* For type vt_attrib, see nvfragprog.h for values */
   GLuint attrib_is_generic;    /* If the attrib was specified through a generic
                                 * vertex attrib                                   */
   GLuint temp_binding;         /* The index of the temp register we are to use    */
   GLuint output_binding;       /* Output/result register number */
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
var_cache_find (struct var_cache *va, const GLubyte * name)
{
   /*struct var_cache *first = va;*/

   while (va) {
      if (!_mesa_strcmp ( (const char*) name, (const char*) va->name)) {
         if (va->type == vt_alias)
            return va->alias_binding;
         return va;
      }

      va = va->next;
   }

   return NULL;
}



/**
 * Called when an error is detected while parsing/compiling a program.
 * Sets the ctx->Program.ErrorString field to descript and records a
 * GL_INVALID_OPERATION error.
 * \param position  position of error in program string
 * \param descrip  verbose error description
 */
static void
program_error(GLcontext *ctx, GLint position, const char *descrip)
{
   if (descrip) {
      const char *prefix = "glProgramString(", *suffix = ")";
      char *str = (char *) _mesa_malloc(_mesa_strlen(descrip) +
                                        _mesa_strlen(prefix) +
                                        _mesa_strlen(suffix) + 1);
      if (str) {
         _mesa_sprintf(str, "%s%s%s", prefix, descrip, suffix);
         _mesa_error(ctx, GL_INVALID_OPERATION, str);
         _mesa_free(str);
      }
   }
   _mesa_set_program_error(ctx, position, descrip);
}


/**
 * As above, but with an extra string parameter for more info.
 */
static void
program_error2(GLcontext *ctx, GLint position, const char *descrip,
               const char *var)
{
   if (descrip) {
      const char *prefix = "glProgramString(", *suffix = ")";
      char *str = (char *) _mesa_malloc(_mesa_strlen(descrip) +
                                        _mesa_strlen(": ") +
                                        _mesa_strlen(var) +
                                        _mesa_strlen(prefix) +
                                        _mesa_strlen(suffix) + 1);
      if (str) {
         _mesa_sprintf(str, "%s%s: %s%s", prefix, descrip, var, suffix);
         _mesa_error(ctx, GL_INVALID_OPERATION, str);
         _mesa_free(str);
      }
   }
   {
      char *str = (char *) _mesa_malloc(_mesa_strlen(descrip) +
                                        _mesa_strlen(": ") +
                                        _mesa_strlen(var) + 1);
      if (str) {
         _mesa_sprintf(str, "%s: %s", descrip, var);
      }
      _mesa_set_program_error(ctx, position, str);
      if (str) {
         _mesa_free(str);
      }
   }
}



/**
 * constructs an integer from 4 GLubytes in LE format
 */
static GLuint
parse_position (const GLubyte ** inst)
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
parse_string (const GLubyte ** inst, struct var_cache **vc_head,
              struct arb_program *Program, GLuint * found)
{
   const GLubyte *i = *inst;
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
   va->name = (const GLubyte *) i;

   var_cache_append (vc_head, va);

   return va;
}

static char *
parse_string_without_adding (const GLubyte ** inst, struct arb_program *Program)
{
   const GLubyte *i = *inst;
   (void) Program;
   
   *inst += _mesa_strlen ((char *) i) + 1;

   return (char *) i;
}

/**
 * \return -1 if we parse '-', return 1 otherwise
 */
static GLint
parse_sign (const GLubyte ** inst)
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
parse_integer (const GLubyte ** inst, struct arb_program *Program)
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
parse_float_string(const GLubyte ** inst, struct arb_program *Program, GLdouble *scale)
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
parse_float (const GLubyte ** inst, struct arb_program *Program)
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
parse_signed_float (const GLubyte ** inst, struct arb_program *Program)
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
parse_constant (const GLubyte ** inst, GLfloat *values, struct arb_program *Program,
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
parse_relative_offset(GLcontext *ctx, const GLubyte **inst,
                      struct arb_program *Program, GLint *offset)
{
   (void) ctx;
   *offset = parse_integer(inst, Program);
   return 0;
}

/**
 * \param  color 0 if color type is primary, 1 if color type is secondary
 * \return 0 on sucess, 1 on error
 */
static GLuint
parse_color_type (GLcontext * ctx, const GLubyte ** inst, struct arb_program *Program,
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
parse_generic_attrib_num(GLcontext *ctx, const GLubyte ** inst,
                       struct arb_program *Program, GLuint *attrib)
{
   GLint i = parse_integer(inst, Program);

   if ((i < 0) || (i >= MAX_VERTEX_PROGRAM_ATTRIBS))
   {
      program_error(ctx, Program->Position,
                    "Invalid generic vertex attribute index");
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
parse_output_color_num (GLcontext * ctx, const GLubyte ** inst,
                    struct arb_program *Program, GLuint * color)
{
   GLint i = parse_integer (inst, Program);

   if ((i < 0) || (i >= (int)ctx->Const.MaxDrawBuffers)) {
      program_error(ctx, Program->Position, "Invalid draw buffer index");
      return 1;
   }

   *color = (GLuint) i;
   return 0;
}


/**
 * Validate the index of a texture coordinate
 *
 * \param coord The texture unit index
 * \return 0 on sucess, 1 on error
 */
static GLuint
parse_texcoord_num (GLcontext * ctx, const GLubyte ** inst,
                    struct arb_program *Program, GLuint * coord)
{
   GLint i = parse_integer (inst, Program);

   if ((i < 0) || (i >= (int)ctx->Const.MaxTextureCoordUnits)) {
      program_error(ctx, Program->Position, "Invalid texture coordinate index");
      return 1;
   }

   *coord = (GLuint) i;
   return 0;
}


/**
 * Validate the index of a texture image unit
 *
 * \param coord The texture unit index
 * \return 0 on sucess, 1 on error
 */
static GLuint
parse_teximage_num (GLcontext * ctx, const GLubyte ** inst,
                    struct arb_program *Program, GLuint * coord)
{
   GLint i = parse_integer (inst, Program);

   if ((i < 0) || (i >= (int)ctx->Const.MaxTextureImageUnits)) {
      program_error(ctx, Program->Position, "Invalid texture image index");
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
parse_weight_num (GLcontext * ctx, const GLubyte ** inst, struct arb_program *Program,
                  GLint * coord)
{
   *coord = parse_integer (inst, Program);

   if ((*coord < 0) || (*coord >= 1)) {
      program_error(ctx, Program->Position, "Invalid weight index");
      return 1;
   }

   return 0;
}

/**
 * \param coord The clip plane index
 * \return 0 on sucess, 1 on error
 */
static GLuint
parse_clipplane_num (GLcontext * ctx, const GLubyte ** inst,
                     struct arb_program *Program, GLint * coord)
{
   *coord = parse_integer (inst, Program);

   if ((*coord < 0) || (*coord >= (GLint) ctx->Const.MaxClipPlanes)) {
      program_error(ctx, Program->Position, "Invalid clip plane index");
      return 1;
   }

   return 0;
}


/**
 * \return 0 on front face, 1 on back face
 */
static GLuint
parse_face_type (const GLubyte ** inst)
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
parse_matrix (GLcontext * ctx, const GLubyte ** inst, struct arb_program *Program,
              GLint * matrix, GLint * matrix_idx, GLint * matrix_modifier)
{
   GLubyte mat = *(*inst)++;

   *matrix_idx = 0;

   switch (mat) {
      case MATRIX_MODELVIEW:
         *matrix = STATE_MODELVIEW_MATRIX;
         *matrix_idx = parse_integer (inst, Program);
         if (*matrix_idx > 0) {
            program_error(ctx, Program->Position,
                          "ARB_vertex_blend not supported");
            return 1;
         }
         break;

      case MATRIX_PROJECTION:
         *matrix = STATE_PROJECTION_MATRIX;
         break;

      case MATRIX_MVP:
         *matrix = STATE_MVP_MATRIX;
         break;

      case MATRIX_TEXTURE:
         *matrix = STATE_TEXTURE_MATRIX;
         *matrix_idx = parse_integer (inst, Program);
         if (*matrix_idx >= (GLint) ctx->Const.MaxTextureUnits) {
            program_error(ctx, Program->Position, "Invalid Texture Unit");
            /* bad *matrix_id */
            return 1;
         }
         break;

         /* This is not currently supported (ARB_matrix_palette) */
      case MATRIX_PALETTE:
         *matrix_idx = parse_integer (inst, Program);
         program_error(ctx, Program->Position,
                       "ARB_matrix_palette not supported");
         return 1;
         break;

      case MATRIX_PROGRAM:
         *matrix = STATE_PROGRAM_MATRIX;
         *matrix_idx = parse_integer (inst, Program);
         if (*matrix_idx >= (GLint) ctx->Const.MaxProgramMatrices) {
            program_error(ctx, Program->Position, "Invalid Program Matrix");
            /* bad *matrix_idx */
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
parse_state_single_item (GLcontext * ctx, const GLubyte ** inst,
                         struct arb_program *Program,
                         gl_state_index state_tokens[STATE_LENGTH])
{
   GLubyte token = *(*inst)++;

   switch (token) {
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
            program_error(ctx, Program->Position, "Invalid Light Number");
            /* bad state_tokens[1] */
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
               state_tokens[2] = STATE_HALF_VECTOR;
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
            program_error(ctx, Program->Position, "Invalid Light Number");
            /* bad state_tokens[1] */
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
            type = *(*inst)++;

            /* 0 - s, 1 - t, 2 - r, 3 - q */
            coord = *(*inst)++;

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
                  default:
                     _mesa_problem(ctx, "bad texgen component in "
                                   "parse_state_single_item()");
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
                  default:
                     _mesa_problem(ctx, "bad texgen component in "
                                   "parse_state_single_item()");
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
         if (parse_clipplane_num (ctx, inst, Program,
                                  (GLint *) &state_tokens[1]))
            return 1;
         break;

      case STATE_POINT:
         switch (*(*inst)++) {
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
         if (parse_matrix(ctx, inst, Program,
                          (GLint *) &state_tokens[0],
                          (GLint *) &state_tokens[1],
                          (GLint *) &state_tokens[4]))
            return 1;

         state_tokens[2] = parse_integer (inst, Program);       /* The first row to grab */

         if ((**inst) != 0) {                                   /* Either the last row, 0 */
            state_tokens[3] = parse_integer (inst, Program);
            if (state_tokens[3] < state_tokens[2]) {
               program_error(ctx, Program->Position,
                             "Second matrix index less than the first");
               /* state_tokens[4] vs. state_tokens[3] */
               return 1;
            }
         }
         else {
            state_tokens[3] = state_tokens[2];
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
parse_program_single_item (GLcontext * ctx, const GLubyte ** inst,
                           struct arb_program *Program,
                           gl_state_index state_tokens[STATE_LENGTH])
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
              (state_tokens[2] >= (GLint) ctx->Const.FragmentProgram.MaxEnvParams))
             ||
             ((Program->Base.Target == GL_VERTEX_PROGRAM_ARB) &&
              (state_tokens[2] >= (GLint) ctx->Const.VertexProgram.MaxEnvParams))) {
            program_error(ctx, Program->Position,
                          "Invalid Program Env Parameter");
            /* bad state_tokens[2] */
            return 1;
         }

         break;

      case PROGRAM_PARAM_LOCAL:
         state_tokens[1] = STATE_LOCAL;
         state_tokens[2] = parse_integer (inst, Program);

         /* Check state_tokens[2] against the number of LOCAL parameters available */
         if (((Program->Base.Target == GL_FRAGMENT_PROGRAM_ARB) &&
              (state_tokens[2] >= (GLint) ctx->Const.FragmentProgram.MaxLocalParams))
             ||
             ((Program->Base.Target == GL_VERTEX_PROGRAM_ARB) &&
              (state_tokens[2] >= (GLint) ctx->Const.VertexProgram.MaxLocalParams))) {
            program_error(ctx, Program->Position,
                          "Invalid Program Local Parameter");
            /* bad state_tokens[2] */
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
            genericAttrib[ curr->attrib_binding ] = GL_TRUE;
         else
            explicitAttrib[ curr->attrib_binding ] = GL_TRUE;
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
 * \param inputReg  returns the input register index, one of the
 *                  VERT_ATTRIB_* or FRAG_ATTRIB_* values.
 * \return returns 0 on success, 1 on error
 */
static GLuint
parse_attrib_binding(GLcontext * ctx, const GLubyte ** inst,
                     struct arb_program *Program,
                     GLuint *inputReg, GLuint *is_generic)
{
   GLint err = 0;

   *is_generic = 0;

   if (Program->Base.Target == GL_FRAGMENT_PROGRAM_ARB) {
      switch (*(*inst)++) {
         case FRAGMENT_ATTRIB_COLOR:
            {
               GLint coord;
               err = parse_color_type (ctx, inst, Program, &coord);
               *inputReg = FRAG_ATTRIB_COL0 + coord;
            }
            break;
         case FRAGMENT_ATTRIB_TEXCOORD:
            {
               GLuint texcoord = 0;
               err = parse_texcoord_num (ctx, inst, Program, &texcoord);
               *inputReg = FRAG_ATTRIB_TEX0 + texcoord;
            }
            break;
         case FRAGMENT_ATTRIB_FOGCOORD:
            *inputReg = FRAG_ATTRIB_FOGC;
            break;
         case FRAGMENT_ATTRIB_POSITION:
            *inputReg = FRAG_ATTRIB_WPOS;
            break;
         default:
            err = 1;
            break;
      }
   }
   else {
      switch (*(*inst)++) {
         case VERTEX_ATTRIB_POSITION:
            *inputReg = VERT_ATTRIB_POS;
            break;

         case VERTEX_ATTRIB_WEIGHT:
            {
               GLint weight;
               err = parse_weight_num (ctx, inst, Program, &weight);
               *inputReg = VERT_ATTRIB_WEIGHT;
#if 1
               /* hack for Warcraft (see bug 8060) */
               _mesa_warning(ctx, "Application error: vertex program uses 'vertex.weight' but GL_ARB_vertex_blend not supported.");
               break;
#else
               program_error(ctx, Program->Position,
                             "ARB_vertex_blend not supported");
               return 1;
#endif
            }

         case VERTEX_ATTRIB_NORMAL:
            *inputReg = VERT_ATTRIB_NORMAL;
            break;

         case VERTEX_ATTRIB_COLOR:
            {
               GLint color;
               err = parse_color_type (ctx, inst, Program, &color);
               if (color) {
                  *inputReg = VERT_ATTRIB_COLOR1;
               }
               else {
                  *inputReg = VERT_ATTRIB_COLOR0;
               }
            }
            break;

         case VERTEX_ATTRIB_FOGCOORD:
            *inputReg = VERT_ATTRIB_FOG;
            break;

         case VERTEX_ATTRIB_TEXCOORD:
            {
               GLuint unit = 0;
               err = parse_texcoord_num (ctx, inst, Program, &unit);
               *inputReg = VERT_ATTRIB_TEX0 + unit;
            }
            break;

         case VERTEX_ATTRIB_MATRIXINDEX:
            /* Not supported at this time */
            {
               const char *msg = "ARB_palette_matrix not supported";
               parse_integer (inst, Program);
               program_error(ctx, Program->Position, msg);
            }
            return 1;

         case VERTEX_ATTRIB_GENERIC:
            {
               GLuint attrib;
               err = parse_generic_attrib_num(ctx, inst, Program, &attrib);
               if (!err) {
                  *is_generic = 1;
                  /* Add VERT_ATTRIB_GENERIC0 here because ARB_vertex_program's
                   * attributes do not alias the conventional vertex
                   * attributes.
                   */
                  if (attrib > 0)
                     *inputReg = attrib + VERT_ATTRIB_GENERIC0;
                  else
                     *inputReg = 0;
               }
            }
            break;

         default:
            err = 1;
            break;
      }
   }

   if (err) {
      program_error(ctx, Program->Position, "Bad attribute binding");
   }

   return err;
}


/**
 * This translates between a binary token for an output variable type
 * and the mesa token for the same thing.
 *
 * \param inst       The parsed tokens
 * \param outputReg  Returned index/number of the output register,
 *                   one of the VERT_RESULT_* or FRAG_RESULT_* values.
 */
static GLuint
parse_result_binding(GLcontext *ctx, const GLubyte **inst,
                     GLuint *outputReg, struct arb_program *Program)
{
   const GLubyte token = *(*inst)++;

   switch (token) {
      case FRAGMENT_RESULT_COLOR:
         if (Program->Base.Target == GL_FRAGMENT_PROGRAM_ARB) {
            GLuint out_color;

            /* This gets result of the color buffer we're supposed to 
             * draw into.  This pertains to GL_ARB_draw_buffers.
             */
            parse_output_color_num(ctx, inst, Program, &out_color);
            ASSERT(out_color < MAX_DRAW_BUFFERS);
            *outputReg = FRAG_RESULT_COLR;
         }
         else {
            /* for vtx programs, this is VERTEX_RESULT_POSITION */
            *outputReg = VERT_RESULT_HPOS;
         }
         break;

      case FRAGMENT_RESULT_DEPTH:
         if (Program->Base.Target == GL_FRAGMENT_PROGRAM_ARB) {
            /* for frag programs, this is FRAGMENT_RESULT_DEPTH */
            *outputReg = FRAG_RESULT_DEPR;
         }
         else {
            /* for vtx programs, this is VERTEX_RESULT_COLOR */
            GLint color_type;
            GLuint face_type = parse_face_type(inst);
	    GLint err = parse_color_type(ctx, inst, Program, &color_type);
            if (err)
               return 1;

            if (face_type) {
               /* back face */
               if (color_type) {
                  *outputReg = VERT_RESULT_BFC1; /* secondary color */
               }
               else {
                  *outputReg = VERT_RESULT_BFC0; /* primary color */
               }
            }
            else {
               /* front face */
               if (color_type) {
                  *outputReg = VERT_RESULT_COL1; /* secondary color */
               }
               /* primary color */
               else {
                  *outputReg = VERT_RESULT_COL0; /* primary color */
               }
            }
         }
         break;

      case VERTEX_RESULT_FOGCOORD:
         *outputReg = VERT_RESULT_FOGC;
         break;

      case VERTEX_RESULT_POINTSIZE:
         *outputReg = VERT_RESULT_PSIZ;
         break;

      case VERTEX_RESULT_TEXCOORD:
         {
            GLuint unit;
            if (parse_texcoord_num (ctx, inst, Program, &unit))
               return 1;
            *outputReg = VERT_RESULT_TEX0 + unit;
         }
         break;
   }

   Program->Base.OutputsWritten |= (1 << *outputReg);

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
parse_attrib (GLcontext * ctx, const GLubyte ** inst, struct var_cache **vc_head,
              struct arb_program *Program)
{
   GLuint found;
   struct var_cache *attrib_var;

   attrib_var = parse_string (inst, vc_head, Program, &found);
   Program->Position = parse_position (inst);
   if (found) {
      program_error2(ctx, Program->Position,
                     "Duplicate variable declaration",
                     (char *) attrib_var->name);
      return 1;
   }

   attrib_var->type = vt_attrib;

   if (parse_attrib_binding(ctx, inst, Program, &attrib_var->attrib_binding,
                            &attrib_var->attrib_is_generic))
      return 1;

   if (generic_attrib_check(*vc_head)) {
      program_error(ctx, Program->Position,
                    "Cannot use both a generic vertex attribute "
                    "and a specific attribute of the same type");
      return 1;
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
parse_param_elements (GLcontext * ctx, const GLubyte ** inst,
                      struct var_cache *param_var,
                      struct arb_program *Program, GLboolean use)
{
   GLint idx;
   GLuint err = 0;
   gl_state_index state_tokens[STATE_LENGTH] = {0, 0, 0, 0, 0};
   GLfloat const_values[4];

   GLubyte token = *(*inst)++;

   switch (token) {
      case PARAM_STATE_ELEMENT:
         if (parse_state_single_item (ctx, inst, Program, state_tokens))
            return 1;

         /* If we adding STATE_MATRIX that has multiple rows, we need to
          * unroll it and call _mesa_add_state_reference() for each row
          */
         if ((state_tokens[0] == STATE_MODELVIEW_MATRIX ||
              state_tokens[0] == STATE_PROJECTION_MATRIX ||
              state_tokens[0] == STATE_MVP_MATRIX ||
              state_tokens[0] == STATE_TEXTURE_MATRIX ||
              state_tokens[0] == STATE_PROGRAM_MATRIX)
             && (state_tokens[2] != state_tokens[3])) {
            GLint row;
            const GLint first_row = state_tokens[2];
            const GLint last_row = state_tokens[3];

            for (row = first_row; row <= last_row; row++) {
               state_tokens[2] = state_tokens[3] = row;

               idx = _mesa_add_state_reference(Program->Base.Parameters,
                                               state_tokens);
               if (param_var->param_binding_begin == ~0U)
                  param_var->param_binding_begin = idx;
               param_var->param_binding_length++;
               Program->Base.NumParameters++;
            }
         }
         else {
            idx = _mesa_add_state_reference(Program->Base.Parameters,
                                            state_tokens);
            if (param_var->param_binding_begin == ~0U)
               param_var->param_binding_begin = idx;
            param_var->param_binding_length++;
            Program->Base.NumParameters++;
         }
         break;

      case PARAM_PROGRAM_ELEMENT:
         if (parse_program_single_item (ctx, inst, Program, state_tokens))
            return 1;
         idx = _mesa_add_state_reference (Program->Base.Parameters, state_tokens);
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
                    && (end_idx >= ctx->Const.FragmentProgram.MaxEnvParams))
                   || ((state_tokens[1] == STATE_LOCAL)
                       && (end_idx >=
                           ctx->Const.FragmentProgram.MaxLocalParams)))
                  out_of_range = 1;
            }
            else {
               if (((state_tokens[1] == STATE_ENV)
                    && (end_idx >= ctx->Const.VertexProgram.MaxEnvParams))
                   || ((state_tokens[1] == STATE_LOCAL)
                       && (end_idx >=
                           ctx->Const.VertexProgram.MaxLocalParams)))
                  out_of_range = 1;
            }
            if (out_of_range) {
               program_error(ctx, Program->Position,
                             "Invalid Program Parameter"); /*end_idx*/
               return 1;
            }

            for (new_idx = start_idx; new_idx <= end_idx; new_idx++) {
               state_tokens[2] = new_idx;
               idx = _mesa_add_state_reference(Program->Base.Parameters,
                                               state_tokens);
               param_var->param_binding_length++;
               Program->Base.NumParameters++;
            }
         }
         else {
            (*inst)++;
         }
         break;

      case PARAM_CONSTANT:
         /* parsing something like {1.0, 2.0, 3.0, 4.0} */
         parse_constant (inst, const_values, Program, use);
         idx = _mesa_add_named_constant(Program->Base.Parameters,
                                        (char *) param_var->name,
                                        const_values, 4);
         if (param_var->param_binding_begin == ~0U)
            param_var->param_binding_begin = idx;
         param_var->param_binding_type = PROGRAM_CONSTANT;
         param_var->param_binding_length++;
         Program->Base.NumParameters++;
         break;

      default:
         program_error(ctx, Program->Position,
                       "Unexpected token (in parse_param_elements())");
         return 1;
   }

   /* Make sure we haven't blown past our parameter limits */
   if (((Program->Base.Target == GL_VERTEX_PROGRAM_ARB) &&
        (Program->Base.NumParameters >=
         ctx->Const.VertexProgram.MaxLocalParams))
       || ((Program->Base.Target == GL_FRAGMENT_PROGRAM_ARB)
           && (Program->Base.NumParameters >=
               ctx->Const.FragmentProgram.MaxLocalParams))) {
      program_error(ctx, Program->Position, "Too many parameter variables");
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
parse_param (GLcontext * ctx, const GLubyte ** inst, struct var_cache **vc_head,
             struct arb_program *Program)
{
   GLuint found, err;
   GLint specified_length;
   struct var_cache *param_var;

   err = 0;
   param_var = parse_string (inst, vc_head, Program, &found);
   Program->Position = parse_position (inst);

   if (found) {
      program_error2(ctx, Program->Position,
                     "Duplicate variable declaration",
                     (char *) param_var->name);
      return 1;
   }

   specified_length = parse_integer (inst, Program);

   if (specified_length < 0) {
      program_error(ctx, Program->Position, "Negative parameter array length");
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
         program_error(ctx, Program->Position,
              "Declared parameter array length does not match parameter list");
      }
   }

   (*inst)++;

   return 0;
}

/**
 *
 */
static GLuint
parse_param_use (GLcontext * ctx, const GLubyte ** inst, struct var_cache **vc_head,
                 struct arb_program *Program, struct var_cache **new_var)
{
   struct var_cache *param_var;

   /* First, insert a dummy entry into the var_cache */
   var_cache_create (&param_var);
   param_var->name = (const GLubyte *) " ";
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
parse_temp (GLcontext * ctx, const GLubyte ** inst, struct var_cache **vc_head,
            struct arb_program *Program)
{
   GLuint found;
   struct var_cache *temp_var;

   while (**inst != 0) {
      temp_var = parse_string (inst, vc_head, Program, &found);
      Program->Position = parse_position (inst);
      if (found) {
         program_error2(ctx, Program->Position,
                        "Duplicate variable declaration",
                        (char *) temp_var->name);
         return 1;
      }

      temp_var->type = vt_temp;

      if (((Program->Base.Target == GL_FRAGMENT_PROGRAM_ARB) &&
           (Program->Base.NumTemporaries >=
            ctx->Const.FragmentProgram.MaxTemps))
          || ((Program->Base.Target == GL_VERTEX_PROGRAM_ARB)
              && (Program->Base.NumTemporaries >=
                  ctx->Const.VertexProgram.MaxTemps))) {
         program_error(ctx, Program->Position,
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
parse_output (GLcontext * ctx, const GLubyte ** inst, struct var_cache **vc_head,
              struct arb_program *Program)
{
   GLuint found;
   struct var_cache *output_var;
   GLuint err;

   output_var = parse_string (inst, vc_head, Program, &found);
   Program->Position = parse_position (inst);
   if (found) {
      program_error2(ctx, Program->Position,
                     "Duplicate variable declaration",
                     (char *) output_var->name);
      return 1;
   }

   output_var->type = vt_output;

   err = parse_result_binding(ctx, inst, &output_var->output_binding, Program);
   return err;
}

/**
 * This handles variables of the ALIAS kind
 *
 * \return 0 on sucess, 1 on error
 */
static GLuint
parse_alias (GLcontext * ctx, const GLubyte ** inst, struct var_cache **vc_head,
             struct arb_program *Program)
{
   GLuint found;
   struct var_cache *temp_var;

   temp_var = parse_string (inst, vc_head, Program, &found);
   Program->Position = parse_position (inst);

   if (found) {
      program_error2(ctx, Program->Position,
                    "Duplicate variable declaration",
                     (char *) temp_var->name);
      return 1;
   }

   temp_var->type = vt_alias;
   temp_var->alias_binding =  parse_string (inst, vc_head, Program, &found);
   Program->Position = parse_position (inst);

   if (!found)
   {
      program_error2(ctx, Program->Position,
                     "Undefined alias value",
                     (char *) temp_var->alias_binding->name);
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
parse_address (GLcontext * ctx, const GLubyte ** inst, struct var_cache **vc_head,
               struct arb_program *Program)
{
   GLuint found;
   struct var_cache *temp_var;

   while (**inst != 0) {
      temp_var = parse_string (inst, vc_head, Program, &found);
      Program->Position = parse_position (inst);
      if (found) {
         program_error2(ctx, Program->Position,
                        "Duplicate variable declaration",
                        (char *) temp_var->name);
         return 1;
      }

      temp_var->type = vt_address;

      if (Program->Base.NumAddressRegs >=
          ctx->Const.VertexProgram.MaxAddressRegs) {
         const char *msg = "Too many ADDRESS variables declared";
         program_error(ctx, Program->Position, msg);
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
parse_declaration (GLcontext * ctx, const GLubyte ** inst, struct var_cache **vc_head,
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
 * Handle the parsing out of a masked destination register, either for a
 * vertex or fragment program.
 *
 * If we are a vertex program, make sure we don't write to
 * result.position if we have specified that the program is
 * position invariant
 *
 * \param File      - The register file we write to
 * \param Index     - The register index we write to
 * \param WriteMask - The mask controlling which components we write (1->write)
 *
 * \return 0 on sucess, 1 on error
 */
static GLuint
parse_masked_dst_reg (GLcontext * ctx, const GLubyte ** inst,
                      struct var_cache **vc_head, struct arb_program *Program,
                      enum register_file *File, GLuint *Index, GLint *WriteMask)
{
   GLuint tmp, result;
   struct var_cache *dst;

   /* We either have a result register specified, or a
    * variable that may or may not be writable
    */
   switch (*(*inst)++) {
      case REGISTER_RESULT:
         if (parse_result_binding(ctx, inst, Index, Program))
            return 1;
         *File = PROGRAM_OUTPUT;
         break;

      case REGISTER_ESTABLISHED_NAME:
         dst = parse_string (inst, vc_head, Program, &result);
         Program->Position = parse_position (inst);

         /* If the name has never been added to our symbol table, we're hosed */
         if (!result) {
            program_error(ctx, Program->Position, "0: Undefined variable");
            return 1;
         }

         switch (dst->type) {
            case vt_output:
               *File = PROGRAM_OUTPUT;
               *Index = dst->output_binding;
               break;

            case vt_temp:
               *File = PROGRAM_TEMPORARY;
               *Index = dst->temp_binding;
               break;

               /* If the var type is not vt_output or vt_temp, no go */
            default:
               program_error(ctx, Program->Position,
                             "Destination register is read only");
               return 1;
         }
         break;

      default:
         program_error(ctx, Program->Position,
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
      program_error(ctx, Program->Position,
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
parse_address_reg (GLcontext * ctx, const GLubyte ** inst,
                          struct var_cache **vc_head,
                          struct arb_program *Program, GLint * Index)
{
   struct var_cache *dst;
   GLuint result;

   *Index = 0; /* XXX */

   dst = parse_string (inst, vc_head, Program, &result);
   Program->Position = parse_position (inst);

   /* If the name has never been added to our symbol table, we're hosed */
   if (!result) {
      program_error(ctx, Program->Position, "Undefined variable");
      return 1;
   }

   if (dst->type != vt_address) {
      program_error(ctx, Program->Position, "Variable is not of type ADDRESS");
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
parse_masked_address_reg (GLcontext * ctx, const GLubyte ** inst,
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
 * Basically convert COMPONENT_X/Y/Z/W to SWIZZLE_X/Y/Z/W
 *
 * The len parameter allows us to grab 4 components for a vector
 * swizzle, or just 1 component for a scalar src register selection
 */
static void
parse_swizzle_mask(const GLubyte ** inst, GLubyte *swizzle, GLint len)
{
   GLint i;

   for (i = 0; i < 4; i++)
      swizzle[i] = i;

   for (i = 0; i < len; i++) {
      switch (*(*inst)++) {
         case COMPONENT_X:
            swizzle[i] = SWIZZLE_X;
            break;
         case COMPONENT_Y:
            swizzle[i] = SWIZZLE_Y;
            break;
         case COMPONENT_Z:
            swizzle[i] = SWIZZLE_Z;
            break;
         case COMPONENT_W:
            swizzle[i] = SWIZZLE_W;
            break;
         default:
            _mesa_problem(NULL, "bad component in parse_swizzle_mask()");
            return;
      }
   }
}


/**
 * Parse an extended swizzle mask which is a sequence of
 * four x/y/z/w/0/1 tokens.
 * \return swizzle  four swizzle values
 * \return negateMask  four element bitfield
 */
static void
parse_extended_swizzle_mask(const GLubyte **inst, GLubyte swizzle[4],
                            GLubyte *negateMask)
{
   GLint i;

   *negateMask = 0x0;
   for (i = 0; i < 4; i++) {
      GLubyte swz;
      if (parse_sign(inst) == -1)
         *negateMask |= (1 << i);

      swz = *(*inst)++;

      switch (swz) {
         case COMPONENT_0:
            swizzle[i] = SWIZZLE_ZERO;
            break;
         case COMPONENT_1:
            swizzle[i] = SWIZZLE_ONE;
            break;
         case COMPONENT_X:
            swizzle[i] = SWIZZLE_X;
            break;
         case COMPONENT_Y:
            swizzle[i] = SWIZZLE_Y;
            break;
         case COMPONENT_Z:
            swizzle[i] = SWIZZLE_Z;
            break;
         case COMPONENT_W:
            swizzle[i] = SWIZZLE_W;
            break;
         default:
            _mesa_problem(NULL, "bad case in parse_extended_swizzle_mask()");
            return;
      }
   }
}


static GLuint
parse_src_reg (GLcontext * ctx, const GLubyte ** inst,
               struct var_cache **vc_head,
               struct arb_program *Program,
               enum register_file * File, GLint * Index,
               GLboolean *IsRelOffset )
{
   struct var_cache *src;
   GLuint binding, is_generic, found;
   GLint offset;

   *IsRelOffset = 0;

   /* And the binding for the src */
   switch (*(*inst)++) {
      case REGISTER_ATTRIB:
         if (parse_attrib_binding
             (ctx, inst, Program, &binding, &is_generic))
            return 1;
         *File = PROGRAM_INPUT;
         *Index = binding;

         /* We need to insert a dummy variable into the var_cache so we can
          * catch generic vertex attrib aliasing errors
          */
         var_cache_create(&src);
         src->type = vt_attrib;
         src->name = (const GLubyte *) "Dummy Attrib Variable";
         src->attrib_binding = binding;
         src->attrib_is_generic = is_generic;
         var_cache_append(vc_head, src);
         if (generic_attrib_check(*vc_head)) {
            program_error(ctx, Program->Position,
                          "Cannot use both a generic vertex attribute "
                          "and a specific attribute of the same type");
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
                  program_error2(ctx, Program->Position,
                                 "Undefined variable",
                                 (char *) src->name);
                  return 1;
               }

               *File = (enum register_file) src->param_binding_type;

               switch (*(*inst)++) {
                  case ARRAY_INDEX_ABSOLUTE:
                     offset = parse_integer (inst, Program);

                     if ((offset < 0)
                         || (offset >= (int)src->param_binding_length)) {
                        program_error(ctx, Program->Position,
                                      "Index out of range");
                        /* offset, src->name */
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

               *File = (enum register_file) src->param_binding_type;
               *Index = src->param_binding_begin;
               break;
         }
         break;

      case REGISTER_ESTABLISHED_NAME:
         src = parse_string (inst, vc_head, Program, &found);
         Program->Position = parse_position (inst);

         /* If the name has never been added to our symbol table, we're hosed */
         if (!found) {
            program_error(ctx, Program->Position,
                          "3: Undefined variable"); /* src->name */
            return 1;
         }

         switch (src->type) {
            case vt_attrib:
               *File = PROGRAM_INPUT;
               *Index = src->attrib_binding;
               break;

               /* XXX: We have to handle offsets someplace in here!  -- or are those above? */
            case vt_param:
               *File = (enum register_file) src->param_binding_type;
               *Index = src->param_binding_begin;
               break;

            case vt_temp:
               *File = PROGRAM_TEMPORARY;
               *Index = src->temp_binding;
               break;

               /* If the var type is vt_output no go */
            default:
               program_error(ctx, Program->Position,
                             "destination register is read only");
               /* bad src->name */
               return 1;
         }
         break;

      default:
         program_error(ctx, Program->Position,
                       "Unknown token in parse_src_reg");
         return 1;
   }

   /* Add attributes to InputsRead only if they are used the program.
    * This avoids the handling of unused ATTRIB declarations in the drivers. */
   if (*File == PROGRAM_INPUT)
      Program->Base.InputsRead |= (1 << *Index);

   return 0;
}


/**
 * Parse vertex/fragment program vector source register.
 */
static GLuint
parse_vector_src_reg(GLcontext *ctx, const GLubyte **inst,
                     struct var_cache **vc_head,
                     struct arb_program *program,
                     struct prog_src_register *reg)
{
   enum register_file file;
   GLint index;
   GLubyte negateMask;
   GLubyte swizzle[4];
   GLboolean isRelOffset;

   /* Grab the sign */
   negateMask = (parse_sign (inst) == -1) ? NEGATE_XYZW : NEGATE_NONE;

   /* And the src reg */
   if (parse_src_reg(ctx, inst, vc_head, program, &file, &index, &isRelOffset))
      return 1;

   /* finally, the swizzle */
   parse_swizzle_mask(inst, swizzle, 4);

   reg->File = file;
   reg->Index = index;
   reg->Swizzle = MAKE_SWIZZLE4(swizzle[0], swizzle[1], swizzle[2], swizzle[3]);
   reg->NegateBase = negateMask;
   reg->RelAddr = isRelOffset;
   return 0;
}


/**
 * Parse vertex/fragment program scalar source register.
 */
static GLuint
parse_scalar_src_reg(GLcontext *ctx, const GLubyte **inst,
                     struct var_cache **vc_head,
                     struct arb_program *program,
                     struct prog_src_register *reg)
{
   enum register_file file;
   GLint index;
   GLubyte negateMask;
   GLubyte swizzle[4];
   GLboolean isRelOffset;

   /* Grab the sign */
   negateMask = (parse_sign (inst) == -1) ? NEGATE_XYZW : NEGATE_NONE;

   /* And the src reg */
   if (parse_src_reg(ctx, inst, vc_head, program, &file, &index, &isRelOffset))
      return 1;

   /* finally, the swizzle */
   parse_swizzle_mask(inst, swizzle, 1);

   reg->File = file;
   reg->Index = index;
   reg->Swizzle = (swizzle[0] << 0);
   reg->NegateBase = negateMask;
   reg->RelAddr = isRelOffset;
   return 0;
}


/**
 * Parse vertex/fragment program destination register.
 * \return 1 if error, 0 if no error.
 */
static GLuint 
parse_dst_reg(GLcontext * ctx, const GLubyte ** inst,
              struct var_cache **vc_head, struct arb_program *program,
              struct prog_dst_register *reg )
{
   GLint mask;
   GLuint idx;
   enum register_file file;

   if (parse_masked_dst_reg (ctx, inst, vc_head, program, &file, &idx, &mask))
      return 1;

   reg->File = file;
   reg->Index = idx;
   reg->WriteMask = mask;
   return 0;
}


/**
 * This is a big mother that handles getting opcodes into the instruction
 * and handling the src & dst registers for fragment program instructions
 * \return 1 if error, 0 if no error
 */
static GLuint
parse_fp_instruction (GLcontext * ctx, const GLubyte ** inst,
                      struct var_cache **vc_head, struct arb_program *Program,
                      struct prog_instruction *fp)
{
   GLint a;
   GLuint texcoord;
   GLubyte instClass, type, code;
   GLboolean rel;
   GLuint shadow_tex = 0;

   _mesa_init_instructions(fp, 1);

   /* Record the position in the program string for debugging */
   fp->StringPos = Program->Position;

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

   switch (type) {
      case OP_ALU_VECTOR:
         switch (code) {
            case OP_ABS_SAT:
               fp->SaturateMode = SATURATE_ZERO_ONE;
            case OP_ABS:
               fp->Opcode = OPCODE_ABS;
               break;

            case OP_FLR_SAT:
               fp->SaturateMode = SATURATE_ZERO_ONE;
            case OP_FLR:
               fp->Opcode = OPCODE_FLR;
               break;

            case OP_FRC_SAT:
               fp->SaturateMode = SATURATE_ZERO_ONE;
            case OP_FRC:
               fp->Opcode = OPCODE_FRC;
               break;

            case OP_LIT_SAT:
               fp->SaturateMode = SATURATE_ZERO_ONE;
            case OP_LIT:
               fp->Opcode = OPCODE_LIT;
               break;

            case OP_MOV_SAT:
               fp->SaturateMode = SATURATE_ZERO_ONE;
            case OP_MOV:
               fp->Opcode = OPCODE_MOV;
               break;
         }

         if (parse_dst_reg (ctx, inst, vc_head, Program, &fp->DstReg))
            return 1;

         if (parse_vector_src_reg(ctx, inst, vc_head, Program, &fp->SrcReg[0]))
            return 1;
         break;

      case OP_ALU_SCALAR:
         switch (code) {
            case OP_COS_SAT:
               fp->SaturateMode = SATURATE_ZERO_ONE;
            case OP_COS:
               fp->Opcode = OPCODE_COS;
               break;

            case OP_EX2_SAT:
               fp->SaturateMode = SATURATE_ZERO_ONE;
            case OP_EX2:
               fp->Opcode = OPCODE_EX2;
               break;

            case OP_LG2_SAT:
               fp->SaturateMode = SATURATE_ZERO_ONE;
            case OP_LG2:
               fp->Opcode = OPCODE_LG2;
               break;

            case OP_RCP_SAT:
               fp->SaturateMode = SATURATE_ZERO_ONE;
            case OP_RCP:
               fp->Opcode = OPCODE_RCP;
               break;

            case OP_RSQ_SAT:
               fp->SaturateMode = SATURATE_ZERO_ONE;
            case OP_RSQ:
               fp->Opcode = OPCODE_RSQ;
               break;

            case OP_SIN_SAT:
               fp->SaturateMode = SATURATE_ZERO_ONE;
            case OP_SIN:
               fp->Opcode = OPCODE_SIN;
               break;

            case OP_SCS_SAT:
               fp->SaturateMode = SATURATE_ZERO_ONE;
            case OP_SCS:

               fp->Opcode = OPCODE_SCS;
               break;
         }

         if (parse_dst_reg (ctx, inst, vc_head, Program, &fp->DstReg))
            return 1;

         if (parse_scalar_src_reg(ctx, inst, vc_head, Program, &fp->SrcReg[0]))
            return 1;
         break;

      case OP_ALU_BINSC:
         switch (code) {
            case OP_POW_SAT:
               fp->SaturateMode = SATURATE_ZERO_ONE;
            case OP_POW:
               fp->Opcode = OPCODE_POW;
               break;
         }

         if (parse_dst_reg(ctx, inst, vc_head, Program, &fp->DstReg))
            return 1;

         for (a = 0; a < 2; a++) {
	    if (parse_scalar_src_reg(ctx, inst, vc_head, Program, &fp->SrcReg[a]))
               return 1;
         }
         break;


      case OP_ALU_BIN:
         switch (code) {
            case OP_ADD_SAT:
               fp->SaturateMode = SATURATE_ZERO_ONE;
            case OP_ADD:
               fp->Opcode = OPCODE_ADD;
               break;

            case OP_DP3_SAT:
               fp->SaturateMode = SATURATE_ZERO_ONE;
            case OP_DP3:
               fp->Opcode = OPCODE_DP3;
               break;

            case OP_DP4_SAT:
               fp->SaturateMode = SATURATE_ZERO_ONE;
            case OP_DP4:
               fp->Opcode = OPCODE_DP4;
               break;

            case OP_DPH_SAT:
               fp->SaturateMode = SATURATE_ZERO_ONE;
            case OP_DPH:
               fp->Opcode = OPCODE_DPH;
               break;

            case OP_DST_SAT:
               fp->SaturateMode = SATURATE_ZERO_ONE;
            case OP_DST:
               fp->Opcode = OPCODE_DST;
               break;

            case OP_MAX_SAT:
               fp->SaturateMode = SATURATE_ZERO_ONE;
            case OP_MAX:
               fp->Opcode = OPCODE_MAX;
               break;

            case OP_MIN_SAT:
               fp->SaturateMode = SATURATE_ZERO_ONE;
            case OP_MIN:
               fp->Opcode = OPCODE_MIN;
               break;

            case OP_MUL_SAT:
               fp->SaturateMode = SATURATE_ZERO_ONE;
            case OP_MUL:
               fp->Opcode = OPCODE_MUL;
               break;

            case OP_SGE_SAT:
               fp->SaturateMode = SATURATE_ZERO_ONE;
            case OP_SGE:
               fp->Opcode = OPCODE_SGE;
               break;

            case OP_SLT_SAT:
               fp->SaturateMode = SATURATE_ZERO_ONE;
            case OP_SLT:
               fp->Opcode = OPCODE_SLT;
               break;

            case OP_SUB_SAT:
               fp->SaturateMode = SATURATE_ZERO_ONE;
            case OP_SUB:
               fp->Opcode = OPCODE_SUB;
               break;

            case OP_XPD_SAT:
               fp->SaturateMode = SATURATE_ZERO_ONE;
            case OP_XPD:
               fp->Opcode = OPCODE_XPD;
               break;
         }

         if (parse_dst_reg (ctx, inst, vc_head, Program, &fp->DstReg))
            return 1;
         for (a = 0; a < 2; a++) {
	    if (parse_vector_src_reg(ctx, inst, vc_head, Program, &fp->SrcReg[a]))
	       return 1;
         }
         break;

      case OP_ALU_TRI:
         switch (code) {
            case OP_CMP_SAT:
               fp->SaturateMode = SATURATE_ZERO_ONE;
            case OP_CMP:
               fp->Opcode = OPCODE_CMP;
               break;

            case OP_LRP_SAT:
               fp->SaturateMode = SATURATE_ZERO_ONE;
            case OP_LRP:
               fp->Opcode = OPCODE_LRP;
               break;

            case OP_MAD_SAT:
               fp->SaturateMode = SATURATE_ZERO_ONE;
            case OP_MAD:
               fp->Opcode = OPCODE_MAD;
               break;
         }

         if (parse_dst_reg (ctx, inst, vc_head, Program, &fp->DstReg))
            return 1;

         for (a = 0; a < 3; a++) {
	    if (parse_vector_src_reg(ctx, inst, vc_head, Program, &fp->SrcReg[a]))
	       return 1;
         }
         break;

      case OP_ALU_SWZ:
         switch (code) {
            case OP_SWZ_SAT:
               fp->SaturateMode = SATURATE_ZERO_ONE;
            case OP_SWZ:
               fp->Opcode = OPCODE_SWZ;
               break;
         }
         if (parse_dst_reg (ctx, inst, vc_head, Program, &fp->DstReg))
            return 1;

	 {
	    GLubyte swizzle[4];
	    GLubyte negateMask;
            enum register_file file;
	    GLint index;

	    if (parse_src_reg(ctx, inst, vc_head, Program, &file, &index, &rel))
	       return 1;
	    parse_extended_swizzle_mask(inst, swizzle, &negateMask);
	    fp->SrcReg[0].File = file;
	    fp->SrcReg[0].Index = index;
	    fp->SrcReg[0].NegateBase = negateMask;
	    fp->SrcReg[0].Swizzle = MAKE_SWIZZLE4(swizzle[0],
                                                  swizzle[1],
                                                  swizzle[2],
                                                  swizzle[3]);
	 }
         break;

      case OP_TEX_SAMPLE:
         switch (code) {
            case OP_TEX_SAT:
               fp->SaturateMode = SATURATE_ZERO_ONE;
            case OP_TEX:
               fp->Opcode = OPCODE_TEX;
               break;

            case OP_TXP_SAT:
               fp->SaturateMode = SATURATE_ZERO_ONE;
            case OP_TXP:
               fp->Opcode = OPCODE_TXP;
               break;

            case OP_TXB_SAT:
               fp->SaturateMode = SATURATE_ZERO_ONE;
            case OP_TXB:
               fp->Opcode = OPCODE_TXB;
               break;
         }

         if (parse_dst_reg (ctx, inst, vc_head, Program, &fp->DstReg))
            return 1;

	 if (parse_vector_src_reg(ctx, inst, vc_head, Program, &fp->SrcReg[0]))
            return 1;

         /* texImageUnit */
         if (parse_teximage_num (ctx, inst, Program, &texcoord))
            return 1;
         fp->TexSrcUnit = texcoord;

         /* texTarget */
         switch (*(*inst)++) {
            case TEXTARGET_SHADOW1D:
               shadow_tex = 1 << texcoord;
               /* FALLTHROUGH */
            case TEXTARGET_1D:
               fp->TexSrcTarget = TEXTURE_1D_INDEX;
               break;
            case TEXTARGET_SHADOW2D:
               shadow_tex = 1 << texcoord;
               /* FALLTHROUGH */
            case TEXTARGET_2D:
               fp->TexSrcTarget = TEXTURE_2D_INDEX;
               break;
            case TEXTARGET_3D:
               fp->TexSrcTarget = TEXTURE_3D_INDEX;
               break;
            case TEXTARGET_SHADOWRECT:
               shadow_tex = 1 << texcoord;
               /* FALLTHROUGH */
            case TEXTARGET_RECT:
               fp->TexSrcTarget = TEXTURE_RECT_INDEX;
               break;
            case TEXTARGET_CUBE:
               fp->TexSrcTarget = TEXTURE_CUBE_INDEX;
               break;
            case TEXTARGET_SHADOW1D_ARRAY:
               shadow_tex = 1 << texcoord;
               /* FALLTHROUGH */
            case TEXTARGET_1D_ARRAY:
               fp->TexSrcTarget = TEXTURE_1D_ARRAY_INDEX;
               break;
            case TEXTARGET_SHADOW2D_ARRAY:
               shadow_tex = 1 << texcoord;
               /* FALLTHROUGH */
            case TEXTARGET_2D_ARRAY:
               fp->TexSrcTarget = TEXTURE_2D_ARRAY_INDEX;
               break;
         }

         /* Don't test the first time a particular sampler is seen.  Each time
          * after that, make sure the shadow state is the same.
          */
         if ((_mesa_bitcount(Program->TexturesUsed[texcoord]) > 0)
             && ((Program->ShadowSamplers & (1 << texcoord)) != shadow_tex)) {
            program_error(ctx, Program->Position,
                          "texture image unit used for shadow sampling and non-shadow sampling");
            return 1;
         }

         Program->TexturesUsed[texcoord] |= (1 << fp->TexSrcTarget);
         /* Check that both "2D" and "CUBE" (for example) aren't both used */
         if (_mesa_bitcount(Program->TexturesUsed[texcoord]) > 1) {
            program_error(ctx, Program->Position,
                          "multiple targets used on one texture image unit");
            return 1;
         }
      

         Program->ShadowSamplers |= shadow_tex;
         break;

      case OP_TEX_KIL:
         Program->UsesKill = 1;
	 if (parse_vector_src_reg(ctx, inst, vc_head, Program, &fp->SrcReg[0]))
            return 1;
         fp->Opcode = OPCODE_KIL;
         break;
      default:
         _mesa_problem(ctx, "bad type 0x%x in parse_fp_instruction()", type);
         return 1;
   }

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
parse_vp_address_reg (GLcontext * ctx, const GLubyte ** inst,
		      struct var_cache **vc_head,
		      struct arb_program *Program,
		      struct prog_dst_register *reg)
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
 * This is a big mother that handles getting opcodes into the instruction
 * and handling the src & dst registers for vertex program instructions
 */
static GLuint
parse_vp_instruction (GLcontext * ctx, const GLubyte ** inst,
                      struct var_cache **vc_head, struct arb_program *Program,
                      struct prog_instruction *vp)
{
   GLint a;
   GLubyte type, code;

   /* OP_ALU_{ARL, VECTOR, SCALAR, BINSC, BIN, TRI, SWZ} */
   type = *(*inst)++;

   /* The actual opcode name */
   code = *(*inst)++;

   _mesa_init_instructions(vp, 1);
   /* Record the position in the program string for debugging */
   vp->StringPos = Program->Position;

   switch (type) {
         /* XXX: */
      case OP_ALU_ARL:
         vp->Opcode = OPCODE_ARL;

         /* Remember to set SrcReg.RelAddr; */

         /* Get the masked address register [dst] */
         if (parse_vp_address_reg(ctx, inst, vc_head, Program, &vp->DstReg))
            return 1;

         vp->DstReg.File = PROGRAM_ADDRESS;

         /* Get a scalar src register */
	 if (parse_scalar_src_reg(ctx, inst, vc_head, Program, &vp->SrcReg[0]))
            return 1;

         break;

      case OP_ALU_VECTOR:
         switch (code) {
            case OP_ABS:
               vp->Opcode = OPCODE_ABS;
               break;
            case OP_FLR:
               vp->Opcode = OPCODE_FLR;
               break;
            case OP_FRC:
               vp->Opcode = OPCODE_FRC;
               break;
            case OP_LIT:
               vp->Opcode = OPCODE_LIT;
               break;
            case OP_MOV:
               vp->Opcode = OPCODE_MOV;
               break;
         }

         if (parse_dst_reg(ctx, inst, vc_head, Program, &vp->DstReg))
            return 1;

         if (parse_vector_src_reg(ctx, inst, vc_head, Program, &vp->SrcReg[0]))
            return 1;
         break;

      case OP_ALU_SCALAR:
         switch (code) {
            case OP_EX2:
               vp->Opcode = OPCODE_EX2;
               break;
            case OP_EXP:
               vp->Opcode = OPCODE_EXP;
               break;
            case OP_LG2:
               vp->Opcode = OPCODE_LG2;
               break;
            case OP_LOG:
               vp->Opcode = OPCODE_LOG;
               break;
            case OP_RCP:
               vp->Opcode = OPCODE_RCP;
               break;
            case OP_RSQ:
               vp->Opcode = OPCODE_RSQ;
               break;
         }
         if (parse_dst_reg(ctx, inst, vc_head, Program, &vp->DstReg))
            return 1;

	 if (parse_scalar_src_reg(ctx, inst, vc_head, Program, &vp->SrcReg[0]))
            return 1;
         break;

      case OP_ALU_BINSC:
         switch (code) {
            case OP_POW:
               vp->Opcode = OPCODE_POW;
               break;
         }
         if (parse_dst_reg(ctx, inst, vc_head, Program, &vp->DstReg))
            return 1;

         for (a = 0; a < 2; a++) {
	    if (parse_scalar_src_reg(ctx, inst, vc_head, Program, &vp->SrcReg[a]))
               return 1;
         }
         break;

      case OP_ALU_BIN:
         switch (code) {
            case OP_ADD:
               vp->Opcode = OPCODE_ADD;
               break;
            case OP_DP3:
               vp->Opcode = OPCODE_DP3;
               break;
            case OP_DP4:
               vp->Opcode = OPCODE_DP4;
               break;
            case OP_DPH:
               vp->Opcode = OPCODE_DPH;
               break;
            case OP_DST:
               vp->Opcode = OPCODE_DST;
               break;
            case OP_MAX:
               vp->Opcode = OPCODE_MAX;
               break;
            case OP_MIN:
               vp->Opcode = OPCODE_MIN;
               break;
            case OP_MUL:
               vp->Opcode = OPCODE_MUL;
               break;
            case OP_SGE:
               vp->Opcode = OPCODE_SGE;
               break;
            case OP_SLT:
               vp->Opcode = OPCODE_SLT;
               break;
            case OP_SUB:
               vp->Opcode = OPCODE_SUB;
               break;
            case OP_XPD:
               vp->Opcode = OPCODE_XPD;
               break;
         }
         if (parse_dst_reg(ctx, inst, vc_head, Program, &vp->DstReg))
            return 1;

         for (a = 0; a < 2; a++) {
	    if (parse_vector_src_reg(ctx, inst, vc_head, Program, &vp->SrcReg[a]))
               return 1;
         }
         break;

      case OP_ALU_TRI:
         switch (code) {
            case OP_MAD:
               vp->Opcode = OPCODE_MAD;
               break;
         }

         if (parse_dst_reg(ctx, inst, vc_head, Program, &vp->DstReg))
            return 1;

         for (a = 0; a < 3; a++) {
	    if (parse_vector_src_reg(ctx, inst, vc_head, Program, &vp->SrcReg[a]))
               return 1;
         }
         break;

      case OP_ALU_SWZ:
         switch (code) {
            case OP_SWZ:
               vp->Opcode = OPCODE_SWZ;
               break;
         }
	 {
	    GLubyte swizzle[4]; 
	    GLubyte negateMask;
	    GLboolean relAddr;
            enum register_file file;
	    GLint index;

	    if (parse_dst_reg(ctx, inst, vc_head, Program, &vp->DstReg))
	       return 1;

	    if (parse_src_reg(ctx, inst, vc_head, Program, &file, &index, &relAddr))
	       return 1;
	    parse_extended_swizzle_mask (inst, swizzle, &negateMask);
	    vp->SrcReg[0].File = file;
	    vp->SrcReg[0].Index = index;
	    vp->SrcReg[0].NegateBase = negateMask;
	    vp->SrcReg[0].Swizzle = MAKE_SWIZZLE4(swizzle[0],
                                                  swizzle[1],
                                                  swizzle[2],
                                                  swizzle[3]);
	    vp->SrcReg[0].RelAddr = relAddr;
	 }
         break;
   }
   return 0;
}

#if DEBUG_PARSING

static GLvoid
debug_variables (GLcontext * ctx, struct var_cache *vc_head,
                 struct arb_program *Program)
{
   struct var_cache *vc;
   GLint a, b;

   fprintf (stderr, "debug_variables, vc_head: %p\n", (void*) vc_head);

   /* First of all, print out the contents of the var_cache */
   vc = vc_head;
   while (vc) {
      fprintf (stderr, "[%p]\n", (void*) vc);
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
                        Program->Base.Parameters->Parameters[a + b].Name);
               if (Program->Base.Parameters->Parameters[a + b].Type == PROGRAM_STATE_VAR) {
                  const char *s;
                  s = _mesa_program_state_string(Program->Base.Parameters->Parameters
                                                 [a + b].StateIndexes);
                  fprintf(stderr, "%s\n", s);
                  _mesa_free((char *) s);
               }
               else
                  fprintf (stderr, "%f %f %f %f\n",
                           Program->Base.Parameters->ParameterValues[a + b][0],
                           Program->Base.Parameters->ParameterValues[a + b][1],
                           Program->Base.Parameters->ParameterValues[a + b][2],
                           Program->Base.Parameters->ParameterValues[a + b][3]);
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
            fprintf (stderr, "          binding: 0x%p (%s)\n",
                     (void*) vc->alias_binding, vc->alias_binding->name);
            break;
         default:
            /* nothing */
            ;
      }
      vc = vc->next;
   }
}

#endif /* DEBUG_PARSING */


/**
 * The main loop for parsing a fragment or vertex program
 *
 * \return 1 on error, 0 on success
 */
static GLint
parse_instructions(GLcontext * ctx, const GLubyte * inst,
                   struct var_cache **vc_head, struct arb_program *Program)
{
   const GLuint maxInst = (Program->Base.Target == GL_FRAGMENT_PROGRAM_ARB)
      ? ctx->Const.FragmentProgram.MaxInstructions
      : ctx->Const.VertexProgram.MaxInstructions;
   GLint err = 0;

   ASSERT(MAX_INSTRUCTIONS >= maxInst);

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
                     Program->HintPositionInvariant = GL_TRUE;
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

               case MESA_TEXTURE_ARRAY:
		  /* do nothing for now */
                  break;
            }
            break;

         case INSTRUCTION:
            /* check length */
            if (Program->Base.NumInstructions + 1 >= maxInst) {
               program_error(ctx, Program->Position,
                             "Max instruction count exceeded");
               return 1;
            }
            Program->Position = parse_position (&inst);
            /* parse the current instruction */
            if (Program->Base.Target == GL_FRAGMENT_PROGRAM_ARB) {
               err = parse_fp_instruction (ctx, &inst, vc_head, Program,
                      &Program->Base.Instructions[Program->Base.NumInstructions]);
            }
            else {
               err = parse_vp_instruction (ctx, &inst, vc_head, Program,
                      &Program->Base.Instructions[Program->Base.NumInstructions]);
            }

            /* increment instuction count */
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
   {
      const GLuint numInst = Program->Base.NumInstructions;
      _mesa_init_instructions(Program->Base.Instructions + numInst, 1);
      Program->Base.Instructions[numInst].Opcode = OPCODE_END;
      /* YYY Wrong Position in program, whatever, at least not random -> crash
	 Program->Position = parse_position (&inst);
      */
      Program->Base.Instructions[numInst].StringPos = Program->Position;
   }
   Program->Base.NumInstructions++;

   /*
    * Initialize native counts to logical counts.  The device driver may
    * change them if program is translated into a hardware program.
    */
   Program->Base.NumNativeInstructions = Program->Base.NumInstructions;
   Program->Base.NumNativeTemporaries = Program->Base.NumTemporaries;
   Program->Base.NumNativeParameters = Program->Base.NumParameters;
   Program->Base.NumNativeAttributes = Program->Base.NumAttributes;
   Program->Base.NumNativeAddressRegs = Program->Base.NumAddressRegs;

   return err;
}


/* XXX temporary */
LONGSTRING static char core_grammar_text[] =
#include "shader/grammar/grammar_syn.h"
;


/**
 * Set a grammar parameter.
 * \param name the grammar parameter
 * \param value the new parameter value
 * \return 0 if OK, 1 if error
 */
static int
set_reg8 (GLcontext *ctx, grammar id, const char *name, GLubyte value)
{
   char error_msg[300];
   GLint error_pos;

   if (grammar_set_reg8 (id, (const byte *) name, value))
      return 0;

   grammar_get_last_error ((byte *) error_msg, 300, &error_pos);
   _mesa_set_program_error (ctx, error_pos, error_msg);
   _mesa_error (ctx, GL_INVALID_OPERATION, "Grammar Register Error");
   return 1;
}


/**
 * Enable support for the given language option in the parser.
 * \return 1 if OK, 0 if error
 */
static int
enable_ext(GLcontext *ctx, grammar id, const char *name)
{
   return !set_reg8(ctx, id, name, 1);
}


/**
 * Enable parser extensions based on which OpenGL extensions are supported
 * by this rendering context.
 *
 * \return GL_TRUE if OK, GL_FALSE if error.
 */
static GLboolean
enable_parser_extensions(GLcontext *ctx, grammar id)
{
#if 0
   /* These are not supported at this time */
   if ((ctx->Extensions.ARB_vertex_blend ||
        ctx->Extensions.EXT_vertex_weighting)
       && !enable_ext(ctx, id, "vertex_blend"))
      return GL_FALSE;
   if (ctx->Extensions.ARB_matrix_palette
       && !enable_ext(ctx, id, "matrix_palette"))
      return GL_FALSE;
#endif
   if (ctx->Extensions.ARB_fragment_program_shadow
       && !enable_ext(ctx, id, "fragment_program_shadow"))
      return GL_FALSE;
   if (ctx->Extensions.EXT_point_parameters
       && !enable_ext(ctx, id, "point_parameters"))
      return GL_FALSE;
   if (ctx->Extensions.EXT_secondary_color
       && !enable_ext(ctx, id, "secondary_color"))
      return GL_FALSE;
   if (ctx->Extensions.EXT_fog_coord
       && !enable_ext(ctx, id, "fog_coord"))
      return GL_FALSE;
   if (ctx->Extensions.NV_texture_rectangle
       && !enable_ext(ctx, id, "texture_rectangle"))
      return GL_FALSE;
   if (ctx->Extensions.ARB_draw_buffers
       && !enable_ext(ctx, id, "draw_buffers"))
      return GL_FALSE;
   if (ctx->Extensions.MESA_texture_array
       && !enable_ext(ctx, id, "texture_array"))
      return GL_FALSE;
#if 1
   /* hack for Warcraft (see bug 8060) */
   enable_ext(ctx, id, "vertex_blend");
#endif

   return GL_TRUE;
}


/**
 * This kicks everything off.
 *
 * \param ctx - The GL Context
 * \param str - The program string
 * \param len - The program string length
 * \param program - The arb_program struct to return all the parsed info in
 * \return GL_TRUE on sucess, GL_FALSE on error
 */
static GLboolean
_mesa_parse_arb_program(GLcontext *ctx, GLenum target,
                        const GLubyte *str, GLsizei len,
                        struct arb_program *program)
{
   GLint a, err, error_pos;
   char error_msg[300];
   GLuint parsed_len;
   struct var_cache *vc_head;
   grammar arbprogram_syn_id;
   GLubyte *parsed, *inst;
   GLubyte *strz = NULL;
   static int arbprogram_syn_is_ok = 0;		/* XXX temporary */

   /* set the program target before parsing */
   program->Base.Target = target;

   /* Reset error state */
   _mesa_set_program_error(ctx, -1, NULL);

   /* check if arb_grammar_text (arbprogram.syn) is syntactically correct */
   if (!arbprogram_syn_is_ok) {
      /* One-time initialization of parsing system */
      grammar grammar_syn_id;
      GLuint parsed_len;

      grammar_syn_id = grammar_load_from_text ((byte *) core_grammar_text);
      if (grammar_syn_id == 0) {
         grammar_get_last_error ((byte *) error_msg, 300, &error_pos);
         /* XXX this is not a GL error - it's an implementation bug! - FIX */
         _mesa_set_program_error (ctx, error_pos, error_msg);
         _mesa_error (ctx, GL_INVALID_OPERATION,
                      "glProgramStringARB(Error loading grammar rule set)");
         return GL_FALSE;
      }

      err = !grammar_check(grammar_syn_id, (byte *) arb_grammar_text,
                           &parsed, &parsed_len);

      /* 'parsed' is unused here */
      _mesa_free (parsed);
      parsed = NULL;

      /* NOTE: we can't destroy grammar_syn_id right here because
       * grammar_destroy() can reset the last error
       */
      if (err) {
         /* XXX this is not a GL error - it's an implementation bug! - FIX */
         grammar_get_last_error ((byte *) error_msg, 300, &error_pos);
         _mesa_set_program_error (ctx, error_pos, error_msg);
         _mesa_error (ctx, GL_INVALID_OPERATION,
                      "glProgramString(Error loading grammar rule set");
         grammar_destroy (grammar_syn_id);
         return GL_FALSE;
      }

      grammar_destroy (grammar_syn_id);

      arbprogram_syn_is_ok = 1;
   }

   /* create the grammar object */
   arbprogram_syn_id = grammar_load_from_text ((byte *) arb_grammar_text);
   if (arbprogram_syn_id == 0) {
      /* XXX this is not a GL error - it's an implementation bug! - FIX */
      grammar_get_last_error ((GLubyte *) error_msg, 300, &error_pos);
      _mesa_set_program_error (ctx, error_pos, error_msg);
      _mesa_error (ctx, GL_INVALID_OPERATION,
                   "glProgramString(Error loading grammer rule set)");
      return GL_FALSE;
   }

   /* Set program_target register value */
   if (set_reg8 (ctx, arbprogram_syn_id, "program_target",
      program->Base.Target == GL_FRAGMENT_PROGRAM_ARB ? 0x10 : 0x20)) {
      grammar_destroy (arbprogram_syn_id);
      return GL_FALSE;
   }

   if (!enable_parser_extensions(ctx, arbprogram_syn_id)) {
      grammar_destroy(arbprogram_syn_id);
      return GL_FALSE;
   }

   /* check for NULL character occurences */
   {
      GLint i;
      for (i = 0; i < len; i++) {
         if (str[i] == '\0') {
            program_error(ctx, i, "illegal character");
            grammar_destroy (arbprogram_syn_id);
            return GL_FALSE;
         }
      }
   }

   /* copy the program string to a null-terminated string */
   strz = (GLubyte *) _mesa_malloc (len + 1);
   if (!strz) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glProgramStringARB");
      grammar_destroy (arbprogram_syn_id);
      return GL_FALSE;
   }
   _mesa_memcpy (strz, str, len);
   strz[len] = '\0';

   /* do a fast check on program string - initial production buffer is 4K */
   err = !grammar_fast_check(arbprogram_syn_id, strz,
                             &parsed, &parsed_len, 0x1000);

   /* Syntax parse error */
   if (err) {
      grammar_get_last_error((GLubyte *) error_msg, 300, &error_pos);
      program_error(ctx, error_pos, error_msg);

#if DEBUG_PARSING
      /* useful for debugging */
      do {
         int line, col;
         char *s;
         fprintf(stderr, "program: %s\n", (char *) strz);
         fprintf(stderr, "Error Pos: %d\n", ctx->Program.ErrorPos);
         s = (char *) _mesa_find_line_column(strz, strz+ctx->Program.ErrorPos,
                                             &line, &col);
         fprintf(stderr, "line %d col %d: %s\n", line, col, s);
      } while (0);
#endif

      _mesa_free(strz);
      _mesa_free(parsed);

      grammar_destroy (arbprogram_syn_id);
      return GL_FALSE;
   }

   grammar_destroy (arbprogram_syn_id);

   /*
    * Program string is syntactically correct at this point
    * Parse the tokenized version of the program now, generating
    * vertex/fragment program instructions.
    */

   /* Initialize the arb_program struct */
   program->Base.String = strz;
   program->Base.Instructions = _mesa_alloc_instructions(MAX_INSTRUCTIONS);
   program->Base.NumInstructions =
   program->Base.NumTemporaries =
   program->Base.NumParameters =
   program->Base.NumAttributes = program->Base.NumAddressRegs = 0;
   program->Base.Parameters = _mesa_new_parameter_list ();
   program->Base.InputsRead = 0x0;
   program->Base.OutputsWritten = 0x0;
   program->Position = 0;
   program->MajorVersion = program->MinorVersion = 0;
   program->PrecisionOption = GL_DONT_CARE;
   program->FogOption = GL_NONE;
   program->HintPositionInvariant = GL_FALSE;
   for (a = 0; a < MAX_TEXTURE_IMAGE_UNITS; a++)
      program->TexturesUsed[a] = 0x0;
   program->ShadowSamplers = 0x0;
   program->NumAluInstructions =
   program->NumTexInstructions =
   program->NumTexIndirections = 0;
   program->UsesKill = 0;

   vc_head = NULL;
   err = GL_FALSE;

   /* Start examining the tokens in the array */
   inst = parsed;

   /* Check the grammer rev */
   if (*inst++ != REVISION) {
      program_error (ctx, 0, "Grammar version mismatch");
      err = GL_TRUE;
   }
   else {
      /* ignore program target */
      inst++;
      err = parse_instructions(ctx, inst, &vc_head, program);
   }

   /*debug_variables(ctx, vc_head, program); */

   /* We're done with the parsed binary array */
   var_cache_destroy (&vc_head);

   _mesa_free (parsed);

   /* Reallocate the instruction array from size [MAX_INSTRUCTIONS]
    * to size [ap.Base.NumInstructions].
    */
   program->Base.Instructions
      = _mesa_realloc_instructions(program->Base.Instructions,
                                   MAX_INSTRUCTIONS,
                                   program->Base.NumInstructions);

   return !err;
}



void
_mesa_parse_arb_fragment_program(GLcontext* ctx, GLenum target,
                                 const GLvoid *str, GLsizei len,
                                 struct gl_fragment_program *program)
{
   struct arb_program ap;
   GLuint i;

   ASSERT(target == GL_FRAGMENT_PROGRAM_ARB);
   if (!_mesa_parse_arb_program(ctx, target, (const GLubyte*) str, len, &ap)) {
      /* Error in the program. Just return. */
      return;
   }

   /* Copy the relevant contents of the arb_program struct into the
    * fragment_program struct.
    */
   program->Base.String          = ap.Base.String;
   program->Base.NumInstructions = ap.Base.NumInstructions;
   program->Base.NumTemporaries  = ap.Base.NumTemporaries;
   program->Base.NumParameters   = ap.Base.NumParameters;
   program->Base.NumAttributes   = ap.Base.NumAttributes;
   program->Base.NumAddressRegs  = ap.Base.NumAddressRegs;
   program->Base.NumNativeInstructions = ap.Base.NumNativeInstructions;
   program->Base.NumNativeTemporaries = ap.Base.NumNativeTemporaries;
   program->Base.NumNativeParameters = ap.Base.NumNativeParameters;
   program->Base.NumNativeAttributes = ap.Base.NumNativeAttributes;
   program->Base.NumNativeAddressRegs = ap.Base.NumNativeAddressRegs;
   program->Base.NumAluInstructions   = ap.Base.NumAluInstructions;
   program->Base.NumTexInstructions   = ap.Base.NumTexInstructions;
   program->Base.NumTexIndirections   = ap.Base.NumTexIndirections;
   program->Base.NumNativeAluInstructions = ap.Base.NumAluInstructions;
   program->Base.NumNativeTexInstructions = ap.Base.NumTexInstructions;
   program->Base.NumNativeTexIndirections = ap.Base.NumTexIndirections;
   program->Base.InputsRead      = ap.Base.InputsRead;
   program->Base.OutputsWritten  = ap.Base.OutputsWritten;
   for (i = 0; i < MAX_TEXTURE_IMAGE_UNITS; i++) {
      program->Base.TexturesUsed[i] = ap.TexturesUsed[i];
      if (ap.TexturesUsed[i])
         program->Base.SamplersUsed |= (1 << i);
   }
   program->Base.ShadowSamplers = ap.ShadowSamplers;
   program->FogOption          = ap.FogOption;
   program->UsesKill          = ap.UsesKill;

   if (program->FogOption)
      program->Base.InputsRead |= FRAG_BIT_FOGC;
      
   if (program->Base.Instructions)
      _mesa_free(program->Base.Instructions);
   program->Base.Instructions = ap.Base.Instructions;

   if (program->Base.Parameters)
      _mesa_free_parameter_list(program->Base.Parameters);
   program->Base.Parameters    = ap.Base.Parameters;

#if DEBUG_FP
   _mesa_printf("____________Fragment program %u ________\n", program->Base.Id);
   _mesa_print_program(&program->Base);
#endif
}



/**
 * Parse the vertex program string.  If success, update the given
 * vertex_program object with the new program.  Else, leave the vertex_program
 * object unchanged.
 */
void
_mesa_parse_arb_vertex_program(GLcontext *ctx, GLenum target,
			       const GLvoid *str, GLsizei len,
			       struct gl_vertex_program *program)
{
   struct arb_program ap;

   ASSERT(target == GL_VERTEX_PROGRAM_ARB);

   if (!_mesa_parse_arb_program(ctx, target, (const GLubyte*) str, len, &ap)) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glProgramString(bad program)");
      return;
   }

   /* Copy the relevant contents of the arb_program struct into the 
    * vertex_program struct.
    */
   program->Base.String          = ap.Base.String;
   program->Base.NumInstructions = ap.Base.NumInstructions;
   program->Base.NumTemporaries  = ap.Base.NumTemporaries;
   program->Base.NumParameters   = ap.Base.NumParameters;
   program->Base.NumAttributes   = ap.Base.NumAttributes;
   program->Base.NumAddressRegs  = ap.Base.NumAddressRegs;
   program->Base.NumNativeInstructions = ap.Base.NumNativeInstructions;
   program->Base.NumNativeTemporaries = ap.Base.NumNativeTemporaries;
   program->Base.NumNativeParameters = ap.Base.NumNativeParameters;
   program->Base.NumNativeAttributes = ap.Base.NumNativeAttributes;
   program->Base.NumNativeAddressRegs = ap.Base.NumNativeAddressRegs;
   program->Base.InputsRead     = ap.Base.InputsRead;
   program->Base.OutputsWritten = ap.Base.OutputsWritten;
   program->IsPositionInvariant = ap.HintPositionInvariant;

   if (program->Base.Instructions)
      _mesa_free(program->Base.Instructions);
   program->Base.Instructions = ap.Base.Instructions;

   if (program->Base.Parameters)
      _mesa_free_parameter_list(program->Base.Parameters);
   program->Base.Parameters = ap.Base.Parameters; 

#if DEBUG_VP
   _mesa_printf("____________Vertex program %u __________\n", program->Base.Id);
   _mesa_print_program(&program->Base);
#endif
}

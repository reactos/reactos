/*
 * Mesa 3-D graphics library
 * Version:  6.3
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

/**
 * \file program.c
 * Vertex and fragment program support functions.
 * \author Brian Paul
 */


/**
 * \mainpage Mesa vertex and fragment program module
 *
 * This module or directory contains most of the code for vertex and
 * fragment programs and shaders, including state management, parsers,
 * and (some) software routines for executing programs
 */

#ifndef PROGRAM_H
#define PROGRAM_H

#include "mtypes.h"


/* for GL_ARB_v_p and GL_ARB_f_p SWZ instruction */
#define SWIZZLE_X    0
#define SWIZZLE_Y    1
#define SWIZZLE_Z    2
#define SWIZZLE_W    3
#define SWIZZLE_ZERO 4		/* keep these values together: KW */
#define SWIZZLE_ONE  5		/* keep these values together: KW */

#define MAKE_SWIZZLE4(a,b,c,d) (((a)<<0) | ((b)<<3) | ((c)<<6) | ((d)<<9))
#define MAKE_SWIZZLE(x)        MAKE_SWIZZLE4((x)[0], (x)[1], (x)[2], (x)[3])
#define SWIZZLE_NOOP           MAKE_SWIZZLE4(0,1,2,3)
#define GET_SWZ(swz, idx)      (((swz) >> ((idx)*3)) & 0x7)
#define GET_BIT(msk, idx)      (((msk) >> (idx)) & 0x1)


#define WRITEMASK_X     0x1
#define WRITEMASK_Y     0x2
#define WRITEMASK_XY    0x3
#define WRITEMASK_Z     0x4
#define WRITEMASK_XZ    0x5
#define WRITEMASK_YZ    0x6
#define WRITEMASK_XYZ   0x7
#define WRITEMASK_W     0x8
#define WRITEMASK_XW    0x9
#define WRITEMASK_YW    0xa
#define WRITEMASK_XYW   0xb
#define WRITEMASK_ZW    0xc
#define WRITEMASK_XZW   0xd
#define WRITEMASK_YZW   0xe
#define WRITEMASK_XYZW  0xf


extern struct program _mesa_DummyProgram;


/*
 * Internal functions
 */

extern void
_mesa_init_program(GLcontext *ctx);

extern void
_mesa_free_program_data(GLcontext *ctx);

extern void
_mesa_set_program_error(GLcontext *ctx, GLint pos, const char *string);

extern const GLubyte *
_mesa_find_line_column(const GLubyte *string, const GLubyte *pos,
                       GLint *line, GLint *col);


extern struct program *
_mesa_init_vertex_program(GLcontext *ctx,
                          struct vertex_program *prog,
                          GLenum target, GLuint id);

extern struct program *
_mesa_init_fragment_program(GLcontext *ctx,
                            struct fragment_program *prog,
                            GLenum target, GLuint id);

extern struct program *
_mesa_init_ati_fragment_shader(GLcontext *ctx,
                               struct ati_fragment_shader *prog,
                               GLenum target, GLuint id );

extern struct program *
_mesa_new_program(GLcontext *ctx, GLenum target, GLuint id);

extern void
_mesa_delete_program(GLcontext *ctx, struct program *prog);



/*
 * Used for describing GL state referenced from inside ARB vertex and
 * fragment programs.
 * A string such as "state.light[0].ambient" gets translated into a
 * sequence of tokens such as [ STATE_LIGHT, 0, STATE_AMBIENT ].
 */
enum state_index {
   STATE_MATERIAL,

   STATE_LIGHT,
   STATE_LIGHTMODEL_AMBIENT,
   STATE_LIGHTMODEL_SCENECOLOR,
   STATE_LIGHTPROD,

   STATE_TEXGEN,

   STATE_FOG_COLOR,
   STATE_FOG_PARAMS,

   STATE_CLIPPLANE,

   STATE_POINT_SIZE,
   STATE_POINT_ATTENUATION,

   STATE_MATRIX,
   STATE_MODELVIEW,
   STATE_PROJECTION,
   STATE_MVP,
   STATE_TEXTURE,
   STATE_PROGRAM,
   STATE_MATRIX_INVERSE,
   STATE_MATRIX_TRANSPOSE,
   STATE_MATRIX_INVTRANS,

   STATE_AMBIENT,
   STATE_DIFFUSE,
   STATE_SPECULAR,
   STATE_EMISSION,
   STATE_SHININESS,
   STATE_HALF,

   STATE_POSITION,
   STATE_ATTENUATION,
   STATE_SPOT_DIRECTION,

   STATE_TEXGEN_EYE_S,
   STATE_TEXGEN_EYE_T,
   STATE_TEXGEN_EYE_R,
   STATE_TEXGEN_EYE_Q,
   STATE_TEXGEN_OBJECT_S,
   STATE_TEXGEN_OBJECT_T,
   STATE_TEXGEN_OBJECT_R,
   STATE_TEXGEN_OBJECT_Q,

   STATE_TEXENV_COLOR,

   STATE_DEPTH_RANGE,

   STATE_VERTEX_PROGRAM,
   STATE_FRAGMENT_PROGRAM,

   STATE_ENV,
   STATE_LOCAL,

   STATE_INTERNAL,		/* Mesa additions */
   STATE_NORMAL_SCALE,
   STATE_POSITION_NORMALIZED
};



/*
 * Named program parameters
 * Used for NV_fragment_program "DEFINE"d constants and "DECLARE"d parameters,
 * and ARB_fragment_program global state references.  For the later, Name
 * might be "state.light[0].diffuse", for example.
 */

enum parameter_type
{
   NAMED_PARAMETER,
   CONSTANT,
   STATE
};


struct program_parameter
{
   const char *Name;                   /* Null-terminated */
   enum parameter_type Type;
   enum state_index StateIndexes[6];   /* Global state reference */
};


struct program_parameter_list
{
   GLuint Size;
   GLuint NumParameters;
   struct program_parameter *Parameters;
   GLfloat (*ParameterValues)[4];
};


/*
 * Program parameter functions
 */

extern struct program_parameter_list *
_mesa_new_parameter_list(void);

extern void
_mesa_free_parameter_list(struct program_parameter_list *paramList);

extern void
_mesa_free_parameters(struct program_parameter_list *paramList);

extern GLint
_mesa_add_named_parameter(struct program_parameter_list *paramList,
                          const char *name, const GLfloat values[4]);

extern GLint
_mesa_add_named_constant(struct program_parameter_list *paramList,
                         const char *name, const GLfloat values[4]);

extern GLint
_mesa_add_unnamed_constant(struct program_parameter_list *paramList,
                           const GLfloat values[4]);

extern GLint
_mesa_add_state_reference(struct program_parameter_list *paramList,
                          GLint *stateTokens);

extern GLfloat *
_mesa_lookup_parameter_value(struct program_parameter_list *paramList,
                             GLsizei nameLen, const char *name);

extern GLint
_mesa_lookup_parameter_index(struct program_parameter_list *paramList,
                             GLsizei nameLen, const char *name);

extern void
_mesa_load_state_parameters(GLcontext *ctx,
                            struct program_parameter_list *paramList);


/*
 * API functions
 */

extern void GLAPIENTRY
_mesa_BindProgram(GLenum target, GLuint id);

extern void GLAPIENTRY
_mesa_DeletePrograms(GLsizei n, const GLuint *ids);

extern void GLAPIENTRY
_mesa_GenPrograms(GLsizei n, GLuint *ids);

extern GLboolean GLAPIENTRY
_mesa_IsProgram(GLuint id);



/*
 * GL_MESA_program_debug
 */

extern void
_mesa_ProgramCallbackMESA(GLenum target, GLprogramcallbackMESA callback,
                          GLvoid *data);

extern void
_mesa_GetProgramRegisterfvMESA(GLenum target, GLsizei len,
                               const GLubyte *registerName, GLfloat *v);


#endif /* PROGRAM_H */

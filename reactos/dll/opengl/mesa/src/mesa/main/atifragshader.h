/*
 * Mesa 3-D graphics library ATI Fragment Shader
 *
 * Copyright (C) 2004  David Airlie   All Rights Reserved.
 *
 */

#ifndef ATIFRAGSHADER_H
#define ATIFRAGSHADER_H

#include "compiler.h"
#include "glheader.h"
#include "mfeatures.h"

struct _glapi_table;
struct gl_context;

#define MAX_NUM_INSTRUCTIONS_PER_PASS_ATI 8
#define MAX_NUM_PASSES_ATI                2
#define MAX_NUM_FRAGMENT_REGISTERS_ATI    6

struct ati_fs_opcode_st
{
   GLenum opcode;
   GLint num_src_args;
};

extern struct ati_fs_opcode_st ati_fs_opcodes[];

struct atifragshader_src_register
{
   GLuint Index;
   GLuint argRep;
   GLuint argMod;
};

struct atifragshader_dst_register
{
   GLuint Index;
   GLuint dstMod;
   GLuint dstMask;
};

#define ATI_FRAGMENT_SHADER_COLOR_OP 0
#define ATI_FRAGMENT_SHADER_ALPHA_OP 1
#define ATI_FRAGMENT_SHADER_PASS_OP  2
#define ATI_FRAGMENT_SHADER_SAMPLE_OP 3

/* two opcodes - one for color/one for alpha */
/* up to three source registers for most ops */
struct atifs_instruction
{
   GLenum Opcode[2];
   GLuint ArgCount[2];
   struct atifragshader_src_register SrcReg[2][3];
   struct atifragshader_dst_register DstReg[2];
};

/* different from arithmetic shader instruction */
struct atifs_setupinst
{
   GLenum Opcode;
   GLuint src;
   GLenum swizzle;
};


#if FEATURE_ATI_fragment_shader

extern void
_mesa_init_ati_fragment_shader_dispatch(struct _glapi_table *disp);

extern struct ati_fragment_shader *
_mesa_new_ati_fragment_shader(struct gl_context *ctx, GLuint id);

extern void
_mesa_delete_ati_fragment_shader(struct gl_context *ctx,
                                 struct ati_fragment_shader *s);


extern GLuint GLAPIENTRY _mesa_GenFragmentShadersATI(GLuint range);

extern void GLAPIENTRY _mesa_BindFragmentShaderATI(GLuint id);

extern void GLAPIENTRY _mesa_DeleteFragmentShaderATI(GLuint id);

extern void GLAPIENTRY _mesa_BeginFragmentShaderATI(void);

extern void GLAPIENTRY _mesa_EndFragmentShaderATI(void);

extern void GLAPIENTRY
_mesa_PassTexCoordATI(GLuint dst, GLuint coord, GLenum swizzle);

extern void GLAPIENTRY
_mesa_SampleMapATI(GLuint dst, GLuint interp, GLenum swizzle);

extern void GLAPIENTRY
_mesa_ColorFragmentOp1ATI(GLenum op, GLuint dst, GLuint dstMask,
			  GLuint dstMod, GLuint arg1, GLuint arg1Rep,
			  GLuint arg1Mod);

extern void GLAPIENTRY
_mesa_ColorFragmentOp2ATI(GLenum op, GLuint dst, GLuint dstMask,
			  GLuint dstMod, GLuint arg1, GLuint arg1Rep,
			  GLuint arg1Mod, GLuint arg2, GLuint arg2Rep,
			  GLuint arg2Mod);

extern void GLAPIENTRY
_mesa_ColorFragmentOp3ATI(GLenum op, GLuint dst, GLuint dstMask,
			  GLuint dstMod, GLuint arg1, GLuint arg1Rep,
			  GLuint arg1Mod, GLuint arg2, GLuint arg2Rep,
			  GLuint arg2Mod, GLuint arg3, GLuint arg3Rep,
			  GLuint arg3Mod);

extern void GLAPIENTRY
_mesa_AlphaFragmentOp1ATI(GLenum op, GLuint dst, GLuint dstMod, GLuint arg1,
			  GLuint arg1Rep, GLuint arg1Mod);

extern void GLAPIENTRY
_mesa_AlphaFragmentOp2ATI(GLenum op, GLuint dst, GLuint dstMod, GLuint arg1,
			  GLuint arg1Rep, GLuint arg1Mod, GLuint arg2,
			  GLuint arg2Rep, GLuint arg2Mod);

extern void GLAPIENTRY
_mesa_AlphaFragmentOp3ATI(GLenum op, GLuint dst, GLuint dstMod, GLuint arg1,
			  GLuint arg1Rep, GLuint arg1Mod, GLuint arg2,
			  GLuint arg2Rep, GLuint arg2Mod, GLuint arg3,
			  GLuint arg3Rep, GLuint arg3Mod);

extern void GLAPIENTRY
_mesa_SetFragmentShaderConstantATI(GLuint dst, const GLfloat * value);

#else /* FEATURE_ATI_fragment_shader */

static inline void
_mesa_init_ati_fragment_shader_dispatch(struct _glapi_table *disp)
{
}

static inline struct ati_fragment_shader *
_mesa_new_ati_fragment_shader(struct gl_context *ctx, GLuint id)
{
   return NULL;
}

static inline void
_mesa_delete_ati_fragment_shader(struct gl_context *ctx,
                                 struct ati_fragment_shader *s)
{
}

#endif /* FEATURE_ATI_fragment_shader */

#endif /* ATIFRAGSHADER_H */

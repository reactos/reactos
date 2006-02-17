/*
 * Mesa 3-D graphics library ATI Fragment Shader
 *
 * Copyright (C) 2004  David Airlie   All Rights Reserved.
 *
 */

#ifndef ATIFRAGSHADER_H
#define ATIFRAGSHADER_H

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
   GLuint Swizzle;
};

#define ATI_FRAGMENT_SHADER_COLOR_OP 0
#define ATI_FRAGMENT_SHADER_ALPHA_OP 1
#define ATI_FRAGMENT_SHADER_PASS_OP  2
#define ATI_FRAGMENT_SHADER_SAMPLE_OP 3

/* two opcodes - one for color/one for alpha - also pass/sample */
/* up to three source registers for most ops */
struct atifs_instruction
{
   GLenum Opcode[2];
   GLuint ArgCount[2];
   struct atifragshader_src_register SrcReg[2][3];
   struct atifragshader_dst_register DstReg[2];
};

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

#endif

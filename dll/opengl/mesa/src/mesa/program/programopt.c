/*
 * Mesa 3-D graphics library
 * Version:  6.5.3
 *
 * Copyright (C) 1999-2007  Brian Paul   All Rights Reserved.
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
 * \file  programopt.c 
 * Vertex/Fragment program optimizations and transformations for program
 * options, etc.
 *
 * \author Brian Paul
 */


#include "main/glheader.h"
#include "main/context.h"
#include "prog_parameter.h"
#include "prog_statevars.h"
#include "program.h"
#include "programopt.h"
#include "prog_instruction.h"


/**
 * This function inserts instructions for coordinate modelview * projection
 * into a vertex program.
 * May be used to implement the position_invariant option.
 */
static void
_mesa_insert_mvp_dp4_code(struct gl_context *ctx, struct gl_vertex_program *vprog)
{
   struct prog_instruction *newInst;
   const GLuint origLen = vprog->Base.NumInstructions;
   const GLuint newLen = origLen + 4;
   GLuint i;

   /*
    * Setup state references for the modelview/projection matrix.
    * XXX we should check if these state vars are already declared.
    */
   static const gl_state_index mvpState[4][STATE_LENGTH] = {
      { STATE_MVP_MATRIX, 0, 0, 0, 0 },  /* state.matrix.mvp.row[0] */
      { STATE_MVP_MATRIX, 0, 1, 1, 0 },  /* state.matrix.mvp.row[1] */
      { STATE_MVP_MATRIX, 0, 2, 2, 0 },  /* state.matrix.mvp.row[2] */
      { STATE_MVP_MATRIX, 0, 3, 3, 0 },  /* state.matrix.mvp.row[3] */
   };
   GLint mvpRef[4];

   for (i = 0; i < 4; i++) {
      mvpRef[i] = _mesa_add_state_reference(vprog->Base.Parameters,
                                            mvpState[i]);
   }

   /* Alloc storage for new instructions */
   newInst = _mesa_alloc_instructions(newLen);
   if (!newInst) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY,
                  "glProgramString(inserting position_invariant code)");
      return;
   }

   /*
    * Generated instructions:
    * newInst[0] = DP4 result.position.x, mvp.row[0], vertex.position;
    * newInst[1] = DP4 result.position.y, mvp.row[1], vertex.position;
    * newInst[2] = DP4 result.position.z, mvp.row[2], vertex.position;
    * newInst[3] = DP4 result.position.w, mvp.row[3], vertex.position;
    */
   _mesa_init_instructions(newInst, 4);
   for (i = 0; i < 4; i++) {
      newInst[i].Opcode = OPCODE_DP4;
      newInst[i].DstReg.File = PROGRAM_OUTPUT;
      newInst[i].DstReg.Index = VERT_RESULT_HPOS;
      newInst[i].DstReg.WriteMask = (WRITEMASK_X << i);
      newInst[i].SrcReg[0].File = PROGRAM_STATE_VAR;
      newInst[i].SrcReg[0].Index = mvpRef[i];
      newInst[i].SrcReg[0].Swizzle = SWIZZLE_NOOP;
      newInst[i].SrcReg[1].File = PROGRAM_INPUT;
      newInst[i].SrcReg[1].Index = VERT_ATTRIB_POS;
      newInst[i].SrcReg[1].Swizzle = SWIZZLE_NOOP;
   }

   /* Append original instructions after new instructions */
   _mesa_copy_instructions (newInst + 4, vprog->Base.Instructions, origLen);

   /* free old instructions */
   _mesa_free_instructions(vprog->Base.Instructions, origLen);

   /* install new instructions */
   vprog->Base.Instructions = newInst;
   vprog->Base.NumInstructions = newLen;
   vprog->Base.InputsRead |= VERT_BIT_POS;
   vprog->Base.OutputsWritten |= BITFIELD64_BIT(VERT_RESULT_HPOS);
}


static void
_mesa_insert_mvp_mad_code(struct gl_context *ctx, struct gl_vertex_program *vprog)
{
   struct prog_instruction *newInst;
   const GLuint origLen = vprog->Base.NumInstructions;
   const GLuint newLen = origLen + 4;
   GLuint hposTemp;
   GLuint i;

   /*
    * Setup state references for the modelview/projection matrix.
    * XXX we should check if these state vars are already declared.
    */
   static const gl_state_index mvpState[4][STATE_LENGTH] = {
      { STATE_MVP_MATRIX, 0, 0, 0, STATE_MATRIX_TRANSPOSE },
      { STATE_MVP_MATRIX, 0, 1, 1, STATE_MATRIX_TRANSPOSE },
      { STATE_MVP_MATRIX, 0, 2, 2, STATE_MATRIX_TRANSPOSE },
      { STATE_MVP_MATRIX, 0, 3, 3, STATE_MATRIX_TRANSPOSE },
   };
   GLint mvpRef[4];

   for (i = 0; i < 4; i++) {
      mvpRef[i] = _mesa_add_state_reference(vprog->Base.Parameters,
                                            mvpState[i]);
   }

   /* Alloc storage for new instructions */
   newInst = _mesa_alloc_instructions(newLen);
   if (!newInst) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY,
                  "glProgramString(inserting position_invariant code)");
      return;
   }

   /* TEMP hposTemp; */
   hposTemp = vprog->Base.NumTemporaries++;

   /*
    * Generated instructions:
    *    emit_op2(p, OPCODE_MUL, tmp, 0, swizzle1(src,X), mat[0]);
    *    emit_op3(p, OPCODE_MAD, tmp, 0, swizzle1(src,Y), mat[1], tmp);
    *    emit_op3(p, OPCODE_MAD, tmp, 0, swizzle1(src,Z), mat[2], tmp);
    *    emit_op3(p, OPCODE_MAD, dest, 0, swizzle1(src,W), mat[3], tmp);
    */
   _mesa_init_instructions(newInst, 4);

   newInst[0].Opcode = OPCODE_MUL;
   newInst[0].DstReg.File = PROGRAM_TEMPORARY;
   newInst[0].DstReg.Index = hposTemp;
   newInst[0].DstReg.WriteMask = WRITEMASK_XYZW;
   newInst[0].SrcReg[0].File = PROGRAM_INPUT;
   newInst[0].SrcReg[0].Index = VERT_ATTRIB_POS;
   newInst[0].SrcReg[0].Swizzle = SWIZZLE_XXXX;
   newInst[0].SrcReg[1].File = PROGRAM_STATE_VAR;
   newInst[0].SrcReg[1].Index = mvpRef[0];
   newInst[0].SrcReg[1].Swizzle = SWIZZLE_NOOP;

   for (i = 1; i <= 2; i++) {
      newInst[i].Opcode = OPCODE_MAD;
      newInst[i].DstReg.File = PROGRAM_TEMPORARY;
      newInst[i].DstReg.Index = hposTemp;
      newInst[i].DstReg.WriteMask = WRITEMASK_XYZW;
      newInst[i].SrcReg[0].File = PROGRAM_INPUT;
      newInst[i].SrcReg[0].Index = VERT_ATTRIB_POS;
      newInst[i].SrcReg[0].Swizzle = MAKE_SWIZZLE4(i,i,i,i);
      newInst[i].SrcReg[1].File = PROGRAM_STATE_VAR;
      newInst[i].SrcReg[1].Index = mvpRef[i];
      newInst[i].SrcReg[1].Swizzle = SWIZZLE_NOOP;
      newInst[i].SrcReg[2].File = PROGRAM_TEMPORARY;
      newInst[i].SrcReg[2].Index = hposTemp;
      newInst[1].SrcReg[2].Swizzle = SWIZZLE_NOOP;
   }

   newInst[3].Opcode = OPCODE_MAD;
   newInst[3].DstReg.File = PROGRAM_OUTPUT;
   newInst[3].DstReg.Index = VERT_RESULT_HPOS;
   newInst[3].DstReg.WriteMask = WRITEMASK_XYZW;
   newInst[3].SrcReg[0].File = PROGRAM_INPUT;
   newInst[3].SrcReg[0].Index = VERT_ATTRIB_POS;
   newInst[3].SrcReg[0].Swizzle = SWIZZLE_WWWW;
   newInst[3].SrcReg[1].File = PROGRAM_STATE_VAR;
   newInst[3].SrcReg[1].Index = mvpRef[3];
   newInst[3].SrcReg[1].Swizzle = SWIZZLE_NOOP;
   newInst[3].SrcReg[2].File = PROGRAM_TEMPORARY;
   newInst[3].SrcReg[2].Index = hposTemp;
   newInst[3].SrcReg[2].Swizzle = SWIZZLE_NOOP;


   /* Append original instructions after new instructions */
   _mesa_copy_instructions (newInst + 4, vprog->Base.Instructions, origLen);

   /* free old instructions */
   _mesa_free_instructions(vprog->Base.Instructions, origLen);

   /* install new instructions */
   vprog->Base.Instructions = newInst;
   vprog->Base.NumInstructions = newLen;
   vprog->Base.InputsRead |= VERT_BIT_POS;
   vprog->Base.OutputsWritten |= BITFIELD64_BIT(VERT_RESULT_HPOS);
}


void
_mesa_insert_mvp_code(struct gl_context *ctx, struct gl_vertex_program *vprog)
{
   if (ctx->mvp_with_dp4) 
      _mesa_insert_mvp_dp4_code( ctx, vprog );
   else
      _mesa_insert_mvp_mad_code( ctx, vprog );
}
      





/**
 * Append instructions to implement fog
 *
 * The \c fragment.fogcoord input is used to compute the fog blend factor.
 *
 * \param ctx      The GL context
 * \param fprog    Fragment program that fog instructions will be appended to.
 * \param fog_mode Fog mode.  One of \c GL_EXP, \c GL_EXP2, or \c GL_LINEAR.
 * \param saturate True if writes to color outputs should be clamped to [0, 1]
 *
 * \note
 * This function sets \c FRAG_BIT_FOGC in \c fprog->Base.InputsRead.
 *
 * \todo With a little work, this function could be adapted to add fog code
 * to vertex programs too.
 */
void
_mesa_append_fog_code(struct gl_context *ctx,
		      struct gl_fragment_program *fprog, GLenum fog_mode,
		      GLboolean saturate)
{
   static const gl_state_index fogPStateOpt[STATE_LENGTH]
      = { STATE_INTERNAL, STATE_FOG_PARAMS_OPTIMIZED, 0, 0, 0 };
   static const gl_state_index fogColorState[STATE_LENGTH]
      = { STATE_FOG_COLOR, 0, 0, 0, 0};
   struct prog_instruction *newInst, *inst;
   const GLuint origLen = fprog->Base.NumInstructions;
   const GLuint newLen = origLen + 5;
   GLuint i;
   GLint fogPRefOpt, fogColorRef; /* state references */
   GLuint colorTemp, fogFactorTemp; /* temporary registerss */

   if (fog_mode == GL_NONE) {
      _mesa_problem(ctx, "_mesa_append_fog_code() called for fragment program"
                    " with fog_mode == GL_NONE");
      return;
   }

   if (!(fprog->Base.OutputsWritten & (1 << FRAG_RESULT_COLOR))) {
      /* program doesn't output color, so nothing to do */
      return;
   }

   /* Alloc storage for new instructions */
   newInst = _mesa_alloc_instructions(newLen);
   if (!newInst) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY,
                  "glProgramString(inserting fog_option code)");
      return;
   }

   /* Copy orig instructions into new instruction buffer */
   _mesa_copy_instructions(newInst, fprog->Base.Instructions, origLen);

   /* PARAM fogParamsRefOpt = internal optimized fog params; */
   fogPRefOpt
      = _mesa_add_state_reference(fprog->Base.Parameters, fogPStateOpt);
   /* PARAM fogColorRef = state.fog.color; */
   fogColorRef
      = _mesa_add_state_reference(fprog->Base.Parameters, fogColorState);

   /* TEMP colorTemp; */
   colorTemp = fprog->Base.NumTemporaries++;
   /* TEMP fogFactorTemp; */
   fogFactorTemp = fprog->Base.NumTemporaries++;

   /* Scan program to find where result.color is written */
   inst = newInst;
   for (i = 0; i < fprog->Base.NumInstructions; i++) {
      if (inst->Opcode == OPCODE_END)
         break;
      if (inst->DstReg.File == PROGRAM_OUTPUT &&
          inst->DstReg.Index == FRAG_RESULT_COLOR) {
         /* change the instruction to write to colorTemp w/ clamping */
         inst->DstReg.File = PROGRAM_TEMPORARY;
         inst->DstReg.Index = colorTemp;
         inst->SaturateMode = saturate;
         /* don't break (may be several writes to result.color) */
      }
      inst++;
   }
   assert(inst->Opcode == OPCODE_END); /* we'll overwrite this inst */

   _mesa_init_instructions(inst, 5);

   /* emit instructions to compute fog blending factor */
   /* this is always clamped to [0, 1] regardless of fragment clamping */
   if (fog_mode == GL_LINEAR) {
      /* MAD fogFactorTemp.x, fragment.fogcoord.x, fogPRefOpt.x, fogPRefOpt.y; */
      inst->Opcode = OPCODE_MAD;
      inst->DstReg.File = PROGRAM_TEMPORARY;
      inst->DstReg.Index = fogFactorTemp;
      inst->DstReg.WriteMask = WRITEMASK_X;
      inst->SrcReg[0].File = PROGRAM_INPUT;
      inst->SrcReg[0].Index = FRAG_ATTRIB_FOGC;
      inst->SrcReg[0].Swizzle = SWIZZLE_XXXX;
      inst->SrcReg[1].File = PROGRAM_STATE_VAR;
      inst->SrcReg[1].Index = fogPRefOpt;
      inst->SrcReg[1].Swizzle = SWIZZLE_XXXX;
      inst->SrcReg[2].File = PROGRAM_STATE_VAR;
      inst->SrcReg[2].Index = fogPRefOpt;
      inst->SrcReg[2].Swizzle = SWIZZLE_YYYY;
      inst->SaturateMode = SATURATE_ZERO_ONE;
      inst++;
   }
   else {
      ASSERT(fog_mode == GL_EXP || fog_mode == GL_EXP2);
      /* fogPRefOpt.z = d/ln(2), fogPRefOpt.w = d/sqrt(ln(2) */
      /* EXP: MUL fogFactorTemp.x, fogPRefOpt.z, fragment.fogcoord.x; */
      /* EXP2: MUL fogFactorTemp.x, fogPRefOpt.w, fragment.fogcoord.x; */
      inst->Opcode = OPCODE_MUL;
      inst->DstReg.File = PROGRAM_TEMPORARY;
      inst->DstReg.Index = fogFactorTemp;
      inst->DstReg.WriteMask = WRITEMASK_X;
      inst->SrcReg[0].File = PROGRAM_STATE_VAR;
      inst->SrcReg[0].Index = fogPRefOpt;
      inst->SrcReg[0].Swizzle
         = (fog_mode == GL_EXP) ? SWIZZLE_ZZZZ : SWIZZLE_WWWW;
      inst->SrcReg[1].File = PROGRAM_INPUT;
      inst->SrcReg[1].Index = FRAG_ATTRIB_FOGC;
      inst->SrcReg[1].Swizzle = SWIZZLE_XXXX;
      inst++;
      if (fog_mode == GL_EXP2) {
         /* MUL fogFactorTemp.x, fogFactorTemp.x, fogFactorTemp.x; */
         inst->Opcode = OPCODE_MUL;
         inst->DstReg.File = PROGRAM_TEMPORARY;
         inst->DstReg.Index = fogFactorTemp;
         inst->DstReg.WriteMask = WRITEMASK_X;
         inst->SrcReg[0].File = PROGRAM_TEMPORARY;
         inst->SrcReg[0].Index = fogFactorTemp;
         inst->SrcReg[0].Swizzle = SWIZZLE_XXXX;
         inst->SrcReg[1].File = PROGRAM_TEMPORARY;
         inst->SrcReg[1].Index = fogFactorTemp;
         inst->SrcReg[1].Swizzle = SWIZZLE_XXXX;
         inst++;
      }
      /* EX2_SAT fogFactorTemp.x, -fogFactorTemp.x; */
      inst->Opcode = OPCODE_EX2;
      inst->DstReg.File = PROGRAM_TEMPORARY;
      inst->DstReg.Index = fogFactorTemp;
      inst->DstReg.WriteMask = WRITEMASK_X;
      inst->SrcReg[0].File = PROGRAM_TEMPORARY;
      inst->SrcReg[0].Index = fogFactorTemp;
      inst->SrcReg[0].Negate = NEGATE_XYZW;
      inst->SrcReg[0].Swizzle = SWIZZLE_XXXX;
      inst->SaturateMode = SATURATE_ZERO_ONE;
      inst++;
   }
   /* LRP result.color.xyz, fogFactorTemp.xxxx, colorTemp, fogColorRef; */
   inst->Opcode = OPCODE_LRP;
   inst->DstReg.File = PROGRAM_OUTPUT;
   inst->DstReg.Index = FRAG_RESULT_COLOR;
   inst->DstReg.WriteMask = WRITEMASK_XYZ;
   inst->SrcReg[0].File = PROGRAM_TEMPORARY;
   inst->SrcReg[0].Index = fogFactorTemp;
   inst->SrcReg[0].Swizzle = SWIZZLE_XXXX;
   inst->SrcReg[1].File = PROGRAM_TEMPORARY;
   inst->SrcReg[1].Index = colorTemp;
   inst->SrcReg[1].Swizzle = SWIZZLE_NOOP;
   inst->SrcReg[2].File = PROGRAM_STATE_VAR;
   inst->SrcReg[2].Index = fogColorRef;
   inst->SrcReg[2].Swizzle = SWIZZLE_NOOP;
   inst++;
   /* MOV result.color.w, colorTemp.x;  # copy alpha */
   inst->Opcode = OPCODE_MOV;
   inst->DstReg.File = PROGRAM_OUTPUT;
   inst->DstReg.Index = FRAG_RESULT_COLOR;
   inst->DstReg.WriteMask = WRITEMASK_W;
   inst->SrcReg[0].File = PROGRAM_TEMPORARY;
   inst->SrcReg[0].Index = colorTemp;
   inst->SrcReg[0].Swizzle = SWIZZLE_NOOP;
   inst++;
   /* END; */
   inst->Opcode = OPCODE_END;
   inst++;

   /* free old instructions */
   _mesa_free_instructions(fprog->Base.Instructions, origLen);

   /* install new instructions */
   fprog->Base.Instructions = newInst;
   fprog->Base.NumInstructions = inst - newInst;
   fprog->Base.InputsRead |= FRAG_BIT_FOGC;
   assert(fprog->Base.OutputsWritten & (1 << FRAG_RESULT_COLOR));
}



static GLboolean
is_texture_instruction(const struct prog_instruction *inst)
{
   switch (inst->Opcode) {
   case OPCODE_TEX:
   case OPCODE_TXB:
   case OPCODE_TXD:
   case OPCODE_TXL:
   case OPCODE_TXP:
   case OPCODE_TXP_NV:
      return GL_TRUE;
   default:
      return GL_FALSE;
   }
}
      

/**
 * Count the number of texure indirections in the given program.
 * The program's NumTexIndirections field will be updated.
 * See the GL_ARB_fragment_program spec (issue 24) for details.
 * XXX we count texture indirections in texenvprogram.c (maybe use this code
 * instead and elsewhere).
 */
void
_mesa_count_texture_indirections(struct gl_program *prog)
{
   GLuint indirections = 1;
   GLbitfield tempsOutput = 0x0;
   GLbitfield aluTemps = 0x0;
   GLuint i;

   for (i = 0; i < prog->NumInstructions; i++) {
      const struct prog_instruction *inst = prog->Instructions + i;

      if (is_texture_instruction(inst)) {
         if (((inst->SrcReg[0].File == PROGRAM_TEMPORARY) && 
              (tempsOutput & (1 << inst->SrcReg[0].Index))) ||
             ((inst->Opcode != OPCODE_KIL) &&
              (inst->DstReg.File == PROGRAM_TEMPORARY) && 
              (aluTemps & (1 << inst->DstReg.Index)))) 
            {
               indirections++;
               tempsOutput = 0x0;
               aluTemps = 0x0;
            }
      }
      else {
         GLuint j;
         for (j = 0; j < 3; j++) {
            if (inst->SrcReg[j].File == PROGRAM_TEMPORARY)
               aluTemps |= (1 << inst->SrcReg[j].Index);
         }
         if (inst->DstReg.File == PROGRAM_TEMPORARY)
            aluTemps |= (1 << inst->DstReg.Index);
      }

      if ((inst->Opcode != OPCODE_KIL) && (inst->DstReg.File == PROGRAM_TEMPORARY))
         tempsOutput |= (1 << inst->DstReg.Index);
   }

   prog->NumTexIndirections = indirections;
}


/**
 * Count number of texture instructions in given program and update the
 * program's NumTexInstructions field.
 */
void
_mesa_count_texture_instructions(struct gl_program *prog)
{
   GLuint i;
   prog->NumTexInstructions = 0;
   for (i = 0; i < prog->NumInstructions; i++) {
      prog->NumTexInstructions += is_texture_instruction(prog->Instructions + i);
   }
}


/**
 * Scan/rewrite program to remove reads of custom (output) registers.
 * The passed type has to be either PROGRAM_OUTPUT or PROGRAM_VARYING
 * (for vertex shaders).
 * In GLSL shaders, varying vars can be read and written.
 * On some hardware, trying to read an output register causes trouble.
 * So, rewrite the program to use a temporary register in this case.
 */
void
_mesa_remove_output_reads(struct gl_program *prog, gl_register_file type)
{
   GLuint i;
   GLint outputMap[VERT_RESULT_MAX];
   GLuint numVaryingReads = 0;
   GLboolean usedTemps[MAX_PROGRAM_TEMPS];
   GLuint firstTemp = 0;

   _mesa_find_used_registers(prog, PROGRAM_TEMPORARY,
                             usedTemps, MAX_PROGRAM_TEMPS);

   assert(type == PROGRAM_VARYING || type == PROGRAM_OUTPUT);
   assert(prog->Target == GL_VERTEX_PROGRAM_ARB || type != PROGRAM_VARYING);

   for (i = 0; i < VERT_RESULT_MAX; i++)
      outputMap[i] = -1;

   /* look for instructions which read from varying vars */
   for (i = 0; i < prog->NumInstructions; i++) {
      struct prog_instruction *inst = prog->Instructions + i;
      const GLuint numSrc = _mesa_num_inst_src_regs(inst->Opcode);
      GLuint j;
      for (j = 0; j < numSrc; j++) {
         if (inst->SrcReg[j].File == type) {
            /* replace the read with a temp reg */
            const GLuint var = inst->SrcReg[j].Index;
            if (outputMap[var] == -1) {
               numVaryingReads++;
               outputMap[var] = _mesa_find_free_register(usedTemps,
                                                         MAX_PROGRAM_TEMPS,
                                                         firstTemp);
               firstTemp = outputMap[var] + 1;
            }
            inst->SrcReg[j].File = PROGRAM_TEMPORARY;
            inst->SrcReg[j].Index = outputMap[var];
         }
      }
   }

   if (numVaryingReads == 0)
      return; /* nothing to be done */

   /* look for instructions which write to the varying vars identified above */
   for (i = 0; i < prog->NumInstructions; i++) {
      struct prog_instruction *inst = prog->Instructions + i;
      if (inst->DstReg.File == type &&
          outputMap[inst->DstReg.Index] >= 0) {
         /* change inst to write to the temp reg, instead of the varying */
         inst->DstReg.File = PROGRAM_TEMPORARY;
         inst->DstReg.Index = outputMap[inst->DstReg.Index];
      }
   }

   /* insert new instructions to copy the temp vars to the varying vars */
   {
      struct prog_instruction *inst;
      GLint endPos, var;

      /* Look for END instruction and insert the new varying writes */
      endPos = -1;
      for (i = 0; i < prog->NumInstructions; i++) {
         struct prog_instruction *inst = prog->Instructions + i;
         if (inst->Opcode == OPCODE_END) {
            endPos = i;
            _mesa_insert_instructions(prog, i, numVaryingReads);
            break;
         }
      }

      assert(endPos >= 0);

      /* insert new MOV instructions here */
      inst = prog->Instructions + endPos;
      for (var = 0; var < VERT_RESULT_MAX; var++) {
         if (outputMap[var] >= 0) {
            /* MOV VAR[var], TEMP[tmp]; */
            inst->Opcode = OPCODE_MOV;
            inst->DstReg.File = type;
            inst->DstReg.Index = var;
            inst->SrcReg[0].File = PROGRAM_TEMPORARY;
            inst->SrcReg[0].Index = outputMap[var];
            inst++;
         }
      }
   }
}


/**
 * Make the given fragment program into a "no-op" shader.
 * Actually, just copy the incoming fragment color (or texcoord)
 * to the output color.
 * This is for debug/test purposes.
 */
void
_mesa_nop_fragment_program(struct gl_context *ctx, struct gl_fragment_program *prog)
{
   struct prog_instruction *inst;
   GLuint inputAttr;

   inst = _mesa_alloc_instructions(2);
   if (!inst) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "_mesa_nop_fragment_program");
      return;
   }

   _mesa_init_instructions(inst, 2);

   inst[0].Opcode = OPCODE_MOV;
   inst[0].DstReg.File = PROGRAM_OUTPUT;
   inst[0].DstReg.Index = FRAG_RESULT_COLOR;
   inst[0].SrcReg[0].File = PROGRAM_INPUT;
   if (prog->Base.InputsRead & FRAG_BIT_COL0)
      inputAttr = FRAG_ATTRIB_COL0;
   else
      inputAttr = FRAG_ATTRIB_TEX0;
   inst[0].SrcReg[0].Index = inputAttr;

   inst[1].Opcode = OPCODE_END;

   _mesa_free_instructions(prog->Base.Instructions,
                           prog->Base.NumInstructions);

   prog->Base.Instructions = inst;
   prog->Base.NumInstructions = 2;
   prog->Base.InputsRead = BITFIELD64_BIT(inputAttr);
   prog->Base.OutputsWritten = BITFIELD64_BIT(FRAG_RESULT_COLOR);
}


/**
 * \sa _mesa_nop_fragment_program
 * Replace the given vertex program with a "no-op" program that just
 * transforms vertex position and emits color.
 */
void
_mesa_nop_vertex_program(struct gl_context *ctx, struct gl_vertex_program *prog)
{
   struct prog_instruction *inst;
   GLuint inputAttr;

   /*
    * Start with a simple vertex program that emits color.
    */
   inst = _mesa_alloc_instructions(2);
   if (!inst) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "_mesa_nop_vertex_program");
      return;
   }

   _mesa_init_instructions(inst, 2);

   inst[0].Opcode = OPCODE_MOV;
   inst[0].DstReg.File = PROGRAM_OUTPUT;
   inst[0].DstReg.Index = VERT_RESULT_COL0;
   inst[0].SrcReg[0].File = PROGRAM_INPUT;
   if (prog->Base.InputsRead & VERT_BIT_COLOR0)
      inputAttr = VERT_ATTRIB_COLOR0;
   else
      inputAttr = VERT_ATTRIB_TEX0;
   inst[0].SrcReg[0].Index = inputAttr;

   inst[1].Opcode = OPCODE_END;

   _mesa_free_instructions(prog->Base.Instructions,
                           prog->Base.NumInstructions);

   prog->Base.Instructions = inst;
   prog->Base.NumInstructions = 2;
   prog->Base.InputsRead = BITFIELD64_BIT(inputAttr);
   prog->Base.OutputsWritten = BITFIELD64_BIT(VERT_RESULT_COL0);

   /*
    * Now insert code to do standard modelview/projection transformation.
    */
   _mesa_insert_mvp_code(ctx, prog);
}

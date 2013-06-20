/*
 * Mesa 3-D graphics library
 * Version:  7.3
 *
 * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
 * Copyright (C) 2009  VMware, Inc.  All Rights Reserved.
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
 * \file prog_print.c
 * Print vertex/fragment programs - for debugging.
 * \author Brian Paul
 */

#include "main/glheader.h"
#include "main/context.h"
#include "main/imports.h"
#include "prog_instruction.h"
#include "prog_parameter.h"
#include "prog_print.h"
#include "prog_statevars.h"



/**
 * Return string name for given program/register file.
 */
const char *
_mesa_register_file_name(gl_register_file f)
{
   switch (f) {
   case PROGRAM_TEMPORARY:
      return "TEMP";
   case PROGRAM_LOCAL_PARAM:
      return "LOCAL";
   case PROGRAM_ENV_PARAM:
      return "ENV";
   case PROGRAM_STATE_VAR:
      return "STATE";
   case PROGRAM_INPUT:
      return "INPUT";
   case PROGRAM_OUTPUT:
      return "OUTPUT";
   case PROGRAM_NAMED_PARAM:
      return "NAMED";
   case PROGRAM_CONSTANT:
      return "CONST";
   case PROGRAM_UNIFORM:
      return "UNIFORM";
   case PROGRAM_VARYING:
      return "VARYING";
   case PROGRAM_WRITE_ONLY:
      return "WRITE_ONLY";
   case PROGRAM_ADDRESS:
      return "ADDR";
   case PROGRAM_SAMPLER:
      return "SAMPLER";
   case PROGRAM_SYSTEM_VALUE:
      return "SYSVAL";
   case PROGRAM_UNDEFINED:
      return "UNDEFINED";
   default:
      {
         static char s[20];
         _mesa_snprintf(s, sizeof(s), "FILE%u", f);
         return s;
      }
   }
}


/**
 * Return ARB_v/f_prog-style input attrib string.
 */
static const char *
arb_input_attrib_string(GLint index, GLenum progType)
{
   /*
    * These strings should match the VERT_ATTRIB_x and FRAG_ATTRIB_x tokens.
    */
   static const char *const vertAttribs[] = {
      "vertex.position",
      "vertex.weight",
      "vertex.normal",
      "vertex.color.primary",
      "vertex.color.secondary",
      "vertex.fogcoord",
      "vertex.(six)", /* VERT_ATTRIB_COLOR_INDEX */
      "vertex.(seven)", /* VERT_ATTRIB_EDGEFLAG */
      "vertex.texcoord[0]",
      "vertex.texcoord[1]",
      "vertex.texcoord[2]",
      "vertex.texcoord[3]",
      "vertex.texcoord[4]",
      "vertex.texcoord[5]",
      "vertex.texcoord[6]",
      "vertex.texcoord[7]",
      "vertex.(sixteen)", /* VERT_ATTRIB_POINT_SIZE */
      "vertex.attrib[0]",
      "vertex.attrib[1]",
      "vertex.attrib[2]",
      "vertex.attrib[3]",
      "vertex.attrib[4]",
      "vertex.attrib[5]",
      "vertex.attrib[6]",
      "vertex.attrib[7]",
      "vertex.attrib[8]",
      "vertex.attrib[9]",
      "vertex.attrib[10]",
      "vertex.attrib[11]",
      "vertex.attrib[12]",
      "vertex.attrib[13]",
      "vertex.attrib[14]",
      "vertex.attrib[15]" /* MAX_VARYING = 16 */
   };
   static const char *const fragAttribs[] = {
      "fragment.position",
      "fragment.color.primary",
      "fragment.color.secondary",
      "fragment.fogcoord",
      "fragment.texcoord[0]",
      "fragment.texcoord[1]",
      "fragment.texcoord[2]",
      "fragment.texcoord[3]",
      "fragment.texcoord[4]",
      "fragment.texcoord[5]",
      "fragment.texcoord[6]",
      "fragment.texcoord[7]",
      "fragment.(twelve)", /* FRAG_ATTRIB_FACE */
      "fragment.(thirteen)", /* FRAG_ATTRIB_PNTC */
      "fragment.(fourteen)", /* FRAG_ATTRIB_CLIP_DIST0 */
      "fragment.(fifteen)", /* FRAG_ATTRIB_CLIP_DIST1 */
      "fragment.varying[0]",
      "fragment.varying[1]",
      "fragment.varying[2]",
      "fragment.varying[3]",
      "fragment.varying[4]",
      "fragment.varying[5]",
      "fragment.varying[6]",
      "fragment.varying[7]",
      "fragment.varying[8]",
      "fragment.varying[9]",
      "fragment.varying[10]",
      "fragment.varying[11]",
      "fragment.varying[12]",
      "fragment.varying[13]",
      "fragment.varying[14]",
      "fragment.varying[15]" /* MAX_VARYING = 16 */
   };

   /* sanity checks */
   STATIC_ASSERT(Elements(vertAttribs) == VERT_ATTRIB_MAX);
   STATIC_ASSERT(Elements(fragAttribs) == FRAG_ATTRIB_MAX);
   assert(strcmp(vertAttribs[VERT_ATTRIB_TEX0], "vertex.texcoord[0]") == 0);
   assert(strcmp(vertAttribs[VERT_ATTRIB_GENERIC15], "vertex.attrib[15]") == 0);
   assert(strcmp(fragAttribs[FRAG_ATTRIB_TEX0], "fragment.texcoord[0]") == 0);
   assert(strcmp(fragAttribs[FRAG_ATTRIB_VAR0+15], "fragment.varying[15]") == 0);

   if (progType == GL_VERTEX_PROGRAM_ARB) {
      assert(index < Elements(vertAttribs));
      return vertAttribs[index];
   }
   else {
      assert(progType == GL_FRAGMENT_PROGRAM_ARB);
      assert(index < Elements(fragAttribs));
      return fragAttribs[index];
   }
}


/**
 * Print a vertex program's InputsRead field in human-readable format.
 * For debugging.
 */
void
_mesa_print_vp_inputs(GLbitfield inputs)
{
   printf("VP Inputs 0x%x: \n", inputs);
   while (inputs) {
      GLint attr = _mesa_ffs(inputs) - 1;
      const char *name = arb_input_attrib_string(attr,
                                                 GL_VERTEX_PROGRAM_ARB);
      printf("  %d: %s\n", attr, name);
      inputs &= ~(1 << attr);
   }
}


/**
 * Print a fragment program's InputsRead field in human-readable format.
 * For debugging.
 */
void
_mesa_print_fp_inputs(GLbitfield inputs)
{
   printf("FP Inputs 0x%x: \n", inputs);
   while (inputs) {
      GLint attr = _mesa_ffs(inputs) - 1;
      const char *name = arb_input_attrib_string(attr,
                                                 GL_FRAGMENT_PROGRAM_ARB);
      printf("  %d: %s\n", attr, name);
      inputs &= ~(1 << attr);
   }
}



/**
 * Return ARB_v/f_prog-style output attrib string.
 */
static const char *
arb_output_attrib_string(GLint index, GLenum progType)
{
   /*
    * These strings should match the VERT_RESULT_x and FRAG_RESULT_x tokens.
    */
   static const char *const vertResults[] = {
      "result.position",
      "result.color.primary",
      "result.color.secondary",
      "result.fogcoord",
      "result.texcoord[0]",
      "result.texcoord[1]",
      "result.texcoord[2]",
      "result.texcoord[3]",
      "result.texcoord[4]",
      "result.texcoord[5]",
      "result.texcoord[6]",
      "result.texcoord[7]",
      "result.pointsize", /* VERT_RESULT_PSIZ */
      "result.(thirteen)", /* VERT_RESULT_BFC0 */
      "result.(fourteen)", /* VERT_RESULT_BFC1 */
      "result.(fifteen)", /* VERT_RESULT_EDGE */
      "result.(sixteen)", /* VERT_RESULT_CLIP_VERTEX */
      "result.(seventeen)", /* VERT_RESULT_CLIP_DIST0 */
      "result.(eighteen)", /* VERT_RESULT_CLIP_DIST1 */
      "result.varying[0]",
      "result.varying[1]",
      "result.varying[2]",
      "result.varying[3]",
      "result.varying[4]",
      "result.varying[5]",
      "result.varying[6]",
      "result.varying[7]",
      "result.varying[8]",
      "result.varying[9]",
      "result.varying[10]",
      "result.varying[11]",
      "result.varying[12]",
      "result.varying[13]",
      "result.varying[14]",
      "result.varying[15]" /* MAX_VARYING = 16 */
   };
   static const char *const fragResults[] = {
      "result.depth", /* FRAG_RESULT_DEPTH */
      "result.(one)", /* FRAG_RESULT_STENCIL */
      "result.color", /* FRAG_RESULT_COLOR */
      "result.color[0]", /* FRAG_RESULT_DATA0 (named for GLSL's gl_FragData) */
      "result.color[1]",
      "result.color[2]",
      "result.color[3]",
      "result.color[4]",
      "result.color[5]",
      "result.color[6]",
      "result.color[7]" /* MAX_DRAW_BUFFERS = 8 */
   };

   /* sanity checks */
   STATIC_ASSERT(Elements(vertResults) == VERT_RESULT_MAX);
   STATIC_ASSERT(Elements(fragResults) == FRAG_RESULT_MAX);
   assert(strcmp(vertResults[VERT_RESULT_HPOS], "result.position") == 0);
   assert(strcmp(vertResults[VERT_RESULT_VAR0], "result.varying[0]") == 0);
   assert(strcmp(fragResults[FRAG_RESULT_DATA0], "result.color[0]") == 0);

   if (progType == GL_VERTEX_PROGRAM_ARB) {
      assert(index < Elements(vertResults));
      return vertResults[index];
   }
   else {
      assert(progType == GL_FRAGMENT_PROGRAM_ARB);
      assert(index < Elements(fragResults));
      return fragResults[index];
   }
}


/**
 * Return string representation of the given register.
 * Note that some types of registers (like PROGRAM_UNIFORM) aren't defined
 * by the ARB/NV program languages so we've taken some liberties here.
 * \param f  the register file (PROGRAM_INPUT, PROGRAM_TEMPORARY, etc)
 * \param index  number of the register in the register file
 * \param mode  the output format/mode/style
 * \param prog  pointer to containing program
 */
static const char *
reg_string(gl_register_file f, GLint index, gl_prog_print_mode mode,
           GLboolean relAddr, const struct gl_program *prog,
           GLboolean hasIndex2, GLboolean relAddr2, GLint index2)
{
   static char str[100];
   const char *addr = relAddr ? "ADDR+" : "";

   str[0] = 0;

   switch (mode) {
   case PROG_PRINT_DEBUG:
      sprintf(str, "%s[%s%d]",
              _mesa_register_file_name(f), addr, index);
      if (hasIndex2) {
         int offset = strlen(str);
         const char *addr2 = relAddr2 ? "ADDR+" : "";
         sprintf(str+offset, "[%s%d]", addr2, index2);
      }
      break;

   case PROG_PRINT_ARB:
      switch (f) {
      case PROGRAM_INPUT:
         sprintf(str, "%s", arb_input_attrib_string(index, prog->Target));
         break;
      case PROGRAM_OUTPUT:
         sprintf(str, "%s", arb_output_attrib_string(index, prog->Target));
         break;
      case PROGRAM_TEMPORARY:
         sprintf(str, "temp%d", index);
         break;
      case PROGRAM_ENV_PARAM:
         sprintf(str, "program.env[%s%d]", addr, index);
         break;
      case PROGRAM_LOCAL_PARAM:
         sprintf(str, "program.local[%s%d]", addr, index);
         break;
      case PROGRAM_VARYING: /* extension */
         sprintf(str, "varying[%s%d]", addr, index);
         break;
      case PROGRAM_CONSTANT: /* extension */
         sprintf(str, "constant[%s%d]", addr, index);
         break;
      case PROGRAM_UNIFORM: /* extension */
         sprintf(str, "uniform[%s%d]", addr, index);
         break;
      case PROGRAM_SYSTEM_VALUE:
         sprintf(str, "sysvalue[%s%d]", addr, index);
         break;
      case PROGRAM_STATE_VAR:
         {
            struct gl_program_parameter *param
               = prog->Parameters->Parameters + index;
            char *state = _mesa_program_state_string(param->StateIndexes);
            sprintf(str, "%s", state);
            free(state);
         }
         break;
      case PROGRAM_ADDRESS:
         sprintf(str, "A%d", index);
         break;
      default:
         _mesa_problem(NULL, "bad file in reg_string()");
      }
      break;

   case PROG_PRINT_NV:
      switch (f) {
      case PROGRAM_INPUT:
         if (prog->Target == GL_VERTEX_PROGRAM_ARB)
            sprintf(str, "v[%d]", index);
         else
            sprintf(str, "f[%d]", index);
         break;
      case PROGRAM_OUTPUT:
         sprintf(str, "o[%d]", index);
         break;
      case PROGRAM_TEMPORARY:
         sprintf(str, "R%d", index);
         break;
      case PROGRAM_ENV_PARAM:
         sprintf(str, "c[%d]", index);
         break;
      case PROGRAM_VARYING: /* extension */
         sprintf(str, "varying[%s%d]", addr, index);
         break;
      case PROGRAM_UNIFORM: /* extension */
         sprintf(str, "uniform[%s%d]", addr, index);
         break;
      case PROGRAM_CONSTANT: /* extension */
         sprintf(str, "constant[%s%d]", addr, index);
         break;
      case PROGRAM_STATE_VAR: /* extension */
         sprintf(str, "state[%s%d]", addr, index);
         break;
      default:
         _mesa_problem(NULL, "bad file in reg_string()");
      }
      break;

   default:
      _mesa_problem(NULL, "bad mode in reg_string()");
   }

   return str;
}


/**
 * Return a string representation of the given swizzle word.
 * If extended is true, use extended (comma-separated) format.
 * \param swizzle  the swizzle field
 * \param negateBase  4-bit negation vector
 * \param extended  if true, also allow 0, 1 values
 */
const char *
_mesa_swizzle_string(GLuint swizzle, GLuint negateMask, GLboolean extended)
{
   static const char swz[] = "xyzw01!?";  /* See SWIZZLE_x definitions */
   static char s[20];
   GLuint i = 0;

   if (!extended && swizzle == SWIZZLE_NOOP && negateMask == 0)
      return ""; /* no swizzle/negation */

   if (!extended)
      s[i++] = '.';

   if (negateMask & NEGATE_X)
      s[i++] = '-';
   s[i++] = swz[GET_SWZ(swizzle, 0)];

   if (extended) {
      s[i++] = ',';
   }

   if (negateMask & NEGATE_Y)
      s[i++] = '-';
   s[i++] = swz[GET_SWZ(swizzle, 1)];

   if (extended) {
      s[i++] = ',';
   }

   if (negateMask & NEGATE_Z)
      s[i++] = '-';
   s[i++] = swz[GET_SWZ(swizzle, 2)];

   if (extended) {
      s[i++] = ',';
   }

   if (negateMask & NEGATE_W)
      s[i++] = '-';
   s[i++] = swz[GET_SWZ(swizzle, 3)];

   s[i] = 0;
   return s;
}


void
_mesa_print_swizzle(GLuint swizzle)
{
   if (swizzle == SWIZZLE_XYZW) {
      printf(".xyzw\n");
   }
   else {
      const char *s = _mesa_swizzle_string(swizzle, 0, 0);
      printf("%s\n", s);
   }
}


const char *
_mesa_writemask_string(GLuint writeMask)
{
   static char s[10];
   GLuint i = 0;

   if (writeMask == WRITEMASK_XYZW)
      return "";

   s[i++] = '.';
   if (writeMask & WRITEMASK_X)
      s[i++] = 'x';
   if (writeMask & WRITEMASK_Y)
      s[i++] = 'y';
   if (writeMask & WRITEMASK_Z)
      s[i++] = 'z';
   if (writeMask & WRITEMASK_W)
      s[i++] = 'w';

   s[i] = 0;
   return s;
}


const char *
_mesa_condcode_string(GLuint condcode)
{
   switch (condcode) {
   case COND_GT:  return "GT";
   case COND_EQ:  return "EQ";
   case COND_LT:  return "LT";
   case COND_UN:  return "UN";
   case COND_GE:  return "GE";
   case COND_LE:  return "LE";
   case COND_NE:  return "NE";
   case COND_TR:  return "TR";
   case COND_FL:  return "FL";
   default: return "cond???";
   }
}


static void
fprint_dst_reg(FILE * f,
               const struct prog_dst_register *dstReg,
               gl_prog_print_mode mode,
               const struct gl_program *prog)
{
   fprintf(f, "%s%s",
	   reg_string((gl_register_file) dstReg->File,
		      dstReg->Index, mode, dstReg->RelAddr, prog,
                      GL_FALSE, GL_FALSE, 0),
	   _mesa_writemask_string(dstReg->WriteMask));
   
   if (dstReg->CondMask != COND_TR) {
      fprintf(f, " (%s.%s)",
	      _mesa_condcode_string(dstReg->CondMask),
	      _mesa_swizzle_string(dstReg->CondSwizzle,
				   GL_FALSE, GL_FALSE));
   }

#if 0
   fprintf(f, "%s[%d]%s",
	   _mesa_register_file_name((gl_register_file) dstReg->File),
	   dstReg->Index,
	   _mesa_writemask_string(dstReg->WriteMask));
#endif
}


static void
fprint_src_reg(FILE *f,
               const struct prog_src_register *srcReg, 
               gl_prog_print_mode mode,
               const struct gl_program *prog)
{
   const char *abs = srcReg->Abs ? "|" : "";

   fprintf(f, "%s%s%s%s",
	   abs,
	   reg_string((gl_register_file) srcReg->File,
		      srcReg->Index, mode, srcReg->RelAddr, prog,
                      srcReg->HasIndex2, srcReg->RelAddr2, srcReg->Index2),
	   _mesa_swizzle_string(srcReg->Swizzle,
				srcReg->Negate, GL_FALSE),
	   abs);
#if 0
   fprintf(f, "%s[%d]%s",
	   _mesa_register_file_name((gl_register_file) srcReg->File),
	   srcReg->Index,
	   _mesa_swizzle_string(srcReg->Swizzle,
				srcReg->Negate, GL_FALSE));
#endif
}


static void
fprint_comment(FILE *f, const struct prog_instruction *inst)
{
   if (inst->Comment)
      fprintf(f, ";  # %s\n", inst->Comment);
   else
      fprintf(f, ";\n");
}


void
_mesa_fprint_alu_instruction(FILE *f,
			     const struct prog_instruction *inst,
			     const char *opcode_string, GLuint numRegs,
			     gl_prog_print_mode mode,
			     const struct gl_program *prog)
{
   GLuint j;

   fprintf(f, "%s", opcode_string);
   if (inst->CondUpdate)
      fprintf(f, ".C");

   /* frag prog only */
   if (inst->SaturateMode == SATURATE_ZERO_ONE)
      fprintf(f, "_SAT");

   fprintf(f, " ");
   if (inst->DstReg.File != PROGRAM_UNDEFINED) {
      fprint_dst_reg(f, &inst->DstReg, mode, prog);
   }
   else {
      fprintf(f, " ???");
   }

   if (numRegs > 0)
      fprintf(f, ", ");

   for (j = 0; j < numRegs; j++) {
      fprint_src_reg(f, inst->SrcReg + j, mode, prog);
      if (j + 1 < numRegs)
	 fprintf(f, ", ");
   }

   fprint_comment(f, inst);
}


void
_mesa_print_alu_instruction(const struct prog_instruction *inst,
                            const char *opcode_string, GLuint numRegs)
{
   _mesa_fprint_alu_instruction(stderr, inst, opcode_string,
				numRegs, PROG_PRINT_DEBUG, NULL);
}


/**
 * Print a single vertex/fragment program instruction.
 */
GLint
_mesa_fprint_instruction_opt(FILE *f,
                            const struct prog_instruction *inst,
                            GLint indent,
                            gl_prog_print_mode mode,
                            const struct gl_program *prog)
{
   GLint i;

   if (inst->Opcode == OPCODE_ELSE ||
       inst->Opcode == OPCODE_ENDIF ||
       inst->Opcode == OPCODE_ENDLOOP ||
       inst->Opcode == OPCODE_ENDSUB) {
      indent -= 3;
   }
   for (i = 0; i < indent; i++) {
      fprintf(f, " ");
   }

   switch (inst->Opcode) {
   case OPCODE_PRINT:
      fprintf(f, "PRINT '%s'", (char *) inst->Data);
      if (inst->SrcReg[0].File != PROGRAM_UNDEFINED) {
         fprintf(f, ", ");
         fprintf(f, "%s[%d]%s",
                 _mesa_register_file_name((gl_register_file) inst->SrcReg[0].File),
		 inst->SrcReg[0].Index,
		 _mesa_swizzle_string(inst->SrcReg[0].Swizzle,
				      inst->SrcReg[0].Negate, GL_FALSE));
      }
      if (inst->Comment)
         fprintf(f, "  # %s", inst->Comment);
      fprint_comment(f, inst);
      break;
   case OPCODE_SWZ:
      fprintf(f, "SWZ");
      if (inst->SaturateMode == SATURATE_ZERO_ONE)
         fprintf(f, "_SAT");
      fprintf(f, " ");
      fprint_dst_reg(f, &inst->DstReg, mode, prog);
      fprintf(f, ", %s[%d], %s",
	      _mesa_register_file_name((gl_register_file) inst->SrcReg[0].File),
	      inst->SrcReg[0].Index,
	      _mesa_swizzle_string(inst->SrcReg[0].Swizzle,
				   inst->SrcReg[0].Negate, GL_TRUE));
      fprint_comment(f, inst);
      break;
   case OPCODE_TEX:
   case OPCODE_TXP:
   case OPCODE_TXL:
   case OPCODE_TXB:
   case OPCODE_TXD:
      fprintf(f, "%s", _mesa_opcode_string(inst->Opcode));
      if (inst->SaturateMode == SATURATE_ZERO_ONE)
         fprintf(f, "_SAT");
      fprintf(f, " ");
      fprint_dst_reg(f, &inst->DstReg, mode, prog);
      fprintf(f, ", ");
      fprint_src_reg(f, &inst->SrcReg[0], mode, prog);
      if (inst->Opcode == OPCODE_TXD) {
         fprintf(f, ", ");
         fprint_src_reg(f, &inst->SrcReg[1], mode, prog);
         fprintf(f, ", ");
         fprint_src_reg(f, &inst->SrcReg[2], mode, prog);
      }
      fprintf(f, ", texture[%d], ", inst->TexSrcUnit);
      switch (inst->TexSrcTarget) {
      case TEXTURE_1D_INDEX:   fprintf(f, "1D");    break;
      case TEXTURE_2D_INDEX:   fprintf(f, "2D");    break;
      case TEXTURE_3D_INDEX:   fprintf(f, "3D");    break;
      case TEXTURE_CUBE_INDEX: fprintf(f, "CUBE");  break;
      case TEXTURE_RECT_INDEX: fprintf(f, "RECT");  break;
      case TEXTURE_1D_ARRAY_INDEX: fprintf(f, "1D_ARRAY"); break;
      case TEXTURE_2D_ARRAY_INDEX: fprintf(f, "2D_ARRAY"); break;
      default:
         ;
      }
      if (inst->TexShadow)
         fprintf(f, " SHADOW");
      fprint_comment(f, inst);
      break;

   case OPCODE_KIL:
      fprintf(f, "%s", _mesa_opcode_string(inst->Opcode));
      fprintf(f, " ");
      fprint_src_reg(f, &inst->SrcReg[0], mode, prog);
      fprint_comment(f, inst);
      break;
   case OPCODE_KIL_NV:
      fprintf(f, "%s", _mesa_opcode_string(inst->Opcode));
      fprintf(f, " ");
      fprintf(f, "%s.%s",
	      _mesa_condcode_string(inst->DstReg.CondMask),
	      _mesa_swizzle_string(inst->DstReg.CondSwizzle,
				   GL_FALSE, GL_FALSE));
      fprint_comment(f, inst);
      break;

   case OPCODE_ARL:
      fprintf(f, "ARL ");
      fprint_dst_reg(f, &inst->DstReg, mode, prog);
      fprintf(f, ", ");
      fprint_src_reg(f, &inst->SrcReg[0], mode, prog);
      fprint_comment(f, inst);
      break;
   case OPCODE_BRA:
      fprintf(f, "BRA %d (%s%s)",
	      inst->BranchTarget,
	      _mesa_condcode_string(inst->DstReg.CondMask),
	      _mesa_swizzle_string(inst->DstReg.CondSwizzle, 0, GL_FALSE));
      fprint_comment(f, inst);
      break;
   case OPCODE_IF:
      if (inst->SrcReg[0].File != PROGRAM_UNDEFINED) {
         /* Use ordinary register */
         fprintf(f, "IF ");
         fprint_src_reg(f, &inst->SrcReg[0], mode, prog);
         fprintf(f, "; ");
      }
      else {
         /* Use cond codes */
         fprintf(f, "IF (%s%s);",
		 _mesa_condcode_string(inst->DstReg.CondMask),
		 _mesa_swizzle_string(inst->DstReg.CondSwizzle,
				      0, GL_FALSE));
      }
      fprintf(f, " # (if false, goto %d)", inst->BranchTarget);
      fprint_comment(f, inst);
      return indent + 3;
   case OPCODE_ELSE:
      fprintf(f, "ELSE; # (goto %d)\n", inst->BranchTarget);
      return indent + 3;
   case OPCODE_ENDIF:
      fprintf(f, "ENDIF;\n");
      break;
   case OPCODE_BGNLOOP:
      fprintf(f, "BGNLOOP; # (end at %d)\n", inst->BranchTarget);
      return indent + 3;
   case OPCODE_ENDLOOP:
      fprintf(f, "ENDLOOP; # (goto %d)\n", inst->BranchTarget);
      break;
   case OPCODE_BRK:
   case OPCODE_CONT:
      fprintf(f, "%s (%s%s); # (goto %d)",
	      _mesa_opcode_string(inst->Opcode),
	      _mesa_condcode_string(inst->DstReg.CondMask),
	      _mesa_swizzle_string(inst->DstReg.CondSwizzle, 0, GL_FALSE),
	      inst->BranchTarget);
      fprint_comment(f, inst);
      break;

   case OPCODE_BGNSUB:
      if (mode == PROG_PRINT_NV) {
         fprintf(f, "%s:\n", inst->Comment); /* comment is label */
         return indent;
      }
      else {
         fprintf(f, "BGNSUB");
         fprint_comment(f, inst);
         return indent + 3;
      }
   case OPCODE_ENDSUB:
      if (mode == PROG_PRINT_DEBUG) {
         fprintf(f, "ENDSUB");
         fprint_comment(f, inst);
      }
      break;
   case OPCODE_CAL:
      if (mode == PROG_PRINT_NV) {
         fprintf(f, "CAL %s;  # (goto %d)\n", inst->Comment, inst->BranchTarget);
      }
      else {
         fprintf(f, "CAL %u", inst->BranchTarget);
         fprint_comment(f, inst);
      }
      break;
   case OPCODE_RET:
      fprintf(f, "RET (%s%s)",
	      _mesa_condcode_string(inst->DstReg.CondMask),
	      _mesa_swizzle_string(inst->DstReg.CondSwizzle, 0, GL_FALSE));
      fprint_comment(f, inst);
      break;

   case OPCODE_END:
      fprintf(f, "END\n");
      break;
   case OPCODE_NOP:
      if (mode == PROG_PRINT_DEBUG) {
         fprintf(f, "NOP");
         fprint_comment(f, inst);
      }
      else if (inst->Comment) {
         /* ARB/NV extensions don't have NOP instruction */
         fprintf(f, "# %s\n", inst->Comment);
      }
      break;
   case OPCODE_EMIT_VERTEX:
      fprintf(f, "EMIT_VERTEX\n");
      break;
   case OPCODE_END_PRIMITIVE:
      fprintf(f, "END_PRIMITIVE\n");
      break;
   /* XXX may need other special-case instructions */
   default:
      if (inst->Opcode < MAX_OPCODE) {
         /* typical alu instruction */
         _mesa_fprint_alu_instruction(f, inst,
				      _mesa_opcode_string(inst->Opcode),
				      _mesa_num_inst_src_regs(inst->Opcode),
				      mode, prog);
      }
      else {
         _mesa_fprint_alu_instruction(f, inst,
				      _mesa_opcode_string(inst->Opcode),
				      3/*_mesa_num_inst_src_regs(inst->Opcode)*/,
				      mode, prog);
      }
      break;
   }
   return indent;
}


GLint
_mesa_print_instruction_opt(const struct prog_instruction *inst,
                            GLint indent,
                            gl_prog_print_mode mode,
                            const struct gl_program *prog)
{
   return _mesa_fprint_instruction_opt(stderr, inst, indent, mode, prog);
}


void
_mesa_print_instruction(const struct prog_instruction *inst)
{
   /* note: 4th param should be ignored for PROG_PRINT_DEBUG */
   _mesa_fprint_instruction_opt(stderr, inst, 0, PROG_PRINT_DEBUG, NULL);
}



/**
 * Print program, with options.
 */
void
_mesa_fprint_program_opt(FILE *f,
                         const struct gl_program *prog,
                         gl_prog_print_mode mode,
                         GLboolean lineNumbers)
{
   GLuint i, indent = 0;

   switch (prog->Target) {
   case GL_VERTEX_PROGRAM_ARB:
      if (mode == PROG_PRINT_ARB)
         fprintf(f, "!!ARBvp1.0\n");
      else if (mode == PROG_PRINT_NV)
         fprintf(f, "!!VP1.0\n");
      else
         fprintf(f, "# Vertex Program/Shader %u\n", prog->Id);
      break;
   case GL_FRAGMENT_PROGRAM_ARB:
   case GL_FRAGMENT_PROGRAM_NV:
      if (mode == PROG_PRINT_ARB)
         fprintf(f, "!!ARBfp1.0\n");
      else if (mode == PROG_PRINT_NV)
         fprintf(f, "!!FP1.0\n");
      else
         fprintf(f, "# Fragment Program/Shader %u\n", prog->Id);
      break;
   case MESA_GEOMETRY_PROGRAM:
      fprintf(f, "# Geometry Shader\n");
   }

   for (i = 0; i < prog->NumInstructions; i++) {
      if (lineNumbers)
         fprintf(f, "%3d: ", i);
      indent = _mesa_fprint_instruction_opt(f, prog->Instructions + i,
                                           indent, mode, prog);
   }
}


/**
 * Print program to stderr, default options.
 */
void
_mesa_print_program(const struct gl_program *prog)
{
   _mesa_fprint_program_opt(stderr, prog, PROG_PRINT_DEBUG, GL_TRUE);
}


/**
 * Return binary representation of 64-bit value (as a string).
 * Insert a comma to separate each group of 8 bits.
 * Note we return a pointer to local static storage so this is not
 * re-entrant, etc.
 * XXX move to imports.[ch] if useful elsewhere.
 */
static const char *
binary(GLbitfield64 val)
{
   static char buf[80];
   GLint i, len = 0;
   for (i = 63; i >= 0; --i) {
      if (val & (BITFIELD64_BIT(i)))
         buf[len++] = '1';
      else if (len > 0 || i == 0)
         buf[len++] = '0';
      if (len > 0 && ((i-1) % 8) == 7)
         buf[len++] = ',';
   }
   buf[len] = '\0';
   return buf;
}


/**
 * Print all of a program's parameters/fields to given file.
 */
static void
_mesa_fprint_program_parameters(FILE *f,
                                struct gl_context *ctx,
                                const struct gl_program *prog)
{
   GLuint i;

   fprintf(f, "InputsRead: 0x%llx (0b%s)\n",
           (unsigned long long) prog->InputsRead, binary(prog->InputsRead));
   fprintf(f, "OutputsWritten: 0x%llx (0b%s)\n",
                 (unsigned long long)prog->OutputsWritten, 
		 binary(prog->OutputsWritten));
   fprintf(f, "NumInstructions=%d\n", prog->NumInstructions);
   fprintf(f, "NumTemporaries=%d\n", prog->NumTemporaries);
   fprintf(f, "NumParameters=%d\n", prog->NumParameters);
   fprintf(f, "NumAttributes=%d\n", prog->NumAttributes);
   fprintf(f, "NumAddressRegs=%d\n", prog->NumAddressRegs);
   fprintf(f, "IndirectRegisterFiles: 0x%x (0b%s)\n",
           prog->IndirectRegisterFiles, binary(prog->IndirectRegisterFiles));
   fprintf(f, "SamplersUsed: 0x%x (0b%s)\n",
                 prog->SamplersUsed, binary(prog->SamplersUsed));
   fprintf(f, "Samplers=[ ");
   for (i = 0; i < MAX_SAMPLERS; i++) {
      fprintf(f, "%d ", prog->SamplerUnits[i]);
   }
   fprintf(f, "]\n");

   _mesa_load_state_parameters(ctx, prog->Parameters);

#if 0
   fprintf(f, "Local Params:\n");
   for (i = 0; i < MAX_PROGRAM_LOCAL_PARAMS; i++){
      const GLfloat *p = prog->LocalParams[i];
      fprintf(f, "%2d: %f, %f, %f, %f\n", i, p[0], p[1], p[2], p[3]);
   }
#endif	
   _mesa_print_parameter_list(prog->Parameters);
}


/**
 * Print all of a program's parameters/fields to stderr.
 */
void
_mesa_print_program_parameters(struct gl_context *ctx, const struct gl_program *prog)
{
   _mesa_fprint_program_parameters(stderr, ctx, prog);
}


/**
 * Print a program parameter list to given file.
 */
static void
_mesa_fprint_parameter_list(FILE *f,
                            const struct gl_program_parameter_list *list)
{
   GLuint i;

   if (!list)
      return;

   if (0)
      fprintf(f, "param list %p\n", (void *) list);
   fprintf(f, "dirty state flags: 0x%x\n", list->StateFlags);
   for (i = 0; i < list->NumParameters; i++){
      struct gl_program_parameter *param = list->Parameters + i;
      const GLfloat *v = (GLfloat *) list->ParameterValues[i];
      fprintf(f, "param[%d] sz=%d %s %s = {%.3g, %.3g, %.3g, %.3g}",
	      i, param->Size,
	      _mesa_register_file_name(list->Parameters[i].Type),
	      param->Name, v[0], v[1], v[2], v[3]);
      if (param->Flags & PROG_PARAM_BIT_CENTROID)
         fprintf(f, " Centroid");
      if (param->Flags & PROG_PARAM_BIT_INVARIANT)
         fprintf(f, " Invariant");
      if (param->Flags & PROG_PARAM_BIT_FLAT)
         fprintf(f, " Flat");
      if (param->Flags & PROG_PARAM_BIT_LINEAR)
         fprintf(f, " Linear");
      fprintf(f, "\n");
   }
}


/**
 * Print a program parameter list to stderr.
 */
void
_mesa_print_parameter_list(const struct gl_program_parameter_list *list)
{
   _mesa_fprint_parameter_list(stderr, list);
}


/**
 * Write shader and associated info to a file.
 */
void
_mesa_write_shader_to_file(const struct gl_shader *shader)
{
   const char *type;
   char filename[100];
   FILE *f;

   if (shader->Type == GL_FRAGMENT_SHADER)
      type = "frag";
   else if (shader->Type == GL_VERTEX_SHADER)
      type = "vert";
   else
      type = "geom";

   _mesa_snprintf(filename, sizeof(filename), "shader_%u.%s", shader->Name, type);
   f = fopen(filename, "w");
   if (!f) {
      fprintf(stderr, "Unable to open %s for writing\n", filename);
      return;
   }

   fprintf(f, "/* Shader %u source, checksum %u */\n", shader->Name, shader->SourceChecksum);
   fputs(shader->Source, f);
   fprintf(f, "\n");

   fprintf(f, "/* Compile status: %s */\n",
           shader->CompileStatus ? "ok" : "fail");
   fprintf(f, "/* Log Info: */\n");
   if (shader->InfoLog) {
      fputs(shader->InfoLog, f);
   }
   if (shader->CompileStatus && shader->Program) {
      fprintf(f, "/* GPU code */\n");
      fprintf(f, "/*\n");
      _mesa_fprint_program_opt(f, shader->Program, PROG_PRINT_DEBUG, GL_TRUE);
      fprintf(f, "*/\n");
      fprintf(f, "/* Parameters / constants */\n");
      fprintf(f, "/*\n");
      _mesa_fprint_parameter_list(f, shader->Program->Parameters);
      fprintf(f, "*/\n");
   }

   fclose(f);
}


/**
 * Append the shader's uniform info/values to the shader log file.
 * The log file will typically have been created by the
 * _mesa_write_shader_to_file function.
 */
void
_mesa_append_uniforms_to_file(const struct gl_shader *shader)
{
   const struct gl_program *const prog = shader->Program;
   const char *type;
   char filename[100];
   FILE *f;

   if (shader->Type == GL_FRAGMENT_SHADER)
      type = "frag";
   else
      type = "vert";

   _mesa_snprintf(filename, sizeof(filename), "shader_%u.%s", shader->Name, type);
   f = fopen(filename, "a"); /* append */
   if (!f) {
      fprintf(stderr, "Unable to open %s for appending\n", filename);
      return;
   }

   fprintf(f, "/* First-draw parameters / constants */\n");
   fprintf(f, "/*\n");
   _mesa_fprint_parameter_list(f, prog->Parameters);
   fprintf(f, "*/\n");

   fclose(f);
}

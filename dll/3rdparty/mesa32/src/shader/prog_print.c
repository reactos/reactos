/*
 * Mesa 3-D graphics library
 * Version:  7.3
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
static const char *
file_string(enum register_file f, gl_prog_print_mode mode)
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
   case PROGRAM_UNDEFINED:
      return "UNDEFINED";
   default:
      return "Unknown program file!";
   }
}


/**
 * Return ARB_v/f_prog-style input attrib string.
 */
static const char *
arb_input_attrib_string(GLint index, GLenum progType)
{
   const char *vertAttribs[] = {
      "vertex.position",
      "vertex.weight",
      "vertex.normal",
      "vertex.color.primary",
      "vertex.color.secondary",
      "vertex.fogcoord",
      "vertex.(six)",
      "vertex.(seven)",
      "vertex.texcoord[0]",
      "vertex.texcoord[1]",
      "vertex.texcoord[2]",
      "vertex.texcoord[3]",
      "vertex.texcoord[4]",
      "vertex.texcoord[5]",
      "vertex.texcoord[6]",
      "vertex.texcoord[7]",
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
      "vertex.attrib[15]"
   };
   const char *fragAttribs[] = {
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
      "fragment.varying[0]",
      "fragment.varying[1]",
      "fragment.varying[2]",
      "fragment.varying[3]",
      "fragment.varying[4]",
      "fragment.varying[5]",
      "fragment.varying[6]",
      "fragment.varying[7]"
   };

   if (progType == GL_VERTEX_PROGRAM_ARB) {
      assert(index < sizeof(vertAttribs) / sizeof(vertAttribs[0]));
      return vertAttribs[index];
   }
   else {
      assert(index < sizeof(fragAttribs) / sizeof(fragAttribs[0]));
      return fragAttribs[index];
   }
}


/**
 * Return ARB_v/f_prog-style output attrib string.
 */
static const char *
arb_output_attrib_string(GLint index, GLenum progType)
{
   const char *vertResults[] = {
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
      "result.varying[0]",
      "result.varying[1]",
      "result.varying[2]",
      "result.varying[3]",
      "result.varying[4]",
      "result.varying[5]",
      "result.varying[6]",
      "result.varying[7]"
   };
   const char *fragResults[] = {
      "result.color",
      "result.depth"
   };

   if (progType == GL_VERTEX_PROGRAM_ARB) {
      assert(index < sizeof(vertResults) / sizeof(vertResults[0]));
      return vertResults[index];
   }
   else {
      assert(index < sizeof(fragResults) / sizeof(fragResults[0]));
      return fragResults[index];
   }
}


/**
 * Return string representation of the given register.
 * Note that some types of registers (like PROGRAM_UNIFORM) aren't defined
 * by the ARB/NV program languages so we've taken some liberties here.
 * \param file  the register file (PROGRAM_INPUT, PROGRAM_TEMPORARY, etc)
 * \param index  number of the register in the register file
 * \param mode  the output format/mode/style
 * \param prog  pointer to containing program
 */
static const char *
reg_string(enum register_file f, GLint index, gl_prog_print_mode mode,
           GLboolean relAddr, const struct gl_program *prog)
{
   static char str[100];

   str[0] = 0;

   switch (mode) {
   case PROG_PRINT_DEBUG:
      if (relAddr)
         sprintf(str, "%s[ADDR+%d]", file_string(f, mode), index);
      else
         sprintf(str, "%s[%d]", file_string(f, mode), index);
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
         sprintf(str, "program.env[%d]", index);
         break;
      case PROGRAM_LOCAL_PARAM:
         sprintf(str, "program.local[%d]", index);
         break;
      case PROGRAM_VARYING: /* extension */
         sprintf(str, "varying[%d]", index);
         break;
      case PROGRAM_CONSTANT: /* extension */
         sprintf(str, "constant[%d]", index);
         break;
      case PROGRAM_UNIFORM: /* extension */
         sprintf(str, "uniform[%d]", index);
         break;
      case PROGRAM_STATE_VAR:
         {
            struct gl_program_parameter *param
               = prog->Parameters->Parameters + index;
            sprintf(str, _mesa_program_state_string(param->StateIndexes));
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
         sprintf(str, "varying[%d]", index);
         break;
      case PROGRAM_UNIFORM: /* extension */
         sprintf(str, "uniform[%d]", index);
         break;
      case PROGRAM_CONSTANT: /* extension */
         sprintf(str, "constant[%d]", index);
         break;
      case PROGRAM_STATE_VAR: /* extension */
         sprintf(str, "state[%d]", index);
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
_mesa_swizzle_string(GLuint swizzle, GLuint negateBase, GLboolean extended)
{
   static const char swz[] = "xyzw01!?";  /* See SWIZZLE_x definitions */
   static char s[20];
   GLuint i = 0;

   if (!extended && swizzle == SWIZZLE_NOOP && negateBase == 0)
      return ""; /* no swizzle/negation */

   if (!extended)
      s[i++] = '.';

   if (negateBase & NEGATE_X)
      s[i++] = '-';
   s[i++] = swz[GET_SWZ(swizzle, 0)];

   if (extended) {
      s[i++] = ',';
   }

   if (negateBase & NEGATE_Y)
      s[i++] = '-';
   s[i++] = swz[GET_SWZ(swizzle, 1)];

   if (extended) {
      s[i++] = ',';
   }

   if (negateBase & NEGATE_Z)
      s[i++] = '-';
   s[i++] = swz[GET_SWZ(swizzle, 2)];

   if (extended) {
      s[i++] = ',';
   }

   if (negateBase & NEGATE_W)
      s[i++] = '-';
   s[i++] = swz[GET_SWZ(swizzle, 3)];

   s[i] = 0;
   return s;
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
print_dst_reg(const struct prog_dst_register *dstReg, gl_prog_print_mode mode,
              const struct gl_program *prog)
{
   _mesa_printf("%s%s",
                reg_string((enum register_file) dstReg->File,
                           dstReg->Index, mode, dstReg->RelAddr, prog),
                _mesa_writemask_string(dstReg->WriteMask));

   if (dstReg->CondMask != COND_TR) {
      _mesa_printf(" (%s.%s)",
                   _mesa_condcode_string(dstReg->CondMask),
                   _mesa_swizzle_string(dstReg->CondSwizzle, GL_FALSE, GL_FALSE));
   }

#if 0
   _mesa_printf("%s[%d]%s",
                file_string((enum register_file) dstReg->File, mode),
                dstReg->Index,
                _mesa_writemask_string(dstReg->WriteMask));
#endif
}

static void
print_src_reg(const struct prog_src_register *srcReg, gl_prog_print_mode mode,
              const struct gl_program *prog)
{
   _mesa_printf("%s%s",
                reg_string((enum register_file) srcReg->File,
                           srcReg->Index, mode, srcReg->RelAddr, prog),
                _mesa_swizzle_string(srcReg->Swizzle,
                                     srcReg->NegateBase, GL_FALSE));
#if 0
   _mesa_printf("%s[%d]%s",
                file_string((enum register_file) srcReg->File, mode),
                srcReg->Index,
                _mesa_swizzle_string(srcReg->Swizzle,
                               srcReg->NegateBase, GL_FALSE));
#endif
}

static void
print_comment(const struct prog_instruction *inst)
{
   if (inst->Comment)
      _mesa_printf(";  # %s\n", inst->Comment);
   else
      _mesa_printf(";\n");
}


static void
print_alu_instruction(const struct prog_instruction *inst,
                      const char *opcode_string, GLuint numRegs,
                      gl_prog_print_mode mode,
                      const struct gl_program *prog)
{
   GLuint j;

   _mesa_printf("%s", opcode_string);
   if (inst->CondUpdate)
      _mesa_printf(".C");

   /* frag prog only */
   if (inst->SaturateMode == SATURATE_ZERO_ONE)
      _mesa_printf("_SAT");

   _mesa_printf(" ");
   if (inst->DstReg.File != PROGRAM_UNDEFINED) {
      print_dst_reg(&inst->DstReg, mode, prog);
   }
   else {
      _mesa_printf(" ???");
   }

   if (numRegs > 0)
      _mesa_printf(", ");

   for (j = 0; j < numRegs; j++) {
      print_src_reg(inst->SrcReg + j, mode, prog);
      if (j + 1 < numRegs)
	 _mesa_printf(", ");
   }

   print_comment(inst);
}


void
_mesa_print_alu_instruction(const struct prog_instruction *inst,
                            const char *opcode_string, GLuint numRegs)
{
   print_alu_instruction(inst, opcode_string, numRegs, PROG_PRINT_DEBUG, NULL);
}


void
_mesa_print_instruction(const struct prog_instruction *inst)
{
   /* note: 4th param should be ignored for PROG_PRINT_DEBUG */
   _mesa_print_instruction_opt(inst, 0, PROG_PRINT_DEBUG, NULL);
}


/**
 * Print a single vertex/fragment program instruction.
 */
GLint
_mesa_print_instruction_opt(const struct prog_instruction *inst, GLint indent,
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
      _mesa_printf(" ");
   }

   switch (inst->Opcode) {
   case OPCODE_PRINT:
      _mesa_printf("PRINT '%s'", inst->Data);
      if (inst->SrcReg[0].File != PROGRAM_UNDEFINED) {
         _mesa_printf(", ");
         _mesa_printf("%s[%d]%s",
                      file_string((enum register_file) inst->SrcReg[0].File,
                                  mode),
                      inst->SrcReg[0].Index,
                      _mesa_swizzle_string(inst->SrcReg[0].Swizzle,
                                           inst->SrcReg[0].NegateBase, GL_FALSE));
      }
      if (inst->Comment)
         _mesa_printf("  # %s", inst->Comment);
      print_comment(inst);
      break;
   case OPCODE_SWZ:
      _mesa_printf("SWZ");
      if (inst->SaturateMode == SATURATE_ZERO_ONE)
         _mesa_printf("_SAT");
      _mesa_printf(" ");
      print_dst_reg(&inst->DstReg, mode, prog);
      _mesa_printf(", %s[%d], %s",
                   file_string((enum register_file) inst->SrcReg[0].File,
                               mode),
                   inst->SrcReg[0].Index,
                   _mesa_swizzle_string(inst->SrcReg[0].Swizzle,
                                        inst->SrcReg[0].NegateBase, GL_TRUE));
      print_comment(inst);
      break;
   case OPCODE_TEX:
   case OPCODE_TXP:
   case OPCODE_TXL:
   case OPCODE_TXB:
      _mesa_printf("%s", _mesa_opcode_string(inst->Opcode));
      if (inst->SaturateMode == SATURATE_ZERO_ONE)
         _mesa_printf("_SAT");
      _mesa_printf(" ");
      print_dst_reg(&inst->DstReg, mode, prog);
      _mesa_printf(", ");
      print_src_reg(&inst->SrcReg[0], mode, prog);
      _mesa_printf(", texture[%d], ", inst->TexSrcUnit);
      switch (inst->TexSrcTarget) {
      case TEXTURE_1D_INDEX:   _mesa_printf("1D");    break;
      case TEXTURE_2D_INDEX:   _mesa_printf("2D");    break;
      case TEXTURE_3D_INDEX:   _mesa_printf("3D");    break;
      case TEXTURE_CUBE_INDEX: _mesa_printf("CUBE");  break;
      case TEXTURE_RECT_INDEX: _mesa_printf("RECT");  break;
      default:
         ;
      }
      print_comment(inst);
      break;

   case OPCODE_KIL:
      _mesa_printf("%s", _mesa_opcode_string(inst->Opcode));
      _mesa_printf(" ");
      print_src_reg(&inst->SrcReg[0], mode, prog);
      print_comment(inst);
      break;
   case OPCODE_KIL_NV:
      _mesa_printf("%s", _mesa_opcode_string(inst->Opcode));
      _mesa_printf(" ");
      _mesa_printf("%s.%s",
                   _mesa_condcode_string(inst->DstReg.CondMask),
                   _mesa_swizzle_string(inst->DstReg.CondSwizzle,
                                        GL_FALSE, GL_FALSE));
      print_comment(inst);
      break;

   case OPCODE_ARL:
      _mesa_printf("ARL ");
      print_dst_reg(&inst->DstReg, mode, prog);
      _mesa_printf(", ");
      print_src_reg(&inst->SrcReg[0], mode, prog);
      print_comment(inst);
      break;
   case OPCODE_BRA:
      _mesa_printf("BRA %d (%s%s)",
                   inst->BranchTarget,
                   _mesa_condcode_string(inst->DstReg.CondMask),
                   _mesa_swizzle_string(inst->DstReg.CondSwizzle, 0, GL_FALSE));
      print_comment(inst);
      break;
   case OPCODE_IF:
      if (inst->SrcReg[0].File != PROGRAM_UNDEFINED) {
         /* Use ordinary register */
         _mesa_printf("IF ");
         print_src_reg(&inst->SrcReg[0], mode, prog);
         _mesa_printf("; ");
      }
      else {
         /* Use cond codes */
         _mesa_printf("IF (%s%s);",
                      _mesa_condcode_string(inst->DstReg.CondMask),
                      _mesa_swizzle_string(inst->DstReg.CondSwizzle,
                                           0, GL_FALSE));
      }
      _mesa_printf(" # (if false, goto %d)", inst->BranchTarget);
      print_comment(inst);
      return indent + 3;
   case OPCODE_ELSE:
      _mesa_printf("ELSE; # (goto %d)\n", inst->BranchTarget);
      return indent + 3;
   case OPCODE_ENDIF:
      _mesa_printf("ENDIF;\n");
      break;
   case OPCODE_BGNLOOP:
      _mesa_printf("BGNLOOP; # (end at %d)\n", inst->BranchTarget);
      return indent + 3;
   case OPCODE_ENDLOOP:
      _mesa_printf("ENDLOOP; # (goto %d)\n", inst->BranchTarget);
      break;
   case OPCODE_BRK:
   case OPCODE_CONT:
      _mesa_printf("%s (%s%s); # (goto %d)",
                   _mesa_opcode_string(inst->Opcode),
                   _mesa_condcode_string(inst->DstReg.CondMask),
                   _mesa_swizzle_string(inst->DstReg.CondSwizzle, 0, GL_FALSE),
                   inst->BranchTarget);
      print_comment(inst);
      break;

   case OPCODE_BGNSUB:
      if (mode == PROG_PRINT_NV) {
         _mesa_printf("%s:\n", inst->Comment); /* comment is label */
         return indent;
      }
      else {
         _mesa_printf("BGNSUB");
         print_comment(inst);
         return indent + 3;
      }
   case OPCODE_ENDSUB:
      if (mode == PROG_PRINT_DEBUG) {
         _mesa_printf("ENDSUB");
         print_comment(inst);
      }
      break;
   case OPCODE_CAL:
      if (mode == PROG_PRINT_NV) {
         _mesa_printf("CAL %s;  # (goto %d)\n", inst->Comment, inst->BranchTarget);
      }
      else {
         _mesa_printf("CAL %u", inst->BranchTarget);
         print_comment(inst);
      }
      break;
   case OPCODE_RET:
      _mesa_printf("RET (%s%s)",
                   _mesa_condcode_string(inst->DstReg.CondMask),
                   _mesa_swizzle_string(inst->DstReg.CondSwizzle, 0, GL_FALSE));
      print_comment(inst);
      break;

   case OPCODE_END:
      _mesa_printf("END\n");
      break;
   case OPCODE_NOP:
      if (mode == PROG_PRINT_DEBUG) {
         _mesa_printf("NOP");
         print_comment(inst);
      }
      else if (inst->Comment) {
         /* ARB/NV extensions don't have NOP instruction */
         _mesa_printf("# %s\n", inst->Comment);
      }
      break;
   /* XXX may need other special-case instructions */
   default:
      /* typical alu instruction */
      print_alu_instruction(inst,
                            _mesa_opcode_string(inst->Opcode),
                            _mesa_num_inst_src_regs(inst->Opcode),
                            mode, prog);
      break;
   }
   return indent;
}


/**
 * Print program to stdout, default options.
 */
void
_mesa_print_program(const struct gl_program *prog)
{
   _mesa_print_program_opt(prog, PROG_PRINT_DEBUG, GL_TRUE);
}


/**
 * Print program, with options.
 */
void
_mesa_print_program_opt(const struct gl_program *prog,
                        gl_prog_print_mode mode,
                        GLboolean lineNumbers)
{
   GLuint i, indent = 0;

   switch (prog->Target) {
   case GL_VERTEX_PROGRAM_ARB:
      if (mode == PROG_PRINT_ARB)
         _mesa_printf("!!ARBvp1.0\n");
      else if (mode == PROG_PRINT_NV)
         _mesa_printf("!!VP1.0\n");
      else
         _mesa_printf("# Vertex Program/Shader\n");
      break;
   case GL_FRAGMENT_PROGRAM_ARB:
   case GL_FRAGMENT_PROGRAM_NV:
      if (mode == PROG_PRINT_ARB)
         _mesa_printf("!!ARBfp1.0\n");
      else if (mode == PROG_PRINT_NV)
         _mesa_printf("!!FP1.0\n");
      else
         _mesa_printf("# Fragment Program/Shader\n");
      break;
   }

   for (i = 0; i < prog->NumInstructions; i++) {
      if (lineNumbers)
         _mesa_printf("%3d: ", i);
      indent = _mesa_print_instruction_opt(prog->Instructions + i,
                                           indent, mode, prog);
   }
}


/**
 * Print all of a program's parameters.
 */
void
_mesa_print_program_parameters(GLcontext *ctx, const struct gl_program *prog)
{
   GLuint i;

   _mesa_printf("InputsRead: 0x%x\n", prog->InputsRead);
   _mesa_printf("OutputsWritten: 0x%x\n", prog->OutputsWritten);
   _mesa_printf("NumInstructions=%d\n", prog->NumInstructions);
   _mesa_printf("NumTemporaries=%d\n", prog->NumTemporaries);
   _mesa_printf("NumParameters=%d\n", prog->NumParameters);
   _mesa_printf("NumAttributes=%d\n", prog->NumAttributes);
   _mesa_printf("NumAddressRegs=%d\n", prog->NumAddressRegs);
   _mesa_printf("Samplers=[ ");
   for (i = 0; i < MAX_SAMPLERS; i++) {
      _mesa_printf("%d ", prog->SamplerUnits[i]);
   }
   _mesa_printf("]\n");

   _mesa_load_state_parameters(ctx, prog->Parameters);

#if 0
   _mesa_printf("Local Params:\n");
   for (i = 0; i < MAX_PROGRAM_LOCAL_PARAMS; i++){
      const GLfloat *p = prog->LocalParams[i];
      _mesa_printf("%2d: %f, %f, %f, %f\n", i, p[0], p[1], p[2], p[3]);
   }
#endif	
   _mesa_print_parameter_list(prog->Parameters);
}


void
_mesa_print_parameter_list(const struct gl_program_parameter_list *list)
{
   const gl_prog_print_mode mode = PROG_PRINT_DEBUG;
   GLuint i;

   if (!list)
      return;

   _mesa_printf("param list %p\n", (void *) list);
   for (i = 0; i < list->NumParameters; i++){
      struct gl_program_parameter *param = list->Parameters + i;
      const GLfloat *v = list->ParameterValues[i];
      _mesa_printf("param[%d] sz=%d %s %s = {%.3g, %.3g, %.3g, %.3g}",
                   i, param->Size,
                   file_string(list->Parameters[i].Type, mode),
                   param->Name, v[0], v[1], v[2], v[3]);
      if (param->Flags & PROG_PARAM_BIT_CENTROID)
         _mesa_printf(" Centroid");
      if (param->Flags & PROG_PARAM_BIT_INVARIANT)
         _mesa_printf(" Invariant");
      if (param->Flags & PROG_PARAM_BIT_FLAT)
         _mesa_printf(" Flat");
      if (param->Flags & PROG_PARAM_BIT_LINEAR)
         _mesa_printf(" Linear");
      _mesa_printf("\n");
   }
}

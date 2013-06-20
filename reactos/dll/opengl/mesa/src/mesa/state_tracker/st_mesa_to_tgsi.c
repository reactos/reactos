/**************************************************************************
 * 
 * Copyright 2007-2008 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

/*
 * \author
 * Michal Krol,
 * Keith Whitwell
 */

#include "pipe/p_compiler.h"
#include "pipe/p_context.h"
#include "pipe/p_screen.h"
#include "pipe/p_shader_tokens.h"
#include "pipe/p_state.h"
#include "tgsi/tgsi_ureg.h"
#include "st_mesa_to_tgsi.h"
#include "st_context.h"
#include "program/prog_instruction.h"
#include "program/prog_parameter.h"
#include "util/u_debug.h"
#include "util/u_math.h"
#include "util/u_memory.h"


#define PROGRAM_ANY_CONST ((1 << PROGRAM_LOCAL_PARAM) |  \
                           (1 << PROGRAM_ENV_PARAM) |    \
                           (1 << PROGRAM_STATE_VAR) |    \
                           (1 << PROGRAM_NAMED_PARAM) |  \
                           (1 << PROGRAM_CONSTANT) |     \
                           (1 << PROGRAM_UNIFORM))


struct label {
   unsigned branch_target;
   unsigned token;
};


/**
 * Intermediate state used during shader translation.
 */
struct st_translate {
   struct ureg_program *ureg;

   struct ureg_dst temps[MAX_PROGRAM_TEMPS];
   struct ureg_src *constants;
   struct ureg_dst outputs[PIPE_MAX_SHADER_OUTPUTS];
   struct ureg_src inputs[PIPE_MAX_SHADER_INPUTS];
   struct ureg_dst address[1];
   struct ureg_src samplers[PIPE_MAX_SAMPLERS];
   struct ureg_src systemValues[SYSTEM_VALUE_MAX];

   /* Extra info for handling point size clamping in vertex shader */
   struct ureg_dst pointSizeResult; /**< Actual point size output register */
   struct ureg_src pointSizeConst;  /**< Point size range constant register */
   GLint pointSizeOutIndex;         /**< Temp point size output register */
   GLboolean prevInstWrotePointSize;

   const GLuint *inputMapping;
   const GLuint *outputMapping;

   /* For every instruction that contains a label (eg CALL), keep
    * details so that we can go back afterwards and emit the correct
    * tgsi instruction number for each label.
    */
   struct label *labels;
   unsigned labels_size;
   unsigned labels_count;

   /* Keep a record of the tgsi instruction number that each mesa
    * instruction starts at, will be used to fix up labels after
    * translation.
    */
   unsigned *insn;
   unsigned insn_size;
   unsigned insn_count;

   unsigned procType;  /**< TGSI_PROCESSOR_VERTEX/FRAGMENT */

   boolean error;
};


/** Map Mesa's SYSTEM_VALUE_x to TGSI_SEMANTIC_x */
static unsigned mesa_sysval_to_semantic[SYSTEM_VALUE_MAX] = {
   TGSI_SEMANTIC_FACE,
   TGSI_SEMANTIC_VERTEXID,
   TGSI_SEMANTIC_INSTANCEID
};


/**
 * Make note of a branch to a label in the TGSI code.
 * After we've emitted all instructions, we'll go over the list
 * of labels built here and patch the TGSI code with the actual
 * location of each label.
 */
static unsigned *get_label( struct st_translate *t,
                            unsigned branch_target )
{
   unsigned i;

   if (t->labels_count + 1 >= t->labels_size) {
      unsigned old_size = t->labels_size;
      t->labels_size = 1 << (util_logbase2(t->labels_size) + 1);
      t->labels = REALLOC( t->labels, 
                           old_size * sizeof t->labels[0], 
                           t->labels_size * sizeof t->labels[0] );
      if (t->labels == NULL) {
         static unsigned dummy;
         t->error = TRUE;
         return &dummy;
      }
   }

   i = t->labels_count++;
   t->labels[i].branch_target = branch_target;
   return &t->labels[i].token;
}


/**
 * Called prior to emitting the TGSI code for each Mesa instruction.
 * Allocate additional space for instructions if needed.
 * Update the insn[] array so the next Mesa instruction points to
 * the next TGSI instruction.
 */
static void set_insn_start( struct st_translate *t,
                            unsigned start )
{
   if (t->insn_count + 1 >= t->insn_size) {
      unsigned old_size = t->insn_size;
      t->insn_size = 1 << (util_logbase2(t->insn_size) + 1);
      t->insn = REALLOC( t->insn, 
                         old_size * sizeof t->insn[0], 
                         t->insn_size * sizeof t->insn[0] );
      if (t->insn == NULL) {
         t->error = TRUE;
         return;
      }
   }

   t->insn[t->insn_count++] = start;
}


/**
 * Map a Mesa dst register to a TGSI ureg_dst register.
 */
static struct ureg_dst
dst_register( struct st_translate *t,
              gl_register_file file,
              GLuint index )
{
   switch( file ) {
   case PROGRAM_UNDEFINED:
      return ureg_dst_undef();

   case PROGRAM_TEMPORARY:
      if (ureg_dst_is_undef(t->temps[index]))
         t->temps[index] = ureg_DECL_temporary( t->ureg );

      return t->temps[index];

   case PROGRAM_OUTPUT:
      if (t->procType == TGSI_PROCESSOR_VERTEX && index == VERT_RESULT_PSIZ)
         t->prevInstWrotePointSize = GL_TRUE;

      if (t->procType == TGSI_PROCESSOR_VERTEX)
         assert(index < VERT_RESULT_MAX);
      else if (t->procType == TGSI_PROCESSOR_FRAGMENT)
         assert(index < FRAG_RESULT_MAX);
      else
         assert(index < GEOM_RESULT_MAX);

      assert(t->outputMapping[index] < Elements(t->outputs));

      return t->outputs[t->outputMapping[index]];

   case PROGRAM_ADDRESS:
      return t->address[index];

   default:
      debug_assert( 0 );
      return ureg_dst_undef();
   }
}


/**
 * Map a Mesa src register to a TGSI ureg_src register.
 */
static struct ureg_src
src_register( struct st_translate *t,
              gl_register_file file,
              GLint index )
{
   switch( file ) {
   case PROGRAM_UNDEFINED:
      return ureg_src_undef();

   case PROGRAM_TEMPORARY:
      assert(index >= 0);
      assert(index < Elements(t->temps));
      if (ureg_dst_is_undef(t->temps[index]))
         t->temps[index] = ureg_DECL_temporary( t->ureg );
      return ureg_src(t->temps[index]);

   case PROGRAM_NAMED_PARAM:
   case PROGRAM_ENV_PARAM:
   case PROGRAM_LOCAL_PARAM:
   case PROGRAM_UNIFORM:
      assert(index >= 0);
      return t->constants[index];
   case PROGRAM_STATE_VAR:
   case PROGRAM_CONSTANT:       /* ie, immediate */
      if (index < 0)
         return ureg_DECL_constant( t->ureg, 0 );
      else
         return t->constants[index];

   case PROGRAM_INPUT:
      assert(t->inputMapping[index] < Elements(t->inputs));
      return t->inputs[t->inputMapping[index]];

   case PROGRAM_OUTPUT:
      assert(t->outputMapping[index] < Elements(t->outputs));
      return ureg_src(t->outputs[t->outputMapping[index]]); /* not needed? */

   case PROGRAM_ADDRESS:
      return ureg_src(t->address[index]);

   case PROGRAM_SYSTEM_VALUE:
      assert(index < Elements(t->systemValues));
      return t->systemValues[index];

   default:
      debug_assert( 0 );
      return ureg_src_undef();
   }
}


/**
 * Map mesa texture target to TGSI texture target.
 */
unsigned
st_translate_texture_target( GLuint textarget,
                          GLboolean shadow )
{
   if (shadow) {
      switch( textarget ) {
      case TEXTURE_1D_INDEX:   return TGSI_TEXTURE_SHADOW1D;
      case TEXTURE_2D_INDEX:   return TGSI_TEXTURE_SHADOW2D;
      case TEXTURE_RECT_INDEX: return TGSI_TEXTURE_SHADOWRECT;
      case TEXTURE_1D_ARRAY_INDEX: return TGSI_TEXTURE_SHADOW1D_ARRAY;
      case TEXTURE_2D_ARRAY_INDEX: return TGSI_TEXTURE_SHADOW2D_ARRAY;
      case TEXTURE_CUBE_INDEX: return TGSI_TEXTURE_SHADOWCUBE;
      default: break;
      }
   }

   switch( textarget ) {
   case TEXTURE_1D_INDEX:   return TGSI_TEXTURE_1D;
   case TEXTURE_2D_INDEX:   return TGSI_TEXTURE_2D;
   case TEXTURE_3D_INDEX:   return TGSI_TEXTURE_3D;
   case TEXTURE_CUBE_INDEX: return TGSI_TEXTURE_CUBE;
   case TEXTURE_RECT_INDEX: return TGSI_TEXTURE_RECT;
   case TEXTURE_1D_ARRAY_INDEX:   return TGSI_TEXTURE_1D_ARRAY;
   case TEXTURE_2D_ARRAY_INDEX:   return TGSI_TEXTURE_2D_ARRAY;
   case TEXTURE_EXTERNAL_INDEX:   return TGSI_TEXTURE_2D;
   default:
      debug_assert( 0 );
      return TGSI_TEXTURE_1D;
   }
}


/**
 * Create a TGSI ureg_dst register from a Mesa dest register.
 */
static struct ureg_dst
translate_dst( struct st_translate *t,
               const struct prog_dst_register *DstReg,
               boolean saturate )
{
   struct ureg_dst dst = dst_register( t, 
                                       DstReg->File,
                                       DstReg->Index );

   dst = ureg_writemask( dst, 
                         DstReg->WriteMask );
   
   if (saturate)
      dst = ureg_saturate( dst );

   if (DstReg->RelAddr)
      dst = ureg_dst_indirect( dst, ureg_src(t->address[0]) );

   return dst;
}


/**
 * Create a TGSI ureg_src register from a Mesa src register.
 */
static struct ureg_src
translate_src( struct st_translate *t,
               const struct prog_src_register *SrcReg )
{
   struct ureg_src src = src_register( t, SrcReg->File, SrcReg->Index );

   if (t->procType == TGSI_PROCESSOR_GEOMETRY && SrcReg->HasIndex2) {
      src = src_register( t, SrcReg->File, SrcReg->Index2 );
      if (SrcReg->RelAddr2)
         src = ureg_src_dimension_indirect( src, ureg_src(t->address[0]),
                                            SrcReg->Index);
      else
         src = ureg_src_dimension( src, SrcReg->Index);
   }

   src = ureg_swizzle( src,
                       GET_SWZ( SrcReg->Swizzle, 0 ) & 0x3,
                       GET_SWZ( SrcReg->Swizzle, 1 ) & 0x3,
                       GET_SWZ( SrcReg->Swizzle, 2 ) & 0x3,
                       GET_SWZ( SrcReg->Swizzle, 3 ) & 0x3);

   if (SrcReg->Negate == NEGATE_XYZW)
      src = ureg_negate(src);

   if (SrcReg->Abs) 
      src = ureg_abs(src);

   if (SrcReg->RelAddr) {
      src = ureg_src_indirect( src, ureg_src(t->address[0]));
      if (SrcReg->File != PROGRAM_INPUT &&
          SrcReg->File != PROGRAM_OUTPUT) {
         /* If SrcReg->Index was negative, it was set to zero in
          * src_register().  Reassign it now.  But don't do this
          * for input/output regs since they get remapped while
          * const buffers don't.
          */
         src.Index = SrcReg->Index;
      }
   }

   return src;
}


static struct ureg_src swizzle_4v( struct ureg_src src,
                                   const unsigned *swz )
{
   return ureg_swizzle( src, swz[0], swz[1], swz[2], swz[3] );
}


/**
 * Translate a SWZ instruction into a MOV, MUL or MAD instruction.  EG:
 *
 *   SWZ dst, src.x-y10 
 * 
 * becomes:
 *
 *   MAD dst {1,-1,0,0}, src.xyxx, {0,0,1,0}
 */
static void emit_swz( struct st_translate *t,
                      struct ureg_dst dst,
                      const struct prog_src_register *SrcReg )
{
   struct ureg_program *ureg = t->ureg;
   struct ureg_src src = src_register( t, SrcReg->File, SrcReg->Index );

   unsigned negate_mask =  SrcReg->Negate;

   unsigned one_mask = ((GET_SWZ(SrcReg->Swizzle, 0) == SWIZZLE_ONE) << 0 |
                        (GET_SWZ(SrcReg->Swizzle, 1) == SWIZZLE_ONE) << 1 |
                        (GET_SWZ(SrcReg->Swizzle, 2) == SWIZZLE_ONE) << 2 |
                        (GET_SWZ(SrcReg->Swizzle, 3) == SWIZZLE_ONE) << 3);

   unsigned zero_mask = ((GET_SWZ(SrcReg->Swizzle, 0) == SWIZZLE_ZERO) << 0 |
                         (GET_SWZ(SrcReg->Swizzle, 1) == SWIZZLE_ZERO) << 1 |
                         (GET_SWZ(SrcReg->Swizzle, 2) == SWIZZLE_ZERO) << 2 |
                         (GET_SWZ(SrcReg->Swizzle, 3) == SWIZZLE_ZERO) << 3);

   unsigned negative_one_mask = one_mask & negate_mask;
   unsigned positive_one_mask = one_mask & ~negate_mask;
   
   struct ureg_src imm;
   unsigned i;
   unsigned mul_swizzle[4] = {0,0,0,0};
   unsigned add_swizzle[4] = {0,0,0,0};
   unsigned src_swizzle[4] = {0,0,0,0};
   boolean need_add = FALSE;
   boolean need_mul = FALSE;

   if (dst.WriteMask == 0)
      return;

   /* Is this just a MOV?
    */
   if (zero_mask == 0 &&
       one_mask == 0 &&
       (negate_mask == 0 || negate_mask == TGSI_WRITEMASK_XYZW)) 
   {
      ureg_MOV( ureg, dst, translate_src( t, SrcReg ));
      return;
   }

#define IMM_ZERO    0
#define IMM_ONE     1
#define IMM_NEG_ONE 2

   imm = ureg_imm3f( ureg, 0, 1, -1 );

   for (i = 0; i < 4; i++) {
      unsigned bit = 1 << i;

      if (dst.WriteMask & bit) {
         if (positive_one_mask & bit) {
            mul_swizzle[i] = IMM_ZERO;
            add_swizzle[i] = IMM_ONE;
            need_add = TRUE;
         }
         else if (negative_one_mask & bit) {
            mul_swizzle[i] = IMM_ZERO;
            add_swizzle[i] = IMM_NEG_ONE;
            need_add = TRUE;
         }
         else if (zero_mask & bit) {
            mul_swizzle[i] = IMM_ZERO;
            add_swizzle[i] = IMM_ZERO;
            need_add = TRUE;
         }
         else {
            add_swizzle[i] = IMM_ZERO;
            src_swizzle[i] = GET_SWZ(SrcReg->Swizzle, i);
            need_mul = TRUE;
            if (negate_mask & bit) {
               mul_swizzle[i] = IMM_NEG_ONE;
            }
            else {
               mul_swizzle[i] = IMM_ONE;
            }
         }
      }
   }

   if (need_mul && need_add) {
      ureg_MAD( ureg, 
                dst,
                swizzle_4v( src, src_swizzle ),
                swizzle_4v( imm, mul_swizzle ),
                swizzle_4v( imm, add_swizzle ) );
   }
   else if (need_mul) {
      ureg_MUL( ureg, 
                dst,
                swizzle_4v( src, src_swizzle ),
                swizzle_4v( imm, mul_swizzle ) );
   }
   else if (need_add) {
      ureg_MOV( ureg, 
                dst,
                swizzle_4v( imm, add_swizzle ) );
   }
   else {
      debug_assert(0);
   }

#undef IMM_ZERO
#undef IMM_ONE
#undef IMM_NEG_ONE
}


/**
 * Negate the value of DDY to match GL semantics where (0,0) is the
 * lower-left corner of the window.
 * Note that the GL_ARB_fragment_coord_conventions extension will
 * effect this someday.
 */
static void emit_ddy( struct st_translate *t,
                      struct ureg_dst dst,
                      const struct prog_src_register *SrcReg )
{
   struct ureg_program *ureg = t->ureg;
   struct ureg_src src = translate_src( t, SrcReg );
   src = ureg_negate( src );
   ureg_DDY( ureg, dst, src );
}



static unsigned
translate_opcode( unsigned op )
{
   switch( op ) {
   case OPCODE_ARL:
      return TGSI_OPCODE_ARL;
   case OPCODE_ABS:
      return TGSI_OPCODE_ABS;
   case OPCODE_ADD:
      return TGSI_OPCODE_ADD;
   case OPCODE_BGNLOOP:
      return TGSI_OPCODE_BGNLOOP;
   case OPCODE_BGNSUB:
      return TGSI_OPCODE_BGNSUB;
   case OPCODE_BRA:
      return TGSI_OPCODE_BRA;
   case OPCODE_BRK:
      return TGSI_OPCODE_BRK;
   case OPCODE_CAL:
      return TGSI_OPCODE_CAL;
   case OPCODE_CMP:
      return TGSI_OPCODE_CMP;
   case OPCODE_CONT:
      return TGSI_OPCODE_CONT;
   case OPCODE_COS:
      return TGSI_OPCODE_COS;
   case OPCODE_DDX:
      return TGSI_OPCODE_DDX;
   case OPCODE_DDY:
      return TGSI_OPCODE_DDY;
   case OPCODE_DP2:
      return TGSI_OPCODE_DP2;
   case OPCODE_DP2A:
      return TGSI_OPCODE_DP2A;
   case OPCODE_DP3:
      return TGSI_OPCODE_DP3;
   case OPCODE_DP4:
      return TGSI_OPCODE_DP4;
   case OPCODE_DPH:
      return TGSI_OPCODE_DPH;
   case OPCODE_DST:
      return TGSI_OPCODE_DST;
   case OPCODE_ELSE:
      return TGSI_OPCODE_ELSE;
   case OPCODE_EMIT_VERTEX:
      return TGSI_OPCODE_EMIT;
   case OPCODE_END_PRIMITIVE:
      return TGSI_OPCODE_ENDPRIM;
   case OPCODE_ENDIF:
      return TGSI_OPCODE_ENDIF;
   case OPCODE_ENDLOOP:
      return TGSI_OPCODE_ENDLOOP;
   case OPCODE_ENDSUB:
      return TGSI_OPCODE_ENDSUB;
   case OPCODE_EX2:
      return TGSI_OPCODE_EX2;
   case OPCODE_EXP:
      return TGSI_OPCODE_EXP;
   case OPCODE_FLR:
      return TGSI_OPCODE_FLR;
   case OPCODE_FRC:
      return TGSI_OPCODE_FRC;
   case OPCODE_IF:
      return TGSI_OPCODE_IF;
   case OPCODE_TRUNC:
      return TGSI_OPCODE_TRUNC;
   case OPCODE_KIL:
      return TGSI_OPCODE_KIL;
   case OPCODE_KIL_NV:
      return TGSI_OPCODE_KILP;
   case OPCODE_LG2:
      return TGSI_OPCODE_LG2;
   case OPCODE_LOG:
      return TGSI_OPCODE_LOG;
   case OPCODE_LIT:
      return TGSI_OPCODE_LIT;
   case OPCODE_LRP:
      return TGSI_OPCODE_LRP;
   case OPCODE_MAD:
      return TGSI_OPCODE_MAD;
   case OPCODE_MAX:
      return TGSI_OPCODE_MAX;
   case OPCODE_MIN:
      return TGSI_OPCODE_MIN;
   case OPCODE_MOV:
      return TGSI_OPCODE_MOV;
   case OPCODE_MUL:
      return TGSI_OPCODE_MUL;
   case OPCODE_NOP:
      return TGSI_OPCODE_NOP;
   case OPCODE_NRM3:
      return TGSI_OPCODE_NRM;
   case OPCODE_NRM4:
      return TGSI_OPCODE_NRM4;
   case OPCODE_POW:
      return TGSI_OPCODE_POW;
   case OPCODE_RCP:
      return TGSI_OPCODE_RCP;
   case OPCODE_RET:
      return TGSI_OPCODE_RET;
   case OPCODE_RSQ:
      return TGSI_OPCODE_RSQ;
   case OPCODE_SCS:
      return TGSI_OPCODE_SCS;
   case OPCODE_SEQ:
      return TGSI_OPCODE_SEQ;
   case OPCODE_SGE:
      return TGSI_OPCODE_SGE;
   case OPCODE_SGT:
      return TGSI_OPCODE_SGT;
   case OPCODE_SIN:
      return TGSI_OPCODE_SIN;
   case OPCODE_SLE:
      return TGSI_OPCODE_SLE;
   case OPCODE_SLT:
      return TGSI_OPCODE_SLT;
   case OPCODE_SNE:
      return TGSI_OPCODE_SNE;
   case OPCODE_SSG:
      return TGSI_OPCODE_SSG;
   case OPCODE_SUB:
      return TGSI_OPCODE_SUB;
   case OPCODE_TEX:
      return TGSI_OPCODE_TEX;
   case OPCODE_TXB:
      return TGSI_OPCODE_TXB;
   case OPCODE_TXD:
      return TGSI_OPCODE_TXD;
   case OPCODE_TXL:
      return TGSI_OPCODE_TXL;
   case OPCODE_TXP:
      return TGSI_OPCODE_TXP;
   case OPCODE_XPD:
      return TGSI_OPCODE_XPD;
   case OPCODE_END:
      return TGSI_OPCODE_END;
   default:
      debug_assert( 0 );
      return TGSI_OPCODE_NOP;
   }
}


static void
compile_instruction(
   struct st_translate *t,
   const struct prog_instruction *inst )
{
   struct ureg_program *ureg = t->ureg;
   GLuint i;
   struct ureg_dst dst[1];
   struct ureg_src src[4];
   unsigned num_dst;
   unsigned num_src;

   num_dst = _mesa_num_inst_dst_regs( inst->Opcode );
   num_src = _mesa_num_inst_src_regs( inst->Opcode );

   if (num_dst) 
      dst[0] = translate_dst( t, 
                              &inst->DstReg,
                              inst->SaturateMode );

   for (i = 0; i < num_src; i++) 
      src[i] = translate_src( t, &inst->SrcReg[i] );

   switch( inst->Opcode ) {
   case OPCODE_SWZ:
      emit_swz( t, dst[0], &inst->SrcReg[0] );
      return;

   case OPCODE_BGNLOOP:
   case OPCODE_CAL:
   case OPCODE_ELSE:
   case OPCODE_ENDLOOP:
   case OPCODE_IF:
      debug_assert(num_dst == 0);
      ureg_label_insn( ureg,
                       translate_opcode( inst->Opcode ),
                       src, num_src,
                       get_label( t, inst->BranchTarget ));
      return;

   case OPCODE_TEX:
   case OPCODE_TXB:
   case OPCODE_TXD:
   case OPCODE_TXL:
   case OPCODE_TXP:
      src[num_src++] = t->samplers[inst->TexSrcUnit];
      ureg_tex_insn( ureg,
                     translate_opcode( inst->Opcode ),
                     dst, num_dst, 
                     st_translate_texture_target( inst->TexSrcTarget,
                                               inst->TexShadow ),
                     NULL, 0,
                     src, num_src );
      return;

   case OPCODE_SCS:
      dst[0] = ureg_writemask(dst[0], TGSI_WRITEMASK_XY );
      ureg_insn( ureg, 
                 translate_opcode( inst->Opcode ), 
                 dst, num_dst, 
                 src, num_src );
      break;

   case OPCODE_XPD:
      dst[0] = ureg_writemask(dst[0], TGSI_WRITEMASK_XYZ );
      ureg_insn( ureg, 
                 translate_opcode( inst->Opcode ), 
                 dst, num_dst, 
                 src, num_src );
      break;

   case OPCODE_NOISE1:
   case OPCODE_NOISE2:
   case OPCODE_NOISE3:
   case OPCODE_NOISE4:
      /* At some point, a motivated person could add a better
       * implementation of noise.  Currently not even the nvidia
       * binary drivers do anything more than this.  In any case, the
       * place to do this is in the GL state tracker, not the poor
       * driver.
       */
      ureg_MOV( ureg, dst[0], ureg_imm1f(ureg, 0.5) );
      break;
		 
   case OPCODE_DDY:
      emit_ddy( t, dst[0], &inst->SrcReg[0] );
      break;

   default:
      ureg_insn( ureg, 
                 translate_opcode( inst->Opcode ), 
                 dst, num_dst, 
                 src, num_src );
      break;
   }
}


/**
 * Emit the TGSI instructions for inverting and adjusting WPOS.
 * This code is unavoidable because it also depends on whether
 * a FBO is bound (STATE_FB_WPOS_Y_TRANSFORM).
 */
static void
emit_wpos_adjustment( struct st_translate *t,
                      const struct gl_program *program,
                      boolean invert,
                      GLfloat adjX, GLfloat adjY[2])
{
   struct ureg_program *ureg = t->ureg;

   /* Fragment program uses fragment position input.
    * Need to replace instances of INPUT[WPOS] with temp T
    * where T = INPUT[WPOS] by y is inverted.
    */
   static const gl_state_index wposTransformState[STATE_LENGTH]
      = { STATE_INTERNAL, STATE_FB_WPOS_Y_TRANSFORM, 0, 0, 0 };
   
   /* XXX: note we are modifying the incoming shader here!  Need to
    * do this before emitting the constant decls below, or this
    * will be missed:
    */
   unsigned wposTransConst = _mesa_add_state_reference(program->Parameters,
                                                       wposTransformState);

   struct ureg_src wpostrans = ureg_DECL_constant( ureg, wposTransConst );
   struct ureg_dst wpos_temp = ureg_DECL_temporary( ureg );
   struct ureg_src wpos_input = t->inputs[t->inputMapping[FRAG_ATTRIB_WPOS]];

   /* First, apply the coordinate shift: */
   if (adjX || adjY[0] || adjY[1]) {
      if (adjY[0] != adjY[1]) {
         /* Adjust the y coordinate by adjY[1] or adjY[0] respectively
          * depending on whether inversion is actually going to be applied
          * or not, which is determined by testing against the inversion
          * state variable used below, which will be either +1 or -1.
          */
         struct ureg_dst adj_temp = ureg_DECL_temporary(ureg);

         ureg_CMP(ureg, adj_temp,
                  ureg_scalar(wpostrans, invert ? 2 : 0),
                  ureg_imm4f(ureg, adjX, adjY[0], 0.0f, 0.0f),
                  ureg_imm4f(ureg, adjX, adjY[1], 0.0f, 0.0f));
         ureg_ADD(ureg, wpos_temp, wpos_input, ureg_src(adj_temp));
      } else {
         ureg_ADD(ureg, wpos_temp, wpos_input,
                  ureg_imm4f(ureg, adjX, adjY[0], 0.0f, 0.0f));
      }
      wpos_input = ureg_src(wpos_temp);
   } else {
      /* MOV wpos_temp, input[wpos]
       */
      ureg_MOV( ureg, wpos_temp, wpos_input );
   }

   /* Now the conditional y flip: STATE_FB_WPOS_Y_TRANSFORM.xy/zw will be
    * inversion/identity, or the other way around if we're drawing to an FBO.
    */
   if (invert) {
      /* MAD wpos_temp.y, wpos_input, wpostrans.xxxx, wpostrans.yyyy
       */
      ureg_MAD( ureg,
                ureg_writemask(wpos_temp, TGSI_WRITEMASK_Y ),
                wpos_input,
                ureg_scalar(wpostrans, 0),
                ureg_scalar(wpostrans, 1));
   } else {
      /* MAD wpos_temp.y, wpos_input, wpostrans.zzzz, wpostrans.wwww
       */
      ureg_MAD( ureg,
                ureg_writemask(wpos_temp, TGSI_WRITEMASK_Y ),
                wpos_input,
                ureg_scalar(wpostrans, 2),
                ureg_scalar(wpostrans, 3));
   }

   /* Use wpos_temp as position input from here on:
    */
   t->inputs[t->inputMapping[FRAG_ATTRIB_WPOS]] = ureg_src(wpos_temp);
}


/**
 * Emit fragment position/ooordinate code.
 */
static void
emit_wpos(struct st_context *st,
          struct st_translate *t,
          const struct gl_program *program,
          struct ureg_program *ureg)
{
   const struct gl_fragment_program *fp =
      (const struct gl_fragment_program *) program;
   struct pipe_screen *pscreen = st->pipe->screen;
   GLfloat adjX = 0.0f;
   GLfloat adjY[2] = { 0.0f, 0.0f };
   boolean invert = FALSE;

   /* Query the pixel center conventions supported by the pipe driver and set
    * adjX, adjY to help out if it cannot handle the requested one internally.
    *
    * The bias of the y-coordinate depends on whether y-inversion takes place
    * (adjY[1]) or not (adjY[0]), which is in turn dependent on whether we are
    * drawing to an FBO (causes additional inversion), and whether the the pipe
    * driver origin and the requested origin differ (the latter condition is
    * stored in the 'invert' variable).
    *
    * For height = 100 (i = integer, h = half-integer, l = lower, u = upper):
    *
    * center shift only:
    * i -> h: +0.5
    * h -> i: -0.5
    *
    * inversion only:
    * l,i -> u,i: ( 0.0 + 1.0) * -1 + 100 = 99
    * l,h -> u,h: ( 0.5 + 0.0) * -1 + 100 = 99.5
    * u,i -> l,i: (99.0 + 1.0) * -1 + 100 = 0
    * u,h -> l,h: (99.5 + 0.0) * -1 + 100 = 0.5
    *
    * inversion and center shift:
    * l,i -> u,h: ( 0.0 + 0.5) * -1 + 100 = 99.5
    * l,h -> u,i: ( 0.5 + 0.5) * -1 + 100 = 99
    * u,i -> l,h: (99.0 + 0.5) * -1 + 100 = 0.5
    * u,h -> l,i: (99.5 + 0.5) * -1 + 100 = 0
    */
   if (fp->OriginUpperLeft) {
      /* Fragment shader wants origin in upper-left */
      if (pscreen->get_param(pscreen, PIPE_CAP_TGSI_FS_COORD_ORIGIN_UPPER_LEFT)) {
         /* the driver supports upper-left origin */
      }
      else if (pscreen->get_param(pscreen, PIPE_CAP_TGSI_FS_COORD_ORIGIN_LOWER_LEFT)) {
         /* the driver supports lower-left origin, need to invert Y */
         ureg_property_fs_coord_origin(ureg, TGSI_FS_COORD_ORIGIN_LOWER_LEFT);
         invert = TRUE;
      }
      else
         assert(0);
   }
   else {
      /* Fragment shader wants origin in lower-left */
      if (pscreen->get_param(pscreen, PIPE_CAP_TGSI_FS_COORD_ORIGIN_LOWER_LEFT))
         /* the driver supports lower-left origin */
         ureg_property_fs_coord_origin(ureg, TGSI_FS_COORD_ORIGIN_LOWER_LEFT);
      else if (pscreen->get_param(pscreen, PIPE_CAP_TGSI_FS_COORD_ORIGIN_UPPER_LEFT))
         /* the driver supports upper-left origin, need to invert Y */
         invert = TRUE;
      else
         assert(0);
   }
   
   if (fp->PixelCenterInteger) {
      /* Fragment shader wants pixel center integer */
      if (pscreen->get_param(pscreen, PIPE_CAP_TGSI_FS_COORD_PIXEL_CENTER_INTEGER)) {
         /* the driver supports pixel center integer */
         adjY[1] = 1.0f;
         ureg_property_fs_coord_pixel_center(ureg, TGSI_FS_COORD_PIXEL_CENTER_INTEGER);
      }
      else if (pscreen->get_param(pscreen, PIPE_CAP_TGSI_FS_COORD_PIXEL_CENTER_HALF_INTEGER)) {
         /* the driver supports pixel center half integer, need to bias X,Y */
         adjX = -0.5f;
         adjY[0] = -0.5f;
         adjY[1] = 0.5f;
      }
      else
         assert(0);
   }
   else {
      /* Fragment shader wants pixel center half integer */
      if (pscreen->get_param(pscreen, PIPE_CAP_TGSI_FS_COORD_PIXEL_CENTER_HALF_INTEGER)) {
         /* the driver supports pixel center half integer */
      }
      else if (pscreen->get_param(pscreen, PIPE_CAP_TGSI_FS_COORD_PIXEL_CENTER_INTEGER)) {
         /* the driver supports pixel center integer, need to bias X,Y */
         adjX = adjY[0] = adjY[1] = 0.5f;
         ureg_property_fs_coord_pixel_center(ureg, TGSI_FS_COORD_PIXEL_CENTER_INTEGER);
      }
      else
         assert(0);
   }

   /* we invert after adjustment so that we avoid the MOV to temporary,
    * and reuse the adjustment ADD instead */
   emit_wpos_adjustment(t, program, invert, adjX, adjY);
}


/**
 * OpenGL's fragment gl_FrontFace input is 1 for front-facing, 0 for back.
 * TGSI uses +1 for front, -1 for back.
 * This function converts the TGSI value to the GL value.  Simply clamping/
 * saturating the value to [0,1] does the job.
 */
static void
emit_face_var( struct st_translate *t,
               const struct gl_program *program )
{
   struct ureg_program *ureg = t->ureg;
   struct ureg_dst face_temp = ureg_DECL_temporary( ureg );
   struct ureg_src face_input = t->inputs[t->inputMapping[FRAG_ATTRIB_FACE]];

   /* MOV_SAT face_temp, input[face]
    */
   face_temp = ureg_saturate( face_temp );
   ureg_MOV( ureg, face_temp, face_input );

   /* Use face_temp as face input from here on:
    */
   t->inputs[t->inputMapping[FRAG_ATTRIB_FACE]] = ureg_src(face_temp);
}


static void
emit_edgeflags( struct st_translate *t,
                 const struct gl_program *program )
{
   struct ureg_program *ureg = t->ureg;
   struct ureg_dst edge_dst = t->outputs[t->outputMapping[VERT_RESULT_EDGE]];
   struct ureg_src edge_src = t->inputs[t->inputMapping[VERT_ATTRIB_EDGEFLAG]];

   ureg_MOV( ureg, edge_dst, edge_src );
}


/**
 * Translate Mesa program to TGSI format.
 * \param program  the program to translate
 * \param numInputs  number of input registers used
 * \param inputMapping  maps Mesa fragment program inputs to TGSI generic
 *                      input indexes
 * \param inputSemanticName  the TGSI_SEMANTIC flag for each input
 * \param inputSemanticIndex  the semantic index (ex: which texcoord) for
 *                            each input
 * \param interpMode  the TGSI_INTERPOLATE_LINEAR/PERSP mode for each input
 * \param numOutputs  number of output registers used
 * \param outputMapping  maps Mesa fragment program outputs to TGSI
 *                       generic outputs
 * \param outputSemanticName  the TGSI_SEMANTIC flag for each output
 * \param outputSemanticIndex  the semantic index (ex: which texcoord) for
 *                             each output
 *
 * \return  PIPE_OK or PIPE_ERROR_OUT_OF_MEMORY
 */
enum pipe_error
st_translate_mesa_program(
   struct gl_context *ctx,
   uint procType,
   struct ureg_program *ureg,
   const struct gl_program *program,
   GLuint numInputs,
   const GLuint inputMapping[],
   const ubyte inputSemanticName[],
   const ubyte inputSemanticIndex[],
   const GLuint interpMode[],
   GLuint numOutputs,
   const GLuint outputMapping[],
   const ubyte outputSemanticName[],
   const ubyte outputSemanticIndex[],
   boolean passthrough_edgeflags )
{
   struct st_translate translate, *t;
   unsigned i;
   enum pipe_error ret = PIPE_OK;

   assert(numInputs <= Elements(t->inputs));
   assert(numOutputs <= Elements(t->outputs));

   t = &translate;
   memset(t, 0, sizeof *t);

   t->procType = procType;
   t->inputMapping = inputMapping;
   t->outputMapping = outputMapping;
   t->ureg = ureg;
   t->pointSizeOutIndex = -1;
   t->prevInstWrotePointSize = GL_FALSE;

   /*_mesa_print_program(program);*/

   /*
    * Declare input attributes.
    */
   if (procType == TGSI_PROCESSOR_FRAGMENT) {
      for (i = 0; i < numInputs; i++) {
         if (program->InputFlags[0] & PROG_PARAM_BIT_CYL_WRAP) {
            t->inputs[i] = ureg_DECL_fs_input_cyl(ureg,
                                                  inputSemanticName[i],
                                                  inputSemanticIndex[i],
                                                  interpMode[i],
                                                  TGSI_CYLINDRICAL_WRAP_X);
         }
         else {
            t->inputs[i] = ureg_DECL_fs_input(ureg,
                                              inputSemanticName[i],
                                              inputSemanticIndex[i],
                                              interpMode[i]);
         }
      }

      if (program->InputsRead & FRAG_BIT_WPOS) {
         /* Must do this after setting up t->inputs, and before
          * emitting constant references, below:
          */
         emit_wpos(st_context(ctx), t, program, ureg);
      }

      if (program->InputsRead & FRAG_BIT_FACE) {
         emit_face_var( t, program );
      }

      /*
       * Declare output attributes.
       */
      for (i = 0; i < numOutputs; i++) {
         switch (outputSemanticName[i]) {
         case TGSI_SEMANTIC_POSITION:
            t->outputs[i] = ureg_DECL_output( ureg,
                                              TGSI_SEMANTIC_POSITION, /* Z / Depth */
                                              outputSemanticIndex[i] );

            t->outputs[i] = ureg_writemask( t->outputs[i],
                                            TGSI_WRITEMASK_Z );
            break;
         case TGSI_SEMANTIC_STENCIL:
            t->outputs[i] = ureg_DECL_output( ureg,
                                              TGSI_SEMANTIC_STENCIL, /* Stencil */
                                              outputSemanticIndex[i] );
            t->outputs[i] = ureg_writemask( t->outputs[i],
                                            TGSI_WRITEMASK_Y );
            break;
         case TGSI_SEMANTIC_COLOR:
            t->outputs[i] = ureg_DECL_output( ureg,
                                              TGSI_SEMANTIC_COLOR,
                                              outputSemanticIndex[i] );
            break;
         default:
            debug_assert(0);
            return 0;
         }
      }
   }
   else if (procType == TGSI_PROCESSOR_GEOMETRY) {
      for (i = 0; i < numInputs; i++) {
         t->inputs[i] = ureg_DECL_gs_input(ureg,
                                           i,
                                           inputSemanticName[i],
                                           inputSemanticIndex[i]);
      }

      for (i = 0; i < numOutputs; i++) {
         t->outputs[i] = ureg_DECL_output( ureg,
                                           outputSemanticName[i],
                                           outputSemanticIndex[i] );
      }
   }
   else {
      assert(procType == TGSI_PROCESSOR_VERTEX);

      for (i = 0; i < numInputs; i++) {
         t->inputs[i] = ureg_DECL_vs_input(ureg, i);
      }

      for (i = 0; i < numOutputs; i++) {
         t->outputs[i] = ureg_DECL_output( ureg,
                                           outputSemanticName[i],
                                           outputSemanticIndex[i] );
         if ((outputSemanticName[i] == TGSI_SEMANTIC_PSIZE) && program->Id) {
            /* Writing to the point size result register requires special
             * handling to implement clamping.
             */
            static const gl_state_index pointSizeClampState[STATE_LENGTH]
               = { STATE_INTERNAL, STATE_POINT_SIZE_IMPL_CLAMP, 0, 0, 0 };
               /* XXX: note we are modifying the incoming shader here!  Need to
               * do this before emitting the constant decls below, or this
               * will be missed:
               */
            unsigned pointSizeClampConst =
               _mesa_add_state_reference(program->Parameters,
                                         pointSizeClampState);
            struct ureg_dst psizregtemp = ureg_DECL_temporary( ureg );
            t->pointSizeConst = ureg_DECL_constant( ureg, pointSizeClampConst );
            t->pointSizeResult = t->outputs[i];
            t->pointSizeOutIndex = i;
            t->outputs[i] = psizregtemp;
         }
      }
      if (passthrough_edgeflags)
         emit_edgeflags( t, program );
   }

   /* Declare address register.
    */
   if (program->NumAddressRegs > 0) {
      debug_assert( program->NumAddressRegs == 1 );
      t->address[0] = ureg_DECL_address( ureg );
   }

   /* Declare misc input registers
    */
   {
      GLbitfield sysInputs = program->SystemValuesRead;
      unsigned numSys = 0;
      for (i = 0; sysInputs; i++) {
         if (sysInputs & (1 << i)) {
            unsigned semName = mesa_sysval_to_semantic[i];
            t->systemValues[i] = ureg_DECL_system_value(ureg, numSys, semName, 0);
            numSys++;
            sysInputs &= ~(1 << i);
         }
      }
   }

   if (program->IndirectRegisterFiles & (1 << PROGRAM_TEMPORARY)) {
      /* If temps are accessed with indirect addressing, declare temporaries
       * in sequential order.  Else, we declare them on demand elsewhere.
       */
      for (i = 0; i < program->NumTemporaries; i++) {
         /* XXX use TGSI_FILE_TEMPORARY_ARRAY when it's supported by ureg */
         t->temps[i] = ureg_DECL_temporary( t->ureg );
      }
   }

   /* Emit constants and immediates.  Mesa uses a single index space
    * for these, so we put all the translated regs in t->constants.
    */
   if (program->Parameters) {
      t->constants = CALLOC( program->Parameters->NumParameters,
                             sizeof t->constants[0] );
      if (t->constants == NULL) {
         ret = PIPE_ERROR_OUT_OF_MEMORY;
         goto out;
      }

      for (i = 0; i < program->Parameters->NumParameters; i++) {
         switch (program->Parameters->Parameters[i].Type) {
         case PROGRAM_ENV_PARAM:
         case PROGRAM_LOCAL_PARAM:
         case PROGRAM_STATE_VAR:
         case PROGRAM_NAMED_PARAM:
         case PROGRAM_UNIFORM:
            t->constants[i] = ureg_DECL_constant( ureg, i );
            break;

            /* Emit immediates only when there's no indirect addressing of
             * the const buffer.
             * FIXME: Be smarter and recognize param arrays:
             * indirect addressing is only valid within the referenced
             * array.
             */
         case PROGRAM_CONSTANT:
            if (program->IndirectRegisterFiles & PROGRAM_ANY_CONST)
               t->constants[i] = ureg_DECL_constant( ureg, i );
            else
               t->constants[i] = 
                  ureg_DECL_immediate( ureg,
                                       (const float*) program->Parameters->ParameterValues[i],
                                       4 );
            break;
         default:
            break;
         }
      }
   }

   /* texture samplers */
   for (i = 0; i < ctx->Const.MaxTextureImageUnits; i++) {
      if (program->SamplersUsed & (1 << i)) {
         t->samplers[i] = ureg_DECL_sampler( ureg, i );
      }
   }

   /* Emit each instruction in turn:
    */
   for (i = 0; i < program->NumInstructions; i++) {
      set_insn_start( t, ureg_get_instruction_number( ureg ));
      compile_instruction( t, &program->Instructions[i] );

      if (t->prevInstWrotePointSize && program->Id) {
         /* The previous instruction wrote to the (fake) vertex point size
          * result register.  Now we need to clamp that value to the min/max
          * point size range, putting the result into the real point size
          * register.
          * Note that we can't do this easily at the end of program due to
          * possible early return.
          */
         set_insn_start( t, ureg_get_instruction_number( ureg ));
         ureg_MAX( t->ureg,
                   ureg_writemask(t->outputs[t->pointSizeOutIndex], WRITEMASK_X),
                   ureg_src(t->outputs[t->pointSizeOutIndex]),
                   ureg_swizzle(t->pointSizeConst, 1,1,1,1));
         ureg_MIN( t->ureg, ureg_writemask(t->pointSizeResult, WRITEMASK_X),
                   ureg_src(t->outputs[t->pointSizeOutIndex]),
                   ureg_swizzle(t->pointSizeConst, 2,2,2,2));
      }
      t->prevInstWrotePointSize = GL_FALSE;
   }

   /* Fix up all emitted labels:
    */
   for (i = 0; i < t->labels_count; i++) {
      ureg_fixup_label( ureg,
                        t->labels[i].token,
                        t->insn[t->labels[i].branch_target] );
   }

out:
   FREE(t->insn);
   FREE(t->labels);
   FREE(t->constants);

   if (t->error) {
      debug_printf("%s: translate error flag set\n", __FUNCTION__);
   }

   return ret;
}


/**
 * Tokens cannot be free with free otherwise the builtin gallium
 * malloc debugging will get confused.
 */
void
st_free_tokens(const struct tgsi_token *tokens)
{
   FREE((void *)tokens);
}

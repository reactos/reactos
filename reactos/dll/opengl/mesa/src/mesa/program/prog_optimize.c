/*
 * Mesa 3-D graphics library
 * Version:  7.5
 *
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
 * VMWARE BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */



#include "main/glheader.h"
#include "main/context.h"
#include "main/macros.h"
#include "program.h"
#include "prog_instruction.h"
#include "prog_optimize.h"
#include "prog_print.h"


#define MAX_LOOP_NESTING 50
/* MAX_PROGRAM_TEMPS is a low number (256), and we want to be able to
 * register allocate many temporary values into that small number of
 * temps.  So allow large temporary indices coming into the register
 * allocator.
 */
#define REG_ALLOCATE_MAX_PROGRAM_TEMPS	((1 << INST_INDEX_BITS) - 1)

static GLboolean dbg = GL_FALSE;

#define NO_MASK 0xf

/**
 * Returns the mask of channels (bitmask of WRITEMASK_X,Y,Z,W) which
 * are read from the given src in this instruction, We also provide
 * one optional masks which may mask other components in the dst
 * register
 */
static GLuint
get_src_arg_mask(const struct prog_instruction *inst,
                 GLuint arg, GLuint dst_mask)
{
   GLuint read_mask, channel_mask;
   GLuint comp;

   ASSERT(arg < _mesa_num_inst_src_regs(inst->Opcode));

   /* Form the dst register, find the written channels */
   if (inst->CondUpdate) {
      channel_mask = WRITEMASK_XYZW;
   }
   else {
      switch (inst->Opcode) {
      case OPCODE_MOV:
      case OPCODE_MIN:
      case OPCODE_MAX:
      case OPCODE_ABS:
      case OPCODE_ADD:
      case OPCODE_MAD:
      case OPCODE_MUL:
      case OPCODE_SUB:
      case OPCODE_CMP:
      case OPCODE_FLR:
      case OPCODE_FRC:
      case OPCODE_LRP:
      case OPCODE_SEQ:
      case OPCODE_SGE:
      case OPCODE_SGT:
      case OPCODE_SLE:
      case OPCODE_SLT:
      case OPCODE_SNE:
      case OPCODE_SSG:
         channel_mask = inst->DstReg.WriteMask & dst_mask;
         break;
      case OPCODE_RCP:
      case OPCODE_SIN:
      case OPCODE_COS:
      case OPCODE_RSQ:
      case OPCODE_POW:
      case OPCODE_EX2:
      case OPCODE_LOG:
         channel_mask = WRITEMASK_X;
         break;
      case OPCODE_DP2:
         channel_mask = WRITEMASK_XY;
         break;
      case OPCODE_DP3:
      case OPCODE_XPD:
         channel_mask = WRITEMASK_XYZ;
         break;
      default:
         channel_mask = WRITEMASK_XYZW;
         break;
      }
   }

   /* Now, given the src swizzle and the written channels, find which
    * components are actually read
    */
   read_mask = 0x0;
   for (comp = 0; comp < 4; ++comp) {
      const GLuint coord = GET_SWZ(inst->SrcReg[arg].Swizzle, comp);
      ASSERT(coord < 4);
      if (channel_mask & (1 << comp) && coord <= SWIZZLE_W)
         read_mask |= 1 << coord;
   }

   return read_mask;
}


/**
 * For a MOV instruction, compute a write mask when src register also has
 * a mask
 */
static GLuint
get_dst_mask_for_mov(const struct prog_instruction *mov, GLuint src_mask)
{
   const GLuint mask = mov->DstReg.WriteMask;
   GLuint comp;
   GLuint updated_mask = 0x0;

   ASSERT(mov->Opcode == OPCODE_MOV);

   for (comp = 0; comp < 4; ++comp) {
      GLuint src_comp;
      if ((mask & (1 << comp)) == 0)
         continue;
      src_comp = GET_SWZ(mov->SrcReg[0].Swizzle, comp);
      if ((src_mask & (1 << src_comp)) == 0)
         continue;
      updated_mask |= 1 << comp;
   }

   return updated_mask;
}


/**
 * Ensure that the swizzle is regular.  That is, all of the swizzle
 * terms are SWIZZLE_X,Y,Z,W and not SWIZZLE_ZERO or SWIZZLE_ONE.
 */
static GLboolean
is_swizzle_regular(GLuint swz)
{
   return GET_SWZ(swz,0) <= SWIZZLE_W &&
          GET_SWZ(swz,1) <= SWIZZLE_W &&
          GET_SWZ(swz,2) <= SWIZZLE_W &&
          GET_SWZ(swz,3) <= SWIZZLE_W;
}


/**
 * In 'prog' remove instruction[i] if removeFlags[i] == TRUE.
 * \return number of instructions removed
 */
static GLuint
remove_instructions(struct gl_program *prog, const GLboolean *removeFlags)
{
   GLint i, removeEnd = 0, removeCount = 0;
   GLuint totalRemoved = 0;

   /* go backward */
   for (i = prog->NumInstructions - 1; i >= 0; i--) {
      if (removeFlags[i]) {
         totalRemoved++;
         if (removeCount == 0) {
            /* begin a run of instructions to remove */
            removeEnd = i;
            removeCount = 1;
         }
         else {
            /* extend the run of instructions to remove */
            removeCount++;
         }
      }
      else {
         /* don't remove this instruction, but check if the preceeding
          * instructions are to be removed.
          */
         if (removeCount > 0) {
            GLint removeStart = removeEnd - removeCount + 1;
            _mesa_delete_instructions(prog, removeStart, removeCount);
            removeStart = removeCount = 0; /* reset removal info */
         }
      }
   }
   /* Finish removing if the first instruction was to be removed. */
   if (removeCount > 0) {
      GLint removeStart = removeEnd - removeCount + 1;
      _mesa_delete_instructions(prog, removeStart, removeCount);
   }
   return totalRemoved;
}


/**
 * Remap register indexes according to map.
 * \param prog  the program to search/replace
 * \param file  the type of register file to search/replace
 * \param map  maps old register indexes to new indexes
 */
static void
replace_regs(struct gl_program *prog, gl_register_file file, const GLint map[])
{
   GLuint i;

   for (i = 0; i < prog->NumInstructions; i++) {
      struct prog_instruction *inst = prog->Instructions + i;
      const GLuint numSrc = _mesa_num_inst_src_regs(inst->Opcode);
      GLuint j;
      for (j = 0; j < numSrc; j++) {
         if (inst->SrcReg[j].File == file) {
            GLuint index = inst->SrcReg[j].Index;
            ASSERT(map[index] >= 0);
            inst->SrcReg[j].Index = map[index];
         }
      }
      if (inst->DstReg.File == file) {
         const GLuint index = inst->DstReg.Index;
         ASSERT(map[index] >= 0);
         inst->DstReg.Index = map[index];
      }
   }
}


/**
 * Remove dead instructions from the given program.
 * This is very primitive for now.  Basically look for temp registers
 * that are written to but never read.  Remove any instructions that
 * write to such registers.  Be careful with condition code setters.
 */
static GLboolean
_mesa_remove_dead_code_global(struct gl_program *prog)
{
   GLboolean tempRead[REG_ALLOCATE_MAX_PROGRAM_TEMPS][4];
   GLboolean *removeInst; /* per-instruction removal flag */
   GLuint i, rem = 0, comp;

   memset(tempRead, 0, sizeof(tempRead));

   if (dbg) {
      printf("Optimize: Begin dead code removal\n");
      /*_mesa_print_program(prog);*/
   }

   removeInst = (GLboolean *)
      calloc(1, prog->NumInstructions * sizeof(GLboolean));

   /* Determine which temps are read and written */
   for (i = 0; i < prog->NumInstructions; i++) {
      const struct prog_instruction *inst = prog->Instructions + i;
      const GLuint numSrc = _mesa_num_inst_src_regs(inst->Opcode);
      GLuint j;

      /* check src regs */
      for (j = 0; j < numSrc; j++) {
         if (inst->SrcReg[j].File == PROGRAM_TEMPORARY) {
            const GLuint index = inst->SrcReg[j].Index;
            GLuint read_mask;
            ASSERT(index < REG_ALLOCATE_MAX_PROGRAM_TEMPS);
	    read_mask = get_src_arg_mask(inst, j, NO_MASK);

            if (inst->SrcReg[j].RelAddr) {
               if (dbg)
                  printf("abort remove dead code (indirect temp)\n");
               goto done;
            }

	    for (comp = 0; comp < 4; comp++) {
	       const GLuint swz = GET_SWZ(inst->SrcReg[j].Swizzle, comp);
	       ASSERT(swz < 4);
               if ((read_mask & (1 << swz)) == 0)
		  continue;
               if (swz <= SWIZZLE_W)
                  tempRead[index][swz] = GL_TRUE;
	    }
         }
      }

      /* check dst reg */
      if (inst->DstReg.File == PROGRAM_TEMPORARY) {
         const GLuint index = inst->DstReg.Index;
         ASSERT(index < REG_ALLOCATE_MAX_PROGRAM_TEMPS);

         if (inst->DstReg.RelAddr) {
            if (dbg)
               printf("abort remove dead code (indirect temp)\n");
            goto done;
         }

         if (inst->CondUpdate) {
            /* If we're writing to this register and setting condition
             * codes we cannot remove the instruction.  Prevent removal
             * by setting the 'read' flag.
             */
            tempRead[index][0] = GL_TRUE;
            tempRead[index][1] = GL_TRUE;
            tempRead[index][2] = GL_TRUE;
            tempRead[index][3] = GL_TRUE;
         }
      }
   }

   /* find instructions that write to dead registers, flag for removal */
   for (i = 0; i < prog->NumInstructions; i++) {
      struct prog_instruction *inst = prog->Instructions + i;
      const GLuint numDst = _mesa_num_inst_dst_regs(inst->Opcode);

      if (numDst != 0 && inst->DstReg.File == PROGRAM_TEMPORARY) {
         GLint chan, index = inst->DstReg.Index;

	 for (chan = 0; chan < 4; chan++) {
	    if (!tempRead[index][chan] &&
		inst->DstReg.WriteMask & (1 << chan)) {
	       if (dbg) {
		  printf("Remove writemask on %u.%c\n", i,
			       chan == 3 ? 'w' : 'x' + chan);
	       }
	       inst->DstReg.WriteMask &= ~(1 << chan);
	       rem++;
	    }
	 }

	 if (inst->DstReg.WriteMask == 0) {
	    /* If we cleared all writes, the instruction can be removed. */
	    if (dbg)
	       printf("Remove instruction %u: \n", i);
	    removeInst[i] = GL_TRUE;
	 }
      }
   }

   /* now remove the instructions which aren't needed */
   rem = remove_instructions(prog, removeInst);

   if (dbg) {
      printf("Optimize: End dead code removal.\n");
      printf("  %u channel writes removed\n", rem);
      printf("  %u instructions removed\n", rem);
      /*_mesa_print_program(prog);*/
   }

done:
   free(removeInst);
   return rem != 0;
}


enum inst_use
{
   READ,
   WRITE,
   FLOW,
   END
};


/**
 * Scan forward in program from 'start' for the next occurances of TEMP[index].
 * We look if an instruction reads the component given by the masks and if they
 * are overwritten.
 * Return READ, WRITE, FLOW or END to indicate the next usage or an indicator
 * that we can't look further.
 */
static enum inst_use
find_next_use(const struct gl_program *prog,
              GLuint start,
              GLuint index,
              GLuint mask)
{
   GLuint i;

   for (i = start; i < prog->NumInstructions; i++) {
      const struct prog_instruction *inst = prog->Instructions + i;
      switch (inst->Opcode) {
      case OPCODE_BGNLOOP:
      case OPCODE_BGNSUB:
      case OPCODE_BRA:
      case OPCODE_CAL:
      case OPCODE_CONT:
      case OPCODE_IF:
      case OPCODE_ELSE:
      case OPCODE_ENDIF:
      case OPCODE_ENDLOOP:
      case OPCODE_ENDSUB:
      case OPCODE_RET:
         return FLOW;
      case OPCODE_END:
         return END;
      default:
         {
            const GLuint numSrc = _mesa_num_inst_src_regs(inst->Opcode);
            GLuint j;
            for (j = 0; j < numSrc; j++) {
               if (inst->SrcReg[j].RelAddr ||
                   (inst->SrcReg[j].File == PROGRAM_TEMPORARY &&
                   inst->SrcReg[j].Index == index &&
                   (get_src_arg_mask(inst,j,NO_MASK) & mask)))
                  return READ;
            }
            if (_mesa_num_inst_dst_regs(inst->Opcode) == 1 &&
                inst->DstReg.File == PROGRAM_TEMPORARY &&
                inst->DstReg.Index == index) {
               mask &= ~inst->DstReg.WriteMask;
               if (mask == 0)
                  return WRITE;
            }
         }
      }
   }
   return END;
}


/**
 * Is the given instruction opcode a flow-control opcode?
 * XXX maybe move this into prog_instruction.[ch]
 */
static GLboolean
_mesa_is_flow_control_opcode(enum prog_opcode opcode)
{
   switch (opcode) {
   case OPCODE_BGNLOOP:
   case OPCODE_BGNSUB:
   case OPCODE_BRA:
   case OPCODE_CAL:
   case OPCODE_CONT:
   case OPCODE_IF:
   case OPCODE_ELSE:
   case OPCODE_END:
   case OPCODE_ENDIF:
   case OPCODE_ENDLOOP:
   case OPCODE_ENDSUB:
   case OPCODE_RET:
      return GL_TRUE;
   default:
      return GL_FALSE;
   }
}


/**
 * Test if the given instruction is a simple MOV (no conditional updating,
 * not relative addressing, no negation/abs, etc).
 */
static GLboolean
can_downward_mov_be_modifed(const struct prog_instruction *mov)
{
   return
      mov->Opcode == OPCODE_MOV &&
      mov->CondUpdate == GL_FALSE &&
      mov->SrcReg[0].RelAddr == 0 &&
      mov->SrcReg[0].Negate == 0 &&
      mov->SrcReg[0].Abs == 0 &&
      mov->SrcReg[0].HasIndex2 == 0 &&
      mov->SrcReg[0].RelAddr2 == 0 &&
      mov->DstReg.RelAddr == 0 &&
      mov->DstReg.CondMask == COND_TR;
}


static GLboolean
can_upward_mov_be_modifed(const struct prog_instruction *mov)
{
   return
      can_downward_mov_be_modifed(mov) &&
      mov->DstReg.File == PROGRAM_TEMPORARY &&
      mov->SaturateMode == SATURATE_OFF;
}


/**
 * Try to remove use of extraneous MOV instructions, to free them up for dead
 * code removal.
 */
static void
_mesa_remove_extra_move_use(struct gl_program *prog)
{
   GLuint i, j;

   if (dbg) {
      printf("Optimize: Begin remove extra move use\n");
      _mesa_print_program(prog);
   }

   /*
    * Look for sequences such as this:
    *    MOV tmpX, arg0;
    *    ...
    *    FOO tmpY, tmpX, arg1;
    * and convert into:
    *    MOV tmpX, arg0;
    *    ...
    *    FOO tmpY, arg0, arg1;
    */

   for (i = 0; i + 1 < prog->NumInstructions; i++) {
      const struct prog_instruction *mov = prog->Instructions + i;
      GLuint dst_mask, src_mask;
      if (can_upward_mov_be_modifed(mov) == GL_FALSE)
         continue;

      /* Scanning the code, we maintain the components which are still active in
       * these two masks
       */
      dst_mask = mov->DstReg.WriteMask;
      src_mask = get_src_arg_mask(mov, 0, NO_MASK);

      /* Walk through remaining instructions until the or src reg gets
       * rewritten or we get into some flow-control, eliminating the use of
       * this MOV.
       */
      for (j = i + 1; j < prog->NumInstructions; j++) {
	 struct prog_instruction *inst2 = prog->Instructions + j;
         GLuint arg;

	 if (_mesa_is_flow_control_opcode(inst2->Opcode))
	     break;

	 /* First rewrite this instruction's args if appropriate. */
	 for (arg = 0; arg < _mesa_num_inst_src_regs(inst2->Opcode); arg++) {
	    GLuint comp, read_mask;

	    if (inst2->SrcReg[arg].File != mov->DstReg.File ||
		inst2->SrcReg[arg].Index != mov->DstReg.Index ||
		inst2->SrcReg[arg].RelAddr ||
		inst2->SrcReg[arg].Abs)
	       continue;
            read_mask = get_src_arg_mask(inst2, arg, NO_MASK);

	    /* Adjust the swizzles of inst2 to point at MOV's source if ALL the
             * components read still come from the mov instructions
             */
            if (is_swizzle_regular(inst2->SrcReg[arg].Swizzle) &&
               (read_mask & dst_mask) == read_mask) {
               for (comp = 0; comp < 4; comp++) {
                  const GLuint inst2_swz =
                     GET_SWZ(inst2->SrcReg[arg].Swizzle, comp);
                  const GLuint s = GET_SWZ(mov->SrcReg[0].Swizzle, inst2_swz);
                  inst2->SrcReg[arg].Swizzle &= ~(7 << (3 * comp));
                  inst2->SrcReg[arg].Swizzle |= s << (3 * comp);
                  inst2->SrcReg[arg].Negate ^= (((mov->SrcReg[0].Negate >>
                                                  inst2_swz) & 0x1) << comp);
               }
               inst2->SrcReg[arg].File = mov->SrcReg[0].File;
               inst2->SrcReg[arg].Index = mov->SrcReg[0].Index;
            }
	 }

	 /* The source of MOV is written. This potentially deactivates some
          * components from the src and dst of the MOV instruction
          */
	 if (inst2->DstReg.File == mov->DstReg.File &&
	     (inst2->DstReg.RelAddr ||
	      inst2->DstReg.Index == mov->DstReg.Index)) {
            dst_mask &= ~inst2->DstReg.WriteMask;
            src_mask = get_src_arg_mask(mov, 0, dst_mask);
         }

         /* Idem when the destination of mov is written */
	 if (inst2->DstReg.File == mov->SrcReg[0].File &&
	     (inst2->DstReg.RelAddr ||
	      inst2->DstReg.Index == mov->SrcReg[0].Index)) {
            src_mask &= ~inst2->DstReg.WriteMask;
            dst_mask &= get_dst_mask_for_mov(mov, src_mask);
         }
         if (dst_mask == 0)
            break;
      }
   }

   if (dbg) {
      printf("Optimize: End remove extra move use.\n");
      /*_mesa_print_program(prog);*/
   }
}


/**
 * Complements dead_code_global. Try to remove code in block of code by
 * carefully monitoring the swizzles. Both functions should be merged into one
 * with a proper control flow graph
 */
static GLboolean
_mesa_remove_dead_code_local(struct gl_program *prog)
{
   GLboolean *removeInst;
   GLuint i, arg, rem = 0;

   removeInst = (GLboolean *)
      calloc(1, prog->NumInstructions * sizeof(GLboolean));

   for (i = 0; i < prog->NumInstructions; i++) {
      const struct prog_instruction *inst = prog->Instructions + i;
      const GLuint index = inst->DstReg.Index;
      const GLuint mask = inst->DstReg.WriteMask;
      enum inst_use use;

      /* We must deactivate the pass as soon as some indirection is used */
      if (inst->DstReg.RelAddr)
         goto done;
      for (arg = 0; arg < _mesa_num_inst_src_regs(inst->Opcode); arg++)
         if (inst->SrcReg[arg].RelAddr)
            goto done;

      if (_mesa_is_flow_control_opcode(inst->Opcode) ||
          _mesa_num_inst_dst_regs(inst->Opcode) == 0 ||
          inst->DstReg.File != PROGRAM_TEMPORARY ||
          inst->DstReg.RelAddr)
         continue;

      use = find_next_use(prog, i+1, index, mask);
      if (use == WRITE || use == END)
         removeInst[i] = GL_TRUE;
   }

   rem = remove_instructions(prog, removeInst);

done:
   free(removeInst);
   return rem != 0;
}


/**
 * Try to inject the destination of mov as the destination of inst and recompute
 * the swizzles operators for the sources of inst if required. Return GL_TRUE
 * of the substitution was possible, GL_FALSE otherwise
 */
static GLboolean
_mesa_merge_mov_into_inst(struct prog_instruction *inst,
                          const struct prog_instruction *mov)
{
   /* Indirection table which associates destination and source components for
    * the mov instruction
    */
   const GLuint mask = get_src_arg_mask(mov, 0, NO_MASK);

   /* Some components are not written by inst. We cannot remove the mov */
   if (mask != (inst->DstReg.WriteMask & mask))
      return GL_FALSE;

   inst->SaturateMode |= mov->SaturateMode;

   /* Depending on the instruction, we may need to recompute the swizzles.
    * Also, some other instructions (like TEX) are not linear. We will only
    * consider completely active sources and destinations
    */
   switch (inst->Opcode) {

   /* Carstesian instructions: we compute the swizzle */
   case OPCODE_MOV:
   case OPCODE_MIN:
   case OPCODE_MAX:
   case OPCODE_ABS:
   case OPCODE_ADD:
   case OPCODE_MAD:
   case OPCODE_MUL:
   case OPCODE_SUB:
   {
      GLuint dst_to_src_comp[4] = {0,0,0,0};
      GLuint dst_comp, arg;
      for (dst_comp = 0; dst_comp < 4; ++dst_comp) {
         if (mov->DstReg.WriteMask & (1 << dst_comp)) {
            const GLuint src_comp = GET_SWZ(mov->SrcReg[0].Swizzle, dst_comp);
            ASSERT(src_comp < 4);
            dst_to_src_comp[dst_comp] = src_comp;
         }
      }

      /* Patch each source of the instruction */
      for (arg = 0; arg < _mesa_num_inst_src_regs(inst->Opcode); arg++) {
         const GLuint arg_swz = inst->SrcReg[arg].Swizzle;
         inst->SrcReg[arg].Swizzle = 0;

         /* Reset each active component of the swizzle */
         for (dst_comp = 0; dst_comp < 4; ++dst_comp) {
            GLuint src_comp, arg_comp;
            if ((mov->DstReg.WriteMask & (1 << dst_comp)) == 0)
               continue;
            src_comp = dst_to_src_comp[dst_comp];
            ASSERT(src_comp < 4);
            arg_comp = GET_SWZ(arg_swz, src_comp);
            ASSERT(arg_comp < 4);
            inst->SrcReg[arg].Swizzle |= arg_comp << (3*dst_comp);
         }
      }
      inst->DstReg = mov->DstReg;
      return GL_TRUE;
   }

   /* Dot products and scalar instructions: we only change the destination */
   case OPCODE_RCP:
   case OPCODE_SIN:
   case OPCODE_COS:
   case OPCODE_RSQ:
   case OPCODE_POW:
   case OPCODE_EX2:
   case OPCODE_LOG:
   case OPCODE_DP2:
   case OPCODE_DP3:
   case OPCODE_DP4:
      inst->DstReg = mov->DstReg;
      return GL_TRUE;

   /* All other instructions require fully active components with no swizzle */
   default:
      if (mov->SrcReg[0].Swizzle != SWIZZLE_XYZW ||
          inst->DstReg.WriteMask != WRITEMASK_XYZW)
         return GL_FALSE;
      inst->DstReg = mov->DstReg;
      return GL_TRUE;
   }
}


/**
 * Try to remove extraneous MOV instructions from the given program.
 */
static GLboolean
_mesa_remove_extra_moves(struct gl_program *prog)
{
   GLboolean *removeInst; /* per-instruction removal flag */
   GLuint i, rem = 0, nesting = 0;

   if (dbg) {
      printf("Optimize: Begin remove extra moves\n");
      _mesa_print_program(prog);
   }

   removeInst = (GLboolean *)
      calloc(1, prog->NumInstructions * sizeof(GLboolean));

   /*
    * Look for sequences such as this:
    *    FOO tmpX, arg0, arg1;
    *    MOV tmpY, tmpX;
    * and convert into:
    *    FOO tmpY, arg0, arg1;
    */

   for (i = 0; i < prog->NumInstructions; i++) {
      const struct prog_instruction *mov = prog->Instructions + i;

      switch (mov->Opcode) {
      case OPCODE_BGNLOOP:
      case OPCODE_BGNSUB:
      case OPCODE_IF:
         nesting++;
         break;
      case OPCODE_ENDLOOP:
      case OPCODE_ENDSUB:
      case OPCODE_ENDIF:
         nesting--;
         break;
      case OPCODE_MOV:
         if (i > 0 &&
             can_downward_mov_be_modifed(mov) &&
             mov->SrcReg[0].File == PROGRAM_TEMPORARY &&
             nesting == 0)
         {

            /* see if this MOV can be removed */
            const GLuint id = mov->SrcReg[0].Index;
            struct prog_instruction *prevInst;
            GLuint prevI;

            /* get pointer to previous instruction */
            prevI = i - 1;
            while (prevI > 0 && removeInst[prevI])
               prevI--;
            prevInst = prog->Instructions + prevI;

            if (prevInst->DstReg.File == PROGRAM_TEMPORARY &&
                prevInst->DstReg.Index == id &&
                prevInst->DstReg.RelAddr == 0 &&
                prevInst->DstReg.CondSrc == 0 && 
                prevInst->DstReg.CondMask == COND_TR) {

               const GLuint dst_mask = prevInst->DstReg.WriteMask;
               enum inst_use next_use = find_next_use(prog, i+1, id, dst_mask);

               if (next_use == WRITE || next_use == END) {
                  /* OK, we can safely remove this MOV instruction.
                   * Transform:
                   *   prevI: FOO tempIndex, x, y;
                   *       i: MOV z, tempIndex;
                   * Into:
                   *   prevI: FOO z, x, y;
                   */
                  if (_mesa_merge_mov_into_inst(prevInst, mov)) {
                     removeInst[i] = GL_TRUE;
                     if (dbg) {
                        printf("Remove MOV at %u\n", i);
                        printf("new prev inst %u: ", prevI);
                        _mesa_print_instruction(prevInst);
                     }
                  }
               }
            }
         }
         break;
      default:
         ; /* nothing */
      }
   }

   /* now remove the instructions which aren't needed */
   rem = remove_instructions(prog, removeInst);

   free(removeInst);

   if (dbg) {
      printf("Optimize: End remove extra moves.  %u instructions removed\n", rem);
      /*_mesa_print_program(prog);*/
   }

   return rem != 0;
}


/** A live register interval */
struct interval
{
   GLuint Reg;         /** The temporary register index */
   GLuint Start, End;  /** Start/end instruction numbers */
};


/** A list of register intervals */
struct interval_list
{
   GLuint Num;
   struct interval Intervals[REG_ALLOCATE_MAX_PROGRAM_TEMPS];
};


static void
append_interval(struct interval_list *list, const struct interval *inv)
{
   list->Intervals[list->Num++] = *inv;
}


/** Insert interval inv into list, sorted by interval end */
static void
insert_interval_by_end(struct interval_list *list, const struct interval *inv)
{
   /* XXX we could do a binary search insertion here since list is sorted */
   GLint i = list->Num - 1;
   while (i >= 0 && list->Intervals[i].End > inv->End) {
      list->Intervals[i + 1] = list->Intervals[i];
      i--;
   }
   list->Intervals[i + 1] = *inv;
   list->Num++;

#ifdef DEBUG
   {
      GLuint i;
      for (i = 0; i + 1 < list->Num; i++) {
         ASSERT(list->Intervals[i].End <= list->Intervals[i + 1].End);
      }
   }
#endif
}


/** Remove the given interval from the interval list */
static void
remove_interval(struct interval_list *list, const struct interval *inv)
{
   /* XXX we could binary search since list is sorted */
   GLuint k;
   for (k = 0; k < list->Num; k++) {
      if (list->Intervals[k].Reg == inv->Reg) {
         /* found, remove it */
         ASSERT(list->Intervals[k].Start == inv->Start);
         ASSERT(list->Intervals[k].End == inv->End);
         while (k < list->Num - 1) {
            list->Intervals[k] = list->Intervals[k + 1];
            k++;
         }
         list->Num--;
         return;
      }
   }
}


/** called by qsort() */
static int
compare_start(const void *a, const void *b)
{
   const struct interval *ia = (const struct interval *) a;
   const struct interval *ib = (const struct interval *) b;
   if (ia->Start < ib->Start)
      return -1;
   else if (ia->Start > ib->Start)
      return +1;
   else
      return 0;
}


/** sort the interval list according to interval starts */
static void
sort_interval_list_by_start(struct interval_list *list)
{
   qsort(list->Intervals, list->Num, sizeof(struct interval), compare_start);
#ifdef DEBUG
   {
      GLuint i;
      for (i = 0; i + 1 < list->Num; i++) {
         ASSERT(list->Intervals[i].Start <= list->Intervals[i + 1].Start);
      }
   }
#endif
}

struct loop_info
{
   GLuint Start, End;  /**< Start, end instructions of loop */
};

/**
 * Update the intermediate interval info for register 'index' and
 * instruction 'ic'.
 */
static void
update_interval(GLint intBegin[], GLint intEnd[],
		struct loop_info *loopStack, GLuint loopStackDepth,
		GLuint index, GLuint ic)
{
   int i;
   GLuint begin = ic;
   GLuint end = ic;

   /* If the register is used in a loop, extend its lifetime through the end
    * of the outermost loop that doesn't contain its definition.
    */
   for (i = 0; i < loopStackDepth; i++) {
      if (intBegin[index] < loopStack[i].Start) {
	 end = loopStack[i].End;
	 break;
      }
   }

   /* Variables that are live at the end of a loop will also be live at the
    * beginning, so an instruction inside of a loop should have its live
    * interval begin at the start of the outermost loop.
    */
   if (loopStackDepth > 0 && ic > loopStack[0].Start && ic < loopStack[0].End) {
      begin = loopStack[0].Start;
   }

   ASSERT(index < REG_ALLOCATE_MAX_PROGRAM_TEMPS);
   if (intBegin[index] == -1) {
      ASSERT(intEnd[index] == -1);
      intBegin[index] = begin;
      intEnd[index] = end;
   }
   else {
      intEnd[index] = end;
   }
}


/**
 * Find first/last instruction that references each temporary register.
 */
GLboolean
_mesa_find_temp_intervals(const struct prog_instruction *instructions,
                          GLuint numInstructions,
                          GLint intBegin[REG_ALLOCATE_MAX_PROGRAM_TEMPS],
                          GLint intEnd[REG_ALLOCATE_MAX_PROGRAM_TEMPS])
{
   struct loop_info loopStack[MAX_LOOP_NESTING];
   GLuint loopStackDepth = 0;
   GLuint i;

   for (i = 0; i < REG_ALLOCATE_MAX_PROGRAM_TEMPS; i++){
      intBegin[i] = intEnd[i] = -1;
   }

   /* Scan instructions looking for temporary registers */
   for (i = 0; i < numInstructions; i++) {
      const struct prog_instruction *inst = instructions + i;
      if (inst->Opcode == OPCODE_BGNLOOP) {
         loopStack[loopStackDepth].Start = i;
         loopStack[loopStackDepth].End = inst->BranchTarget;
         loopStackDepth++;
      }
      else if (inst->Opcode == OPCODE_ENDLOOP) {
         loopStackDepth--;
      }
      else if (inst->Opcode == OPCODE_CAL) {
         return GL_FALSE;
      }
      else {
         const GLuint numSrc = 3;/*_mesa_num_inst_src_regs(inst->Opcode);*/
         GLuint j;
         for (j = 0; j < numSrc; j++) {
            if (inst->SrcReg[j].File == PROGRAM_TEMPORARY) {
               const GLuint index = inst->SrcReg[j].Index;
               if (inst->SrcReg[j].RelAddr)
                  return GL_FALSE;
               update_interval(intBegin, intEnd, loopStack, loopStackDepth,
			       index, i);
            }
         }
         if (inst->DstReg.File == PROGRAM_TEMPORARY) {
            const GLuint index = inst->DstReg.Index;
            if (inst->DstReg.RelAddr)
               return GL_FALSE;
            update_interval(intBegin, intEnd, loopStack, loopStackDepth,
			    index, i);
         }
      }
   }

   return GL_TRUE;
}


/**
 * Find the live intervals for each temporary register in the program.
 * For register R, the interval [A,B] indicates that R is referenced
 * from instruction A through instruction B.
 * Special consideration is needed for loops and subroutines.
 * \return GL_TRUE if success, GL_FALSE if we cannot proceed for some reason
 */
static GLboolean
find_live_intervals(struct gl_program *prog,
                    struct interval_list *liveIntervals)
{
   GLint intBegin[REG_ALLOCATE_MAX_PROGRAM_TEMPS];
   GLint intEnd[REG_ALLOCATE_MAX_PROGRAM_TEMPS];
   GLuint i;

   /*
    * Note: we'll return GL_FALSE below if we find relative indexing
    * into the TEMP register file.  We can't handle that yet.
    * We also give up on subroutines for now.
    */

   if (dbg) {
      printf("Optimize: Begin find intervals\n");
   }

   /* build intermediate arrays */
   if (!_mesa_find_temp_intervals(prog->Instructions, prog->NumInstructions,
                                  intBegin, intEnd))
      return GL_FALSE;

   /* Build live intervals list from intermediate arrays */
   liveIntervals->Num = 0;
   for (i = 0; i < REG_ALLOCATE_MAX_PROGRAM_TEMPS; i++) {
      if (intBegin[i] >= 0) {
         struct interval inv;
         inv.Reg = i;
         inv.Start = intBegin[i];
         inv.End = intEnd[i];
         append_interval(liveIntervals, &inv);
      }
   }

   /* Sort the list according to interval starts */
   sort_interval_list_by_start(liveIntervals);

   if (dbg) {
      /* print interval info */
      for (i = 0; i < liveIntervals->Num; i++) {
         const struct interval *inv = liveIntervals->Intervals + i;
         printf("Reg[%d] live [%d, %d]:",
                      inv->Reg, inv->Start, inv->End);
         if (1) {
            GLuint j;
            for (j = 0; j < inv->Start; j++)
               printf(" ");
            for (j = inv->Start; j <= inv->End; j++)
               printf("x");
         }
         printf("\n");
      }
   }

   return GL_TRUE;
}


/** Scan the array of used register flags to find free entry */
static GLint
alloc_register(GLboolean usedRegs[REG_ALLOCATE_MAX_PROGRAM_TEMPS])
{
   GLuint k;
   for (k = 0; k < REG_ALLOCATE_MAX_PROGRAM_TEMPS; k++) {
      if (!usedRegs[k]) {
         usedRegs[k] = GL_TRUE;
         return k;
      }
   }
   return -1;
}


/**
 * This function implements "Linear Scan Register Allocation" to reduce
 * the number of temporary registers used by the program.
 *
 * We compute the "live interval" for all temporary registers then
 * examine the overlap of the intervals to allocate new registers.
 * Basically, if two intervals do not overlap, they can use the same register.
 */
static void
_mesa_reallocate_registers(struct gl_program *prog)
{
   struct interval_list liveIntervals;
   GLint registerMap[REG_ALLOCATE_MAX_PROGRAM_TEMPS];
   GLboolean usedRegs[REG_ALLOCATE_MAX_PROGRAM_TEMPS];
   GLuint i;
   GLint maxTemp = -1;

   if (dbg) {
      printf("Optimize: Begin live-interval register reallocation\n");
      _mesa_print_program(prog);
   }

   for (i = 0; i < REG_ALLOCATE_MAX_PROGRAM_TEMPS; i++){
      registerMap[i] = -1;
      usedRegs[i] = GL_FALSE;
   }

   if (!find_live_intervals(prog, &liveIntervals)) {
      if (dbg)
         printf("Aborting register reallocation\n");
      return;
   }

   {
      struct interval_list activeIntervals;
      activeIntervals.Num = 0;

      /* loop over live intervals, allocating a new register for each */
      for (i = 0; i < liveIntervals.Num; i++) {
         const struct interval *live = liveIntervals.Intervals + i;

         if (dbg)
            printf("Consider register %u\n", live->Reg);

         /* Expire old intervals.  Intervals which have ended with respect
          * to the live interval can have their remapped registers freed.
          */
         {
            GLint j;
            for (j = 0; j < (GLint) activeIntervals.Num; j++) {
               const struct interval *inv = activeIntervals.Intervals + j;
               if (inv->End >= live->Start) {
                  /* Stop now.  Since the activeInterval list is sorted
                   * we know we don't have to go further.
                   */
                  break;
               }
               else {
                  /* Interval 'inv' has expired */
                  const GLint regNew = registerMap[inv->Reg];
                  ASSERT(regNew >= 0);

                  if (dbg)
                     printf("  expire interval for reg %u\n", inv->Reg);

                  /* remove interval j from active list */
                  remove_interval(&activeIntervals, inv);
                  j--;  /* counter-act j++ in for-loop above */

                  /* return register regNew to the free pool */
                  if (dbg)
                     printf("  free reg %d\n", regNew);
                  ASSERT(usedRegs[regNew] == GL_TRUE);
                  usedRegs[regNew] = GL_FALSE;
               }
            }
         }

         /* find a free register for this live interval */
         {
            const GLint k = alloc_register(usedRegs);
            if (k < 0) {
               /* out of registers, give up */
               return;
            }
            registerMap[live->Reg] = k;
            maxTemp = MAX2(maxTemp, k);
            if (dbg)
               printf("  remap register %u -> %d\n", live->Reg, k);
         }

         /* Insert this live interval into the active list which is sorted
          * by increasing end points.
          */
         insert_interval_by_end(&activeIntervals, live);
      }
   }

   if (maxTemp + 1 < (GLint) liveIntervals.Num) {
      /* OK, we've reduced the number of registers needed.
       * Scan the program and replace all the old temporary register
       * indexes with the new indexes.
       */
      replace_regs(prog, PROGRAM_TEMPORARY, registerMap);

      prog->NumTemporaries = maxTemp + 1;
   }

   if (dbg) {
      printf("Optimize: End live-interval register reallocation\n");
      printf("Num temp regs before: %u  after: %u\n",
                   liveIntervals.Num, maxTemp + 1);
      _mesa_print_program(prog);
   }
}


#if 0
static void
print_it(struct gl_context *ctx, struct gl_program *program, const char *txt) {
   fprintf(stderr, "%s (%u inst):\n", txt, program->NumInstructions);
   _mesa_print_program(program);
   _mesa_print_program_parameters(ctx, program);
   fprintf(stderr, "\n\n");
}
#endif

/**
 * This pass replaces CMP T0, T1 T2 T0 with MOV T0, T2 when the CMP
 * instruction is the first instruction to write to register T0.  The are
 * several lowering passes done in GLSL IR (e.g. branches and
 * relative addressing) that create a large number of conditional assignments
 * that ir_to_mesa converts to CMP instructions like the one mentioned above.
 *
 * Here is why this conversion is safe:
 * CMP T0, T1 T2 T0 can be expanded to:
 * if (T1 < 0.0)
 * 	MOV T0, T2;
 * else
 * 	MOV T0, T0;
 *
 * If (T1 < 0.0) evaluates to true then our replacement MOV T0, T2 is the same
 * as the original program.  If (T1 < 0.0) evaluates to false, executing
 * MOV T0, T0 will store a garbage value in T0 since T0 is uninitialized.
 * Therefore, it doesn't matter that we are replacing MOV T0, T0 with MOV T0, T2
 * because any instruction that was going to read from T0 after this was going
 * to read a garbage value anyway.
 */
static void
_mesa_simplify_cmp(struct gl_program * program)
{
   GLuint tempWrites[REG_ALLOCATE_MAX_PROGRAM_TEMPS];
   GLuint outputWrites[MAX_PROGRAM_OUTPUTS];
   GLuint i;

   if (dbg) {
      printf("Optimize: Begin reads without writes\n");
      _mesa_print_program(program);
   }

   for (i = 0; i < REG_ALLOCATE_MAX_PROGRAM_TEMPS; i++) {
      tempWrites[i] = 0;
   }

   for (i = 0; i < MAX_PROGRAM_OUTPUTS; i++) {
      outputWrites[i] = 0;
   }

   for (i = 0; i < program->NumInstructions; i++) {
      struct prog_instruction *inst = program->Instructions + i;
      GLuint prevWriteMask;

      /* Give up if we encounter relative addressing or flow control. */
      if (_mesa_is_flow_control_opcode(inst->Opcode) || inst->DstReg.RelAddr) {
         return;
      }

      if (inst->DstReg.File == PROGRAM_OUTPUT) {
         assert(inst->DstReg.Index < MAX_PROGRAM_OUTPUTS);
         prevWriteMask = outputWrites[inst->DstReg.Index];
         outputWrites[inst->DstReg.Index] |= inst->DstReg.WriteMask;
      } else if (inst->DstReg.File == PROGRAM_TEMPORARY) {
         assert(inst->DstReg.Index < REG_ALLOCATE_MAX_PROGRAM_TEMPS);
         prevWriteMask = tempWrites[inst->DstReg.Index];
         tempWrites[inst->DstReg.Index] |= inst->DstReg.WriteMask;
      } else {
         /* No other register type can be a destination register. */
         continue;
      }

      /* For a CMP to be considered a conditional write, the destination
       * register and source register two must be the same. */
      if (inst->Opcode == OPCODE_CMP
          && !(inst->DstReg.WriteMask & prevWriteMask)
          && inst->SrcReg[2].File == inst->DstReg.File
          && inst->SrcReg[2].Index == inst->DstReg.Index
          && inst->DstReg.WriteMask == get_src_arg_mask(inst, 2, NO_MASK)) {

         inst->Opcode = OPCODE_MOV;
         inst->SrcReg[0] = inst->SrcReg[1];

	 /* Unused operands are expected to have the file set to
	  * PROGRAM_UNDEFINED.  This is how _mesa_init_instructions initializes
	  * all of the sources.
	  */
	 inst->SrcReg[1].File = PROGRAM_UNDEFINED;
	 inst->SrcReg[1].Swizzle = SWIZZLE_NOOP;
	 inst->SrcReg[2].File = PROGRAM_UNDEFINED;
	 inst->SrcReg[2].Swizzle = SWIZZLE_NOOP;
      }
   }
   if (dbg) {
      printf("Optimize: End reads without writes\n");
      _mesa_print_program(program);
   }
}

/**
 * Apply optimizations to the given program to eliminate unnecessary
 * instructions, temp regs, etc.
 */
void
_mesa_optimize_program(struct gl_context *ctx, struct gl_program *program)
{
   GLboolean any_change;

   _mesa_simplify_cmp(program);
   /* Stop when no modifications were output */
   do {
      any_change = GL_FALSE;
      _mesa_remove_extra_move_use(program);
      if (_mesa_remove_dead_code_global(program))
         any_change = GL_TRUE;
      if (_mesa_remove_extra_moves(program))
         any_change = GL_TRUE;
      if (_mesa_remove_dead_code_local(program))
         any_change = GL_TRUE;

      any_change = _mesa_constant_fold(program) || any_change;
      _mesa_reallocate_registers(program);
   } while (any_change);
}


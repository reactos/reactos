/**************************************************************************
 * 
 * Copyright 2007-2008 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * Copyright 2009-2010 VMware, Inc.  All rights Reserved.
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

#ifndef TGSI_EXEC_H
#define TGSI_EXEC_H

#include "pipe/p_compiler.h"
#include "pipe/p_state.h"
#include "pipe/p_shader_tokens.h"

#if defined __cplusplus
extern "C" {
#endif


#define NUM_CHANNELS 4  /* R,G,B,A */
#define QUAD_SIZE    4  /* 4 pixel/quad */


/**
  * Registers may be treated as float, signed int or unsigned int.
  */
union tgsi_exec_channel
{
   float    f[QUAD_SIZE];
   int      i[QUAD_SIZE];
   unsigned u[QUAD_SIZE];
};

/**
  * A vector[RGBA] of channels[4 pixels]
  */
struct tgsi_exec_vector
{
   union tgsi_exec_channel xyzw[NUM_CHANNELS];
};

/**
 * For fragment programs, information for computing fragment input
 * values from plane equation of the triangle/line.
 */
struct tgsi_interp_coef
{
   float a0[NUM_CHANNELS];	/* in an xyzw layout */
   float dadx[NUM_CHANNELS];
   float dady[NUM_CHANNELS];
};

enum tgsi_sampler_control {
   tgsi_sampler_lod_bias,
   tgsi_sampler_lod_explicit
};

/**
 * Information for sampling textures, which must be implemented
 * by code outside the TGSI executor.
 */
struct tgsi_sampler
{
   /** Get samples for four fragments in a quad */
   void (*get_samples)(struct tgsi_sampler *sampler,
                       const float s[QUAD_SIZE],
                       const float t[QUAD_SIZE],
                       const float p[QUAD_SIZE],
                       const float c0[QUAD_SIZE],
                       enum tgsi_sampler_control control,
                       float rgba[NUM_CHANNELS][QUAD_SIZE]);
   void (*get_dims)(struct tgsi_sampler *sampler, int level,
		    int dims[4]);
   void (*get_texel)(struct tgsi_sampler *sampler, const int i[QUAD_SIZE],
		     const int j[QUAD_SIZE], const int k[QUAD_SIZE],
		     const int lod[QUAD_SIZE], const int8_t offset[3],
		     float rgba[NUM_CHANNELS][QUAD_SIZE]);
};

#define TGSI_EXEC_NUM_TEMPS       128
#define TGSI_EXEC_NUM_IMMEDIATES  256
#define TGSI_EXEC_NUM_TEMP_ARRAYS 8

/*
 * Locations of various utility registers (_I = Index, _C = Channel)
 */
#define TGSI_EXEC_TEMP_00000000_I   (TGSI_EXEC_NUM_TEMPS + 0)
#define TGSI_EXEC_TEMP_00000000_C   0

#define TGSI_EXEC_TEMP_7FFFFFFF_I   (TGSI_EXEC_NUM_TEMPS + 0)
#define TGSI_EXEC_TEMP_7FFFFFFF_C   1

#define TGSI_EXEC_TEMP_80000000_I   (TGSI_EXEC_NUM_TEMPS + 0)
#define TGSI_EXEC_TEMP_80000000_C   2

#define TGSI_EXEC_TEMP_FFFFFFFF_I   (TGSI_EXEC_NUM_TEMPS + 0)
#define TGSI_EXEC_TEMP_FFFFFFFF_C   3

#define TGSI_EXEC_TEMP_ONE_I        (TGSI_EXEC_NUM_TEMPS + 1)
#define TGSI_EXEC_TEMP_ONE_C        0

#define TGSI_EXEC_TEMP_TWO_I        (TGSI_EXEC_NUM_TEMPS + 1)
#define TGSI_EXEC_TEMP_TWO_C        1

#define TGSI_EXEC_TEMP_128_I        (TGSI_EXEC_NUM_TEMPS + 1)
#define TGSI_EXEC_TEMP_128_C        2

#define TGSI_EXEC_TEMP_MINUS_128_I  (TGSI_EXEC_NUM_TEMPS + 1)
#define TGSI_EXEC_TEMP_MINUS_128_C  3

#define TGSI_EXEC_TEMP_KILMASK_I    (TGSI_EXEC_NUM_TEMPS + 2)
#define TGSI_EXEC_TEMP_KILMASK_C    0

#define TGSI_EXEC_TEMP_OUTPUT_I     (TGSI_EXEC_NUM_TEMPS + 2)
#define TGSI_EXEC_TEMP_OUTPUT_C     1

#define TGSI_EXEC_TEMP_PRIMITIVE_I  (TGSI_EXEC_NUM_TEMPS + 2)
#define TGSI_EXEC_TEMP_PRIMITIVE_C  2

#define TGSI_EXEC_TEMP_THREE_I      (TGSI_EXEC_NUM_TEMPS + 2)
#define TGSI_EXEC_TEMP_THREE_C      3

#define TGSI_EXEC_TEMP_HALF_I       (TGSI_EXEC_NUM_TEMPS + 3)
#define TGSI_EXEC_TEMP_HALF_C       0

/* execution mask, each value is either 0 or ~0 */
#define TGSI_EXEC_MASK_I            (TGSI_EXEC_NUM_TEMPS + 3)
#define TGSI_EXEC_MASK_C            1

/* 4 register buffer for various purposes */
#define TGSI_EXEC_TEMP_R0           (TGSI_EXEC_NUM_TEMPS + 4)
#define TGSI_EXEC_NUM_TEMP_R        4

#define TGSI_EXEC_TEMP_ADDR         (TGSI_EXEC_NUM_TEMPS + 8)
#define TGSI_EXEC_NUM_ADDRS         1

/* predicate register */
#define TGSI_EXEC_TEMP_P0           (TGSI_EXEC_NUM_TEMPS + 9)
#define TGSI_EXEC_NUM_PREDS         1

#define TGSI_EXEC_NUM_TEMP_EXTRAS   10



#define TGSI_EXEC_MAX_NESTING  32
#define TGSI_EXEC_MAX_COND_NESTING  TGSI_EXEC_MAX_NESTING
#define TGSI_EXEC_MAX_LOOP_NESTING  TGSI_EXEC_MAX_NESTING
#define TGSI_EXEC_MAX_SWITCH_NESTING TGSI_EXEC_MAX_NESTING
#define TGSI_EXEC_MAX_CALL_NESTING  TGSI_EXEC_MAX_NESTING

/* The maximum number of input attributes per vertex. For 2D
 * input register files, this is the stride between two 1D
 * arrays.
 */
#define TGSI_EXEC_MAX_INPUT_ATTRIBS 17

/* The maximum number of constant vectors per constant buffer.
 */
#define TGSI_EXEC_MAX_CONST_BUFFER  4096

/* The maximum number of vertices per primitive */
#define TGSI_MAX_PRIM_VERTICES 6

/* The maximum number of primitives to be generated */
#define TGSI_MAX_PRIMITIVES 64

/* The maximum total number of vertices */
#define TGSI_MAX_TOTAL_VERTICES (TGSI_MAX_PRIM_VERTICES * TGSI_MAX_PRIMITIVES * PIPE_MAX_ATTRIBS)

#define TGSI_MAX_MISC_INPUTS 8

/** function call/activation record */
struct tgsi_call_record
{
   uint CondStackTop;
   uint LoopStackTop;
   uint ContStackTop;
   int SwitchStackTop;
   int BreakStackTop;
   uint ReturnAddr;
};


/* Switch-case block state. */
struct tgsi_switch_record {
   uint mask;                          /**< execution mask */
   union tgsi_exec_channel selector;   /**< a value case statements are compared to */
   uint defaultMask;                   /**< non-execute mask for default case */
};


enum tgsi_break_type {
   TGSI_EXEC_BREAK_INSIDE_LOOP,
   TGSI_EXEC_BREAK_INSIDE_SWITCH
};


#define TGSI_EXEC_MAX_BREAK_STACK (TGSI_EXEC_MAX_LOOP_NESTING + TGSI_EXEC_MAX_SWITCH_NESTING)


/**
 * Run-time virtual machine state for executing TGSI shader.
 */
struct tgsi_exec_machine
{
   /* Total = program temporaries + internal temporaries
    */
   struct tgsi_exec_vector       Temps[TGSI_EXEC_NUM_TEMPS +
                                       TGSI_EXEC_NUM_TEMP_EXTRAS];
   struct tgsi_exec_vector       TempArray[TGSI_EXEC_NUM_TEMP_ARRAYS][TGSI_EXEC_NUM_TEMPS];

   float                         Imms[TGSI_EXEC_NUM_IMMEDIATES][4];

   float                         ImmArray[TGSI_EXEC_NUM_IMMEDIATES][4];

   struct tgsi_exec_vector       *Inputs;
   struct tgsi_exec_vector       *Outputs;

   /* System values */
   unsigned                      SysSemanticToIndex[TGSI_SEMANTIC_COUNT];
   union tgsi_exec_channel       SystemValue[TGSI_MAX_MISC_INPUTS];

   struct tgsi_exec_vector       *Addrs;
   struct tgsi_exec_vector       *Predicates;

   struct tgsi_sampler           **Samplers;

   unsigned                      ImmLimit;

   const void *Consts[PIPE_MAX_CONSTANT_BUFFERS];
   unsigned ConstsSize[PIPE_MAX_CONSTANT_BUFFERS];

   const struct tgsi_token       *Tokens;   /**< Declarations, instructions */
   unsigned                      Processor; /**< TGSI_PROCESSOR_x */

   /* GEOMETRY processor only. */
   unsigned                      *Primitives;
   unsigned                       NumOutputs;
   unsigned                       MaxGeometryShaderOutputs;

   /* FRAGMENT processor only. */
   const struct tgsi_interp_coef *InterpCoefs;
   struct tgsi_exec_vector       QuadPos;
   float                         Face;    /**< +1 if front facing, -1 if back facing */
   bool                          flatshade_color;
   /* Conditional execution masks */
   uint CondMask;  /**< For IF/ELSE/ENDIF */
   uint LoopMask;  /**< For BGNLOOP/ENDLOOP */
   uint ContMask;  /**< For loop CONT statements */
   uint FuncMask;  /**< For function calls */
   uint ExecMask;  /**< = CondMask & LoopMask */

   /* Current switch-case state. */
   struct tgsi_switch_record Switch;

   /* Current break type. */
   enum tgsi_break_type BreakType;

   /** Condition mask stack (for nested conditionals) */
   uint CondStack[TGSI_EXEC_MAX_COND_NESTING];
   int CondStackTop;

   /** Loop mask stack (for nested loops) */
   uint LoopStack[TGSI_EXEC_MAX_LOOP_NESTING];
   int LoopStackTop;

   /** Loop label stack */
   uint LoopLabelStack[TGSI_EXEC_MAX_LOOP_NESTING];
   int LoopLabelStackTop;

   /** Loop continue mask stack (see comments in tgsi_exec.c) */
   uint ContStack[TGSI_EXEC_MAX_LOOP_NESTING];
   int ContStackTop;

   /** Switch case stack */
   struct tgsi_switch_record SwitchStack[TGSI_EXEC_MAX_SWITCH_NESTING];
   int SwitchStackTop;

   enum tgsi_break_type BreakStack[TGSI_EXEC_MAX_BREAK_STACK];
   int BreakStackTop;

   /** Function execution mask stack (for executing subroutine code) */
   uint FuncStack[TGSI_EXEC_MAX_CALL_NESTING];
   int FuncStackTop;

   /** Function call stack for saving/restoring the program counter */
   struct tgsi_call_record CallStack[TGSI_EXEC_MAX_CALL_NESTING];
   int CallStackTop;

   struct tgsi_full_instruction *Instructions;
   uint NumInstructions;

   struct tgsi_full_declaration *Declarations;
   uint NumDeclarations;

   struct tgsi_declaration_resource Resources[PIPE_MAX_SHADER_RESOURCES];

   boolean UsedGeometryShader;
};

struct tgsi_exec_machine *
tgsi_exec_machine_create( void );

void
tgsi_exec_machine_destroy(struct tgsi_exec_machine *mach);


void 
tgsi_exec_machine_bind_shader(
   struct tgsi_exec_machine *mach,
   const struct tgsi_token *tokens,
   uint numSamplers,
   struct tgsi_sampler **samplers);

uint
tgsi_exec_machine_run(
   struct tgsi_exec_machine *mach );


void
tgsi_exec_machine_free_data(struct tgsi_exec_machine *mach);


boolean
tgsi_check_soa_dependencies(const struct tgsi_full_instruction *inst);


static INLINE void
tgsi_set_kill_mask(struct tgsi_exec_machine *mach, unsigned mask)
{
   mach->Temps[TGSI_EXEC_TEMP_KILMASK_I].xyzw[TGSI_EXEC_TEMP_KILMASK_C].u[0] =
      mask;
}


/** Set execution mask values prior to executing the shader */
static INLINE void
tgsi_set_exec_mask(struct tgsi_exec_machine *mach,
                   boolean ch0, boolean ch1, boolean ch2, boolean ch3)
{
   int *mask = mach->Temps[TGSI_EXEC_MASK_I].xyzw[TGSI_EXEC_MASK_C].i;
   mask[0] = ch0 ? ~0 : 0;
   mask[1] = ch1 ? ~0 : 0;
   mask[2] = ch2 ? ~0 : 0;
   mask[3] = ch3 ? ~0 : 0;
}


extern void
tgsi_exec_set_constant_buffers(struct tgsi_exec_machine *mach,
                               unsigned num_bufs,
                               const void **bufs,
                               const unsigned *buf_sizes);


static INLINE int
tgsi_exec_get_shader_param(enum pipe_shader_cap param)
{
   switch(param) {
   case PIPE_SHADER_CAP_MAX_INSTRUCTIONS:
   case PIPE_SHADER_CAP_MAX_ALU_INSTRUCTIONS:
   case PIPE_SHADER_CAP_MAX_TEX_INSTRUCTIONS:
   case PIPE_SHADER_CAP_MAX_TEX_INDIRECTIONS:
      return INT_MAX;
   case PIPE_SHADER_CAP_MAX_CONTROL_FLOW_DEPTH:
      return TGSI_EXEC_MAX_NESTING;
   case PIPE_SHADER_CAP_MAX_INPUTS:
      return TGSI_EXEC_MAX_INPUT_ATTRIBS;
   case PIPE_SHADER_CAP_MAX_CONSTS:
      return TGSI_EXEC_MAX_CONST_BUFFER;
   case PIPE_SHADER_CAP_MAX_CONST_BUFFERS:
      return PIPE_MAX_CONSTANT_BUFFERS;
   case PIPE_SHADER_CAP_MAX_TEMPS:
      return TGSI_EXEC_NUM_TEMPS;
   case PIPE_SHADER_CAP_MAX_ADDRS:
      return TGSI_EXEC_NUM_ADDRS;
   case PIPE_SHADER_CAP_MAX_PREDS:
      return TGSI_EXEC_NUM_PREDS;
   case PIPE_SHADER_CAP_TGSI_CONT_SUPPORTED:
      return 1;
   case PIPE_SHADER_CAP_INDIRECT_INPUT_ADDR:
   case PIPE_SHADER_CAP_INDIRECT_OUTPUT_ADDR:
   case PIPE_SHADER_CAP_INDIRECT_TEMP_ADDR:
   case PIPE_SHADER_CAP_INDIRECT_CONST_ADDR:
      return 1;
   case PIPE_SHADER_CAP_SUBROUTINES:
      return 1;
   case PIPE_SHADER_CAP_INTEGERS:
      return 1;
   case PIPE_SHADER_CAP_MAX_TEXTURE_SAMPLERS:
      return PIPE_MAX_SAMPLERS;
   default:
      return 0;
   }
}

#if defined __cplusplus
} /* extern "C" */
#endif

#endif /* TGSI_EXEC_H */

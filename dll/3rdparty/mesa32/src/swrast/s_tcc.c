/*
 * Mesa 3-D graphics library
 * Version:  6.1
 *
 * Copyright (C) 1999-2004  Brian Paul   All Rights Reserved.
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

/* An attempt to hook s_fragprog_to_c.c up to libtcc.a to try &
 * generate some real code.
 *
 * TCC isn't threadsafe, so it will need additional locking help if we
 * end up using it as a backend in mesa.
 */

#include <stdlib.h>
#include <stdio.h>


#include "glheader.h"
#include "colormac.h"
#include "context.h"
#include "nvfragprog.h"
#include "macros.h"
#include "program.h"

#include "s_nvfragprog.h"
#include "s_texture.h"

#ifdef USE_TCC

#include <libtcc.h>

typedef int (*cfunc)( void *ctx, 
		      const GLfloat (*local_param)[4], 
		      const GLfloat (*env_param)[4], 
		      const struct program_parameter *state_param, 
		      const GLfloat (*interp)[4], 
		      GLfloat (*outputs)[4]);


static cfunc current_func;
static struct fragment_program *current_program;
static TCCState *current_tcc_state;


static void TEX( void *cc, const float *texcoord, int unit, float *result )
{
   GLcontext *ctx = (GLcontext *)cc;
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   GLfloat lambda = 1.0;	/* hack */
   GLchan rgba[4];

   swrast->TextureSample[unit](ctx, unit, ctx->Texture.Unit[unit]._Current,
                               1, (const GLfloat (*)[4]) texcoord,
                               &lambda, &rgba);

   result[0] = CHAN_TO_FLOAT(rgba[0]);
   result[1] = CHAN_TO_FLOAT(rgba[1]);
   result[2] = CHAN_TO_FLOAT(rgba[2]);
   result[3] = CHAN_TO_FLOAT(rgba[3]);
}


static void TXB( void *cc, const float *texcoord, int unit, float *result )
{
   GLcontext *ctx = (GLcontext *)cc;
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   GLfloat lambda = 1.0;	/* hack */
   GLchan rgba[4];

   /* texcoord[3] is the bias to add to lambda */
   lambda += texcoord[3];


   /* Is it necessary to reset texcoord[3] to 1 at this point?
    */
   swrast->TextureSample[unit](ctx, unit, ctx->Texture.Unit[unit]._Current,
                               1, (const GLfloat (*)[4]) texcoord,
                               &lambda, &rgba);

   result[0] = CHAN_TO_FLOAT(rgba[0]);
   result[1] = CHAN_TO_FLOAT(rgba[1]);
   result[2] = CHAN_TO_FLOAT(rgba[2]);
   result[3] = CHAN_TO_FLOAT(rgba[3]);
}


static void TXP( void *cc, const float *texcoord, int unit, float *result )
{
   /* I think that TEX needs to undo the perspective divide which has
    * already occurred.  In the meantime, TXP is correct to do this:
    */
   TEX( cc, texcoord, unit, result );
}


static cfunc codegen( TCCState *s, const char *prog, const char *fname )
{
    unsigned long val;

    if (s) 
       tcc_delete(s);
    
    s = tcc_new();
    if (!s) 
       return 0;

    tcc_set_output_type(s, TCC_OUTPUT_MEMORY);
    tcc_compile_string(s, prog);

/*     tcc_add_dll("/usr/lib/libm.so"); */

    tcc_add_symbol(s, "TEX", (unsigned long)&TEX);
    tcc_add_symbol(s, "TXB", (unsigned long)&TXB);
    tcc_add_symbol(s, "TXP", (unsigned long)&TXP);


    tcc_relocate(s);
    tcc_get_symbol(s, &val, fname);
    return (cfunc) val;
}

/* TCC isn't threadsafe and even seems not to like having more than
 * one TCCState created or used at any one time in a single threaded
 * environment.  So, this code is all for investigation only and can't
 * currently be used in Mesa proper.
 *
 * I've taken some liberties with globals myself, now.
 */
GLboolean
_swrast_execute_codegen_program( GLcontext *ctx,
			 const struct fragment_program *program, GLuint maxInst,
			 struct fp_machine *machine, const struct sw_span *span,
			 GLuint column )
{
   if (program != current_program) {
      
      _swrast_translate_program( ctx );

      fprintf(stderr, "%s: compiling:\n%s\n", __FUNCTION__, program->c_str);

      current_program = program;
      current_func = codegen( current_tcc_state, program->c_str, 
			      "run_program" );
   }

   assert(current_func);

   return current_func( ctx,
			program->Base.LocalParams,
 			(const GLfloat (*)[4])ctx->FragmentProgram.Parameters, 
			program->Parameters->Parameters,
			(const GLfloat (*)[4])machine->Inputs,
			machine->Outputs );
}

#else  /* USE_TCC */

GLboolean
_swrast_execute_codegen_program( GLcontext *ctx,
			 const struct fragment_program *program, GLuint maxInst,
			 struct fp_machine *machine, const struct sw_span *span,
			 GLuint column )
{
   (void) ctx;
   (void) program; (void) maxInst;
   (void) machine; (void) span;
   (void) column;
   return 0;
}

#endif

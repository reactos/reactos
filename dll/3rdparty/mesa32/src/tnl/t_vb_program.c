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
 * \file tnl/t_vb_program.c
 * \brief Pipeline stage for executing vertex programs.
 * \author Brian Paul,  Keith Whitwell
 */


#include "glheader.h"
#include "colormac.h"
#include "context.h"
#include "macros.h"
#include "imports.h"
#include "prog_instruction.h"
#include "prog_statevars.h"
#include "prog_execute.h"

#include "tnl.h"
#include "t_context.h"
#include "t_pipeline.h"

#include "swrast/s_context.h"
#include "swrast/s_texfilter.h"

/**
 * XXX the texture sampling code in this module is a bit of a hack.
 * The texture sampling code is in swrast, though it doesn't have any
 * real dependencies on the rest of swrast.  It should probably be
 * moved into main/ someday.
 */

static void
vp_fetch_texel(GLcontext *ctx, const GLfloat texcoord[4], GLfloat lambda,
               GLuint unit, GLfloat color[4])
{
   GLchan rgba[4];
   SWcontext *swrast = SWRAST_CONTEXT(ctx);

   /* XXX use a float-valued TextureSample routine here!!! */
   swrast->TextureSample[unit](ctx, ctx->Texture.Unit[unit]._Current,
                               1, (const GLfloat (*)[4]) texcoord,
                               &lambda, &rgba);
   color[0] = CHAN_TO_FLOAT(rgba[0]);
   color[1] = CHAN_TO_FLOAT(rgba[1]);
   color[2] = CHAN_TO_FLOAT(rgba[2]);
   color[3] = CHAN_TO_FLOAT(rgba[3]);
}


/**
 * Called via ctx->Driver.ProgramStringNotify() after a new vertex program
 * string has been parsed.
 */
void
_tnl_program_string(GLcontext *ctx, GLenum target, struct gl_program *program)
{
   /* No-op.
    * If we had derived anything from the program that was private to this
    * stage we'd recompute/validate it here.
    */
}


/*!
 * Private storage for the vertex program pipeline stage.
 */
struct vp_stage_data {
   /** The results of running the vertex program go into these arrays. */
   GLvector4f results[VERT_RESULT_MAX];

   GLvector4f ndcCoords;              /**< normalized device coords */
   GLubyte *clipmask;                 /**< clip flags */
   GLubyte ormask, andmask;           /**< for clipping */
};


#define VP_STAGE_DATA(stage) ((struct vp_stage_data *)(stage->privatePtr))


/**
 * Initialize virtual machine state prior to executing vertex program.
 */
static void
init_machine(GLcontext *ctx, struct gl_program_machine *machine)
{
   /* Input registers get initialized from the current vertex attribs */
   MEMCPY(machine->VertAttribs, ctx->Current.Attrib,
          MAX_VERTEX_PROGRAM_ATTRIBS * 4 * sizeof(GLfloat));

   if (ctx->VertexProgram._Current->IsNVProgram) {
      GLuint i;
      /* Output/result regs are initialized to [0,0,0,1] */
      for (i = 0; i < MAX_NV_VERTEX_PROGRAM_OUTPUTS; i++) {
         ASSIGN_4V(machine->Outputs[i], 0.0F, 0.0F, 0.0F, 1.0F);
      }
      /* Temp regs are initialized to [0,0,0,0] */
      for (i = 0; i < MAX_NV_VERTEX_PROGRAM_TEMPS; i++) {
         ASSIGN_4V(machine->Temporaries[i], 0.0F, 0.0F, 0.0F, 0.0F);
      }
      for (i = 0; i < MAX_VERTEX_PROGRAM_ADDRESS_REGS; i++) {
         ASSIGN_4V(machine->AddressReg[i], 0, 0, 0, 0);
      }
   }

   machine->NumDeriv = 0;

   /* init condition codes */
   machine->CondCodes[0] = COND_EQ;
   machine->CondCodes[1] = COND_EQ;
   machine->CondCodes[2] = COND_EQ;
   machine->CondCodes[3] = COND_EQ;

   /* init call stack */
   machine->StackDepth = 0;

   machine->FetchTexelLod = vp_fetch_texel;
   machine->FetchTexelDeriv = NULL; /* not used by vertex programs */
}


/**
 * Copy the 16 elements of a matrix into four consecutive program
 * registers starting at 'pos'.
 */
static void
load_matrix(GLfloat registers[][4], GLuint pos, const GLfloat mat[16])
{
   GLuint i;
   for (i = 0; i < 4; i++) {
      registers[pos + i][0] = mat[0 + i];
      registers[pos + i][1] = mat[4 + i];
      registers[pos + i][2] = mat[8 + i];
      registers[pos + i][3] = mat[12 + i];
   }
}


/**
 * As above, but transpose the matrix.
 */
static void
load_transpose_matrix(GLfloat registers[][4], GLuint pos,
                      const GLfloat mat[16])
{
   MEMCPY(registers[pos], mat, 16 * sizeof(GLfloat));
}


/**
 * Load current vertex program's parameter registers with tracked
 * matrices (if NV program).  This only needs to be done per
 * glBegin/glEnd, not per-vertex.
 */
void
_mesa_load_tracked_matrices(GLcontext *ctx)
{
   GLuint i;

   for (i = 0; i < MAX_NV_VERTEX_PROGRAM_PARAMS / 4; i++) {
      /* point 'mat' at source matrix */
      GLmatrix *mat;
      if (ctx->VertexProgram.TrackMatrix[i] == GL_MODELVIEW) {
         mat = ctx->ModelviewMatrixStack.Top;
      }
      else if (ctx->VertexProgram.TrackMatrix[i] == GL_PROJECTION) {
         mat = ctx->ProjectionMatrixStack.Top;
      }
      else if (ctx->VertexProgram.TrackMatrix[i] == GL_TEXTURE) {
         mat = ctx->TextureMatrixStack[ctx->Texture.CurrentUnit].Top;
      }
      else if (ctx->VertexProgram.TrackMatrix[i] == GL_COLOR) {
         mat = ctx->ColorMatrixStack.Top;
      }
      else if (ctx->VertexProgram.TrackMatrix[i]==GL_MODELVIEW_PROJECTION_NV) {
         /* XXX verify the combined matrix is up to date */
         mat = &ctx->_ModelProjectMatrix;
      }
      else if (ctx->VertexProgram.TrackMatrix[i] >= GL_MATRIX0_NV &&
               ctx->VertexProgram.TrackMatrix[i] <= GL_MATRIX7_NV) {
         GLuint n = ctx->VertexProgram.TrackMatrix[i] - GL_MATRIX0_NV;
         ASSERT(n < MAX_PROGRAM_MATRICES);
         mat = ctx->ProgramMatrixStack[n].Top;
      }
      else {
         /* no matrix is tracked, but we leave the register values as-is */
         assert(ctx->VertexProgram.TrackMatrix[i] == GL_NONE);
         continue;
      }

      /* load the matrix values into sequential registers */
      if (ctx->VertexProgram.TrackMatrixTransform[i] == GL_IDENTITY_NV) {
         load_matrix(ctx->VertexProgram.Parameters, i*4, mat->m);
      }
      else if (ctx->VertexProgram.TrackMatrixTransform[i] == GL_INVERSE_NV) {
         _math_matrix_analyse(mat); /* update the inverse */
         ASSERT(!_math_matrix_is_dirty(mat));
         load_matrix(ctx->VertexProgram.Parameters, i*4, mat->inv);
      }
      else if (ctx->VertexProgram.TrackMatrixTransform[i] == GL_TRANSPOSE_NV) {
         load_transpose_matrix(ctx->VertexProgram.Parameters, i*4, mat->m);
      }
      else {
         assert(ctx->VertexProgram.TrackMatrixTransform[i]
                == GL_INVERSE_TRANSPOSE_NV);
         _math_matrix_analyse(mat); /* update the inverse */
         ASSERT(!_math_matrix_is_dirty(mat));
         load_transpose_matrix(ctx->VertexProgram.Parameters, i*4, mat->inv);
      }
   }
}


/**
 * This function executes vertex programs
 */
static GLboolean
run_vp( GLcontext *ctx, struct tnl_pipeline_stage *stage )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vp_stage_data *store = VP_STAGE_DATA(stage);
   struct vertex_buffer *VB = &tnl->vb;
   struct gl_vertex_program *program = ctx->VertexProgram._Current;
   struct gl_program_machine machine;
   GLuint outputs[VERT_RESULT_MAX], numOutputs;
   GLuint i, j;

   if (!program)
      return GL_TRUE;

   if (program->IsNVProgram) {
      _mesa_load_tracked_matrices(ctx);
   }
   else {
      /* ARB program or vertex shader */
      _mesa_load_state_parameters(ctx, program->Base.Parameters);
   }

   numOutputs = 0;
   for (i = 0; i < VERT_RESULT_MAX; i++) {
      if (program->Base.OutputsWritten & (1 << i)) {
         outputs[numOutputs++] = i;
      }
   }

   for (i = 0; i < VB->Count; i++) {
      GLuint attr;

      init_machine(ctx, &machine);

#if 0
      printf("Input  %d: %f, %f, %f, %f\n", i,
             VB->AttribPtr[0]->data[i][0],
             VB->AttribPtr[0]->data[i][1],
             VB->AttribPtr[0]->data[i][2],
             VB->AttribPtr[0]->data[i][3]);
      printf("   color: %f, %f, %f, %f\n",
             VB->AttribPtr[3]->data[i][0],
             VB->AttribPtr[3]->data[i][1],
             VB->AttribPtr[3]->data[i][2],
             VB->AttribPtr[3]->data[i][3]);
      printf("  normal: %f, %f, %f, %f\n",
             VB->AttribPtr[2]->data[i][0],
             VB->AttribPtr[2]->data[i][1],
             VB->AttribPtr[2]->data[i][2],
             VB->AttribPtr[2]->data[i][3]);
#endif

      /* the vertex array case */
      for (attr = 0; attr < VERT_ATTRIB_MAX; attr++) {
	 if (program->Base.InputsRead & (1 << attr)) {
	    const GLubyte *ptr = (const GLubyte*) VB->AttribPtr[attr]->data;
	    const GLuint size = VB->AttribPtr[attr]->size;
	    const GLuint stride = VB->AttribPtr[attr]->stride;
	    const GLfloat *data = (GLfloat *) (ptr + stride * i);
	    COPY_CLEAN_4V(machine.VertAttribs[attr], size, data);
	 }
      }

      /* execute the program */
      _mesa_execute_program(ctx, &program->Base, &machine);

      /* copy the output registers into the VB->attribs arrays */
      for (j = 0; j < numOutputs; j++) {
         const GLuint attr = outputs[j];
         COPY_4V(store->results[attr].data[i], machine.Outputs[attr]);
      }
#if 0
      printf("HPOS: %f %f %f %f\n",
             machine.Outputs[0][0], 
             machine.Outputs[0][1], 
             machine.Outputs[0][2], 
             machine.Outputs[0][3]);
#endif
   }

   /* Fixup fog and point size results if needed */
   if (program->IsNVProgram) {
      if (ctx->Fog.Enabled &&
          (program->Base.OutputsWritten & (1 << VERT_RESULT_FOGC)) == 0) {
         for (i = 0; i < VB->Count; i++) {
            store->results[VERT_RESULT_FOGC].data[i][0] = 1.0;
         }
      }

      if (ctx->VertexProgram.PointSizeEnabled &&
          (program->Base.OutputsWritten & (1 << VERT_RESULT_PSIZ)) == 0) {
         for (i = 0; i < VB->Count; i++) {
            store->results[VERT_RESULT_PSIZ].data[i][0] = ctx->Point.Size;
         }
      }
   }

   /* Setup the VB pointers so that the next pipeline stages get
    * their data from the right place (the program output arrays).
    */
   VB->ClipPtr = &store->results[VERT_RESULT_HPOS];
   VB->ClipPtr->size = 4;
   VB->ClipPtr->count = VB->Count;
   VB->ColorPtr[0] = &store->results[VERT_RESULT_COL0];
   VB->ColorPtr[1] = &store->results[VERT_RESULT_BFC0];
   VB->SecondaryColorPtr[0] = &store->results[VERT_RESULT_COL1];
   VB->SecondaryColorPtr[1] = &store->results[VERT_RESULT_BFC1];
   VB->FogCoordPtr = &store->results[VERT_RESULT_FOGC];

   VB->AttribPtr[VERT_ATTRIB_COLOR0] = &store->results[VERT_RESULT_COL0];
   VB->AttribPtr[VERT_ATTRIB_COLOR1] = &store->results[VERT_RESULT_COL1];
   VB->AttribPtr[VERT_ATTRIB_FOG] = &store->results[VERT_RESULT_FOGC];
   VB->AttribPtr[_TNL_ATTRIB_POINTSIZE] = &store->results[VERT_RESULT_PSIZ];

   for (i = 0; i < ctx->Const.MaxTextureCoordUnits; i++) {
      VB->TexCoordPtr[i] = 
      VB->AttribPtr[_TNL_ATTRIB_TEX0 + i]
         = &store->results[VERT_RESULT_TEX0 + i];
   }

   for (i = 0; i < ctx->Const.MaxVarying; i++) {
      if (program->Base.OutputsWritten & (1 << (VERT_RESULT_VAR0 + i))) {
         /* Note: varying results get put into the generic attributes */
	 VB->AttribPtr[VERT_ATTRIB_GENERIC0+i]
            = &store->results[VERT_RESULT_VAR0 + i];
      }
   }

   /* Cliptest and perspective divide.  Clip functions must clear
    * the clipmask.
    */
   store->ormask = 0;
   store->andmask = CLIP_FRUSTUM_BITS;

   if (tnl->NeedNdcCoords) {
      VB->NdcPtr =
         _mesa_clip_tab[VB->ClipPtr->size]( VB->ClipPtr,
                                            &store->ndcCoords,
                                            store->clipmask,
                                            &store->ormask,
                                            &store->andmask );
   }
   else {
      VB->NdcPtr = NULL;
      _mesa_clip_np_tab[VB->ClipPtr->size]( VB->ClipPtr,
                                            NULL,
                                            store->clipmask,
                                            &store->ormask,
                                            &store->andmask );
   }

   if (store->andmask)  /* All vertices are outside the frustum */
      return GL_FALSE;


   /* This is where we'd do clip testing against the user-defined
    * clipping planes, but they're not supported by vertex programs.
    */

   VB->ClipOrMask = store->ormask;
   VB->ClipMask = store->clipmask;

   return GL_TRUE;
}


/**
 * Called the first time stage->run is called.  In effect, don't
 * allocate data until the first time the stage is run.
 */
static GLboolean
init_vp(GLcontext *ctx, struct tnl_pipeline_stage *stage)
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vertex_buffer *VB = &(tnl->vb);
   struct vp_stage_data *store;
   const GLuint size = VB->Size;
   GLuint i;

   stage->privatePtr = MALLOC(sizeof(*store));
   store = VP_STAGE_DATA(stage);
   if (!store)
      return GL_FALSE;

   /* Allocate arrays of vertex output values */
   for (i = 0; i < VERT_RESULT_MAX; i++) {
      _mesa_vector4f_alloc( &store->results[i], 0, size, 32 );
      store->results[i].size = 4;
   }

   /* a few other misc allocations */
   _mesa_vector4f_alloc( &store->ndcCoords, 0, size, 32 );
   store->clipmask = (GLubyte *) ALIGN_MALLOC(sizeof(GLubyte)*size, 32 );

   return GL_TRUE;
}


/**
 * Destructor for this pipeline stage.
 */
static void
dtr(struct tnl_pipeline_stage *stage)
{
   struct vp_stage_data *store = VP_STAGE_DATA(stage);

   if (store) {
      GLuint i;

      /* free the vertex program result arrays */
      for (i = 0; i < VERT_RESULT_MAX; i++)
         _mesa_vector4f_free( &store->results[i] );

      /* free misc arrays */
      _mesa_vector4f_free( &store->ndcCoords );
      ALIGN_FREE( store->clipmask );

      FREE( store );
      stage->privatePtr = NULL;
   }
}


static void
validate_vp_stage(GLcontext *ctx, struct tnl_pipeline_stage *stage)
{
   if (ctx->VertexProgram._Current) {
      _swrast_update_texture_samplers(ctx);
   }
}



/**
 * Public description of this pipeline stage.
 */
const struct tnl_pipeline_stage _tnl_vertex_program_stage =
{
   "vertex-program",
   NULL,			/* private_data */
   init_vp,			/* create */
   dtr,				/* destroy */
   validate_vp_stage, 		/* validate */
   run_vp			/* run -- initially set to ctr */
};

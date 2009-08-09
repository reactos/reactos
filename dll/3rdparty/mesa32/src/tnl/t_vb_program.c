/*
 * Mesa 3-D graphics library
 * Version:  7.1
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


#include "main/glheader.h"
#include "main/colormac.h"
#include "main/context.h"
#include "main/macros.h"
#include "main/imports.h"
#include "shader/prog_instruction.h"
#include "shader/prog_statevars.h"
#include "shader/prog_execute.h"
#include "swrast/s_context.h"
#include "swrast/s_texfilter.h"

#include "tnl/tnl.h"
#include "tnl/t_context.h"
#include "tnl/t_pipeline.h"



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


static void
userclip( GLcontext *ctx,
          GLvector4f *clip,
          GLubyte *clipmask,
          GLubyte *clipormask,
          GLubyte *clipandmask )
{
   GLuint p;

   for (p = 0; p < ctx->Const.MaxClipPlanes; p++) {
      if (ctx->Transform.ClipPlanesEnabled & (1 << p)) {
	 GLuint nr, i;
	 const GLfloat a = ctx->Transform._ClipUserPlane[p][0];
	 const GLfloat b = ctx->Transform._ClipUserPlane[p][1];
	 const GLfloat c = ctx->Transform._ClipUserPlane[p][2];
	 const GLfloat d = ctx->Transform._ClipUserPlane[p][3];
         GLfloat *coord = (GLfloat *)clip->data;
         GLuint stride = clip->stride;
         GLuint count = clip->count;

	 for (nr = 0, i = 0 ; i < count ; i++) {
	    GLfloat dp = (coord[0] * a + 
			  coord[1] * b +
			  coord[2] * c +
			  coord[3] * d);

	    if (dp < 0) {
	       nr++;
	       clipmask[i] |= CLIP_USER_BIT;
	    }

	    STRIDE_F(coord, stride);
	 }

	 if (nr > 0) {
	    *clipormask |= CLIP_USER_BIT;
	    if (nr == count) {
	       *clipandmask |= CLIP_USER_BIT;
	       return;
	    }
	 }
      }
   }
}


static GLboolean
do_ndc_cliptest(GLcontext *ctx, struct vp_stage_data *store)
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vertex_buffer *VB = &tnl->vb;
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

   if (store->andmask) {
      /* All vertices are outside the frustum */
      return GL_FALSE;
   }

   /* Test userclip planes.  This contributes to VB->ClipMask.
    */
   /** XXX NEW_SLANG _Enabled ??? */
   if (ctx->Transform.ClipPlanesEnabled && (!ctx->VertexProgram._Enabled ||
      ctx->VertexProgram.Current->IsPositionInvariant)) {
      userclip( ctx,
		VB->ClipPtr,
		store->clipmask,
		&store->ormask,
		&store->andmask );

      if (store->andmask) {
	 return GL_FALSE;
      }
   }

   VB->ClipAndMask = store->andmask;
   VB->ClipOrMask = store->ormask;
   VB->ClipMask = store->clipmask;

   return GL_TRUE;
}


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

   machine->Samplers = ctx->VertexProgram._Current->Base.SamplerUnits;
}


/**
 * Map the texture images which the vertex program will access (if any).
 */
static void
map_textures(GLcontext *ctx, const struct gl_vertex_program *vp)
{
   GLuint u;

   if (!ctx->Driver.MapTexture)
      return;

   for (u = 0; u < ctx->Const.MaxVertexTextureImageUnits; u++) {
      if (vp->Base.TexturesUsed[u]) {
         /* Note: _Current *should* correspond to the target indicated
          * in TexturesUsed[u].
          */
         ctx->Driver.MapTexture(ctx, ctx->Texture.Unit[u]._Current);
      }
   }
}


/**
 * Unmap the texture images which were used by the vertex program (if any).
 */
static void
unmap_textures(GLcontext *ctx, const struct gl_vertex_program *vp)
{
   GLuint u;

   if (!ctx->Driver.MapTexture)
      return;

   for (u = 0; u < ctx->Const.MaxVertexTextureImageUnits; u++) {
      if (vp->Base.TexturesUsed[u]) {
         /* Note: _Current *should* correspond to the target indicated
          * in TexturesUsed[u].
          */
         ctx->Driver.UnmapTexture(ctx, ctx->Texture.Unit[u]._Current);
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

   /* make list of outputs to save some time below */
   numOutputs = 0;
   for (i = 0; i < VERT_RESULT_MAX; i++) {
      if (program->Base.OutputsWritten & (1 << i)) {
         outputs[numOutputs++] = i;
      }
   }

   map_textures(ctx, program);

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

   unmap_textures(ctx, program);

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

   if (program->IsPositionInvariant) {
      /* We need the exact same transform as in the fixed function path here
       * to guarantee invariance, depending on compiler optimization flags
       * results could be different otherwise.
       */
      VB->ClipPtr = TransformRaw( &store->results[0],
				  &ctx->_ModelProjectMatrix,
				  VB->AttribPtr[0] );

      /* Drivers expect this to be clean to element 4...
       */
      switch (VB->ClipPtr->size) {
      case 1:
	 /* impossible */
      case 2:
	 _mesa_vector4f_clean_elem( VB->ClipPtr, VB->Count, 2 );
	 /* fall-through */
      case 3:
	 _mesa_vector4f_clean_elem( VB->ClipPtr, VB->Count, 3 );
	 /* fall-through */
      case 4:
	 break;
      }
   }
   else {
      /* Setup the VB pointers so that the next pipeline stages get
       * their data from the right place (the program output arrays).
       */
      VB->ClipPtr = &store->results[VERT_RESULT_HPOS];
      VB->ClipPtr->size = 4;
      VB->ClipPtr->count = VB->Count;
   }

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


   /* Perform NDC and cliptest operations:
    */
   return do_ndc_cliptest(ctx, store);
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

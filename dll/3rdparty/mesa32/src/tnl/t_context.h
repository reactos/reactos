/*
 * mesa 3-D graphics library
 * Version:  6.5
 *
 * Copyright (C) 1999-2006  Brian Paul   All Rights Reserved.
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
 * \file t_context.h
 * \brief TnL module datatypes and definitions.
 * \author Keith Whitwell
 */


/**
 * \mainpage The TNL-module
 *
 * TNL stands for "transform and lighting", i.e. this module implements
 * a pipeline that receives as input a buffer of vertices and does all
 * necessary transformations (rotations, clipping, vertex shader etc.)
 * and passes then the output to the rasterizer.
 *
 * The tnl_pipeline contains the array of all stages, which should be
 * applied. Each stage is a black-box, which is described by an
 * tnl_pipeline_stage. The function ::_tnl_run_pipeline applies all the
 * stages to the vertex_buffer TNLcontext::vb, where the vertex data
 * is stored. The last stage in the pipeline is the rasterizer.
 *
 */


#ifndef _T_CONTEXT_H
#define _T_CONTEXT_H

#include "main/glheader.h"
#include "main/mtypes.h"

#include "math/m_matrix.h"
#include "math/m_vector.h"
#include "math/m_xform.h"

#include "vbo/vbo.h"

#define MAX_PIPELINE_STAGES     30

/*
 * Note: The first attributes match the VERT_ATTRIB_* definitions
 * in mtypes.h.  However, the tnl module has additional attributes
 * for materials, color indexes, edge flags, etc.
 */
/* Although it's nice to use these as bit indexes in a DWORD flag, we
 * could manage without if necessary.  Another limit currently is the
 * number of bits allocated for these numbers in places like vertex
 * program instruction formats and register layouts.
 */
/* The bit space exhaustion is a fact now, done by _TNL_ATTRIB_ATTRIBUTE* for
 * GLSL vertex shader which cannot be aliased with conventional vertex attribs.
 * Compacting _TNL_ATTRIB_MAT_* attribs would not work, they would not give
 * as many free bits (11 plus already 1 free bit) as _TNL_ATTRIB_ATTRIBUTE*
 * attribs want (16).
 */
enum {
	_TNL_ATTRIB_POS = 0,
	_TNL_ATTRIB_WEIGHT = 1,
	_TNL_ATTRIB_NORMAL = 2,
	_TNL_ATTRIB_COLOR0 = 3,
	_TNL_ATTRIB_COLOR1 = 4,
	_TNL_ATTRIB_FOG = 5,
	_TNL_ATTRIB_COLOR_INDEX = 6,
	_TNL_ATTRIB_EDGEFLAG = 7,
	_TNL_ATTRIB_TEX0 = 8,
	_TNL_ATTRIB_TEX1 = 9,
	_TNL_ATTRIB_TEX2 = 10,
	_TNL_ATTRIB_TEX3 = 11,
	_TNL_ATTRIB_TEX4 = 12,
	_TNL_ATTRIB_TEX5 = 13,
	_TNL_ATTRIB_TEX6 = 14,
	_TNL_ATTRIB_TEX7 = 15,

	_TNL_ATTRIB_GENERIC0 = 16, /* doesn't really exist! */
	_TNL_ATTRIB_GENERIC1 = 17,
	_TNL_ATTRIB_GENERIC2 = 18,
	_TNL_ATTRIB_GENERIC3 = 19,
	_TNL_ATTRIB_GENERIC4 = 20,
	_TNL_ATTRIB_GENERIC5 = 21,
	_TNL_ATTRIB_GENERIC6 = 22,
	_TNL_ATTRIB_GENERIC7 = 23,
	_TNL_ATTRIB_GENERIC8 = 24,
	_TNL_ATTRIB_GENERIC9 = 25,
	_TNL_ATTRIB_GENERIC10 = 26,
	_TNL_ATTRIB_GENERIC11 = 27,
	_TNL_ATTRIB_GENERIC12 = 28,
	_TNL_ATTRIB_GENERIC13 = 29,
	_TNL_ATTRIB_GENERIC14 = 30,
	_TNL_ATTRIB_GENERIC15 = 31,

	/* These alias with the generics, but they are not active
	 * concurrently, so it's not a problem.  The TNL module
	 * doesn't have to do anything about this as this is how they
	 * are passed into the _draw_prims callback.
	 *
	 * When we generate fixed-function replacement programs (in
	 * t_vp_build.c currently), they refer to the appropriate
	 * generic attribute in order to pick up per-vertex material
	 * data.
	 */
	_TNL_ATTRIB_MAT_FRONT_AMBIENT = 16,
	_TNL_ATTRIB_MAT_BACK_AMBIENT = 17,
	_TNL_ATTRIB_MAT_FRONT_DIFFUSE = 18,
	_TNL_ATTRIB_MAT_BACK_DIFFUSE = 19,
	_TNL_ATTRIB_MAT_FRONT_SPECULAR = 20,
	_TNL_ATTRIB_MAT_BACK_SPECULAR = 21,
	_TNL_ATTRIB_MAT_FRONT_EMISSION = 22,
	_TNL_ATTRIB_MAT_BACK_EMISSION = 23,
	_TNL_ATTRIB_MAT_FRONT_SHININESS = 24,
	_TNL_ATTRIB_MAT_BACK_SHININESS = 25,
	_TNL_ATTRIB_MAT_FRONT_INDEXES = 26,
	_TNL_ATTRIB_MAT_BACK_INDEXES = 27,

	/* This is really a VERT_RESULT, not an attrib.  Need to fix
	 * tnl to understand the difference.
	 */
	_TNL_ATTRIB_POINTSIZE = 16,

	_TNL_ATTRIB_MAX = 32
} ;

#define _TNL_ATTRIB_TEX(u)       (_TNL_ATTRIB_TEX0 + (u))
#define _TNL_ATTRIB_GENERIC(n) (_TNL_ATTRIB_GENERIC0 + (n))

/* special index used for handing invalid glVertexAttribute() indices */
#define _TNL_ATTRIB_ERROR    (_TNL_ATTRIB_GENERIC15 + 1)

/**
 * Handy attribute ranges:
 */
#define _TNL_FIRST_PROG      _TNL_ATTRIB_WEIGHT
#define _TNL_LAST_PROG       _TNL_ATTRIB_TEX7

#define _TNL_FIRST_TEX       _TNL_ATTRIB_TEX0
#define _TNL_LAST_TEX        _TNL_ATTRIB_TEX7

#define _TNL_FIRST_GENERIC _TNL_ATTRIB_GENERIC0
#define _TNL_LAST_GENERIC  _TNL_ATTRIB_GENERIC15

#define _TNL_FIRST_MAT       _TNL_ATTRIB_MAT_FRONT_AMBIENT /* GENERIC0 */
#define _TNL_LAST_MAT        _TNL_ATTRIB_MAT_BACK_INDEXES  /* GENERIC11 */

/* Number of available generic attributes */
#define _TNL_NUM_GENERIC 16

/* Number of attributes used for evaluators */
#define _TNL_NUM_EVAL 16


#define PRIM_BEGIN     0x10
#define PRIM_END       0x20
#define PRIM_MODE_MASK 0x0f

static INLINE GLuint _tnl_translate_prim( const struct _mesa_prim *prim )
{
   GLuint flag;
   flag = prim->mode;
   if (prim->begin) flag |= PRIM_BEGIN;
   if (prim->end) flag |= PRIM_END;
   return flag;
}




/**
 * Contains the current state of a running pipeline.
 */
struct vertex_buffer
{
   GLuint Size;  /**< Max vertices per vertex buffer, constant */

   /* Constant over the pipeline.
    */
   GLuint Count;  /**< Number of vertices currently in buffer */

   /* Pointers to current data.
    * XXX some of these fields alias AttribPtr below and should be removed
    * such as NormalPtr, TexCoordPtr, FogCoordPtr, etc.
    */
   GLuint      *Elts;		                
   GLvector4f  *ObjPtr;		                /* _TNL_BIT_POS */
   GLvector4f  *EyePtr;		                /* _TNL_BIT_POS */
   GLvector4f  *ClipPtr;	                /* _TNL_BIT_POS */
   GLvector4f  *NdcPtr;                         /* _TNL_BIT_POS */
   GLubyte     ClipOrMask;	                /* _TNL_BIT_POS */
   GLubyte     ClipAndMask;	                /* _TNL_BIT_POS */
   GLubyte     *ClipMask;		        /* _TNL_BIT_POS */
   GLvector4f  *NormalPtr;	                /* _TNL_BIT_NORMAL */
   GLfloat     *NormalLengthPtr;	        /* _TNL_BIT_NORMAL */
   GLboolean   *EdgeFlag;	                /* _TNL_BIT_EDGEFLAG */
   GLvector4f  *TexCoordPtr[MAX_TEXTURE_COORD_UNITS]; /* VERT_TEX_0..n */
   GLvector4f  *IndexPtr[2];	                /* _TNL_BIT_INDEX */
   GLvector4f  *ColorPtr[2];	                /* _TNL_BIT_COLOR0 */
   GLvector4f  *SecondaryColorPtr[2];           /* _TNL_BIT_COLOR1 */
   GLvector4f  *FogCoordPtr;	                /* _TNL_BIT_FOG */

   const struct _mesa_prim  *Primitive;	              
   GLuint      PrimitiveCount;	      

   /* Inputs to the vertex program stage */
   GLvector4f *AttribPtr[_TNL_ATTRIB_MAX];      /* GL_NV_vertex_program */
};


/**
 * Describes an individual operation on the pipeline.
 */
struct tnl_pipeline_stage
{
   const char *name;

   /* Private data for the pipeline stage:
    */
   void *privatePtr;

   /* Allocate private data
    */
   GLboolean (*create)( GLcontext *ctx, struct tnl_pipeline_stage * );

   /* Free private data.
    */
   void (*destroy)( struct tnl_pipeline_stage * );

   /* Called on any statechange or input array size change or
    * input array change to/from zero stride.
    */
   void (*validate)( GLcontext *ctx, struct tnl_pipeline_stage * );

   /* Called from _tnl_run_pipeline().  The stage.changed_inputs value
    * encodes all inputs to thee struct which have changed.  If
    * non-zero, recompute all affected outputs of the stage, otherwise
    * execute any 'sideeffects' of the stage.
    *
    * Return value: GL_TRUE - keep going
    *               GL_FALSE - finished pipeline
    */
   GLboolean (*run)( GLcontext *ctx, struct tnl_pipeline_stage * );
};



/** Contains the array of all pipeline stages.
 * The default values are defined at the end of t_pipeline.c 
 */
struct tnl_pipeline {
   
   GLuint last_attrib_stride[_TNL_ATTRIB_MAX];
   GLuint last_attrib_size[_TNL_ATTRIB_MAX];
   GLuint input_changes;
   GLuint new_state;

   struct tnl_pipeline_stage stages[MAX_PIPELINE_STAGES+1];
   GLuint nr_stages;
};

struct tnl_clipspace;
struct tnl_clipspace_attr;

typedef void (*tnl_extract_func)( const struct tnl_clipspace_attr *a, 
				  GLfloat *out, 
				  const GLubyte *v );

typedef void (*tnl_insert_func)( const struct tnl_clipspace_attr *a, 
				 GLubyte *v, 
				 const GLfloat *in );

typedef void (*tnl_emit_func)( GLcontext *ctx, 
			       GLuint count, 
			       GLubyte *dest );


/**
 * Describes how to convert/move a vertex attribute from a vertex array
 * to a vertex structure.
 */
struct tnl_clipspace_attr
{
   GLuint attrib;          /* which vertex attrib (0=position, etc) */
   GLuint format;
   GLuint vertoffset;      /* position of the attrib in the vertex struct */
   GLuint vertattrsize;    /* size of the attribute in bytes */
   GLubyte *inputptr;
   GLuint inputstride;
   GLuint inputsize;
   const tnl_insert_func *insert;
   tnl_insert_func emit;
   tnl_extract_func extract;
   const GLfloat *vp;   /* NDC->Viewport mapping matrix */
};




typedef void (*tnl_points_func)( GLcontext *ctx, GLuint first, GLuint last );
typedef void (*tnl_line_func)( GLcontext *ctx, GLuint v1, GLuint v2 );
typedef void (*tnl_triangle_func)( GLcontext *ctx,
				   GLuint v1, GLuint v2, GLuint v3 );
typedef void (*tnl_quad_func)( GLcontext *ctx, GLuint v1, GLuint v2,
			       GLuint v3, GLuint v4 );
typedef void (*tnl_render_func)( GLcontext *ctx, GLuint start, GLuint count,
				 GLuint flags );
typedef void (*tnl_interp_func)( GLcontext *ctx,
				 GLfloat t, GLuint dst, GLuint out, GLuint in,
				 GLboolean force_boundary );
typedef void (*tnl_copy_pv_func)( GLcontext *ctx, GLuint dst, GLuint src );
typedef void (*tnl_setup_func)( GLcontext *ctx,
				GLuint start, GLuint end,
				GLuint new_inputs);


struct tnl_attr_type {
   GLuint format;
   GLuint size;
   GLuint stride;
   GLuint offset;
};

struct tnl_clipspace_fastpath {
   GLuint vertex_size;
   GLuint attr_count;
   GLboolean match_strides;

   struct tnl_attr_type *attr;

   tnl_emit_func func;
   struct tnl_clipspace_fastpath *next;
};

/**
 * Used to describe conversion of vertex arrays to vertex structures.
 * I.e. Structure of arrays to arrays of structs.
 */
struct tnl_clipspace
{
   GLboolean need_extras;
   
   GLuint new_inputs;

   GLubyte *vertex_buf;
   GLuint vertex_size;
   GLuint max_vertex_size;

   struct tnl_clipspace_attr attr[_TNL_ATTRIB_MAX];
   GLuint attr_count;

   tnl_emit_func emit;
   tnl_interp_func interp;
   tnl_copy_pv_func copy_pv;

   /* Parameters and constants for codegen:
    */
   GLboolean need_viewport;
   GLfloat vp_scale[4];		
   GLfloat vp_xlate[4];
   GLfloat chan_scale[4];
   GLfloat identity[4];

   struct tnl_clipspace_fastpath *fastpath;
   
   void (*codegen_emit)( GLcontext *ctx );
};


struct tnl_device_driver
{
   /***
    *** TNL Pipeline
    ***/

   void (*RunPipeline)(GLcontext *ctx);
   /* Replaces PipelineStart/PipelineFinish -- intended to allow
    * drivers to wrap _tnl_run_pipeline() with code to validate state
    * and grab/release hardware locks.  
    */

   void (*NotifyMaterialChange)(GLcontext *ctx);
   /* Alert tnl-aware drivers of changes to material.
    */

   void (*NotifyInputChanges)(GLcontext *ctx, GLuint bitmask);
   /* Alert tnl-aware drivers of changes to size and stride of input
    * arrays.
    */

   /***
    *** Rendering -- These functions called only from t_vb_render.c
    ***/
   struct
   {
      void (*Start)(GLcontext *ctx);
      void (*Finish)(GLcontext *ctx);
      /* Called before and after all rendering operations, including DrawPixels,
       * ReadPixels, Bitmap, span functions, and CopyTexImage, etc commands.
       * These are a suitable place for grabbing/releasing hardware locks.
       */

      void (*PrimitiveNotify)(GLcontext *ctx, GLenum mode);
      /* Called between RenderStart() and RenderFinish() to indicate the
       * type of primitive we're about to draw.  Mode will be one of the
       * modes accepted by glBegin().
       */

      tnl_interp_func Interp;
      /* The interp function is called by the clipping routines when we need
       * to generate an interpolated vertex.  All pertinant vertex ancilliary
       * data should be computed by interpolating between the 'in' and 'out'
       * vertices.
       */

      tnl_copy_pv_func CopyPV;
      /* The copy function is used to make a copy of a vertex.  All pertinant
       * vertex attributes should be copied.
       */

      void (*ClippedPolygon)( GLcontext *ctx, const GLuint *elts, GLuint n );
      /* Render a polygon with <n> vertices whose indexes are in the <elts>
       * array.
       */

      void (*ClippedLine)( GLcontext *ctx, GLuint v0, GLuint v1 );
      /* Render a line between the two vertices given by indexes v0 and v1. */

      tnl_points_func           Points; /* must now respect vb->elts */
      tnl_line_func             Line;
      tnl_triangle_func         Triangle;
      tnl_quad_func             Quad;
      /* These functions are called in order to render points, lines,
       * triangles and quads.  These are only called via the T&L module.
       */

      tnl_render_func          *PrimTabVerts;
      tnl_render_func          *PrimTabElts;
      /* Render whole unclipped primitives (points, lines, linestrips,
       * lineloops, etc).  The tables are indexed by the GL enum of the
       * primitive to be rendered.  RenderTabVerts is used for non-indexed
       * arrays of vertices.  RenderTabElts is used for indexed arrays of
       * vertices.
       */

      void (*ResetLineStipple)( GLcontext *ctx );
      /* Reset the hardware's line stipple counter.
       */

      tnl_setup_func BuildVertices;
      /* This function is called whenever new vertices are required for
       * rendering.  The vertices in question are those n such that start
       * <= n < end.  The new_inputs parameter indicates those fields of
       * the vertex which need to be updated, if only a partial repair of
       * the vertex is required.
       *
       * This function is called only from _tnl_render_stage in tnl/t_render.c.
       */
      

      GLboolean (*Multipass)( GLcontext *ctx, GLuint passno );
      /* Driver may request additional render passes by returning GL_TRUE
       * when this function is called.  This function will be called
       * after the first pass, and passes will be made until the function
       * returns GL_FALSE.  If no function is registered, only one pass
       * is made.
       *
       * This function will be first invoked with passno == 1.
       */
   } Render;
};


#define DECLARE_RENDERINPUTS(name) BITSET64_DECLARE(name, _TNL_ATTRIB_MAX)
#define RENDERINPUTS_COPY BITSET64_COPY
#define RENDERINPUTS_EQUAL BITSET64_EQUAL
#define RENDERINPUTS_ZERO BITSET64_ZERO
#define RENDERINPUTS_ONES BITSET64_ONES
#define RENDERINPUTS_TEST BITSET64_TEST
#define RENDERINPUTS_SET BITSET64_SET
#define RENDERINPUTS_CLEAR BITSET64_CLEAR
#define RENDERINPUTS_TEST_RANGE BITSET64_TEST_RANGE
#define RENDERINPUTS_SET_RANGE BITSET64_SET_RANGE
#define RENDERINPUTS_CLEAR_RANGE BITSET64_CLEAR_RANGE


/**
 * Context state for T&L context.
 */
typedef struct
{
   /* Driver interface.
    */
   struct tnl_device_driver Driver;

   /* Pipeline
    */
   struct tnl_pipeline pipeline;
   struct vertex_buffer vb;

   /* Clipspace/ndc/window vertex managment:
    */
   struct tnl_clipspace clipspace;

   /* Probably need a better configuration mechanism:
    */
   GLboolean NeedNdcCoords;
   GLboolean AllowVertexFog;
   GLboolean AllowPixelFog;
   GLboolean _DoVertexFog;  /* eval fog function at each vertex? */

   DECLARE_RENDERINPUTS(render_inputs_bitset);

   GLvector4f tmp_inputs[VERT_ATTRIB_MAX];

   /* Temp storage for t_draw.c: 
    */
   GLubyte *block[VERT_ATTRIB_MAX];
   GLuint nr_blocks;

} TNLcontext;



#define TNL_CONTEXT(ctx) ((TNLcontext *)((ctx)->swtnl_context))


#define TYPE_IDX(t) ((t) & 0xf)
#define MAX_TYPES TYPE_IDX(GL_DOUBLE)+1      /* 0xa + 1 */


#endif

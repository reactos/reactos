/*
 * mesa 3-D graphics library
 * Version:  6.3
 *
 * Copyright (C) 1999-2005  Brian Paul   All Rights Reserved.
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
 * The initial vertex_buffer data may either come from an ::immediate
 * structure or client vertex_arrays or display lists:
 *
 *
 * - The ::immediate structure records all the GL commands issued between
 * glBegin and glEnd.  \n
 * The structure accumulates data, until it is either full or it is
 * flushed (usually by a state change). Before starting then the pipeline,
 * the collected vertex data in ::immediate has to be pushed into
 * TNLcontext::vb.
 * This happens in ::_tnl_vb_bind_immediate. The pipeline is then run by
 * calling tnl_device_driver::RunPipeline = ::_tnl_run_pipeline, which
 * is stored in TNLcontext::Driver.   \n
 * An ::immediate does (for performance reasons) usually not finish with a
 * glEnd, and hence it also does not need to start with a glBegin.
 * This means that the last vertices of one ::immediate may need to be
 * saved for the next one.
 *
 *
 * - NOT SURE ABOUT THIS: The vertex_arrays structure is used to handle
 * glDrawArrays etc.  \n
 * Here, the data of the vertex_arrays is copied by ::_tnl_vb_bind_arrays
 * into TNLcontext::vb, so that the pipeline can be started.
 */


#ifndef _T_CONTEXT_H
#define _T_CONTEXT_H

#include "glheader.h"
#include "mtypes.h"

#include "math/m_matrix.h"
#include "math/m_vector.h"
#include "math/m_xform.h"


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
enum {
	_TNL_ATTRIB_POS = 0,
	_TNL_ATTRIB_WEIGHT = 1,
	_TNL_ATTRIB_NORMAL = 2,
	_TNL_ATTRIB_COLOR0 = 3,
	_TNL_ATTRIB_COLOR1 = 4,
	_TNL_ATTRIB_FOG = 5,
	_TNL_ATTRIB_SIX = 6,
	_TNL_ATTRIB_SEVEN = 7,
	_TNL_ATTRIB_TEX0 = 8,
	_TNL_ATTRIB_TEX1 = 9,
	_TNL_ATTRIB_TEX2 = 10,
	_TNL_ATTRIB_TEX3 = 11,
	_TNL_ATTRIB_TEX4 = 12,
	_TNL_ATTRIB_TEX5 = 13,
	_TNL_ATTRIB_TEX6 = 14,
	_TNL_ATTRIB_TEX7 = 15,
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
	_TNL_ATTRIB_INDEX = 28,
	_TNL_ATTRIB_EDGEFLAG = 29,
	_TNL_ATTRIB_POINTSIZE = 30,
	_TNL_ATTRIB_MAX = 31
} ;

/* Will probably have to revise this scheme fairly shortly, eg. by
 * compacting all the MAT flags down to one bit, or by using two
 * dwords to store the flags.
 */
#define _TNL_BIT_POS                 (1<<0)
#define _TNL_BIT_WEIGHT              (1<<1)
#define _TNL_BIT_NORMAL              (1<<2)
#define _TNL_BIT_COLOR0              (1<<3)
#define _TNL_BIT_COLOR1              (1<<4)
#define _TNL_BIT_FOG                 (1<<5)
#define _TNL_BIT_SIX                 (1<<6)
#define _TNL_BIT_SEVEN               (1<<7)
#define _TNL_BIT_TEX0                (1<<8)
#define _TNL_BIT_TEX1                (1<<9)
#define _TNL_BIT_TEX2                (1<<10)
#define _TNL_BIT_TEX3                (1<<11)
#define _TNL_BIT_TEX4                (1<<12)
#define _TNL_BIT_TEX5                (1<<13)
#define _TNL_BIT_TEX6                (1<<14)
#define _TNL_BIT_TEX7                (1<<15)
#define _TNL_BIT_MAT_FRONT_AMBIENT   (1<<16)
#define _TNL_BIT_MAT_BACK_AMBIENT    (1<<17)
#define _TNL_BIT_MAT_FRONT_DIFFUSE   (1<<18)
#define _TNL_BIT_MAT_BACK_DIFFUSE    (1<<19)
#define _TNL_BIT_MAT_FRONT_SPECULAR  (1<<20)
#define _TNL_BIT_MAT_BACK_SPECULAR   (1<<21)
#define _TNL_BIT_MAT_FRONT_EMISSION  (1<<22)
#define _TNL_BIT_MAT_BACK_EMISSION   (1<<23)
#define _TNL_BIT_MAT_FRONT_SHININESS (1<<24)
#define _TNL_BIT_MAT_BACK_SHININESS  (1<<25)
#define _TNL_BIT_MAT_FRONT_INDEXES   (1<<26)
#define _TNL_BIT_MAT_BACK_INDEXES    (1<<27)
#define _TNL_BIT_INDEX               (1<<28)
#define _TNL_BIT_EDGEFLAG            (1<<29)
#define _TNL_BIT_POINTSIZE           (1<<30)

#define _TNL_BIT_TEX(u)  (1 << (_TNL_ATTRIB_TEX0 + (u)))



#define _TNL_BITS_MAT_ANY  (_TNL_BIT_MAT_FRONT_AMBIENT   | 	\
			    _TNL_BIT_MAT_BACK_AMBIENT    | 	\
			    _TNL_BIT_MAT_FRONT_DIFFUSE   | 	\
			    _TNL_BIT_MAT_BACK_DIFFUSE    | 	\
			    _TNL_BIT_MAT_FRONT_SPECULAR  | 	\
			    _TNL_BIT_MAT_BACK_SPECULAR   | 	\
			    _TNL_BIT_MAT_FRONT_EMISSION  | 	\
			    _TNL_BIT_MAT_BACK_EMISSION   | 	\
			    _TNL_BIT_MAT_FRONT_SHININESS | 	\
			    _TNL_BIT_MAT_BACK_SHININESS  | 	\
			    _TNL_BIT_MAT_FRONT_INDEXES   | 	\
			    _TNL_BIT_MAT_BACK_INDEXES)


#define _TNL_BITS_TEX_ANY  (_TNL_BIT_TEX0 |	\
                            _TNL_BIT_TEX1 |	\
                            _TNL_BIT_TEX2 |	\
                            _TNL_BIT_TEX3 |	\
                            _TNL_BIT_TEX4 |	\
                            _TNL_BIT_TEX5 |	\
                            _TNL_BIT_TEX6 |	\
                            _TNL_BIT_TEX7)


#define _TNL_BITS_PROG_ANY   (_TNL_BIT_POS    |		\
			      _TNL_BIT_WEIGHT |		\
			      _TNL_BIT_NORMAL |		\
			      _TNL_BIT_COLOR0 |		\
			      _TNL_BIT_COLOR1 |		\
			      _TNL_BIT_FOG    |		\
			      _TNL_BIT_SIX    |		\
			      _TNL_BIT_SEVEN  |		\
			      _TNL_BITS_TEX_ANY)



#define PRIM_BEGIN     0x10
#define PRIM_END       0x20
#define PRIM_WEAK      0x40
#define PRIM_MODE_MASK 0x0f

/*
 */
struct tnl_prim {
   GLuint mode;
   GLuint start;
   GLuint count;
};



struct tnl_eval1_map {
   struct gl_1d_map *map;
   GLuint sz;
};

struct tnl_eval2_map {
   struct gl_2d_map *map;
   GLuint sz;
};

struct tnl_eval {
   GLuint new_state;
   struct tnl_eval1_map map1[_TNL_ATTRIB_INDEX + 1];
   struct tnl_eval2_map map2[_TNL_ATTRIB_INDEX + 1];
};


#define TNL_MAX_PRIM 16
#define TNL_MAX_COPIED_VERTS 3

struct tnl_copied_vtx {
   GLfloat buffer[_TNL_ATTRIB_MAX * 4 * TNL_MAX_COPIED_VERTS];
   GLuint nr;
};

#define VERT_BUFFER_SIZE 2048	/* 8kbytes */


typedef void (*tnl_attrfv_func)( const GLfloat * );

struct _tnl_dynfn {
   struct _tnl_dynfn *next, *prev;
   GLuint key;
   char *code;
};

struct _tnl_dynfn_lists {
   struct _tnl_dynfn Vertex[4];
   struct _tnl_dynfn Attribute[4];
};

struct _tnl_dynfn_generators {
   struct _tnl_dynfn *(*Vertex[4])( GLcontext *ctx, int key );
   struct _tnl_dynfn *(*Attribute[4])( GLcontext *ctx, int key );
};

#define _TNL_MAX_ATTR_CODEGEN 16


/* The assembly of vertices in immediate mode is separated from
 * display list compilation.  This allows a simpler immediate mode
 * treatment and a display list compiler better suited to
 * hardware-acceleration.
 */
struct tnl_vtx {
   GLfloat buffer[VERT_BUFFER_SIZE];
   GLubyte attrsz[_TNL_ATTRIB_MAX];
   GLuint vertex_size;
   struct tnl_prim prim[TNL_MAX_PRIM];
   GLuint prim_count;
   GLfloat *vbptr;		      /* cursor, points into buffer */
   GLfloat vertex[_TNL_ATTRIB_MAX*4]; /* current vertex */
   GLfloat *attrptr[_TNL_ATTRIB_MAX]; /* points into vertex */
   GLfloat *current[_TNL_ATTRIB_MAX]; /* points into ctx->Current, etc */
   GLfloat CurrentFloatEdgeFlag;
   GLuint counter, initial_counter;
   struct tnl_copied_vtx copied;

   tnl_attrfv_func tabfv[_TNL_MAX_ATTR_CODEGEN+1][4]; /* plus 1 for ERROR_ATTRIB */

   struct _tnl_dynfn_lists cache;
   struct _tnl_dynfn_generators gen;

   struct tnl_eval eval;
   GLboolean *edgeflag_tmp;
   GLboolean have_materials;
};




/* For display lists, this structure holds a run of vertices of the
 * same format, and a strictly well-formed set of begin/end pairs,
 * starting on the first vertex and ending at the last.  Vertex
 * copying on buffer breaks is precomputed according to these
 * primitives, though there are situations where the copying will need
 * correction at execute-time, perhaps by replaying the list as
 * immediate mode commands.
 *
 * On executing this list, the 'current' values may be updated with
 * the values of the final vertex, and often no fixup of the start of
 * the vertex list is required.
 *
 * Eval and other commands that don't fit into these vertex lists are
 * compiled using the fallback opcode mechanism provided by dlist.c.
 */
struct tnl_vertex_list {
   GLubyte attrsz[_TNL_ATTRIB_MAX];
   GLuint vertex_size;

   GLfloat *buffer;
   GLuint count;
   GLuint wrap_count;		/* number of copied vertices at start */
   GLboolean have_materials;	/* bit of a hack - quick check for materials */
   GLboolean dangling_attr_ref;	/* current attr implicitly referenced
				   outside the list */

   GLfloat *normal_lengths;
   struct tnl_prim *prim;
   GLuint prim_count;

   struct tnl_vertex_store *vertex_store;
   struct tnl_primitive_store *prim_store;
};

/* These buffers should be a reasonable size to support upload to
 * hardware?  Maybe drivers should stitch them back together, or
 * specify a desired size?
 */
#define SAVE_BUFFER_SIZE (16*1024)
#define SAVE_PRIM_SIZE   128

/* Storage to be shared among several vertex_lists.
 */
struct tnl_vertex_store {
   GLfloat buffer[SAVE_BUFFER_SIZE];
   GLuint used;
   GLuint refcount;
};

struct tnl_primitive_store {
   struct tnl_prim buffer[SAVE_PRIM_SIZE];
   GLuint used;
   GLuint refcount;
};


struct tnl_save {
   GLubyte attrsz[_TNL_ATTRIB_MAX];
   GLuint vertex_size;

   GLfloat *buffer;
   GLuint count;
   GLuint wrap_count;
   GLuint replay_flags;

   struct tnl_prim *prim;
   GLuint prim_count, prim_max;

   struct tnl_vertex_store *vertex_store;
   struct tnl_primitive_store *prim_store;

   GLfloat *vbptr;		   /* cursor, points into buffer */
   GLfloat vertex[_TNL_ATTRIB_MAX*4];	   /* current values */
   GLfloat *attrptr[_TNL_ATTRIB_MAX];
   GLuint counter, initial_counter;
   GLboolean dangling_attr_ref;
   GLboolean have_materials;

   GLuint opcode_vertex_list;

   struct tnl_copied_vtx copied;

   GLfloat CurrentFloatEdgeFlag;

   GLfloat *current[_TNL_ATTRIB_MAX]; /* points into ctx->ListState */
   GLubyte *currentsz[_TNL_ATTRIB_MAX];

   void (*tabfv[_TNL_ATTRIB_MAX][4])( const GLfloat * );
};


struct tnl_vertex_arrays
{
   /* Conventional vertex attribute arrays */
   GLvector4f  Obj;
   GLvector4f  Normal;
   GLvector4f  Color;
   GLvector4f  SecondaryColor;
   GLvector4f  FogCoord;
   GLvector4f  TexCoord[MAX_TEXTURE_COORD_UNITS];
   GLvector4f  Index;

   GLubyte     *EdgeFlag;
   GLuint      *Elt;

   /* These attributes don't alias with the conventional attributes.
    * The GL_NV_vertex_program extension defines 16 extra sets of vertex
    * arrays which have precedent over the conventional arrays when enabled.
    */
   GLvector4f  Attribs[_TNL_ATTRIB_MAX];
};


/**
 * Contains the current state of a running pipeline.
 */
struct vertex_buffer
{
   /* Constant over life of the vertex_buffer.
    */
   GLuint      Size;

   /* Constant over the pipeline.
    */
   GLuint      Count;		              /* for everything except Elts */

   /* Pointers to current data.
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
   GLvector4f  *PointSizePtr;	                /* _TNL_BIT_POS */
   GLvector4f  *FogCoordPtr;	                /* _TNL_BIT_FOG */

   struct tnl_prim  *Primitive;
   GLuint      PrimitiveCount;

   /* Inputs to the vertex program stage */
   GLvector4f *AttribPtr[_TNL_ATTRIB_MAX];      /* GL_NV_vertex_program */

   GLuint LastClipped;
   /* Private data from _tnl_render_stage that has no business being
    * in this struct.
    */
};


/** Describes an individual operation on the pipeline.
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


struct tnl_clipspace_fastpath {
   GLuint vertex_size;
   GLuint attr_count;
   GLboolean match_strides;

   struct {
      GLuint format;
      GLuint size;
      GLuint stride;
      GLuint offset;
   } *attr;

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



struct tnl_cache {
   GLuint hash;
   void *key;
   void *data;
   struct tnl_cache *next;
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

   GLboolean (*NotifyBegin)(GLcontext *ctx, GLenum p);
   /* Allow drivers to hook in optimized begin/end engines.
    * Return value:  GL_TRUE - driver handled the begin
    *                GL_FALSE - driver didn't handle the begin
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


/**
 * Context state for T&L context.
 */
typedef struct
{
   /* Driver interface.
    */
   struct tnl_device_driver Driver;

   /* Execute:
    */
   struct tnl_vtx vtx;

   /* Compile:
    */
   struct tnl_save save;

   /* Pipeline
    */
   struct tnl_pipeline pipeline;
   struct vertex_buffer vb;

   /* GLvectors for binding to vb:
    */
   struct tnl_vertex_arrays vtx_inputs;
   struct tnl_vertex_arrays save_inputs;
   struct tnl_vertex_arrays current;
   struct tnl_vertex_arrays array_inputs;

   /* Clipspace/ndc/window vertex managment:
    */
   struct tnl_clipspace clipspace;

   /* Probably need a better configuration mechanism:
    */
   GLboolean NeedNdcCoords;
   GLboolean LoopbackDListCassettes;
   GLboolean CalcDListNormalLengths;
   GLboolean IsolateMaterials;
   GLboolean AllowVertexFog;
   GLboolean AllowPixelFog;
   GLboolean AllowCodegen;

   GLboolean _DoVertexFog;  /* eval fog function at each vertex? */

   GLuint render_inputs;

   GLvertexformat exec_vtxfmt;
   GLvertexformat save_vtxfmt;

   struct tnl_cache *vp_cache;

} TNLcontext;



#define TNL_CONTEXT(ctx) ((TNLcontext *)((ctx)->swtnl_context))


#define TYPE_IDX(t) ((t) & 0xf)
#define MAX_TYPES TYPE_IDX(GL_DOUBLE)+1      /* 0xa + 1 */

extern void _tnl_MakeCurrent( GLcontext *ctx,
			      GLframebuffer *drawBuffer,
			      GLframebuffer *readBuffer );




#endif

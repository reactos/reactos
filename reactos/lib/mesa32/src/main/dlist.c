/**
 * \file dlist.c
 * Display lists management functions.
 */

/*
 * Mesa 3-D graphics library
 * Version:  6.0
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


#include "glheader.h"
#include "imports.h"
#include "api_arrayelt.h"
#include "api_loopback.h"
#include "config.h"
#if FEATURE_ARB_vertex_program || FEATURE_ARB_fragment_program
#include "arbprogram.h"
#include "program.h"
#endif
#include "attrib.h"
#include "blend.h"
#include "buffers.h"
#if FEATURE_ARB_vertex_buffer_object
#include "bufferobj.h"
#endif
#include "clip.h"
#include "colormac.h"
#include "colortab.h"
#include "context.h"
#include "convolve.h"
#include "depth.h"
#include "dlist.h"
#include "enable.h"
#include "enums.h"
#include "eval.h"
#include "extensions.h"
#include "feedback.h"
#include "get.h"
#include "glapi.h"
#include "hash.h"
#include "histogram.h"
#include "image.h"
#include "light.h"
#include "lines.h"
#include "dlist.h"
#include "macros.h"
#include "matrix.h"
#include "pixel.h"
#include "points.h"
#include "polygon.h"
#include "state.h"
#include "texobj.h"
#include "teximage.h"
#include "texstate.h"
#include "mtypes.h"
#include "varray.h"
#if FEATURE_NV_vertex_program || FEATURE_NV_fragment_program
#include "nvprogram.h"
#include "program.h"
#endif

#include "math/m_matrix.h"
#include "math/m_xform.h"


/**
 * Flush vertices.
 *
 * \param ctx GL context.
 *
 * Checks if dd_function_table::SaveNeedFlush is marked to flush
 * stored (save) vertices, and calls
 * dd_function_table::SaveFlushVertices if so.
 */
#define SAVE_FLUSH_VERTICES(ctx)		\
do {						\
   if (ctx->Driver.SaveNeedFlush)		\
      ctx->Driver.SaveFlushVertices(ctx);	\
} while (0)


/**
 * Macro to assert that the API call was made outside the
 * glBegin()/glEnd() pair, with return value.
 * 
 * \param ctx GL context.
 * \param retval value to return value in case the assertion fails.
 */
#define ASSERT_OUTSIDE_SAVE_BEGIN_END_WITH_RETVAL(ctx, retval)		\
do {									\
   if (ctx->Driver.CurrentSavePrimitive <= GL_POLYGON ||		\
       ctx->Driver.CurrentSavePrimitive == PRIM_INSIDE_UNKNOWN_PRIM) {	\
      _mesa_compile_error( ctx, GL_INVALID_OPERATION, "begin/end" );	\
      return retval;							\
   }									\
} while (0)

/**
 * Macro to assert that the API call was made outside the
 * glBegin()/glEnd() pair.
 * 
 * \param ctx GL context.
 */
#define ASSERT_OUTSIDE_SAVE_BEGIN_END(ctx)				\
do {									\
   if (ctx->Driver.CurrentSavePrimitive <= GL_POLYGON ||		\
       ctx->Driver.CurrentSavePrimitive == PRIM_INSIDE_UNKNOWN_PRIM) {	\
      _mesa_compile_error( ctx, GL_INVALID_OPERATION, "begin/end" );	\
      return;								\
   }									\
} while (0)

/**
 * Macro to assert that the API call was made outside the
 * glBegin()/glEnd() pair and flush the vertices.
 * 
 * \param ctx GL context.
 */
#define ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx)			\
do {									\
   ASSERT_OUTSIDE_SAVE_BEGIN_END(ctx);					\
   SAVE_FLUSH_VERTICES(ctx);						\
} while (0)

/**
 * Macro to assert that the API call was made outside the
 * glBegin()/glEnd() pair and flush the vertices, with return value.
 * 
 * \param ctx GL context.
 * \param retval value to return value in case the assertion fails.
 */
#define ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH_WITH_RETVAL(ctx, retval)\
do {									\
   ASSERT_OUTSIDE_SAVE_BEGIN_END_WITH_RETVAL(ctx, retval);		\
   SAVE_FLUSH_VERTICES(ctx);						\
} while (0)



/**
 * Display list opcodes.
 *
 * The fact that these identifiers are assigned consecutive
 * integer values starting at 0 is very important, see InstSize array usage)
 */
typedef enum {
	OPCODE_ACCUM,
	OPCODE_ALPHA_FUNC,
        OPCODE_BIND_TEXTURE,
	OPCODE_BITMAP,
	OPCODE_BLEND_COLOR,
	OPCODE_BLEND_EQUATION,
	OPCODE_BLEND_FUNC,
	OPCODE_BLEND_FUNC_SEPARATE,
        OPCODE_CALL_LIST,
        OPCODE_CALL_LIST_OFFSET,
	OPCODE_CLEAR,
	OPCODE_CLEAR_ACCUM,
	OPCODE_CLEAR_COLOR,
	OPCODE_CLEAR_DEPTH,
	OPCODE_CLEAR_INDEX,
	OPCODE_CLEAR_STENCIL,
        OPCODE_CLIP_PLANE,
	OPCODE_COLOR_MASK,
	OPCODE_COLOR_MATERIAL,
	OPCODE_COLOR_TABLE,
	OPCODE_COLOR_TABLE_PARAMETER_FV,
	OPCODE_COLOR_TABLE_PARAMETER_IV,
	OPCODE_COLOR_SUB_TABLE,
        OPCODE_CONVOLUTION_FILTER_1D,
        OPCODE_CONVOLUTION_FILTER_2D,
        OPCODE_CONVOLUTION_PARAMETER_I,
        OPCODE_CONVOLUTION_PARAMETER_IV,
        OPCODE_CONVOLUTION_PARAMETER_F,
        OPCODE_CONVOLUTION_PARAMETER_FV,
        OPCODE_COPY_COLOR_SUB_TABLE,
        OPCODE_COPY_COLOR_TABLE,
	OPCODE_COPY_PIXELS,
        OPCODE_COPY_TEX_IMAGE1D,
        OPCODE_COPY_TEX_IMAGE2D,
        OPCODE_COPY_TEX_SUB_IMAGE1D,
        OPCODE_COPY_TEX_SUB_IMAGE2D,
        OPCODE_COPY_TEX_SUB_IMAGE3D,
	OPCODE_CULL_FACE,
	OPCODE_DEPTH_FUNC,
	OPCODE_DEPTH_MASK,
	OPCODE_DEPTH_RANGE,
	OPCODE_DISABLE,
	OPCODE_DRAW_BUFFER,
	OPCODE_DRAW_PIXELS,
	OPCODE_ENABLE,
	OPCODE_EVALMESH1,
	OPCODE_EVALMESH2,
	OPCODE_FOG,
	OPCODE_FRONT_FACE,
	OPCODE_FRUSTUM,
	OPCODE_HINT,
	OPCODE_HISTOGRAM,
	OPCODE_INDEX_MASK,
	OPCODE_INIT_NAMES,
	OPCODE_LIGHT,
	OPCODE_LIGHT_MODEL,
	OPCODE_LINE_STIPPLE,
	OPCODE_LINE_WIDTH,
	OPCODE_LIST_BASE,
	OPCODE_LOAD_IDENTITY,
	OPCODE_LOAD_MATRIX,
	OPCODE_LOAD_NAME,
	OPCODE_LOGIC_OP,
	OPCODE_MAP1,
	OPCODE_MAP2,
	OPCODE_MAPGRID1,
	OPCODE_MAPGRID2,
	OPCODE_MATRIX_MODE,
        OPCODE_MIN_MAX,
	OPCODE_MULT_MATRIX,
	OPCODE_ORTHO,
	OPCODE_PASSTHROUGH,
	OPCODE_PIXEL_MAP,
	OPCODE_PIXEL_TRANSFER,
	OPCODE_PIXEL_ZOOM,
	OPCODE_POINT_SIZE,
        OPCODE_POINT_PARAMETERS,
	OPCODE_POLYGON_MODE,
        OPCODE_POLYGON_STIPPLE,
	OPCODE_POLYGON_OFFSET,
	OPCODE_POP_ATTRIB,
	OPCODE_POP_MATRIX,
	OPCODE_POP_NAME,
	OPCODE_PRIORITIZE_TEXTURE,
	OPCODE_PUSH_ATTRIB,
	OPCODE_PUSH_MATRIX,
	OPCODE_PUSH_NAME,
	OPCODE_RASTER_POS,
	OPCODE_READ_BUFFER,
        OPCODE_RESET_HISTOGRAM,
        OPCODE_RESET_MIN_MAX,
        OPCODE_ROTATE,
        OPCODE_SCALE,
	OPCODE_SCISSOR,
	OPCODE_SELECT_TEXTURE_SGIS,
	OPCODE_SELECT_TEXTURE_COORD_SET,
	OPCODE_SHADE_MODEL,
	OPCODE_STENCIL_FUNC,
	OPCODE_STENCIL_MASK,
	OPCODE_STENCIL_OP,
        OPCODE_TEXENV,
        OPCODE_TEXGEN,
        OPCODE_TEXPARAMETER,
	OPCODE_TEX_IMAGE1D,
	OPCODE_TEX_IMAGE2D,
	OPCODE_TEX_IMAGE3D,
	OPCODE_TEX_SUB_IMAGE1D,
	OPCODE_TEX_SUB_IMAGE2D,
	OPCODE_TEX_SUB_IMAGE3D,
        OPCODE_TRANSLATE,
	OPCODE_VIEWPORT,
	OPCODE_WINDOW_POS,
        /* GL_ARB_multitexture */
        OPCODE_ACTIVE_TEXTURE,
        /* GL_SGIX/SGIS_pixel_texture */
        OPCODE_PIXEL_TEXGEN_SGIX,
        OPCODE_PIXEL_TEXGEN_PARAMETER_SGIS,
        /* GL_ARB_texture_compression */
        OPCODE_COMPRESSED_TEX_IMAGE_1D,
        OPCODE_COMPRESSED_TEX_IMAGE_2D,
        OPCODE_COMPRESSED_TEX_IMAGE_3D,
        OPCODE_COMPRESSED_TEX_SUB_IMAGE_1D,
        OPCODE_COMPRESSED_TEX_SUB_IMAGE_2D,
        OPCODE_COMPRESSED_TEX_SUB_IMAGE_3D,
        /* GL_ARB_multisample */
        OPCODE_SAMPLE_COVERAGE,
        /* GL_ARB_window_pos */
	OPCODE_WINDOW_POS_ARB,
        /* GL_NV_vertex_program */
        OPCODE_BIND_PROGRAM_NV,
        OPCODE_EXECUTE_PROGRAM_NV,
        OPCODE_REQUEST_RESIDENT_PROGRAMS_NV,
        OPCODE_LOAD_PROGRAM_NV,
        OPCODE_PROGRAM_PARAMETER4F_NV,
        OPCODE_TRACK_MATRIX_NV,
        /* GL_NV_fragment_program */
        OPCODE_PROGRAM_LOCAL_PARAMETER_ARB,
        OPCODE_PROGRAM_NAMED_PARAMETER_NV,
        /* GL_EXT_stencil_two_side */
        OPCODE_ACTIVE_STENCIL_FACE_EXT,
        /* GL_EXT_depth_bounds_test */
        OPCODE_DEPTH_BOUNDS_EXT,
        /* GL_ARB_vertex/fragment_program */
        OPCODE_PROGRAM_STRING_ARB,
        OPCODE_PROGRAM_ENV_PARAMETER_ARB,


	/* Vertex attributes -- fallback for when optimized display
	 * list build isn't active.
	 */
	OPCODE_ATTR_1F,
	OPCODE_ATTR_2F,
	OPCODE_ATTR_3F,
	OPCODE_ATTR_4F,
	OPCODE_MATERIAL,
	OPCODE_INDEX,
	OPCODE_EDGEFLAG,
	OPCODE_BEGIN,
	OPCODE_END,
	OPCODE_RECTF,
	OPCODE_EVAL_C1,
	OPCODE_EVAL_C2,
	OPCODE_EVAL_P1,
	OPCODE_EVAL_P2,


	/* The following three are meta instructions */
	OPCODE_ERROR,	        /* raise compiled-in error */
	OPCODE_CONTINUE,
	OPCODE_END_OF_LIST,
	OPCODE_DRV_0
} OpCode;



/**
 * Display list node.
 *
 * Display list instructions are stored as sequences of "nodes".  Nodes
 * are allocated in blocks.  Each block has BLOCK_SIZE nodes.  Blocks
 * are linked together with a pointer.
 *
 * Each instruction in the display list is stored as a sequence of
 * contiguous nodes in memory.
 * Each node is the union of a variety of data types.
 */
union node {
	OpCode		opcode;
	GLboolean	b;
	GLbitfield	bf;
	GLubyte		ub;
	GLshort		s;
	GLushort	us;
	GLint		i;
	GLuint		ui;
	GLenum		e;
	GLfloat		f;
	GLvoid		*data;
	void		*next;	/* If prev node's opcode==OPCODE_CONTINUE */
};


/**
 * How many nodes to allocate at a time.
 *
 * \note Reduced now that we hold vertices etc. elsewhere.
 */
#define BLOCK_SIZE 256



/**
 * Number of nodes of storage needed for each instruction.
 * Sizes for dynamically allocated opcodes are stored in the context struct.
 */
static GLuint InstSize[ OPCODE_END_OF_LIST+1 ];

void mesa_print_display_list( GLuint list );


/**********************************************************************/
/*****                           Private                          *****/
/**********************************************************************/

/*
 * Make an empty display list.  This is used by glGenLists() to
 * reserver display list IDs.
 */
static Node *make_empty_list( void )
{
   Node *n = (Node *) MALLOC( sizeof(Node) );
   n[0].opcode = OPCODE_END_OF_LIST;
   return n;
}



/*
 * Destroy all nodes in a display list.
 * \param list - display list number
 */
void _mesa_destroy_list( GLcontext *ctx, GLuint list )
{
   Node *n, *block;
   GLboolean done;

   if (list==0)
      return;

   block = (Node *) _mesa_HashLookup(ctx->Shared->DisplayList, list);
   n = block;

   done = block ? GL_FALSE : GL_TRUE;
   while (!done) {

      /* check for extension opcodes first */

      GLint i = (GLint) n[0].opcode - (GLint) OPCODE_DRV_0;
      if (i >= 0 && i < (GLint) ctx->listext.nr_opcodes) {
	 ctx->listext.opcode[i].destroy(ctx, &n[1]);
	 n += ctx->listext.opcode[i].size;
      }
      else {
	 switch (n[0].opcode) {
         /* for some commands, we need to free malloc'd memory */
	 case OPCODE_MAP1:
            FREE(n[6].data);
	    n += InstSize[n[0].opcode];
	    break;
	 case OPCODE_MAP2:
            FREE(n[10].data);
	    n += InstSize[n[0].opcode];
	    break;
	 case OPCODE_DRAW_PIXELS:
	    FREE( n[5].data );
	    n += InstSize[n[0].opcode];
	    break;
	 case OPCODE_BITMAP:
	    FREE( n[7].data );
	    n += InstSize[n[0].opcode];
	    break;
         case OPCODE_COLOR_TABLE:
            FREE( n[6].data );
            n += InstSize[n[0].opcode];
            break;
         case OPCODE_COLOR_SUB_TABLE:
            FREE( n[6].data );
            n += InstSize[n[0].opcode];
            break;
         case OPCODE_CONVOLUTION_FILTER_1D:
            FREE( n[6].data );
            n += InstSize[n[0].opcode];
            break;
         case OPCODE_CONVOLUTION_FILTER_2D:
            FREE( n[7].data );
            n += InstSize[n[0].opcode];
            break;
         case OPCODE_POLYGON_STIPPLE:
            FREE( n[1].data );
	    n += InstSize[n[0].opcode];
            break;
	 case OPCODE_TEX_IMAGE1D:
            FREE(n[8].data);
            n += InstSize[n[0].opcode];
	    break;
	 case OPCODE_TEX_IMAGE2D:
            FREE( n[9]. data );
            n += InstSize[n[0].opcode];
	    break;
	 case OPCODE_TEX_IMAGE3D:
            FREE( n[10]. data );
            n += InstSize[n[0].opcode];
	    break;
         case OPCODE_TEX_SUB_IMAGE1D:
            FREE(n[7].data);
            n += InstSize[n[0].opcode];
            break;
         case OPCODE_TEX_SUB_IMAGE2D:
            FREE(n[9].data);
            n += InstSize[n[0].opcode];
            break;
         case OPCODE_TEX_SUB_IMAGE3D:
            FREE(n[11].data);
            n += InstSize[n[0].opcode];
            break;
         case OPCODE_COMPRESSED_TEX_IMAGE_1D:
            FREE(n[7].data);
            n += InstSize[n[0].opcode];
            break;
         case OPCODE_COMPRESSED_TEX_IMAGE_2D:
            FREE(n[8].data);
            n += InstSize[n[0].opcode];
            break;
         case OPCODE_COMPRESSED_TEX_IMAGE_3D:
            FREE(n[9].data);
            n += InstSize[n[0].opcode];
            break;
         case OPCODE_COMPRESSED_TEX_SUB_IMAGE_1D:
            FREE(n[7].data);
            n += InstSize[n[0].opcode];
            break;
         case OPCODE_COMPRESSED_TEX_SUB_IMAGE_2D:
            FREE(n[9].data);
            n += InstSize[n[0].opcode];
            break;
         case OPCODE_COMPRESSED_TEX_SUB_IMAGE_3D:
            FREE(n[11].data);
            n += InstSize[n[0].opcode];
            break;
#if FEATURE_NV_vertex_program
         case OPCODE_LOAD_PROGRAM_NV:
            FREE(n[4].data);  /* program string */
            n += InstSize[n[0].opcode];
            break;
         case OPCODE_REQUEST_RESIDENT_PROGRAMS_NV:
            FREE(n[2].data);  /* array of program ids */
            n += InstSize[n[0].opcode];
            break;
#endif
#if FEATURE_NV_fragment_program
         case OPCODE_PROGRAM_NAMED_PARAMETER_NV:
            FREE(n[3].data);  /* parameter name */
            n += InstSize[n[0].opcode];
            break;
#endif
#if FEATURE_ARB_vertex_program || FEATURE_ARB_fragment_program
         case OPCODE_PROGRAM_STRING_ARB:
            FREE(n[4].data);  /* program string */
            n += InstSize[n[0].opcode];
            break;
#endif
	 case OPCODE_CONTINUE:
	    n = (Node *) n[1].next;
	    FREE( block );
	    block = n;
	    break;
	 case OPCODE_END_OF_LIST:
	    FREE( block );
	    done = GL_TRUE;
	    break;
	 default:
	    /* Most frequent case */
	    n += InstSize[n[0].opcode];
	    break;
	 }
      }
   }

   _mesa_HashRemove(ctx->Shared->DisplayList, list);
}



/*
 * Translate the nth element of list from type to GLuint.
 */
static GLuint translate_id( GLsizei n, GLenum type, const GLvoid *list )
{
   GLbyte *bptr;
   GLubyte *ubptr;
   GLshort *sptr;
   GLushort *usptr;
   GLint *iptr;
   GLuint *uiptr;
   GLfloat *fptr;

   switch (type) {
      case GL_BYTE:
         bptr = (GLbyte *) list;
         return (GLuint) *(bptr+n);
      case GL_UNSIGNED_BYTE:
         ubptr = (GLubyte *) list;
         return (GLuint) *(ubptr+n);
      case GL_SHORT:
         sptr = (GLshort *) list;
         return (GLuint) *(sptr+n);
      case GL_UNSIGNED_SHORT:
         usptr = (GLushort *) list;
         return (GLuint) *(usptr+n);
      case GL_INT:
         iptr = (GLint *) list;
         return (GLuint) *(iptr+n);
      case GL_UNSIGNED_INT:
         uiptr = (GLuint *) list;
         return (GLuint) *(uiptr+n);
      case GL_FLOAT:
         fptr = (GLfloat *) list;
         return (GLuint) *(fptr+n);
      case GL_2_BYTES:
         ubptr = ((GLubyte *) list) + 2*n;
         return (GLuint) *ubptr * 256 + (GLuint) *(ubptr+1);
      case GL_3_BYTES:
         ubptr = ((GLubyte *) list) + 3*n;
         return (GLuint) *ubptr * 65536
              + (GLuint) *(ubptr+1) * 256
              + (GLuint) *(ubptr+2);
      case GL_4_BYTES:
         ubptr = ((GLubyte *) list) + 4*n;
         return (GLuint) *ubptr * 16777216
              + (GLuint) *(ubptr+1) * 65536
              + (GLuint) *(ubptr+2) * 256
              + (GLuint) *(ubptr+3);
      default:
         return 0;
   }
}




/**********************************************************************/
/*****                        Public                              *****/
/**********************************************************************/

void _mesa_init_lists( void )
{
   static int init_flag = 0;

   if (init_flag==0) {
      InstSize[OPCODE_ACCUM] = 3;
      InstSize[OPCODE_ALPHA_FUNC] = 3;
      InstSize[OPCODE_BIND_TEXTURE] = 3;
      InstSize[OPCODE_BITMAP] = 8;
      InstSize[OPCODE_BLEND_COLOR] = 5;
      InstSize[OPCODE_BLEND_EQUATION] = 2;
      InstSize[OPCODE_BLEND_FUNC] = 3;
      InstSize[OPCODE_BLEND_FUNC_SEPARATE] = 5;
      InstSize[OPCODE_CALL_LIST] = 2;
      InstSize[OPCODE_CALL_LIST_OFFSET] = 3;
      InstSize[OPCODE_CLEAR] = 2;
      InstSize[OPCODE_CLEAR_ACCUM] = 5;
      InstSize[OPCODE_CLEAR_COLOR] = 5;
      InstSize[OPCODE_CLEAR_DEPTH] = 2;
      InstSize[OPCODE_CLEAR_INDEX] = 2;
      InstSize[OPCODE_CLEAR_STENCIL] = 2;
      InstSize[OPCODE_CLIP_PLANE] = 6;
      InstSize[OPCODE_COLOR_MASK] = 5;
      InstSize[OPCODE_COLOR_MATERIAL] = 3;
      InstSize[OPCODE_COLOR_TABLE] = 7;
      InstSize[OPCODE_COLOR_TABLE_PARAMETER_FV] = 7;
      InstSize[OPCODE_COLOR_TABLE_PARAMETER_IV] = 7;
      InstSize[OPCODE_COLOR_SUB_TABLE] = 7;
      InstSize[OPCODE_CONVOLUTION_FILTER_1D] = 7;
      InstSize[OPCODE_CONVOLUTION_FILTER_2D] = 8;
      InstSize[OPCODE_CONVOLUTION_PARAMETER_I] = 4;
      InstSize[OPCODE_CONVOLUTION_PARAMETER_IV] = 7;
      InstSize[OPCODE_CONVOLUTION_PARAMETER_F] = 4;
      InstSize[OPCODE_CONVOLUTION_PARAMETER_FV] = 7;
      InstSize[OPCODE_COPY_PIXELS] = 6;
      InstSize[OPCODE_COPY_COLOR_SUB_TABLE] = 6;
      InstSize[OPCODE_COPY_COLOR_TABLE] = 6;
      InstSize[OPCODE_COPY_TEX_IMAGE1D] = 8;
      InstSize[OPCODE_COPY_TEX_IMAGE2D] = 9;
      InstSize[OPCODE_COPY_TEX_SUB_IMAGE1D] = 7;
      InstSize[OPCODE_COPY_TEX_SUB_IMAGE2D] = 9;
      InstSize[OPCODE_COPY_TEX_SUB_IMAGE3D] = 10;
      InstSize[OPCODE_CULL_FACE] = 2;
      InstSize[OPCODE_DEPTH_FUNC] = 2;
      InstSize[OPCODE_DEPTH_MASK] = 2;
      InstSize[OPCODE_DEPTH_RANGE] = 3;
      InstSize[OPCODE_DISABLE] = 2;
      InstSize[OPCODE_DRAW_BUFFER] = 2;
      InstSize[OPCODE_DRAW_PIXELS] = 6;
      InstSize[OPCODE_ENABLE] = 2;
      InstSize[OPCODE_EVALMESH1] = 4;
      InstSize[OPCODE_EVALMESH2] = 6;
      InstSize[OPCODE_FOG] = 6;
      InstSize[OPCODE_FRONT_FACE] = 2;
      InstSize[OPCODE_FRUSTUM] = 7;
      InstSize[OPCODE_HINT] = 3;
      InstSize[OPCODE_HISTOGRAM] = 5;
      InstSize[OPCODE_INDEX_MASK] = 2;
      InstSize[OPCODE_INIT_NAMES] = 1;
      InstSize[OPCODE_LIGHT] = 7;
      InstSize[OPCODE_LIGHT_MODEL] = 6;
      InstSize[OPCODE_LINE_STIPPLE] = 3;
      InstSize[OPCODE_LINE_WIDTH] = 2;
      InstSize[OPCODE_LIST_BASE] = 2;
      InstSize[OPCODE_LOAD_IDENTITY] = 1;
      InstSize[OPCODE_LOAD_MATRIX] = 17;
      InstSize[OPCODE_LOAD_NAME] = 2;
      InstSize[OPCODE_LOGIC_OP] = 2;
      InstSize[OPCODE_MAP1] = 7;
      InstSize[OPCODE_MAP2] = 11;
      InstSize[OPCODE_MAPGRID1] = 4;
      InstSize[OPCODE_MAPGRID2] = 7;
      InstSize[OPCODE_MATRIX_MODE] = 2;
      InstSize[OPCODE_MIN_MAX] = 4;
      InstSize[OPCODE_MULT_MATRIX] = 17;
      InstSize[OPCODE_ORTHO] = 7;
      InstSize[OPCODE_PASSTHROUGH] = 2;
      InstSize[OPCODE_PIXEL_MAP] = 4;
      InstSize[OPCODE_PIXEL_TRANSFER] = 3;
      InstSize[OPCODE_PIXEL_ZOOM] = 3;
      InstSize[OPCODE_POINT_SIZE] = 2;
      InstSize[OPCODE_POINT_PARAMETERS] = 5;
      InstSize[OPCODE_POLYGON_MODE] = 3;
      InstSize[OPCODE_POLYGON_STIPPLE] = 2;
      InstSize[OPCODE_POLYGON_OFFSET] = 3;
      InstSize[OPCODE_POP_ATTRIB] = 1;
      InstSize[OPCODE_POP_MATRIX] = 1;
      InstSize[OPCODE_POP_NAME] = 1;
      InstSize[OPCODE_PRIORITIZE_TEXTURE] = 3;
      InstSize[OPCODE_PUSH_ATTRIB] = 2;
      InstSize[OPCODE_PUSH_MATRIX] = 1;
      InstSize[OPCODE_PUSH_NAME] = 2;
      InstSize[OPCODE_RASTER_POS] = 5;
      InstSize[OPCODE_READ_BUFFER] = 2;
      InstSize[OPCODE_RESET_HISTOGRAM] = 2;
      InstSize[OPCODE_RESET_MIN_MAX] = 2;
      InstSize[OPCODE_ROTATE] = 5;
      InstSize[OPCODE_SCALE] = 4;
      InstSize[OPCODE_SCISSOR] = 5;
      InstSize[OPCODE_STENCIL_FUNC] = 4;
      InstSize[OPCODE_STENCIL_MASK] = 2;
      InstSize[OPCODE_STENCIL_OP] = 4;
      InstSize[OPCODE_SHADE_MODEL] = 2;
      InstSize[OPCODE_TEXENV] = 7;
      InstSize[OPCODE_TEXGEN] = 7;
      InstSize[OPCODE_TEXPARAMETER] = 7;
      InstSize[OPCODE_TEX_IMAGE1D] = 9;
      InstSize[OPCODE_TEX_IMAGE2D] = 10;
      InstSize[OPCODE_TEX_IMAGE3D] = 11;
      InstSize[OPCODE_TEX_SUB_IMAGE1D] = 8;
      InstSize[OPCODE_TEX_SUB_IMAGE2D] = 10;
      InstSize[OPCODE_TEX_SUB_IMAGE3D] = 12;
      InstSize[OPCODE_TRANSLATE] = 4;
      InstSize[OPCODE_VIEWPORT] = 5;
      InstSize[OPCODE_WINDOW_POS] = 5;
      InstSize[OPCODE_CONTINUE] = 2;
      InstSize[OPCODE_ERROR] = 3;
      InstSize[OPCODE_END_OF_LIST] = 1;
      /* GL_SGIX/SGIS_pixel_texture */
      InstSize[OPCODE_PIXEL_TEXGEN_SGIX] = 2;
      InstSize[OPCODE_PIXEL_TEXGEN_PARAMETER_SGIS] = 3;
      /* GL_ARB_texture_compression */
      InstSize[OPCODE_COMPRESSED_TEX_IMAGE_1D] = 8;
      InstSize[OPCODE_COMPRESSED_TEX_IMAGE_2D] = 9;
      InstSize[OPCODE_COMPRESSED_TEX_IMAGE_3D] = 10;
      InstSize[OPCODE_COMPRESSED_TEX_SUB_IMAGE_1D] = 8;
      InstSize[OPCODE_COMPRESSED_TEX_SUB_IMAGE_2D] = 10;
      InstSize[OPCODE_COMPRESSED_TEX_SUB_IMAGE_3D] = 12;
      /* GL_ARB_multisample */
      InstSize[OPCODE_SAMPLE_COVERAGE] = 3;
      /* GL_ARB_multitexture */
      InstSize[OPCODE_ACTIVE_TEXTURE] = 2;
      /* GL_ARB_window_pos */
      InstSize[OPCODE_WINDOW_POS_ARB] = 4;
      /* GL_NV_vertex_program */
      InstSize[OPCODE_BIND_PROGRAM_NV] = 3;
      InstSize[OPCODE_EXECUTE_PROGRAM_NV] = 7;
      InstSize[OPCODE_REQUEST_RESIDENT_PROGRAMS_NV] = 2;
      InstSize[OPCODE_LOAD_PROGRAM_NV] = 5;
      InstSize[OPCODE_PROGRAM_PARAMETER4F_NV] = 7;
      InstSize[OPCODE_TRACK_MATRIX_NV] = 5;
      /* GL_NV_fragment_program */
      InstSize[OPCODE_PROGRAM_LOCAL_PARAMETER_ARB] = 7;
      InstSize[OPCODE_PROGRAM_NAMED_PARAMETER_NV] = 8;
      /* GL_EXT_stencil_two_side */
      InstSize[OPCODE_ACTIVE_STENCIL_FACE_EXT] = 2;
      /* GL_EXT_depth_bounds_test */
      InstSize[OPCODE_DEPTH_BOUNDS_EXT] = 3;
#if FEATURE_ARB_vertex_program || FEATURE_ARB_fragment_program
      InstSize[OPCODE_PROGRAM_STRING_ARB] = 5;
      InstSize[OPCODE_PROGRAM_ENV_PARAMETER_ARB] = 7;
#endif
      InstSize[OPCODE_ATTR_1F] = 3;
      InstSize[OPCODE_ATTR_2F] = 4;
      InstSize[OPCODE_ATTR_3F] = 5;
      InstSize[OPCODE_ATTR_4F] = 6;
      InstSize[OPCODE_MATERIAL] = 7;
      InstSize[OPCODE_INDEX] = 2;
      InstSize[OPCODE_EDGEFLAG] = 2;
      InstSize[OPCODE_BEGIN] = 2;
      InstSize[OPCODE_END] = 1;
      InstSize[OPCODE_RECTF] = 5;
      InstSize[OPCODE_EVAL_C1] = 2;
      InstSize[OPCODE_EVAL_C2] = 3;
      InstSize[OPCODE_EVAL_P1] = 2;
      InstSize[OPCODE_EVAL_P2] = 3;
   }
   init_flag = 1;
}


/*
 * Allocate space for a display list instruction.
 * \param opcode - type of instruction
 *         argcount - size in bytes of data required.
 * \return pointer to the usable data area (not including the internal
 *         opcode).
 */
void *
_mesa_alloc_instruction( GLcontext *ctx, int opcode, GLint sz )
{
   Node *n, *newblock;
   GLuint count = 1 + (sz + sizeof(Node) - 1) / sizeof(Node);

#ifdef DEBUG
   if (opcode < (int) OPCODE_DRV_0) {
      assert( count == InstSize[opcode] );
   }
#endif

   if (ctx->ListState.CurrentPos + count + 2 > BLOCK_SIZE) {
      /* This block is full.  Allocate a new block and chain to it */
      n = ctx->ListState.CurrentBlock + ctx->ListState.CurrentPos;
      n[0].opcode = OPCODE_CONTINUE;
      newblock = (Node *) MALLOC( sizeof(Node) * BLOCK_SIZE );
      if (!newblock) {
         _mesa_error( ctx, GL_OUT_OF_MEMORY, "Building display list" );
         return NULL;
      }
      n[1].next = (Node *) newblock;
      ctx->ListState.CurrentBlock = newblock;
      ctx->ListState.CurrentPos = 0;
   }

   n = ctx->ListState.CurrentBlock + ctx->ListState.CurrentPos;
   ctx->ListState.CurrentPos += count;

   n[0].opcode = (OpCode) opcode;

   return (void *)&n[1];
}


/* Allow modules and drivers to get their own opcodes.
 */
int
_mesa_alloc_opcode( GLcontext *ctx,
		    GLuint sz,
		    void (*execute)( GLcontext *, void * ),
		    void (*destroy)( GLcontext *, void * ),
		    void (*print)( GLcontext *, void * ) )
{
   if (ctx->listext.nr_opcodes < GL_MAX_EXT_OPCODES) {
      GLuint i = ctx->listext.nr_opcodes++;
      ctx->listext.opcode[i].size = 1 + (sz + sizeof(Node) - 1)/sizeof(Node);
      ctx->listext.opcode[i].execute = execute;
      ctx->listext.opcode[i].destroy = destroy;
      ctx->listext.opcode[i].print = print;
      return i + OPCODE_DRV_0;
   }
   return -1;
}



/* Mimic the old behaviour of alloc_instruction:
 *   - sz is in units of sizeof(Node)
 *   - return value a pointer to sizeof(Node) before the actual
 *     usable data area.
 */
#define ALLOC_INSTRUCTION(ctx, opcode, sz) \
        ((Node *)_mesa_alloc_instruction(ctx, opcode, sz*sizeof(Node)) - 1)



/*
 * Display List compilation functions
 */
static void GLAPIENTRY save_Accum( GLenum op, GLfloat value )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_ACCUM, 2 );
   if (n) {
      n[1].e = op;
      n[2].f = value;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->Accum)( op, value );
   }
}


static void GLAPIENTRY save_AlphaFunc( GLenum func, GLclampf ref )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_ALPHA_FUNC, 2 );
   if (n) {
      n[1].e = func;
      n[2].f = (GLfloat) ref;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->AlphaFunc)( func, ref );
   }
}


static void GLAPIENTRY save_BindTexture( GLenum target, GLuint texture )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_BIND_TEXTURE, 2 );
   if (n) {
      n[1].e = target;
      n[2].ui = texture;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->BindTexture)( target, texture );
   }
}


static void GLAPIENTRY save_Bitmap( GLsizei width, GLsizei height,
                         GLfloat xorig, GLfloat yorig,
                         GLfloat xmove, GLfloat ymove,
                         const GLubyte *pixels )
{
   GET_CURRENT_CONTEXT(ctx);
   GLvoid *image = _mesa_unpack_bitmap( width, height, pixels, &ctx->Unpack );
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_BITMAP, 7 );
   if (n) {
      n[1].i = (GLint) width;
      n[2].i = (GLint) height;
      n[3].f = xorig;
      n[4].f = yorig;
      n[5].f = xmove;
      n[6].f = ymove;
      n[7].data = image;
   }
   else if (image) {
      FREE(image);
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->Bitmap)( width, height,
                           xorig, yorig, xmove, ymove, pixels );
   }
}


static void GLAPIENTRY save_BlendEquation( GLenum mode )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_BLEND_EQUATION, 1 );
   if (n) {
      n[1].e = mode;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->BlendEquation)( mode );
   }
}


static void GLAPIENTRY save_BlendFunc( GLenum sfactor, GLenum dfactor )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_BLEND_FUNC, 2 );
   if (n) {
      n[1].e = sfactor;
      n[2].e = dfactor;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->BlendFunc)( sfactor, dfactor );
   }
}


static void GLAPIENTRY save_BlendFuncSeparateEXT(GLenum sfactorRGB, GLenum dfactorRGB,
                                      GLenum sfactorA, GLenum dfactorA)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_BLEND_FUNC_SEPARATE, 4 );
   if (n) {
      n[1].e = sfactorRGB;
      n[2].e = dfactorRGB;
      n[3].e = sfactorA;
      n[4].e = dfactorA;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->BlendFuncSeparateEXT)( sfactorRGB, dfactorRGB,
                                          sfactorA, dfactorA);
   }
}


static void GLAPIENTRY save_BlendColor( GLfloat red, GLfloat green,
                             GLfloat blue, GLfloat alpha )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_BLEND_COLOR, 4 );
   if (n) {
      n[1].f = red;
      n[2].f = green;
      n[3].f = blue;
      n[4].f = alpha;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->BlendColor)( red, green, blue, alpha );
   }
}


void GLAPIENTRY _mesa_save_CallList( GLuint list )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   SAVE_FLUSH_VERTICES(ctx);

   n = ALLOC_INSTRUCTION( ctx, OPCODE_CALL_LIST, 1 );
   if (n) {
      n[1].ui = list;
   }
   
   /* After this, we don't know what begin/end state we're in:
    */
   ctx->Driver.CurrentSavePrimitive = PRIM_UNKNOWN;

   if (ctx->ExecuteFlag) {
      (*ctx->Exec->CallList)( list );
   }
}


void GLAPIENTRY _mesa_save_CallLists( GLsizei n, GLenum type, const GLvoid *lists )
{
   GET_CURRENT_CONTEXT(ctx);
   GLint i;
   GLboolean typeErrorFlag;

   SAVE_FLUSH_VERTICES(ctx);

   switch (type) {
      case GL_BYTE:
      case GL_UNSIGNED_BYTE:
      case GL_SHORT:
      case GL_UNSIGNED_SHORT:
      case GL_INT:
      case GL_UNSIGNED_INT:
      case GL_FLOAT:
      case GL_2_BYTES:
      case GL_3_BYTES:
      case GL_4_BYTES:
         typeErrorFlag = GL_FALSE;
         break;
      default:
         typeErrorFlag = GL_TRUE;
   }

   for (i=0;i<n;i++) {
      GLuint list = translate_id( i, type, lists );
      Node *n = ALLOC_INSTRUCTION( ctx, OPCODE_CALL_LIST_OFFSET, 2 );
      if (n) {
         n[1].ui = list;
         n[2].b = typeErrorFlag;
      }
   }

   /* After this, we don't know what begin/end state we're in:
    */
   ctx->Driver.CurrentSavePrimitive = PRIM_UNKNOWN;

   if (ctx->ExecuteFlag) {
      (*ctx->Exec->CallLists)( n, type, lists );
   }
}


static void GLAPIENTRY save_Clear( GLbitfield mask )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_CLEAR, 1 );
   if (n) {
      n[1].bf = mask;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->Clear)( mask );
   }
}


static void GLAPIENTRY save_ClearAccum( GLfloat red, GLfloat green,
                             GLfloat blue, GLfloat alpha )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_CLEAR_ACCUM, 4 );
   if (n) {
      n[1].f = red;
      n[2].f = green;
      n[3].f = blue;
      n[4].f = alpha;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->ClearAccum)( red, green, blue, alpha );
   }
}


static void GLAPIENTRY save_ClearColor( GLclampf red, GLclampf green,
                             GLclampf blue, GLclampf alpha )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_CLEAR_COLOR, 4 );
   if (n) {
      n[1].f = red;
      n[2].f = green;
      n[3].f = blue;
      n[4].f = alpha;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->ClearColor)( red, green, blue, alpha );
   }
}


static void GLAPIENTRY save_ClearDepth( GLclampd depth )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_CLEAR_DEPTH, 1 );
   if (n) {
      n[1].f = (GLfloat) depth;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->ClearDepth)( depth );
   }
}


static void GLAPIENTRY save_ClearIndex( GLfloat c )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_CLEAR_INDEX, 1 );
   if (n) {
      n[1].f = c;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->ClearIndex)( c );
   }
}


static void GLAPIENTRY save_ClearStencil( GLint s )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_CLEAR_STENCIL, 1 );
   if (n) {
      n[1].i = s;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->ClearStencil)( s );
   }
}


static void GLAPIENTRY save_ClipPlane( GLenum plane, const GLdouble *equ )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_CLIP_PLANE, 5 );
   if (n) {
      n[1].e = plane;
      n[2].f = (GLfloat) equ[0];
      n[3].f = (GLfloat) equ[1];
      n[4].f = (GLfloat) equ[2];
      n[5].f = (GLfloat) equ[3];
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->ClipPlane)( plane, equ );
   }
}



static void GLAPIENTRY save_ColorMask( GLboolean red, GLboolean green,
                            GLboolean blue, GLboolean alpha )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_COLOR_MASK, 4 );
   if (n) {
      n[1].b = red;
      n[2].b = green;
      n[3].b = blue;
      n[4].b = alpha;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->ColorMask)( red, green, blue, alpha );
   }
}


static void GLAPIENTRY save_ColorMaterial( GLenum face, GLenum mode )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);

   n = ALLOC_INSTRUCTION( ctx, OPCODE_COLOR_MATERIAL, 2 );
   if (n) {
      n[1].e = face;
      n[2].e = mode;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->ColorMaterial)( face, mode );
   }
}


static void GLAPIENTRY save_ColorTable( GLenum target, GLenum internalFormat,
                             GLsizei width, GLenum format, GLenum type,
                             const GLvoid *table )
{
   GET_CURRENT_CONTEXT(ctx);
   if (target == GL_PROXY_TEXTURE_1D ||
       target == GL_PROXY_TEXTURE_2D ||
       target == GL_PROXY_TEXTURE_3D ||
       target == GL_PROXY_TEXTURE_CUBE_MAP_ARB) {
      /* execute immediately */
      (*ctx->Exec->ColorTable)( target, internalFormat, width,
                                format, type, table );
   }
   else {
      GLvoid *image = _mesa_unpack_image(width, 1, 1, format, type, table,
                                         &ctx->Unpack);
      Node *n;
      ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
      n = ALLOC_INSTRUCTION( ctx, OPCODE_COLOR_TABLE, 6 );
      if (n) {
         n[1].e = target;
         n[2].e = internalFormat;
         n[3].i = width;
         n[4].e = format;
         n[5].e = type;
         n[6].data = image;
      }
      else if (image) {
         FREE(image);
      }
      if (ctx->ExecuteFlag) {
         (*ctx->Exec->ColorTable)( target, internalFormat, width,
                                   format, type, table );
      }
   }
}



static void GLAPIENTRY
save_ColorTableParameterfv(GLenum target, GLenum pname, const GLfloat *params)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;

   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);

   n = ALLOC_INSTRUCTION( ctx, OPCODE_COLOR_TABLE_PARAMETER_FV, 6 );
   if (n) {
      n[1].e = target;
      n[2].e = pname;
      n[3].f = params[0];
      if (pname == GL_COLOR_TABLE_SGI ||
          pname == GL_POST_CONVOLUTION_COLOR_TABLE_SGI ||
          pname == GL_POST_COLOR_MATRIX_COLOR_TABLE_SGI ||
          pname == GL_TEXTURE_COLOR_TABLE_SGI) {
         n[4].f = params[1];
         n[5].f = params[2];
         n[6].f = params[3];
      }
   }

   if (ctx->ExecuteFlag) {
      (*ctx->Exec->ColorTableParameterfv)( target, pname, params );
   }
}


static void GLAPIENTRY
save_ColorTableParameteriv(GLenum target, GLenum pname, const GLint *params)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;

   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);

   n = ALLOC_INSTRUCTION( ctx, OPCODE_COLOR_TABLE_PARAMETER_IV, 6 );
   if (n) {
      n[1].e = target;
      n[2].e = pname;
      n[3].i = params[0];
      if (pname == GL_COLOR_TABLE_SGI ||
          pname == GL_POST_CONVOLUTION_COLOR_TABLE_SGI ||
          pname == GL_POST_COLOR_MATRIX_COLOR_TABLE_SGI ||
          pname == GL_TEXTURE_COLOR_TABLE_SGI) {
         n[4].i = params[1];
         n[5].i = params[2];
         n[6].i = params[3];
      }
   }

   if (ctx->ExecuteFlag) {
      (*ctx->Exec->ColorTableParameteriv)( target, pname, params );
   }
}



static void GLAPIENTRY save_ColorSubTable( GLenum target, GLsizei start, GLsizei count,
                                GLenum format, GLenum type,
                                const GLvoid *table)
{
   GET_CURRENT_CONTEXT(ctx);
   GLvoid *image = _mesa_unpack_image(count, 1, 1, format, type, table,
                                      &ctx->Unpack);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_COLOR_SUB_TABLE, 6 );
   if (n) {
      n[1].e = target;
      n[2].i = start;
      n[3].i = count;
      n[4].e = format;
      n[5].e = type;
      n[6].data = image;
   }
   else if (image) {
      FREE(image);
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->ColorSubTable)(target, start, count, format, type, table);
   }
}


static void GLAPIENTRY
save_CopyColorSubTable(GLenum target, GLsizei start,
                       GLint x, GLint y, GLsizei width)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;

   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_COPY_COLOR_SUB_TABLE, 5 );
   if (n) {
      n[1].e = target;
      n[2].i = start;
      n[3].i = x;
      n[4].i = y;
      n[5].i = width;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->CopyColorSubTable)(target, start, x, y, width);
   }
}


static void GLAPIENTRY
save_CopyColorTable(GLenum target, GLenum internalformat,
                    GLint x, GLint y, GLsizei width)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;

   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_COPY_COLOR_TABLE, 5 );
   if (n) {
      n[1].e = target;
      n[2].e = internalformat;
      n[3].i = x;
      n[4].i = y;
      n[5].i = width;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->CopyColorTable)(target, internalformat, x, y, width);
   }
}


static void GLAPIENTRY
save_ConvolutionFilter1D(GLenum target, GLenum internalFormat, GLsizei width,
                         GLenum format, GLenum type, const GLvoid *filter)
{
   GET_CURRENT_CONTEXT(ctx);
   GLvoid *image = _mesa_unpack_image(width, 1, 1, format, type, filter,
                                      &ctx->Unpack);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_CONVOLUTION_FILTER_1D, 6 );
   if (n) {
      n[1].e = target;
      n[2].e = internalFormat;
      n[3].i = width;
      n[4].e = format;
      n[5].e = type;
      n[6].data = image;
   }
   else if (image) {
      FREE(image);
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->ConvolutionFilter1D)( target, internalFormat, width,
                                         format, type, filter );
   }
}


static void GLAPIENTRY
save_ConvolutionFilter2D(GLenum target, GLenum internalFormat,
                         GLsizei width, GLsizei height, GLenum format,
                         GLenum type, const GLvoid *filter)
{
   GET_CURRENT_CONTEXT(ctx);
   GLvoid *image = _mesa_unpack_image(width, height, 1, format, type, filter,
                                      &ctx->Unpack);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_CONVOLUTION_FILTER_2D, 7 );
   if (n) {
      n[1].e = target;
      n[2].e = internalFormat;
      n[3].i = width;
      n[4].i = height;
      n[5].e = format;
      n[6].e = type;
      n[7].data = image;
   }
   else if (image) {
      FREE(image);
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->ConvolutionFilter2D)( target, internalFormat, width, height,
                                         format, type, filter );
   }
}


static void GLAPIENTRY
save_ConvolutionParameteri(GLenum target, GLenum pname, GLint param)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_CONVOLUTION_PARAMETER_I, 3 );
   if (n) {
      n[1].e = target;
      n[2].e = pname;
      n[3].i = param;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->ConvolutionParameteri)( target, pname, param );
   }
}


static void GLAPIENTRY
save_ConvolutionParameteriv(GLenum target, GLenum pname, const GLint *params)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_CONVOLUTION_PARAMETER_IV, 6 );
   if (n) {
      n[1].e = target;
      n[2].e = pname;
      n[3].i = params[0];
      if (pname == GL_CONVOLUTION_BORDER_COLOR ||
          pname == GL_CONVOLUTION_FILTER_SCALE ||
          pname == GL_CONVOLUTION_FILTER_BIAS) {
         n[4].i = params[1];
         n[5].i = params[2];
         n[6].i = params[3];
      }
      else {
         n[4].i = n[5].i = n[6].i = 0;
      }
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->ConvolutionParameteriv)( target, pname, params );
   }
}


static void GLAPIENTRY
save_ConvolutionParameterf(GLenum target, GLenum pname, GLfloat param)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_CONVOLUTION_PARAMETER_F, 3 );
   if (n) {
      n[1].e = target;
      n[2].e = pname;
      n[3].f = param;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->ConvolutionParameterf)( target, pname, param );
   }
}


static void GLAPIENTRY
save_ConvolutionParameterfv(GLenum target, GLenum pname, const GLfloat *params)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_CONVOLUTION_PARAMETER_FV, 6 );
   if (n) {
      n[1].e = target;
      n[2].e = pname;
      n[3].f = params[0];
      if (pname == GL_CONVOLUTION_BORDER_COLOR ||
          pname == GL_CONVOLUTION_FILTER_SCALE ||
          pname == GL_CONVOLUTION_FILTER_BIAS) {
         n[4].f = params[1];
         n[5].f = params[2];
         n[6].f = params[3];
      }
      else {
         n[4].f = n[5].f = n[6].f = 0.0F;
      }
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->ConvolutionParameterfv)( target, pname, params );
   }
}


static void GLAPIENTRY
save_CopyPixels( GLint x, GLint y,
		 GLsizei width, GLsizei height, GLenum type )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_COPY_PIXELS, 5 );
   if (n) {
      n[1].i = x;
      n[2].i = y;
      n[3].i = (GLint) width;
      n[4].i = (GLint) height;
      n[5].e = type;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->CopyPixels)( x, y, width, height, type );
   }
}



static void GLAPIENTRY
save_CopyTexImage1D( GLenum target, GLint level, GLenum internalformat,
                     GLint x, GLint y, GLsizei width, GLint border )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_COPY_TEX_IMAGE1D, 7 );
   if (n) {
      n[1].e = target;
      n[2].i = level;
      n[3].e = internalformat;
      n[4].i = x;
      n[5].i = y;
      n[6].i = width;
      n[7].i = border;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->CopyTexImage1D)( target, level, internalformat,
                                   x, y, width, border );
   }
}


static void GLAPIENTRY
save_CopyTexImage2D( GLenum target, GLint level,
                     GLenum internalformat,
                     GLint x, GLint y, GLsizei width,
                     GLsizei height, GLint border )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_COPY_TEX_IMAGE2D, 8 );
   if (n) {
      n[1].e = target;
      n[2].i = level;
      n[3].e = internalformat;
      n[4].i = x;
      n[5].i = y;
      n[6].i = width;
      n[7].i = height;
      n[8].i = border;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->CopyTexImage2D)( target, level, internalformat,
                                   x, y, width, height, border );
   }
}



static void GLAPIENTRY
save_CopyTexSubImage1D( GLenum target, GLint level,
                        GLint xoffset, GLint x, GLint y,
                        GLsizei width )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_COPY_TEX_SUB_IMAGE1D, 6 );
   if (n) {
      n[1].e = target;
      n[2].i = level;
      n[3].i = xoffset;
      n[4].i = x;
      n[5].i = y;
      n[6].i = width;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->CopyTexSubImage1D)( target, level, xoffset, x, y, width );
   }
}


static void GLAPIENTRY
save_CopyTexSubImage2D( GLenum target, GLint level,
                        GLint xoffset, GLint yoffset,
                        GLint x, GLint y,
                        GLsizei width, GLint height )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_COPY_TEX_SUB_IMAGE2D, 8 );
   if (n) {
      n[1].e = target;
      n[2].i = level;
      n[3].i = xoffset;
      n[4].i = yoffset;
      n[5].i = x;
      n[6].i = y;
      n[7].i = width;
      n[8].i = height;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->CopyTexSubImage2D)( target, level, xoffset, yoffset,
                               x, y, width, height );
   }
}


static void GLAPIENTRY
save_CopyTexSubImage3D( GLenum target, GLint level,
                        GLint xoffset, GLint yoffset, GLint zoffset,
                        GLint x, GLint y,
                        GLsizei width, GLint height )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_COPY_TEX_SUB_IMAGE3D, 9 );
   if (n) {
      n[1].e = target;
      n[2].i = level;
      n[3].i = xoffset;
      n[4].i = yoffset;
      n[5].i = zoffset;
      n[6].i = x;
      n[7].i = y;
      n[8].i = width;
      n[9].i = height;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->CopyTexSubImage3D)( target, level,
                                      xoffset, yoffset, zoffset,
                                      x, y, width, height );
   }
}


static void GLAPIENTRY save_CullFace( GLenum mode )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_CULL_FACE, 1 );
   if (n) {
      n[1].e = mode;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->CullFace)( mode );
   }
}


static void GLAPIENTRY save_DepthFunc( GLenum func )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_DEPTH_FUNC, 1 );
   if (n) {
      n[1].e = func;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->DepthFunc)( func );
   }
}


static void GLAPIENTRY save_DepthMask( GLboolean mask )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_DEPTH_MASK, 1 );
   if (n) {
      n[1].b = mask;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->DepthMask)( mask );
   }
}


static void GLAPIENTRY save_DepthRange( GLclampd nearval, GLclampd farval )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_DEPTH_RANGE, 2 );
   if (n) {
      n[1].f = (GLfloat) nearval;
      n[2].f = (GLfloat) farval;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->DepthRange)( nearval, farval );
   }
}


static void GLAPIENTRY save_Disable( GLenum cap )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_DISABLE, 1 );
   if (n) {
      n[1].e = cap;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->Disable)( cap );
   }
}


static void GLAPIENTRY save_DrawBuffer( GLenum mode )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_DRAW_BUFFER, 1 );
   if (n) {
      n[1].e = mode;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->DrawBuffer)( mode );
   }
}


static void GLAPIENTRY save_DrawPixels( GLsizei width, GLsizei height,
                             GLenum format, GLenum type,
                             const GLvoid *pixels )
{
   GET_CURRENT_CONTEXT(ctx);
   GLvoid *image = _mesa_unpack_image(width, height, 1, format, type,
                                      pixels, &ctx->Unpack);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_DRAW_PIXELS, 5 );
   if (n) {
      n[1].i = width;
      n[2].i = height;
      n[3].e = format;
      n[4].e = type;
      n[5].data = image;
   }
   else if (image) {
      FREE(image);
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->DrawPixels)( width, height, format, type, pixels );
   }
}



static void GLAPIENTRY save_Enable( GLenum cap )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_ENABLE, 1 );
   if (n) {
      n[1].e = cap;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->Enable)( cap );
   }
}



void GLAPIENTRY _mesa_save_EvalMesh1( GLenum mode, GLint i1, GLint i2 )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_EVALMESH1, 3 );
   if (n) {
      n[1].e = mode;
      n[2].i = i1;
      n[3].i = i2;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->EvalMesh1)( mode, i1, i2 );
   }
}


void GLAPIENTRY _mesa_save_EvalMesh2(GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2 )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_EVALMESH2, 5 );
   if (n) {
      n[1].e = mode;
      n[2].i = i1;
      n[3].i = i2;
      n[4].i = j1;
      n[5].i = j2;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->EvalMesh2)( mode, i1, i2, j1, j2 );
   }
}




static void GLAPIENTRY save_Fogfv( GLenum pname, const GLfloat *params )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_FOG, 5 );
   if (n) {
      n[1].e = pname;
      n[2].f = params[0];
      n[3].f = params[1];
      n[4].f = params[2];
      n[5].f = params[3];
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->Fogfv)( pname, params );
   }
}


static void GLAPIENTRY save_Fogf( GLenum pname, GLfloat param )
{
   save_Fogfv(pname, &param);
}


static void GLAPIENTRY save_Fogiv(GLenum pname, const GLint *params )
{
   GLfloat p[4];
   switch (pname) {
      case GL_FOG_MODE:
      case GL_FOG_DENSITY:
      case GL_FOG_START:
      case GL_FOG_END:
      case GL_FOG_INDEX:
	 p[0] = (GLfloat) *params;
	 break;
      case GL_FOG_COLOR:
	 p[0] = INT_TO_FLOAT( params[0] );
	 p[1] = INT_TO_FLOAT( params[1] );
	 p[2] = INT_TO_FLOAT( params[2] );
	 p[3] = INT_TO_FLOAT( params[3] );
	 break;
      default:
         /* Error will be caught later in gl_Fogfv */
         ;
   }
   save_Fogfv(pname, p);
}


static void GLAPIENTRY save_Fogi(GLenum pname, GLint param )
{
   save_Fogiv(pname, &param);
}


static void GLAPIENTRY save_FrontFace( GLenum mode )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_FRONT_FACE, 1 );
   if (n) {
      n[1].e = mode;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->FrontFace)( mode );
   }
}


static void GLAPIENTRY save_Frustum( GLdouble left, GLdouble right,
                      GLdouble bottom, GLdouble top,
                      GLdouble nearval, GLdouble farval )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_FRUSTUM, 6 );
   if (n) {
      n[1].f = (GLfloat) left;
      n[2].f = (GLfloat) right;
      n[3].f = (GLfloat) bottom;
      n[4].f = (GLfloat) top;
      n[5].f = (GLfloat) nearval;
      n[6].f = (GLfloat) farval;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->Frustum)( left, right, bottom, top, nearval, farval );
   }
}


static void GLAPIENTRY save_Hint( GLenum target, GLenum mode )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_HINT, 2 );
   if (n) {
      n[1].e = target;
      n[2].e = mode;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->Hint)( target, mode );
   }
}


static void GLAPIENTRY
save_Histogram(GLenum target, GLsizei width, GLenum internalFormat, GLboolean sink)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;

   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_HISTOGRAM, 4 );
   if (n) {
      n[1].e = target;
      n[2].i = width;
      n[3].e = internalFormat;
      n[4].b = sink;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->Histogram)( target, width, internalFormat, sink );
   }
}


static void GLAPIENTRY save_IndexMask( GLuint mask )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_INDEX_MASK, 1 );
   if (n) {
      n[1].ui = mask;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->IndexMask)( mask );
   }
}


static void GLAPIENTRY save_InitNames( void )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   (void) ALLOC_INSTRUCTION( ctx, OPCODE_INIT_NAMES, 0 );
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->InitNames)();
   }
}


static void GLAPIENTRY save_Lightfv( GLenum light, GLenum pname, const GLfloat *params )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_LIGHT, 6 );
   if (OPCODE_LIGHT) {
      GLint i, nParams;
      n[1].e = light;
      n[2].e = pname;
      switch (pname) {
         case GL_AMBIENT:
            nParams = 4;
            break;
         case GL_DIFFUSE:
            nParams = 4;
            break;
         case GL_SPECULAR:
            nParams = 4;
            break;
         case GL_POSITION:
            nParams = 4;
            break;
         case GL_SPOT_DIRECTION:
            nParams = 3;
            break;
         case GL_SPOT_EXPONENT:
            nParams = 1;
            break;
         case GL_SPOT_CUTOFF:
            nParams = 1;
            break;
         case GL_CONSTANT_ATTENUATION:
            nParams = 1;
            break;
         case GL_LINEAR_ATTENUATION:
            nParams = 1;
            break;
         case GL_QUADRATIC_ATTENUATION:
            nParams = 1;
            break;
         default:
            nParams = 0;
      }
      for (i = 0; i < nParams; i++) {
	 n[3+i].f = params[i];
      }
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->Lightfv)( light, pname, params );
   }
}


static void GLAPIENTRY save_Lightf( GLenum light, GLenum pname, GLfloat params )
{
   save_Lightfv(light, pname, &params);
}


static void GLAPIENTRY save_Lightiv( GLenum light, GLenum pname, const GLint *params )
{
   GLfloat fparam[4];
   switch (pname) {
      case GL_AMBIENT:
      case GL_DIFFUSE:
      case GL_SPECULAR:
         fparam[0] = INT_TO_FLOAT( params[0] );
         fparam[1] = INT_TO_FLOAT( params[1] );
         fparam[2] = INT_TO_FLOAT( params[2] );
         fparam[3] = INT_TO_FLOAT( params[3] );
         break;
      case GL_POSITION:
         fparam[0] = (GLfloat) params[0];
         fparam[1] = (GLfloat) params[1];
         fparam[2] = (GLfloat) params[2];
         fparam[3] = (GLfloat) params[3];
         break;
      case GL_SPOT_DIRECTION:
         fparam[0] = (GLfloat) params[0];
         fparam[1] = (GLfloat) params[1];
         fparam[2] = (GLfloat) params[2];
         break;
      case GL_SPOT_EXPONENT:
      case GL_SPOT_CUTOFF:
      case GL_CONSTANT_ATTENUATION:
      case GL_LINEAR_ATTENUATION:
      case GL_QUADRATIC_ATTENUATION:
         fparam[0] = (GLfloat) params[0];
         break;
      default:
         /* error will be caught later in gl_Lightfv */
         ;
   }
   save_Lightfv( light, pname, fparam );
}


static void GLAPIENTRY save_Lighti( GLenum light, GLenum pname, GLint param )
{
   save_Lightiv( light, pname, &param );
}


static void GLAPIENTRY save_LightModelfv( GLenum pname, const GLfloat *params )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_LIGHT_MODEL, 5 );
   if (n) {
      n[1].e = pname;
      n[2].f = params[0];
      n[3].f = params[1];
      n[4].f = params[2];
      n[5].f = params[3];
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->LightModelfv)( pname, params );
   }
}


static void GLAPIENTRY save_LightModelf( GLenum pname, GLfloat param )
{
   save_LightModelfv(pname, &param);
}


static void GLAPIENTRY save_LightModeliv( GLenum pname, const GLint *params )
{
   GLfloat fparam[4];
   switch (pname) {
      case GL_LIGHT_MODEL_AMBIENT:
         fparam[0] = INT_TO_FLOAT( params[0] );
         fparam[1] = INT_TO_FLOAT( params[1] );
         fparam[2] = INT_TO_FLOAT( params[2] );
         fparam[3] = INT_TO_FLOAT( params[3] );
         break;
      case GL_LIGHT_MODEL_LOCAL_VIEWER:
      case GL_LIGHT_MODEL_TWO_SIDE:
      case GL_LIGHT_MODEL_COLOR_CONTROL:
         fparam[0] = (GLfloat) params[0];
         break;
      default:
         /* Error will be caught later in gl_LightModelfv */
         ;
   }
   save_LightModelfv(pname, fparam);
}


static void GLAPIENTRY save_LightModeli( GLenum pname, GLint param )
{
   save_LightModeliv(pname, &param);
}


static void GLAPIENTRY save_LineStipple( GLint factor, GLushort pattern )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_LINE_STIPPLE, 2 );
   if (n) {
      n[1].i = factor;
      n[2].us = pattern;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->LineStipple)( factor, pattern );
   }
}


static void GLAPIENTRY save_LineWidth( GLfloat width )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_LINE_WIDTH, 1 );
   if (n) {
      n[1].f = width;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->LineWidth)( width );
   }
}


static void GLAPIENTRY save_ListBase( GLuint base )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_LIST_BASE, 1 );
   if (n) {
      n[1].ui = base;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->ListBase)( base );
   }
}


static void GLAPIENTRY save_LoadIdentity( void )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   (void) ALLOC_INSTRUCTION( ctx, OPCODE_LOAD_IDENTITY, 0 );
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->LoadIdentity)();
   }
}


static void GLAPIENTRY save_LoadMatrixf( const GLfloat *m )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_LOAD_MATRIX, 16 );
   if (n) {
      GLuint i;
      for (i=0;i<16;i++) {
	 n[1+i].f = m[i];
      }
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->LoadMatrixf)( m );
   }
}


static void GLAPIENTRY save_LoadMatrixd( const GLdouble *m )
{
   GLfloat f[16];
   GLint i;
   for (i = 0; i < 16; i++) {
      f[i] = (GLfloat) m[i];
   }
   save_LoadMatrixf(f);
}


static void GLAPIENTRY save_LoadName( GLuint name )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_LOAD_NAME, 1 );
   if (n) {
      n[1].ui = name;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->LoadName)( name );
   }
}


static void GLAPIENTRY save_LogicOp( GLenum opcode )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_LOGIC_OP, 1 );
   if (n) {
      n[1].e = opcode;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->LogicOp)( opcode );
   }
}


static void GLAPIENTRY save_Map1d( GLenum target, GLdouble u1, GLdouble u2, GLint stride,
                        GLint order, const GLdouble *points)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_MAP1, 6 );
   if (n) {
      GLfloat *pnts = _mesa_copy_map_points1d( target, stride, order, points );
      n[1].e = target;
      n[2].f = (GLfloat) u1;
      n[3].f = (GLfloat) u2;
      n[4].i = _mesa_evaluator_components(target);  /* stride */
      n[5].i = order;
      n[6].data = (void *) pnts;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->Map1d)( target, u1, u2, stride, order, points );
   }
}

static void GLAPIENTRY save_Map1f( GLenum target, GLfloat u1, GLfloat u2, GLint stride,
                        GLint order, const GLfloat *points)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_MAP1, 6 );
   if (n) {
      GLfloat *pnts = _mesa_copy_map_points1f( target, stride, order, points );
      n[1].e = target;
      n[2].f = u1;
      n[3].f = u2;
      n[4].i = _mesa_evaluator_components(target);  /* stride */
      n[5].i = order;
      n[6].data = (void *) pnts;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->Map1f)( target, u1, u2, stride, order, points );
   }
}


static void GLAPIENTRY save_Map2d( GLenum target,
                        GLdouble u1, GLdouble u2, GLint ustride, GLint uorder,
                        GLdouble v1, GLdouble v2, GLint vstride, GLint vorder,
                        const GLdouble *points )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_MAP2, 10 );
   if (n) {
      GLfloat *pnts = _mesa_copy_map_points2d( target, ustride, uorder,
                                            vstride, vorder, points );
      n[1].e = target;
      n[2].f = (GLfloat) u1;
      n[3].f = (GLfloat) u2;
      n[4].f = (GLfloat) v1;
      n[5].f = (GLfloat) v2;
      /* XXX verify these strides are correct */
      n[6].i = _mesa_evaluator_components(target) * vorder;  /*ustride*/
      n[7].i = _mesa_evaluator_components(target);           /*vstride*/
      n[8].i = uorder;
      n[9].i = vorder;
      n[10].data = (void *) pnts;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->Map2d)( target,
                          u1, u2, ustride, uorder,
                          v1, v2, vstride, vorder, points );
   }
}


static void GLAPIENTRY save_Map2f( GLenum target,
                        GLfloat u1, GLfloat u2, GLint ustride, GLint uorder,
                        GLfloat v1, GLfloat v2, GLint vstride, GLint vorder,
                        const GLfloat *points )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_MAP2, 10 );
   if (n) {
      GLfloat *pnts = _mesa_copy_map_points2f( target, ustride, uorder,
                                            vstride, vorder, points );
      n[1].e = target;
      n[2].f = u1;
      n[3].f = u2;
      n[4].f = v1;
      n[5].f = v2;
      /* XXX verify these strides are correct */
      n[6].i = _mesa_evaluator_components(target) * vorder;  /*ustride*/
      n[7].i = _mesa_evaluator_components(target);           /*vstride*/
      n[8].i = uorder;
      n[9].i = vorder;
      n[10].data = (void *) pnts;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->Map2f)( target, u1, u2, ustride, uorder,
                          v1, v2, vstride, vorder, points );
   }
}


static void GLAPIENTRY save_MapGrid1f( GLint un, GLfloat u1, GLfloat u2 )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_MAPGRID1, 3 );
   if (n) {
      n[1].i = un;
      n[2].f = u1;
      n[3].f = u2;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->MapGrid1f)( un, u1, u2 );
   }
}


static void GLAPIENTRY save_MapGrid1d( GLint un, GLdouble u1, GLdouble u2 )
{
   save_MapGrid1f(un, (GLfloat) u1, (GLfloat) u2);
}


static void GLAPIENTRY save_MapGrid2f( GLint un, GLfloat u1, GLfloat u2,
                            GLint vn, GLfloat v1, GLfloat v2 )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_MAPGRID2, 6 );
   if (n) {
      n[1].i = un;
      n[2].f = u1;
      n[3].f = u2;
      n[4].i = vn;
      n[5].f = v1;
      n[6].f = v2;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->MapGrid2f)( un, u1, u2, vn, v1, v2 );
   }
}



static void GLAPIENTRY save_MapGrid2d( GLint un, GLdouble u1, GLdouble u2,
                            GLint vn, GLdouble v1, GLdouble v2 )
{
   save_MapGrid2f(un, (GLfloat) u1, (GLfloat) u2,
		  vn, (GLfloat) v1, (GLfloat) v2);
}


static void GLAPIENTRY save_MatrixMode( GLenum mode )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_MATRIX_MODE, 1 );
   if (n) {
      n[1].e = mode;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->MatrixMode)( mode );
   }
}


static void GLAPIENTRY
save_Minmax(GLenum target, GLenum internalFormat, GLboolean sink)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;

   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_MIN_MAX, 3 );
   if (n) {
      n[1].e = target;
      n[2].e = internalFormat;
      n[3].b = sink;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->Minmax)( target, internalFormat, sink );
   }
}


static void GLAPIENTRY save_MultMatrixf( const GLfloat *m )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_MULT_MATRIX, 16 );
   if (n) {
      GLuint i;
      for (i=0;i<16;i++) {
	 n[1+i].f = m[i];
      }
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->MultMatrixf)( m );
   }
}


static void GLAPIENTRY save_MultMatrixd( const GLdouble *m )
{
   GLfloat f[16];
   GLint i;
   for (i = 0; i < 16; i++) {
      f[i] = (GLfloat) m[i];
   }
   save_MultMatrixf(f);
}


static void GLAPIENTRY save_NewList( GLuint list, GLenum mode )
{
   GET_CURRENT_CONTEXT(ctx);
   /* It's an error to call this function while building a display list */
   _mesa_error( ctx, GL_INVALID_OPERATION, "glNewList" );
   (void) list;
   (void) mode;
}



static void GLAPIENTRY save_Ortho( GLdouble left, GLdouble right,
                    GLdouble bottom, GLdouble top,
                    GLdouble nearval, GLdouble farval )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_ORTHO, 6 );
   if (n) {
      n[1].f = (GLfloat) left;
      n[2].f = (GLfloat) right;
      n[3].f = (GLfloat) bottom;
      n[4].f = (GLfloat) top;
      n[5].f = (GLfloat) nearval;
      n[6].f = (GLfloat) farval;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->Ortho)( left, right, bottom, top, nearval, farval );
   }
}


static void GLAPIENTRY
save_PixelMapfv( GLenum map, GLint mapsize, const GLfloat *values )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_PIXEL_MAP, 3 );
   if (n) {
      n[1].e = map;
      n[2].i = mapsize;
      n[3].data  = (void *) MALLOC( mapsize * sizeof(GLfloat) );
      MEMCPY( n[3].data, (void *) values, mapsize * sizeof(GLfloat) );
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->PixelMapfv)( map, mapsize, values );
   }
}


static void GLAPIENTRY
save_PixelMapuiv(GLenum map, GLint mapsize, const GLuint *values )
{
   GLfloat fvalues[MAX_PIXEL_MAP_TABLE];
   GLint i;
   if (map==GL_PIXEL_MAP_I_TO_I || map==GL_PIXEL_MAP_S_TO_S) {
      for (i=0;i<mapsize;i++) {
         fvalues[i] = (GLfloat) values[i];
      }
   }
   else {
      for (i=0;i<mapsize;i++) {
         fvalues[i] = UINT_TO_FLOAT( values[i] );
      }
   }
   save_PixelMapfv(map, mapsize, fvalues);
}


static void GLAPIENTRY
save_PixelMapusv(GLenum map, GLint mapsize, const GLushort *values)
{
   GLfloat fvalues[MAX_PIXEL_MAP_TABLE];
   GLint i;
   if (map==GL_PIXEL_MAP_I_TO_I || map==GL_PIXEL_MAP_S_TO_S) {
      for (i=0;i<mapsize;i++) {
         fvalues[i] = (GLfloat) values[i];
      }
   }
   else {
      for (i=0;i<mapsize;i++) {
         fvalues[i] = USHORT_TO_FLOAT( values[i] );
      }
   }
   save_PixelMapfv(map, mapsize, fvalues);
}


static void GLAPIENTRY
save_PixelTransferf( GLenum pname, GLfloat param )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_PIXEL_TRANSFER, 2 );
   if (n) {
      n[1].e = pname;
      n[2].f = param;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->PixelTransferf)( pname, param );
   }
}


static void GLAPIENTRY
save_PixelTransferi( GLenum pname, GLint param )
{
   save_PixelTransferf( pname, (GLfloat) param );
}


static void GLAPIENTRY
save_PixelZoom( GLfloat xfactor, GLfloat yfactor )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_PIXEL_ZOOM, 2 );
   if (n) {
      n[1].f = xfactor;
      n[2].f = yfactor;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->PixelZoom)( xfactor, yfactor );
   }
}


static void GLAPIENTRY
save_PointParameterfvEXT( GLenum pname, const GLfloat *params )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_POINT_PARAMETERS, 4 );
   if (n) {
      n[1].e = pname;
      n[2].f = params[0];
      n[3].f = params[1];
      n[4].f = params[2];
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->PointParameterfvEXT)( pname, params );
   }
}


static void GLAPIENTRY save_PointParameterfEXT( GLenum pname, GLfloat param )
{
   save_PointParameterfvEXT(pname, &param);
}

static void GLAPIENTRY save_PointParameteriNV( GLenum pname, GLint param )
{
   GLfloat p = (GLfloat) param;
   save_PointParameterfvEXT(pname, &p);
}

static void GLAPIENTRY save_PointParameterivNV( GLenum pname, const GLint *param )
{
   GLfloat p = (GLfloat) param[0];
   save_PointParameterfvEXT(pname, &p);
}


static void GLAPIENTRY save_PointSize( GLfloat size )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_POINT_SIZE, 1 );
   if (n) {
      n[1].f = size;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->PointSize)( size );
   }
}


static void GLAPIENTRY save_PolygonMode( GLenum face, GLenum mode )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_POLYGON_MODE, 2 );
   if (n) {
      n[1].e = face;
      n[2].e = mode;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->PolygonMode)( face, mode );
   }
}


/*
 * Polygon stipple must have been upacked already!
 */
static void GLAPIENTRY save_PolygonStipple( const GLubyte *pattern )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_POLYGON_STIPPLE, 1 );
   if (n) {
      void *data;
      n[1].data = MALLOC( 32 * 4 );
      data = n[1].data;   /* This needed for Acorn compiler */
      MEMCPY( data, pattern, 32 * 4 );
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->PolygonStipple)( (GLubyte*) pattern );
   }
}


static void GLAPIENTRY save_PolygonOffset( GLfloat factor, GLfloat units )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_POLYGON_OFFSET, 2 );
   if (n) {
      n[1].f = factor;
      n[2].f = units;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->PolygonOffset)( factor, units );
   }
}


static void GLAPIENTRY save_PolygonOffsetEXT( GLfloat factor, GLfloat bias )
{
   GET_CURRENT_CONTEXT(ctx);
   save_PolygonOffset(factor, ctx->DepthMaxF * bias);
}


static void GLAPIENTRY save_PopAttrib( void )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   (void) ALLOC_INSTRUCTION( ctx, OPCODE_POP_ATTRIB, 0 );
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->PopAttrib)();
   }
}


static void GLAPIENTRY save_PopMatrix( void )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   (void) ALLOC_INSTRUCTION( ctx, OPCODE_POP_MATRIX, 0 );
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->PopMatrix)();
   }
}


static void GLAPIENTRY save_PopName( void )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   (void) ALLOC_INSTRUCTION( ctx, OPCODE_POP_NAME, 0 );
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->PopName)();
   }
}


static void GLAPIENTRY save_PrioritizeTextures( GLsizei num, const GLuint *textures,
                                     const GLclampf *priorities )
{
   GET_CURRENT_CONTEXT(ctx);
   GLint i;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);

   for (i=0;i<num;i++) {
      Node *n;
      n = ALLOC_INSTRUCTION( ctx,  OPCODE_PRIORITIZE_TEXTURE, 2 );
      if (n) {
         n[1].ui = textures[i];
         n[2].f = priorities[i];
      }
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->PrioritizeTextures)( num, textures, priorities );
   }
}


static void GLAPIENTRY save_PushAttrib( GLbitfield mask )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_PUSH_ATTRIB, 1 );
   if (n) {
      n[1].bf = mask;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->PushAttrib)( mask );
   }
}


static void GLAPIENTRY save_PushMatrix( void )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   (void) ALLOC_INSTRUCTION( ctx, OPCODE_PUSH_MATRIX, 0 );
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->PushMatrix)();
   }
}


static void GLAPIENTRY save_PushName( GLuint name )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_PUSH_NAME, 1 );
   if (n) {
      n[1].ui = name;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->PushName)( name );
   }
}


static void GLAPIENTRY save_RasterPos4f( GLfloat x, GLfloat y, GLfloat z, GLfloat w )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_RASTER_POS, 4 );
   if (n) {
      n[1].f = x;
      n[2].f = y;
      n[3].f = z;
      n[4].f = w;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->RasterPos4f)( x, y, z, w );
   }
}

static void GLAPIENTRY save_RasterPos2d(GLdouble x, GLdouble y)
{
   save_RasterPos4f((GLfloat) x, (GLfloat) y, 0.0F, 1.0F);
}

static void GLAPIENTRY save_RasterPos2f(GLfloat x, GLfloat y)
{
   save_RasterPos4f(x, y, 0.0F, 1.0F);
}

static void GLAPIENTRY save_RasterPos2i(GLint x, GLint y)
{
   save_RasterPos4f((GLfloat) x, (GLfloat) y, 0.0F, 1.0F);
}

static void GLAPIENTRY save_RasterPos2s(GLshort x, GLshort y)
{
   save_RasterPos4f(x, y, 0.0F, 1.0F);
}

static void GLAPIENTRY save_RasterPos3d(GLdouble x, GLdouble y, GLdouble z)
{
   save_RasterPos4f((GLfloat) x, (GLfloat) y, (GLfloat) z, 1.0F);
}

static void GLAPIENTRY save_RasterPos3f(GLfloat x, GLfloat y, GLfloat z)
{
   save_RasterPos4f(x, y, z, 1.0F);
}

static void GLAPIENTRY save_RasterPos3i(GLint x, GLint y, GLint z)
{
   save_RasterPos4f((GLfloat) x, (GLfloat) y, (GLfloat) z, 1.0F);
}

static void GLAPIENTRY save_RasterPos3s(GLshort x, GLshort y, GLshort z)
{
   save_RasterPos4f(x, y, z, 1.0F);
}

static void GLAPIENTRY save_RasterPos4d(GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
   save_RasterPos4f((GLfloat) x, (GLfloat) y, (GLfloat) z, (GLfloat) w);
}

static void GLAPIENTRY save_RasterPos4i(GLint x, GLint y, GLint z, GLint w)
{
   save_RasterPos4f((GLfloat) x, (GLfloat) y, (GLfloat) z, (GLfloat) w);
}

static void GLAPIENTRY save_RasterPos4s(GLshort x, GLshort y, GLshort z, GLshort w)
{
   save_RasterPos4f(x, y, z, w);
}

static void GLAPIENTRY save_RasterPos2dv(const GLdouble *v)
{
   save_RasterPos4f((GLfloat) v[0], (GLfloat) v[1], 0.0F, 1.0F);
}

static void GLAPIENTRY save_RasterPos2fv(const GLfloat *v)
{
   save_RasterPos4f(v[0], v[1], 0.0F, 1.0F);
}

static void GLAPIENTRY save_RasterPos2iv(const GLint *v)
{
   save_RasterPos4f((GLfloat) v[0], (GLfloat) v[1], 0.0F, 1.0F);
}

static void GLAPIENTRY save_RasterPos2sv(const GLshort *v)
{
   save_RasterPos4f(v[0], v[1], 0.0F, 1.0F);
}

static void GLAPIENTRY save_RasterPos3dv(const GLdouble *v)
{
   save_RasterPos4f((GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2], 1.0F);
}

static void GLAPIENTRY save_RasterPos3fv(const GLfloat *v)
{
   save_RasterPos4f(v[0], v[1], v[2], 1.0F);
}

static void GLAPIENTRY save_RasterPos3iv(const GLint *v)
{
   save_RasterPos4f((GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2], 1.0F);
}

static void GLAPIENTRY save_RasterPos3sv(const GLshort *v)
{
   save_RasterPos4f(v[0], v[1], v[2], 1.0F);
}

static void GLAPIENTRY save_RasterPos4dv(const GLdouble *v)
{
   save_RasterPos4f((GLfloat) v[0], (GLfloat) v[1],
		    (GLfloat) v[2], (GLfloat) v[3]);
}

static void GLAPIENTRY save_RasterPos4fv(const GLfloat *v)
{
   save_RasterPos4f(v[0], v[1], v[2], v[3]);
}

static void GLAPIENTRY save_RasterPos4iv(const GLint *v)
{
   save_RasterPos4f((GLfloat) v[0], (GLfloat) v[1],
		    (GLfloat) v[2], (GLfloat) v[3]);
}

static void GLAPIENTRY save_RasterPos4sv(const GLshort *v)
{
   save_RasterPos4f(v[0], v[1], v[2], v[3]);
}


static void GLAPIENTRY save_PassThrough( GLfloat token )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_PASSTHROUGH, 1 );
   if (n) {
      n[1].f = token;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->PassThrough)( token );
   }
}


static void GLAPIENTRY save_ReadBuffer( GLenum mode )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_READ_BUFFER, 1 );
   if (n) {
      n[1].e = mode;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->ReadBuffer)( mode );
   }
}


static void GLAPIENTRY
save_ResetHistogram(GLenum target)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_RESET_HISTOGRAM, 1 );
   if (n) {
      n[1].e = target;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->ResetHistogram)( target );
   }
}


static void GLAPIENTRY
save_ResetMinmax(GLenum target)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_RESET_MIN_MAX, 1 );
   if (n) {
      n[1].e = target;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->ResetMinmax)( target );
   }
}


static void GLAPIENTRY save_Rotatef( GLfloat angle, GLfloat x, GLfloat y, GLfloat z )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_ROTATE, 4 );
   if (n) {
      n[1].f = angle;
      n[2].f = x;
      n[3].f = y;
      n[4].f = z;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->Rotatef)( angle, x, y, z );
   }
}


static void GLAPIENTRY save_Rotated( GLdouble angle, GLdouble x, GLdouble y, GLdouble z )
{
   save_Rotatef((GLfloat) angle, (GLfloat) x, (GLfloat) y, (GLfloat) z);
}


static void GLAPIENTRY save_Scalef( GLfloat x, GLfloat y, GLfloat z )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_SCALE, 3 );
   if (n) {
      n[1].f = x;
      n[2].f = y;
      n[3].f = z;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->Scalef)( x, y, z );
   }
}


static void GLAPIENTRY save_Scaled( GLdouble x, GLdouble y, GLdouble z )
{
   save_Scalef((GLfloat) x, (GLfloat) y, (GLfloat) z);
}


static void GLAPIENTRY save_Scissor( GLint x, GLint y, GLsizei width, GLsizei height )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_SCISSOR, 4 );
   if (n) {
      n[1].i = x;
      n[2].i = y;
      n[3].i = width;
      n[4].i = height;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->Scissor)( x, y, width, height );
   }
}


static void GLAPIENTRY save_ShadeModel( GLenum mode )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_SHADE_MODEL, 1 );
   if (n) {
      n[1].e = mode;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->ShadeModel)( mode );
   }
}


static void GLAPIENTRY save_StencilFunc( GLenum func, GLint ref, GLuint mask )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_STENCIL_FUNC, 3 );
   if (n) {
      n[1].e = func;
      n[2].i = ref;
      n[3].ui = mask;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->StencilFunc)( func, ref, mask );
   }
}


static void GLAPIENTRY save_StencilMask( GLuint mask )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_STENCIL_MASK, 1 );
   if (n) {
      n[1].ui = mask;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->StencilMask)( mask );
   }
}


static void GLAPIENTRY save_StencilOp( GLenum fail, GLenum zfail, GLenum zpass )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_STENCIL_OP, 3 );
   if (n) {
      n[1].e = fail;
      n[2].e = zfail;
      n[3].e = zpass;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->StencilOp)( fail, zfail, zpass );
   }
}


static void GLAPIENTRY save_TexEnvfv( GLenum target, GLenum pname, const GLfloat *params )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_TEXENV, 6 );
   if (n) {
      n[1].e = target;
      n[2].e = pname;
      n[3].f = params[0];
      n[4].f = params[1];
      n[5].f = params[2];
      n[6].f = params[3];
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->TexEnvfv)( target, pname, params );
   }
}


static void GLAPIENTRY save_TexEnvf( GLenum target, GLenum pname, GLfloat param )
{
   save_TexEnvfv( target, pname, &param );
}


static void GLAPIENTRY save_TexEnvi( GLenum target, GLenum pname, GLint param )
{
   GLfloat p[4];
   p[0] = (GLfloat) param;
   p[1] = p[2] = p[3] = 0.0;
   save_TexEnvfv( target, pname, p );
}


static void GLAPIENTRY save_TexEnviv( GLenum target, GLenum pname, const GLint *param )
{
   GLfloat p[4];
   p[0] = INT_TO_FLOAT( param[0] );
   p[1] = INT_TO_FLOAT( param[1] );
   p[2] = INT_TO_FLOAT( param[2] );
   p[3] = INT_TO_FLOAT( param[3] );
   save_TexEnvfv( target, pname, p );
}


static void GLAPIENTRY save_TexGenfv( GLenum coord, GLenum pname, const GLfloat *params )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_TEXGEN, 6 );
   if (n) {
      n[1].e = coord;
      n[2].e = pname;
      n[3].f = params[0];
      n[4].f = params[1];
      n[5].f = params[2];
      n[6].f = params[3];
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->TexGenfv)( coord, pname, params );
   }
}


static void GLAPIENTRY save_TexGeniv(GLenum coord, GLenum pname, const GLint *params )
{
   GLfloat p[4];
   p[0] = (GLfloat) params[0];
   p[1] = (GLfloat) params[1];
   p[2] = (GLfloat) params[2];
   p[3] = (GLfloat) params[3];
   save_TexGenfv(coord, pname, p);
}


static void GLAPIENTRY save_TexGend(GLenum coord, GLenum pname, GLdouble param )
{
   GLfloat p = (GLfloat) param;
   save_TexGenfv( coord, pname, &p );
}


static void GLAPIENTRY save_TexGendv(GLenum coord, GLenum pname, const GLdouble *params )
{
   GLfloat p[4];
   p[0] = (GLfloat) params[0];
   p[1] = (GLfloat) params[1];
   p[2] = (GLfloat) params[2];
   p[3] = (GLfloat) params[3];
   save_TexGenfv( coord, pname, p );
}


static void GLAPIENTRY save_TexGenf( GLenum coord, GLenum pname, GLfloat param )
{
   save_TexGenfv(coord, pname, &param);
}


static void GLAPIENTRY save_TexGeni( GLenum coord, GLenum pname, GLint param )
{
   save_TexGeniv( coord, pname, &param );
}


static void GLAPIENTRY save_TexParameterfv( GLenum target,
                             GLenum pname, const GLfloat *params )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_TEXPARAMETER, 6 );
   if (n) {
      n[1].e = target;
      n[2].e = pname;
      n[3].f = params[0];
      n[4].f = params[1];
      n[5].f = params[2];
      n[6].f = params[3];
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->TexParameterfv)( target, pname, params );
   }
}


static void GLAPIENTRY save_TexParameterf( GLenum target, GLenum pname, GLfloat param )
{
   save_TexParameterfv(target, pname, &param);
}


static void GLAPIENTRY save_TexParameteri( GLenum target, GLenum pname, GLint param )
{
   GLfloat fparam[4];
   fparam[0] = (GLfloat) param;
   fparam[1] = fparam[2] = fparam[3] = 0.0;
   save_TexParameterfv(target, pname, fparam);
}


static void GLAPIENTRY save_TexParameteriv( GLenum target, GLenum pname, const GLint *params )
{
   GLfloat fparam[4];
   fparam[0] = (GLfloat) params[0];
   fparam[1] = fparam[2] = fparam[3] = 0.0;
   save_TexParameterfv(target, pname, fparam);
}


static void GLAPIENTRY save_TexImage1D( GLenum target,
                             GLint level, GLint components,
                             GLsizei width, GLint border,
                             GLenum format, GLenum type,
                             const GLvoid *pixels )
{
   GET_CURRENT_CONTEXT(ctx);
   if (target == GL_PROXY_TEXTURE_1D) {
      /* don't compile, execute immediately */
      (*ctx->Exec->TexImage1D)( target, level, components, width,
                               border, format, type, pixels );
   }
   else {
      GLvoid *image = _mesa_unpack_image(width, 1, 1, format, type,
                                         pixels, &ctx->Unpack);
      Node *n;
      ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
      n = ALLOC_INSTRUCTION( ctx, OPCODE_TEX_IMAGE1D, 8 );
      if (n) {
         n[1].e = target;
         n[2].i = level;
         n[3].i = components;
         n[4].i = (GLint) width;
         n[5].i = border;
         n[6].e = format;
         n[7].e = type;
         n[8].data = image;
      }
      else if (image) {
         FREE(image);
      }
      if (ctx->ExecuteFlag) {
         (*ctx->Exec->TexImage1D)( target, level, components, width,
                                  border, format, type, pixels );
      }
   }
}


static void GLAPIENTRY save_TexImage2D( GLenum target,
                             GLint level, GLint components,
                             GLsizei width, GLsizei height, GLint border,
                             GLenum format, GLenum type,
                             const GLvoid *pixels)
{
   GET_CURRENT_CONTEXT(ctx);
   if (target == GL_PROXY_TEXTURE_2D) {
      /* don't compile, execute immediately */
      (*ctx->Exec->TexImage2D)( target, level, components, width,
                               height, border, format, type, pixels );
   }
   else {
      GLvoid *image = _mesa_unpack_image(width, height, 1, format, type,
                                         pixels, &ctx->Unpack);
      Node *n;
      ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
      n = ALLOC_INSTRUCTION( ctx, OPCODE_TEX_IMAGE2D, 9 );
      if (n) {
         n[1].e = target;
         n[2].i = level;
         n[3].i = components;
         n[4].i = (GLint) width;
         n[5].i = (GLint) height;
         n[6].i = border;
         n[7].e = format;
         n[8].e = type;
         n[9].data = image;
      }
      else if (image) {
         FREE(image);
      }
      if (ctx->ExecuteFlag) {
         (*ctx->Exec->TexImage2D)( target, level, components, width,
                                  height, border, format, type, pixels );
      }
   }
}


static void GLAPIENTRY save_TexImage3D( GLenum target,
                             GLint level, GLint internalFormat,
                             GLsizei width, GLsizei height, GLsizei depth,
                             GLint border,
                             GLenum format, GLenum type,
                             const GLvoid *pixels )
{
   GET_CURRENT_CONTEXT(ctx);
   if (target == GL_PROXY_TEXTURE_3D) {
      /* don't compile, execute immediately */
      (*ctx->Exec->TexImage3D)( target, level, internalFormat, width,
                               height, depth, border, format, type, pixels );
   }
   else {
      Node *n;
      GLvoid *image = _mesa_unpack_image(width, height, depth, format, type,
                                         pixels, &ctx->Unpack);
      ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
      n = ALLOC_INSTRUCTION( ctx, OPCODE_TEX_IMAGE3D, 10 );
      if (n) {
         n[1].e = target;
         n[2].i = level;
         n[3].i = (GLint) internalFormat;
         n[4].i = (GLint) width;
         n[5].i = (GLint) height;
         n[6].i = (GLint) depth;
         n[7].i = border;
         n[8].e = format;
         n[9].e = type;
         n[10].data = image;
      }
      else if (image) {
         FREE(image);
      }
      if (ctx->ExecuteFlag) {
         (*ctx->Exec->TexImage3D)( target, level, internalFormat, width,
                                height, depth, border, format, type, pixels );
      }
   }
}


static void GLAPIENTRY save_TexSubImage1D( GLenum target, GLint level, GLint xoffset,
                                GLsizei width, GLenum format, GLenum type,
                                const GLvoid *pixels )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   GLvoid *image = _mesa_unpack_image(width, 1, 1, format, type,
                                      pixels, &ctx->Unpack);
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_TEX_SUB_IMAGE1D, 7 );
   if (n) {
      n[1].e = target;
      n[2].i = level;
      n[3].i = xoffset;
      n[4].i = (GLint) width;
      n[5].e = format;
      n[6].e = type;
      n[7].data = image;
   }
   else if (image) {
      FREE(image);
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->TexSubImage1D)( target, level, xoffset, width,
                                  format, type, pixels );
   }
}


static void GLAPIENTRY save_TexSubImage2D( GLenum target, GLint level,
                                GLint xoffset, GLint yoffset,
                                GLsizei width, GLsizei height,
                                GLenum format, GLenum type,
                                const GLvoid *pixels )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   GLvoid *image = _mesa_unpack_image(width, height, 1, format, type,
                                      pixels, &ctx->Unpack);
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_TEX_SUB_IMAGE2D, 9 );
   if (n) {
      n[1].e = target;
      n[2].i = level;
      n[3].i = xoffset;
      n[4].i = yoffset;
      n[5].i = (GLint) width;
      n[6].i = (GLint) height;
      n[7].e = format;
      n[8].e = type;
      n[9].data = image;
   }
   else if (image) {
      FREE(image);
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->TexSubImage2D)( target, level, xoffset, yoffset,
                           width, height, format, type, pixels );
   }
}


static void GLAPIENTRY save_TexSubImage3D( GLenum target, GLint level,
                                GLint xoffset, GLint yoffset,GLint zoffset,
                                GLsizei width, GLsizei height, GLsizei depth,
                                GLenum format, GLenum type,
                                const GLvoid *pixels )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   GLvoid *image = _mesa_unpack_image(width, height, depth, format, type,
                                      pixels, &ctx->Unpack);
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_TEX_SUB_IMAGE3D, 11 );
   if (n) {
      n[1].e = target;
      n[2].i = level;
      n[3].i = xoffset;
      n[4].i = yoffset;
      n[5].i = zoffset;
      n[6].i = (GLint) width;
      n[7].i = (GLint) height;
      n[8].i = (GLint) depth;
      n[9].e = format;
      n[10].e = type;
      n[11].data = image;
   }
   else if (image) {
      FREE(image);
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->TexSubImage3D)( target, level,
                                  xoffset, yoffset, zoffset,
                                  width, height, depth, format, type, pixels );
   }
}


static void GLAPIENTRY save_Translatef( GLfloat x, GLfloat y, GLfloat z )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx,  OPCODE_TRANSLATE, 3 );
   if (n) {
      n[1].f = x;
      n[2].f = y;
      n[3].f = z;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->Translatef)( x, y, z );
   }
}


static void GLAPIENTRY save_Translated( GLdouble x, GLdouble y, GLdouble z )
{
   save_Translatef((GLfloat) x, (GLfloat) y, (GLfloat) z);
}



static void GLAPIENTRY save_Viewport( GLint x, GLint y, GLsizei width, GLsizei height )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx,  OPCODE_VIEWPORT, 4 );
   if (n) {
      n[1].i = x;
      n[2].i = y;
      n[3].i = (GLint) width;
      n[4].i = (GLint) height;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->Viewport)( x, y, width, height );
   }
}


static void GLAPIENTRY save_WindowPos4fMESA( GLfloat x, GLfloat y, GLfloat z, GLfloat w )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx,  OPCODE_WINDOW_POS, 4 );
   if (n) {
      n[1].f = x;
      n[2].f = y;
      n[3].f = z;
      n[4].f = w;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->WindowPos4fMESA)( x, y, z, w );
   }
}

static void GLAPIENTRY save_WindowPos2dMESA(GLdouble x, GLdouble y)
{
   save_WindowPos4fMESA((GLfloat) x, (GLfloat) y, 0.0F, 1.0F);
}

static void GLAPIENTRY save_WindowPos2fMESA(GLfloat x, GLfloat y)
{
   save_WindowPos4fMESA(x, y, 0.0F, 1.0F);
}

static void GLAPIENTRY save_WindowPos2iMESA(GLint x, GLint y)
{
   save_WindowPos4fMESA((GLfloat) x, (GLfloat) y, 0.0F, 1.0F);
}

static void GLAPIENTRY save_WindowPos2sMESA(GLshort x, GLshort y)
{
   save_WindowPos4fMESA(x, y, 0.0F, 1.0F);
}

static void GLAPIENTRY save_WindowPos3dMESA(GLdouble x, GLdouble y, GLdouble z)
{
   save_WindowPos4fMESA((GLfloat) x, (GLfloat) y, (GLfloat) z, 1.0F);
}

static void GLAPIENTRY save_WindowPos3fMESA(GLfloat x, GLfloat y, GLfloat z)
{
   save_WindowPos4fMESA(x, y, z, 1.0F);
}

static void GLAPIENTRY save_WindowPos3iMESA(GLint x, GLint y, GLint z)
{
   save_WindowPos4fMESA((GLfloat) x, (GLfloat) y, (GLfloat) z, 1.0F);
}

static void GLAPIENTRY save_WindowPos3sMESA(GLshort x, GLshort y, GLshort z)
{
   save_WindowPos4fMESA(x, y, z, 1.0F);
}

static void GLAPIENTRY save_WindowPos4dMESA(GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
   save_WindowPos4fMESA((GLfloat) x, (GLfloat) y, (GLfloat) z, (GLfloat) w);
}

static void GLAPIENTRY save_WindowPos4iMESA(GLint x, GLint y, GLint z, GLint w)
{
   save_WindowPos4fMESA((GLfloat) x, (GLfloat) y, (GLfloat) z, (GLfloat) w);
}

static void GLAPIENTRY save_WindowPos4sMESA(GLshort x, GLshort y, GLshort z, GLshort w)
{
   save_WindowPos4fMESA(x, y, z, w);
}

static void GLAPIENTRY save_WindowPos2dvMESA(const GLdouble *v)
{
   save_WindowPos4fMESA((GLfloat) v[0], (GLfloat) v[1], 0.0F, 1.0F);
}

static void GLAPIENTRY save_WindowPos2fvMESA(const GLfloat *v)
{
   save_WindowPos4fMESA(v[0], v[1], 0.0F, 1.0F);
}

static void GLAPIENTRY save_WindowPos2ivMESA(const GLint *v)
{
   save_WindowPos4fMESA((GLfloat) v[0], (GLfloat) v[1], 0.0F, 1.0F);
}

static void GLAPIENTRY save_WindowPos2svMESA(const GLshort *v)
{
   save_WindowPos4fMESA(v[0], v[1], 0.0F, 1.0F);
}

static void GLAPIENTRY save_WindowPos3dvMESA(const GLdouble *v)
{
   save_WindowPos4fMESA((GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2], 1.0F);
}

static void GLAPIENTRY save_WindowPos3fvMESA(const GLfloat *v)
{
   save_WindowPos4fMESA(v[0], v[1], v[2], 1.0F);
}

static void GLAPIENTRY save_WindowPos3ivMESA(const GLint *v)
{
   save_WindowPos4fMESA((GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2], 1.0F);
}

static void GLAPIENTRY save_WindowPos3svMESA(const GLshort *v)
{
   save_WindowPos4fMESA(v[0], v[1], v[2], 1.0F);
}

static void GLAPIENTRY save_WindowPos4dvMESA(const GLdouble *v)
{
   save_WindowPos4fMESA((GLfloat) v[0], (GLfloat) v[1],
			(GLfloat) v[2], (GLfloat) v[3]);
}

static void GLAPIENTRY save_WindowPos4fvMESA(const GLfloat *v)
{
   save_WindowPos4fMESA(v[0], v[1], v[2], v[3]);
}

static void GLAPIENTRY save_WindowPos4ivMESA(const GLint *v)
{
   save_WindowPos4fMESA((GLfloat) v[0], (GLfloat) v[1],
			(GLfloat) v[2], (GLfloat) v[3]);
}

static void GLAPIENTRY save_WindowPos4svMESA(const GLshort *v)
{
   save_WindowPos4fMESA(v[0], v[1], v[2], v[3]);
}



/* GL_ARB_multitexture */
static void GLAPIENTRY save_ActiveTextureARB( GLenum target )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_ACTIVE_TEXTURE, 1 );
   if (n) {
      n[1].e = target;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->ActiveTextureARB)( target );
   }
}


/* GL_ARB_transpose_matrix */

static void GLAPIENTRY save_LoadTransposeMatrixdARB( const GLdouble m[16] )
{
   GLfloat tm[16];
   _math_transposefd(tm, m);
   save_LoadMatrixf(tm);
}


static void GLAPIENTRY save_LoadTransposeMatrixfARB( const GLfloat m[16] )
{
   GLfloat tm[16];
   _math_transposef(tm, m);
   save_LoadMatrixf(tm);
}


static void GLAPIENTRY
save_MultTransposeMatrixdARB( const GLdouble m[16] )
{
   GLfloat tm[16];
   _math_transposefd(tm, m);
   save_MultMatrixf(tm);
}


static void GLAPIENTRY
save_MultTransposeMatrixfARB( const GLfloat m[16] )
{
   GLfloat tm[16];
   _math_transposef(tm, m);
   save_MultMatrixf(tm);
}


static void GLAPIENTRY
save_PixelTexGenSGIX(GLenum mode)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_PIXEL_TEXGEN_SGIX, 1 );
   if (n) {
      n[1].e = mode;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->PixelTexGenSGIX)( mode );
   }
}


/* GL_ARB_texture_compression */
static void GLAPIENTRY
save_CompressedTexImage1DARB(GLenum target, GLint level,
                             GLenum internalFormat, GLsizei width,
                             GLint border, GLsizei imageSize,
                             const GLvoid *data)
{
   GET_CURRENT_CONTEXT(ctx);
   if (target == GL_PROXY_TEXTURE_1D) {
      /* don't compile, execute immediately */
      (*ctx->Exec->CompressedTexImage1DARB)(target, level, internalFormat,
                                            width, border, imageSize, data);
   }
   else {
      Node *n;
      GLvoid *image;
      ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
      /* make copy of image */
      image = MALLOC(imageSize);
      if (!image) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glCompressedTexImage1DARB");
         return;
      }
      MEMCPY(image, data, imageSize);
      n = ALLOC_INSTRUCTION( ctx, OPCODE_COMPRESSED_TEX_IMAGE_1D, 7 );
      if (n) {
         n[1].e = target;
         n[2].i = level;
         n[3].e = internalFormat;
         n[4].i = (GLint) width;
         n[5].i = border;
         n[6].i = imageSize;
         n[7].data = image;
      }
      else if (image) {
         FREE(image);
      }
      if (ctx->ExecuteFlag) {
         (*ctx->Exec->CompressedTexImage1DARB)(target, level, internalFormat,
                                               width, border, imageSize, data);
      }
   }
}


static void GLAPIENTRY
save_CompressedTexImage2DARB(GLenum target, GLint level,
                             GLenum internalFormat, GLsizei width,
                             GLsizei height, GLint border, GLsizei imageSize,
                             const GLvoid *data)
{
   GET_CURRENT_CONTEXT(ctx);
   if (target == GL_PROXY_TEXTURE_2D) {
      /* don't compile, execute immediately */
      (*ctx->Exec->CompressedTexImage2DARB)(target, level, internalFormat,
                                       width, height, border, imageSize, data);
   }
   else {
      Node *n;
      GLvoid *image;
      ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
      /* make copy of image */
      image = MALLOC(imageSize);
      if (!image) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glCompressedTexImage2DARB");
         return;
      }
      MEMCPY(image, data, imageSize);
      n = ALLOC_INSTRUCTION( ctx, OPCODE_COMPRESSED_TEX_IMAGE_2D, 8 );
      if (n) {
         n[1].e = target;
         n[2].i = level;
         n[3].e = internalFormat;
         n[4].i = (GLint) width;
         n[5].i = (GLint) height;
         n[6].i = border;
         n[7].i = imageSize;
         n[8].data = image;
      }
      else if (image) {
         FREE(image);
      }
      if (ctx->ExecuteFlag) {
         (*ctx->Exec->CompressedTexImage2DARB)(target, level, internalFormat,
                                      width, height, border, imageSize, data);
      }
   }
}


static void GLAPIENTRY
save_CompressedTexImage3DARB(GLenum target, GLint level,
                             GLenum internalFormat, GLsizei width,
                             GLsizei height, GLsizei depth, GLint border,
                             GLsizei imageSize, const GLvoid *data)
{
   GET_CURRENT_CONTEXT(ctx);
   if (target == GL_PROXY_TEXTURE_3D) {
      /* don't compile, execute immediately */
      (*ctx->Exec->CompressedTexImage3DARB)(target, level, internalFormat,
                                width, height, depth, border, imageSize, data);
   }
   else {
      Node *n;
      GLvoid *image;
      ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
      /* make copy of image */
      image = MALLOC(imageSize);
      if (!image) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glCompressedTexImage3DARB");
         return;
      }
      MEMCPY(image, data, imageSize);
      n = ALLOC_INSTRUCTION( ctx, OPCODE_COMPRESSED_TEX_IMAGE_3D, 9 );
      if (n) {
         n[1].e = target;
         n[2].i = level;
         n[3].e = internalFormat;
         n[4].i = (GLint) width;
         n[5].i = (GLint) height;
         n[6].i = (GLint) depth;
         n[7].i = border;
         n[8].i = imageSize;
         n[9].data = image;
      }
      else if (image) {
         FREE(image);
      }
      if (ctx->ExecuteFlag) {
         (*ctx->Exec->CompressedTexImage3DARB)(target, level, internalFormat,
                                width, height, depth, border, imageSize, data);
      }
   }
}


static void GLAPIENTRY
save_CompressedTexSubImage1DARB(GLenum target, GLint level, GLint xoffset,
                                GLsizei width, GLenum format,
                                GLsizei imageSize, const GLvoid *data)
{
   Node *n;
   GLvoid *image;

   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);

   /* make copy of image */
   image = MALLOC(imageSize);
   if (!image) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glCompressedTexSubImage1DARB");
      return;
   }
   MEMCPY(image, data, imageSize);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_COMPRESSED_TEX_SUB_IMAGE_1D, 7 );
   if (n) {
      n[1].e = target;
      n[2].i = level;
      n[3].i = xoffset;
      n[4].i = (GLint) width;
      n[5].e = format;
      n[6].i = imageSize;
      n[7].data = image;
   }
   else if (image) {
      FREE(image);
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->CompressedTexSubImage1DARB)(target, level, xoffset,
                                            width, format, imageSize, data);
   }
}


static void GLAPIENTRY
save_CompressedTexSubImage2DARB(GLenum target, GLint level, GLint xoffset,
                                GLint yoffset, GLsizei width, GLsizei height,
                                GLenum format, GLsizei imageSize,
                                const GLvoid *data)
{
   Node *n;
   GLvoid *image;

   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);

   /* make copy of image */
   image = MALLOC(imageSize);
   if (!image) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glCompressedTexSubImage2DARB");
      return;
   }
   MEMCPY(image, data, imageSize);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_COMPRESSED_TEX_SUB_IMAGE_2D, 9 );
   if (n) {
      n[1].e = target;
      n[2].i = level;
      n[3].i = xoffset;
      n[4].i = yoffset;
      n[5].i = (GLint) width;
      n[6].i = (GLint) height;
      n[7].e = format;
      n[8].i = imageSize;
      n[9].data = image;
   }
   else if (image) {
      FREE(image);
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->CompressedTexSubImage2DARB)(target, level, xoffset, yoffset,
                                       width, height, format, imageSize, data);
   }
}


static void GLAPIENTRY
save_CompressedTexSubImage3DARB(GLenum target, GLint level, GLint xoffset,
                                GLint yoffset, GLint zoffset, GLsizei width,
                                GLsizei height, GLsizei depth, GLenum format,
                                GLsizei imageSize, const GLvoid *data)
{
   Node *n;
   GLvoid *image;

   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);

   /* make copy of image */
   image = MALLOC(imageSize);
   if (!image) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glCompressedTexSubImage3DARB");
      return;
   }
   MEMCPY(image, data, imageSize);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_COMPRESSED_TEX_SUB_IMAGE_3D, 11 );
   if (n) {
      n[1].e = target;
      n[2].i = level;
      n[3].i = xoffset;
      n[4].i = yoffset;
      n[5].i = zoffset;
      n[6].i = (GLint) width;
      n[7].i = (GLint) height;
      n[8].i = (GLint) depth;
      n[9].e = format;
      n[10].i = imageSize;
      n[11].data = image;
   }
   else if (image) {
      FREE(image);
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->CompressedTexSubImage3DARB)(target, level, xoffset, yoffset,
                       zoffset, width, height, depth, format, imageSize, data);
   }
}


/* GL_ARB_multisample */
static void GLAPIENTRY
save_SampleCoverageARB(GLclampf value, GLboolean invert)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_SAMPLE_COVERAGE, 2 );
   if (n) {
      n[1].f = value;
      n[2].b = invert;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->SampleCoverageARB)( value, invert );
   }
}


/* GL_SGIS_pixel_texture */

static void GLAPIENTRY
save_PixelTexGenParameteriSGIS(GLenum target, GLint value)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_PIXEL_TEXGEN_PARAMETER_SGIS, 2 );
   if (n) {
      n[1].e = target;
      n[2].i = value;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->PixelTexGenParameteriSGIS)( target, value );
   }
}


static void GLAPIENTRY
save_PixelTexGenParameterfSGIS(GLenum target, GLfloat value)
{
   save_PixelTexGenParameteriSGIS(target, (GLint) value);
}


static void GLAPIENTRY
save_PixelTexGenParameterivSGIS(GLenum target, const GLint *value)
{
   save_PixelTexGenParameteriSGIS(target, *value);
}


static void GLAPIENTRY
save_PixelTexGenParameterfvSGIS(GLenum target, const GLfloat *value)
{
   save_PixelTexGenParameteriSGIS(target, (GLint) *value);
}


/*
 * GL_NV_vertex_program
 */
#if FEATURE_NV_vertex_program || FEATURE_ARB_vertex_program || FEATURE_ARB_fragment_program
static void GLAPIENTRY
save_BindProgramNV(GLenum target, GLuint id)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_BIND_PROGRAM_NV, 2 );
   if (n) {
      n[1].e = target;
      n[2].ui = id;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->BindProgramNV)( target, id );
   }
}
#endif /* FEATURE_NV_vertex_program || FEATURE_ARB_vertex_program || FEATURE_ARB_fragment_program */

#if FEATURE_NV_vertex_program
static void GLAPIENTRY
save_ExecuteProgramNV(GLenum target, GLuint id, const GLfloat *params)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_EXECUTE_PROGRAM_NV, 6 );
   if (n) {
      n[1].e = target;
      n[2].ui = id;
      n[3].f = params[0];
      n[4].f = params[1];
      n[5].f = params[2];
      n[6].f = params[3];
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->ExecuteProgramNV)(target, id, params);
   }
}


static void GLAPIENTRY
save_ProgramParameter4fNV(GLenum target, GLuint index,
                          GLfloat x, GLfloat y,
                          GLfloat z, GLfloat w)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_PROGRAM_PARAMETER4F_NV, 6 );
   if (n) {
      n[1].e = target;
      n[2].ui = index;
      n[3].f = x;
      n[4].f = y;
      n[5].f = z;
      n[6].f = w;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->ProgramParameter4fNV)(target, index, x, y, z, w);
   }
}


static void GLAPIENTRY
save_ProgramParameter4fvNV(GLenum target, GLuint index, const GLfloat *params)
{
   save_ProgramParameter4fNV(target, index, params[0], params[1],
                             params[2], params[3]);
}


static void GLAPIENTRY
save_ProgramParameter4dNV(GLenum target, GLuint index,
                          GLdouble x, GLdouble y,
                          GLdouble z, GLdouble w)
{
   save_ProgramParameter4fNV(target, index, (GLfloat) x, (GLfloat) y,
                             (GLfloat) z, (GLfloat) w);
}


static void GLAPIENTRY
save_ProgramParameter4dvNV(GLenum target, GLuint index,
                           const GLdouble *params)
{
   save_ProgramParameter4fNV(target, index, (GLfloat) params[0],
                             (GLfloat) params[1], (GLfloat) params[2],
                             (GLfloat) params[3]);
}


static void GLAPIENTRY
save_ProgramParameters4dvNV(GLenum target, GLuint index,
                            GLuint num, const GLdouble *params)
{
   GLuint i;
   for (i = 0; i < num; i++) {
      save_ProgramParameter4dvNV(target, index + i, params + 4 * i);
   }
}


static void GLAPIENTRY
save_ProgramParameters4fvNV(GLenum target, GLuint index,
                            GLuint num, const GLfloat *params)
{
   GLuint i;
   for (i = 0; i < num; i++) {
      save_ProgramParameter4fvNV(target, index + i, params + 4 * i);
   }
}


static void GLAPIENTRY
save_LoadProgramNV(GLenum target, GLuint id, GLsizei len,
                   const GLubyte *program)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   GLubyte *programCopy;

   programCopy = (GLubyte *) _mesa_malloc(len);
   if (!programCopy) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glLoadProgramNV");
      return;
   }
   _mesa_memcpy(programCopy, program, len);

   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_LOAD_PROGRAM_NV, 4 );
   if (n) {
      n[1].e = target;
      n[2].ui = id;
      n[3].i = len;
      n[4].data = programCopy;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->LoadProgramNV)(target, id, len, program);
   }
}


static void GLAPIENTRY
save_RequestResidentProgramsNV(GLsizei num, const GLuint *ids)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   GLuint *idCopy = (GLuint *) _mesa_malloc(num * sizeof(GLuint));
   if (!idCopy) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glRequestResidentProgramsNV");
      return;
   }
   _mesa_memcpy(idCopy, ids, num * sizeof(GLuint));
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_TRACK_MATRIX_NV, 2 );
   if (n) {
      n[1].i = num;
      n[2].data = idCopy;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->RequestResidentProgramsNV)(num, ids);
   }
}


static void GLAPIENTRY
save_TrackMatrixNV(GLenum target, GLuint address,
                   GLenum matrix, GLenum transform)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_TRACK_MATRIX_NV, 4 );
   if (n) {
      n[1].e = target;
      n[2].ui = address;
      n[3].e = matrix;
      n[4].e = transform;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->TrackMatrixNV)(target, address, matrix, transform);
   }
}
#endif /* FEATURE_NV_vertex_program */


/*
 * GL_NV_fragment_program
 */
#if FEATURE_NV_fragment_program
static void GLAPIENTRY
save_ProgramLocalParameter4fARB(GLenum target, GLuint index,
                                GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_PROGRAM_LOCAL_PARAMETER_ARB, 6 );
   if (n) {
      n[1].e = target;
      n[2].ui = index;
      n[3].f = x;
      n[4].f = y;
      n[5].f = z;
      n[6].f = w;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->ProgramLocalParameter4fARB)(target, index, x, y, z, w);
   }
}


static void GLAPIENTRY
save_ProgramLocalParameter4fvARB(GLenum target, GLuint index,
                                 const GLfloat *params)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_PROGRAM_LOCAL_PARAMETER_ARB, 6 );
   if (n) {
      n[1].e = target;
      n[2].ui = index;
      n[3].f = params[0];
      n[4].f = params[1];
      n[5].f = params[2];
      n[6].f = params[3];
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->ProgramLocalParameter4fvARB)(target, index, params);
   }
}


static void GLAPIENTRY
save_ProgramLocalParameter4dARB(GLenum target, GLuint index,
                                GLdouble x, GLdouble y,
                                GLdouble z, GLdouble w)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_PROGRAM_LOCAL_PARAMETER_ARB, 6 );
   if (n) {
      n[1].e = target;
      n[2].ui = index;
      n[3].f = (GLfloat) x;
      n[4].f = (GLfloat) y;
      n[5].f = (GLfloat) z;
      n[6].f = (GLfloat) w;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->ProgramLocalParameter4dARB)(target, index, x, y, z, w);
   }
}


static void GLAPIENTRY
save_ProgramLocalParameter4dvARB(GLenum target, GLuint index,
                                 const GLdouble *params)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_PROGRAM_LOCAL_PARAMETER_ARB, 6 );
   if (n) {
      n[1].e = target;
      n[2].ui = index;
      n[3].f = (GLfloat) params[0];
      n[4].f = (GLfloat) params[1];
      n[5].f = (GLfloat) params[2];
      n[6].f = (GLfloat) params[3];
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->ProgramLocalParameter4dvARB)(target, index, params);
   }
}

static void GLAPIENTRY
save_ProgramNamedParameter4fNV(GLuint id, GLsizei len, const GLubyte *name,
                               GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   GLubyte *nameCopy = (GLubyte *) _mesa_malloc(len);
   if (!nameCopy) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glProgramNamedParameter4fNV");
      return;
   }
   _mesa_memcpy(nameCopy, name, len);

   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_PROGRAM_NAMED_PARAMETER_NV, 6 );
   if (n) {
      n[1].ui = id;
      n[2].i = len;
      n[3].data = nameCopy;
      n[4].f = x;
      n[5].f = y;
      n[6].f = z;
      n[7].f = w;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->ProgramNamedParameter4fNV)(id, len, name, x, y, z, w);
   }
}


static void GLAPIENTRY
save_ProgramNamedParameter4fvNV(GLuint id, GLsizei len, const GLubyte *name,
                                const float v[])
{
   save_ProgramNamedParameter4fNV(id, len, name, v[0], v[1], v[2], v[3]);
}


static void GLAPIENTRY
save_ProgramNamedParameter4dNV(GLuint id, GLsizei len, const GLubyte *name,
                               GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
   save_ProgramNamedParameter4fNV(id, len, name, (GLfloat) x, (GLfloat) y,
                                  (GLfloat) z,(GLfloat) w);
}


static void GLAPIENTRY
save_ProgramNamedParameter4dvNV(GLuint id, GLsizei len, const GLubyte *name,
                                const double v[])
{
   save_ProgramNamedParameter4fNV(id, len, name, (GLfloat) v[0],
                                  (GLfloat) v[1], (GLfloat) v[2],
                                  (GLfloat) v[3]);
}

#endif /* FEATURE_NV_fragment_program */



/* GL_EXT_stencil_two_side */
static void GLAPIENTRY save_ActiveStencilFaceEXT( GLenum face )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_ACTIVE_STENCIL_FACE_EXT, 1 );
   if (n) {
      n[1].e = face;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->ActiveStencilFaceEXT)( face );
   }
}


/* GL_EXT_depth_bounds_test */
static void GLAPIENTRY save_DepthBoundsEXT( GLclampd zmin, GLclampd zmax )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_ACTIVE_STENCIL_FACE_EXT, 2 );
   if (n) {
      n[1].f = (GLfloat) zmin;
      n[2].f = (GLfloat) zmax;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->DepthBoundsEXT)( zmin, zmax );
   }
}



#if FEATURE_ARB_vertex_program || FEATURE_ARB_fragment_program

static void GLAPIENTRY
save_ProgramStringARB(GLenum target, GLenum format, GLsizei len,
                      const GLvoid *string)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   GLubyte *programCopy;

   programCopy = (GLubyte *) _mesa_malloc(len);
   if (!programCopy) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glProgramStringARB");
      return;
   }
   _mesa_memcpy(programCopy, string, len);

   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_PROGRAM_STRING_ARB, 4 );
   if (n) {
      n[1].e = target;
      n[2].e = format;
      n[3].i = len;
      n[4].data = programCopy;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->ProgramStringARB)(target, format, len, string);
   }
}


static void GLAPIENTRY
save_ProgramEnvParameter4fARB(GLenum target, GLuint index,
                              GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = ALLOC_INSTRUCTION( ctx, OPCODE_PROGRAM_ENV_PARAMETER_ARB, 6 );
   if (n) {
      n[1].e = target;
      n[2].ui = index;
      n[3].f = x;
      n[4].f = y;
      n[5].f = z;
      n[6].f = w;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->ProgramEnvParameter4fARB)( target, index, x, y, z, w);
   }
}


static void GLAPIENTRY
save_ProgramEnvParameter4fvARB(GLenum target, GLuint index,
                               const GLfloat *params)
{
   save_ProgramEnvParameter4fARB(target, index, params[0], params[1],
                                 params[2], params[3]);
}


static void GLAPIENTRY
save_ProgramEnvParameter4dARB(GLenum target, GLuint index,
                              GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
   save_ProgramEnvParameter4fARB(target, index,
                                 (GLfloat) x,
                                 (GLfloat) y,
                                 (GLfloat) z,
                                 (GLfloat) w);
}


static void GLAPIENTRY
save_ProgramEnvParameter4dvARB(GLenum target, GLuint index,
                               const GLdouble *params)
{
   save_ProgramEnvParameter4fARB(target, index,
                                 (GLfloat) params[0],
                                 (GLfloat) params[1],
                                 (GLfloat) params[2],
                                 (GLfloat) params[3]);
}

#endif /* FEATURE_ARB_vertex_program || FEATURE_ARB_fragment_program */




static void save_Attr1f( GLenum attr, GLfloat x )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   SAVE_FLUSH_VERTICES( ctx );
   n = ALLOC_INSTRUCTION( ctx, OPCODE_ATTR_1F, 2 );
   if (n) {
      n[1].e = attr;
      n[2].f = x;
   }

   ctx->ListState.ActiveAttribSize[attr] = 1;
   ASSIGN_4V( ctx->ListState.CurrentAttrib[attr], x, 0, 0, 1);

   if (ctx->ExecuteFlag) {
      (*ctx->Exec->VertexAttrib1fNV)( attr, x );
   }
}

static void save_Attr2f( GLenum attr, GLfloat x, GLfloat y )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   SAVE_FLUSH_VERTICES( ctx );
   n = ALLOC_INSTRUCTION( ctx, OPCODE_ATTR_2F, 3 );
   if (n) {
      n[1].e = attr;
      n[2].f = x;
      n[3].f = y;
   }

   ctx->ListState.ActiveAttribSize[attr] = 2;
   ASSIGN_4V( ctx->ListState.CurrentAttrib[attr], x, y, 0, 1);

   if (ctx->ExecuteFlag) {
      (*ctx->Exec->VertexAttrib2fNV)( attr, x, y );
   }
}

static void save_Attr3f( GLenum attr, GLfloat x, GLfloat y, GLfloat z )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   SAVE_FLUSH_VERTICES( ctx );
   n = ALLOC_INSTRUCTION( ctx, OPCODE_ATTR_3F, 4 );
   if (n) {
      n[1].e = attr;
      n[2].f = x;
      n[3].f = y;
      n[4].f = z;
   }

   ctx->ListState.ActiveAttribSize[attr] = 3;
   ASSIGN_4V( ctx->ListState.CurrentAttrib[attr], x, y, z, 1);

   if (ctx->ExecuteFlag) {
      (*ctx->Exec->VertexAttrib3fNV)( attr, x, y, z );
   }
}

static void save_Attr4f( GLenum attr, GLfloat x, GLfloat y, GLfloat z,
			 GLfloat w )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   SAVE_FLUSH_VERTICES( ctx );
   n = ALLOC_INSTRUCTION( ctx, OPCODE_ATTR_4F, 5 );
   if (n) {
      n[1].e = attr;
      n[2].f = x;
      n[3].f = y;
      n[4].f = z;
      n[5].f = w;
   }

   ctx->ListState.ActiveAttribSize[attr] = 4;
   ASSIGN_4V( ctx->ListState.CurrentAttrib[attr], x, y, z, w);

   if (ctx->ExecuteFlag) {
      (*ctx->Exec->VertexAttrib4fNV)( attr, x, y, z, w );
   }
}

static void GLAPIENTRY save_EvalCoord1f( GLfloat x )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   SAVE_FLUSH_VERTICES( ctx );
   n = ALLOC_INSTRUCTION( ctx, OPCODE_EVAL_C1, 1 );
   if (n) {
      n[1].f = x;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->EvalCoord1f)( x );
   }
}

static void GLAPIENTRY save_EvalCoord1fv( const GLfloat *v )
{
   save_EvalCoord1f( v[0] );
}

static void GLAPIENTRY save_EvalCoord2f( GLfloat x, GLfloat y )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   SAVE_FLUSH_VERTICES( ctx );
   n = ALLOC_INSTRUCTION( ctx, OPCODE_EVAL_C2, 2 );
   if (n) {
      n[1].f = x;
      n[2].f = y;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->EvalCoord2f)( x, y );
   }
}

static void GLAPIENTRY save_EvalCoord2fv( const GLfloat *v )
{
   save_EvalCoord2f( v[0], v[1] );
}


static void GLAPIENTRY save_EvalPoint1( GLint x )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   SAVE_FLUSH_VERTICES( ctx );
   n = ALLOC_INSTRUCTION( ctx, OPCODE_EVAL_P1, 1 );
   if (n) {
      n[1].i = x;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->EvalPoint1)( x );
   }
}

static void GLAPIENTRY save_EvalPoint2( GLint x, GLint y )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   SAVE_FLUSH_VERTICES( ctx );
   n = ALLOC_INSTRUCTION( ctx, OPCODE_EVAL_P2, 2 );
   if (n) {
      n[1].i = x;
      n[2].i = y;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->EvalPoint2)( x, y );
   }
}

static void GLAPIENTRY save_Indexf( GLfloat x )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   SAVE_FLUSH_VERTICES( ctx );
   n = ALLOC_INSTRUCTION( ctx, OPCODE_INDEX, 1 );
   if (n) {
      n[1].f = x;
   }

   ctx->ListState.ActiveIndex = 1;
   ctx->ListState.CurrentIndex = x;

   if (ctx->ExecuteFlag) {
      (*ctx->Exec->Indexi)( (GLint) x );
   }
}

static void GLAPIENTRY save_Indexfv( const GLfloat *v )
{
   save_Indexf( v[0] );
}

static void GLAPIENTRY save_EdgeFlag( GLboolean x )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   SAVE_FLUSH_VERTICES( ctx );
   n = ALLOC_INSTRUCTION( ctx, OPCODE_EDGEFLAG, 1 );
   if (n) {
      n[1].b = x;
   }

   ctx->ListState.ActiveEdgeFlag = 1;
   ctx->ListState.CurrentEdgeFlag = x;

   if (ctx->ExecuteFlag) {
      (*ctx->Exec->EdgeFlag)( x );
   }
}

static void GLAPIENTRY save_EdgeFlagv( const GLboolean *v )
{
   save_EdgeFlag( v[0] );
}

static void GLAPIENTRY save_Materialfv( GLenum face, GLenum pname, const GLfloat *param )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   int args, i;

   SAVE_FLUSH_VERTICES( ctx );

   switch (face) {
   case GL_BACK:
   case GL_FRONT:
   case GL_FRONT_AND_BACK:
      break;
   default:
      _mesa_compile_error( ctx, GL_INVALID_ENUM, "material(face)" );
      return;
   }

   switch (pname) {
   case GL_EMISSION:
   case GL_AMBIENT:
   case GL_DIFFUSE:
   case GL_SPECULAR:
   case GL_AMBIENT_AND_DIFFUSE:
      args = 4;
      break;
   case GL_SHININESS:
      args = 1;
      break;
   case GL_COLOR_INDEXES:
      args = 3;
      break;
   default:
      _mesa_compile_error( ctx, GL_INVALID_ENUM, "material(pname)" );
      return;
   }

   n = ALLOC_INSTRUCTION( ctx, OPCODE_MATERIAL, 6 );
   if (n) {
      n[1].e = face;
      n[2].e = pname;
      for (i = 0 ; i < args ; i++)
	 n[3+i].f = param[i];
   }

   {
      GLuint bitmask = _mesa_material_bitmask( ctx, face, pname, ~0, 0 );
      for (i = 0 ; i < MAT_ATTRIB_MAX ; i++) 
	 if (bitmask & (1<<i)) {
	    ctx->ListState.ActiveMaterialSize[i] = args;
	    COPY_SZ_4V( ctx->ListState.CurrentMaterial[i], args, param );
	 }
   }

   if (ctx->ExecuteFlag) {
      (*ctx->Exec->Materialfv)( face, pname, param );
   }
}

static void GLAPIENTRY save_Begin( GLenum mode )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   GLboolean error = GL_FALSE;

   if (/*mode < GL_POINTS ||*/ mode > GL_POLYGON) {
      _mesa_compile_error( ctx, GL_INVALID_ENUM, "Begin (mode)");
      error = GL_TRUE;
   }
   else if (ctx->Driver.CurrentSavePrimitive == PRIM_UNKNOWN) {
      /* Typically the first begin.  This may raise an error on
       * playback, depending on whether CallList is issued from inside
       * a begin/end or not.
       */
      ctx->Driver.CurrentSavePrimitive = PRIM_INSIDE_UNKNOWN_PRIM;
   }
   else if (ctx->Driver.CurrentSavePrimitive == PRIM_OUTSIDE_BEGIN_END) {
      ctx->Driver.CurrentSavePrimitive = mode;
   }
   else {
      _mesa_compile_error( ctx, GL_INVALID_OPERATION, "recursive begin" );
      error = GL_TRUE;
   }

   if (!error) {
      /* Give the driver an opportunity to hook in an optimized
       * display list compiler.
       */
      if (ctx->Driver.NotifySaveBegin( ctx, mode ))
	 return;

      SAVE_FLUSH_VERTICES( ctx );
      n = ALLOC_INSTRUCTION( ctx, OPCODE_BEGIN, 1 );
      if (n) {
	 n[1].e = mode;
      }
   }

   if (ctx->ExecuteFlag) {
      (*ctx->Exec->Begin)( mode );
   }
}

static void GLAPIENTRY save_End( void )
{
   GET_CURRENT_CONTEXT(ctx);
   SAVE_FLUSH_VERTICES( ctx );
   (void) ALLOC_INSTRUCTION( ctx, OPCODE_END, 0 );
   ctx->Driver.CurrentSavePrimitive = PRIM_OUTSIDE_BEGIN_END;
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->End)( );
   }
}

static void GLAPIENTRY save_Rectf( GLfloat a, GLfloat b,
			GLfloat c, GLfloat d )
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   SAVE_FLUSH_VERTICES( ctx );
   n = ALLOC_INSTRUCTION( ctx, OPCODE_RECTF, 4 );
   if (n) {
      n[1].f = a;
      n[2].f = b;
      n[3].f = c;
      n[4].f = d;
   }
   if (ctx->ExecuteFlag) {
      (*ctx->Exec->Rectf)( a, b, c, d );
   }
}

/*
 */
static void GLAPIENTRY save_Vertex2f( GLfloat x, GLfloat y )
{
   save_Attr2f( VERT_ATTRIB_POS, x, y );
}

static void GLAPIENTRY save_Vertex2fv( const GLfloat *v )
{
   save_Attr2f( VERT_ATTRIB_POS, v[0], v[1] );
}

static void GLAPIENTRY save_Vertex3f( GLfloat x, GLfloat y, GLfloat z )
{
   save_Attr3f( VERT_ATTRIB_POS, x, y, z );
}

static void GLAPIENTRY save_Vertex3fv( const GLfloat *v )
{
   save_Attr3f( VERT_ATTRIB_POS, v[0], v[1], v[2] );
}

static void GLAPIENTRY save_Vertex4f( GLfloat x, GLfloat y, GLfloat z, GLfloat w )
{
   save_Attr4f( VERT_ATTRIB_POS, x, y, z, w );
}

static void GLAPIENTRY save_Vertex4fv( const GLfloat *v )
{
   save_Attr4f( VERT_ATTRIB_POS, v[0], v[1], v[2], v[3] );
}

static void GLAPIENTRY save_TexCoord1f( GLfloat x )
{
   save_Attr1f( VERT_ATTRIB_TEX0, x );
}

static void GLAPIENTRY save_TexCoord1fv( const GLfloat *v )
{
   save_Attr1f( VERT_ATTRIB_TEX0, v[0] );
}

static void GLAPIENTRY save_TexCoord2f( GLfloat x, GLfloat y )
{
   save_Attr2f( VERT_ATTRIB_TEX0, x, y );
}

static void GLAPIENTRY save_TexCoord2fv( const GLfloat *v )
{
   save_Attr2f( VERT_ATTRIB_TEX0, v[0], v[1] );
}

static void GLAPIENTRY save_TexCoord3f( GLfloat x, GLfloat y, GLfloat z )
{
   save_Attr3f( VERT_ATTRIB_TEX0, x, y, z );
}

static void GLAPIENTRY save_TexCoord3fv( const GLfloat *v )
{
   save_Attr3f( VERT_ATTRIB_TEX0, v[0], v[1], v[2] );
}

static void GLAPIENTRY save_TexCoord4f( GLfloat x, GLfloat y, GLfloat z, GLfloat w )
{
   save_Attr4f( VERT_ATTRIB_TEX0, x, y, z, w );
}

static void GLAPIENTRY save_TexCoord4fv( const GLfloat *v )
{
   save_Attr4f( VERT_ATTRIB_TEX0, v[0], v[1], v[2], v[3] );
}

static void GLAPIENTRY save_Normal3f( GLfloat x, GLfloat y, GLfloat z )
{
   save_Attr3f( VERT_ATTRIB_NORMAL, x, y, z );
}

static void GLAPIENTRY save_Normal3fv( const GLfloat *v )
{
   save_Attr3f( VERT_ATTRIB_NORMAL, v[0], v[1], v[2] );
}

static void GLAPIENTRY save_FogCoordfEXT( GLfloat x )
{
   save_Attr1f( VERT_ATTRIB_FOG, x );
}

static void GLAPIENTRY save_FogCoordfvEXT( const GLfloat *v )
{
   save_Attr1f( VERT_ATTRIB_FOG, v[0] );
}

static void GLAPIENTRY save_Color3f( GLfloat x, GLfloat y, GLfloat z )
{
   save_Attr3f( VERT_ATTRIB_COLOR0, x, y, z );
}

static void GLAPIENTRY save_Color3fv( const GLfloat *v )
{
   save_Attr3f( VERT_ATTRIB_COLOR0, v[0], v[1], v[2] );
}

static void GLAPIENTRY save_Color4f( GLfloat x, GLfloat y, GLfloat z, GLfloat w )
{
   save_Attr4f( VERT_ATTRIB_COLOR0, x, y, z, w );
}

static void GLAPIENTRY save_Color4fv( const GLfloat *v )
{
   save_Attr4f( VERT_ATTRIB_COLOR0, v[0], v[1], v[2], v[3] );
}

static void GLAPIENTRY save_SecondaryColor3fEXT( GLfloat x, GLfloat y, GLfloat z )
{
   save_Attr3f( VERT_ATTRIB_COLOR1, x, y, z );
}

static void GLAPIENTRY save_SecondaryColor3fvEXT( const GLfloat *v )
{
   save_Attr3f( VERT_ATTRIB_COLOR1, v[0], v[1], v[2] );
}


/* Just call the respective ATTR for texcoord
 */
static void GLAPIENTRY save_MultiTexCoord1f( GLenum target, GLfloat x  )
{
   GLuint attr = (target & 0x7) + VERT_ATTRIB_TEX0;
   save_Attr1f( attr, x );
}

static void GLAPIENTRY save_MultiTexCoord1fv( GLenum target, const GLfloat *v )
{
   GLuint attr = (target & 0x7) + VERT_ATTRIB_TEX0;
   save_Attr1f( attr, v[0] );
}

static void GLAPIENTRY save_MultiTexCoord2f( GLenum target, GLfloat x, GLfloat y )
{
   GLuint attr = (target & 0x7) + VERT_ATTRIB_TEX0;
   save_Attr2f( attr, x, y );
}

static void GLAPIENTRY save_MultiTexCoord2fv( GLenum target, const GLfloat *v )
{
   GLuint attr = (target & 0x7) + VERT_ATTRIB_TEX0;
   save_Attr2f( attr, v[0], v[1] );
}

static void GLAPIENTRY save_MultiTexCoord3f( GLenum target, GLfloat x, GLfloat y,
				    GLfloat z)
{
   GLuint attr = (target & 0x7) + VERT_ATTRIB_TEX0;
   save_Attr3f( attr, x, y, z );
}

static void GLAPIENTRY save_MultiTexCoord3fv( GLenum target, const GLfloat *v )
{
   GLuint attr = (target & 0x7) + VERT_ATTRIB_TEX0;
   save_Attr3f( attr, v[0], v[1], v[2] );
}

static void GLAPIENTRY save_MultiTexCoord4f( GLenum target, GLfloat x, GLfloat y,
				  GLfloat z, GLfloat w )
{
   GLuint attr = (target & 0x7) + VERT_ATTRIB_TEX0;
   save_Attr4f( attr, x, y, z, w );
}

static void GLAPIENTRY save_MultiTexCoord4fv( GLenum target, const GLfloat *v )
{
   GLuint attr = (target & 0x7) + VERT_ATTRIB_TEX0;
   save_Attr4f( attr, v[0], v[1], v[2], v[3] );
}


static void enum_error() 
{
   GET_CURRENT_CONTEXT( ctx );
   _mesa_error( ctx, GL_INVALID_ENUM, "VertexAttribfNV" );
}

/* First level for NV_vertex_program:
 *
 * Check for errors at compile time?.
 */
static void GLAPIENTRY save_VertexAttrib1fNV( GLuint index, GLfloat x )
{
   if (index < VERT_ATTRIB_MAX)
      save_Attr1f( index, x );
   else
      enum_error(); 
}

static void GLAPIENTRY save_VertexAttrib1fvNV( GLuint index, const GLfloat *v )
{
   if (index < VERT_ATTRIB_MAX)
      save_Attr1f( index, v[0] );
   else
      enum_error();
}

static void GLAPIENTRY save_VertexAttrib2fNV( GLuint index, GLfloat x, GLfloat y )
{
   if (index < VERT_ATTRIB_MAX)
      save_Attr2f( index, x, y );
   else
      enum_error();
}

static void GLAPIENTRY save_VertexAttrib2fvNV( GLuint index, const GLfloat *v )
{
   if (index < VERT_ATTRIB_MAX)
      save_Attr2f( index, v[0], v[1] );
   else
      enum_error();
}

static void GLAPIENTRY save_VertexAttrib3fNV( GLuint index, GLfloat x, GLfloat y, 
				   GLfloat z )
{
   if (index < VERT_ATTRIB_MAX)
      save_Attr3f( index, x, y, z );
   else
      enum_error();
}

static void GLAPIENTRY save_VertexAttrib3fvNV( GLuint index, const GLfloat *v )
{
   if (index < VERT_ATTRIB_MAX)
      save_Attr3f( index, v[0], v[1], v[2] );
   else
      enum_error();
}

static void GLAPIENTRY save_VertexAttrib4fNV( GLuint index, GLfloat x, GLfloat y,
				   GLfloat z, GLfloat w )
{
   if (index < VERT_ATTRIB_MAX)
      save_Attr4f( index, x, y, z, w );
   else
      enum_error();
}

static void GLAPIENTRY save_VertexAttrib4fvNV( GLuint index, const GLfloat *v )
{
   if (index < VERT_ATTRIB_MAX)
      save_Attr4f( index, v[0], v[1], v[2], v[3] );
   else
      enum_error();
}





/* KW: Compile commands
 *
 * Will appear in the list before the vertex buffer containing the
 * command that provoked the error.  I don't see this as a problem.
 */
void
_mesa_save_error( GLcontext *ctx, GLenum error, const char *s )
{
   Node *n;
   n = ALLOC_INSTRUCTION( ctx, OPCODE_ERROR, 2 );
   if (n) {
      n[1].e = error;
      n[2].data = (void *) s;
   }
   /* execute already done */
}


/*
 * Compile an error into current display list.
 */
void
_mesa_compile_error( GLcontext *ctx, GLenum error, const char *s )
{
   if (ctx->CompileFlag)
      _mesa_save_error( ctx, error, s );

   if (ctx->ExecuteFlag)
      _mesa_error( ctx, error, s );
}



static GLboolean
islist(GLcontext *ctx, GLuint list)
{
   if (list > 0 && _mesa_HashLookup(ctx->Shared->DisplayList, list)) {
      return GL_TRUE;
   }
   else {
      return GL_FALSE;
   }
}



/**********************************************************************/
/*                     Display list execution                         */
/**********************************************************************/


/*
 * Execute a display list.  Note that the ListBase offset must have already
 * been added before calling this function.  I.e. the list argument is
 * the absolute list number, not relative to ListBase.
 * \param list - display list number
 */
static void GLAPIENTRY
execute_list( GLcontext *ctx, GLuint list )
{
   Node *n;
   GLboolean done;

   if (list == 0 || !islist(ctx,list))
      return;

   if (ctx->Driver.BeginCallList)
      ctx->Driver.BeginCallList( ctx, list );

   ctx->ListState.CallDepth++;

   n = (Node *) _mesa_HashLookup(ctx->Shared->DisplayList, list);

   done = GL_FALSE;
   while (!done) {
      OpCode opcode = n[0].opcode;
      int i = (int)n[0].opcode - (int)OPCODE_DRV_0;

      if (i >= 0 && i < (GLint) ctx->listext.nr_opcodes) {
	 ctx->listext.opcode[i].execute(ctx, &n[1]);
	 n += ctx->listext.opcode[i].size;
      }
      else {
	 switch (opcode) {
	 case OPCODE_ERROR:
	    _mesa_error( ctx, n[1].e, (const char *) n[2].data );
	    break;
         case OPCODE_ACCUM:
	    (*ctx->Exec->Accum)( n[1].e, n[2].f );
	    break;
         case OPCODE_ALPHA_FUNC:
	    (*ctx->Exec->AlphaFunc)( n[1].e, n[2].f );
	    break;
         case OPCODE_BIND_TEXTURE:
            (*ctx->Exec->BindTexture)( n[1].e, n[2].ui );
            break;
	 case OPCODE_BITMAP:
            {
               struct gl_pixelstore_attrib save = ctx->Unpack;
               ctx->Unpack = _mesa_native_packing;
               (*ctx->Exec->Bitmap)( (GLsizei) n[1].i, (GLsizei) n[2].i,
                 n[3].f, n[4].f, n[5].f, n[6].f, (const GLubyte *) n[7].data );
               ctx->Unpack = save;  /* restore */
            }
	    break;
	 case OPCODE_BLEND_COLOR:
	    (*ctx->Exec->BlendColor)( n[1].f, n[2].f, n[3].f, n[4].f );
	    break;
	 case OPCODE_BLEND_EQUATION:
	    (*ctx->Exec->BlendEquation)( n[1].e );
	    break;
	 case OPCODE_BLEND_FUNC:
	    (*ctx->Exec->BlendFunc)( n[1].e, n[2].e );
	    break;
	 case OPCODE_BLEND_FUNC_SEPARATE:
	    (*ctx->Exec->BlendFuncSeparateEXT)(n[1].e, n[2].e, n[3].e, n[4].e);
	    break;
         case OPCODE_CALL_LIST:
	    /* Generated by glCallList(), don't add ListBase */
            if (ctx->ListState.CallDepth<MAX_LIST_NESTING) {
               execute_list( ctx, n[1].ui );
            }
            break;
         case OPCODE_CALL_LIST_OFFSET:
	    /* Generated by glCallLists() so we must add ListBase */
            if (n[2].b) {
               /* user specified a bad data type at compile time */
               _mesa_error(ctx, GL_INVALID_ENUM, "glCallLists(type)");
            }
            else if (ctx->ListState.CallDepth < MAX_LIST_NESTING) {
               execute_list( ctx, ctx->List.ListBase + n[1].ui );
            }
            break;
	 case OPCODE_CLEAR:
	    (*ctx->Exec->Clear)( n[1].bf );
	    break;
	 case OPCODE_CLEAR_COLOR:
	    (*ctx->Exec->ClearColor)( n[1].f, n[2].f, n[3].f, n[4].f );
	    break;
	 case OPCODE_CLEAR_ACCUM:
	    (*ctx->Exec->ClearAccum)( n[1].f, n[2].f, n[3].f, n[4].f );
	    break;
	 case OPCODE_CLEAR_DEPTH:
	    (*ctx->Exec->ClearDepth)( (GLclampd) n[1].f );
	    break;
	 case OPCODE_CLEAR_INDEX:
	    (*ctx->Exec->ClearIndex)( (GLfloat) n[1].ui );
	    break;
	 case OPCODE_CLEAR_STENCIL:
	    (*ctx->Exec->ClearStencil)( n[1].i );
	    break;
         case OPCODE_CLIP_PLANE:
            {
               GLdouble eq[4];
               eq[0] = n[2].f;
               eq[1] = n[3].f;
               eq[2] = n[4].f;
               eq[3] = n[5].f;
               (*ctx->Exec->ClipPlane)( n[1].e, eq );
            }
            break;
	 case OPCODE_COLOR_MASK:
	    (*ctx->Exec->ColorMask)( n[1].b, n[2].b, n[3].b, n[4].b );
	    break;
	 case OPCODE_COLOR_MATERIAL:
	    (*ctx->Exec->ColorMaterial)( n[1].e, n[2].e );
	    break;
         case OPCODE_COLOR_TABLE:
            {
               struct gl_pixelstore_attrib save = ctx->Unpack;
               ctx->Unpack = _mesa_native_packing;
               (*ctx->Exec->ColorTable)( n[1].e, n[2].e, n[3].i, n[4].e,
                                         n[5].e, n[6].data );
               ctx->Unpack = save;  /* restore */
            }
            break;
         case OPCODE_COLOR_TABLE_PARAMETER_FV:
            {
               GLfloat params[4];
               params[0] = n[3].f;
               params[1] = n[4].f;
               params[2] = n[5].f;
               params[3] = n[6].f;
               (*ctx->Exec->ColorTableParameterfv)( n[1].e, n[2].e, params );
            }
            break;
         case OPCODE_COLOR_TABLE_PARAMETER_IV:
            {
               GLint params[4];
               params[0] = n[3].i;
               params[1] = n[4].i;
               params[2] = n[5].i;
               params[3] = n[6].i;
               (*ctx->Exec->ColorTableParameteriv)( n[1].e, n[2].e, params );
            }
            break;
         case OPCODE_COLOR_SUB_TABLE:
            {
               struct gl_pixelstore_attrib save = ctx->Unpack;
               ctx->Unpack = _mesa_native_packing;
               (*ctx->Exec->ColorSubTable)( n[1].e, n[2].i, n[3].i,
                                            n[4].e, n[5].e, n[6].data );
               ctx->Unpack = save;  /* restore */
            }
            break;
         case OPCODE_CONVOLUTION_FILTER_1D:
            {
               struct gl_pixelstore_attrib save = ctx->Unpack;
               ctx->Unpack = _mesa_native_packing;
               (*ctx->Exec->ConvolutionFilter1D)( n[1].e, n[2].i, n[3].i,
                                                  n[4].e, n[5].e, n[6].data );
               ctx->Unpack = save;  /* restore */
            }
            break;
         case OPCODE_CONVOLUTION_FILTER_2D:
            {
               struct gl_pixelstore_attrib save = ctx->Unpack;
               ctx->Unpack = _mesa_native_packing;
               (*ctx->Exec->ConvolutionFilter2D)( n[1].e, n[2].i, n[3].i,
                                       n[4].i, n[5].e, n[6].e, n[7].data );
               ctx->Unpack = save;  /* restore */
            }
            break;
         case OPCODE_CONVOLUTION_PARAMETER_I:
            (*ctx->Exec->ConvolutionParameteri)( n[1].e, n[2].e, n[3].i );
            break;
         case OPCODE_CONVOLUTION_PARAMETER_IV:
            {
               GLint params[4];
               params[0] = n[3].i;
               params[1] = n[4].i;
               params[2] = n[5].i;
               params[3] = n[6].i;
               (*ctx->Exec->ConvolutionParameteriv)( n[1].e, n[2].e, params );
            }
            break;
         case OPCODE_CONVOLUTION_PARAMETER_F:
            (*ctx->Exec->ConvolutionParameterf)( n[1].e, n[2].e, n[3].f );
            break;
         case OPCODE_CONVOLUTION_PARAMETER_FV:
            {
               GLfloat params[4];
               params[0] = n[3].f;
               params[1] = n[4].f;
               params[2] = n[5].f;
               params[3] = n[6].f;
               (*ctx->Exec->ConvolutionParameterfv)( n[1].e, n[2].e, params );
            }
            break;
         case OPCODE_COPY_COLOR_SUB_TABLE:
            (*ctx->Exec->CopyColorSubTable)( n[1].e, n[2].i,
                                             n[3].i, n[4].i, n[5].i );
            break;
         case OPCODE_COPY_COLOR_TABLE:
            (*ctx->Exec->CopyColorSubTable)( n[1].e, n[2].i,
                                             n[3].i, n[4].i, n[5].i );
            break;
	 case OPCODE_COPY_PIXELS:
	    (*ctx->Exec->CopyPixels)( n[1].i, n[2].i,
			   (GLsizei) n[3].i, (GLsizei) n[4].i, n[5].e );
	    break;
         case OPCODE_COPY_TEX_IMAGE1D:
	    (*ctx->Exec->CopyTexImage1D)( n[1].e, n[2].i, n[3].e, n[4].i,
                                         n[5].i, n[6].i, n[7].i );
            break;
         case OPCODE_COPY_TEX_IMAGE2D:
	    (*ctx->Exec->CopyTexImage2D)( n[1].e, n[2].i, n[3].e, n[4].i,
                                         n[5].i, n[6].i, n[7].i, n[8].i );
            break;
         case OPCODE_COPY_TEX_SUB_IMAGE1D:
	    (*ctx->Exec->CopyTexSubImage1D)( n[1].e, n[2].i, n[3].i,
                                            n[4].i, n[5].i, n[6].i );
            break;
         case OPCODE_COPY_TEX_SUB_IMAGE2D:
	    (*ctx->Exec->CopyTexSubImage2D)( n[1].e, n[2].i, n[3].i,
                                     n[4].i, n[5].i, n[6].i, n[7].i, n[8].i );
            break;
         case OPCODE_COPY_TEX_SUB_IMAGE3D:
            (*ctx->Exec->CopyTexSubImage3D)( n[1].e, n[2].i, n[3].i,
                            n[4].i, n[5].i, n[6].i, n[7].i, n[8].i , n[9].i);
            break;
	 case OPCODE_CULL_FACE:
	    (*ctx->Exec->CullFace)( n[1].e );
	    break;
	 case OPCODE_DEPTH_FUNC:
	    (*ctx->Exec->DepthFunc)( n[1].e );
	    break;
	 case OPCODE_DEPTH_MASK:
	    (*ctx->Exec->DepthMask)( n[1].b );
	    break;
	 case OPCODE_DEPTH_RANGE:
	    (*ctx->Exec->DepthRange)( (GLclampd) n[1].f, (GLclampd) n[2].f );
	    break;
	 case OPCODE_DISABLE:
	    (*ctx->Exec->Disable)( n[1].e );
	    break;
	 case OPCODE_DRAW_BUFFER:
	    (*ctx->Exec->DrawBuffer)( n[1].e );
	    break;
	 case OPCODE_DRAW_PIXELS:
            {
               struct gl_pixelstore_attrib save = ctx->Unpack;
               ctx->Unpack = _mesa_native_packing;
               (*ctx->Exec->DrawPixels)( n[1].i, n[2].i, n[3].e, n[4].e,
                                        n[5].data );
               ctx->Unpack = save;  /* restore */
            }
	    break;
	 case OPCODE_ENABLE:
	    (*ctx->Exec->Enable)( n[1].e );
	    break;
	 case OPCODE_EVALMESH1:
	    (*ctx->Exec->EvalMesh1)( n[1].e, n[2].i, n[3].i );
	    break;
	 case OPCODE_EVALMESH2:
	    (*ctx->Exec->EvalMesh2)( n[1].e, n[2].i, n[3].i, n[4].i, n[5].i );
	    break;
	 case OPCODE_FOG:
	    {
	       GLfloat p[4];
	       p[0] = n[2].f;
	       p[1] = n[3].f;
	       p[2] = n[4].f;
	       p[3] = n[5].f;
	       (*ctx->Exec->Fogfv)( n[1].e, p );
	    }
	    break;
	 case OPCODE_FRONT_FACE:
	    (*ctx->Exec->FrontFace)( n[1].e );
	    break;
         case OPCODE_FRUSTUM:
            (*ctx->Exec->Frustum)( n[1].f, n[2].f, n[3].f, n[4].f, n[5].f, n[6].f );
            break;
	 case OPCODE_HINT:
	    (*ctx->Exec->Hint)( n[1].e, n[2].e );
	    break;
	 case OPCODE_HISTOGRAM:
	    (*ctx->Exec->Histogram)( n[1].e, n[2].i, n[3].e, n[4].b );
	    break;
	 case OPCODE_INDEX_MASK:
	    (*ctx->Exec->IndexMask)( n[1].ui );
	    break;
	 case OPCODE_INIT_NAMES:
	    (*ctx->Exec->InitNames)();
	    break;
         case OPCODE_LIGHT:
	    {
	       GLfloat p[4];
	       p[0] = n[3].f;
	       p[1] = n[4].f;
	       p[2] = n[5].f;
	       p[3] = n[6].f;
	       (*ctx->Exec->Lightfv)( n[1].e, n[2].e, p );
	    }
	    break;
         case OPCODE_LIGHT_MODEL:
	    {
	       GLfloat p[4];
	       p[0] = n[2].f;
	       p[1] = n[3].f;
	       p[2] = n[4].f;
	       p[3] = n[5].f;
	       (*ctx->Exec->LightModelfv)( n[1].e, p );
	    }
	    break;
	 case OPCODE_LINE_STIPPLE:
	    (*ctx->Exec->LineStipple)( n[1].i, n[2].us );
	    break;
	 case OPCODE_LINE_WIDTH:
	    (*ctx->Exec->LineWidth)( n[1].f );
	    break;
	 case OPCODE_LIST_BASE:
	    (*ctx->Exec->ListBase)( n[1].ui );
	    break;
	 case OPCODE_LOAD_IDENTITY:
            (*ctx->Exec->LoadIdentity)();
            break;
	 case OPCODE_LOAD_MATRIX:
	    if (sizeof(Node)==sizeof(GLfloat)) {
	       (*ctx->Exec->LoadMatrixf)( &n[1].f );
	    }
	    else {
	       GLfloat m[16];
	       GLuint i;
	       for (i=0;i<16;i++) {
		  m[i] = n[1+i].f;
	       }
	       (*ctx->Exec->LoadMatrixf)( m );
	    }
	    break;
	 case OPCODE_LOAD_NAME:
	    (*ctx->Exec->LoadName)( n[1].ui );
	    break;
	 case OPCODE_LOGIC_OP:
	    (*ctx->Exec->LogicOp)( n[1].e );
	    break;
	 case OPCODE_MAP1:
            {
               GLenum target = n[1].e;
               GLint ustride = _mesa_evaluator_components(target);
               GLint uorder = n[5].i;
               GLfloat u1 = n[2].f;
               GLfloat u2 = n[3].f;
               (*ctx->Exec->Map1f)( target, u1, u2, ustride, uorder,
                                   (GLfloat *) n[6].data );
            }
	    break;
	 case OPCODE_MAP2:
            {
               GLenum target = n[1].e;
               GLfloat u1 = n[2].f;
               GLfloat u2 = n[3].f;
               GLfloat v1 = n[4].f;
               GLfloat v2 = n[5].f;
               GLint ustride = n[6].i;
               GLint vstride = n[7].i;
               GLint uorder = n[8].i;
               GLint vorder = n[9].i;
               (*ctx->Exec->Map2f)( target, u1, u2, ustride, uorder,
                                   v1, v2, vstride, vorder,
                                   (GLfloat *) n[10].data );
            }
	    break;
	 case OPCODE_MAPGRID1:
	    (*ctx->Exec->MapGrid1f)( n[1].i, n[2].f, n[3].f );
	    break;
	 case OPCODE_MAPGRID2:
	    (*ctx->Exec->MapGrid2f)( n[1].i, n[2].f, n[3].f, n[4].i, n[5].f, n[6].f);
	    break;
         case OPCODE_MATRIX_MODE:
            (*ctx->Exec->MatrixMode)( n[1].e );
            break;
         case OPCODE_MIN_MAX:
            (*ctx->Exec->Minmax)(n[1].e, n[2].e, n[3].b);
            break;
	 case OPCODE_MULT_MATRIX:
	    if (sizeof(Node)==sizeof(GLfloat)) {
	       (*ctx->Exec->MultMatrixf)( &n[1].f );
	    }
	    else {
	       GLfloat m[16];
	       GLuint i;
	       for (i=0;i<16;i++) {
		  m[i] = n[1+i].f;
	       }
	       (*ctx->Exec->MultMatrixf)( m );
	    }
	    break;
         case OPCODE_ORTHO:
            (*ctx->Exec->Ortho)( n[1].f, n[2].f, n[3].f, n[4].f, n[5].f, n[6].f );
            break;
	 case OPCODE_PASSTHROUGH:
	    (*ctx->Exec->PassThrough)( n[1].f );
	    break;
	 case OPCODE_PIXEL_MAP:
	    (*ctx->Exec->PixelMapfv)( n[1].e, n[2].i, (GLfloat *) n[3].data );
	    break;
	 case OPCODE_PIXEL_TRANSFER:
	    (*ctx->Exec->PixelTransferf)( n[1].e, n[2].f );
	    break;
	 case OPCODE_PIXEL_ZOOM:
	    (*ctx->Exec->PixelZoom)( n[1].f, n[2].f );
	    break;
	 case OPCODE_POINT_SIZE:
	    (*ctx->Exec->PointSize)( n[1].f );
	    break;
	 case OPCODE_POINT_PARAMETERS:
	    {
		GLfloat params[3];
		params[0] = n[2].f;
		params[1] = n[3].f;
		params[2] = n[4].f;
		(*ctx->Exec->PointParameterfvEXT)( n[1].e, params );
	    }
	    break;
	 case OPCODE_POLYGON_MODE:
	    (*ctx->Exec->PolygonMode)( n[1].e, n[2].e );
	    break;
	 case OPCODE_POLYGON_STIPPLE:
	    (*ctx->Exec->PolygonStipple)( (GLubyte *) n[1].data );
	    break;
	 case OPCODE_POLYGON_OFFSET:
	    (*ctx->Exec->PolygonOffset)( n[1].f, n[2].f );
	    break;
	 case OPCODE_POP_ATTRIB:
	    (*ctx->Exec->PopAttrib)();
	    break;
	 case OPCODE_POP_MATRIX:
	    (*ctx->Exec->PopMatrix)();
	    break;
	 case OPCODE_POP_NAME:
	    (*ctx->Exec->PopName)();
	    break;
	 case OPCODE_PRIORITIZE_TEXTURE:
            (*ctx->Exec->PrioritizeTextures)( 1, &n[1].ui, &n[2].f );
	    break;
	 case OPCODE_PUSH_ATTRIB:
	    (*ctx->Exec->PushAttrib)( n[1].bf );
	    break;
	 case OPCODE_PUSH_MATRIX:
	    (*ctx->Exec->PushMatrix)();
	    break;
	 case OPCODE_PUSH_NAME:
	    (*ctx->Exec->PushName)( n[1].ui );
	    break;
	 case OPCODE_RASTER_POS:
            (*ctx->Exec->RasterPos4f)( n[1].f, n[2].f, n[3].f, n[4].f );
	    break;
	 case OPCODE_READ_BUFFER:
	    (*ctx->Exec->ReadBuffer)( n[1].e );
	    break;
         case OPCODE_RESET_HISTOGRAM:
            (*ctx->Exec->ResetHistogram)( n[1].e );
            break;
         case OPCODE_RESET_MIN_MAX:
            (*ctx->Exec->ResetMinmax)( n[1].e );
            break;
         case OPCODE_ROTATE:
            (*ctx->Exec->Rotatef)( n[1].f, n[2].f, n[3].f, n[4].f );
            break;
         case OPCODE_SCALE:
            (*ctx->Exec->Scalef)( n[1].f, n[2].f, n[3].f );
            break;
	 case OPCODE_SCISSOR:
	    (*ctx->Exec->Scissor)( n[1].i, n[2].i, n[3].i, n[4].i );
	    break;
	 case OPCODE_SHADE_MODEL:
	    (*ctx->Exec->ShadeModel)( n[1].e );
	    break;
	 case OPCODE_STENCIL_FUNC:
	    (*ctx->Exec->StencilFunc)( n[1].e, n[2].i, n[3].ui );
	    break;
	 case OPCODE_STENCIL_MASK:
	    (*ctx->Exec->StencilMask)( n[1].ui );
	    break;
	 case OPCODE_STENCIL_OP:
	    (*ctx->Exec->StencilOp)( n[1].e, n[2].e, n[3].e );
	    break;
         case OPCODE_TEXENV:
            {
               GLfloat params[4];
               params[0] = n[3].f;
               params[1] = n[4].f;
               params[2] = n[5].f;
               params[3] = n[6].f;
               (*ctx->Exec->TexEnvfv)( n[1].e, n[2].e, params );
            }
            break;
         case OPCODE_TEXGEN:
            {
               GLfloat params[4];
               params[0] = n[3].f;
               params[1] = n[4].f;
               params[2] = n[5].f;
               params[3] = n[6].f;
               (*ctx->Exec->TexGenfv)( n[1].e, n[2].e, params );
            }
            break;
         case OPCODE_TEXPARAMETER:
            {
               GLfloat params[4];
               params[0] = n[3].f;
               params[1] = n[4].f;
               params[2] = n[5].f;
               params[3] = n[6].f;
               (*ctx->Exec->TexParameterfv)( n[1].e, n[2].e, params );
            }
            break;
	 case OPCODE_TEX_IMAGE1D:
            {
               struct gl_pixelstore_attrib save = ctx->Unpack;
               ctx->Unpack = _mesa_native_packing;
               (*ctx->Exec->TexImage1D)(
                                        n[1].e, /* target */
                                        n[2].i, /* level */
                                        n[3].i, /* components */
                                        n[4].i, /* width */
                                        n[5].e, /* border */
                                        n[6].e, /* format */
                                        n[7].e, /* type */
                                        n[8].data );
               ctx->Unpack = save;  /* restore */
            }
	    break;
	 case OPCODE_TEX_IMAGE2D:
            {
               struct gl_pixelstore_attrib save = ctx->Unpack;
               ctx->Unpack = _mesa_native_packing;
               (*ctx->Exec->TexImage2D)(
                                        n[1].e, /* target */
                                        n[2].i, /* level */
                                        n[3].i, /* components */
                                        n[4].i, /* width */
                                        n[5].i, /* height */
                                        n[6].e, /* border */
                                        n[7].e, /* format */
                                        n[8].e, /* type */
                                        n[9].data );
               ctx->Unpack = save;  /* restore */
            }
	    break;
         case OPCODE_TEX_IMAGE3D:
            {
               struct gl_pixelstore_attrib save = ctx->Unpack;
               ctx->Unpack = _mesa_native_packing;
               (*ctx->Exec->TexImage3D)(
                                        n[1].e, /* target */
                                        n[2].i, /* level */
                                        n[3].i, /* components */
                                        n[4].i, /* width */
                                        n[5].i, /* height */
                                        n[6].i, /* depth  */
                                        n[7].e, /* border */
                                        n[8].e, /* format */
                                        n[9].e, /* type */
                                        n[10].data );
               ctx->Unpack = save;  /* restore */
            }
            break;
         case OPCODE_TEX_SUB_IMAGE1D:
            {
               struct gl_pixelstore_attrib save = ctx->Unpack;
               ctx->Unpack = _mesa_native_packing;
               (*ctx->Exec->TexSubImage1D)( n[1].e, n[2].i, n[3].i,
                                           n[4].i, n[5].e,
                                           n[6].e, n[7].data );
               ctx->Unpack = save;  /* restore */
            }
            break;
         case OPCODE_TEX_SUB_IMAGE2D:
            {
               struct gl_pixelstore_attrib save = ctx->Unpack;
               ctx->Unpack = _mesa_native_packing;
               (*ctx->Exec->TexSubImage2D)( n[1].e, n[2].i, n[3].i,
                                           n[4].i, n[5].e,
                                           n[6].i, n[7].e, n[8].e, n[9].data );
               ctx->Unpack = save;  /* restore */
            }
            break;
         case OPCODE_TEX_SUB_IMAGE3D:
            {
               struct gl_pixelstore_attrib save = ctx->Unpack;
               ctx->Unpack = _mesa_native_packing;
               (*ctx->Exec->TexSubImage3D)( n[1].e, n[2].i, n[3].i,
                                           n[4].i, n[5].i, n[6].i, n[7].i,
                                           n[8].i, n[9].e, n[10].e,
                                           n[11].data );
               ctx->Unpack = save;  /* restore */
            }
            break;
         case OPCODE_TRANSLATE:
            (*ctx->Exec->Translatef)( n[1].f, n[2].f, n[3].f );
            break;
	 case OPCODE_VIEWPORT:
	    (*ctx->Exec->Viewport)(n[1].i, n[2].i,
                                  (GLsizei) n[3].i, (GLsizei) n[4].i);
	    break;
	 case OPCODE_WINDOW_POS:
            (*ctx->Exec->WindowPos4fMESA)( n[1].f, n[2].f, n[3].f, n[4].f );
	    break;
         case OPCODE_ACTIVE_TEXTURE:  /* GL_ARB_multitexture */
            (*ctx->Exec->ActiveTextureARB)( n[1].e );
            break;
         case OPCODE_PIXEL_TEXGEN_SGIX:  /* GL_SGIX_pixel_texture */
            (*ctx->Exec->PixelTexGenSGIX)( n[1].e );
            break;
         case OPCODE_PIXEL_TEXGEN_PARAMETER_SGIS:  /* GL_SGIS_pixel_texture */
            (*ctx->Exec->PixelTexGenParameteriSGIS)( n[1].e, n[2].i );
            break;
         case OPCODE_COMPRESSED_TEX_IMAGE_1D: /* GL_ARB_texture_compression */
            (*ctx->Exec->CompressedTexImage1DARB)(n[1].e, n[2].i, n[3].e,
                                            n[4].i, n[5].i, n[6].i, n[7].data);
            break;
         case OPCODE_COMPRESSED_TEX_IMAGE_2D: /* GL_ARB_texture_compression */
            (*ctx->Exec->CompressedTexImage2DARB)(n[1].e, n[2].i, n[3].e,
                                    n[4].i, n[5].i, n[6].i, n[7].i, n[8].data);
            break;
         case OPCODE_COMPRESSED_TEX_IMAGE_3D: /* GL_ARB_texture_compression */
            (*ctx->Exec->CompressedTexImage3DARB)(n[1].e, n[2].i, n[3].e,
                            n[4].i, n[5].i, n[6].i, n[7].i, n[8].i, n[9].data);
            break;
         case OPCODE_COMPRESSED_TEX_SUB_IMAGE_1D: /* GL_ARB_texture_compress */
            (*ctx->Exec->CompressedTexSubImage1DARB)(n[1].e, n[2].i, n[3].i,
                                            n[4].i, n[5].e, n[6].i, n[7].data);
            break;
         case OPCODE_COMPRESSED_TEX_SUB_IMAGE_2D: /* GL_ARB_texture_compress */
            (*ctx->Exec->CompressedTexSubImage2DARB)(n[1].e, n[2].i, n[3].i,
                            n[4].i, n[5].i, n[6].i, n[7].e, n[8].i, n[9].data);
            break;
         case OPCODE_COMPRESSED_TEX_SUB_IMAGE_3D: /* GL_ARB_texture_compress */
            (*ctx->Exec->CompressedTexSubImage3DARB)(n[1].e, n[2].i, n[3].i,
                                        n[4].i, n[5].i, n[6].i, n[7].i, n[8].i,
                                        n[9].e, n[10].i, n[11].data);
            break;
         case OPCODE_SAMPLE_COVERAGE: /* GL_ARB_multisample */
            (*ctx->Exec->SampleCoverageARB)(n[1].f, n[2].b);
            break;
	 case OPCODE_WINDOW_POS_ARB: /* GL_ARB_window_pos */
            (*ctx->Exec->WindowPos3fMESA)( n[1].f, n[2].f, n[3].f );
	    break;
#if FEATURE_NV_vertex_program || FEATURE_ARB_vertex_program || FEATURE_ARB_fragment_program
         case OPCODE_BIND_PROGRAM_NV: /* GL_NV_vertex_program */
            (*ctx->Exec->BindProgramNV)( n[1].e, n[2].ui );
            break;
#endif
#if FEATURE_NV_vertex_program
         case OPCODE_EXECUTE_PROGRAM_NV:
            {
               GLfloat v[4];
               v[0] = n[3].f;
               v[1] = n[4].f;
               v[2] = n[5].f;
               v[3] = n[6].f;
               (*ctx->Exec->ExecuteProgramNV)(n[1].e, n[2].ui, v);
            }
            break;
         case OPCODE_REQUEST_RESIDENT_PROGRAMS_NV:
            (*ctx->Exec->RequestResidentProgramsNV)(n[1].ui,
                                                    (GLuint *) n[2].data);
            break;
         case OPCODE_LOAD_PROGRAM_NV:
            (*ctx->Exec->LoadProgramNV)(n[1].e, n[2].ui, n[3].i,
                                        (const GLubyte *) n[4].data);
            break;
         case OPCODE_PROGRAM_PARAMETER4F_NV:
            (*ctx->Exec->ProgramParameter4fNV)(n[1].e, n[2].ui, n[3].f,
                                               n[4].f, n[5].f, n[6].f);
            break;
         case OPCODE_TRACK_MATRIX_NV:
            (*ctx->Exec->TrackMatrixNV)(n[1].e, n[2].ui, n[3].e, n[4].e);
            break;
#endif

#if FEATURE_NV_fragment_program
         case OPCODE_PROGRAM_LOCAL_PARAMETER_ARB:
            (*ctx->Exec->ProgramLocalParameter4fARB)(n[1].e, n[2].ui, n[3].f,
                                                     n[4].f, n[5].f, n[6].f);
            break;
         case OPCODE_PROGRAM_NAMED_PARAMETER_NV:
            (*ctx->Exec->ProgramNamedParameter4fNV)(n[1].ui, n[2].i,
                                               (const GLubyte *) n[3].data,
                                               n[4].f, n[5].f, n[6].f, n[7].f);
            break;
#endif

         case OPCODE_ACTIVE_STENCIL_FACE_EXT:
            (*ctx->Exec->ActiveStencilFaceEXT)(n[1].e);
            break;
         case OPCODE_DEPTH_BOUNDS_EXT:
            (*ctx->Exec->DepthBoundsEXT)(n[1].f, n[2].f);
            break;
#if FEATURE_ARB_vertex_program || FEATURE_ARB_fragment_program
         case OPCODE_PROGRAM_STRING_ARB:
            (*ctx->Exec->ProgramStringARB)(n[1].e, n[2].e, n[3].i, n[4].data);
            break;
         case OPCODE_PROGRAM_ENV_PARAMETER_ARB:
            (*ctx->Exec->ProgramEnvParameter4fARB)(n[1].e, n[2].ui, n[3].f,
                                                   n[4].f, n[5].f, n[6].f);
            break;
#endif

	 case OPCODE_ATTR_1F:
	    (*ctx->Exec->VertexAttrib1fNV)(n[1].e, n[2].f);
	    break;
	 case OPCODE_ATTR_2F:
	    (*ctx->Exec->VertexAttrib2fvNV)(n[1].e, &n[2].f);
	    break;
	 case OPCODE_ATTR_3F:
	    (*ctx->Exec->VertexAttrib3fvNV)(n[1].e, &n[2].f);
	    break;
	 case OPCODE_ATTR_4F:
	    (*ctx->Exec->VertexAttrib4fvNV)(n[1].e, &n[2].f);
	    break;
	 case OPCODE_MATERIAL:
	    (*ctx->Exec->Materialfv)(n[1].e, n[2].e, &n[3].f);
	    break;
	 case OPCODE_INDEX:
	    (*ctx->Exec->Indexi)(n[1].i);
	    break;
	 case OPCODE_EDGEFLAG:
	    (*ctx->Exec->EdgeFlag)(n[1].b);
	    break;
	 case OPCODE_BEGIN:
	    (*ctx->Exec->Begin)(n[1].e);
	    break;
	 case OPCODE_END:
	    (*ctx->Exec->End)();
	    break;
	 case OPCODE_RECTF:
	    (*ctx->Exec->Rectf)(n[1].f, n[2].f, n[3].f, n[4].f);
	    break;
	 case OPCODE_EVAL_C1:
	    (*ctx->Exec->EvalCoord1f)(n[1].f);
	    break;
	 case OPCODE_EVAL_C2:
	    (*ctx->Exec->EvalCoord2fv)(&n[1].f);
	    break;
	 case OPCODE_EVAL_P1:
	    (*ctx->Exec->EvalPoint1)(n[1].i);
	    break;
	 case OPCODE_EVAL_P2:
	    (*ctx->Exec->EvalPoint2)(n[1].i, n[2].i);
	    break;




	 case OPCODE_CONTINUE:
	    n = (Node *) n[1].next;
	    break;
	 case OPCODE_END_OF_LIST:
	    done = GL_TRUE;
	    break;
	 default:
            {
               char msg[1000];
               _mesa_sprintf(msg, "Error in execute_list: opcode=%d", (int) opcode);
               _mesa_problem(ctx, msg);
            }
            done = GL_TRUE;
	 }

	 /* increment n to point to next compiled command */
	 if (opcode!=OPCODE_CONTINUE) {
	    n += InstSize[opcode];
	 }
      }
   }
   ctx->ListState.CallDepth--;

   if (ctx->Driver.EndCallList)
      ctx->Driver.EndCallList( ctx );
}





/**********************************************************************/
/*                           GL functions                             */
/**********************************************************************/




/*
 * Test if a display list number is valid.
 */
GLboolean GLAPIENTRY
_mesa_IsList( GLuint list )
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);	/* must be called before assert */
   ASSERT_OUTSIDE_BEGIN_END_WITH_RETVAL(ctx, GL_FALSE);
   return islist(ctx, list);
}


/*
 * Delete a sequence of consecutive display lists.
 */
void GLAPIENTRY
_mesa_DeleteLists( GLuint list, GLsizei range )
{
   GET_CURRENT_CONTEXT(ctx);
   GLuint i;
   FLUSH_VERTICES(ctx, 0);	/* must be called before assert */
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (range<0) {
      _mesa_error( ctx, GL_INVALID_VALUE, "glDeleteLists" );
      return;
   }
   for (i=list;i<list+range;i++) {
      _mesa_destroy_list( ctx, i );
   }
}



/*
 * Return a display list number, n, such that lists n through n+range-1
 * are free.
 */
GLuint GLAPIENTRY
_mesa_GenLists(GLsizei range )
{
   GET_CURRENT_CONTEXT(ctx);
   GLuint base;
   FLUSH_VERTICES(ctx, 0);	/* must be called before assert */
   ASSERT_OUTSIDE_BEGIN_END_WITH_RETVAL(ctx, 0);

   if (range<0) {
      _mesa_error( ctx, GL_INVALID_VALUE, "glGenLists" );
      return 0;
   }
   if (range==0) {
      return 0;
   }

   /*
    * Make this an atomic operation
    */
   _glthread_LOCK_MUTEX(ctx->Shared->Mutex);

   base = _mesa_HashFindFreeKeyBlock(ctx->Shared->DisplayList, range);
   if (base) {
      /* reserve the list IDs by with empty/dummy lists */
      GLint i;
      for (i=0; i<range; i++) {
         _mesa_HashInsert(ctx->Shared->DisplayList, base+i, make_empty_list());
      }
   }

   _glthread_UNLOCK_MUTEX(ctx->Shared->Mutex);

   return base;
}



/*
 * Begin a new display list.
 */
void GLAPIENTRY
_mesa_NewList( GLuint list, GLenum mode )
{
   GET_CURRENT_CONTEXT(ctx);
   GLint i;

   FLUSH_CURRENT(ctx, 0);	/* must be called before assert */
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (MESA_VERBOSE&VERBOSE_API)
      _mesa_debug(ctx, "glNewList %u %s\n", list,
                  _mesa_lookup_enum_by_nr(mode));

   if (list==0) {
      _mesa_error( ctx, GL_INVALID_VALUE, "glNewList" );
      return;
   }

   if (mode!=GL_COMPILE && mode!=GL_COMPILE_AND_EXECUTE) {
      _mesa_error( ctx, GL_INVALID_ENUM, "glNewList" );
      return;
   }

   if (ctx->ListState.CurrentListPtr) {
      /* already compiling a display list */
      _mesa_error( ctx, GL_INVALID_OPERATION, "glNewList" );
      return;
   }

   ctx->CompileFlag = GL_TRUE;
   ctx->ExecuteFlag = (mode == GL_COMPILE_AND_EXECUTE);

   /* Allocate new display list */
   ctx->ListState.CurrentListNum = list;
   ctx->ListState.CurrentBlock = (Node *) CALLOC( sizeof(Node) * BLOCK_SIZE );
   ctx->ListState.CurrentListPtr = ctx->ListState.CurrentBlock;
   ctx->ListState.CurrentPos = 0;

   /* Reset acumulated list state:
    */
   for (i = 0; i < VERT_ATTRIB_MAX; i++)
      ctx->ListState.ActiveAttribSize[i] = 0;

   for (i = 0; i < MAT_ATTRIB_MAX; i++)
      ctx->ListState.ActiveMaterialSize[i] = 0;
      
   ctx->ListState.ActiveIndex = 0;
   ctx->ListState.ActiveEdgeFlag = 0;

   ctx->Driver.CurrentSavePrimitive = PRIM_UNKNOWN;
   ctx->Driver.NewList( ctx, list, mode );

   ctx->CurrentDispatch = ctx->Save;
   _glapi_set_dispatch( ctx->CurrentDispatch );
}



/*
 * End definition of current display list. 
 */
void GLAPIENTRY
_mesa_EndList( void )
{
   GET_CURRENT_CONTEXT(ctx);
   SAVE_FLUSH_VERTICES(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   if (MESA_VERBOSE&VERBOSE_API)
      _mesa_debug(ctx, "glEndList\n");

   /* Check that a list is under construction */
   if (!ctx->ListState.CurrentListPtr) {
      _mesa_error( ctx, GL_INVALID_OPERATION, "glEndList" );
      return;
   }

   (void) ALLOC_INSTRUCTION( ctx, OPCODE_END_OF_LIST, 0 );

   /* Destroy old list, if any */
   _mesa_destroy_list(ctx, ctx->ListState.CurrentListNum);
   /* Install the list */
   _mesa_HashInsert(ctx->Shared->DisplayList, ctx->ListState.CurrentListNum, ctx->ListState.CurrentListPtr);


   if (MESA_VERBOSE & VERBOSE_DISPLAY_LIST)
      mesa_print_display_list(ctx->ListState.CurrentListNum);

   ctx->ListState.CurrentListNum = 0;
   ctx->ListState.CurrentListPtr = NULL;
   ctx->ExecuteFlag = GL_TRUE;
   ctx->CompileFlag = GL_FALSE;

   ctx->Driver.EndList( ctx );

   ctx->CurrentDispatch = ctx->Exec;
   _glapi_set_dispatch( ctx->CurrentDispatch );
}



void GLAPIENTRY
_mesa_CallList( GLuint list )
{
   GLboolean save_compile_flag;
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_CURRENT(ctx, 0);
   /* VERY IMPORTANT:  Save the CompileFlag status, turn it off, */
   /* execute the display list, and restore the CompileFlag. */

   if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx, "glCallList %d\n", list);

   if (list == 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glCallList(list==0)");
      return;
   }

/*     mesa_print_display_list( list ); */

   save_compile_flag = ctx->CompileFlag;
   if (save_compile_flag) {
      ctx->CompileFlag = GL_FALSE;
   }

   execute_list( ctx, list );
   ctx->CompileFlag = save_compile_flag;

   /* also restore API function pointers to point to "save" versions */
   if (save_compile_flag) {
      ctx->CurrentDispatch = ctx->Save;
      _glapi_set_dispatch( ctx->CurrentDispatch );
   }
}



/*
 * Execute glCallLists:  call multiple display lists.
 */
void GLAPIENTRY
_mesa_CallLists( GLsizei n, GLenum type, const GLvoid *lists )
{
   GET_CURRENT_CONTEXT(ctx);
   GLuint list;
   GLint i;
   GLboolean save_compile_flag;

   if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx, "glCallLists %d\n", n);

   switch (type) {
      case GL_BYTE:
      case GL_UNSIGNED_BYTE:
      case GL_SHORT:
      case GL_UNSIGNED_SHORT:
      case GL_INT:
      case GL_UNSIGNED_INT:
      case GL_FLOAT:
      case GL_2_BYTES:
      case GL_3_BYTES:
      case GL_4_BYTES:
         /* OK */
         break;
      default:
         _mesa_error(ctx, GL_INVALID_ENUM, "glCallLists(type)");
         return;
   }

   /* Save the CompileFlag status, turn it off, execute display list,
    * and restore the CompileFlag.
    */
   save_compile_flag = ctx->CompileFlag;
   ctx->CompileFlag = GL_FALSE;

   for (i=0;i<n;i++) {
      list = translate_id( i, type, lists );
      execute_list( ctx, ctx->List.ListBase + list );
   }

   ctx->CompileFlag = save_compile_flag;

   /* also restore API function pointers to point to "save" versions */
   if (save_compile_flag) {
      ctx->CurrentDispatch = ctx->Save;
      _glapi_set_dispatch( ctx->CurrentDispatch );
   }
}



/*
 * Set the offset added to list numbers in glCallLists.
 */
void GLAPIENTRY
_mesa_ListBase( GLuint base )
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);	/* must be called before assert */
   ASSERT_OUTSIDE_BEGIN_END(ctx);
   ctx->List.ListBase = base;
}


/* Can no longer assume ctx->Exec->Func is equal to _mesa_Func.
 */
static void GLAPIENTRY exec_Finish( void )
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   ctx->Exec->Finish();
}

static void GLAPIENTRY exec_Flush( void )
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   ctx->Exec->Flush( );
}

static void GLAPIENTRY exec_GetBooleanv( GLenum pname, GLboolean *params )
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   ctx->Exec->GetBooleanv( pname, params );
}

static void GLAPIENTRY exec_GetClipPlane( GLenum plane, GLdouble *equation )
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   ctx->Exec->GetClipPlane( plane, equation );
}

static void GLAPIENTRY exec_GetDoublev( GLenum pname, GLdouble *params )
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   ctx->Exec->GetDoublev(  pname, params );
}

static GLenum GLAPIENTRY exec_GetError( void )
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   return ctx->Exec->GetError( );
}

static void GLAPIENTRY exec_GetFloatv( GLenum pname, GLfloat *params )
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   ctx->Exec->GetFloatv( pname, params );
}

static void GLAPIENTRY exec_GetIntegerv( GLenum pname, GLint *params )
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   ctx->Exec->GetIntegerv( pname, params );
}

static void GLAPIENTRY exec_GetLightfv( GLenum light, GLenum pname, GLfloat *params )
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   ctx->Exec->GetLightfv( light, pname, params );
}

static void GLAPIENTRY exec_GetLightiv( GLenum light, GLenum pname, GLint *params )
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   ctx->Exec->GetLightiv( light, pname, params );
}

static void GLAPIENTRY exec_GetMapdv( GLenum target, GLenum query, GLdouble *v )
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   ctx->Exec->GetMapdv( target, query, v );
}

static void GLAPIENTRY exec_GetMapfv( GLenum target, GLenum query, GLfloat *v )
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   ctx->Exec->GetMapfv( target, query, v );
}

static void GLAPIENTRY exec_GetMapiv( GLenum target, GLenum query, GLint *v )
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   ctx->Exec->GetMapiv( target, query, v );
}

static void GLAPIENTRY exec_GetMaterialfv( GLenum face, GLenum pname, GLfloat *params )
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   ctx->Exec->GetMaterialfv( face, pname, params );
}

static void GLAPIENTRY exec_GetMaterialiv( GLenum face, GLenum pname, GLint *params )
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   ctx->Exec->GetMaterialiv( face, pname, params );
}

static void GLAPIENTRY exec_GetPixelMapfv( GLenum map, GLfloat *values )
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   ctx->Exec->GetPixelMapfv( map,  values );
}

static void GLAPIENTRY exec_GetPixelMapuiv( GLenum map, GLuint *values )
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   ctx->Exec->GetPixelMapuiv( map, values );
}

static void GLAPIENTRY exec_GetPixelMapusv( GLenum map, GLushort *values )
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   ctx->Exec->GetPixelMapusv( map, values );
}

static void GLAPIENTRY exec_GetPolygonStipple( GLubyte *dest )
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   ctx->Exec->GetPolygonStipple( dest );
}

static const GLubyte * GLAPIENTRY exec_GetString( GLenum name )
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   return ctx->Exec->GetString( name );
}

static void GLAPIENTRY exec_GetTexEnvfv( GLenum target, GLenum pname, GLfloat *params )
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   ctx->Exec->GetTexEnvfv( target, pname, params );
}

static void GLAPIENTRY exec_GetTexEnviv( GLenum target, GLenum pname, GLint *params )
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   ctx->Exec->GetTexEnviv( target, pname, params );
}

static void GLAPIENTRY exec_GetTexGendv( GLenum coord, GLenum pname, GLdouble *params )
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   ctx->Exec->GetTexGendv( coord, pname, params );
}

static void GLAPIENTRY exec_GetTexGenfv( GLenum coord, GLenum pname, GLfloat *params )
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   ctx->Exec->GetTexGenfv( coord, pname, params );
}

static void GLAPIENTRY exec_GetTexGeniv( GLenum coord, GLenum pname, GLint *params )
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   ctx->Exec->GetTexGeniv( coord, pname, params );
}

static void GLAPIENTRY exec_GetTexImage( GLenum target, GLint level, GLenum format,
                   GLenum type, GLvoid *pixels )
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   ctx->Exec->GetTexImage( target, level, format, type, pixels );
}

static void GLAPIENTRY exec_GetTexLevelParameterfv( GLenum target, GLint level,
                              GLenum pname, GLfloat *params )
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   ctx->Exec->GetTexLevelParameterfv( target, level, pname, params );
}

static void GLAPIENTRY exec_GetTexLevelParameteriv( GLenum target, GLint level,
                              GLenum pname, GLint *params )
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   ctx->Exec->GetTexLevelParameteriv( target, level, pname, params );
}

static void GLAPIENTRY exec_GetTexParameterfv( GLenum target, GLenum pname,
				    GLfloat *params )
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   ctx->Exec->GetTexParameterfv( target, pname, params );
}

static void GLAPIENTRY exec_GetTexParameteriv( GLenum target, GLenum pname, GLint *params )
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   ctx->Exec->GetTexParameteriv( target, pname, params );
}

static GLboolean GLAPIENTRY exec_IsEnabled( GLenum cap )
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   return ctx->Exec->IsEnabled( cap );
}

static void GLAPIENTRY exec_PixelStoref( GLenum pname, GLfloat param )
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   ctx->Exec->PixelStoref( pname, param );
}

static void GLAPIENTRY exec_PixelStorei( GLenum pname, GLint param )
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   ctx->Exec->PixelStorei( pname, param );
}

static void GLAPIENTRY exec_ReadPixels( GLint x, GLint y, GLsizei width, GLsizei height,
		  GLenum format, GLenum type, GLvoid *pixels )
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   ctx->Exec->ReadPixels( x, y, width, height, format, type, pixels );
}

static GLint GLAPIENTRY exec_RenderMode( GLenum mode )
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   return ctx->Exec->RenderMode( mode );
}

static void GLAPIENTRY exec_FeedbackBuffer( GLsizei size, GLenum type, GLfloat *buffer )
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   ctx->Exec->FeedbackBuffer( size, type, buffer );
}

static void GLAPIENTRY exec_SelectBuffer( GLsizei size, GLuint *buffer )
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   ctx->Exec->SelectBuffer( size, buffer );
}

static GLboolean GLAPIENTRY exec_AreTexturesResident(GLsizei n, const GLuint *texName,
					  GLboolean *residences)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   return ctx->Exec->AreTexturesResident( n, texName, residences);
}

static void GLAPIENTRY exec_ColorPointer(GLint size, GLenum type, GLsizei stride,
			      const GLvoid *ptr)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   ctx->Exec->ColorPointer( size, type, stride, ptr);
}

static void GLAPIENTRY exec_DeleteTextures( GLsizei n, const GLuint *texName)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   ctx->Exec->DeleteTextures( n, texName);
}

static void GLAPIENTRY exec_DisableClientState( GLenum cap )
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   ctx->Exec->DisableClientState( cap );
}

static void GLAPIENTRY exec_EdgeFlagPointer(GLsizei stride, const GLvoid *vptr)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   ctx->Exec->EdgeFlagPointer( stride, vptr);
}

static void GLAPIENTRY exec_EnableClientState( GLenum cap )
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   ctx->Exec->EnableClientState( cap );
}

static void GLAPIENTRY exec_GenTextures( GLsizei n, GLuint *texName )
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   ctx->Exec->GenTextures( n, texName );
}

static void GLAPIENTRY exec_GetPointerv( GLenum pname, GLvoid **params )
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   ctx->Exec->GetPointerv( pname, params );
}

static void GLAPIENTRY exec_IndexPointer(GLenum type, GLsizei stride, const GLvoid *ptr)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   ctx->Exec->IndexPointer( type, stride, ptr);
}

static void GLAPIENTRY exec_InterleavedArrays(GLenum format, GLsizei stride,
				   const GLvoid *pointer)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   ctx->Exec->InterleavedArrays( format, stride, pointer);
}

static GLboolean GLAPIENTRY exec_IsTexture( GLuint texture )
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   return ctx->Exec->IsTexture( texture );
}

static void GLAPIENTRY exec_NormalPointer(GLenum type, GLsizei stride, const GLvoid *ptr )
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   ctx->Exec->NormalPointer( type, stride, ptr );
}

static void GLAPIENTRY exec_PopClientAttrib(void)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   ctx->Exec->PopClientAttrib();
}

static void GLAPIENTRY exec_PushClientAttrib(GLbitfield mask)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   ctx->Exec->PushClientAttrib( mask);
}

static void GLAPIENTRY exec_TexCoordPointer(GLint size, GLenum type, GLsizei stride,
				 const GLvoid *ptr)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   ctx->Exec->TexCoordPointer( size,  type,  stride, ptr);
}

static void GLAPIENTRY exec_GetCompressedTexImageARB(GLenum target, GLint level,
					  GLvoid *img)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   ctx->Exec->GetCompressedTexImageARB( target, level, img);
}

static void GLAPIENTRY exec_VertexPointer(GLint size, GLenum type, GLsizei stride,
			       const GLvoid *ptr)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   ctx->Exec->VertexPointer( size, type, stride, ptr);
}

static void GLAPIENTRY exec_CopyConvolutionFilter1D(GLenum target, GLenum internalFormat,
					 GLint x, GLint y, GLsizei width)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   ctx->Exec->CopyConvolutionFilter1D( target, internalFormat, x, y, width);
}

static void GLAPIENTRY exec_CopyConvolutionFilter2D(GLenum target, GLenum internalFormat,
					 GLint x, GLint y, GLsizei width,
					 GLsizei height)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   ctx->Exec->CopyConvolutionFilter2D( target, internalFormat, x, y, width,
				       height);
}

static void GLAPIENTRY exec_GetColorTable( GLenum target, GLenum format,
				GLenum type, GLvoid *data )
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   ctx->Exec->GetColorTable( target, format, type, data );
}

static void GLAPIENTRY exec_GetColorTableParameterfv( GLenum target, GLenum pname,
					   GLfloat *params )
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   ctx->Exec->GetColorTableParameterfv( target, pname, params );
}

static void GLAPIENTRY exec_GetColorTableParameteriv( GLenum target, GLenum pname,
					   GLint *params )
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   ctx->Exec->GetColorTableParameteriv( target, pname, params );
}

static void GLAPIENTRY exec_GetConvolutionFilter(GLenum target, GLenum format, GLenum type,
				      GLvoid *image)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   ctx->Exec->GetConvolutionFilter( target, format, type, image);
}

static void GLAPIENTRY exec_GetConvolutionParameterfv(GLenum target, GLenum pname,
					   GLfloat *params)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   ctx->Exec->GetConvolutionParameterfv( target, pname, params);
}

static void GLAPIENTRY exec_GetConvolutionParameteriv(GLenum target, GLenum pname,
					   GLint *params)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   ctx->Exec->GetConvolutionParameteriv( target, pname, params);
}

static void GLAPIENTRY exec_GetHistogram(GLenum target, GLboolean reset, GLenum format,
			      GLenum type, GLvoid *values)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   ctx->Exec->GetHistogram( target, reset, format, type, values);
}

static void GLAPIENTRY exec_GetHistogramParameterfv(GLenum target, GLenum pname,
					 GLfloat *params)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   ctx->Exec->GetHistogramParameterfv( target, pname, params);
}

static void GLAPIENTRY exec_GetHistogramParameteriv(GLenum target, GLenum pname,
					 GLint *params)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   ctx->Exec->GetHistogramParameteriv( target, pname, params);
}

static void GLAPIENTRY exec_GetMinmax(GLenum target, GLboolean reset, GLenum format,
			   GLenum type, GLvoid *values)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   ctx->Exec->GetMinmax( target, reset, format, type, values);
}

static void GLAPIENTRY exec_GetMinmaxParameterfv(GLenum target, GLenum pname,
				      GLfloat *params)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   ctx->Exec->GetMinmaxParameterfv( target, pname, params);
}

static void GLAPIENTRY exec_GetMinmaxParameteriv(GLenum target, GLenum pname,
				      GLint *params)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   ctx->Exec->GetMinmaxParameteriv( target, pname, params);
}

static void GLAPIENTRY exec_GetSeparableFilter(GLenum target, GLenum format, GLenum type,
				    GLvoid *row, GLvoid *column, GLvoid *span)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   ctx->Exec->GetSeparableFilter( target, format, type, row, column, span);
}

static void GLAPIENTRY exec_SeparableFilter2D(GLenum target, GLenum internalFormat,
				   GLsizei width, GLsizei height, GLenum format,
				   GLenum type, const GLvoid *row,
				   const GLvoid *column)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   ctx->Exec->SeparableFilter2D( target, internalFormat, width, height, format,
				 type, row, column);
}

static void GLAPIENTRY exec_GetPixelTexGenParameterivSGIS(GLenum target, GLint *value)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   ctx->Exec->GetPixelTexGenParameterivSGIS( target, value);
}

static void GLAPIENTRY exec_GetPixelTexGenParameterfvSGIS(GLenum target, GLfloat *value)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   ctx->Exec->GetPixelTexGenParameterfvSGIS( target, value);
}

static void GLAPIENTRY exec_ColorPointerEXT(GLint size, GLenum type, GLsizei stride,
				 GLsizei count, const GLvoid *ptr)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   ctx->Exec->ColorPointerEXT( size, type, stride, count, ptr);
}

static void GLAPIENTRY exec_EdgeFlagPointerEXT(GLsizei stride, GLsizei count,
				    const GLboolean *ptr)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   ctx->Exec->EdgeFlagPointerEXT( stride, count, ptr);
}

static void GLAPIENTRY exec_IndexPointerEXT(GLenum type, GLsizei stride, GLsizei count,
                      const GLvoid *ptr)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   ctx->Exec->IndexPointerEXT( type, stride, count, ptr);
}

static void GLAPIENTRY exec_NormalPointerEXT(GLenum type, GLsizei stride, GLsizei count,
                       const GLvoid *ptr)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   ctx->Exec->NormalPointerEXT( type, stride, count, ptr);
}

static void GLAPIENTRY exec_TexCoordPointerEXT(GLint size, GLenum type, GLsizei stride,
				    GLsizei count, const GLvoid *ptr)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   ctx->Exec->TexCoordPointerEXT( size, type, stride, count, ptr);
}

static void GLAPIENTRY exec_VertexPointerEXT(GLint size, GLenum type, GLsizei stride,
                       GLsizei count, const GLvoid *ptr)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   ctx->Exec->VertexPointerEXT( size, type, stride, count, ptr);
}

static void GLAPIENTRY exec_LockArraysEXT(GLint first, GLsizei count)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   ctx->Exec->LockArraysEXT( first, count);
}

static void GLAPIENTRY exec_UnlockArraysEXT( void )
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   ctx->Exec->UnlockArraysEXT( );
}

static void GLAPIENTRY exec_ResizeBuffersMESA( void )
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   ctx->Exec->ResizeBuffersMESA( );
}


static void GLAPIENTRY exec_ClientActiveTextureARB( GLenum target )
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   ctx->Exec->ClientActiveTextureARB(target);
}

static void GLAPIENTRY exec_SecondaryColorPointerEXT(GLint size, GLenum type,
			       GLsizei stride, const GLvoid *ptr)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   ctx->Exec->SecondaryColorPointerEXT( size, type, stride, ptr);
}

static void GLAPIENTRY exec_FogCoordPointerEXT(GLenum type, GLsizei stride,
				    const GLvoid *ptr)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   ctx->Exec->FogCoordPointerEXT( type, stride, ptr);
}

/* GL_EXT_multi_draw_arrays */
static void GLAPIENTRY exec_MultiDrawArraysEXT(GLenum mode, GLint *first,
                                    GLsizei *count, GLsizei primcount)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   ctx->Exec->MultiDrawArraysEXT( mode, first, count, primcount );
}

/* GL_EXT_multi_draw_arrays */
static void GLAPIENTRY exec_MultiDrawElementsEXT(GLenum mode, const GLsizei *count,
                                      GLenum type, const GLvoid **indices,
                                      GLsizei primcount)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   ctx->Exec->MultiDrawElementsEXT(mode, count, type, indices, primcount);
}

/* GL_IBM_multimode_draw_arrays */
static void GLAPIENTRY exec_MultiModeDrawArraysIBM(const GLenum *mode, const GLint *first,
					const GLsizei *count, GLsizei primcount,
					GLint modestride)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   ctx->Exec->MultiModeDrawArraysIBM(mode, first, count, primcount, modestride);
}

/* GL_IBM_multimode_draw_arrays */
static void GLAPIENTRY exec_MultiModeDrawElementsIBM(const GLenum *mode,
					  const GLsizei *count,
					  GLenum type,
					  const GLvoid * const *indices,
					  GLsizei primcount, GLint modestride)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   ctx->Exec->MultiModeDrawElementsIBM(mode, count, type, indices, primcount,
				       modestride);
}




/**
 * Setup the given dispatch table to point to Mesa's display list
 * building functions.
 *
 * This does not include any of the tnl functions - they are
 * initialized from _mesa_init_api_defaults and from the active vtxfmt
 * struct.
 */
void
_mesa_init_dlist_table( struct _glapi_table *table, GLuint tableSize )
{
   _mesa_init_no_op_table(table, tableSize);

   _mesa_loopback_init_api_table( table );

   /* GL 1.0 */
   table->Accum = save_Accum;
   table->AlphaFunc = save_AlphaFunc;
   table->Bitmap = save_Bitmap;
   table->BlendFunc = save_BlendFunc;
   table->CallList = _mesa_save_CallList;
   table->CallLists = _mesa_save_CallLists;
   table->Clear = save_Clear;
   table->ClearAccum = save_ClearAccum;
   table->ClearColor = save_ClearColor;
   table->ClearDepth = save_ClearDepth;
   table->ClearIndex = save_ClearIndex;
   table->ClearStencil = save_ClearStencil;
   table->ClipPlane = save_ClipPlane;
   table->ColorMask = save_ColorMask;
   table->ColorMaterial = save_ColorMaterial;
   table->CopyPixels = save_CopyPixels;
   table->CullFace = save_CullFace;
   table->DeleteLists = _mesa_DeleteLists;
   table->DepthFunc = save_DepthFunc;
   table->DepthMask = save_DepthMask;
   table->DepthRange = save_DepthRange;
   table->Disable = save_Disable;
   table->DrawBuffer = save_DrawBuffer;
   table->DrawPixels = save_DrawPixels;
   table->Enable = save_Enable;
   table->EndList = _mesa_EndList;
   table->EvalMesh1 = _mesa_save_EvalMesh1;
   table->EvalMesh2 = _mesa_save_EvalMesh2;
   table->Finish = exec_Finish;
   table->Flush = exec_Flush;
   table->Fogf = save_Fogf;
   table->Fogfv = save_Fogfv;
   table->Fogi = save_Fogi;
   table->Fogiv = save_Fogiv;
   table->FrontFace = save_FrontFace;
   table->Frustum = save_Frustum;
   table->GenLists = _mesa_GenLists;
   table->GetBooleanv = exec_GetBooleanv;
   table->GetClipPlane = exec_GetClipPlane;
   table->GetDoublev = exec_GetDoublev;
   table->GetError = exec_GetError;
   table->GetFloatv = exec_GetFloatv;
   table->GetIntegerv = exec_GetIntegerv;
   table->GetLightfv = exec_GetLightfv;
   table->GetLightiv = exec_GetLightiv;
   table->GetMapdv = exec_GetMapdv;
   table->GetMapfv = exec_GetMapfv;
   table->GetMapiv = exec_GetMapiv;
   table->GetMaterialfv = exec_GetMaterialfv;
   table->GetMaterialiv = exec_GetMaterialiv;
   table->GetPixelMapfv = exec_GetPixelMapfv;
   table->GetPixelMapuiv = exec_GetPixelMapuiv;
   table->GetPixelMapusv = exec_GetPixelMapusv;
   table->GetPolygonStipple = exec_GetPolygonStipple;
   table->GetString = exec_GetString;
   table->GetTexEnvfv = exec_GetTexEnvfv;
   table->GetTexEnviv = exec_GetTexEnviv;
   table->GetTexGendv = exec_GetTexGendv;
   table->GetTexGenfv = exec_GetTexGenfv;
   table->GetTexGeniv = exec_GetTexGeniv;
   table->GetTexImage = exec_GetTexImage;
   table->GetTexLevelParameterfv = exec_GetTexLevelParameterfv;
   table->GetTexLevelParameteriv = exec_GetTexLevelParameteriv;
   table->GetTexParameterfv = exec_GetTexParameterfv;
   table->GetTexParameteriv = exec_GetTexParameteriv;
   table->Hint = save_Hint;
   table->IndexMask = save_IndexMask;
   table->InitNames = save_InitNames;
   table->IsEnabled = exec_IsEnabled;
   table->IsList = _mesa_IsList;
   table->LightModelf = save_LightModelf;
   table->LightModelfv = save_LightModelfv;
   table->LightModeli = save_LightModeli;
   table->LightModeliv = save_LightModeliv;
   table->Lightf = save_Lightf;
   table->Lightfv = save_Lightfv;
   table->Lighti = save_Lighti;
   table->Lightiv = save_Lightiv;
   table->LineStipple = save_LineStipple;
   table->LineWidth = save_LineWidth;
   table->ListBase = save_ListBase;
   table->LoadIdentity = save_LoadIdentity;
   table->LoadMatrixd = save_LoadMatrixd;
   table->LoadMatrixf = save_LoadMatrixf;
   table->LoadName = save_LoadName;
   table->LogicOp = save_LogicOp;
   table->Map1d = save_Map1d;
   table->Map1f = save_Map1f;
   table->Map2d = save_Map2d;
   table->Map2f = save_Map2f;
   table->MapGrid1d = save_MapGrid1d;
   table->MapGrid1f = save_MapGrid1f;
   table->MapGrid2d = save_MapGrid2d;
   table->MapGrid2f = save_MapGrid2f;
   table->MatrixMode = save_MatrixMode;
   table->MultMatrixd = save_MultMatrixd;
   table->MultMatrixf = save_MultMatrixf;
   table->NewList = save_NewList;
   table->Ortho = save_Ortho;
   table->PassThrough = save_PassThrough;
   table->PixelMapfv = save_PixelMapfv;
   table->PixelMapuiv = save_PixelMapuiv;
   table->PixelMapusv = save_PixelMapusv;
   table->PixelStoref = exec_PixelStoref;
   table->PixelStorei = exec_PixelStorei;
   table->PixelTransferf = save_PixelTransferf;
   table->PixelTransferi = save_PixelTransferi;
   table->PixelZoom = save_PixelZoom;
   table->PointSize = save_PointSize;
   table->PolygonMode = save_PolygonMode;
   table->PolygonOffset = save_PolygonOffset;
   table->PolygonStipple = save_PolygonStipple;
   table->PopAttrib = save_PopAttrib;
   table->PopMatrix = save_PopMatrix;
   table->PopName = save_PopName;
   table->PushAttrib = save_PushAttrib;
   table->PushMatrix = save_PushMatrix;
   table->PushName = save_PushName;
   table->RasterPos2d = save_RasterPos2d;
   table->RasterPos2dv = save_RasterPos2dv;
   table->RasterPos2f = save_RasterPos2f;
   table->RasterPos2fv = save_RasterPos2fv;
   table->RasterPos2i = save_RasterPos2i;
   table->RasterPos2iv = save_RasterPos2iv;
   table->RasterPos2s = save_RasterPos2s;
   table->RasterPos2sv = save_RasterPos2sv;
   table->RasterPos3d = save_RasterPos3d;
   table->RasterPos3dv = save_RasterPos3dv;
   table->RasterPos3f = save_RasterPos3f;
   table->RasterPos3fv = save_RasterPos3fv;
   table->RasterPos3i = save_RasterPos3i;
   table->RasterPos3iv = save_RasterPos3iv;
   table->RasterPos3s = save_RasterPos3s;
   table->RasterPos3sv = save_RasterPos3sv;
   table->RasterPos4d = save_RasterPos4d;
   table->RasterPos4dv = save_RasterPos4dv;
   table->RasterPos4f = save_RasterPos4f;
   table->RasterPos4fv = save_RasterPos4fv;
   table->RasterPos4i = save_RasterPos4i;
   table->RasterPos4iv = save_RasterPos4iv;
   table->RasterPos4s = save_RasterPos4s;
   table->RasterPos4sv = save_RasterPos4sv;
   table->ReadBuffer = save_ReadBuffer;
   table->ReadPixels = exec_ReadPixels;
   table->RenderMode = exec_RenderMode;
   table->Rotated = save_Rotated;
   table->Rotatef = save_Rotatef;
   table->Scaled = save_Scaled;
   table->Scalef = save_Scalef;
   table->Scissor = save_Scissor;
   table->FeedbackBuffer = exec_FeedbackBuffer;
   table->SelectBuffer = exec_SelectBuffer;
   table->ShadeModel = save_ShadeModel;
   table->StencilFunc = save_StencilFunc;
   table->StencilMask = save_StencilMask;
   table->StencilOp = save_StencilOp;
   table->TexEnvf = save_TexEnvf;
   table->TexEnvfv = save_TexEnvfv;
   table->TexEnvi = save_TexEnvi;
   table->TexEnviv = save_TexEnviv;
   table->TexGend = save_TexGend;
   table->TexGendv = save_TexGendv;
   table->TexGenf = save_TexGenf;
   table->TexGenfv = save_TexGenfv;
   table->TexGeni = save_TexGeni;
   table->TexGeniv = save_TexGeniv;
   table->TexImage1D = save_TexImage1D;
   table->TexImage2D = save_TexImage2D;
   table->TexParameterf = save_TexParameterf;
   table->TexParameterfv = save_TexParameterfv;
   table->TexParameteri = save_TexParameteri;
   table->TexParameteriv = save_TexParameteriv;
   table->Translated = save_Translated;
   table->Translatef = save_Translatef;
   table->Viewport = save_Viewport;

   /* GL 1.1 */
   table->AreTexturesResident = exec_AreTexturesResident;
   table->AreTexturesResidentEXT = exec_AreTexturesResident;
   table->BindTexture = save_BindTexture;
   table->ColorPointer = exec_ColorPointer;
   table->CopyTexImage1D = save_CopyTexImage1D;
   table->CopyTexImage2D = save_CopyTexImage2D;
   table->CopyTexSubImage1D = save_CopyTexSubImage1D;
   table->CopyTexSubImage2D = save_CopyTexSubImage2D;
   table->DeleteTextures = exec_DeleteTextures;
   table->DisableClientState = exec_DisableClientState;
   table->EdgeFlagPointer = exec_EdgeFlagPointer;
   table->EnableClientState = exec_EnableClientState;
   table->GenTextures = exec_GenTextures;
   table->GenTexturesEXT = exec_GenTextures;
   table->GetPointerv = exec_GetPointerv;
   table->IndexPointer = exec_IndexPointer;
   table->InterleavedArrays = exec_InterleavedArrays;
   table->IsTexture = exec_IsTexture;
   table->IsTextureEXT = exec_IsTexture;
   table->NormalPointer = exec_NormalPointer;
   table->PopClientAttrib = exec_PopClientAttrib;
   table->PrioritizeTextures = save_PrioritizeTextures;
   table->PushClientAttrib = exec_PushClientAttrib;
   table->TexCoordPointer = exec_TexCoordPointer;
   table->TexSubImage1D = save_TexSubImage1D;
   table->TexSubImage2D = save_TexSubImage2D;
   table->VertexPointer = exec_VertexPointer;

   /* GL 1.2 */
   table->CopyTexSubImage3D = save_CopyTexSubImage3D;
   table->TexImage3D = save_TexImage3D;
   table->TexSubImage3D = save_TexSubImage3D;

   /* GL_ARB_imaging */
   /* Not all are supported */
   table->BlendColor = save_BlendColor;
   table->BlendEquation = save_BlendEquation;
   table->ColorSubTable = save_ColorSubTable;
   table->ColorTable = save_ColorTable;
   table->ColorTableParameterfv = save_ColorTableParameterfv;
   table->ColorTableParameteriv = save_ColorTableParameteriv;
   table->ConvolutionFilter1D = save_ConvolutionFilter1D;
   table->ConvolutionFilter2D = save_ConvolutionFilter2D;
   table->ConvolutionParameterf = save_ConvolutionParameterf;
   table->ConvolutionParameterfv = save_ConvolutionParameterfv;
   table->ConvolutionParameteri = save_ConvolutionParameteri;
   table->ConvolutionParameteriv = save_ConvolutionParameteriv;
   table->CopyColorSubTable = save_CopyColorSubTable;
   table->CopyColorTable = save_CopyColorTable;
   table->CopyConvolutionFilter1D = exec_CopyConvolutionFilter1D;
   table->CopyConvolutionFilter2D = exec_CopyConvolutionFilter2D;
   table->GetColorTable = exec_GetColorTable;
   table->GetColorTableEXT = exec_GetColorTable;
   table->GetColorTableParameterfv = exec_GetColorTableParameterfv;
   table->GetColorTableParameterfvEXT = exec_GetColorTableParameterfv;
   table->GetColorTableParameteriv = exec_GetColorTableParameteriv;
   table->GetColorTableParameterivEXT = exec_GetColorTableParameteriv;
   table->GetConvolutionFilter = exec_GetConvolutionFilter;
   table->GetConvolutionFilterEXT = exec_GetConvolutionFilter;
   table->GetConvolutionParameterfv = exec_GetConvolutionParameterfv;
   table->GetConvolutionParameterfvEXT = exec_GetConvolutionParameterfv;
   table->GetConvolutionParameteriv = exec_GetConvolutionParameteriv;
   table->GetConvolutionParameterivEXT = exec_GetConvolutionParameteriv;
   table->GetHistogram = exec_GetHistogram;
   table->GetHistogramEXT = exec_GetHistogram;
   table->GetHistogramParameterfv = exec_GetHistogramParameterfv;
   table->GetHistogramParameterfvEXT = exec_GetHistogramParameterfv;
   table->GetHistogramParameteriv = exec_GetHistogramParameteriv;
   table->GetHistogramParameterivEXT = exec_GetHistogramParameteriv;
   table->GetMinmax = exec_GetMinmax;
   table->GetMinmaxEXT = exec_GetMinmax;
   table->GetMinmaxParameterfv = exec_GetMinmaxParameterfv;
   table->GetMinmaxParameterfvEXT = exec_GetMinmaxParameterfv;
   table->GetMinmaxParameteriv = exec_GetMinmaxParameteriv;
   table->GetMinmaxParameterivEXT = exec_GetMinmaxParameteriv;
   table->GetSeparableFilter = exec_GetSeparableFilter;
   table->GetSeparableFilterEXT = exec_GetSeparableFilter;
   table->Histogram = save_Histogram;
   table->Minmax = save_Minmax;
   table->ResetHistogram = save_ResetHistogram;
   table->ResetMinmax = save_ResetMinmax;
   table->SeparableFilter2D = exec_SeparableFilter2D;

   /* 2. GL_EXT_blend_color */
#if 0
   table->BlendColorEXT = save_BlendColorEXT;
#endif

   /* 3. GL_EXT_polygon_offset */
   table->PolygonOffsetEXT = save_PolygonOffsetEXT;

   /* 6. GL_EXT_texture3d */
#if 0
   table->CopyTexSubImage3DEXT = save_CopyTexSubImage3D;
   table->TexImage3DEXT = save_TexImage3DEXT;
   table->TexSubImage3DEXT = save_TexSubImage3D;
#endif

   /* 15. GL_SGIX_pixel_texture */
   table->PixelTexGenSGIX = save_PixelTexGenSGIX;

   /* 15. GL_SGIS_pixel_texture */
   table->PixelTexGenParameteriSGIS = save_PixelTexGenParameteriSGIS;
   table->PixelTexGenParameterfSGIS = save_PixelTexGenParameterfSGIS;
   table->PixelTexGenParameterivSGIS = save_PixelTexGenParameterivSGIS;
   table->PixelTexGenParameterfvSGIS = save_PixelTexGenParameterfvSGIS;
   table->GetPixelTexGenParameterivSGIS = exec_GetPixelTexGenParameterivSGIS;
   table->GetPixelTexGenParameterfvSGIS = exec_GetPixelTexGenParameterfvSGIS;

   /* 30. GL_EXT_vertex_array */
   table->ColorPointerEXT = exec_ColorPointerEXT;
   table->EdgeFlagPointerEXT = exec_EdgeFlagPointerEXT;
   table->IndexPointerEXT = exec_IndexPointerEXT;
   table->NormalPointerEXT = exec_NormalPointerEXT;
   table->TexCoordPointerEXT = exec_TexCoordPointerEXT;
   table->VertexPointerEXT = exec_VertexPointerEXT;

   /* 37. GL_EXT_blend_minmax */
#if 0
   table->BlendEquationEXT = save_BlendEquationEXT;
#endif

   /* 54. GL_EXT_point_parameters */
   table->PointParameterfEXT = save_PointParameterfEXT;
   table->PointParameterfvEXT = save_PointParameterfvEXT;

   /* 78. GL_EXT_paletted_texture */
#if 0
   table->ColorTableEXT = save_ColorTable;
   table->ColorSubTableEXT = save_ColorSubTable;
#endif
   table->GetColorTableEXT = exec_GetColorTable;
   table->GetColorTableParameterfvEXT = exec_GetColorTableParameterfv;
   table->GetColorTableParameterivEXT = exec_GetColorTableParameteriv;

   /* 97. GL_EXT_compiled_vertex_array */
   table->LockArraysEXT = exec_LockArraysEXT;
   table->UnlockArraysEXT = exec_UnlockArraysEXT;

   /* 145. GL_EXT_secondary_color */
   table->SecondaryColorPointerEXT = exec_SecondaryColorPointerEXT;

   /* 148. GL_EXT_multi_draw_arrays */
   table->MultiDrawArraysEXT = exec_MultiDrawArraysEXT;
   table->MultiDrawElementsEXT = exec_MultiDrawElementsEXT;

   /* 149. GL_EXT_fog_coord */
   table->FogCoordPointerEXT = exec_FogCoordPointerEXT;

   /* 173. GL_EXT_blend_func_separate */
   table->BlendFuncSeparateEXT = save_BlendFuncSeparateEXT;

   /* 196. GL_MESA_resize_buffers */
   table->ResizeBuffersMESA = exec_ResizeBuffersMESA;

   /* 197. GL_MESA_window_pos */
   table->WindowPos2dMESA = save_WindowPos2dMESA;
   table->WindowPos2dvMESA = save_WindowPos2dvMESA;
   table->WindowPos2fMESA = save_WindowPos2fMESA;
   table->WindowPos2fvMESA = save_WindowPos2fvMESA;
   table->WindowPos2iMESA = save_WindowPos2iMESA;
   table->WindowPos2ivMESA = save_WindowPos2ivMESA;
   table->WindowPos2sMESA = save_WindowPos2sMESA;
   table->WindowPos2svMESA = save_WindowPos2svMESA;
   table->WindowPos3dMESA = save_WindowPos3dMESA;
   table->WindowPos3dvMESA = save_WindowPos3dvMESA;
   table->WindowPos3fMESA = save_WindowPos3fMESA;
   table->WindowPos3fvMESA = save_WindowPos3fvMESA;
   table->WindowPos3iMESA = save_WindowPos3iMESA;
   table->WindowPos3ivMESA = save_WindowPos3ivMESA;
   table->WindowPos3sMESA = save_WindowPos3sMESA;
   table->WindowPos3svMESA = save_WindowPos3svMESA;
   table->WindowPos4dMESA = save_WindowPos4dMESA;
   table->WindowPos4dvMESA = save_WindowPos4dvMESA;
   table->WindowPos4fMESA = save_WindowPos4fMESA;
   table->WindowPos4fvMESA = save_WindowPos4fvMESA;
   table->WindowPos4iMESA = save_WindowPos4iMESA;
   table->WindowPos4ivMESA = save_WindowPos4ivMESA;
   table->WindowPos4sMESA = save_WindowPos4sMESA;
   table->WindowPos4svMESA = save_WindowPos4svMESA;

   /* 200. GL_IBM_multimode_draw_arrays */
   table->MultiModeDrawArraysIBM = exec_MultiModeDrawArraysIBM;
   table->MultiModeDrawElementsIBM = exec_MultiModeDrawElementsIBM;

#if FEATURE_NV_vertex_program
   /* 233. GL_NV_vertex_program */
   /* The following commands DO NOT go into display lists:
    * AreProgramsResidentNV, IsProgramNV, GenProgramsNV, DeleteProgramsNV,
    * VertexAttribPointerNV, GetProgram*, GetVertexAttrib*
    */
   table->BindProgramNV = save_BindProgramNV;
   table->DeleteProgramsNV = _mesa_DeletePrograms;
   table->ExecuteProgramNV = save_ExecuteProgramNV;
   table->GenProgramsNV = _mesa_GenPrograms;
   table->AreProgramsResidentNV = _mesa_AreProgramsResidentNV;
   table->RequestResidentProgramsNV = save_RequestResidentProgramsNV;
   table->GetProgramParameterfvNV = _mesa_GetProgramParameterfvNV;
   table->GetProgramParameterdvNV = _mesa_GetProgramParameterdvNV;
   table->GetProgramivNV = _mesa_GetProgramivNV;
   table->GetProgramStringNV = _mesa_GetProgramStringNV;
   table->GetTrackMatrixivNV = _mesa_GetTrackMatrixivNV;
   table->GetVertexAttribdvNV = _mesa_GetVertexAttribdvNV;
   table->GetVertexAttribfvNV = _mesa_GetVertexAttribfvNV;
   table->GetVertexAttribivNV = _mesa_GetVertexAttribivNV;
   table->GetVertexAttribPointervNV = _mesa_GetVertexAttribPointervNV;
   table->IsProgramNV = _mesa_IsProgram;
   table->LoadProgramNV = save_LoadProgramNV;
   table->ProgramParameter4dNV = save_ProgramParameter4dNV;
   table->ProgramParameter4dvNV = save_ProgramParameter4dvNV;
   table->ProgramParameter4fNV = save_ProgramParameter4fNV;
   table->ProgramParameter4fvNV = save_ProgramParameter4fvNV;
   table->ProgramParameters4dvNV = save_ProgramParameters4dvNV;
   table->ProgramParameters4fvNV = save_ProgramParameters4fvNV;
   table->TrackMatrixNV = save_TrackMatrixNV;
   table->VertexAttribPointerNV = _mesa_VertexAttribPointerNV;
#endif

   /* 282. GL_NV_fragment_program */
#if FEATURE_NV_fragment_program
   table->ProgramNamedParameter4fNV = save_ProgramNamedParameter4fNV;
   table->ProgramNamedParameter4dNV = save_ProgramNamedParameter4dNV;
   table->ProgramNamedParameter4fvNV = save_ProgramNamedParameter4fvNV;
   table->ProgramNamedParameter4dvNV = save_ProgramNamedParameter4dvNV;
   table->GetProgramNamedParameterfvNV = _mesa_GetProgramNamedParameterfvNV;
   table->GetProgramNamedParameterdvNV = _mesa_GetProgramNamedParameterdvNV;
   table->ProgramLocalParameter4dARB = save_ProgramLocalParameter4dARB;
   table->ProgramLocalParameter4dvARB = save_ProgramLocalParameter4dvARB;
   table->ProgramLocalParameter4fARB = save_ProgramLocalParameter4fARB;
   table->ProgramLocalParameter4fvARB = save_ProgramLocalParameter4fvARB;
   table->GetProgramLocalParameterdvARB = _mesa_GetProgramLocalParameterdvARB;
   table->GetProgramLocalParameterfvARB = _mesa_GetProgramLocalParameterfvARB;
#endif

   /* 262. GL_NV_point_sprite */
   table->PointParameteriNV = save_PointParameteriNV;
   table->PointParameterivNV = save_PointParameterivNV;

   /* 268. GL_EXT_stencil_two_side */
   table->ActiveStencilFaceEXT = save_ActiveStencilFaceEXT;

   /* ???. GL_EXT_depth_bounds_test */
   table->DepthBoundsEXT = save_DepthBoundsEXT;

   /* ARB 1. GL_ARB_multitexture */
   table->ActiveTextureARB = save_ActiveTextureARB;
   table->ClientActiveTextureARB = exec_ClientActiveTextureARB;

   /* ARB 3. GL_ARB_transpose_matrix */
   table->LoadTransposeMatrixdARB = save_LoadTransposeMatrixdARB;
   table->LoadTransposeMatrixfARB = save_LoadTransposeMatrixfARB;
   table->MultTransposeMatrixdARB = save_MultTransposeMatrixdARB;
   table->MultTransposeMatrixfARB = save_MultTransposeMatrixfARB;

   /* ARB 5. GL_ARB_multisample */
   table->SampleCoverageARB = save_SampleCoverageARB;

   /* ARB 12. GL_ARB_texture_compression */
   table->CompressedTexImage3DARB = save_CompressedTexImage3DARB;
   table->CompressedTexImage2DARB = save_CompressedTexImage2DARB;
   table->CompressedTexImage1DARB = save_CompressedTexImage1DARB;
   table->CompressedTexSubImage3DARB = save_CompressedTexSubImage3DARB;
   table->CompressedTexSubImage2DARB = save_CompressedTexSubImage2DARB;
   table->CompressedTexSubImage1DARB = save_CompressedTexSubImage1DARB;
   table->GetCompressedTexImageARB = exec_GetCompressedTexImageARB;

   /* ARB 14. GL_ARB_point_parameters */
   /* aliased with EXT_point_parameters functions */

   /* ARB 25. GL_ARB_window_pos */
   /* aliased with MESA_window_pos functions */

   /* ARB 26. GL_ARB_vertex_program */
   /* ARB 27. GL_ARB_fragment_program */
#if FEATURE_ARB_vertex_program || FEATURE_ARB_fragment_program
   /* glVertexAttrib* functions alias the NV ones, handled elsewhere */
   table->VertexAttribPointerARB = _mesa_VertexAttribPointerARB;
   table->EnableVertexAttribArrayARB = _mesa_EnableVertexAttribArrayARB;
   table->DisableVertexAttribArrayARB = _mesa_DisableVertexAttribArrayARB;
   table->ProgramStringARB = save_ProgramStringARB;
   table->BindProgramNV = save_BindProgramNV;
   table->DeleteProgramsNV = _mesa_DeletePrograms;
   table->GenProgramsNV = _mesa_GenPrograms;
   table->IsProgramNV = _mesa_IsProgram;
   table->GetVertexAttribdvNV = _mesa_GetVertexAttribdvNV;
   table->GetVertexAttribfvNV = _mesa_GetVertexAttribfvNV;
   table->GetVertexAttribivNV = _mesa_GetVertexAttribivNV;
   table->GetVertexAttribPointervNV = _mesa_GetVertexAttribPointervNV;
   table->ProgramEnvParameter4dARB = save_ProgramEnvParameter4dARB;
   table->ProgramEnvParameter4dvARB = save_ProgramEnvParameter4dvARB;
   table->ProgramEnvParameter4fARB = save_ProgramEnvParameter4fARB;
   table->ProgramEnvParameter4fvARB = save_ProgramEnvParameter4fvARB;
   table->ProgramLocalParameter4dARB = save_ProgramLocalParameter4dARB;
   table->ProgramLocalParameter4dvARB = save_ProgramLocalParameter4dvARB;
   table->ProgramLocalParameter4fARB = save_ProgramLocalParameter4fARB;
   table->ProgramLocalParameter4fvARB = save_ProgramLocalParameter4fvARB;
   table->GetProgramEnvParameterdvARB = _mesa_GetProgramEnvParameterdvARB;
   table->GetProgramEnvParameterfvARB = _mesa_GetProgramEnvParameterfvARB;
   table->GetProgramLocalParameterdvARB = _mesa_GetProgramLocalParameterdvARB;
   table->GetProgramLocalParameterfvARB = _mesa_GetProgramLocalParameterfvARB;
   table->GetProgramivARB = _mesa_GetProgramivARB;
   table->GetProgramStringARB = _mesa_GetProgramStringARB;
#endif

   /* ARB 28. GL_ARB_vertex_buffer_object */
#if FEATURE_ARB_vertex_buffer_object
   /* None of the extension's functions get compiled */
   table->BindBufferARB = _mesa_BindBufferARB;
   table->BufferDataARB = _mesa_BufferDataARB;
   table->BufferSubDataARB = _mesa_BufferSubDataARB;
   table->DeleteBuffersARB = _mesa_DeleteBuffersARB;
   table->GenBuffersARB = _mesa_GenBuffersARB;
   table->GetBufferParameterivARB = _mesa_GetBufferParameterivARB;
   table->GetBufferPointervARB = _mesa_GetBufferPointervARB;
   table->GetBufferSubDataARB = _mesa_GetBufferSubDataARB;
   table->IsBufferARB = _mesa_IsBufferARB;
   table->MapBufferARB = _mesa_MapBufferARB;
   table->UnmapBufferARB = _mesa_UnmapBufferARB;
#endif
}



/***
 *** Debugging code
 ***/
static const char *enum_string( GLenum k )
{
   return _mesa_lookup_enum_by_nr( k );
}


/*
 * Print the commands in a display list.  For debugging only.
 * TODO: many commands aren't handled yet.
 */
static void GLAPIENTRY print_list( GLcontext *ctx, GLuint list )
{
   Node *n;
   GLboolean done;

   if (!glIsList(list)) {
      _mesa_printf("%u is not a display list ID\n", list);
      return;
   }

   n = (Node *) _mesa_HashLookup(ctx->Shared->DisplayList, list);

   _mesa_printf("START-LIST %u, address %p\n", list, (void*)n );

   done = n ? GL_FALSE : GL_TRUE;
   while (!done) {
      OpCode opcode = n[0].opcode;
      GLint i = (GLint) n[0].opcode - (GLint) OPCODE_DRV_0;

      if (i >= 0 && i < (GLint) ctx->listext.nr_opcodes) {
	 ctx->listext.opcode[i].print(ctx, &n[1]);
	 n += ctx->listext.opcode[i].size;
      }
      else {
	 switch (opcode) {
         case OPCODE_ACCUM:
            _mesa_printf("accum %s %g\n", enum_string(n[1].e), n[2].f );
	    break;
	 case OPCODE_BITMAP:
            _mesa_printf("Bitmap %d %d %g %g %g %g %p\n", n[1].i, n[2].i,
		       n[3].f, n[4].f, n[5].f, n[6].f, (void *) n[7].data );
	    break;
         case OPCODE_CALL_LIST:
            _mesa_printf("CallList %d\n", (int) n[1].ui );
            break;
         case OPCODE_CALL_LIST_OFFSET:
            _mesa_printf("CallList %d + offset %u = %u\n", (int) n[1].ui,
                    ctx->List.ListBase, ctx->List.ListBase + n[1].ui );
            break;
         case OPCODE_COLOR_TABLE_PARAMETER_FV:
            _mesa_printf("ColorTableParameterfv %s %s %f %f %f %f\n",
                    enum_string(n[1].e), enum_string(n[2].e),
                    n[3].f, n[4].f, n[5].f, n[6].f);
            break;
         case OPCODE_COLOR_TABLE_PARAMETER_IV:
            _mesa_printf("ColorTableParameteriv %s %s %d %d %d %d\n",
                    enum_string(n[1].e), enum_string(n[2].e),
                    n[3].i, n[4].i, n[5].i, n[6].i);
            break;
	 case OPCODE_DISABLE:
            _mesa_printf("Disable %s\n", enum_string(n[1].e));
	    break;
	 case OPCODE_ENABLE:
            _mesa_printf("Enable %s\n", enum_string(n[1].e));
	    break;
         case OPCODE_FRUSTUM:
            _mesa_printf("Frustum %g %g %g %g %g %g\n",
                    n[1].f, n[2].f, n[3].f, n[4].f, n[5].f, n[6].f );
            break;
	 case OPCODE_LINE_STIPPLE:
	    _mesa_printf("LineStipple %d %x\n", n[1].i, (int) n[2].us );
	    break;
	 case OPCODE_LOAD_IDENTITY:
            _mesa_printf("LoadIdentity\n");
	    break;
	 case OPCODE_LOAD_MATRIX:
            _mesa_printf("LoadMatrix\n");
            _mesa_printf("  %8f %8f %8f %8f\n",
                         n[1].f, n[5].f,  n[9].f, n[13].f);
            _mesa_printf("  %8f %8f %8f %8f\n",
                         n[2].f, n[6].f, n[10].f, n[14].f);
            _mesa_printf("  %8f %8f %8f %8f\n",
                         n[3].f, n[7].f, n[11].f, n[15].f);
            _mesa_printf("  %8f %8f %8f %8f\n",
                         n[4].f, n[8].f, n[12].f, n[16].f);
	    break;
	 case OPCODE_MULT_MATRIX:
            _mesa_printf("MultMatrix (or Rotate)\n");
            _mesa_printf("  %8f %8f %8f %8f\n",
                         n[1].f, n[5].f,  n[9].f, n[13].f);
            _mesa_printf("  %8f %8f %8f %8f\n",
                         n[2].f, n[6].f, n[10].f, n[14].f);
            _mesa_printf("  %8f %8f %8f %8f\n",
                         n[3].f, n[7].f, n[11].f, n[15].f);
            _mesa_printf("  %8f %8f %8f %8f\n",
                         n[4].f, n[8].f, n[12].f, n[16].f);
	    break;
         case OPCODE_ORTHO:
            _mesa_printf("Ortho %g %g %g %g %g %g\n",
                    n[1].f, n[2].f, n[3].f, n[4].f, n[5].f, n[6].f );
            break;
	 case OPCODE_POP_ATTRIB:
            _mesa_printf("PopAttrib\n");
	    break;
	 case OPCODE_POP_MATRIX:
            _mesa_printf("PopMatrix\n");
	    break;
	 case OPCODE_POP_NAME:
            _mesa_printf("PopName\n");
	    break;
	 case OPCODE_PUSH_ATTRIB:
            _mesa_printf("PushAttrib %x\n", n[1].bf );
	    break;
	 case OPCODE_PUSH_MATRIX:
            _mesa_printf("PushMatrix\n");
	    break;
	 case OPCODE_PUSH_NAME:
            _mesa_printf("PushName %d\n", (int) n[1].ui );
	    break;
	 case OPCODE_RASTER_POS:
            _mesa_printf("RasterPos %g %g %g %g\n",
                         n[1].f, n[2].f,n[3].f,n[4].f);
	    break;
         case OPCODE_ROTATE:
            _mesa_printf("Rotate %g %g %g %g\n",
                         n[1].f, n[2].f, n[3].f, n[4].f );
            break;
         case OPCODE_SCALE:
            _mesa_printf("Scale %g %g %g\n", n[1].f, n[2].f, n[3].f );
            break;
         case OPCODE_TRANSLATE:
            _mesa_printf("Translate %g %g %g\n", n[1].f, n[2].f, n[3].f );
            break;
         case OPCODE_BIND_TEXTURE:
	    _mesa_printf("BindTexture %s %d\n",
                         _mesa_lookup_enum_by_nr(n[1].ui), n[2].ui);
	    break;
         case OPCODE_SHADE_MODEL:
	    _mesa_printf("ShadeModel %s\n",
                         _mesa_lookup_enum_by_nr(n[1].ui));
	    break;
	 case OPCODE_MAP1:
	    _mesa_printf("Map1 %s %.3f %.3f %d %d\n",
		    _mesa_lookup_enum_by_nr(n[1].ui),
		    n[2].f, n[3].f, n[4].i, n[5].i);
	    break;
	 case OPCODE_MAP2:
	    _mesa_printf("Map2 %s %.3f %.3f %.3f %.3f %d %d %d %d\n",
                         _mesa_lookup_enum_by_nr(n[1].ui),
                         n[2].f, n[3].f, n[4].f, n[5].f,
                         n[6].i, n[7].i, n[8].i, n[9].i);
	    break;
	 case OPCODE_MAPGRID1:
	    _mesa_printf("MapGrid1 %d %.3f %.3f\n",
                         n[1].i, n[2].f, n[3].f);
	    break;
	 case OPCODE_MAPGRID2:
	    _mesa_printf("MapGrid2 %d %.3f %.3f, %d %.3f %.3f\n",
                         n[1].i, n[2].f, n[3].f,
                         n[4].i, n[5].f, n[6].f);
	    break;
	 case OPCODE_EVALMESH1:
	    _mesa_printf("EvalMesh1 %d %d\n", n[1].i, n[2].i);
	    break;
	 case OPCODE_EVALMESH2:
	    _mesa_printf("EvalMesh2 %d %d %d %d\n",
                         n[1].i, n[2].i, n[3].i, n[4].i);
	    break;

	 /*
	  * meta opcodes/commands
	  */
         case OPCODE_ERROR:
            _mesa_printf("Error: %s %s\n",
                         enum_string(n[1].e), (const char *)n[2].data );
            break;
	 case OPCODE_CONTINUE:
            _mesa_printf("DISPLAY-LIST-CONTINUE\n");
	    n = (Node *) n[1].next;
	    break;
	 case OPCODE_END_OF_LIST:
            _mesa_printf("END-LIST %u\n", list);
	    done = GL_TRUE;
	    break;
         default:
            if (opcode < 0 || opcode > OPCODE_END_OF_LIST) {
               _mesa_printf("ERROR IN DISPLAY LIST: opcode = %d, address = %p\n",
                            opcode, (void*) n);
               return;
            }
            else {
               _mesa_printf("command %d, %u operands\n", opcode, InstSize[opcode]);
            }
	 }
	 /* increment n to point to next compiled command */
	 if (opcode!=OPCODE_CONTINUE) {
	    n += InstSize[opcode];
	 }
      }
   }
}



/*
 * Clients may call this function to help debug display list problems.
 * This function is _ONLY_FOR_DEBUGGING_PURPOSES_.  It may be removed,
 * changed, or break in the future without notice.
 */
void mesa_print_display_list( GLuint list )
{
   GET_CURRENT_CONTEXT(ctx);
   print_list( ctx, list );
}


/**********************************************************************/
/*****                      Initialization                        *****/
/**********************************************************************/


void _mesa_save_vtxfmt_init( GLvertexformat *vfmt )
{
   vfmt->ArrayElement = _ae_loopback_array_elt;	        /* generic helper */
   vfmt->Begin = save_Begin;
   vfmt->CallList = _mesa_save_CallList;
   vfmt->CallLists = _mesa_save_CallLists;
   vfmt->Color3f = save_Color3f;
   vfmt->Color3fv = save_Color3fv;
   vfmt->Color4f = save_Color4f;
   vfmt->Color4fv = save_Color4fv;
   vfmt->EdgeFlag = save_EdgeFlag;
   vfmt->EdgeFlagv = save_EdgeFlagv;
   vfmt->End = save_End;
   vfmt->EvalCoord1f = save_EvalCoord1f;
   vfmt->EvalCoord1fv = save_EvalCoord1fv;
   vfmt->EvalCoord2f = save_EvalCoord2f;
   vfmt->EvalCoord2fv = save_EvalCoord2fv;
   vfmt->EvalPoint1 = save_EvalPoint1;
   vfmt->EvalPoint2 = save_EvalPoint2;
   vfmt->FogCoordfEXT = save_FogCoordfEXT;
   vfmt->FogCoordfvEXT = save_FogCoordfvEXT;
   vfmt->Indexf = save_Indexf;
   vfmt->Indexfv = save_Indexfv;
   vfmt->Materialfv = save_Materialfv;
   vfmt->MultiTexCoord1fARB = save_MultiTexCoord1f;
   vfmt->MultiTexCoord1fvARB = save_MultiTexCoord1fv;
   vfmt->MultiTexCoord2fARB = save_MultiTexCoord2f;
   vfmt->MultiTexCoord2fvARB = save_MultiTexCoord2fv;
   vfmt->MultiTexCoord3fARB = save_MultiTexCoord3f;
   vfmt->MultiTexCoord3fvARB = save_MultiTexCoord3fv;
   vfmt->MultiTexCoord4fARB = save_MultiTexCoord4f;
   vfmt->MultiTexCoord4fvARB = save_MultiTexCoord4fv;
   vfmt->Normal3f = save_Normal3f;
   vfmt->Normal3fv = save_Normal3fv;
   vfmt->SecondaryColor3fEXT = save_SecondaryColor3fEXT;
   vfmt->SecondaryColor3fvEXT = save_SecondaryColor3fvEXT;
   vfmt->TexCoord1f = save_TexCoord1f;
   vfmt->TexCoord1fv = save_TexCoord1fv;
   vfmt->TexCoord2f = save_TexCoord2f;
   vfmt->TexCoord2fv = save_TexCoord2fv;
   vfmt->TexCoord3f = save_TexCoord3f;
   vfmt->TexCoord3fv = save_TexCoord3fv;
   vfmt->TexCoord4f = save_TexCoord4f;
   vfmt->TexCoord4fv = save_TexCoord4fv;
   vfmt->Vertex2f = save_Vertex2f;
   vfmt->Vertex2fv = save_Vertex2fv;
   vfmt->Vertex3f = save_Vertex3f;
   vfmt->Vertex3fv = save_Vertex3fv;
   vfmt->Vertex4f = save_Vertex4f;
   vfmt->Vertex4fv = save_Vertex4fv;
   vfmt->VertexAttrib1fNV = save_VertexAttrib1fNV;
   vfmt->VertexAttrib1fvNV = save_VertexAttrib1fvNV;
   vfmt->VertexAttrib2fNV = save_VertexAttrib2fNV;
   vfmt->VertexAttrib2fvNV = save_VertexAttrib2fvNV;
   vfmt->VertexAttrib3fNV = save_VertexAttrib3fNV;
   vfmt->VertexAttrib3fvNV = save_VertexAttrib3fvNV;
   vfmt->VertexAttrib4fNV = save_VertexAttrib4fNV;
   vfmt->VertexAttrib4fvNV = save_VertexAttrib4fvNV;

   vfmt->EvalMesh1 = _mesa_save_EvalMesh1;
   vfmt->EvalMesh2 = _mesa_save_EvalMesh2;
   vfmt->Rectf = save_Rectf;

   /* The driver is required to implement these as
    * 1) They can probably do a better job.
    * 2) A lot of new mechanisms would have to be added to this module
    *     to support it.  That code would probably never get used,
    *     because of (1).
    */
#if 0
   vfmt->DrawArrays = 0;
   vfmt->DrawElements = 0;
   vfmt->DrawRangeElements = 0;
#endif
}



void _mesa_init_display_list( GLcontext * ctx )
{
   /* Display list */
   ctx->ListState.CallDepth = 0;
   ctx->ExecuteFlag = GL_TRUE;
   ctx->CompileFlag = GL_FALSE;
   ctx->ListState.CurrentListPtr = NULL;
   ctx->ListState.CurrentBlock = NULL;
   ctx->ListState.CurrentListNum = 0;
   ctx->ListState.CurrentPos = 0;

   /* Display List group */
   ctx->List.ListBase = 0;

   _mesa_save_vtxfmt_init( &ctx->ListState.ListVtxfmt );
}

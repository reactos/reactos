/*
 * Mesa 3-D graphics library
 * Version:  7.7
 *
 * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
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
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


/**
 * \file dlist.c
 * Display lists management functions.
 */

#include "glheader.h"
#include "imports.h"
#include "api_arrayelt.h"
#include "api_exec.h"
#include "api_loopback.h"
#include "api_validate.h"
#if FEATURE_ATI_fragment_shader
#include "atifragshader.h"
#endif
#include "config.h"
#include "mfeatures.h"
#include "bufferobj.h"
#include "arrayobj.h"
#include "context.h"
#include "dlist.h"
#include "enums.h"
#include "eval.h"
#if FEATURE_EXT_framebuffer_object
#include "fbobject.h"
#endif
#include "framebuffer.h"
#include "glapi/glapi.h"
#include "hash.h"
#include "image.h"
#include "light.h"
#include "macros.h"
#include "pack.h"
#include "pbo.h"
#include "queryobj.h"
#include "samplerobj.h"
#include "shaderapi.h"
#include "syncobj.h"
#include "teximage.h"
#include "texstorage.h"
#include "mtypes.h"
#include "varray.h"
#if FEATURE_ARB_vertex_program || FEATURE_ARB_fragment_program
#include "arbprogram.h"
#endif
#if FEATURE_NV_vertex_program || FEATURE_NV_fragment_program
#include "nvprogram.h"
#endif
#if FEATURE_EXT_transform_feedback
#include "transformfeedback.h"
#endif

#include "math/m_matrix.h"

#include "main/dispatch.h"



/**
 * Other parts of Mesa (such as the VBO module) can plug into the display
 * list system.  This structure describes new display list instructions.
 */
struct gl_list_instruction
{
   GLuint Size;
   void (*Execute)( struct gl_context *ctx, void *data );
   void (*Destroy)( struct gl_context *ctx, void *data );
   void (*Print)( struct gl_context *ctx, void *data );
};


#define MAX_DLIST_EXT_OPCODES 16

/**
 * Used by device drivers to hook new commands into display lists.
 */
struct gl_list_extensions
{
   struct gl_list_instruction Opcode[MAX_DLIST_EXT_OPCODES];
   GLuint NumOpcodes;
};



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
typedef enum
{
   OPCODE_INVALID = -1,         /* Force signed enum */
   OPCODE_ACCUM,
   OPCODE_ALPHA_FUNC,
   OPCODE_BIND_TEXTURE,
   OPCODE_BITMAP,
   OPCODE_BLEND_COLOR,
   OPCODE_BLEND_EQUATION,
   OPCODE_BLEND_EQUATION_SEPARATE,
   OPCODE_BLEND_FUNC_SEPARATE,

   OPCODE_BLEND_EQUATION_I,
   OPCODE_BLEND_EQUATION_SEPARATE_I,
   OPCODE_BLEND_FUNC_I,
   OPCODE_BLEND_FUNC_SEPARATE_I,

   OPCODE_CALL_LIST,
   OPCODE_CALL_LIST_OFFSET,
   OPCODE_CLEAR,
   OPCODE_CLEAR_ACCUM,
   OPCODE_CLEAR_COLOR,
   OPCODE_CLEAR_DEPTH,
   OPCODE_CLEAR_INDEX,
   OPCODE_CLEAR_STENCIL,
   OPCODE_CLEAR_BUFFER_IV,
   OPCODE_CLEAR_BUFFER_UIV,
   OPCODE_CLEAR_BUFFER_FV,
   OPCODE_CLEAR_BUFFER_FI,
   OPCODE_CLIP_PLANE,
   OPCODE_COLOR_MASK,
   OPCODE_COLOR_MASK_INDEXED,
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
   OPCODE_DISABLE_INDEXED,
   OPCODE_DRAW_BUFFER,
   OPCODE_DRAW_PIXELS,
   OPCODE_ENABLE,
   OPCODE_ENABLE_INDEXED,
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
   /* GL_ARB_occlusion_query */
   OPCODE_BEGIN_QUERY_ARB,
   OPCODE_END_QUERY_ARB,
   /* GL_ARB_draw_buffers */
   OPCODE_DRAW_BUFFERS_ARB,
   /* GL_ATI_fragment_shader */
   OPCODE_TEX_BUMP_PARAMETER_ATI,
   /* GL_ATI_fragment_shader */
   OPCODE_BIND_FRAGMENT_SHADER_ATI,
   OPCODE_SET_FRAGMENT_SHADER_CONSTANTS_ATI,
   /* OpenGL 2.0 */
   OPCODE_STENCIL_FUNC_SEPARATE,
   OPCODE_STENCIL_OP_SEPARATE,
   OPCODE_STENCIL_MASK_SEPARATE,

   /* GL_ARB_shader_objects */
   OPCODE_USE_PROGRAM,
   OPCODE_UNIFORM_1F,
   OPCODE_UNIFORM_2F,
   OPCODE_UNIFORM_3F,
   OPCODE_UNIFORM_4F,
   OPCODE_UNIFORM_1FV,
   OPCODE_UNIFORM_2FV,
   OPCODE_UNIFORM_3FV,
   OPCODE_UNIFORM_4FV,
   OPCODE_UNIFORM_1I,
   OPCODE_UNIFORM_2I,
   OPCODE_UNIFORM_3I,
   OPCODE_UNIFORM_4I,
   OPCODE_UNIFORM_1IV,
   OPCODE_UNIFORM_2IV,
   OPCODE_UNIFORM_3IV,
   OPCODE_UNIFORM_4IV,
   OPCODE_UNIFORM_MATRIX22,
   OPCODE_UNIFORM_MATRIX33,
   OPCODE_UNIFORM_MATRIX44,
   OPCODE_UNIFORM_MATRIX23,
   OPCODE_UNIFORM_MATRIX32,
   OPCODE_UNIFORM_MATRIX24,
   OPCODE_UNIFORM_MATRIX42,
   OPCODE_UNIFORM_MATRIX34,
   OPCODE_UNIFORM_MATRIX43,

   /* OpenGL 3.0 */
   OPCODE_UNIFORM_1UI,
   OPCODE_UNIFORM_2UI,
   OPCODE_UNIFORM_3UI,
   OPCODE_UNIFORM_4UI,
   OPCODE_UNIFORM_1UIV,
   OPCODE_UNIFORM_2UIV,
   OPCODE_UNIFORM_3UIV,
   OPCODE_UNIFORM_4UIV,

   /* GL_ARB_color_buffer_float */
   OPCODE_CLAMP_COLOR,

   /* GL_EXT_framebuffer_blit */
   OPCODE_BLIT_FRAMEBUFFER,

   /* Vertex attributes -- fallback for when optimized display
    * list build isn't active.
    */
   OPCODE_ATTR_1F_NV,
   OPCODE_ATTR_2F_NV,
   OPCODE_ATTR_3F_NV,
   OPCODE_ATTR_4F_NV,
   OPCODE_ATTR_1F_ARB,
   OPCODE_ATTR_2F_ARB,
   OPCODE_ATTR_3F_ARB,
   OPCODE_ATTR_4F_ARB,
   OPCODE_MATERIAL,
   OPCODE_BEGIN,
   OPCODE_END,
   OPCODE_RECTF,
   OPCODE_EVAL_C1,
   OPCODE_EVAL_C2,
   OPCODE_EVAL_P1,
   OPCODE_EVAL_P2,

   /* GL_EXT_provoking_vertex */
   OPCODE_PROVOKING_VERTEX,

   /* GL_EXT_transform_feedback */
   OPCODE_BEGIN_TRANSFORM_FEEDBACK,
   OPCODE_END_TRANSFORM_FEEDBACK,
   OPCODE_BIND_TRANSFORM_FEEDBACK,
   OPCODE_PAUSE_TRANSFORM_FEEDBACK,
   OPCODE_RESUME_TRANSFORM_FEEDBACK,
   OPCODE_DRAW_TRANSFORM_FEEDBACK,

   /* GL_EXT_texture_integer */
   OPCODE_CLEARCOLOR_I,
   OPCODE_CLEARCOLOR_UI,
   OPCODE_TEXPARAMETER_I,
   OPCODE_TEXPARAMETER_UI,

   /* GL_EXT_separate_shader_objects */
   OPCODE_ACTIVE_PROGRAM_EXT,
   OPCODE_USE_SHADER_PROGRAM_EXT,

   /* GL_ARB_instanced_arrays */
   OPCODE_VERTEX_ATTRIB_DIVISOR,

   /* GL_NV_texture_barrier */
   OPCODE_TEXTURE_BARRIER_NV,

   /* GL_ARB_sampler_object */
   OPCODE_BIND_SAMPLER,
   OPCODE_SAMPLER_PARAMETERIV,
   OPCODE_SAMPLER_PARAMETERFV,
   OPCODE_SAMPLER_PARAMETERIIV,
   OPCODE_SAMPLER_PARAMETERUIV,

   /* GL_ARB_geometry_shader4 */
   OPCODE_PROGRAM_PARAMETERI,
   OPCODE_FRAMEBUFFER_TEXTURE,
   OPCODE_FRAMEBUFFER_TEXTURE_FACE,

   /* GL_ARB_sync */
   OPCODE_WAIT_SYNC,

   /* GL_NV_conditional_render */
   OPCODE_BEGIN_CONDITIONAL_RENDER,
   OPCODE_END_CONDITIONAL_RENDER,

   /* The following three are meta instructions */
   OPCODE_ERROR,                /* raise compiled-in error */
   OPCODE_CONTINUE,
   OPCODE_END_OF_LIST,
   OPCODE_EXT_0
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
union gl_dlist_node
{
   OpCode opcode;
   GLboolean b;
   GLbitfield bf;
   GLubyte ub;
   GLshort s;
   GLushort us;
   GLint i;
   GLuint ui;
   GLenum e;
   GLfloat f;
   GLvoid *data;
   void *next;                  /* If prev node's opcode==OPCODE_CONTINUE */
};


typedef union gl_dlist_node Node;


/**
 * Used to store a 64-bit uint in a pair of "Nodes" for the sake of 32-bit
 * environment.  In 64-bit env, sizeof(Node)==8 anyway.
 */
union uint64_pair
{
   GLuint64 uint64;
   GLuint uint32[2];
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
static GLuint InstSize[OPCODE_END_OF_LIST + 1];


#if FEATURE_dlist


void mesa_print_display_list(GLuint list);


/**********************************************************************/
/*****                           Private                          *****/
/**********************************************************************/


/**
 * Make an empty display list.  This is used by glGenLists() to
 * reserve display list IDs.
 */
static struct gl_display_list *
make_list(GLuint name, GLuint count)
{
   struct gl_display_list *dlist = CALLOC_STRUCT(gl_display_list);
   dlist->Name = name;
   dlist->Head = (Node *) malloc(sizeof(Node) * count);
   dlist->Head[0].opcode = OPCODE_END_OF_LIST;
   return dlist;
}


/**
 * Lookup function to just encapsulate casting.
 */
static inline struct gl_display_list *
lookup_list(struct gl_context *ctx, GLuint list)
{
   return (struct gl_display_list *)
      _mesa_HashLookup(ctx->Shared->DisplayList, list);
}


/** Is the given opcode an extension code? */
static inline GLboolean
is_ext_opcode(OpCode opcode)
{
   return (opcode >= OPCODE_EXT_0);
}


/** Destroy an extended opcode instruction */
static GLint
ext_opcode_destroy(struct gl_context *ctx, Node *node)
{
   const GLint i = node[0].opcode - OPCODE_EXT_0;
   GLint step;
   ctx->ListExt->Opcode[i].Destroy(ctx, &node[1]);
   step = ctx->ListExt->Opcode[i].Size;
   return step;
}


/** Execute an extended opcode instruction */
static GLint
ext_opcode_execute(struct gl_context *ctx, Node *node)
{
   const GLint i = node[0].opcode - OPCODE_EXT_0;
   GLint step;
   ctx->ListExt->Opcode[i].Execute(ctx, &node[1]);
   step = ctx->ListExt->Opcode[i].Size;
   return step;
}


/** Print an extended opcode instruction */
static GLint
ext_opcode_print(struct gl_context *ctx, Node *node)
{
   const GLint i = node[0].opcode - OPCODE_EXT_0;
   GLint step;
   ctx->ListExt->Opcode[i].Print(ctx, &node[1]);
   step = ctx->ListExt->Opcode[i].Size;
   return step;
}


/**
 * Delete the named display list, but don't remove from hash table.
 * \param dlist - display list pointer
 */
void
_mesa_delete_list(struct gl_context *ctx, struct gl_display_list *dlist)
{
   Node *n, *block;
   GLboolean done;

   n = block = dlist->Head;

   done = block ? GL_FALSE : GL_TRUE;
   while (!done) {
      const OpCode opcode = n[0].opcode;

      /* check for extension opcodes first */
      if (is_ext_opcode(opcode)) {
         n += ext_opcode_destroy(ctx, n);
      }
      else {
         switch (opcode) {
            /* for some commands, we need to free malloc'd memory */
         case OPCODE_MAP1:
            free(n[6].data);
            n += InstSize[n[0].opcode];
            break;
         case OPCODE_MAP2:
            free(n[10].data);
            n += InstSize[n[0].opcode];
            break;
         case OPCODE_DRAW_PIXELS:
            free(n[5].data);
            n += InstSize[n[0].opcode];
            break;
         case OPCODE_BITMAP:
            free(n[7].data);
            n += InstSize[n[0].opcode];
            break;
         case OPCODE_COLOR_TABLE:
            free(n[6].data);
            n += InstSize[n[0].opcode];
            break;
         case OPCODE_COLOR_SUB_TABLE:
            free(n[6].data);
            n += InstSize[n[0].opcode];
            break;
         case OPCODE_CONVOLUTION_FILTER_1D:
            free(n[6].data);
            n += InstSize[n[0].opcode];
            break;
         case OPCODE_CONVOLUTION_FILTER_2D:
            free(n[7].data);
            n += InstSize[n[0].opcode];
            break;
         case OPCODE_POLYGON_STIPPLE:
            free(n[1].data);
            n += InstSize[n[0].opcode];
            break;
         case OPCODE_TEX_IMAGE1D:
            free(n[8].data);
            n += InstSize[n[0].opcode];
            break;
         case OPCODE_TEX_IMAGE2D:
            free(n[9].data);
            n += InstSize[n[0].opcode];
            break;
         case OPCODE_TEX_IMAGE3D:
            free(n[10].data);
            n += InstSize[n[0].opcode];
            break;
         case OPCODE_TEX_SUB_IMAGE1D:
            free(n[7].data);
            n += InstSize[n[0].opcode];
            break;
         case OPCODE_TEX_SUB_IMAGE2D:
            free(n[9].data);
            n += InstSize[n[0].opcode];
            break;
         case OPCODE_TEX_SUB_IMAGE3D:
            free(n[11].data);
            n += InstSize[n[0].opcode];
            break;
         case OPCODE_COMPRESSED_TEX_IMAGE_1D:
            free(n[7].data);
            n += InstSize[n[0].opcode];
            break;
         case OPCODE_COMPRESSED_TEX_IMAGE_2D:
            free(n[8].data);
            n += InstSize[n[0].opcode];
            break;
         case OPCODE_COMPRESSED_TEX_IMAGE_3D:
            free(n[9].data);
            n += InstSize[n[0].opcode];
            break;
         case OPCODE_COMPRESSED_TEX_SUB_IMAGE_1D:
            free(n[7].data);
            n += InstSize[n[0].opcode];
            break;
         case OPCODE_COMPRESSED_TEX_SUB_IMAGE_2D:
            free(n[9].data);
            n += InstSize[n[0].opcode];
            break;
         case OPCODE_COMPRESSED_TEX_SUB_IMAGE_3D:
            free(n[11].data);
            n += InstSize[n[0].opcode];
            break;
#if FEATURE_NV_vertex_program
         case OPCODE_LOAD_PROGRAM_NV:
            free(n[4].data);      /* program string */
            n += InstSize[n[0].opcode];
            break;
         case OPCODE_REQUEST_RESIDENT_PROGRAMS_NV:
            free(n[2].data);      /* array of program ids */
            n += InstSize[n[0].opcode];
            break;
#endif
#if FEATURE_NV_fragment_program
         case OPCODE_PROGRAM_NAMED_PARAMETER_NV:
            free(n[3].data);      /* parameter name */
            n += InstSize[n[0].opcode];
            break;
#endif
#if FEATURE_ARB_vertex_program || FEATURE_ARB_fragment_program
         case OPCODE_PROGRAM_STRING_ARB:
            free(n[4].data);      /* program string */
            n += InstSize[n[0].opcode];
            break;
#endif
         case OPCODE_UNIFORM_1FV:
         case OPCODE_UNIFORM_2FV:
         case OPCODE_UNIFORM_3FV:
         case OPCODE_UNIFORM_4FV:
         case OPCODE_UNIFORM_1IV:
         case OPCODE_UNIFORM_2IV:
         case OPCODE_UNIFORM_3IV:
         case OPCODE_UNIFORM_4IV:
         case OPCODE_UNIFORM_1UIV:
         case OPCODE_UNIFORM_2UIV:
         case OPCODE_UNIFORM_3UIV:
         case OPCODE_UNIFORM_4UIV:
            free(n[3].data);
            n += InstSize[n[0].opcode];
            break;
         case OPCODE_UNIFORM_MATRIX22:
         case OPCODE_UNIFORM_MATRIX33:
         case OPCODE_UNIFORM_MATRIX44:
         case OPCODE_UNIFORM_MATRIX24:
         case OPCODE_UNIFORM_MATRIX42:
         case OPCODE_UNIFORM_MATRIX23:
         case OPCODE_UNIFORM_MATRIX32:
         case OPCODE_UNIFORM_MATRIX34:
         case OPCODE_UNIFORM_MATRIX43:
            free(n[4].data);
            n += InstSize[n[0].opcode];
            break;

         case OPCODE_CONTINUE:
            n = (Node *) n[1].next;
            free(block);
            block = n;
            break;
         case OPCODE_END_OF_LIST:
            free(block);
            done = GL_TRUE;
            break;
         default:
            /* Most frequent case */
            n += InstSize[n[0].opcode];
            break;
         }
      }
   }

   free(dlist);
}


/**
 * Destroy a display list and remove from hash table.
 * \param list - display list number
 */
static void
destroy_list(struct gl_context *ctx, GLuint list)
{
   struct gl_display_list *dlist;

   if (list == 0)
      return;

   dlist = lookup_list(ctx, list);
   if (!dlist)
      return;

   _mesa_delete_list(ctx, dlist);
   _mesa_HashRemove(ctx->Shared->DisplayList, list);
}


/*
 * Translate the nth element of list from <type> to GLint.
 */
static GLint
translate_id(GLsizei n, GLenum type, const GLvoid * list)
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
      return (GLint) bptr[n];
   case GL_UNSIGNED_BYTE:
      ubptr = (GLubyte *) list;
      return (GLint) ubptr[n];
   case GL_SHORT:
      sptr = (GLshort *) list;
      return (GLint) sptr[n];
   case GL_UNSIGNED_SHORT:
      usptr = (GLushort *) list;
      return (GLint) usptr[n];
   case GL_INT:
      iptr = (GLint *) list;
      return iptr[n];
   case GL_UNSIGNED_INT:
      uiptr = (GLuint *) list;
      return (GLint) uiptr[n];
   case GL_FLOAT:
      fptr = (GLfloat *) list;
      return (GLint) FLOORF(fptr[n]);
   case GL_2_BYTES:
      ubptr = ((GLubyte *) list) + 2 * n;
      return (GLint) ubptr[0] * 256
           + (GLint) ubptr[1];
   case GL_3_BYTES:
      ubptr = ((GLubyte *) list) + 3 * n;
      return (GLint) ubptr[0] * 65536
           + (GLint) ubptr[1] * 256
           + (GLint) ubptr[2];
   case GL_4_BYTES:
      ubptr = ((GLubyte *) list) + 4 * n;
      return (GLint) ubptr[0] * 16777216
           + (GLint) ubptr[1] * 65536
           + (GLint) ubptr[2] * 256
           + (GLint) ubptr[3];
   default:
      return 0;
   }
}




/**********************************************************************/
/*****                        Public                              *****/
/**********************************************************************/

/**
 * Wrapper for _mesa_unpack_image/bitmap() that handles pixel buffer objects.
 * If width < 0 or height < 0 or format or type are invalid we'll just
 * return NULL.  We will not generate an error since OpenGL command
 * arguments aren't error-checked until the command is actually executed
 * (not when they're compiled).
 * But if we run out of memory, GL_OUT_OF_MEMORY will be recorded.
 */
static GLvoid *
unpack_image(struct gl_context *ctx, GLuint dimensions,
             GLsizei width, GLsizei height, GLsizei depth,
             GLenum format, GLenum type, const GLvoid * pixels,
             const struct gl_pixelstore_attrib *unpack)
{
   if (width <= 0 || height <= 0) {
      return NULL;
   }

   if (_mesa_bytes_per_pixel(format, type) < 0) {
      /* bad format and/or type */
      return NULL;
   }

   if (!_mesa_is_bufferobj(unpack->BufferObj)) {
      /* no PBO */
      GLvoid *image;

      if (type == GL_BITMAP)
         image = _mesa_unpack_bitmap(width, height, pixels, unpack);
      else
         image = _mesa_unpack_image(dimensions, width, height, depth,
                                    format, type, pixels, unpack);
      if (pixels && !image) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "display list construction");
      }
      return image;
   }
   else if (_mesa_validate_pbo_access(dimensions, unpack, width, height,
                                      depth, format, type, INT_MAX, pixels)) {
      const GLubyte *map, *src;
      GLvoid *image;

      map = (GLubyte *)
         ctx->Driver.MapBufferRange(ctx, 0, unpack->BufferObj->Size,
				    GL_MAP_READ_BIT, unpack->BufferObj);
      if (!map) {
         /* unable to map src buffer! */
         _mesa_error(ctx, GL_INVALID_OPERATION, "unable to map PBO");
         return NULL;
      }

      src = ADD_POINTERS(map, pixels);
      if (type == GL_BITMAP)
         image = _mesa_unpack_bitmap(width, height, src, unpack);
      else
         image = _mesa_unpack_image(dimensions, width, height, depth,
                                    format, type, src, unpack);

      ctx->Driver.UnmapBuffer(ctx, unpack->BufferObj);

      if (!image) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "display list construction");
      }
      return image;
   }

   /* bad access! */
   _mesa_error(ctx, GL_INVALID_OPERATION, "invalid PBO access");
   return NULL;
}

/**
 * Allocate space for a display list instruction (opcode + payload space).
 * \param opcode  the instruction opcode (OPCODE_* value)
 * \param bytes   instruction payload size (not counting opcode)
 * \return pointer to allocated memory (the opcode space)
 */
static Node *
dlist_alloc(struct gl_context *ctx, OpCode opcode, GLuint bytes)
{
   const GLuint numNodes = 1 + (bytes + sizeof(Node) - 1) / sizeof(Node);
   Node *n;

   if (opcode < (GLuint) OPCODE_EXT_0) {
      if (InstSize[opcode] == 0) {
         /* save instruction size now */
         InstSize[opcode] = numNodes;
      }
      else {
         /* make sure instruction size agrees */
         ASSERT(numNodes == InstSize[opcode]);
      }
   }

   if (ctx->ListState.CurrentPos + numNodes + 2 > BLOCK_SIZE) {
      /* This block is full.  Allocate a new block and chain to it */
      Node *newblock;
      n = ctx->ListState.CurrentBlock + ctx->ListState.CurrentPos;
      n[0].opcode = OPCODE_CONTINUE;
      newblock = (Node *) malloc(sizeof(Node) * BLOCK_SIZE);
      if (!newblock) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "Building display list");
         return NULL;
      }
      n[1].next = (Node *) newblock;
      ctx->ListState.CurrentBlock = newblock;
      ctx->ListState.CurrentPos = 0;
   }

   n = ctx->ListState.CurrentBlock + ctx->ListState.CurrentPos;
   ctx->ListState.CurrentPos += numNodes;

   n[0].opcode = opcode;

   return n;
}



/**
 * Allocate space for a display list instruction.  Used by callers outside
 * this file for things like VBO vertex data.
 *
 * \param opcode  the instruction opcode (OPCODE_* value)
 * \param bytes   instruction size in bytes, not counting opcode.
 * \return pointer to the usable data area (not including the internal
 *         opcode).
 */
void *
_mesa_dlist_alloc(struct gl_context *ctx, GLuint opcode, GLuint bytes)
{
   Node *n = dlist_alloc(ctx, (OpCode) opcode, bytes);
   if (n)
      return n + 1;  /* return pointer to payload area, after opcode */
   else
      return NULL;
}


/**
 * This function allows modules and drivers to get their own opcodes
 * for extending display list functionality.
 * \param ctx  the rendering context
 * \param size  number of bytes for storing the new display list command
 * \param execute  function to execute the new display list command
 * \param destroy  function to destroy the new display list command
 * \param print  function to print the new display list command
 * \return  the new opcode number or -1 if error
 */
GLint
_mesa_dlist_alloc_opcode(struct gl_context *ctx,
                         GLuint size,
                         void (*execute) (struct gl_context *, void *),
                         void (*destroy) (struct gl_context *, void *),
                         void (*print) (struct gl_context *, void *))
{
   if (ctx->ListExt->NumOpcodes < MAX_DLIST_EXT_OPCODES) {
      const GLuint i = ctx->ListExt->NumOpcodes++;
      ctx->ListExt->Opcode[i].Size =
         1 + (size + sizeof(Node) - 1) / sizeof(Node);
      ctx->ListExt->Opcode[i].Execute = execute;
      ctx->ListExt->Opcode[i].Destroy = destroy;
      ctx->ListExt->Opcode[i].Print = print;
      return i + OPCODE_EXT_0;
   }
   return -1;
}


/**
 * Allocate space for a display list instruction.  The space is basically
 * an array of Nodes where node[0] holds the opcode, node[1] is the first
 * function parameter, node[2] is the second parameter, etc.
 *
 * \param opcode  one of OPCODE_x
 * \param nparams  number of function parameters
 * \return  pointer to start of instruction space
 */
static inline Node *
alloc_instruction(struct gl_context *ctx, OpCode opcode, GLuint nparams)
{
   return dlist_alloc(ctx, opcode, nparams * sizeof(Node));
}



/*
 * Display List compilation functions
 */
static void GLAPIENTRY
save_Accum(GLenum op, GLfloat value)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_ACCUM, 2);
   if (n) {
      n[1].e = op;
      n[2].f = value;
   }
   if (ctx->ExecuteFlag) {
      CALL_Accum(ctx->Exec, (op, value));
   }
}


static void GLAPIENTRY
save_AlphaFunc(GLenum func, GLclampf ref)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_ALPHA_FUNC, 2);
   if (n) {
      n[1].e = func;
      n[2].f = (GLfloat) ref;
   }
   if (ctx->ExecuteFlag) {
      CALL_AlphaFunc(ctx->Exec, (func, ref));
   }
}


static void GLAPIENTRY
save_BindTexture(GLenum target, GLuint texture)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_BIND_TEXTURE, 2);
   if (n) {
      n[1].e = target;
      n[2].ui = texture;
   }
   if (ctx->ExecuteFlag) {
      CALL_BindTexture(ctx->Exec, (target, texture));
   }
}


static void GLAPIENTRY
save_Bitmap(GLsizei width, GLsizei height,
            GLfloat xorig, GLfloat yorig,
            GLfloat xmove, GLfloat ymove, const GLubyte * pixels)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_BITMAP, 7);
   if (n) {
      n[1].i = (GLint) width;
      n[2].i = (GLint) height;
      n[3].f = xorig;
      n[4].f = yorig;
      n[5].f = xmove;
      n[6].f = ymove;
      n[7].data = unpack_image(ctx, 2, width, height, 1, GL_COLOR_INDEX,
                               GL_BITMAP, pixels, &ctx->Unpack);
   }
   if (ctx->ExecuteFlag) {
      CALL_Bitmap(ctx->Exec, (width, height,
                              xorig, yorig, xmove, ymove, pixels));
   }
}


static void GLAPIENTRY
save_BlendEquation(GLenum mode)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_BLEND_EQUATION, 1);
   if (n) {
      n[1].e = mode;
   }
   if (ctx->ExecuteFlag) {
      CALL_BlendEquation(ctx->Exec, (mode));
   }
}


static void GLAPIENTRY
save_BlendEquationSeparateEXT(GLenum modeRGB, GLenum modeA)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_BLEND_EQUATION_SEPARATE, 2);
   if (n) {
      n[1].e = modeRGB;
      n[2].e = modeA;
   }
   if (ctx->ExecuteFlag) {
      CALL_BlendEquationSeparateEXT(ctx->Exec, (modeRGB, modeA));
   }
}


static void GLAPIENTRY
save_BlendFuncSeparateEXT(GLenum sfactorRGB, GLenum dfactorRGB,
                          GLenum sfactorA, GLenum dfactorA)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_BLEND_FUNC_SEPARATE, 4);
   if (n) {
      n[1].e = sfactorRGB;
      n[2].e = dfactorRGB;
      n[3].e = sfactorA;
      n[4].e = dfactorA;
   }
   if (ctx->ExecuteFlag) {
      CALL_BlendFuncSeparateEXT(ctx->Exec,
                                (sfactorRGB, dfactorRGB, sfactorA, dfactorA));
   }
}


static void GLAPIENTRY
save_BlendFunc(GLenum srcfactor, GLenum dstfactor)
{
   save_BlendFuncSeparateEXT(srcfactor, dstfactor, srcfactor, dstfactor);
}


static void GLAPIENTRY
save_BlendColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_BLEND_COLOR, 4);
   if (n) {
      n[1].f = red;
      n[2].f = green;
      n[3].f = blue;
      n[4].f = alpha;
   }
   if (ctx->ExecuteFlag) {
      CALL_BlendColor(ctx->Exec, (red, green, blue, alpha));
   }
}

/* GL_ARB_draw_buffers_blend */
static void GLAPIENTRY
save_BlendFuncSeparatei(GLuint buf, GLenum sfactorRGB, GLenum dfactorRGB,
                        GLenum sfactorA, GLenum dfactorA)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_BLEND_FUNC_SEPARATE_I, 5);
   if (n) {
      n[1].ui = buf;
      n[2].e = sfactorRGB;
      n[3].e = dfactorRGB;
      n[4].e = sfactorA;
      n[5].e = dfactorA;
   }
   if (ctx->ExecuteFlag) {
      CALL_BlendFuncSeparateiARB(ctx->Exec, (buf, sfactorRGB, dfactorRGB,
                                             sfactorA, dfactorA));
   }
}

/* GL_ARB_draw_buffers_blend */
static void GLAPIENTRY
save_BlendFunci(GLuint buf, GLenum sfactor, GLenum dfactor)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_BLEND_FUNC_SEPARATE_I, 3);
   if (n) {
      n[1].ui = buf;
      n[2].e = sfactor;
      n[3].e = dfactor;
   }
   if (ctx->ExecuteFlag) {
      CALL_BlendFunciARB(ctx->Exec, (buf, sfactor, dfactor));
   }
}

/* GL_ARB_draw_buffers_blend */
static void GLAPIENTRY
save_BlendEquationi(GLuint buf, GLenum mode)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_BLEND_EQUATION_I, 2);
   if (n) {
      n[1].ui = buf;
      n[2].e = mode;
   }
   if (ctx->ExecuteFlag) {
      CALL_BlendEquationiARB(ctx->Exec, (buf, mode));
   }
}

/* GL_ARB_draw_buffers_blend */
static void GLAPIENTRY
save_BlendEquationSeparatei(GLuint buf, GLenum modeRGB, GLenum modeA)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_BLEND_EQUATION_SEPARATE_I, 3);
   if (n) {
      n[1].ui = buf;
      n[2].e = modeRGB;
      n[3].e = modeA;
   }
   if (ctx->ExecuteFlag) {
      CALL_BlendEquationSeparateiARB(ctx->Exec, (buf, modeRGB, modeA));
   }
}


/* GL_ARB_draw_instanced. */
static void GLAPIENTRY
save_DrawArraysInstancedARB(GLenum mode,
			    GLint first,
			    GLsizei count,
			    GLsizei primcount)
{
   GET_CURRENT_CONTEXT(ctx);
   _mesa_error(ctx, GL_INVALID_OPERATION,
	       "glDrawArraysInstanced() during display list compile");
}

static void GLAPIENTRY
save_DrawElementsInstancedARB(GLenum mode,
			      GLsizei count,
			      GLenum type,
			      const GLvoid *indices,
			      GLsizei primcount)
{
   GET_CURRENT_CONTEXT(ctx);
   _mesa_error(ctx, GL_INVALID_OPERATION,
	       "glDrawElementsInstanced() during display list compile");
}

static void GLAPIENTRY
save_DrawElementsInstancedBaseVertexARB(GLenum mode,
					GLsizei count,
					GLenum type,
					const GLvoid *indices,
					GLsizei primcount,
					GLint basevertex)
{
   GET_CURRENT_CONTEXT(ctx);
   _mesa_error(ctx, GL_INVALID_OPERATION,
	       "glDrawElementsInstancedBaseVertex() during display list compile");
}

static void invalidate_saved_current_state( struct gl_context *ctx )
{
   GLint i;

   for (i = 0; i < VERT_ATTRIB_MAX; i++)
      ctx->ListState.ActiveAttribSize[i] = 0;

   for (i = 0; i < MAT_ATTRIB_MAX; i++)
      ctx->ListState.ActiveMaterialSize[i] = 0;

   memset(&ctx->ListState.Current, 0, sizeof ctx->ListState.Current);

   ctx->Driver.CurrentSavePrimitive = PRIM_UNKNOWN;
}

static void GLAPIENTRY
save_CallList(GLuint list)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   SAVE_FLUSH_VERTICES(ctx);

   n = alloc_instruction(ctx, OPCODE_CALL_LIST, 1);
   if (n) {
      n[1].ui = list;
   }

   /* After this, we don't know what state we're in.  Invalidate all
    * cached information previously gathered:
    */
   invalidate_saved_current_state( ctx );

   if (ctx->ExecuteFlag) {
      _mesa_CallList(list);
   }
}


static void GLAPIENTRY
save_CallLists(GLsizei num, GLenum type, const GLvoid * lists)
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

   for (i = 0; i < num; i++) {
      GLint list = translate_id(i, type, lists);
      Node *n = alloc_instruction(ctx, OPCODE_CALL_LIST_OFFSET, 2);
      if (n) {
         n[1].i = list;
         n[2].b = typeErrorFlag;
      }
   }

   /* After this, we don't know what state we're in.  Invalidate all
    * cached information previously gathered:
    */
   invalidate_saved_current_state( ctx );

   if (ctx->ExecuteFlag) {
      CALL_CallLists(ctx->Exec, (num, type, lists));
   }
}


static void GLAPIENTRY
save_Clear(GLbitfield mask)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_CLEAR, 1);
   if (n) {
      n[1].bf = mask;
   }
   if (ctx->ExecuteFlag) {
      CALL_Clear(ctx->Exec, (mask));
   }
}


static void GLAPIENTRY
save_ClearBufferiv(GLenum buffer, GLint drawbuffer, const GLint *value)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_CLEAR_BUFFER_IV, 6);
   if (n) {
      n[1].e = buffer;
      n[2].i = drawbuffer;
      n[3].i = value[0];
      if (buffer == GL_COLOR) {
         n[4].i = value[1];
         n[5].i = value[2];
         n[6].i = value[3];
      }
      else {
         n[4].i = 0;
         n[5].i = 0;
         n[6].i = 0;
      }
   }
   if (ctx->ExecuteFlag) {
      CALL_ClearBufferiv(ctx->Exec, (buffer, drawbuffer, value));
   }
}


static void GLAPIENTRY
save_ClearBufferuiv(GLenum buffer, GLint drawbuffer, const GLuint *value)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_CLEAR_BUFFER_UIV, 6);
   if (n) {
      n[1].e = buffer;
      n[2].i = drawbuffer;
      n[3].ui = value[0];
      if (buffer == GL_COLOR) {
         n[4].ui = value[1];
         n[5].ui = value[2];
         n[6].ui = value[3];
      }
      else {
         n[4].ui = 0;
         n[5].ui = 0;
         n[6].ui = 0;
      }
   }
   if (ctx->ExecuteFlag) {
      CALL_ClearBufferuiv(ctx->Exec, (buffer, drawbuffer, value));
   }
}


static void GLAPIENTRY
save_ClearBufferfv(GLenum buffer, GLint drawbuffer, const GLfloat *value)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_CLEAR_BUFFER_FV, 6);
   if (n) {
      n[1].e = buffer;
      n[2].i = drawbuffer;
      n[3].f = value[0];
      if (buffer == GL_COLOR) {
         n[4].f = value[1];
         n[5].f = value[2];
         n[6].f = value[3];
      }
      else {
         n[4].f = 0.0F;
         n[5].f = 0.0F;
         n[6].f = 0.0F;
      }
   }
   if (ctx->ExecuteFlag) {
      CALL_ClearBufferfv(ctx->Exec, (buffer, drawbuffer, value));
   }
}


static void GLAPIENTRY
save_ClearBufferfi(GLenum buffer, GLint drawbuffer,
                   GLfloat depth, GLint stencil)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_CLEAR_BUFFER_FI, 4);
   if (n) {
      n[1].e = buffer;
      n[2].i = drawbuffer;
      n[3].f = depth;
      n[4].i = stencil;
   }
   if (ctx->ExecuteFlag) {
      CALL_ClearBufferfi(ctx->Exec, (buffer, drawbuffer, depth, stencil));
   }
}


static void GLAPIENTRY
save_ClearAccum(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_CLEAR_ACCUM, 4);
   if (n) {
      n[1].f = red;
      n[2].f = green;
      n[3].f = blue;
      n[4].f = alpha;
   }
   if (ctx->ExecuteFlag) {
      CALL_ClearAccum(ctx->Exec, (red, green, blue, alpha));
   }
}


static void GLAPIENTRY
save_ClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_CLEAR_COLOR, 4);
   if (n) {
      n[1].f = red;
      n[2].f = green;
      n[3].f = blue;
      n[4].f = alpha;
   }
   if (ctx->ExecuteFlag) {
      CALL_ClearColor(ctx->Exec, (red, green, blue, alpha));
   }
}


static void GLAPIENTRY
save_ClearDepth(GLclampd depth)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_CLEAR_DEPTH, 1);
   if (n) {
      n[1].f = (GLfloat) depth;
   }
   if (ctx->ExecuteFlag) {
      CALL_ClearDepth(ctx->Exec, (depth));
   }
}


static void GLAPIENTRY
save_ClearIndex(GLfloat c)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_CLEAR_INDEX, 1);
   if (n) {
      n[1].f = c;
   }
   if (ctx->ExecuteFlag) {
      CALL_ClearIndex(ctx->Exec, (c));
   }
}


static void GLAPIENTRY
save_ClearStencil(GLint s)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_CLEAR_STENCIL, 1);
   if (n) {
      n[1].i = s;
   }
   if (ctx->ExecuteFlag) {
      CALL_ClearStencil(ctx->Exec, (s));
   }
}


static void GLAPIENTRY
save_ClipPlane(GLenum plane, const GLdouble * equ)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_CLIP_PLANE, 5);
   if (n) {
      n[1].e = plane;
      n[2].f = (GLfloat) equ[0];
      n[3].f = (GLfloat) equ[1];
      n[4].f = (GLfloat) equ[2];
      n[5].f = (GLfloat) equ[3];
   }
   if (ctx->ExecuteFlag) {
      CALL_ClipPlane(ctx->Exec, (plane, equ));
   }
}



static void GLAPIENTRY
save_ColorMask(GLboolean red, GLboolean green,
               GLboolean blue, GLboolean alpha)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_COLOR_MASK, 4);
   if (n) {
      n[1].b = red;
      n[2].b = green;
      n[3].b = blue;
      n[4].b = alpha;
   }
   if (ctx->ExecuteFlag) {
      CALL_ColorMask(ctx->Exec, (red, green, blue, alpha));
   }
}


static void GLAPIENTRY
save_ColorMaskIndexed(GLuint buf, GLboolean red, GLboolean green,
                      GLboolean blue, GLboolean alpha)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_COLOR_MASK_INDEXED, 5);
   if (n) {
      n[1].ui = buf;
      n[2].b = red;
      n[3].b = green;
      n[4].b = blue;
      n[5].b = alpha;
   }
   if (ctx->ExecuteFlag) {
      /*CALL_ColorMaskIndexedEXT(ctx->Exec, (buf, red, green, blue, alpha));*/
   }
}


static void GLAPIENTRY
save_ColorMaterial(GLenum face, GLenum mode)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);

   n = alloc_instruction(ctx, OPCODE_COLOR_MATERIAL, 2);
   if (n) {
      n[1].e = face;
      n[2].e = mode;
   }
   if (ctx->ExecuteFlag) {
      CALL_ColorMaterial(ctx->Exec, (face, mode));
   }
}


static void GLAPIENTRY
save_ColorTable(GLenum target, GLenum internalFormat,
                GLsizei width, GLenum format, GLenum type,
                const GLvoid * table)
{
   GET_CURRENT_CONTEXT(ctx);
   if (_mesa_is_proxy_texture(target)) {
      /* execute immediately */
      CALL_ColorTable(ctx->Exec, (target, internalFormat, width,
                                  format, type, table));
   }
   else {
      Node *n;
      ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
      n = alloc_instruction(ctx, OPCODE_COLOR_TABLE, 6);
      if (n) {
         n[1].e = target;
         n[2].e = internalFormat;
         n[3].i = width;
         n[4].e = format;
         n[5].e = type;
         n[6].data = unpack_image(ctx, 1, width, 1, 1, format, type, table,
                                  &ctx->Unpack);
      }
      if (ctx->ExecuteFlag) {
         CALL_ColorTable(ctx->Exec, (target, internalFormat, width,
                                     format, type, table));
      }
   }
}



static void GLAPIENTRY
save_ColorTableParameterfv(GLenum target, GLenum pname,
                           const GLfloat *params)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;

   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);

   n = alloc_instruction(ctx, OPCODE_COLOR_TABLE_PARAMETER_FV, 6);
   if (n) {
      n[1].e = target;
      n[2].e = pname;
      n[3].f = params[0];
      if (pname == GL_COLOR_TABLE_SGI ||
          pname == GL_POST_CONVOLUTION_COLOR_TABLE_SGI ||
          pname == GL_TEXTURE_COLOR_TABLE_SGI) {
         n[4].f = params[1];
         n[5].f = params[2];
         n[6].f = params[3];
      }
   }

   if (ctx->ExecuteFlag) {
      CALL_ColorTableParameterfv(ctx->Exec, (target, pname, params));
   }
}


static void GLAPIENTRY
save_ColorTableParameteriv(GLenum target, GLenum pname, const GLint *params)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;

   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);

   n = alloc_instruction(ctx, OPCODE_COLOR_TABLE_PARAMETER_IV, 6);
   if (n) {
      n[1].e = target;
      n[2].e = pname;
      n[3].i = params[0];
      if (pname == GL_COLOR_TABLE_SGI ||
          pname == GL_POST_CONVOLUTION_COLOR_TABLE_SGI ||
          pname == GL_TEXTURE_COLOR_TABLE_SGI) {
         n[4].i = params[1];
         n[5].i = params[2];
         n[6].i = params[3];
      }
   }

   if (ctx->ExecuteFlag) {
      CALL_ColorTableParameteriv(ctx->Exec, (target, pname, params));
   }
}



static void GLAPIENTRY
save_ColorSubTable(GLenum target, GLsizei start, GLsizei count,
                   GLenum format, GLenum type, const GLvoid * table)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_COLOR_SUB_TABLE, 6);
   if (n) {
      n[1].e = target;
      n[2].i = start;
      n[3].i = count;
      n[4].e = format;
      n[5].e = type;
      n[6].data = unpack_image(ctx, 1, count, 1, 1, format, type, table,
                               &ctx->Unpack);
   }
   if (ctx->ExecuteFlag) {
      CALL_ColorSubTable(ctx->Exec,
                         (target, start, count, format, type, table));
   }
}


static void GLAPIENTRY
save_CopyColorSubTable(GLenum target, GLsizei start,
                       GLint x, GLint y, GLsizei width)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;

   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_COPY_COLOR_SUB_TABLE, 5);
   if (n) {
      n[1].e = target;
      n[2].i = start;
      n[3].i = x;
      n[4].i = y;
      n[5].i = width;
   }
   if (ctx->ExecuteFlag) {
      CALL_CopyColorSubTable(ctx->Exec, (target, start, x, y, width));
   }
}


static void GLAPIENTRY
save_CopyColorTable(GLenum target, GLenum internalformat,
                    GLint x, GLint y, GLsizei width)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;

   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_COPY_COLOR_TABLE, 5);
   if (n) {
      n[1].e = target;
      n[2].e = internalformat;
      n[3].i = x;
      n[4].i = y;
      n[5].i = width;
   }
   if (ctx->ExecuteFlag) {
      CALL_CopyColorTable(ctx->Exec, (target, internalformat, x, y, width));
   }
}


static void GLAPIENTRY
save_ConvolutionFilter1D(GLenum target, GLenum internalFormat, GLsizei width,
                         GLenum format, GLenum type, const GLvoid * filter)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;

   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);

   n = alloc_instruction(ctx, OPCODE_CONVOLUTION_FILTER_1D, 6);
   if (n) {
      n[1].e = target;
      n[2].e = internalFormat;
      n[3].i = width;
      n[4].e = format;
      n[5].e = type;
      n[6].data = unpack_image(ctx, 1, width, 1, 1, format, type, filter,
                               &ctx->Unpack);
   }
   if (ctx->ExecuteFlag) {
      CALL_ConvolutionFilter1D(ctx->Exec, (target, internalFormat, width,
                                           format, type, filter));
   }
}


static void GLAPIENTRY
save_ConvolutionFilter2D(GLenum target, GLenum internalFormat,
                         GLsizei width, GLsizei height, GLenum format,
                         GLenum type, const GLvoid * filter)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;

   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);

   n = alloc_instruction(ctx, OPCODE_CONVOLUTION_FILTER_2D, 7);
   if (n) {
      n[1].e = target;
      n[2].e = internalFormat;
      n[3].i = width;
      n[4].i = height;
      n[5].e = format;
      n[6].e = type;
      n[7].data = unpack_image(ctx, 2, width, height, 1, format, type, filter,
                               &ctx->Unpack);
   }
   if (ctx->ExecuteFlag) {
      CALL_ConvolutionFilter2D(ctx->Exec,
                               (target, internalFormat, width, height, format,
                                type, filter));
   }
}


static void GLAPIENTRY
save_ConvolutionParameteri(GLenum target, GLenum pname, GLint param)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_CONVOLUTION_PARAMETER_I, 3);
   if (n) {
      n[1].e = target;
      n[2].e = pname;
      n[3].i = param;
   }
   if (ctx->ExecuteFlag) {
      CALL_ConvolutionParameteri(ctx->Exec, (target, pname, param));
   }
}


static void GLAPIENTRY
save_ConvolutionParameteriv(GLenum target, GLenum pname, const GLint *params)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_CONVOLUTION_PARAMETER_IV, 6);
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
      CALL_ConvolutionParameteriv(ctx->Exec, (target, pname, params));
   }
}


static void GLAPIENTRY
save_ConvolutionParameterf(GLenum target, GLenum pname, GLfloat param)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_CONVOLUTION_PARAMETER_F, 3);
   if (n) {
      n[1].e = target;
      n[2].e = pname;
      n[3].f = param;
   }
   if (ctx->ExecuteFlag) {
      CALL_ConvolutionParameterf(ctx->Exec, (target, pname, param));
   }
}


static void GLAPIENTRY
save_ConvolutionParameterfv(GLenum target, GLenum pname,
                            const GLfloat *params)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_CONVOLUTION_PARAMETER_FV, 6);
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
      CALL_ConvolutionParameterfv(ctx->Exec, (target, pname, params));
   }
}


static void GLAPIENTRY
save_CopyPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum type)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_COPY_PIXELS, 5);
   if (n) {
      n[1].i = x;
      n[2].i = y;
      n[3].i = (GLint) width;
      n[4].i = (GLint) height;
      n[5].e = type;
   }
   if (ctx->ExecuteFlag) {
      CALL_CopyPixels(ctx->Exec, (x, y, width, height, type));
   }
}



static void GLAPIENTRY
save_CopyTexImage1D(GLenum target, GLint level, GLenum internalformat,
                    GLint x, GLint y, GLsizei width, GLint border)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_COPY_TEX_IMAGE1D, 7);
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
      CALL_CopyTexImage1D(ctx->Exec, (target, level, internalformat,
                                      x, y, width, border));
   }
}


static void GLAPIENTRY
save_CopyTexImage2D(GLenum target, GLint level,
                    GLenum internalformat,
                    GLint x, GLint y, GLsizei width,
                    GLsizei height, GLint border)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_COPY_TEX_IMAGE2D, 8);
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
      CALL_CopyTexImage2D(ctx->Exec, (target, level, internalformat,
                                      x, y, width, height, border));
   }
}



static void GLAPIENTRY
save_CopyTexSubImage1D(GLenum target, GLint level,
                       GLint xoffset, GLint x, GLint y, GLsizei width)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_COPY_TEX_SUB_IMAGE1D, 6);
   if (n) {
      n[1].e = target;
      n[2].i = level;
      n[3].i = xoffset;
      n[4].i = x;
      n[5].i = y;
      n[6].i = width;
   }
   if (ctx->ExecuteFlag) {
      CALL_CopyTexSubImage1D(ctx->Exec,
                             (target, level, xoffset, x, y, width));
   }
}


static void GLAPIENTRY
save_CopyTexSubImage2D(GLenum target, GLint level,
                       GLint xoffset, GLint yoffset,
                       GLint x, GLint y, GLsizei width, GLint height)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_COPY_TEX_SUB_IMAGE2D, 8);
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
      CALL_CopyTexSubImage2D(ctx->Exec, (target, level, xoffset, yoffset,
                                         x, y, width, height));
   }
}


static void GLAPIENTRY
save_CopyTexSubImage3D(GLenum target, GLint level,
                       GLint xoffset, GLint yoffset, GLint zoffset,
                       GLint x, GLint y, GLsizei width, GLint height)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_COPY_TEX_SUB_IMAGE3D, 9);
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
      CALL_CopyTexSubImage3D(ctx->Exec, (target, level,
                                         xoffset, yoffset, zoffset,
                                         x, y, width, height));
   }
}


static void GLAPIENTRY
save_CullFace(GLenum mode)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_CULL_FACE, 1);
   if (n) {
      n[1].e = mode;
   }
   if (ctx->ExecuteFlag) {
      CALL_CullFace(ctx->Exec, (mode));
   }
}


static void GLAPIENTRY
save_DepthFunc(GLenum func)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_DEPTH_FUNC, 1);
   if (n) {
      n[1].e = func;
   }
   if (ctx->ExecuteFlag) {
      CALL_DepthFunc(ctx->Exec, (func));
   }
}


static void GLAPIENTRY
save_DepthMask(GLboolean mask)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_DEPTH_MASK, 1);
   if (n) {
      n[1].b = mask;
   }
   if (ctx->ExecuteFlag) {
      CALL_DepthMask(ctx->Exec, (mask));
   }
}


static void GLAPIENTRY
save_DepthRange(GLclampd nearval, GLclampd farval)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_DEPTH_RANGE, 2);
   if (n) {
      n[1].f = (GLfloat) nearval;
      n[2].f = (GLfloat) farval;
   }
   if (ctx->ExecuteFlag) {
      CALL_DepthRange(ctx->Exec, (nearval, farval));
   }
}


static void GLAPIENTRY
save_Disable(GLenum cap)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_DISABLE, 1);
   if (n) {
      n[1].e = cap;
   }
   if (ctx->ExecuteFlag) {
      CALL_Disable(ctx->Exec, (cap));
   }
}


static void GLAPIENTRY
save_DisableIndexed(GLuint index, GLenum cap)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_DISABLE_INDEXED, 2);
   if (n) {
      n[1].ui = index;
      n[2].e = cap;
   }
   if (ctx->ExecuteFlag) {
      CALL_DisableIndexedEXT(ctx->Exec, (index, cap));
   }
}


static void GLAPIENTRY
save_DrawBuffer(GLenum mode)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_DRAW_BUFFER, 1);
   if (n) {
      n[1].e = mode;
   }
   if (ctx->ExecuteFlag) {
      CALL_DrawBuffer(ctx->Exec, (mode));
   }
}


static void GLAPIENTRY
save_DrawPixels(GLsizei width, GLsizei height,
                GLenum format, GLenum type, const GLvoid * pixels)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;

   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);

   n = alloc_instruction(ctx, OPCODE_DRAW_PIXELS, 5);
   if (n) {
      n[1].i = width;
      n[2].i = height;
      n[3].e = format;
      n[4].e = type;
      n[5].data = unpack_image(ctx, 2, width, height, 1, format, type,
                               pixels, &ctx->Unpack);
   }
   if (ctx->ExecuteFlag) {
      CALL_DrawPixels(ctx->Exec, (width, height, format, type, pixels));
   }
}



static void GLAPIENTRY
save_Enable(GLenum cap)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_ENABLE, 1);
   if (n) {
      n[1].e = cap;
   }
   if (ctx->ExecuteFlag) {
      CALL_Enable(ctx->Exec, (cap));
   }
}



static void GLAPIENTRY
save_EnableIndexed(GLuint index, GLenum cap)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_ENABLE_INDEXED, 2);
   if (n) {
      n[1].ui = index;
      n[2].e = cap;
   }
   if (ctx->ExecuteFlag) {
      CALL_EnableIndexedEXT(ctx->Exec, (index, cap));
   }
}



static void GLAPIENTRY
save_EvalMesh1(GLenum mode, GLint i1, GLint i2)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_EVALMESH1, 3);
   if (n) {
      n[1].e = mode;
      n[2].i = i1;
      n[3].i = i2;
   }
   if (ctx->ExecuteFlag) {
      CALL_EvalMesh1(ctx->Exec, (mode, i1, i2));
   }
}


static void GLAPIENTRY
save_EvalMesh2(GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_EVALMESH2, 5);
   if (n) {
      n[1].e = mode;
      n[2].i = i1;
      n[3].i = i2;
      n[4].i = j1;
      n[5].i = j2;
   }
   if (ctx->ExecuteFlag) {
      CALL_EvalMesh2(ctx->Exec, (mode, i1, i2, j1, j2));
   }
}




static void GLAPIENTRY
save_Fogfv(GLenum pname, const GLfloat *params)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_FOG, 5);
   if (n) {
      n[1].e = pname;
      n[2].f = params[0];
      n[3].f = params[1];
      n[4].f = params[2];
      n[5].f = params[3];
   }
   if (ctx->ExecuteFlag) {
      CALL_Fogfv(ctx->Exec, (pname, params));
   }
}


static void GLAPIENTRY
save_Fogf(GLenum pname, GLfloat param)
{
   GLfloat parray[4];
   parray[0] = param;
   parray[1] = parray[2] = parray[3] = 0.0F;
   save_Fogfv(pname, parray);
}


static void GLAPIENTRY
save_Fogiv(GLenum pname, const GLint *params)
{
   GLfloat p[4];
   switch (pname) {
   case GL_FOG_MODE:
   case GL_FOG_DENSITY:
   case GL_FOG_START:
   case GL_FOG_END:
   case GL_FOG_INDEX:
      p[0] = (GLfloat) *params;
      p[1] = 0.0f;
      p[2] = 0.0f;
      p[3] = 0.0f;
      break;
   case GL_FOG_COLOR:
      p[0] = INT_TO_FLOAT(params[0]);
      p[1] = INT_TO_FLOAT(params[1]);
      p[2] = INT_TO_FLOAT(params[2]);
      p[3] = INT_TO_FLOAT(params[3]);
      break;
   default:
      /* Error will be caught later in gl_Fogfv */
      ASSIGN_4V(p, 0.0F, 0.0F, 0.0F, 0.0F);
   }
   save_Fogfv(pname, p);
}


static void GLAPIENTRY
save_Fogi(GLenum pname, GLint param)
{
   GLint parray[4];
   parray[0] = param;
   parray[1] = parray[2] = parray[3] = 0;
   save_Fogiv(pname, parray);
}


static void GLAPIENTRY
save_FrontFace(GLenum mode)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_FRONT_FACE, 1);
   if (n) {
      n[1].e = mode;
   }
   if (ctx->ExecuteFlag) {
      CALL_FrontFace(ctx->Exec, (mode));
   }
}


static void GLAPIENTRY
save_Frustum(GLdouble left, GLdouble right,
             GLdouble bottom, GLdouble top, GLdouble nearval, GLdouble farval)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_FRUSTUM, 6);
   if (n) {
      n[1].f = (GLfloat) left;
      n[2].f = (GLfloat) right;
      n[3].f = (GLfloat) bottom;
      n[4].f = (GLfloat) top;
      n[5].f = (GLfloat) nearval;
      n[6].f = (GLfloat) farval;
   }
   if (ctx->ExecuteFlag) {
      CALL_Frustum(ctx->Exec, (left, right, bottom, top, nearval, farval));
   }
}


static void GLAPIENTRY
save_Hint(GLenum target, GLenum mode)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_HINT, 2);
   if (n) {
      n[1].e = target;
      n[2].e = mode;
   }
   if (ctx->ExecuteFlag) {
      CALL_Hint(ctx->Exec, (target, mode));
   }
}


static void GLAPIENTRY
save_Histogram(GLenum target, GLsizei width, GLenum internalFormat,
               GLboolean sink)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;

   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_HISTOGRAM, 4);
   if (n) {
      n[1].e = target;
      n[2].i = width;
      n[3].e = internalFormat;
      n[4].b = sink;
   }
   if (ctx->ExecuteFlag) {
      CALL_Histogram(ctx->Exec, (target, width, internalFormat, sink));
   }
}


static void GLAPIENTRY
save_IndexMask(GLuint mask)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_INDEX_MASK, 1);
   if (n) {
      n[1].ui = mask;
   }
   if (ctx->ExecuteFlag) {
      CALL_IndexMask(ctx->Exec, (mask));
   }
}


static void GLAPIENTRY
save_InitNames(void)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   (void) alloc_instruction(ctx, OPCODE_INIT_NAMES, 0);
   if (ctx->ExecuteFlag) {
      CALL_InitNames(ctx->Exec, ());
   }
}


static void GLAPIENTRY
save_Lightfv(GLenum light, GLenum pname, const GLfloat *params)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_LIGHT, 6);
   if (n) {
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
         n[3 + i].f = params[i];
      }
   }
   if (ctx->ExecuteFlag) {
      CALL_Lightfv(ctx->Exec, (light, pname, params));
   }
}


static void GLAPIENTRY
save_Lightf(GLenum light, GLenum pname, GLfloat param)
{
   GLfloat parray[4];
   parray[0] = param;
   parray[1] = parray[2] = parray[3] = 0.0F;
   save_Lightfv(light, pname, parray);
}


static void GLAPIENTRY
save_Lightiv(GLenum light, GLenum pname, const GLint *params)
{
   GLfloat fparam[4];
   switch (pname) {
   case GL_AMBIENT:
   case GL_DIFFUSE:
   case GL_SPECULAR:
      fparam[0] = INT_TO_FLOAT(params[0]);
      fparam[1] = INT_TO_FLOAT(params[1]);
      fparam[2] = INT_TO_FLOAT(params[2]);
      fparam[3] = INT_TO_FLOAT(params[3]);
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
   save_Lightfv(light, pname, fparam);
}


static void GLAPIENTRY
save_Lighti(GLenum light, GLenum pname, GLint param)
{
   GLint parray[4];
   parray[0] = param;
   parray[1] = parray[2] = parray[3] = 0;
   save_Lightiv(light, pname, parray);
}


static void GLAPIENTRY
save_LightModelfv(GLenum pname, const GLfloat *params)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_LIGHT_MODEL, 5);
   if (n) {
      n[1].e = pname;
      n[2].f = params[0];
      n[3].f = params[1];
      n[4].f = params[2];
      n[5].f = params[3];
   }
   if (ctx->ExecuteFlag) {
      CALL_LightModelfv(ctx->Exec, (pname, params));
   }
}


static void GLAPIENTRY
save_LightModelf(GLenum pname, GLfloat param)
{
   GLfloat parray[4];
   parray[0] = param;
   parray[1] = parray[2] = parray[3] = 0.0F;
   save_LightModelfv(pname, parray);
}


static void GLAPIENTRY
save_LightModeliv(GLenum pname, const GLint *params)
{
   GLfloat fparam[4];
   switch (pname) {
   case GL_LIGHT_MODEL_AMBIENT:
      fparam[0] = INT_TO_FLOAT(params[0]);
      fparam[1] = INT_TO_FLOAT(params[1]);
      fparam[2] = INT_TO_FLOAT(params[2]);
      fparam[3] = INT_TO_FLOAT(params[3]);
      break;
   case GL_LIGHT_MODEL_LOCAL_VIEWER:
   case GL_LIGHT_MODEL_TWO_SIDE:
   case GL_LIGHT_MODEL_COLOR_CONTROL:
      fparam[0] = (GLfloat) params[0];
      fparam[1] = 0.0F;
      fparam[2] = 0.0F;
      fparam[3] = 0.0F;
      break;
   default:
      /* Error will be caught later in gl_LightModelfv */
      ASSIGN_4V(fparam, 0.0F, 0.0F, 0.0F, 0.0F);
   }
   save_LightModelfv(pname, fparam);
}


static void GLAPIENTRY
save_LightModeli(GLenum pname, GLint param)
{
   GLint parray[4];
   parray[0] = param;
   parray[1] = parray[2] = parray[3] = 0;
   save_LightModeliv(pname, parray);
}


static void GLAPIENTRY
save_LineStipple(GLint factor, GLushort pattern)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_LINE_STIPPLE, 2);
   if (n) {
      n[1].i = factor;
      n[2].us = pattern;
   }
   if (ctx->ExecuteFlag) {
      CALL_LineStipple(ctx->Exec, (factor, pattern));
   }
}


static void GLAPIENTRY
save_LineWidth(GLfloat width)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_LINE_WIDTH, 1);
   if (n) {
      n[1].f = width;
   }
   if (ctx->ExecuteFlag) {
      CALL_LineWidth(ctx->Exec, (width));
   }
}


static void GLAPIENTRY
save_ListBase(GLuint base)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_LIST_BASE, 1);
   if (n) {
      n[1].ui = base;
   }
   if (ctx->ExecuteFlag) {
      CALL_ListBase(ctx->Exec, (base));
   }
}


static void GLAPIENTRY
save_LoadIdentity(void)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   (void) alloc_instruction(ctx, OPCODE_LOAD_IDENTITY, 0);
   if (ctx->ExecuteFlag) {
      CALL_LoadIdentity(ctx->Exec, ());
   }
}


static void GLAPIENTRY
save_LoadMatrixf(const GLfloat * m)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_LOAD_MATRIX, 16);
   if (n) {
      GLuint i;
      for (i = 0; i < 16; i++) {
         n[1 + i].f = m[i];
      }
   }
   if (ctx->ExecuteFlag) {
      CALL_LoadMatrixf(ctx->Exec, (m));
   }
}


static void GLAPIENTRY
save_LoadMatrixd(const GLdouble * m)
{
   GLfloat f[16];
   GLint i;
   for (i = 0; i < 16; i++) {
      f[i] = (GLfloat) m[i];
   }
   save_LoadMatrixf(f);
}


static void GLAPIENTRY
save_LoadName(GLuint name)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_LOAD_NAME, 1);
   if (n) {
      n[1].ui = name;
   }
   if (ctx->ExecuteFlag) {
      CALL_LoadName(ctx->Exec, (name));
   }
}


static void GLAPIENTRY
save_LogicOp(GLenum opcode)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_LOGIC_OP, 1);
   if (n) {
      n[1].e = opcode;
   }
   if (ctx->ExecuteFlag) {
      CALL_LogicOp(ctx->Exec, (opcode));
   }
}


static void GLAPIENTRY
save_Map1d(GLenum target, GLdouble u1, GLdouble u2, GLint stride,
           GLint order, const GLdouble * points)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_MAP1, 6);
   if (n) {
      GLfloat *pnts = _mesa_copy_map_points1d(target, stride, order, points);
      n[1].e = target;
      n[2].f = (GLfloat) u1;
      n[3].f = (GLfloat) u2;
      n[4].i = _mesa_evaluator_components(target);      /* stride */
      n[5].i = order;
      n[6].data = (void *) pnts;
   }
   if (ctx->ExecuteFlag) {
      CALL_Map1d(ctx->Exec, (target, u1, u2, stride, order, points));
   }
}

static void GLAPIENTRY
save_Map1f(GLenum target, GLfloat u1, GLfloat u2, GLint stride,
           GLint order, const GLfloat * points)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_MAP1, 6);
   if (n) {
      GLfloat *pnts = _mesa_copy_map_points1f(target, stride, order, points);
      n[1].e = target;
      n[2].f = u1;
      n[3].f = u2;
      n[4].i = _mesa_evaluator_components(target);      /* stride */
      n[5].i = order;
      n[6].data = (void *) pnts;
   }
   if (ctx->ExecuteFlag) {
      CALL_Map1f(ctx->Exec, (target, u1, u2, stride, order, points));
   }
}


static void GLAPIENTRY
save_Map2d(GLenum target,
           GLdouble u1, GLdouble u2, GLint ustride, GLint uorder,
           GLdouble v1, GLdouble v2, GLint vstride, GLint vorder,
           const GLdouble * points)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_MAP2, 10);
   if (n) {
      GLfloat *pnts = _mesa_copy_map_points2d(target, ustride, uorder,
                                              vstride, vorder, points);
      n[1].e = target;
      n[2].f = (GLfloat) u1;
      n[3].f = (GLfloat) u2;
      n[4].f = (GLfloat) v1;
      n[5].f = (GLfloat) v2;
      /* XXX verify these strides are correct */
      n[6].i = _mesa_evaluator_components(target) * vorder;     /*ustride */
      n[7].i = _mesa_evaluator_components(target);      /*vstride */
      n[8].i = uorder;
      n[9].i = vorder;
      n[10].data = (void *) pnts;
   }
   if (ctx->ExecuteFlag) {
      CALL_Map2d(ctx->Exec, (target,
                             u1, u2, ustride, uorder,
                             v1, v2, vstride, vorder, points));
   }
}


static void GLAPIENTRY
save_Map2f(GLenum target,
           GLfloat u1, GLfloat u2, GLint ustride, GLint uorder,
           GLfloat v1, GLfloat v2, GLint vstride, GLint vorder,
           const GLfloat * points)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_MAP2, 10);
   if (n) {
      GLfloat *pnts = _mesa_copy_map_points2f(target, ustride, uorder,
                                              vstride, vorder, points);
      n[1].e = target;
      n[2].f = u1;
      n[3].f = u2;
      n[4].f = v1;
      n[5].f = v2;
      /* XXX verify these strides are correct */
      n[6].i = _mesa_evaluator_components(target) * vorder;     /*ustride */
      n[7].i = _mesa_evaluator_components(target);      /*vstride */
      n[8].i = uorder;
      n[9].i = vorder;
      n[10].data = (void *) pnts;
   }
   if (ctx->ExecuteFlag) {
      CALL_Map2f(ctx->Exec, (target, u1, u2, ustride, uorder,
                             v1, v2, vstride, vorder, points));
   }
}


static void GLAPIENTRY
save_MapGrid1f(GLint un, GLfloat u1, GLfloat u2)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_MAPGRID1, 3);
   if (n) {
      n[1].i = un;
      n[2].f = u1;
      n[3].f = u2;
   }
   if (ctx->ExecuteFlag) {
      CALL_MapGrid1f(ctx->Exec, (un, u1, u2));
   }
}


static void GLAPIENTRY
save_MapGrid1d(GLint un, GLdouble u1, GLdouble u2)
{
   save_MapGrid1f(un, (GLfloat) u1, (GLfloat) u2);
}


static void GLAPIENTRY
save_MapGrid2f(GLint un, GLfloat u1, GLfloat u2,
               GLint vn, GLfloat v1, GLfloat v2)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_MAPGRID2, 6);
   if (n) {
      n[1].i = un;
      n[2].f = u1;
      n[3].f = u2;
      n[4].i = vn;
      n[5].f = v1;
      n[6].f = v2;
   }
   if (ctx->ExecuteFlag) {
      CALL_MapGrid2f(ctx->Exec, (un, u1, u2, vn, v1, v2));
   }
}



static void GLAPIENTRY
save_MapGrid2d(GLint un, GLdouble u1, GLdouble u2,
               GLint vn, GLdouble v1, GLdouble v2)
{
   save_MapGrid2f(un, (GLfloat) u1, (GLfloat) u2,
                  vn, (GLfloat) v1, (GLfloat) v2);
}


static void GLAPIENTRY
save_MatrixMode(GLenum mode)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_MATRIX_MODE, 1);
   if (n) {
      n[1].e = mode;
   }
   if (ctx->ExecuteFlag) {
      CALL_MatrixMode(ctx->Exec, (mode));
   }
}


static void GLAPIENTRY
save_Minmax(GLenum target, GLenum internalFormat, GLboolean sink)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;

   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_MIN_MAX, 3);
   if (n) {
      n[1].e = target;
      n[2].e = internalFormat;
      n[3].b = sink;
   }
   if (ctx->ExecuteFlag) {
      CALL_Minmax(ctx->Exec, (target, internalFormat, sink));
   }
}


static void GLAPIENTRY
save_MultMatrixf(const GLfloat * m)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_MULT_MATRIX, 16);
   if (n) {
      GLuint i;
      for (i = 0; i < 16; i++) {
         n[1 + i].f = m[i];
      }
   }
   if (ctx->ExecuteFlag) {
      CALL_MultMatrixf(ctx->Exec, (m));
   }
}


static void GLAPIENTRY
save_MultMatrixd(const GLdouble * m)
{
   GLfloat f[16];
   GLint i;
   for (i = 0; i < 16; i++) {
      f[i] = (GLfloat) m[i];
   }
   save_MultMatrixf(f);
}


static void GLAPIENTRY
save_NewList(GLuint name, GLenum mode)
{
   GET_CURRENT_CONTEXT(ctx);
   /* It's an error to call this function while building a display list */
   _mesa_error(ctx, GL_INVALID_OPERATION, "glNewList");
   (void) name;
   (void) mode;
}



static void GLAPIENTRY
save_Ortho(GLdouble left, GLdouble right,
           GLdouble bottom, GLdouble top, GLdouble nearval, GLdouble farval)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_ORTHO, 6);
   if (n) {
      n[1].f = (GLfloat) left;
      n[2].f = (GLfloat) right;
      n[3].f = (GLfloat) bottom;
      n[4].f = (GLfloat) top;
      n[5].f = (GLfloat) nearval;
      n[6].f = (GLfloat) farval;
   }
   if (ctx->ExecuteFlag) {
      CALL_Ortho(ctx->Exec, (left, right, bottom, top, nearval, farval));
   }
}


static void GLAPIENTRY
save_PixelMapfv(GLenum map, GLint mapsize, const GLfloat *values)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_PIXEL_MAP, 3);
   if (n) {
      n[1].e = map;
      n[2].i = mapsize;
      n[3].data = (void *) malloc(mapsize * sizeof(GLfloat));
      memcpy(n[3].data, (void *) values, mapsize * sizeof(GLfloat));
   }
   if (ctx->ExecuteFlag) {
      CALL_PixelMapfv(ctx->Exec, (map, mapsize, values));
   }
}


static void GLAPIENTRY
save_PixelMapuiv(GLenum map, GLint mapsize, const GLuint *values)
{
   GLfloat fvalues[MAX_PIXEL_MAP_TABLE];
   GLint i;
   if (map == GL_PIXEL_MAP_I_TO_I || map == GL_PIXEL_MAP_S_TO_S) {
      for (i = 0; i < mapsize; i++) {
         fvalues[i] = (GLfloat) values[i];
      }
   }
   else {
      for (i = 0; i < mapsize; i++) {
         fvalues[i] = UINT_TO_FLOAT(values[i]);
      }
   }
   save_PixelMapfv(map, mapsize, fvalues);
}


static void GLAPIENTRY
save_PixelMapusv(GLenum map, GLint mapsize, const GLushort *values)
{
   GLfloat fvalues[MAX_PIXEL_MAP_TABLE];
   GLint i;
   if (map == GL_PIXEL_MAP_I_TO_I || map == GL_PIXEL_MAP_S_TO_S) {
      for (i = 0; i < mapsize; i++) {
         fvalues[i] = (GLfloat) values[i];
      }
   }
   else {
      for (i = 0; i < mapsize; i++) {
         fvalues[i] = USHORT_TO_FLOAT(values[i]);
      }
   }
   save_PixelMapfv(map, mapsize, fvalues);
}


static void GLAPIENTRY
save_PixelTransferf(GLenum pname, GLfloat param)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_PIXEL_TRANSFER, 2);
   if (n) {
      n[1].e = pname;
      n[2].f = param;
   }
   if (ctx->ExecuteFlag) {
      CALL_PixelTransferf(ctx->Exec, (pname, param));
   }
}


static void GLAPIENTRY
save_PixelTransferi(GLenum pname, GLint param)
{
   save_PixelTransferf(pname, (GLfloat) param);
}


static void GLAPIENTRY
save_PixelZoom(GLfloat xfactor, GLfloat yfactor)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_PIXEL_ZOOM, 2);
   if (n) {
      n[1].f = xfactor;
      n[2].f = yfactor;
   }
   if (ctx->ExecuteFlag) {
      CALL_PixelZoom(ctx->Exec, (xfactor, yfactor));
   }
}


static void GLAPIENTRY
save_PointParameterfvEXT(GLenum pname, const GLfloat *params)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_POINT_PARAMETERS, 4);
   if (n) {
      n[1].e = pname;
      n[2].f = params[0];
      n[3].f = params[1];
      n[4].f = params[2];
   }
   if (ctx->ExecuteFlag) {
      CALL_PointParameterfvEXT(ctx->Exec, (pname, params));
   }
}


static void GLAPIENTRY
save_PointParameterfEXT(GLenum pname, GLfloat param)
{
   GLfloat parray[3];
   parray[0] = param;
   parray[1] = parray[2] = 0.0F;
   save_PointParameterfvEXT(pname, parray);
}

static void GLAPIENTRY
save_PointParameteriNV(GLenum pname, GLint param)
{
   GLfloat parray[3];
   parray[0] = (GLfloat) param;
   parray[1] = parray[2] = 0.0F;
   save_PointParameterfvEXT(pname, parray);
}

static void GLAPIENTRY
save_PointParameterivNV(GLenum pname, const GLint * param)
{
   GLfloat parray[3];
   parray[0] = (GLfloat) param[0];
   parray[1] = parray[2] = 0.0F;
   save_PointParameterfvEXT(pname, parray);
}


static void GLAPIENTRY
save_PointSize(GLfloat size)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_POINT_SIZE, 1);
   if (n) {
      n[1].f = size;
   }
   if (ctx->ExecuteFlag) {
      CALL_PointSize(ctx->Exec, (size));
   }
}


static void GLAPIENTRY
save_PolygonMode(GLenum face, GLenum mode)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_POLYGON_MODE, 2);
   if (n) {
      n[1].e = face;
      n[2].e = mode;
   }
   if (ctx->ExecuteFlag) {
      CALL_PolygonMode(ctx->Exec, (face, mode));
   }
}


static void GLAPIENTRY
save_PolygonStipple(const GLubyte * pattern)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;

   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);

   n = alloc_instruction(ctx, OPCODE_POLYGON_STIPPLE, 1);
   if (n) {
      n[1].data = unpack_image(ctx, 2, 32, 32, 1, GL_COLOR_INDEX, GL_BITMAP,
                               pattern, &ctx->Unpack);
   }
   if (ctx->ExecuteFlag) {
      CALL_PolygonStipple(ctx->Exec, ((GLubyte *) pattern));
   }
}


static void GLAPIENTRY
save_PolygonOffset(GLfloat factor, GLfloat units)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_POLYGON_OFFSET, 2);
   if (n) {
      n[1].f = factor;
      n[2].f = units;
   }
   if (ctx->ExecuteFlag) {
      CALL_PolygonOffset(ctx->Exec, (factor, units));
   }
}


static void GLAPIENTRY
save_PolygonOffsetEXT(GLfloat factor, GLfloat bias)
{
   GET_CURRENT_CONTEXT(ctx);
   /* XXX mult by DepthMaxF here??? */
   save_PolygonOffset(factor, ctx->DrawBuffer->_DepthMaxF * bias);
}


static void GLAPIENTRY
save_PopAttrib(void)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   (void) alloc_instruction(ctx, OPCODE_POP_ATTRIB, 0);
   if (ctx->ExecuteFlag) {
      CALL_PopAttrib(ctx->Exec, ());
   }
}


static void GLAPIENTRY
save_PopMatrix(void)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   (void) alloc_instruction(ctx, OPCODE_POP_MATRIX, 0);
   if (ctx->ExecuteFlag) {
      CALL_PopMatrix(ctx->Exec, ());
   }
}


static void GLAPIENTRY
save_PopName(void)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   (void) alloc_instruction(ctx, OPCODE_POP_NAME, 0);
   if (ctx->ExecuteFlag) {
      CALL_PopName(ctx->Exec, ());
   }
}


static void GLAPIENTRY
save_PrioritizeTextures(GLsizei num, const GLuint * textures,
                        const GLclampf * priorities)
{
   GET_CURRENT_CONTEXT(ctx);
   GLint i;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);

   for (i = 0; i < num; i++) {
      Node *n;
      n = alloc_instruction(ctx, OPCODE_PRIORITIZE_TEXTURE, 2);
      if (n) {
         n[1].ui = textures[i];
         n[2].f = priorities[i];
      }
   }
   if (ctx->ExecuteFlag) {
      CALL_PrioritizeTextures(ctx->Exec, (num, textures, priorities));
   }
}


static void GLAPIENTRY
save_PushAttrib(GLbitfield mask)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_PUSH_ATTRIB, 1);
   if (n) {
      n[1].bf = mask;
   }
   if (ctx->ExecuteFlag) {
      CALL_PushAttrib(ctx->Exec, (mask));
   }
}


static void GLAPIENTRY
save_PushMatrix(void)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   (void) alloc_instruction(ctx, OPCODE_PUSH_MATRIX, 0);
   if (ctx->ExecuteFlag) {
      CALL_PushMatrix(ctx->Exec, ());
   }
}


static void GLAPIENTRY
save_PushName(GLuint name)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_PUSH_NAME, 1);
   if (n) {
      n[1].ui = name;
   }
   if (ctx->ExecuteFlag) {
      CALL_PushName(ctx->Exec, (name));
   }
}


static void GLAPIENTRY
save_RasterPos4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_RASTER_POS, 4);
   if (n) {
      n[1].f = x;
      n[2].f = y;
      n[3].f = z;
      n[4].f = w;
   }
   if (ctx->ExecuteFlag) {
      CALL_RasterPos4f(ctx->Exec, (x, y, z, w));
   }
}

static void GLAPIENTRY
save_RasterPos2d(GLdouble x, GLdouble y)
{
   save_RasterPos4f((GLfloat) x, (GLfloat) y, 0.0F, 1.0F);
}

static void GLAPIENTRY
save_RasterPos2f(GLfloat x, GLfloat y)
{
   save_RasterPos4f(x, y, 0.0F, 1.0F);
}

static void GLAPIENTRY
save_RasterPos2i(GLint x, GLint y)
{
   save_RasterPos4f((GLfloat) x, (GLfloat) y, 0.0F, 1.0F);
}

static void GLAPIENTRY
save_RasterPos2s(GLshort x, GLshort y)
{
   save_RasterPos4f(x, y, 0.0F, 1.0F);
}

static void GLAPIENTRY
save_RasterPos3d(GLdouble x, GLdouble y, GLdouble z)
{
   save_RasterPos4f((GLfloat) x, (GLfloat) y, (GLfloat) z, 1.0F);
}

static void GLAPIENTRY
save_RasterPos3f(GLfloat x, GLfloat y, GLfloat z)
{
   save_RasterPos4f(x, y, z, 1.0F);
}

static void GLAPIENTRY
save_RasterPos3i(GLint x, GLint y, GLint z)
{
   save_RasterPos4f((GLfloat) x, (GLfloat) y, (GLfloat) z, 1.0F);
}

static void GLAPIENTRY
save_RasterPos3s(GLshort x, GLshort y, GLshort z)
{
   save_RasterPos4f(x, y, z, 1.0F);
}

static void GLAPIENTRY
save_RasterPos4d(GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
   save_RasterPos4f((GLfloat) x, (GLfloat) y, (GLfloat) z, (GLfloat) w);
}

static void GLAPIENTRY
save_RasterPos4i(GLint x, GLint y, GLint z, GLint w)
{
   save_RasterPos4f((GLfloat) x, (GLfloat) y, (GLfloat) z, (GLfloat) w);
}

static void GLAPIENTRY
save_RasterPos4s(GLshort x, GLshort y, GLshort z, GLshort w)
{
   save_RasterPos4f(x, y, z, w);
}

static void GLAPIENTRY
save_RasterPos2dv(const GLdouble * v)
{
   save_RasterPos4f((GLfloat) v[0], (GLfloat) v[1], 0.0F, 1.0F);
}

static void GLAPIENTRY
save_RasterPos2fv(const GLfloat * v)
{
   save_RasterPos4f(v[0], v[1], 0.0F, 1.0F);
}

static void GLAPIENTRY
save_RasterPos2iv(const GLint * v)
{
   save_RasterPos4f((GLfloat) v[0], (GLfloat) v[1], 0.0F, 1.0F);
}

static void GLAPIENTRY
save_RasterPos2sv(const GLshort * v)
{
   save_RasterPos4f(v[0], v[1], 0.0F, 1.0F);
}

static void GLAPIENTRY
save_RasterPos3dv(const GLdouble * v)
{
   save_RasterPos4f((GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2], 1.0F);
}

static void GLAPIENTRY
save_RasterPos3fv(const GLfloat * v)
{
   save_RasterPos4f(v[0], v[1], v[2], 1.0F);
}

static void GLAPIENTRY
save_RasterPos3iv(const GLint * v)
{
   save_RasterPos4f((GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2], 1.0F);
}

static void GLAPIENTRY
save_RasterPos3sv(const GLshort * v)
{
   save_RasterPos4f(v[0], v[1], v[2], 1.0F);
}

static void GLAPIENTRY
save_RasterPos4dv(const GLdouble * v)
{
   save_RasterPos4f((GLfloat) v[0], (GLfloat) v[1],
                    (GLfloat) v[2], (GLfloat) v[3]);
}

static void GLAPIENTRY
save_RasterPos4fv(const GLfloat * v)
{
   save_RasterPos4f(v[0], v[1], v[2], v[3]);
}

static void GLAPIENTRY
save_RasterPos4iv(const GLint * v)
{
   save_RasterPos4f((GLfloat) v[0], (GLfloat) v[1],
                    (GLfloat) v[2], (GLfloat) v[3]);
}

static void GLAPIENTRY
save_RasterPos4sv(const GLshort * v)
{
   save_RasterPos4f(v[0], v[1], v[2], v[3]);
}


static void GLAPIENTRY
save_PassThrough(GLfloat token)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_PASSTHROUGH, 1);
   if (n) {
      n[1].f = token;
   }
   if (ctx->ExecuteFlag) {
      CALL_PassThrough(ctx->Exec, (token));
   }
}


static void GLAPIENTRY
save_ReadBuffer(GLenum mode)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_READ_BUFFER, 1);
   if (n) {
      n[1].e = mode;
   }
   if (ctx->ExecuteFlag) {
      CALL_ReadBuffer(ctx->Exec, (mode));
   }
}


static void GLAPIENTRY
save_ResetHistogram(GLenum target)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_RESET_HISTOGRAM, 1);
   if (n) {
      n[1].e = target;
   }
   if (ctx->ExecuteFlag) {
      CALL_ResetHistogram(ctx->Exec, (target));
   }
}


static void GLAPIENTRY
save_ResetMinmax(GLenum target)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_RESET_MIN_MAX, 1);
   if (n) {
      n[1].e = target;
   }
   if (ctx->ExecuteFlag) {
      CALL_ResetMinmax(ctx->Exec, (target));
   }
}


static void GLAPIENTRY
save_Rotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_ROTATE, 4);
   if (n) {
      n[1].f = angle;
      n[2].f = x;
      n[3].f = y;
      n[4].f = z;
   }
   if (ctx->ExecuteFlag) {
      CALL_Rotatef(ctx->Exec, (angle, x, y, z));
   }
}


static void GLAPIENTRY
save_Rotated(GLdouble angle, GLdouble x, GLdouble y, GLdouble z)
{
   save_Rotatef((GLfloat) angle, (GLfloat) x, (GLfloat) y, (GLfloat) z);
}


static void GLAPIENTRY
save_Scalef(GLfloat x, GLfloat y, GLfloat z)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_SCALE, 3);
   if (n) {
      n[1].f = x;
      n[2].f = y;
      n[3].f = z;
   }
   if (ctx->ExecuteFlag) {
      CALL_Scalef(ctx->Exec, (x, y, z));
   }
}


static void GLAPIENTRY
save_Scaled(GLdouble x, GLdouble y, GLdouble z)
{
   save_Scalef((GLfloat) x, (GLfloat) y, (GLfloat) z);
}


static void GLAPIENTRY
save_Scissor(GLint x, GLint y, GLsizei width, GLsizei height)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_SCISSOR, 4);
   if (n) {
      n[1].i = x;
      n[2].i = y;
      n[3].i = width;
      n[4].i = height;
   }
   if (ctx->ExecuteFlag) {
      CALL_Scissor(ctx->Exec, (x, y, width, height));
   }
}


static void GLAPIENTRY
save_ShadeModel(GLenum mode)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END(ctx);

   if (ctx->ExecuteFlag) {
      CALL_ShadeModel(ctx->Exec, (mode));
   }

   if (ctx->ListState.Current.ShadeModel == mode)
      return;

   SAVE_FLUSH_VERTICES(ctx);

   /* Only save the value if we know the statechange will take effect:
    */
   if (ctx->Driver.CurrentSavePrimitive == PRIM_OUTSIDE_BEGIN_END)
      ctx->ListState.Current.ShadeModel = mode;

   n = alloc_instruction(ctx, OPCODE_SHADE_MODEL, 1);
   if (n) {
      n[1].e = mode;
   }
}


static void GLAPIENTRY
save_StencilFunc(GLenum func, GLint ref, GLuint mask)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_STENCIL_FUNC, 3);
   if (n) {
      n[1].e = func;
      n[2].i = ref;
      n[3].ui = mask;
   }
   if (ctx->ExecuteFlag) {
      CALL_StencilFunc(ctx->Exec, (func, ref, mask));
   }
}


static void GLAPIENTRY
save_StencilMask(GLuint mask)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_STENCIL_MASK, 1);
   if (n) {
      n[1].ui = mask;
   }
   if (ctx->ExecuteFlag) {
      CALL_StencilMask(ctx->Exec, (mask));
   }
}


static void GLAPIENTRY
save_StencilOp(GLenum fail, GLenum zfail, GLenum zpass)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_STENCIL_OP, 3);
   if (n) {
      n[1].e = fail;
      n[2].e = zfail;
      n[3].e = zpass;
   }
   if (ctx->ExecuteFlag) {
      CALL_StencilOp(ctx->Exec, (fail, zfail, zpass));
   }
}


static void GLAPIENTRY
save_StencilFuncSeparate(GLenum face, GLenum func, GLint ref, GLuint mask)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_STENCIL_FUNC_SEPARATE, 4);
   if (n) {
      n[1].e = face;
      n[2].e = func;
      n[3].i = ref;
      n[4].ui = mask;
   }
   if (ctx->ExecuteFlag) {
      CALL_StencilFuncSeparate(ctx->Exec, (face, func, ref, mask));
   }
}


static void GLAPIENTRY
save_StencilFuncSeparateATI(GLenum frontfunc, GLenum backfunc, GLint ref,
                            GLuint mask)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   /* GL_FRONT */
   n = alloc_instruction(ctx, OPCODE_STENCIL_FUNC_SEPARATE, 4);
   if (n) {
      n[1].e = GL_FRONT;
      n[2].e = frontfunc;
      n[3].i = ref;
      n[4].ui = mask;
   }
   /* GL_BACK */
   n = alloc_instruction(ctx, OPCODE_STENCIL_FUNC_SEPARATE, 4);
   if (n) {
      n[1].e = GL_BACK;
      n[2].e = backfunc;
      n[3].i = ref;
      n[4].ui = mask;
   }
   if (ctx->ExecuteFlag) {
      CALL_StencilFuncSeparate(ctx->Exec, (GL_FRONT, frontfunc, ref, mask));
      CALL_StencilFuncSeparate(ctx->Exec, (GL_BACK, backfunc, ref, mask));
   }
}


static void GLAPIENTRY
save_StencilMaskSeparate(GLenum face, GLuint mask)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_STENCIL_MASK_SEPARATE, 2);
   if (n) {
      n[1].e = face;
      n[2].ui = mask;
   }
   if (ctx->ExecuteFlag) {
      CALL_StencilMaskSeparate(ctx->Exec, (face, mask));
   }
}


static void GLAPIENTRY
save_StencilOpSeparate(GLenum face, GLenum fail, GLenum zfail, GLenum zpass)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_STENCIL_OP_SEPARATE, 4);
   if (n) {
      n[1].e = face;
      n[2].e = fail;
      n[3].e = zfail;
      n[4].e = zpass;
   }
   if (ctx->ExecuteFlag) {
      CALL_StencilOpSeparate(ctx->Exec, (face, fail, zfail, zpass));
   }
}


static void GLAPIENTRY
save_TexEnvfv(GLenum target, GLenum pname, const GLfloat *params)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_TEXENV, 6);
   if (n) {
      n[1].e = target;
      n[2].e = pname;
      if (pname == GL_TEXTURE_ENV_COLOR) {
         n[3].f = params[0];
         n[4].f = params[1];
         n[5].f = params[2];
         n[6].f = params[3];
      }
      else {
         n[3].f = params[0];
         n[4].f = n[5].f = n[6].f = 0.0F;
      }
   }
   if (ctx->ExecuteFlag) {
      CALL_TexEnvfv(ctx->Exec, (target, pname, params));
   }
}


static void GLAPIENTRY
save_TexEnvf(GLenum target, GLenum pname, GLfloat param)
{
   GLfloat parray[4];
   parray[0] = (GLfloat) param;
   parray[1] = parray[2] = parray[3] = 0.0F;
   save_TexEnvfv(target, pname, parray);
}


static void GLAPIENTRY
save_TexEnvi(GLenum target, GLenum pname, GLint param)
{
   GLfloat p[4];
   p[0] = (GLfloat) param;
   p[1] = p[2] = p[3] = 0.0F;
   save_TexEnvfv(target, pname, p);
}


static void GLAPIENTRY
save_TexEnviv(GLenum target, GLenum pname, const GLint * param)
{
   GLfloat p[4];
   if (pname == GL_TEXTURE_ENV_COLOR) {
      p[0] = INT_TO_FLOAT(param[0]);
      p[1] = INT_TO_FLOAT(param[1]);
      p[2] = INT_TO_FLOAT(param[2]);
      p[3] = INT_TO_FLOAT(param[3]);
   }
   else {
      p[0] = (GLfloat) param[0];
      p[1] = p[2] = p[3] = 0.0F;
   }
   save_TexEnvfv(target, pname, p);
}


static void GLAPIENTRY
save_TexGenfv(GLenum coord, GLenum pname, const GLfloat *params)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_TEXGEN, 6);
   if (n) {
      n[1].e = coord;
      n[2].e = pname;
      n[3].f = params[0];
      n[4].f = params[1];
      n[5].f = params[2];
      n[6].f = params[3];
   }
   if (ctx->ExecuteFlag) {
      CALL_TexGenfv(ctx->Exec, (coord, pname, params));
   }
}


static void GLAPIENTRY
save_TexGeniv(GLenum coord, GLenum pname, const GLint *params)
{
   GLfloat p[4];
   p[0] = (GLfloat) params[0];
   p[1] = (GLfloat) params[1];
   p[2] = (GLfloat) params[2];
   p[3] = (GLfloat) params[3];
   save_TexGenfv(coord, pname, p);
}


static void GLAPIENTRY
save_TexGend(GLenum coord, GLenum pname, GLdouble param)
{
   GLfloat parray[4];
   parray[0] = (GLfloat) param;
   parray[1] = parray[2] = parray[3] = 0.0F;
   save_TexGenfv(coord, pname, parray);
}


static void GLAPIENTRY
save_TexGendv(GLenum coord, GLenum pname, const GLdouble *params)
{
   GLfloat p[4];
   p[0] = (GLfloat) params[0];
   p[1] = (GLfloat) params[1];
   p[2] = (GLfloat) params[2];
   p[3] = (GLfloat) params[3];
   save_TexGenfv(coord, pname, p);
}


static void GLAPIENTRY
save_TexGenf(GLenum coord, GLenum pname, GLfloat param)
{
   GLfloat parray[4];
   parray[0] = param;
   parray[1] = parray[2] = parray[3] = 0.0F;
   save_TexGenfv(coord, pname, parray);
}


static void GLAPIENTRY
save_TexGeni(GLenum coord, GLenum pname, GLint param)
{
   GLint parray[4];
   parray[0] = param;
   parray[1] = parray[2] = parray[3] = 0;
   save_TexGeniv(coord, pname, parray);
}


static void GLAPIENTRY
save_TexParameterfv(GLenum target, GLenum pname, const GLfloat *params)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_TEXPARAMETER, 6);
   if (n) {
      n[1].e = target;
      n[2].e = pname;
      n[3].f = params[0];
      n[4].f = params[1];
      n[5].f = params[2];
      n[6].f = params[3];
   }
   if (ctx->ExecuteFlag) {
      CALL_TexParameterfv(ctx->Exec, (target, pname, params));
   }
}


static void GLAPIENTRY
save_TexParameterf(GLenum target, GLenum pname, GLfloat param)
{
   GLfloat parray[4];
   parray[0] = param;
   parray[1] = parray[2] = parray[3] = 0.0F;
   save_TexParameterfv(target, pname, parray);
}


static void GLAPIENTRY
save_TexParameteri(GLenum target, GLenum pname, GLint param)
{
   GLfloat fparam[4];
   fparam[0] = (GLfloat) param;
   fparam[1] = fparam[2] = fparam[3] = 0.0F;
   save_TexParameterfv(target, pname, fparam);
}


static void GLAPIENTRY
save_TexParameteriv(GLenum target, GLenum pname, const GLint *params)
{
   GLfloat fparam[4];
   fparam[0] = (GLfloat) params[0];
   fparam[1] = fparam[2] = fparam[3] = 0.0F;
   save_TexParameterfv(target, pname, fparam);
}


static void GLAPIENTRY
save_TexImage1D(GLenum target,
                GLint level, GLint components,
                GLsizei width, GLint border,
                GLenum format, GLenum type, const GLvoid * pixels)
{
   GET_CURRENT_CONTEXT(ctx);
   if (target == GL_PROXY_TEXTURE_1D) {
      /* don't compile, execute immediately */
      CALL_TexImage1D(ctx->Exec, (target, level, components, width,
                                  border, format, type, pixels));
   }
   else {
      Node *n;
      ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
      n = alloc_instruction(ctx, OPCODE_TEX_IMAGE1D, 8);
      if (n) {
         n[1].e = target;
         n[2].i = level;
         n[3].i = components;
         n[4].i = (GLint) width;
         n[5].i = border;
         n[6].e = format;
         n[7].e = type;
         n[8].data = unpack_image(ctx, 1, width, 1, 1, format, type,
                                  pixels, &ctx->Unpack);
      }
      if (ctx->ExecuteFlag) {
         CALL_TexImage1D(ctx->Exec, (target, level, components, width,
                                     border, format, type, pixels));
      }
   }
}


static void GLAPIENTRY
save_TexImage2D(GLenum target,
                GLint level, GLint components,
                GLsizei width, GLsizei height, GLint border,
                GLenum format, GLenum type, const GLvoid * pixels)
{
   GET_CURRENT_CONTEXT(ctx);
   if (target == GL_PROXY_TEXTURE_2D) {
      /* don't compile, execute immediately */
      CALL_TexImage2D(ctx->Exec, (target, level, components, width,
                                  height, border, format, type, pixels));
   }
   else {
      Node *n;
      ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
      n = alloc_instruction(ctx, OPCODE_TEX_IMAGE2D, 9);
      if (n) {
         n[1].e = target;
         n[2].i = level;
         n[3].i = components;
         n[4].i = (GLint) width;
         n[5].i = (GLint) height;
         n[6].i = border;
         n[7].e = format;
         n[8].e = type;
         n[9].data = unpack_image(ctx, 2, width, height, 1, format, type,
                                  pixels, &ctx->Unpack);
      }
      if (ctx->ExecuteFlag) {
         CALL_TexImage2D(ctx->Exec, (target, level, components, width,
                                     height, border, format, type, pixels));
      }
   }
}


static void GLAPIENTRY
save_TexImage3D(GLenum target,
                GLint level, GLint internalFormat,
                GLsizei width, GLsizei height, GLsizei depth,
                GLint border,
                GLenum format, GLenum type, const GLvoid * pixels)
{
   GET_CURRENT_CONTEXT(ctx);
   if (target == GL_PROXY_TEXTURE_3D) {
      /* don't compile, execute immediately */
      CALL_TexImage3D(ctx->Exec, (target, level, internalFormat, width,
                                  height, depth, border, format, type,
                                  pixels));
   }
   else {
      Node *n;
      ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
      n = alloc_instruction(ctx, OPCODE_TEX_IMAGE3D, 10);
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
         n[10].data = unpack_image(ctx, 3, width, height, depth, format, type,
                                   pixels, &ctx->Unpack);
      }
      if (ctx->ExecuteFlag) {
         CALL_TexImage3D(ctx->Exec, (target, level, internalFormat, width,
                                     height, depth, border, format, type,
                                     pixels));
      }
   }
}


static void GLAPIENTRY
save_TexSubImage1D(GLenum target, GLint level, GLint xoffset,
                   GLsizei width, GLenum format, GLenum type,
                   const GLvoid * pixels)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;

   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);

   n = alloc_instruction(ctx, OPCODE_TEX_SUB_IMAGE1D, 7);
   if (n) {
      n[1].e = target;
      n[2].i = level;
      n[3].i = xoffset;
      n[4].i = (GLint) width;
      n[5].e = format;
      n[6].e = type;
      n[7].data = unpack_image(ctx, 1, width, 1, 1, format, type,
                               pixels, &ctx->Unpack);
   }
   if (ctx->ExecuteFlag) {
      CALL_TexSubImage1D(ctx->Exec, (target, level, xoffset, width,
                                     format, type, pixels));
   }
}


static void GLAPIENTRY
save_TexSubImage2D(GLenum target, GLint level,
                   GLint xoffset, GLint yoffset,
                   GLsizei width, GLsizei height,
                   GLenum format, GLenum type, const GLvoid * pixels)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;

   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);

   n = alloc_instruction(ctx, OPCODE_TEX_SUB_IMAGE2D, 9);
   if (n) {
      n[1].e = target;
      n[2].i = level;
      n[3].i = xoffset;
      n[4].i = yoffset;
      n[5].i = (GLint) width;
      n[6].i = (GLint) height;
      n[7].e = format;
      n[8].e = type;
      n[9].data = unpack_image(ctx, 2, width, height, 1, format, type,
                               pixels, &ctx->Unpack);
   }
   if (ctx->ExecuteFlag) {
      CALL_TexSubImage2D(ctx->Exec, (target, level, xoffset, yoffset,
                                     width, height, format, type, pixels));
   }
}


static void GLAPIENTRY
save_TexSubImage3D(GLenum target, GLint level,
                   GLint xoffset, GLint yoffset, GLint zoffset,
                   GLsizei width, GLsizei height, GLsizei depth,
                   GLenum format, GLenum type, const GLvoid * pixels)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;

   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);

   n = alloc_instruction(ctx, OPCODE_TEX_SUB_IMAGE3D, 11);
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
      n[11].data = unpack_image(ctx, 3, width, height, depth, format, type,
                                pixels, &ctx->Unpack);
   }
   if (ctx->ExecuteFlag) {
      CALL_TexSubImage3D(ctx->Exec, (target, level,
                                     xoffset, yoffset, zoffset,
                                     width, height, depth, format, type,
                                     pixels));
   }
}


static void GLAPIENTRY
save_Translatef(GLfloat x, GLfloat y, GLfloat z)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_TRANSLATE, 3);
   if (n) {
      n[1].f = x;
      n[2].f = y;
      n[3].f = z;
   }
   if (ctx->ExecuteFlag) {
      CALL_Translatef(ctx->Exec, (x, y, z));
   }
}


static void GLAPIENTRY
save_Translated(GLdouble x, GLdouble y, GLdouble z)
{
   save_Translatef((GLfloat) x, (GLfloat) y, (GLfloat) z);
}



static void GLAPIENTRY
save_Viewport(GLint x, GLint y, GLsizei width, GLsizei height)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_VIEWPORT, 4);
   if (n) {
      n[1].i = x;
      n[2].i = y;
      n[3].i = (GLint) width;
      n[4].i = (GLint) height;
   }
   if (ctx->ExecuteFlag) {
      CALL_Viewport(ctx->Exec, (x, y, width, height));
   }
}


static void GLAPIENTRY
save_WindowPos4fMESA(GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_WINDOW_POS, 4);
   if (n) {
      n[1].f = x;
      n[2].f = y;
      n[3].f = z;
      n[4].f = w;
   }
   if (ctx->ExecuteFlag) {
      CALL_WindowPos4fMESA(ctx->Exec, (x, y, z, w));
   }
}

static void GLAPIENTRY
save_WindowPos2dMESA(GLdouble x, GLdouble y)
{
   save_WindowPos4fMESA((GLfloat) x, (GLfloat) y, 0.0F, 1.0F);
}

static void GLAPIENTRY
save_WindowPos2fMESA(GLfloat x, GLfloat y)
{
   save_WindowPos4fMESA(x, y, 0.0F, 1.0F);
}

static void GLAPIENTRY
save_WindowPos2iMESA(GLint x, GLint y)
{
   save_WindowPos4fMESA((GLfloat) x, (GLfloat) y, 0.0F, 1.0F);
}

static void GLAPIENTRY
save_WindowPos2sMESA(GLshort x, GLshort y)
{
   save_WindowPos4fMESA(x, y, 0.0F, 1.0F);
}

static void GLAPIENTRY
save_WindowPos3dMESA(GLdouble x, GLdouble y, GLdouble z)
{
   save_WindowPos4fMESA((GLfloat) x, (GLfloat) y, (GLfloat) z, 1.0F);
}

static void GLAPIENTRY
save_WindowPos3fMESA(GLfloat x, GLfloat y, GLfloat z)
{
   save_WindowPos4fMESA(x, y, z, 1.0F);
}

static void GLAPIENTRY
save_WindowPos3iMESA(GLint x, GLint y, GLint z)
{
   save_WindowPos4fMESA((GLfloat) x, (GLfloat) y, (GLfloat) z, 1.0F);
}

static void GLAPIENTRY
save_WindowPos3sMESA(GLshort x, GLshort y, GLshort z)
{
   save_WindowPos4fMESA(x, y, z, 1.0F);
}

static void GLAPIENTRY
save_WindowPos4dMESA(GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
   save_WindowPos4fMESA((GLfloat) x, (GLfloat) y, (GLfloat) z, (GLfloat) w);
}

static void GLAPIENTRY
save_WindowPos4iMESA(GLint x, GLint y, GLint z, GLint w)
{
   save_WindowPos4fMESA((GLfloat) x, (GLfloat) y, (GLfloat) z, (GLfloat) w);
}

static void GLAPIENTRY
save_WindowPos4sMESA(GLshort x, GLshort y, GLshort z, GLshort w)
{
   save_WindowPos4fMESA(x, y, z, w);
}

static void GLAPIENTRY
save_WindowPos2dvMESA(const GLdouble * v)
{
   save_WindowPos4fMESA((GLfloat) v[0], (GLfloat) v[1], 0.0F, 1.0F);
}

static void GLAPIENTRY
save_WindowPos2fvMESA(const GLfloat * v)
{
   save_WindowPos4fMESA(v[0], v[1], 0.0F, 1.0F);
}

static void GLAPIENTRY
save_WindowPos2ivMESA(const GLint * v)
{
   save_WindowPos4fMESA((GLfloat) v[0], (GLfloat) v[1], 0.0F, 1.0F);
}

static void GLAPIENTRY
save_WindowPos2svMESA(const GLshort * v)
{
   save_WindowPos4fMESA(v[0], v[1], 0.0F, 1.0F);
}

static void GLAPIENTRY
save_WindowPos3dvMESA(const GLdouble * v)
{
   save_WindowPos4fMESA((GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2], 1.0F);
}

static void GLAPIENTRY
save_WindowPos3fvMESA(const GLfloat * v)
{
   save_WindowPos4fMESA(v[0], v[1], v[2], 1.0F);
}

static void GLAPIENTRY
save_WindowPos3ivMESA(const GLint * v)
{
   save_WindowPos4fMESA((GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2], 1.0F);
}

static void GLAPIENTRY
save_WindowPos3svMESA(const GLshort * v)
{
   save_WindowPos4fMESA(v[0], v[1], v[2], 1.0F);
}

static void GLAPIENTRY
save_WindowPos4dvMESA(const GLdouble * v)
{
   save_WindowPos4fMESA((GLfloat) v[0], (GLfloat) v[1],
                        (GLfloat) v[2], (GLfloat) v[3]);
}

static void GLAPIENTRY
save_WindowPos4fvMESA(const GLfloat * v)
{
   save_WindowPos4fMESA(v[0], v[1], v[2], v[3]);
}

static void GLAPIENTRY
save_WindowPos4ivMESA(const GLint * v)
{
   save_WindowPos4fMESA((GLfloat) v[0], (GLfloat) v[1],
                        (GLfloat) v[2], (GLfloat) v[3]);
}

static void GLAPIENTRY
save_WindowPos4svMESA(const GLshort * v)
{
   save_WindowPos4fMESA(v[0], v[1], v[2], v[3]);
}



/* GL_ARB_multitexture */
static void GLAPIENTRY
save_ActiveTextureARB(GLenum target)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_ACTIVE_TEXTURE, 1);
   if (n) {
      n[1].e = target;
   }
   if (ctx->ExecuteFlag) {
      CALL_ActiveTextureARB(ctx->Exec, (target));
   }
}


/* GL_ARB_transpose_matrix */

static void GLAPIENTRY
save_LoadTransposeMatrixdARB(const GLdouble m[16])
{
   GLfloat tm[16];
   _math_transposefd(tm, m);
   save_LoadMatrixf(tm);
}


static void GLAPIENTRY
save_LoadTransposeMatrixfARB(const GLfloat m[16])
{
   GLfloat tm[16];
   _math_transposef(tm, m);
   save_LoadMatrixf(tm);
}


static void GLAPIENTRY
save_MultTransposeMatrixdARB(const GLdouble m[16])
{
   GLfloat tm[16];
   _math_transposefd(tm, m);
   save_MultMatrixf(tm);
}


static void GLAPIENTRY
save_MultTransposeMatrixfARB(const GLfloat m[16])
{
   GLfloat tm[16];
   _math_transposef(tm, m);
   save_MultMatrixf(tm);
}

static GLvoid *copy_data(const GLvoid *data, GLsizei size, const char *func)
{
   GET_CURRENT_CONTEXT(ctx);
   GLvoid *image;

   if (!data)
      return NULL;

   image = malloc(size);
   if (!image) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "%s", func);
      return NULL;
   }
   memcpy(image, data, size);

   return image;
}


/* GL_ARB_texture_compression */
static void GLAPIENTRY
save_CompressedTexImage1DARB(GLenum target, GLint level,
                             GLenum internalFormat, GLsizei width,
                             GLint border, GLsizei imageSize,
                             const GLvoid * data)
{
   GET_CURRENT_CONTEXT(ctx);
   if (target == GL_PROXY_TEXTURE_1D) {
      /* don't compile, execute immediately */
      CALL_CompressedTexImage1DARB(ctx->Exec, (target, level, internalFormat,
                                               width, border, imageSize,
                                               data));
   }
   else {
      Node *n;
      ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);

      n = alloc_instruction(ctx, OPCODE_COMPRESSED_TEX_IMAGE_1D, 7);
      if (n) {
         n[1].e = target;
         n[2].i = level;
         n[3].e = internalFormat;
         n[4].i = (GLint) width;
         n[5].i = border;
         n[6].i = imageSize;
         n[7].data = copy_data(data, imageSize, "glCompressedTexImage1DARB");
      }
      if (ctx->ExecuteFlag) {
         CALL_CompressedTexImage1DARB(ctx->Exec,
                                      (target, level, internalFormat, width,
                                       border, imageSize, data));
      }
   }
}


static void GLAPIENTRY
save_CompressedTexImage2DARB(GLenum target, GLint level,
                             GLenum internalFormat, GLsizei width,
                             GLsizei height, GLint border, GLsizei imageSize,
                             const GLvoid * data)
{
   GET_CURRENT_CONTEXT(ctx);
   if (target == GL_PROXY_TEXTURE_2D) {
      /* don't compile, execute immediately */
      CALL_CompressedTexImage2DARB(ctx->Exec, (target, level, internalFormat,
                                               width, height, border,
                                               imageSize, data));
   }
   else {
      Node *n;
      ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);

      n = alloc_instruction(ctx, OPCODE_COMPRESSED_TEX_IMAGE_2D, 8);
      if (n) {
         n[1].e = target;
         n[2].i = level;
         n[3].e = internalFormat;
         n[4].i = (GLint) width;
         n[5].i = (GLint) height;
         n[6].i = border;
         n[7].i = imageSize;
         n[8].data = copy_data(data, imageSize, "glCompressedTexImage2DARB");
      }
      if (ctx->ExecuteFlag) {
         CALL_CompressedTexImage2DARB(ctx->Exec,
                                      (target, level, internalFormat, width,
                                       height, border, imageSize, data));
      }
   }
}


static void GLAPIENTRY
save_CompressedTexImage3DARB(GLenum target, GLint level,
                             GLenum internalFormat, GLsizei width,
                             GLsizei height, GLsizei depth, GLint border,
                             GLsizei imageSize, const GLvoid * data)
{
   GET_CURRENT_CONTEXT(ctx);
   if (target == GL_PROXY_TEXTURE_3D) {
      /* don't compile, execute immediately */
      CALL_CompressedTexImage3DARB(ctx->Exec, (target, level, internalFormat,
                                               width, height, depth, border,
                                               imageSize, data));
   }
   else {
      Node *n;
      ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);

      n = alloc_instruction(ctx, OPCODE_COMPRESSED_TEX_IMAGE_3D, 9);
      if (n) {
         n[1].e = target;
         n[2].i = level;
         n[3].e = internalFormat;
         n[4].i = (GLint) width;
         n[5].i = (GLint) height;
         n[6].i = (GLint) depth;
         n[7].i = border;
         n[8].i = imageSize;
         n[9].data = copy_data(data, imageSize, "glCompressedTexImage3DARB");
      }
      if (ctx->ExecuteFlag) {
         CALL_CompressedTexImage3DARB(ctx->Exec,
                                      (target, level, internalFormat, width,
                                       height, depth, border, imageSize,
                                       data));
      }
   }
}


static void GLAPIENTRY
save_CompressedTexSubImage1DARB(GLenum target, GLint level, GLint xoffset,
                                GLsizei width, GLenum format,
                                GLsizei imageSize, const GLvoid * data)
{
   Node *n;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);

   n = alloc_instruction(ctx, OPCODE_COMPRESSED_TEX_SUB_IMAGE_1D, 7);
   if (n) {
      n[1].e = target;
      n[2].i = level;
      n[3].i = xoffset;
      n[4].i = (GLint) width;
      n[5].e = format;
      n[6].i = imageSize;
      n[7].data = copy_data(data, imageSize, "glCompressedTexSubImage1DARB");
   }
   if (ctx->ExecuteFlag) {
      CALL_CompressedTexSubImage1DARB(ctx->Exec, (target, level, xoffset,
                                                  width, format, imageSize,
                                                  data));
   }
}


static void GLAPIENTRY
save_CompressedTexSubImage2DARB(GLenum target, GLint level, GLint xoffset,
                                GLint yoffset, GLsizei width, GLsizei height,
                                GLenum format, GLsizei imageSize,
                                const GLvoid * data)
{
   Node *n;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);

   n = alloc_instruction(ctx, OPCODE_COMPRESSED_TEX_SUB_IMAGE_2D, 9);
   if (n) {
      n[1].e = target;
      n[2].i = level;
      n[3].i = xoffset;
      n[4].i = yoffset;
      n[5].i = (GLint) width;
      n[6].i = (GLint) height;
      n[7].e = format;
      n[8].i = imageSize;
      n[9].data = copy_data(data, imageSize, "glCompressedTexSubImage2DARB");
   }
   if (ctx->ExecuteFlag) {
      CALL_CompressedTexSubImage2DARB(ctx->Exec,
                                      (target, level, xoffset, yoffset, width,
                                       height, format, imageSize, data));
   }
}


static void GLAPIENTRY
save_CompressedTexSubImage3DARB(GLenum target, GLint level, GLint xoffset,
                                GLint yoffset, GLint zoffset, GLsizei width,
                                GLsizei height, GLsizei depth, GLenum format,
                                GLsizei imageSize, const GLvoid * data)
{
   Node *n;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);

   n = alloc_instruction(ctx, OPCODE_COMPRESSED_TEX_SUB_IMAGE_3D, 11);
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
      n[11].data = copy_data(data, imageSize, "glCompressedTexSubImage3DARB");
   }
   if (ctx->ExecuteFlag) {
      CALL_CompressedTexSubImage3DARB(ctx->Exec,
                                      (target, level, xoffset, yoffset,
                                       zoffset, width, height, depth, format,
                                       imageSize, data));
   }
}


/* GL_ARB_multisample */
static void GLAPIENTRY
save_SampleCoverageARB(GLclampf value, GLboolean invert)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_SAMPLE_COVERAGE, 2);
   if (n) {
      n[1].f = value;
      n[2].b = invert;
   }
   if (ctx->ExecuteFlag) {
      CALL_SampleCoverageARB(ctx->Exec, (value, invert));
   }
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
   n = alloc_instruction(ctx, OPCODE_BIND_PROGRAM_NV, 2);
   if (n) {
      n[1].e = target;
      n[2].ui = id;
   }
   if (ctx->ExecuteFlag) {
      CALL_BindProgramNV(ctx->Exec, (target, id));
   }
}

static void GLAPIENTRY
save_ProgramEnvParameter4fARB(GLenum target, GLuint index,
                              GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_PROGRAM_ENV_PARAMETER_ARB, 6);
   if (n) {
      n[1].e = target;
      n[2].ui = index;
      n[3].f = x;
      n[4].f = y;
      n[5].f = z;
      n[6].f = w;
   }
   if (ctx->ExecuteFlag) {
      CALL_ProgramEnvParameter4fARB(ctx->Exec, (target, index, x, y, z, w));
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
save_ProgramEnvParameters4fvEXT(GLenum target, GLuint index, GLsizei count,
				const GLfloat * params)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);

   if (count > 0) {
      GLint i;
      const GLfloat * p = params;

      for (i = 0 ; i < count ; i++) {
	 n = alloc_instruction(ctx, OPCODE_PROGRAM_ENV_PARAMETER_ARB, 6);
	 if (n) {
	    n[1].e = target;
	    n[2].ui = index;
	    n[3].f = p[0];
	    n[4].f = p[1];
	    n[5].f = p[2];
	    n[6].f = p[3];
	    p += 4;
	 }
      }
   }

   if (ctx->ExecuteFlag) {
      CALL_ProgramEnvParameters4fvEXT(ctx->Exec, (target, index, count, params));
   }
}


static void GLAPIENTRY
save_ProgramEnvParameter4dARB(GLenum target, GLuint index,
                              GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
   save_ProgramEnvParameter4fARB(target, index,
                                 (GLfloat) x,
                                 (GLfloat) y, (GLfloat) z, (GLfloat) w);
}


static void GLAPIENTRY
save_ProgramEnvParameter4dvARB(GLenum target, GLuint index,
                               const GLdouble *params)
{
   save_ProgramEnvParameter4fARB(target, index,
                                 (GLfloat) params[0],
                                 (GLfloat) params[1],
                                 (GLfloat) params[2], (GLfloat) params[3]);
}

#endif /* FEATURE_ARB_vertex_program || FEATURE_ARB_fragment_program || FEATURE_NV_vertex_program */

#if FEATURE_NV_vertex_program
static void GLAPIENTRY
save_ExecuteProgramNV(GLenum target, GLuint id, const GLfloat *params)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_EXECUTE_PROGRAM_NV, 6);
   if (n) {
      n[1].e = target;
      n[2].ui = id;
      n[3].f = params[0];
      n[4].f = params[1];
      n[5].f = params[2];
      n[6].f = params[3];
   }
   if (ctx->ExecuteFlag) {
      CALL_ExecuteProgramNV(ctx->Exec, (target, id, params));
   }
}


static void GLAPIENTRY
save_ProgramParameters4dvNV(GLenum target, GLuint index,
                            GLsizei num, const GLdouble *params)
{
   GLint i;
   for (i = 0; i < num; i++) {
      save_ProgramEnvParameter4dvARB(target, index + i, params + 4 * i);
   }
}


static void GLAPIENTRY
save_ProgramParameters4fvNV(GLenum target, GLuint index,
                            GLsizei num, const GLfloat *params)
{
   GLint i;
   for (i = 0; i < num; i++) {
      save_ProgramEnvParameter4fvARB(target, index + i, params + 4 * i);
   }
}


static void GLAPIENTRY
save_LoadProgramNV(GLenum target, GLuint id, GLsizei len,
                   const GLubyte * program)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;

   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);

   n = alloc_instruction(ctx, OPCODE_LOAD_PROGRAM_NV, 4);
   if (n) {
      GLubyte *programCopy = (GLubyte *) malloc(len);
      if (!programCopy) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glLoadProgramNV");
         return;
      }
      memcpy(programCopy, program, len);
      n[1].e = target;
      n[2].ui = id;
      n[3].i = len;
      n[4].data = programCopy;
   }
   if (ctx->ExecuteFlag) {
      CALL_LoadProgramNV(ctx->Exec, (target, id, len, program));
   }
}


static void GLAPIENTRY
save_RequestResidentProgramsNV(GLsizei num, const GLuint * ids)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;

   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);

   n = alloc_instruction(ctx, OPCODE_TRACK_MATRIX_NV, 2);
   if (n) {
      GLuint *idCopy = (GLuint *) malloc(num * sizeof(GLuint));
      if (!idCopy) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glRequestResidentProgramsNV");
         return;
      }
      memcpy(idCopy, ids, num * sizeof(GLuint));
      n[1].i = num;
      n[2].data = idCopy;
   }
   if (ctx->ExecuteFlag) {
      CALL_RequestResidentProgramsNV(ctx->Exec, (num, ids));
   }
}


static void GLAPIENTRY
save_TrackMatrixNV(GLenum target, GLuint address,
                   GLenum matrix, GLenum transform)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_TRACK_MATRIX_NV, 4);
   if (n) {
      n[1].e = target;
      n[2].ui = address;
      n[3].e = matrix;
      n[4].e = transform;
   }
   if (ctx->ExecuteFlag) {
      CALL_TrackMatrixNV(ctx->Exec, (target, address, matrix, transform));
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
   n = alloc_instruction(ctx, OPCODE_PROGRAM_LOCAL_PARAMETER_ARB, 6);
   if (n) {
      n[1].e = target;
      n[2].ui = index;
      n[3].f = x;
      n[4].f = y;
      n[5].f = z;
      n[6].f = w;
   }
   if (ctx->ExecuteFlag) {
      CALL_ProgramLocalParameter4fARB(ctx->Exec, (target, index, x, y, z, w));
   }
}


static void GLAPIENTRY
save_ProgramLocalParameter4fvARB(GLenum target, GLuint index,
                                 const GLfloat *params)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_PROGRAM_LOCAL_PARAMETER_ARB, 6);
   if (n) {
      n[1].e = target;
      n[2].ui = index;
      n[3].f = params[0];
      n[4].f = params[1];
      n[5].f = params[2];
      n[6].f = params[3];
   }
   if (ctx->ExecuteFlag) {
      CALL_ProgramLocalParameter4fvARB(ctx->Exec, (target, index, params));
   }
}


static void GLAPIENTRY
save_ProgramLocalParameters4fvEXT(GLenum target, GLuint index, GLsizei count,
				  const GLfloat *params)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);

   if (count > 0) {
      GLint i;
      const GLfloat * p = params;

      for (i = 0 ; i < count ; i++) {
	 n = alloc_instruction(ctx, OPCODE_PROGRAM_LOCAL_PARAMETER_ARB, 6);
	 if (n) {
	    n[1].e = target;
	    n[2].ui = index;
	    n[3].f = p[0];
	    n[4].f = p[1];
	    n[5].f = p[2];
	    n[6].f = p[3];
	    p += 4;
	 }
      }
   }

   if (ctx->ExecuteFlag) {
      CALL_ProgramLocalParameters4fvEXT(ctx->Exec, (target, index, count, params));
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
   n = alloc_instruction(ctx, OPCODE_PROGRAM_LOCAL_PARAMETER_ARB, 6);
   if (n) {
      n[1].e = target;
      n[2].ui = index;
      n[3].f = (GLfloat) x;
      n[4].f = (GLfloat) y;
      n[5].f = (GLfloat) z;
      n[6].f = (GLfloat) w;
   }
   if (ctx->ExecuteFlag) {
      CALL_ProgramLocalParameter4dARB(ctx->Exec, (target, index, x, y, z, w));
   }
}


static void GLAPIENTRY
save_ProgramLocalParameter4dvARB(GLenum target, GLuint index,
                                 const GLdouble *params)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_PROGRAM_LOCAL_PARAMETER_ARB, 6);
   if (n) {
      n[1].e = target;
      n[2].ui = index;
      n[3].f = (GLfloat) params[0];
      n[4].f = (GLfloat) params[1];
      n[5].f = (GLfloat) params[2];
      n[6].f = (GLfloat) params[3];
   }
   if (ctx->ExecuteFlag) {
      CALL_ProgramLocalParameter4dvARB(ctx->Exec, (target, index, params));
   }
}

static void GLAPIENTRY
save_ProgramNamedParameter4fNV(GLuint id, GLsizei len, const GLubyte * name,
                               GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;

   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);

   n = alloc_instruction(ctx, OPCODE_PROGRAM_NAMED_PARAMETER_NV, 6);
   if (n) {
      GLubyte *nameCopy = (GLubyte *) malloc(len);
      if (!nameCopy) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glProgramNamedParameter4fNV");
         return;
      }
      memcpy(nameCopy, name, len);
      n[1].ui = id;
      n[2].i = len;
      n[3].data = nameCopy;
      n[4].f = x;
      n[5].f = y;
      n[6].f = z;
      n[7].f = w;
   }
   if (ctx->ExecuteFlag) {
      CALL_ProgramNamedParameter4fNV(ctx->Exec, (id, len, name, x, y, z, w));
   }
}


static void GLAPIENTRY
save_ProgramNamedParameter4fvNV(GLuint id, GLsizei len, const GLubyte * name,
                                const float v[])
{
   save_ProgramNamedParameter4fNV(id, len, name, v[0], v[1], v[2], v[3]);
}


static void GLAPIENTRY
save_ProgramNamedParameter4dNV(GLuint id, GLsizei len, const GLubyte * name,
                               GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
   save_ProgramNamedParameter4fNV(id, len, name, (GLfloat) x, (GLfloat) y,
                                  (GLfloat) z, (GLfloat) w);
}


static void GLAPIENTRY
save_ProgramNamedParameter4dvNV(GLuint id, GLsizei len, const GLubyte * name,
                                const double v[])
{
   save_ProgramNamedParameter4fNV(id, len, name, (GLfloat) v[0],
                                  (GLfloat) v[1], (GLfloat) v[2],
                                  (GLfloat) v[3]);
}

#endif /* FEATURE_NV_fragment_program */



/* GL_EXT_stencil_two_side */
static void GLAPIENTRY
save_ActiveStencilFaceEXT(GLenum face)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_ACTIVE_STENCIL_FACE_EXT, 1);
   if (n) {
      n[1].e = face;
   }
   if (ctx->ExecuteFlag) {
      CALL_ActiveStencilFaceEXT(ctx->Exec, (face));
   }
}


/* GL_EXT_depth_bounds_test */
static void GLAPIENTRY
save_DepthBoundsEXT(GLclampd zmin, GLclampd zmax)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_DEPTH_BOUNDS_EXT, 2);
   if (n) {
      n[1].f = (GLfloat) zmin;
      n[2].f = (GLfloat) zmax;
   }
   if (ctx->ExecuteFlag) {
      CALL_DepthBoundsEXT(ctx->Exec, (zmin, zmax));
   }
}



#if FEATURE_ARB_vertex_program || FEATURE_ARB_fragment_program

static void GLAPIENTRY
save_ProgramStringARB(GLenum target, GLenum format, GLsizei len,
                      const GLvoid * string)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;

   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);

   n = alloc_instruction(ctx, OPCODE_PROGRAM_STRING_ARB, 4);
   if (n) {
      GLubyte *programCopy = (GLubyte *) malloc(len);
      if (!programCopy) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glProgramStringARB");
         return;
      }
      memcpy(programCopy, string, len);
      n[1].e = target;
      n[2].e = format;
      n[3].i = len;
      n[4].data = programCopy;
   }
   if (ctx->ExecuteFlag) {
      CALL_ProgramStringARB(ctx->Exec, (target, format, len, string));
   }
}

#endif /* FEATURE_ARB_vertex_program || FEATURE_ARB_fragment_program */


#if FEATURE_queryobj

static void GLAPIENTRY
save_BeginQueryARB(GLenum target, GLuint id)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_BEGIN_QUERY_ARB, 2);
   if (n) {
      n[1].e = target;
      n[2].ui = id;
   }
   if (ctx->ExecuteFlag) {
      CALL_BeginQueryARB(ctx->Exec, (target, id));
   }
}


static void GLAPIENTRY
save_EndQueryARB(GLenum target)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_END_QUERY_ARB, 1);
   if (n) {
      n[1].e = target;
   }
   if (ctx->ExecuteFlag) {
      CALL_EndQueryARB(ctx->Exec, (target));
   }
}

#endif /* FEATURE_queryobj */


static void GLAPIENTRY
save_DrawBuffersARB(GLsizei count, const GLenum * buffers)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_DRAW_BUFFERS_ARB, 1 + MAX_DRAW_BUFFERS);
   if (n) {
      GLint i;
      n[1].i = count;
      if (count > MAX_DRAW_BUFFERS)
         count = MAX_DRAW_BUFFERS;
      for (i = 0; i < count; i++) {
         n[2 + i].e = buffers[i];
      }
   }
   if (ctx->ExecuteFlag) {
      CALL_DrawBuffersARB(ctx->Exec, (count, buffers));
   }
}

static void GLAPIENTRY
save_TexBumpParameterfvATI(GLenum pname, const GLfloat *param)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;

   n = alloc_instruction(ctx, OPCODE_TEX_BUMP_PARAMETER_ATI, 5);
   if (n) {
      n[1].ui = pname;
      n[2].f = param[0];
      n[3].f = param[1];
      n[4].f = param[2];
      n[5].f = param[3];
   }
   if (ctx->ExecuteFlag) {
      CALL_TexBumpParameterfvATI(ctx->Exec, (pname, param));
   }
}

static void GLAPIENTRY
save_TexBumpParameterivATI(GLenum pname, const GLint *param)
{
   GLfloat p[4];
   p[0] = INT_TO_FLOAT(param[0]);
   p[1] = INT_TO_FLOAT(param[1]);
   p[2] = INT_TO_FLOAT(param[2]);
   p[3] = INT_TO_FLOAT(param[3]);
   save_TexBumpParameterfvATI(pname, p);
}

#if FEATURE_ATI_fragment_shader
static void GLAPIENTRY
save_BindFragmentShaderATI(GLuint id)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;

   n = alloc_instruction(ctx, OPCODE_BIND_FRAGMENT_SHADER_ATI, 1);
   if (n) {
      n[1].ui = id;
   }
   if (ctx->ExecuteFlag) {
      CALL_BindFragmentShaderATI(ctx->Exec, (id));
   }
}

static void GLAPIENTRY
save_SetFragmentShaderConstantATI(GLuint dst, const GLfloat *value)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;

   n = alloc_instruction(ctx, OPCODE_SET_FRAGMENT_SHADER_CONSTANTS_ATI, 5);
   if (n) {
      n[1].ui = dst;
      n[2].f = value[0];
      n[3].f = value[1];
      n[4].f = value[2];
      n[5].f = value[3];
   }
   if (ctx->ExecuteFlag) {
      CALL_SetFragmentShaderConstantATI(ctx->Exec, (dst, value));
   }
}
#endif

static void GLAPIENTRY
save_Attr1fNV(GLenum attr, GLfloat x)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   SAVE_FLUSH_VERTICES(ctx);
   n = alloc_instruction(ctx, OPCODE_ATTR_1F_NV, 2);
   if (n) {
      n[1].e = attr;
      n[2].f = x;
   }

   ASSERT(attr < MAX_VERTEX_GENERIC_ATTRIBS);
   ctx->ListState.ActiveAttribSize[attr] = 1;
   ASSIGN_4V(ctx->ListState.CurrentAttrib[attr], x, 0, 0, 1);

   if (ctx->ExecuteFlag) {
      CALL_VertexAttrib1fNV(ctx->Exec, (attr, x));
   }
}

static void GLAPIENTRY
save_Attr2fNV(GLenum attr, GLfloat x, GLfloat y)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   SAVE_FLUSH_VERTICES(ctx);
   n = alloc_instruction(ctx, OPCODE_ATTR_2F_NV, 3);
   if (n) {
      n[1].e = attr;
      n[2].f = x;
      n[3].f = y;
   }

   ASSERT(attr < MAX_VERTEX_GENERIC_ATTRIBS);
   ctx->ListState.ActiveAttribSize[attr] = 2;
   ASSIGN_4V(ctx->ListState.CurrentAttrib[attr], x, y, 0, 1);

   if (ctx->ExecuteFlag) {
      CALL_VertexAttrib2fNV(ctx->Exec, (attr, x, y));
   }
}

static void GLAPIENTRY
save_Attr3fNV(GLenum attr, GLfloat x, GLfloat y, GLfloat z)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   SAVE_FLUSH_VERTICES(ctx);
   n = alloc_instruction(ctx, OPCODE_ATTR_3F_NV, 4);
   if (n) {
      n[1].e = attr;
      n[2].f = x;
      n[3].f = y;
      n[4].f = z;
   }

   ASSERT(attr < MAX_VERTEX_GENERIC_ATTRIBS);
   ctx->ListState.ActiveAttribSize[attr] = 3;
   ASSIGN_4V(ctx->ListState.CurrentAttrib[attr], x, y, z, 1);

   if (ctx->ExecuteFlag) {
      CALL_VertexAttrib3fNV(ctx->Exec, (attr, x, y, z));
   }
}

static void GLAPIENTRY
save_Attr4fNV(GLenum attr, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   SAVE_FLUSH_VERTICES(ctx);
   n = alloc_instruction(ctx, OPCODE_ATTR_4F_NV, 5);
   if (n) {
      n[1].e = attr;
      n[2].f = x;
      n[3].f = y;
      n[4].f = z;
      n[5].f = w;
   }

   ASSERT(attr < MAX_VERTEX_GENERIC_ATTRIBS);
   ctx->ListState.ActiveAttribSize[attr] = 4;
   ASSIGN_4V(ctx->ListState.CurrentAttrib[attr], x, y, z, w);

   if (ctx->ExecuteFlag) {
      CALL_VertexAttrib4fNV(ctx->Exec, (attr, x, y, z, w));
   }
}


static void GLAPIENTRY
save_Attr1fARB(GLenum attr, GLfloat x)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   SAVE_FLUSH_VERTICES(ctx);
   n = alloc_instruction(ctx, OPCODE_ATTR_1F_ARB, 2);
   if (n) {
      n[1].e = attr;
      n[2].f = x;
   }

   ASSERT(attr < MAX_VERTEX_GENERIC_ATTRIBS);
   ctx->ListState.ActiveAttribSize[attr] = 1;
   ASSIGN_4V(ctx->ListState.CurrentAttrib[attr], x, 0, 0, 1);

   if (ctx->ExecuteFlag) {
      CALL_VertexAttrib1fARB(ctx->Exec, (attr, x));
   }
}

static void GLAPIENTRY
save_Attr2fARB(GLenum attr, GLfloat x, GLfloat y)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   SAVE_FLUSH_VERTICES(ctx);
   n = alloc_instruction(ctx, OPCODE_ATTR_2F_ARB, 3);
   if (n) {
      n[1].e = attr;
      n[2].f = x;
      n[3].f = y;
   }

   ASSERT(attr < MAX_VERTEX_GENERIC_ATTRIBS);
   ctx->ListState.ActiveAttribSize[attr] = 2;
   ASSIGN_4V(ctx->ListState.CurrentAttrib[attr], x, y, 0, 1);

   if (ctx->ExecuteFlag) {
      CALL_VertexAttrib2fARB(ctx->Exec, (attr, x, y));
   }
}

static void GLAPIENTRY
save_Attr3fARB(GLenum attr, GLfloat x, GLfloat y, GLfloat z)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   SAVE_FLUSH_VERTICES(ctx);
   n = alloc_instruction(ctx, OPCODE_ATTR_3F_ARB, 4);
   if (n) {
      n[1].e = attr;
      n[2].f = x;
      n[3].f = y;
      n[4].f = z;
   }

   ASSERT(attr < MAX_VERTEX_GENERIC_ATTRIBS);
   ctx->ListState.ActiveAttribSize[attr] = 3;
   ASSIGN_4V(ctx->ListState.CurrentAttrib[attr], x, y, z, 1);

   if (ctx->ExecuteFlag) {
      CALL_VertexAttrib3fARB(ctx->Exec, (attr, x, y, z));
   }
}

static void GLAPIENTRY
save_Attr4fARB(GLenum attr, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   SAVE_FLUSH_VERTICES(ctx);
   n = alloc_instruction(ctx, OPCODE_ATTR_4F_ARB, 5);
   if (n) {
      n[1].e = attr;
      n[2].f = x;
      n[3].f = y;
      n[4].f = z;
      n[5].f = w;
   }

   ASSERT(attr < MAX_VERTEX_GENERIC_ATTRIBS);
   ctx->ListState.ActiveAttribSize[attr] = 4;
   ASSIGN_4V(ctx->ListState.CurrentAttrib[attr], x, y, z, w);

   if (ctx->ExecuteFlag) {
      CALL_VertexAttrib4fARB(ctx->Exec, (attr, x, y, z, w));
   }
}


static void GLAPIENTRY
save_EvalCoord1f(GLfloat x)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   SAVE_FLUSH_VERTICES(ctx);
   n = alloc_instruction(ctx, OPCODE_EVAL_C1, 1);
   if (n) {
      n[1].f = x;
   }
   if (ctx->ExecuteFlag) {
      CALL_EvalCoord1f(ctx->Exec, (x));
   }
}

static void GLAPIENTRY
save_EvalCoord1fv(const GLfloat * v)
{
   save_EvalCoord1f(v[0]);
}

static void GLAPIENTRY
save_EvalCoord2f(GLfloat x, GLfloat y)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   SAVE_FLUSH_VERTICES(ctx);
   n = alloc_instruction(ctx, OPCODE_EVAL_C2, 2);
   if (n) {
      n[1].f = x;
      n[2].f = y;
   }
   if (ctx->ExecuteFlag) {
      CALL_EvalCoord2f(ctx->Exec, (x, y));
   }
}

static void GLAPIENTRY
save_EvalCoord2fv(const GLfloat * v)
{
   save_EvalCoord2f(v[0], v[1]);
}


static void GLAPIENTRY
save_EvalPoint1(GLint x)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   SAVE_FLUSH_VERTICES(ctx);
   n = alloc_instruction(ctx, OPCODE_EVAL_P1, 1);
   if (n) {
      n[1].i = x;
   }
   if (ctx->ExecuteFlag) {
      CALL_EvalPoint1(ctx->Exec, (x));
   }
}

static void GLAPIENTRY
save_EvalPoint2(GLint x, GLint y)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   SAVE_FLUSH_VERTICES(ctx);
   n = alloc_instruction(ctx, OPCODE_EVAL_P2, 2);
   if (n) {
      n[1].i = x;
      n[2].i = y;
   }
   if (ctx->ExecuteFlag) {
      CALL_EvalPoint2(ctx->Exec, (x, y));
   }
}

static void GLAPIENTRY
save_Indexf(GLfloat x)
{
   save_Attr1fNV(VERT_ATTRIB_COLOR_INDEX, x);
}

static void GLAPIENTRY
save_Indexfv(const GLfloat * v)
{
   save_Attr1fNV(VERT_ATTRIB_COLOR_INDEX, v[0]);
}

static void GLAPIENTRY
save_EdgeFlag(GLboolean x)
{
   save_Attr1fNV(VERT_ATTRIB_EDGEFLAG, x ? (GLfloat)1.0 : (GLfloat)0.0);
}

static inline GLboolean compare4fv( const GLfloat *a,
                                    const GLfloat *b,
                                    GLuint count )
{
   return memcmp( a, b, count * sizeof(GLfloat) ) == 0;
}
                              

static void GLAPIENTRY
save_Materialfv(GLenum face, GLenum pname, const GLfloat * param)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   int args, i;
   GLuint bitmask;

   switch (face) {
   case GL_BACK:
   case GL_FRONT:
   case GL_FRONT_AND_BACK:
      break;
   default:
      _mesa_compile_error(ctx, GL_INVALID_ENUM, "material(face)");
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
      _mesa_compile_error(ctx, GL_INVALID_ENUM, "material(pname)");
      return;
   }
   
   if (ctx->ExecuteFlag) {
      CALL_Materialfv(ctx->Exec, (face, pname, param));
   }

   bitmask = _mesa_material_bitmask(ctx, face, pname, ~0, NULL);

   /* Try to eliminate redundant statechanges.  Because it is legal to
    * call glMaterial even inside begin/end calls, don't need to worry
    * about ctx->Driver.CurrentSavePrimitive here.
    */
   for (i = 0; i < MAT_ATTRIB_MAX; i++) {
      if (bitmask & (1 << i)) {
         if (ctx->ListState.ActiveMaterialSize[i] == args &&
             compare4fv(ctx->ListState.CurrentMaterial[i], param, args)) {
            bitmask &= ~(1 << i);
         }
         else {
            ctx->ListState.ActiveMaterialSize[i] = args;
            COPY_SZ_4V(ctx->ListState.CurrentMaterial[i], args, param);
         }
      }
   }

   /* If this call has effect, return early:
    */
   if (bitmask == 0)
      return;

   SAVE_FLUSH_VERTICES(ctx);

   n = alloc_instruction(ctx, OPCODE_MATERIAL, 6);
   if (n) {
      n[1].e = face;
      n[2].e = pname;
      for (i = 0; i < args; i++)
         n[3 + i].f = param[i];
   }
}

static void GLAPIENTRY
save_Begin(GLenum mode)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   GLboolean error = GL_FALSE;

   if (!_mesa_valid_prim_mode(ctx, mode)) {
      _mesa_compile_error(ctx, GL_INVALID_ENUM, "glBegin(mode)");
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
      _mesa_compile_error(ctx, GL_INVALID_OPERATION, "recursive begin");
      error = GL_TRUE;
   }

   if (!error) {
      /* Give the driver an opportunity to hook in an optimized
       * display list compiler.
       */
      if (ctx->Driver.NotifySaveBegin(ctx, mode))
         return;

      SAVE_FLUSH_VERTICES(ctx);
      n = alloc_instruction(ctx, OPCODE_BEGIN, 1);
      if (n) {
         n[1].e = mode;
      }
   }

   if (ctx->ExecuteFlag) {
      CALL_Begin(ctx->Exec, (mode));
   }
}

static void GLAPIENTRY
save_End(void)
{
   GET_CURRENT_CONTEXT(ctx);
   SAVE_FLUSH_VERTICES(ctx);
   (void) alloc_instruction(ctx, OPCODE_END, 0);
   ctx->Driver.CurrentSavePrimitive = PRIM_OUTSIDE_BEGIN_END;
   if (ctx->ExecuteFlag) {
      CALL_End(ctx->Exec, ());
   }
}

static void GLAPIENTRY
save_Rectf(GLfloat a, GLfloat b, GLfloat c, GLfloat d)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   SAVE_FLUSH_VERTICES(ctx);
   n = alloc_instruction(ctx, OPCODE_RECTF, 4);
   if (n) {
      n[1].f = a;
      n[2].f = b;
      n[3].f = c;
      n[4].f = d;
   }
   if (ctx->ExecuteFlag) {
      CALL_Rectf(ctx->Exec, (a, b, c, d));
   }
}


static void GLAPIENTRY
save_Vertex2f(GLfloat x, GLfloat y)
{
   save_Attr2fNV(VERT_ATTRIB_POS, x, y);
}

static void GLAPIENTRY
save_Vertex2fv(const GLfloat * v)
{
   save_Attr2fNV(VERT_ATTRIB_POS, v[0], v[1]);
}

static void GLAPIENTRY
save_Vertex3f(GLfloat x, GLfloat y, GLfloat z)
{
   save_Attr3fNV(VERT_ATTRIB_POS, x, y, z);
}

static void GLAPIENTRY
save_Vertex3fv(const GLfloat * v)
{
   save_Attr3fNV(VERT_ATTRIB_POS, v[0], v[1], v[2]);
}

static void GLAPIENTRY
save_Vertex4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
   save_Attr4fNV(VERT_ATTRIB_POS, x, y, z, w);
}

static void GLAPIENTRY
save_Vertex4fv(const GLfloat * v)
{
   save_Attr4fNV(VERT_ATTRIB_POS, v[0], v[1], v[2], v[3]);
}

static void GLAPIENTRY
save_TexCoord1f(GLfloat x)
{
   save_Attr1fNV(VERT_ATTRIB_TEX0, x);
}

static void GLAPIENTRY
save_TexCoord1fv(const GLfloat * v)
{
   save_Attr1fNV(VERT_ATTRIB_TEX0, v[0]);
}

static void GLAPIENTRY
save_TexCoord2f(GLfloat x, GLfloat y)
{
   save_Attr2fNV(VERT_ATTRIB_TEX0, x, y);
}

static void GLAPIENTRY
save_TexCoord2fv(const GLfloat * v)
{
   save_Attr2fNV(VERT_ATTRIB_TEX0, v[0], v[1]);
}

static void GLAPIENTRY
save_TexCoord3f(GLfloat x, GLfloat y, GLfloat z)
{
   save_Attr3fNV(VERT_ATTRIB_TEX0, x, y, z);
}

static void GLAPIENTRY
save_TexCoord3fv(const GLfloat * v)
{
   save_Attr3fNV(VERT_ATTRIB_TEX0, v[0], v[1], v[2]);
}

static void GLAPIENTRY
save_TexCoord4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
   save_Attr4fNV(VERT_ATTRIB_TEX0, x, y, z, w);
}

static void GLAPIENTRY
save_TexCoord4fv(const GLfloat * v)
{
   save_Attr4fNV(VERT_ATTRIB_TEX0, v[0], v[1], v[2], v[3]);
}

static void GLAPIENTRY
save_Normal3f(GLfloat x, GLfloat y, GLfloat z)
{
   save_Attr3fNV(VERT_ATTRIB_NORMAL, x, y, z);
}

static void GLAPIENTRY
save_Normal3fv(const GLfloat * v)
{
   save_Attr3fNV(VERT_ATTRIB_NORMAL, v[0], v[1], v[2]);
}

static void GLAPIENTRY
save_FogCoordfEXT(GLfloat x)
{
   save_Attr1fNV(VERT_ATTRIB_FOG, x);
}

static void GLAPIENTRY
save_FogCoordfvEXT(const GLfloat * v)
{
   save_Attr1fNV(VERT_ATTRIB_FOG, v[0]);
}

static void GLAPIENTRY
save_Color3f(GLfloat x, GLfloat y, GLfloat z)
{
   save_Attr3fNV(VERT_ATTRIB_COLOR0, x, y, z);
}

static void GLAPIENTRY
save_Color3fv(const GLfloat * v)
{
   save_Attr3fNV(VERT_ATTRIB_COLOR0, v[0], v[1], v[2]);
}

static void GLAPIENTRY
save_Color4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
   save_Attr4fNV(VERT_ATTRIB_COLOR0, x, y, z, w);
}

static void GLAPIENTRY
save_Color4fv(const GLfloat * v)
{
   save_Attr4fNV(VERT_ATTRIB_COLOR0, v[0], v[1], v[2], v[3]);
}

static void GLAPIENTRY
save_SecondaryColor3fEXT(GLfloat x, GLfloat y, GLfloat z)
{
   save_Attr3fNV(VERT_ATTRIB_COLOR1, x, y, z);
}

static void GLAPIENTRY
save_SecondaryColor3fvEXT(const GLfloat * v)
{
   save_Attr3fNV(VERT_ATTRIB_COLOR1, v[0], v[1], v[2]);
}


/* Just call the respective ATTR for texcoord
 */
static void GLAPIENTRY
save_MultiTexCoord1f(GLenum target, GLfloat x)
{
   GLuint attr = (target & 0x7) + VERT_ATTRIB_TEX0;
   save_Attr1fNV(attr, x);
}

static void GLAPIENTRY
save_MultiTexCoord1fv(GLenum target, const GLfloat * v)
{
   GLuint attr = (target & 0x7) + VERT_ATTRIB_TEX0;
   save_Attr1fNV(attr, v[0]);
}

static void GLAPIENTRY
save_MultiTexCoord2f(GLenum target, GLfloat x, GLfloat y)
{
   GLuint attr = (target & 0x7) + VERT_ATTRIB_TEX0;
   save_Attr2fNV(attr, x, y);
}

static void GLAPIENTRY
save_MultiTexCoord2fv(GLenum target, const GLfloat * v)
{
   GLuint attr = (target & 0x7) + VERT_ATTRIB_TEX0;
   save_Attr2fNV(attr, v[0], v[1]);
}

static void GLAPIENTRY
save_MultiTexCoord3f(GLenum target, GLfloat x, GLfloat y, GLfloat z)
{
   GLuint attr = (target & 0x7) + VERT_ATTRIB_TEX0;
   save_Attr3fNV(attr, x, y, z);
}

static void GLAPIENTRY
save_MultiTexCoord3fv(GLenum target, const GLfloat * v)
{
   GLuint attr = (target & 0x7) + VERT_ATTRIB_TEX0;
   save_Attr3fNV(attr, v[0], v[1], v[2]);
}

static void GLAPIENTRY
save_MultiTexCoord4f(GLenum target, GLfloat x, GLfloat y,
                     GLfloat z, GLfloat w)
{
   GLuint attr = (target & 0x7) + VERT_ATTRIB_TEX0;
   save_Attr4fNV(attr, x, y, z, w);
}

static void GLAPIENTRY
save_MultiTexCoord4fv(GLenum target, const GLfloat * v)
{
   GLuint attr = (target & 0x7) + VERT_ATTRIB_TEX0;
   save_Attr4fNV(attr, v[0], v[1], v[2], v[3]);
}


/**
 * Record a GL_INVALID_VALUE error when a invalid vertex attribute
 * index is found.
 */
static void
index_error(void)
{
   GET_CURRENT_CONTEXT(ctx);
   _mesa_error(ctx, GL_INVALID_VALUE, "VertexAttribf(index)");
}


/* First level for NV_vertex_program:
 *
 * Check for errors at compile time?.
 */
static void GLAPIENTRY
save_VertexAttrib1fNV(GLuint index, GLfloat x)
{
   if (index < MAX_NV_VERTEX_PROGRAM_INPUTS)
      save_Attr1fNV(index, x);
   else
      index_error();
}

static void GLAPIENTRY
save_VertexAttrib1fvNV(GLuint index, const GLfloat * v)
{
   if (index < MAX_NV_VERTEX_PROGRAM_INPUTS)
      save_Attr1fNV(index, v[0]);
   else
      index_error();
}

static void GLAPIENTRY
save_VertexAttrib2fNV(GLuint index, GLfloat x, GLfloat y)
{
   if (index < MAX_NV_VERTEX_PROGRAM_INPUTS)
      save_Attr2fNV(index, x, y);
   else
      index_error();
}

static void GLAPIENTRY
save_VertexAttrib2fvNV(GLuint index, const GLfloat * v)
{
   if (index < MAX_NV_VERTEX_PROGRAM_INPUTS)
      save_Attr2fNV(index, v[0], v[1]);
   else
      index_error();
}

static void GLAPIENTRY
save_VertexAttrib3fNV(GLuint index, GLfloat x, GLfloat y, GLfloat z)
{
   if (index < MAX_NV_VERTEX_PROGRAM_INPUTS)
      save_Attr3fNV(index, x, y, z);
   else
      index_error();
}

static void GLAPIENTRY
save_VertexAttrib3fvNV(GLuint index, const GLfloat * v)
{
   if (index < MAX_NV_VERTEX_PROGRAM_INPUTS)
      save_Attr3fNV(index, v[0], v[1], v[2]);
   else
      index_error();
}

static void GLAPIENTRY
save_VertexAttrib4fNV(GLuint index, GLfloat x, GLfloat y,
                      GLfloat z, GLfloat w)
{
   if (index < MAX_NV_VERTEX_PROGRAM_INPUTS)
      save_Attr4fNV(index, x, y, z, w);
   else
      index_error();
}

static void GLAPIENTRY
save_VertexAttrib4fvNV(GLuint index, const GLfloat * v)
{
   if (index < MAX_NV_VERTEX_PROGRAM_INPUTS)
      save_Attr4fNV(index, v[0], v[1], v[2], v[3]);
   else
      index_error();
}




static void GLAPIENTRY
save_VertexAttrib1fARB(GLuint index, GLfloat x)
{
   if (index < MAX_VERTEX_GENERIC_ATTRIBS)
      save_Attr1fARB(index, x);
   else
      index_error();
}

static void GLAPIENTRY
save_VertexAttrib1fvARB(GLuint index, const GLfloat * v)
{
   if (index < MAX_VERTEX_GENERIC_ATTRIBS)
      save_Attr1fARB(index, v[0]);
   else
      index_error();
}

static void GLAPIENTRY
save_VertexAttrib2fARB(GLuint index, GLfloat x, GLfloat y)
{
   if (index < MAX_VERTEX_GENERIC_ATTRIBS)
      save_Attr2fARB(index, x, y);
   else
      index_error();
}

static void GLAPIENTRY
save_VertexAttrib2fvARB(GLuint index, const GLfloat * v)
{
   if (index < MAX_VERTEX_GENERIC_ATTRIBS)
      save_Attr2fARB(index, v[0], v[1]);
   else
      index_error();
}

static void GLAPIENTRY
save_VertexAttrib3fARB(GLuint index, GLfloat x, GLfloat y, GLfloat z)
{
   if (index < MAX_VERTEX_GENERIC_ATTRIBS)
      save_Attr3fARB(index, x, y, z);
   else
      index_error();
}

static void GLAPIENTRY
save_VertexAttrib3fvARB(GLuint index, const GLfloat * v)
{
   if (index < MAX_VERTEX_GENERIC_ATTRIBS)
      save_Attr3fARB(index, v[0], v[1], v[2]);
   else
      index_error();
}

static void GLAPIENTRY
save_VertexAttrib4fARB(GLuint index, GLfloat x, GLfloat y, GLfloat z,
                       GLfloat w)
{
   if (index < MAX_VERTEX_GENERIC_ATTRIBS)
      save_Attr4fARB(index, x, y, z, w);
   else
      index_error();
}

static void GLAPIENTRY
save_VertexAttrib4fvARB(GLuint index, const GLfloat * v)
{
   if (index < MAX_VERTEX_GENERIC_ATTRIBS)
      save_Attr4fARB(index, v[0], v[1], v[2], v[3]);
   else
      index_error();
}


/* GL_ARB_shader_objects, GL_ARB_vertex/fragment_shader */

static void GLAPIENTRY
exec_BindAttribLocationARB(GLuint program, GLuint index, const GLchar *name)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   CALL_BindAttribLocationARB(ctx->Exec, (program, index, name));
}

static GLint GLAPIENTRY
exec_GetAttribLocationARB(GLuint program, const GLchar *name)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   return CALL_GetAttribLocationARB(ctx->Exec, (program, name));
}

static GLint GLAPIENTRY
exec_GetUniformLocationARB(GLuint program, const GLchar *name)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   return CALL_GetUniformLocationARB(ctx->Exec, (program, name));
}
/* XXX more shader functions needed here */


#if FEATURE_EXT_framebuffer_blit
static void GLAPIENTRY
save_BlitFramebufferEXT(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1,
                        GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1,
                        GLbitfield mask, GLenum filter)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_BLIT_FRAMEBUFFER, 10);
   if (n) {
      n[1].i = srcX0;
      n[2].i = srcY0;
      n[3].i = srcX1;
      n[4].i = srcY1;
      n[5].i = dstX0;
      n[6].i = dstY0;
      n[7].i = dstX1;
      n[8].i = dstY1;
      n[9].i = mask;
      n[10].e = filter;
   }
   if (ctx->ExecuteFlag) {
      CALL_BlitFramebufferEXT(ctx->Exec, (srcX0, srcY0, srcX1, srcY1,
                                          dstX0, dstY0, dstX1, dstY1,
                                          mask, filter));
   }
}
#endif


/** GL_EXT_provoking_vertex */
static void GLAPIENTRY
save_ProvokingVertexEXT(GLenum mode)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_PROVOKING_VERTEX, 1);
   if (n) {
      n[1].e = mode;
   }
   if (ctx->ExecuteFlag) {
      /*CALL_ProvokingVertexEXT(ctx->Exec, (mode));*/
      _mesa_ProvokingVertexEXT(mode);
   }
}


/** GL_EXT_transform_feedback */
static void GLAPIENTRY
save_BeginTransformFeedback(GLenum mode)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_BEGIN_TRANSFORM_FEEDBACK, 1);
   if (n) {
      n[1].e = mode;
   }
   if (ctx->ExecuteFlag) {
      CALL_BeginTransformFeedbackEXT(ctx->Exec, (mode));
   }
}


/** GL_EXT_transform_feedback */
static void GLAPIENTRY
save_EndTransformFeedback(void)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   (void) alloc_instruction(ctx, OPCODE_END_TRANSFORM_FEEDBACK, 0);
   if (ctx->ExecuteFlag) {
      CALL_EndTransformFeedbackEXT(ctx->Exec, ());
   }
}

static void GLAPIENTRY
save_BindTransformFeedback(GLenum target, GLuint name)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_BIND_TRANSFORM_FEEDBACK, 2);
   if (n) {
      n[1].e = target;
      n[2].ui = name;
   }
   if (ctx->ExecuteFlag) {
      CALL_BindTransformFeedback(ctx->Exec, (target, name));
   }
}

static void GLAPIENTRY
save_PauseTransformFeedback(void)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   (void) alloc_instruction(ctx, OPCODE_PAUSE_TRANSFORM_FEEDBACK, 0);
   if (ctx->ExecuteFlag) {
      CALL_PauseTransformFeedback(ctx->Exec, ());
   }
}

static void GLAPIENTRY
save_ResumeTransformFeedback(void)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   (void) alloc_instruction(ctx, OPCODE_RESUME_TRANSFORM_FEEDBACK, 0);
   if (ctx->ExecuteFlag) {
      CALL_ResumeTransformFeedback(ctx->Exec, ());
   }
}

static void GLAPIENTRY
save_DrawTransformFeedback(GLenum mode, GLuint name)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_DRAW_TRANSFORM_FEEDBACK, 2);
   if (n) {
      n[1].e = mode;
      n[2].ui = name;
   }
   if (ctx->ExecuteFlag) {
      CALL_DrawTransformFeedback(ctx->Exec, (mode, name));
   }
}


/* aka UseProgram() */
static void GLAPIENTRY
save_UseProgramObjectARB(GLhandleARB program)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_USE_PROGRAM, 1);
   if (n) {
      n[1].ui = program;
   }
   if (ctx->ExecuteFlag) {
      CALL_UseProgramObjectARB(ctx->Exec, (program));
   }
}


static void GLAPIENTRY
save_Uniform1fARB(GLint location, GLfloat x)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_UNIFORM_1F, 2);
   if (n) {
      n[1].i = location;
      n[2].f = x;
   }
   if (ctx->ExecuteFlag) {
      CALL_Uniform1fARB(ctx->Exec, (location, x));
   }
}


static void GLAPIENTRY
save_Uniform2fARB(GLint location, GLfloat x, GLfloat y)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_UNIFORM_2F, 3);
   if (n) {
      n[1].i = location;
      n[2].f = x;
      n[3].f = y;
   }
   if (ctx->ExecuteFlag) {
      CALL_Uniform2fARB(ctx->Exec, (location, x, y));
   }
}


static void GLAPIENTRY
save_Uniform3fARB(GLint location, GLfloat x, GLfloat y, GLfloat z)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_UNIFORM_3F, 4);
   if (n) {
      n[1].i = location;
      n[2].f = x;
      n[3].f = y;
      n[4].f = z;
   }
   if (ctx->ExecuteFlag) {
      CALL_Uniform3fARB(ctx->Exec, (location, x, y, z));
   }
}


static void GLAPIENTRY
save_Uniform4fARB(GLint location, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_UNIFORM_4F, 5);
   if (n) {
      n[1].i = location;
      n[2].f = x;
      n[3].f = y;
      n[4].f = z;
      n[5].f = w;
   }
   if (ctx->ExecuteFlag) {
      CALL_Uniform4fARB(ctx->Exec, (location, x, y, z, w));
   }
}


/** Return copy of memory */
static void *
memdup(const void *src, GLsizei bytes)
{
   void *b = bytes >= 0 ? malloc(bytes) : NULL;
   if (b)
      memcpy(b, src, bytes);
   return b;
}


static void GLAPIENTRY
save_Uniform1fvARB(GLint location, GLsizei count, const GLfloat *v)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_UNIFORM_1FV, 3);
   if (n) {
      n[1].i = location;
      n[2].i = count;
      n[3].data = memdup(v, count * 1 * sizeof(GLfloat));
   }
   if (ctx->ExecuteFlag) {
      CALL_Uniform1fvARB(ctx->Exec, (location, count, v));
   }
}

static void GLAPIENTRY
save_Uniform2fvARB(GLint location, GLsizei count, const GLfloat *v)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_UNIFORM_2FV, 3);
   if (n) {
      n[1].i = location;
      n[2].i = count;
      n[3].data = memdup(v, count * 2 * sizeof(GLfloat));
   }
   if (ctx->ExecuteFlag) {
      CALL_Uniform2fvARB(ctx->Exec, (location, count, v));
   }
}

static void GLAPIENTRY
save_Uniform3fvARB(GLint location, GLsizei count, const GLfloat *v)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_UNIFORM_3FV, 3);
   if (n) {
      n[1].i = location;
      n[2].i = count;
      n[3].data = memdup(v, count * 3 * sizeof(GLfloat));
   }
   if (ctx->ExecuteFlag) {
      CALL_Uniform3fvARB(ctx->Exec, (location, count, v));
   }
}

static void GLAPIENTRY
save_Uniform4fvARB(GLint location, GLsizei count, const GLfloat *v)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_UNIFORM_4FV, 3);
   if (n) {
      n[1].i = location;
      n[2].i = count;
      n[3].data = memdup(v, count * 4 * sizeof(GLfloat));
   }
   if (ctx->ExecuteFlag) {
      CALL_Uniform4fvARB(ctx->Exec, (location, count, v));
   }
}


static void GLAPIENTRY
save_Uniform1iARB(GLint location, GLint x)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_UNIFORM_1I, 2);
   if (n) {
      n[1].i = location;
      n[2].i = x;
   }
   if (ctx->ExecuteFlag) {
      CALL_Uniform1iARB(ctx->Exec, (location, x));
   }
}

static void GLAPIENTRY
save_Uniform2iARB(GLint location, GLint x, GLint y)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_UNIFORM_2I, 3);
   if (n) {
      n[1].i = location;
      n[2].i = x;
      n[3].i = y;
   }
   if (ctx->ExecuteFlag) {
      CALL_Uniform2iARB(ctx->Exec, (location, x, y));
   }
}

static void GLAPIENTRY
save_Uniform3iARB(GLint location, GLint x, GLint y, GLint z)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_UNIFORM_3I, 4);
   if (n) {
      n[1].i = location;
      n[2].i = x;
      n[3].i = y;
      n[4].i = z;
   }
   if (ctx->ExecuteFlag) {
      CALL_Uniform3iARB(ctx->Exec, (location, x, y, z));
   }
}

static void GLAPIENTRY
save_Uniform4iARB(GLint location, GLint x, GLint y, GLint z, GLint w)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_UNIFORM_4I, 5);
   if (n) {
      n[1].i = location;
      n[2].i = x;
      n[3].i = y;
      n[4].i = z;
      n[5].i = w;
   }
   if (ctx->ExecuteFlag) {
      CALL_Uniform4iARB(ctx->Exec, (location, x, y, z, w));
   }
}



static void GLAPIENTRY
save_Uniform1ivARB(GLint location, GLsizei count, const GLint *v)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_UNIFORM_1IV, 3);
   if (n) {
      n[1].i = location;
      n[2].i = count;
      n[3].data = memdup(v, count * 1 * sizeof(GLint));
   }
   if (ctx->ExecuteFlag) {
      CALL_Uniform1ivARB(ctx->Exec, (location, count, v));
   }
}

static void GLAPIENTRY
save_Uniform2ivARB(GLint location, GLsizei count, const GLint *v)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_UNIFORM_2IV, 3);
   if (n) {
      n[1].i = location;
      n[2].i = count;
      n[3].data = memdup(v, count * 2 * sizeof(GLint));
   }
   if (ctx->ExecuteFlag) {
      CALL_Uniform2ivARB(ctx->Exec, (location, count, v));
   }
}

static void GLAPIENTRY
save_Uniform3ivARB(GLint location, GLsizei count, const GLint *v)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_UNIFORM_3IV, 3);
   if (n) {
      n[1].i = location;
      n[2].i = count;
      n[3].data = memdup(v, count * 3 * sizeof(GLint));
   }
   if (ctx->ExecuteFlag) {
      CALL_Uniform3ivARB(ctx->Exec, (location, count, v));
   }
}

static void GLAPIENTRY
save_Uniform4ivARB(GLint location, GLsizei count, const GLint *v)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_UNIFORM_4IV, 3);
   if (n) {
      n[1].i = location;
      n[2].i = count;
      n[3].data = memdup(v, count * 4 * sizeof(GLfloat));
   }
   if (ctx->ExecuteFlag) {
      CALL_Uniform4ivARB(ctx->Exec, (location, count, v));
   }
}



static void GLAPIENTRY
save_Uniform1ui(GLint location, GLuint x)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_UNIFORM_1UI, 2);
   if (n) {
      n[1].i = location;
      n[2].i = x;
   }
   if (ctx->ExecuteFlag) {
      /*CALL_Uniform1ui(ctx->Exec, (location, x));*/
   }
}

static void GLAPIENTRY
save_Uniform2ui(GLint location, GLuint x, GLuint y)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_UNIFORM_2UI, 3);
   if (n) {
      n[1].i = location;
      n[2].i = x;
      n[3].i = y;
   }
   if (ctx->ExecuteFlag) {
      /*CALL_Uniform2ui(ctx->Exec, (location, x, y));*/
   }
}

static void GLAPIENTRY
save_Uniform3ui(GLint location, GLuint x, GLuint y, GLuint z)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_UNIFORM_3UI, 4);
   if (n) {
      n[1].i = location;
      n[2].i = x;
      n[3].i = y;
      n[4].i = z;
   }
   if (ctx->ExecuteFlag) {
      /*CALL_Uniform3ui(ctx->Exec, (location, x, y, z));*/
   }
}

static void GLAPIENTRY
save_Uniform4ui(GLint location, GLuint x, GLuint y, GLuint z, GLuint w)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_UNIFORM_4UI, 5);
   if (n) {
      n[1].i = location;
      n[2].i = x;
      n[3].i = y;
      n[4].i = z;
      n[5].i = w;
   }
   if (ctx->ExecuteFlag) {
      /*CALL_Uniform4ui(ctx->Exec, (location, x, y, z, w));*/
   }
}



static void GLAPIENTRY
save_Uniform1uiv(GLint location, GLsizei count, const GLuint *v)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_UNIFORM_1UIV, 3);
   if (n) {
      n[1].i = location;
      n[2].i = count;
      n[3].data = memdup(v, count * 1 * sizeof(*v));
   }
   if (ctx->ExecuteFlag) {
      /*CALL_Uniform1uiv(ctx->Exec, (location, count, v));*/
   }
}

static void GLAPIENTRY
save_Uniform2uiv(GLint location, GLsizei count, const GLuint *v)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_UNIFORM_2UIV, 3);
   if (n) {
      n[1].i = location;
      n[2].i = count;
      n[3].data = memdup(v, count * 2 * sizeof(*v));
   }
   if (ctx->ExecuteFlag) {
      /*CALL_Uniform2uiv(ctx->Exec, (location, count, v));*/
   }
}

static void GLAPIENTRY
save_Uniform3uiv(GLint location, GLsizei count, const GLuint *v)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_UNIFORM_3UIV, 3);
   if (n) {
      n[1].i = location;
      n[2].i = count;
      n[3].data = memdup(v, count * 3 * sizeof(*v));
   }
   if (ctx->ExecuteFlag) {
      /*CALL_Uniform3uiv(ctx->Exec, (location, count, v));*/
   }
}

static void GLAPIENTRY
save_Uniform4uiv(GLint location, GLsizei count, const GLuint *v)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_UNIFORM_4UIV, 3);
   if (n) {
      n[1].i = location;
      n[2].i = count;
      n[3].data = memdup(v, count * 4 * sizeof(*v));
   }
   if (ctx->ExecuteFlag) {
      /*CALL_Uniform4uiv(ctx->Exec, (location, count, v));*/
   }
}



static void GLAPIENTRY
save_UniformMatrix2fvARB(GLint location, GLsizei count, GLboolean transpose,
                         const GLfloat *m)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_UNIFORM_MATRIX22, 4);
   if (n) {
      n[1].i = location;
      n[2].i = count;
      n[3].b = transpose;
      n[4].data = memdup(m, count * 2 * 2 * sizeof(GLfloat));
   }
   if (ctx->ExecuteFlag) {
      CALL_UniformMatrix2fvARB(ctx->Exec, (location, count, transpose, m));
   }
}

static void GLAPIENTRY
save_UniformMatrix3fvARB(GLint location, GLsizei count, GLboolean transpose,
                         const GLfloat *m)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_UNIFORM_MATRIX33, 4);
   if (n) {
      n[1].i = location;
      n[2].i = count;
      n[3].b = transpose;
      n[4].data = memdup(m, count * 3 * 3 * sizeof(GLfloat));
   }
   if (ctx->ExecuteFlag) {
      CALL_UniformMatrix3fvARB(ctx->Exec, (location, count, transpose, m));
   }
}

static void GLAPIENTRY
save_UniformMatrix4fvARB(GLint location, GLsizei count, GLboolean transpose,
                         const GLfloat *m)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_UNIFORM_MATRIX44, 4);
   if (n) {
      n[1].i = location;
      n[2].i = count;
      n[3].b = transpose;
      n[4].data = memdup(m, count * 4 * 4 * sizeof(GLfloat));
   }
   if (ctx->ExecuteFlag) {
      CALL_UniformMatrix4fvARB(ctx->Exec, (location, count, transpose, m));
   }
}


static void GLAPIENTRY
save_UniformMatrix2x3fv(GLint location, GLsizei count, GLboolean transpose,
                        const GLfloat *m)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_UNIFORM_MATRIX23, 4);
   if (n) {
      n[1].i = location;
      n[2].i = count;
      n[3].b = transpose;
      n[4].data = memdup(m, count * 2 * 3 * sizeof(GLfloat));
   }
   if (ctx->ExecuteFlag) {
      CALL_UniformMatrix2x3fv(ctx->Exec, (location, count, transpose, m));
   }
}

static void GLAPIENTRY
save_UniformMatrix3x2fv(GLint location, GLsizei count, GLboolean transpose,
                        const GLfloat *m)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_UNIFORM_MATRIX32, 4);
   if (n) {
      n[1].i = location;
      n[2].i = count;
      n[3].b = transpose;
      n[4].data = memdup(m, count * 3 * 2 * sizeof(GLfloat));
   }
   if (ctx->ExecuteFlag) {
      CALL_UniformMatrix3x2fv(ctx->Exec, (location, count, transpose, m));
   }
}


static void GLAPIENTRY
save_UniformMatrix2x4fv(GLint location, GLsizei count, GLboolean transpose,
                        const GLfloat *m)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_UNIFORM_MATRIX24, 4);
   if (n) {
      n[1].i = location;
      n[2].i = count;
      n[3].b = transpose;
      n[4].data = memdup(m, count * 2 * 4 * sizeof(GLfloat));
   }
   if (ctx->ExecuteFlag) {
      CALL_UniformMatrix2x4fv(ctx->Exec, (location, count, transpose, m));
   }
}

static void GLAPIENTRY
save_UniformMatrix4x2fv(GLint location, GLsizei count, GLboolean transpose,
                        const GLfloat *m)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_UNIFORM_MATRIX42, 4);
   if (n) {
      n[1].i = location;
      n[2].i = count;
      n[3].b = transpose;
      n[4].data = memdup(m, count * 4 * 2 * sizeof(GLfloat));
   }
   if (ctx->ExecuteFlag) {
      CALL_UniformMatrix4x2fv(ctx->Exec, (location, count, transpose, m));
   }
}


static void GLAPIENTRY
save_UniformMatrix3x4fv(GLint location, GLsizei count, GLboolean transpose,
                        const GLfloat *m)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_UNIFORM_MATRIX34, 4);
   if (n) {
      n[1].i = location;
      n[2].i = count;
      n[3].b = transpose;
      n[4].data = memdup(m, count * 3 * 4 * sizeof(GLfloat));
   }
   if (ctx->ExecuteFlag) {
      CALL_UniformMatrix3x4fv(ctx->Exec, (location, count, transpose, m));
   }
}

static void GLAPIENTRY
save_UniformMatrix4x3fv(GLint location, GLsizei count, GLboolean transpose,
                        const GLfloat *m)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_UNIFORM_MATRIX43, 4);
   if (n) {
      n[1].i = location;
      n[2].i = count;
      n[3].b = transpose;
      n[4].data = memdup(m, count * 4 * 3 * sizeof(GLfloat));
   }
   if (ctx->ExecuteFlag) {
      CALL_UniformMatrix4x3fv(ctx->Exec, (location, count, transpose, m));
   }
}

static void GLAPIENTRY
save_ClampColorARB(GLenum target, GLenum clamp)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_CLAMP_COLOR, 2);
   if (n) {
      n[1].e = target;
      n[2].e = clamp;
   }
   if (ctx->ExecuteFlag) {
      CALL_ClampColorARB(ctx->Exec, (target, clamp));
   }
}

static void GLAPIENTRY
save_UseShaderProgramEXT(GLenum type, GLuint program)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_USE_SHADER_PROGRAM_EXT, 2);
   if (n) {
      n[1].ui = type;
      n[2].ui = program;
   }
   if (ctx->ExecuteFlag) {
      CALL_UseShaderProgramEXT(ctx->Exec, (type, program));
   }
}

static void GLAPIENTRY
save_ActiveProgramEXT(GLuint program)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_ACTIVE_PROGRAM_EXT, 1);
   if (n) {
      n[1].ui = program;
   }
   if (ctx->ExecuteFlag) {
      CALL_ActiveProgramEXT(ctx->Exec, (program));
   }
}

/** GL_EXT_texture_integer */
static void GLAPIENTRY
save_ClearColorIi(GLint red, GLint green, GLint blue, GLint alpha)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_CLEARCOLOR_I, 4);
   if (n) {
      n[1].i = red;
      n[2].i = green;
      n[3].i = blue;
      n[4].i = alpha;
   }
   if (ctx->ExecuteFlag) {
      CALL_ClearColorIiEXT(ctx->Exec, (red, green, blue, alpha));
   }
}

/** GL_EXT_texture_integer */
static void GLAPIENTRY
save_ClearColorIui(GLuint red, GLuint green, GLuint blue, GLuint alpha)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_CLEARCOLOR_UI, 4);
   if (n) {
      n[1].ui = red;
      n[2].ui = green;
      n[3].ui = blue;
      n[4].ui = alpha;
   }
   if (ctx->ExecuteFlag) {
      CALL_ClearColorIuiEXT(ctx->Exec, (red, green, blue, alpha));
   }
}

/** GL_EXT_texture_integer */
static void GLAPIENTRY
save_TexParameterIiv(GLenum target, GLenum pname, const GLint *params)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_TEXPARAMETER_I, 6);
   if (n) {
      n[1].e = target;
      n[2].e = pname;
      n[3].i = params[0];
      n[4].i = params[1];
      n[5].i = params[2];
      n[6].i = params[3];
   }
   if (ctx->ExecuteFlag) {
      CALL_TexParameterIivEXT(ctx->Exec, (target, pname, params));
   }
}

/** GL_EXT_texture_integer */
static void GLAPIENTRY
save_TexParameterIuiv(GLenum target, GLenum pname, const GLuint *params)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_TEXPARAMETER_UI, 6);
   if (n) {
      n[1].e = target;
      n[2].e = pname;
      n[3].ui = params[0];
      n[4].ui = params[1];
      n[5].ui = params[2];
      n[6].ui = params[3];
   }
   if (ctx->ExecuteFlag) {
      CALL_TexParameterIuivEXT(ctx->Exec, (target, pname, params));
   }
}

/** GL_EXT_texture_integer */
static void GLAPIENTRY
exec_GetTexParameterIiv(GLenum target, GLenum pname, GLint *params)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   CALL_GetTexParameterIivEXT(ctx->Exec, (target, pname, params));
}

/** GL_EXT_texture_integer */
static void GLAPIENTRY
exec_GetTexParameterIuiv(GLenum target, GLenum pname, GLuint *params)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   CALL_GetTexParameterIuivEXT(ctx->Exec, (target, pname, params));
}


/* GL_ARB_instanced_arrays */
static void GLAPIENTRY
save_VertexAttribDivisor(GLuint index, GLuint divisor)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_VERTEX_ATTRIB_DIVISOR, 2);
   if (n) {
      n[1].ui = index;
      n[2].ui = divisor;
   }
   if (ctx->ExecuteFlag) {
      CALL_VertexAttribDivisorARB(ctx->Exec, (index, divisor));
   }
}


/* GL_NV_texture_barrier */
static void GLAPIENTRY
save_TextureBarrierNV(void)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   alloc_instruction(ctx, OPCODE_TEXTURE_BARRIER_NV, 0);
   if (ctx->ExecuteFlag) {
      CALL_TextureBarrierNV(ctx->Exec, ());
   }
}


/* GL_ARB_sampler_objects */
static void GLAPIENTRY
save_BindSampler(GLuint unit, GLuint sampler)
{
   Node *n;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_BIND_SAMPLER, 2);
   if (n) {
      n[1].ui = unit;
      n[2].ui = sampler;
   }
   if (ctx->ExecuteFlag) {
      CALL_BindSampler(ctx->Exec, (unit, sampler));
   }
}

static void GLAPIENTRY
save_SamplerParameteriv(GLuint sampler, GLenum pname, const GLint *params)
{
   Node *n;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_SAMPLER_PARAMETERIV, 6);
   if (n) {
      n[1].ui = sampler;
      n[2].e = pname;
      n[3].i = params[0];
      if (pname == GL_TEXTURE_BORDER_COLOR) {
         n[4].i = params[1];
         n[5].i = params[2];
         n[6].i = params[3];
      }
      else {
         n[4].i = n[5].i = n[6].i = 0;
      }
   }
   if (ctx->ExecuteFlag) {
      CALL_SamplerParameteriv(ctx->Exec, (sampler, pname, params));
   }
}

static void GLAPIENTRY
save_SamplerParameteri(GLuint sampler, GLenum pname, GLint param)
{
   save_SamplerParameteriv(sampler, pname, &param);
}

static void GLAPIENTRY
save_SamplerParameterfv(GLuint sampler, GLenum pname, const GLfloat *params)
{
   Node *n;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_SAMPLER_PARAMETERFV, 6);
   if (n) {
      n[1].ui = sampler;
      n[2].e = pname;
      n[3].f = params[0];
      if (pname == GL_TEXTURE_BORDER_COLOR) {
         n[4].f = params[1];
         n[5].f = params[2];
         n[6].f = params[3];
      }
      else {
         n[4].f = n[5].f = n[6].f = 0.0F;
      }
   }
   if (ctx->ExecuteFlag) {
      CALL_SamplerParameterfv(ctx->Exec, (sampler, pname, params));
   }
}

static void GLAPIENTRY
save_SamplerParameterf(GLuint sampler, GLenum pname, GLfloat param)
{
   save_SamplerParameterfv(sampler, pname, &param);
}

static void GLAPIENTRY
save_SamplerParameterIiv(GLuint sampler, GLenum pname, const GLint *params)
{
   Node *n;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_SAMPLER_PARAMETERIIV, 6);
   if (n) {
      n[1].ui = sampler;
      n[2].e = pname;
      n[3].i = params[0];
      if (pname == GL_TEXTURE_BORDER_COLOR) {
         n[4].i = params[1];
         n[5].i = params[2];
         n[6].i = params[3];
      }
      else {
         n[4].i = n[5].i = n[6].i = 0;
      }
   }
   if (ctx->ExecuteFlag) {
      CALL_SamplerParameterIiv(ctx->Exec, (sampler, pname, params));
   }
}

static void GLAPIENTRY
save_SamplerParameterIuiv(GLuint sampler, GLenum pname, const GLuint *params)
{
   Node *n;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_SAMPLER_PARAMETERUIV, 6);
   if (n) {
      n[1].ui = sampler;
      n[2].e = pname;
      n[3].ui = params[0];
      if (pname == GL_TEXTURE_BORDER_COLOR) {
         n[4].ui = params[1];
         n[5].ui = params[2];
         n[6].ui = params[3];
      }
      else {
         n[4].ui = n[5].ui = n[6].ui = 0;
      }
   }
   if (ctx->ExecuteFlag) {
      CALL_SamplerParameterIuiv(ctx->Exec, (sampler, pname, params));
   }
}

/* GL_ARB_geometry_shader4 */
static void GLAPIENTRY
save_ProgramParameteri(GLuint program, GLenum pname, GLint value)
{
   Node *n;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_PROGRAM_PARAMETERI, 3);
   if (n) {
      n[1].ui = program;
      n[2].e = pname;
      n[3].i = value;
   }
   if (ctx->ExecuteFlag) {
      CALL_ProgramParameteriARB(ctx->Exec, (program, pname, value));
   }
}

static void GLAPIENTRY
save_FramebufferTexture(GLenum target, GLenum attachment,
                        GLuint texture, GLint level)
{
   Node *n;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_FRAMEBUFFER_TEXTURE, 4);
   if (n) {
      n[1].e = target;
      n[2].e = attachment;
      n[3].ui = texture;
      n[4].i = level;
   }
   if (ctx->ExecuteFlag) {
      CALL_FramebufferTextureARB(ctx->Exec, (target, attachment, texture, level));
   }
}

static void GLAPIENTRY
save_FramebufferTextureFace(GLenum target, GLenum attachment,
                            GLuint texture, GLint level, GLenum face)
{
   Node *n;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_FRAMEBUFFER_TEXTURE_FACE, 5);
   if (n) {
      n[1].e = target;
      n[2].e = attachment;
      n[3].ui = texture;
      n[4].i = level;
      n[5].e = face;
   }
   if (ctx->ExecuteFlag) {
      CALL_FramebufferTextureFaceARB(ctx->Exec, (target, attachment, texture,
                                                 level, face));
   }
}



static void GLAPIENTRY
save_WaitSync(GLsync sync, GLbitfield flags, GLuint64 timeout)
{
   Node *n;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_WAIT_SYNC, 4);
   if (n) {
      union uint64_pair p;
      p.uint64 = timeout;
      n[1].data = sync;
      n[2].e = flags;
      n[3].ui = p.uint32[0];
      n[4].ui = p.uint32[1];
   }
   if (ctx->ExecuteFlag) {
      CALL_WaitSync(ctx->Exec, (sync, flags, timeout));
   }
}


/** GL_NV_conditional_render */
static void GLAPIENTRY
save_BeginConditionalRender(GLuint queryId, GLenum mode)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_BEGIN_CONDITIONAL_RENDER, 2);
   if (n) {
      n[1].i = queryId;
      n[2].e = mode;
   }
   if (ctx->ExecuteFlag) {
      CALL_BeginConditionalRenderNV(ctx->Exec, (queryId, mode));
   }
}

static void GLAPIENTRY
save_EndConditionalRender(void)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   alloc_instruction(ctx, OPCODE_END_CONDITIONAL_RENDER, 0);
   if (ctx->ExecuteFlag) {
      CALL_EndConditionalRenderNV(ctx->Exec, ());
   }
}


/**
 * Save an error-generating command into display list.
 *
 * KW: Will appear in the list before the vertex buffer containing the
 * command that provoked the error.  I don't see this as a problem.
 */
static void
save_error(struct gl_context *ctx, GLenum error, const char *s)
{
   Node *n;
   n = alloc_instruction(ctx, OPCODE_ERROR, 2);
   if (n) {
      n[1].e = error;
      n[2].data = (void *) s;
   }
}


/**
 * Compile an error into current display list.
 */
void
_mesa_compile_error(struct gl_context *ctx, GLenum error, const char *s)
{
   if (ctx->CompileFlag)
      save_error(ctx, error, s);
   if (ctx->ExecuteFlag)
      _mesa_error(ctx, error, "%s", s);
}


/**
 * Test if ID names a display list.
 */
static GLboolean
islist(struct gl_context *ctx, GLuint list)
{
   if (list > 0 && lookup_list(ctx, list)) {
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
static void
execute_list(struct gl_context *ctx, GLuint list)
{
   struct gl_display_list *dlist;
   Node *n;
   GLboolean done;

   if (list == 0 || !islist(ctx, list))
      return;

   if (ctx->ListState.CallDepth == MAX_LIST_NESTING) {
      /* raise an error? */
      return;
   }

   dlist = lookup_list(ctx, list);
   if (!dlist)
      return;

   ctx->ListState.CallDepth++;

   if (ctx->Driver.BeginCallList)
      ctx->Driver.BeginCallList(ctx, dlist);

   n = dlist->Head;

   done = GL_FALSE;
   while (!done) {
      const OpCode opcode = n[0].opcode;

      if (is_ext_opcode(opcode)) {
         n += ext_opcode_execute(ctx, n);
      }
      else {
         switch (opcode) {
         case OPCODE_ERROR:
            _mesa_error(ctx, n[1].e, "%s", (const char *) n[2].data);
            break;
         case OPCODE_ACCUM:
            CALL_Accum(ctx->Exec, (n[1].e, n[2].f));
            break;
         case OPCODE_ALPHA_FUNC:
            CALL_AlphaFunc(ctx->Exec, (n[1].e, n[2].f));
            break;
         case OPCODE_BIND_TEXTURE:
            CALL_BindTexture(ctx->Exec, (n[1].e, n[2].ui));
            break;
         case OPCODE_BITMAP:
            {
               const struct gl_pixelstore_attrib save = ctx->Unpack;
               ctx->Unpack = ctx->DefaultPacking;
               CALL_Bitmap(ctx->Exec, ((GLsizei) n[1].i, (GLsizei) n[2].i,
                                       n[3].f, n[4].f, n[5].f, n[6].f,
                                       (const GLubyte *) n[7].data));
               ctx->Unpack = save;      /* restore */
            }
            break;
         case OPCODE_BLEND_COLOR:
            CALL_BlendColor(ctx->Exec, (n[1].f, n[2].f, n[3].f, n[4].f));
            break;
         case OPCODE_BLEND_EQUATION:
            CALL_BlendEquation(ctx->Exec, (n[1].e));
            break;
         case OPCODE_BLEND_EQUATION_SEPARATE:
            CALL_BlendEquationSeparateEXT(ctx->Exec, (n[1].e, n[2].e));
            break;
         case OPCODE_BLEND_FUNC_SEPARATE:
            CALL_BlendFuncSeparateEXT(ctx->Exec,
                                      (n[1].e, n[2].e, n[3].e, n[4].e));
            break;

         case OPCODE_BLEND_FUNC_I:
            /* GL_ARB_draw_buffers_blend */
            CALL_BlendFunciARB(ctx->Exec, (n[1].ui, n[2].e, n[3].e));
            break;
         case OPCODE_BLEND_FUNC_SEPARATE_I:
            /* GL_ARB_draw_buffers_blend */
            CALL_BlendFuncSeparateiARB(ctx->Exec, (n[1].ui, n[2].e, n[3].e,
                                                   n[4].e, n[5].e));
            break;
         case OPCODE_BLEND_EQUATION_I:
            /* GL_ARB_draw_buffers_blend */
            CALL_BlendEquationiARB(ctx->Exec, (n[1].ui, n[2].e));
            break;
         case OPCODE_BLEND_EQUATION_SEPARATE_I:
            /* GL_ARB_draw_buffers_blend */
            CALL_BlendEquationSeparateiARB(ctx->Exec,
                                           (n[1].ui, n[2].e, n[3].e));
            break;

         case OPCODE_CALL_LIST:
            /* Generated by glCallList(), don't add ListBase */
            if (ctx->ListState.CallDepth < MAX_LIST_NESTING) {
               execute_list(ctx, n[1].ui);
            }
            break;
         case OPCODE_CALL_LIST_OFFSET:
            /* Generated by glCallLists() so we must add ListBase */
            if (n[2].b) {
               /* user specified a bad data type at compile time */
               _mesa_error(ctx, GL_INVALID_ENUM, "glCallLists(type)");
            }
            else if (ctx->ListState.CallDepth < MAX_LIST_NESTING) {
               GLuint list = (GLuint) (ctx->List.ListBase + n[1].i);
               execute_list(ctx, list);
            }
            break;
         case OPCODE_CLEAR:
            CALL_Clear(ctx->Exec, (n[1].bf));
            break;
         case OPCODE_CLEAR_BUFFER_IV:
            {
               GLint value[4];
               value[0] = n[3].i;
               value[1] = n[4].i;
               value[2] = n[5].i;
               value[3] = n[6].i;
               CALL_ClearBufferiv(ctx->Exec, (n[1].e, n[2].i, value));
            }
            break;
         case OPCODE_CLEAR_BUFFER_UIV:
            {
               GLuint value[4];
               value[0] = n[3].ui;
               value[1] = n[4].ui;
               value[2] = n[5].ui;
               value[3] = n[6].ui;
               CALL_ClearBufferuiv(ctx->Exec, (n[1].e, n[2].i, value));
            }
            break;
         case OPCODE_CLEAR_BUFFER_FV:
            {
               GLfloat value[4];
               value[0] = n[3].f;
               value[1] = n[4].f;
               value[2] = n[5].f;
               value[3] = n[6].f;
               CALL_ClearBufferfv(ctx->Exec, (n[1].e, n[2].i, value));
            }
            break;
         case OPCODE_CLEAR_BUFFER_FI:
            CALL_ClearBufferfi(ctx->Exec, (n[1].e, n[2].i, n[3].f, n[4].i));
            break;
         case OPCODE_CLEAR_COLOR:
            CALL_ClearColor(ctx->Exec, (n[1].f, n[2].f, n[3].f, n[4].f));
            break;
         case OPCODE_CLEAR_ACCUM:
            CALL_ClearAccum(ctx->Exec, (n[1].f, n[2].f, n[3].f, n[4].f));
            break;
         case OPCODE_CLEAR_DEPTH:
            CALL_ClearDepth(ctx->Exec, ((GLclampd) n[1].f));
            break;
         case OPCODE_CLEAR_INDEX:
            CALL_ClearIndex(ctx->Exec, ((GLfloat) n[1].ui));
            break;
         case OPCODE_CLEAR_STENCIL:
            CALL_ClearStencil(ctx->Exec, (n[1].i));
            break;
         case OPCODE_CLIP_PLANE:
            {
               GLdouble eq[4];
               eq[0] = n[2].f;
               eq[1] = n[3].f;
               eq[2] = n[4].f;
               eq[3] = n[5].f;
               CALL_ClipPlane(ctx->Exec, (n[1].e, eq));
            }
            break;
         case OPCODE_COLOR_MASK:
            CALL_ColorMask(ctx->Exec, (n[1].b, n[2].b, n[3].b, n[4].b));
            break;
         case OPCODE_COLOR_MASK_INDEXED:
            CALL_ColorMaskIndexedEXT(ctx->Exec, (n[1].ui, n[2].b, n[3].b,
                                                 n[4].b, n[5].b));
            break;
         case OPCODE_COLOR_MATERIAL:
            CALL_ColorMaterial(ctx->Exec, (n[1].e, n[2].e));
            break;
         case OPCODE_COLOR_TABLE:
            {
               const struct gl_pixelstore_attrib save = ctx->Unpack;
               ctx->Unpack = ctx->DefaultPacking;
               CALL_ColorTable(ctx->Exec, (n[1].e, n[2].e, n[3].i, n[4].e,
                                           n[5].e, n[6].data));
               ctx->Unpack = save;      /* restore */
            }
            break;
         case OPCODE_COLOR_TABLE_PARAMETER_FV:
            {
               GLfloat params[4];
               params[0] = n[3].f;
               params[1] = n[4].f;
               params[2] = n[5].f;
               params[3] = n[6].f;
               CALL_ColorTableParameterfv(ctx->Exec,
                                          (n[1].e, n[2].e, params));
            }
            break;
         case OPCODE_COLOR_TABLE_PARAMETER_IV:
            {
               GLint params[4];
               params[0] = n[3].i;
               params[1] = n[4].i;
               params[2] = n[5].i;
               params[3] = n[6].i;
               CALL_ColorTableParameteriv(ctx->Exec,
                                          (n[1].e, n[2].e, params));
            }
            break;
         case OPCODE_COLOR_SUB_TABLE:
            {
               const struct gl_pixelstore_attrib save = ctx->Unpack;
               ctx->Unpack = ctx->DefaultPacking;
               CALL_ColorSubTable(ctx->Exec, (n[1].e, n[2].i, n[3].i,
                                              n[4].e, n[5].e, n[6].data));
               ctx->Unpack = save;      /* restore */
            }
            break;
         case OPCODE_CONVOLUTION_FILTER_1D:
            {
               const struct gl_pixelstore_attrib save = ctx->Unpack;
               ctx->Unpack = ctx->DefaultPacking;
               CALL_ConvolutionFilter1D(ctx->Exec, (n[1].e, n[2].i, n[3].i,
                                                    n[4].e, n[5].e,
                                                    n[6].data));
               ctx->Unpack = save;      /* restore */
            }
            break;
         case OPCODE_CONVOLUTION_FILTER_2D:
            {
               const struct gl_pixelstore_attrib save = ctx->Unpack;
               ctx->Unpack = ctx->DefaultPacking;
               CALL_ConvolutionFilter2D(ctx->Exec, (n[1].e, n[2].i, n[3].i,
                                                    n[4].i, n[5].e, n[6].e,
                                                    n[7].data));
               ctx->Unpack = save;      /* restore */
            }
            break;
         case OPCODE_CONVOLUTION_PARAMETER_I:
            CALL_ConvolutionParameteri(ctx->Exec, (n[1].e, n[2].e, n[3].i));
            break;
         case OPCODE_CONVOLUTION_PARAMETER_IV:
            {
               GLint params[4];
               params[0] = n[3].i;
               params[1] = n[4].i;
               params[2] = n[5].i;
               params[3] = n[6].i;
               CALL_ConvolutionParameteriv(ctx->Exec,
                                           (n[1].e, n[2].e, params));
            }
            break;
         case OPCODE_CONVOLUTION_PARAMETER_F:
            CALL_ConvolutionParameterf(ctx->Exec, (n[1].e, n[2].e, n[3].f));
            break;
         case OPCODE_CONVOLUTION_PARAMETER_FV:
            {
               GLfloat params[4];
               params[0] = n[3].f;
               params[1] = n[4].f;
               params[2] = n[5].f;
               params[3] = n[6].f;
               CALL_ConvolutionParameterfv(ctx->Exec,
                                           (n[1].e, n[2].e, params));
            }
            break;
         case OPCODE_COPY_COLOR_SUB_TABLE:
            CALL_CopyColorSubTable(ctx->Exec, (n[1].e, n[2].i,
                                               n[3].i, n[4].i, n[5].i));
            break;
         case OPCODE_COPY_COLOR_TABLE:
            CALL_CopyColorSubTable(ctx->Exec, (n[1].e, n[2].i,
                                               n[3].i, n[4].i, n[5].i));
            break;
         case OPCODE_COPY_PIXELS:
            CALL_CopyPixels(ctx->Exec, (n[1].i, n[2].i,
                                        (GLsizei) n[3].i, (GLsizei) n[4].i,
                                        n[5].e));
            break;
         case OPCODE_COPY_TEX_IMAGE1D:
            CALL_CopyTexImage1D(ctx->Exec, (n[1].e, n[2].i, n[3].e, n[4].i,
                                            n[5].i, n[6].i, n[7].i));
            break;
         case OPCODE_COPY_TEX_IMAGE2D:
            CALL_CopyTexImage2D(ctx->Exec, (n[1].e, n[2].i, n[3].e, n[4].i,
                                            n[5].i, n[6].i, n[7].i, n[8].i));
            break;
         case OPCODE_COPY_TEX_SUB_IMAGE1D:
            CALL_CopyTexSubImage1D(ctx->Exec, (n[1].e, n[2].i, n[3].i,
                                               n[4].i, n[5].i, n[6].i));
            break;
         case OPCODE_COPY_TEX_SUB_IMAGE2D:
            CALL_CopyTexSubImage2D(ctx->Exec, (n[1].e, n[2].i, n[3].i,
                                               n[4].i, n[5].i, n[6].i, n[7].i,
                                               n[8].i));
            break;
         case OPCODE_COPY_TEX_SUB_IMAGE3D:
            CALL_CopyTexSubImage3D(ctx->Exec, (n[1].e, n[2].i, n[3].i,
                                               n[4].i, n[5].i, n[6].i, n[7].i,
                                               n[8].i, n[9].i));
            break;
         case OPCODE_CULL_FACE:
            CALL_CullFace(ctx->Exec, (n[1].e));
            break;
         case OPCODE_DEPTH_FUNC:
            CALL_DepthFunc(ctx->Exec, (n[1].e));
            break;
         case OPCODE_DEPTH_MASK:
            CALL_DepthMask(ctx->Exec, (n[1].b));
            break;
         case OPCODE_DEPTH_RANGE:
            CALL_DepthRange(ctx->Exec,
                            ((GLclampd) n[1].f, (GLclampd) n[2].f));
            break;
         case OPCODE_DISABLE:
            CALL_Disable(ctx->Exec, (n[1].e));
            break;
         case OPCODE_DISABLE_INDEXED:
            CALL_DisableIndexedEXT(ctx->Exec, (n[1].ui, n[2].e));
            break;
         case OPCODE_DRAW_BUFFER:
            CALL_DrawBuffer(ctx->Exec, (n[1].e));
            break;
         case OPCODE_DRAW_PIXELS:
            {
               const struct gl_pixelstore_attrib save = ctx->Unpack;
               ctx->Unpack = ctx->DefaultPacking;
               CALL_DrawPixels(ctx->Exec, (n[1].i, n[2].i, n[3].e, n[4].e,
                                           n[5].data));
               ctx->Unpack = save;      /* restore */
            }
            break;
         case OPCODE_ENABLE:
            CALL_Enable(ctx->Exec, (n[1].e));
            break;
         case OPCODE_ENABLE_INDEXED:
            CALL_EnableIndexedEXT(ctx->Exec, (n[1].ui, n[2].e));
            break;
         case OPCODE_EVALMESH1:
            CALL_EvalMesh1(ctx->Exec, (n[1].e, n[2].i, n[3].i));
            break;
         case OPCODE_EVALMESH2:
            CALL_EvalMesh2(ctx->Exec,
                           (n[1].e, n[2].i, n[3].i, n[4].i, n[5].i));
            break;
         case OPCODE_FOG:
            {
               GLfloat p[4];
               p[0] = n[2].f;
               p[1] = n[3].f;
               p[2] = n[4].f;
               p[3] = n[5].f;
               CALL_Fogfv(ctx->Exec, (n[1].e, p));
            }
            break;
         case OPCODE_FRONT_FACE:
            CALL_FrontFace(ctx->Exec, (n[1].e));
            break;
         case OPCODE_FRUSTUM:
            CALL_Frustum(ctx->Exec,
                         (n[1].f, n[2].f, n[3].f, n[4].f, n[5].f, n[6].f));
            break;
         case OPCODE_HINT:
            CALL_Hint(ctx->Exec, (n[1].e, n[2].e));
            break;
         case OPCODE_HISTOGRAM:
            CALL_Histogram(ctx->Exec, (n[1].e, n[2].i, n[3].e, n[4].b));
            break;
         case OPCODE_INDEX_MASK:
            CALL_IndexMask(ctx->Exec, (n[1].ui));
            break;
         case OPCODE_INIT_NAMES:
            CALL_InitNames(ctx->Exec, ());
            break;
         case OPCODE_LIGHT:
            {
               GLfloat p[4];
               p[0] = n[3].f;
               p[1] = n[4].f;
               p[2] = n[5].f;
               p[3] = n[6].f;
               CALL_Lightfv(ctx->Exec, (n[1].e, n[2].e, p));
            }
            break;
         case OPCODE_LIGHT_MODEL:
            {
               GLfloat p[4];
               p[0] = n[2].f;
               p[1] = n[3].f;
               p[2] = n[4].f;
               p[3] = n[5].f;
               CALL_LightModelfv(ctx->Exec, (n[1].e, p));
            }
            break;
         case OPCODE_LINE_STIPPLE:
            CALL_LineStipple(ctx->Exec, (n[1].i, n[2].us));
            break;
         case OPCODE_LINE_WIDTH:
            CALL_LineWidth(ctx->Exec, (n[1].f));
            break;
         case OPCODE_LIST_BASE:
            CALL_ListBase(ctx->Exec, (n[1].ui));
            break;
         case OPCODE_LOAD_IDENTITY:
            CALL_LoadIdentity(ctx->Exec, ());
            break;
         case OPCODE_LOAD_MATRIX:
            if (sizeof(Node) == sizeof(GLfloat)) {
               CALL_LoadMatrixf(ctx->Exec, (&n[1].f));
            }
            else {
               GLfloat m[16];
               GLuint i;
               for (i = 0; i < 16; i++) {
                  m[i] = n[1 + i].f;
               }
               CALL_LoadMatrixf(ctx->Exec, (m));
            }
            break;
         case OPCODE_LOAD_NAME:
            CALL_LoadName(ctx->Exec, (n[1].ui));
            break;
         case OPCODE_LOGIC_OP:
            CALL_LogicOp(ctx->Exec, (n[1].e));
            break;
         case OPCODE_MAP1:
            {
               GLenum target = n[1].e;
               GLint ustride = _mesa_evaluator_components(target);
               GLint uorder = n[5].i;
               GLfloat u1 = n[2].f;
               GLfloat u2 = n[3].f;
               CALL_Map1f(ctx->Exec, (target, u1, u2, ustride, uorder,
                                      (GLfloat *) n[6].data));
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
               CALL_Map2f(ctx->Exec, (target, u1, u2, ustride, uorder,
                                      v1, v2, vstride, vorder,
                                      (GLfloat *) n[10].data));
            }
            break;
         case OPCODE_MAPGRID1:
            CALL_MapGrid1f(ctx->Exec, (n[1].i, n[2].f, n[3].f));
            break;
         case OPCODE_MAPGRID2:
            CALL_MapGrid2f(ctx->Exec,
                           (n[1].i, n[2].f, n[3].f, n[4].i, n[5].f, n[6].f));
            break;
         case OPCODE_MATRIX_MODE:
            CALL_MatrixMode(ctx->Exec, (n[1].e));
            break;
         case OPCODE_MIN_MAX:
            CALL_Minmax(ctx->Exec, (n[1].e, n[2].e, n[3].b));
            break;
         case OPCODE_MULT_MATRIX:
            if (sizeof(Node) == sizeof(GLfloat)) {
               CALL_MultMatrixf(ctx->Exec, (&n[1].f));
            }
            else {
               GLfloat m[16];
               GLuint i;
               for (i = 0; i < 16; i++) {
                  m[i] = n[1 + i].f;
               }
               CALL_MultMatrixf(ctx->Exec, (m));
            }
            break;
         case OPCODE_ORTHO:
            CALL_Ortho(ctx->Exec,
                       (n[1].f, n[2].f, n[3].f, n[4].f, n[5].f, n[6].f));
            break;
         case OPCODE_PASSTHROUGH:
            CALL_PassThrough(ctx->Exec, (n[1].f));
            break;
         case OPCODE_PIXEL_MAP:
            CALL_PixelMapfv(ctx->Exec,
                            (n[1].e, n[2].i, (GLfloat *) n[3].data));
            break;
         case OPCODE_PIXEL_TRANSFER:
            CALL_PixelTransferf(ctx->Exec, (n[1].e, n[2].f));
            break;
         case OPCODE_PIXEL_ZOOM:
            CALL_PixelZoom(ctx->Exec, (n[1].f, n[2].f));
            break;
         case OPCODE_POINT_SIZE:
            CALL_PointSize(ctx->Exec, (n[1].f));
            break;
         case OPCODE_POINT_PARAMETERS:
            {
               GLfloat params[3];
               params[0] = n[2].f;
               params[1] = n[3].f;
               params[2] = n[4].f;
               CALL_PointParameterfvEXT(ctx->Exec, (n[1].e, params));
            }
            break;
         case OPCODE_POLYGON_MODE:
            CALL_PolygonMode(ctx->Exec, (n[1].e, n[2].e));
            break;
         case OPCODE_POLYGON_STIPPLE:
            {
               const struct gl_pixelstore_attrib save = ctx->Unpack;
               ctx->Unpack = ctx->DefaultPacking;
               CALL_PolygonStipple(ctx->Exec, ((GLubyte *) n[1].data));
               ctx->Unpack = save;      /* restore */
            }
            break;
         case OPCODE_POLYGON_OFFSET:
            CALL_PolygonOffset(ctx->Exec, (n[1].f, n[2].f));
            break;
         case OPCODE_POP_ATTRIB:
            CALL_PopAttrib(ctx->Exec, ());
            break;
         case OPCODE_POP_MATRIX:
            CALL_PopMatrix(ctx->Exec, ());
            break;
         case OPCODE_POP_NAME:
            CALL_PopName(ctx->Exec, ());
            break;
         case OPCODE_PRIORITIZE_TEXTURE:
            CALL_PrioritizeTextures(ctx->Exec, (1, &n[1].ui, &n[2].f));
            break;
         case OPCODE_PUSH_ATTRIB:
            CALL_PushAttrib(ctx->Exec, (n[1].bf));
            break;
         case OPCODE_PUSH_MATRIX:
            CALL_PushMatrix(ctx->Exec, ());
            break;
         case OPCODE_PUSH_NAME:
            CALL_PushName(ctx->Exec, (n[1].ui));
            break;
         case OPCODE_RASTER_POS:
            CALL_RasterPos4f(ctx->Exec, (n[1].f, n[2].f, n[3].f, n[4].f));
            break;
         case OPCODE_READ_BUFFER:
            CALL_ReadBuffer(ctx->Exec, (n[1].e));
            break;
         case OPCODE_RESET_HISTOGRAM:
            CALL_ResetHistogram(ctx->Exec, (n[1].e));
            break;
         case OPCODE_RESET_MIN_MAX:
            CALL_ResetMinmax(ctx->Exec, (n[1].e));
            break;
         case OPCODE_ROTATE:
            CALL_Rotatef(ctx->Exec, (n[1].f, n[2].f, n[3].f, n[4].f));
            break;
         case OPCODE_SCALE:
            CALL_Scalef(ctx->Exec, (n[1].f, n[2].f, n[3].f));
            break;
         case OPCODE_SCISSOR:
            CALL_Scissor(ctx->Exec, (n[1].i, n[2].i, n[3].i, n[4].i));
            break;
         case OPCODE_SHADE_MODEL:
            CALL_ShadeModel(ctx->Exec, (n[1].e));
            break;
         case OPCODE_PROVOKING_VERTEX:
            CALL_ProvokingVertexEXT(ctx->Exec, (n[1].e));
            break;
         case OPCODE_STENCIL_FUNC:
            CALL_StencilFunc(ctx->Exec, (n[1].e, n[2].i, n[3].ui));
            break;
         case OPCODE_STENCIL_MASK:
            CALL_StencilMask(ctx->Exec, (n[1].ui));
            break;
         case OPCODE_STENCIL_OP:
            CALL_StencilOp(ctx->Exec, (n[1].e, n[2].e, n[3].e));
            break;
         case OPCODE_STENCIL_FUNC_SEPARATE:
            CALL_StencilFuncSeparate(ctx->Exec,
                                     (n[1].e, n[2].e, n[3].i, n[4].ui));
            break;
         case OPCODE_STENCIL_MASK_SEPARATE:
            CALL_StencilMaskSeparate(ctx->Exec, (n[1].e, n[2].ui));
            break;
         case OPCODE_STENCIL_OP_SEPARATE:
            CALL_StencilOpSeparate(ctx->Exec,
                                   (n[1].e, n[2].e, n[3].e, n[4].e));
            break;
         case OPCODE_TEXENV:
            {
               GLfloat params[4];
               params[0] = n[3].f;
               params[1] = n[4].f;
               params[2] = n[5].f;
               params[3] = n[6].f;
               CALL_TexEnvfv(ctx->Exec, (n[1].e, n[2].e, params));
            }
            break;
         case OPCODE_TEXGEN:
            {
               GLfloat params[4];
               params[0] = n[3].f;
               params[1] = n[4].f;
               params[2] = n[5].f;
               params[3] = n[6].f;
               CALL_TexGenfv(ctx->Exec, (n[1].e, n[2].e, params));
            }
            break;
         case OPCODE_TEXPARAMETER:
            {
               GLfloat params[4];
               params[0] = n[3].f;
               params[1] = n[4].f;
               params[2] = n[5].f;
               params[3] = n[6].f;
               CALL_TexParameterfv(ctx->Exec, (n[1].e, n[2].e, params));
            }
            break;
         case OPCODE_TEX_IMAGE1D:
            {
               const struct gl_pixelstore_attrib save = ctx->Unpack;
               ctx->Unpack = ctx->DefaultPacking;
               CALL_TexImage1D(ctx->Exec, (n[1].e,      /* target */
                                           n[2].i,      /* level */
                                           n[3].i,      /* components */
                                           n[4].i,      /* width */
                                           n[5].e,      /* border */
                                           n[6].e,      /* format */
                                           n[7].e,      /* type */
                                           n[8].data));
               ctx->Unpack = save;      /* restore */
            }
            break;
         case OPCODE_TEX_IMAGE2D:
            {
               const struct gl_pixelstore_attrib save = ctx->Unpack;
               ctx->Unpack = ctx->DefaultPacking;
               CALL_TexImage2D(ctx->Exec, (n[1].e,      /* target */
                                           n[2].i,      /* level */
                                           n[3].i,      /* components */
                                           n[4].i,      /* width */
                                           n[5].i,      /* height */
                                           n[6].e,      /* border */
                                           n[7].e,      /* format */
                                           n[8].e,      /* type */
                                           n[9].data));
               ctx->Unpack = save;      /* restore */
            }
            break;
         case OPCODE_TEX_IMAGE3D:
            {
               const struct gl_pixelstore_attrib save = ctx->Unpack;
               ctx->Unpack = ctx->DefaultPacking;
               CALL_TexImage3D(ctx->Exec, (n[1].e,      /* target */
                                           n[2].i,      /* level */
                                           n[3].i,      /* components */
                                           n[4].i,      /* width */
                                           n[5].i,      /* height */
                                           n[6].i,      /* depth  */
                                           n[7].e,      /* border */
                                           n[8].e,      /* format */
                                           n[9].e,      /* type */
                                           n[10].data));
               ctx->Unpack = save;      /* restore */
            }
            break;
         case OPCODE_TEX_SUB_IMAGE1D:
            {
               const struct gl_pixelstore_attrib save = ctx->Unpack;
               ctx->Unpack = ctx->DefaultPacking;
               CALL_TexSubImage1D(ctx->Exec, (n[1].e, n[2].i, n[3].i,
                                              n[4].i, n[5].e,
                                              n[6].e, n[7].data));
               ctx->Unpack = save;      /* restore */
            }
            break;
         case OPCODE_TEX_SUB_IMAGE2D:
            {
               const struct gl_pixelstore_attrib save = ctx->Unpack;
               ctx->Unpack = ctx->DefaultPacking;
               CALL_TexSubImage2D(ctx->Exec, (n[1].e, n[2].i, n[3].i,
                                              n[4].i, n[5].e,
                                              n[6].i, n[7].e, n[8].e,
                                              n[9].data));
               ctx->Unpack = save;      /* restore */
            }
            break;
         case OPCODE_TEX_SUB_IMAGE3D:
            {
               const struct gl_pixelstore_attrib save = ctx->Unpack;
               ctx->Unpack = ctx->DefaultPacking;
               CALL_TexSubImage3D(ctx->Exec, (n[1].e, n[2].i, n[3].i,
                                              n[4].i, n[5].i, n[6].i, n[7].i,
                                              n[8].i, n[9].e, n[10].e,
                                              n[11].data));
               ctx->Unpack = save;      /* restore */
            }
            break;
         case OPCODE_TRANSLATE:
            CALL_Translatef(ctx->Exec, (n[1].f, n[2].f, n[3].f));
            break;
         case OPCODE_VIEWPORT:
            CALL_Viewport(ctx->Exec, (n[1].i, n[2].i,
                                      (GLsizei) n[3].i, (GLsizei) n[4].i));
            break;
         case OPCODE_WINDOW_POS:
            CALL_WindowPos4fMESA(ctx->Exec, (n[1].f, n[2].f, n[3].f, n[4].f));
            break;
         case OPCODE_ACTIVE_TEXTURE:   /* GL_ARB_multitexture */
            CALL_ActiveTextureARB(ctx->Exec, (n[1].e));
            break;
         case OPCODE_COMPRESSED_TEX_IMAGE_1D:  /* GL_ARB_texture_compression */
            CALL_CompressedTexImage1DARB(ctx->Exec, (n[1].e, n[2].i, n[3].e,
                                                     n[4].i, n[5].i, n[6].i,
                                                     n[7].data));
            break;
         case OPCODE_COMPRESSED_TEX_IMAGE_2D:  /* GL_ARB_texture_compression */
            CALL_CompressedTexImage2DARB(ctx->Exec, (n[1].e, n[2].i, n[3].e,
                                                     n[4].i, n[5].i, n[6].i,
                                                     n[7].i, n[8].data));
            break;
         case OPCODE_COMPRESSED_TEX_IMAGE_3D:  /* GL_ARB_texture_compression */
            CALL_CompressedTexImage3DARB(ctx->Exec, (n[1].e, n[2].i, n[3].e,
                                                     n[4].i, n[5].i, n[6].i,
                                                     n[7].i, n[8].i,
                                                     n[9].data));
            break;
         case OPCODE_COMPRESSED_TEX_SUB_IMAGE_1D:      /* GL_ARB_texture_compress */
            CALL_CompressedTexSubImage1DARB(ctx->Exec,
                                            (n[1].e, n[2].i, n[3].i, n[4].i,
                                             n[5].e, n[6].i, n[7].data));
            break;
         case OPCODE_COMPRESSED_TEX_SUB_IMAGE_2D:      /* GL_ARB_texture_compress */
            CALL_CompressedTexSubImage2DARB(ctx->Exec,
                                            (n[1].e, n[2].i, n[3].i, n[4].i,
                                             n[5].i, n[6].i, n[7].e, n[8].i,
                                             n[9].data));
            break;
         case OPCODE_COMPRESSED_TEX_SUB_IMAGE_3D:      /* GL_ARB_texture_compress */
            CALL_CompressedTexSubImage3DARB(ctx->Exec,
                                            (n[1].e, n[2].i, n[3].i, n[4].i,
                                             n[5].i, n[6].i, n[7].i, n[8].i,
                                             n[9].e, n[10].i, n[11].data));
            break;
         case OPCODE_SAMPLE_COVERAGE:  /* GL_ARB_multisample */
            CALL_SampleCoverageARB(ctx->Exec, (n[1].f, n[2].b));
            break;
         case OPCODE_WINDOW_POS_ARB:   /* GL_ARB_window_pos */
            CALL_WindowPos3fMESA(ctx->Exec, (n[1].f, n[2].f, n[3].f));
            break;
#if FEATURE_NV_vertex_program || FEATURE_ARB_vertex_program || FEATURE_ARB_fragment_program
         case OPCODE_BIND_PROGRAM_NV:  /* GL_NV_vertex_program */
            CALL_BindProgramNV(ctx->Exec, (n[1].e, n[2].ui));
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
               CALL_ExecuteProgramNV(ctx->Exec, (n[1].e, n[2].ui, v));
            }
            break;
         case OPCODE_REQUEST_RESIDENT_PROGRAMS_NV:
            CALL_RequestResidentProgramsNV(ctx->Exec, (n[1].ui,
                                                       (GLuint *) n[2].data));
            break;
         case OPCODE_LOAD_PROGRAM_NV:
            CALL_LoadProgramNV(ctx->Exec, (n[1].e, n[2].ui, n[3].i,
                                           (const GLubyte *) n[4].data));
            break;
         case OPCODE_TRACK_MATRIX_NV:
            CALL_TrackMatrixNV(ctx->Exec, (n[1].e, n[2].ui, n[3].e, n[4].e));
            break;
#endif

#if FEATURE_NV_fragment_program
         case OPCODE_PROGRAM_LOCAL_PARAMETER_ARB:
            CALL_ProgramLocalParameter4fARB(ctx->Exec,
                                            (n[1].e, n[2].ui, n[3].f, n[4].f,
                                             n[5].f, n[6].f));
            break;
         case OPCODE_PROGRAM_NAMED_PARAMETER_NV:
            CALL_ProgramNamedParameter4fNV(ctx->Exec, (n[1].ui, n[2].i,
                                                       (const GLubyte *) n[3].
                                                       data, n[4].f, n[5].f,
                                                       n[6].f, n[7].f));
            break;
#endif

         case OPCODE_ACTIVE_STENCIL_FACE_EXT:
            CALL_ActiveStencilFaceEXT(ctx->Exec, (n[1].e));
            break;
         case OPCODE_DEPTH_BOUNDS_EXT:
            CALL_DepthBoundsEXT(ctx->Exec, (n[1].f, n[2].f));
            break;
#if FEATURE_ARB_vertex_program || FEATURE_ARB_fragment_program
         case OPCODE_PROGRAM_STRING_ARB:
            CALL_ProgramStringARB(ctx->Exec,
                                  (n[1].e, n[2].e, n[3].i, n[4].data));
            break;
#endif
#if FEATURE_ARB_vertex_program || FEATURE_ARB_fragment_program || FEATURE_NV_vertex_program
         case OPCODE_PROGRAM_ENV_PARAMETER_ARB:
            CALL_ProgramEnvParameter4fARB(ctx->Exec, (n[1].e, n[2].ui, n[3].f,
                                                      n[4].f, n[5].f,
                                                      n[6].f));
            break;
#endif
#if FEATURE_queryobj
         case OPCODE_BEGIN_QUERY_ARB:
            CALL_BeginQueryARB(ctx->Exec, (n[1].e, n[2].ui));
            break;
         case OPCODE_END_QUERY_ARB:
            CALL_EndQueryARB(ctx->Exec, (n[1].e));
            break;
#endif
         case OPCODE_DRAW_BUFFERS_ARB:
            {
               GLenum buffers[MAX_DRAW_BUFFERS];
               GLint i, count = MIN2(n[1].i, MAX_DRAW_BUFFERS);
               for (i = 0; i < count; i++)
                  buffers[i] = n[2 + i].e;
               CALL_DrawBuffersARB(ctx->Exec, (n[1].i, buffers));
            }
            break;
#if FEATURE_EXT_framebuffer_blit
	 case OPCODE_BLIT_FRAMEBUFFER:
	    CALL_BlitFramebufferEXT(ctx->Exec, (n[1].i, n[2].i, n[3].i, n[4].i,
                                                n[5].i, n[6].i, n[7].i, n[8].i,
                                                n[9].i, n[10].e));
	    break;
#endif

	 case OPCODE_USE_PROGRAM:
	    CALL_UseProgramObjectARB(ctx->Exec, (n[1].ui));
	    break;
	 case OPCODE_USE_SHADER_PROGRAM_EXT:
	    CALL_UseShaderProgramEXT(ctx->Exec, (n[1].ui, n[2].ui));
	    break;
	 case OPCODE_ACTIVE_PROGRAM_EXT:
	    CALL_ActiveProgramEXT(ctx->Exec, (n[1].ui));
	    break;
	 case OPCODE_UNIFORM_1F:
	    CALL_Uniform1fARB(ctx->Exec, (n[1].i, n[2].f));
	    break;
	 case OPCODE_UNIFORM_2F:
	    CALL_Uniform2fARB(ctx->Exec, (n[1].i, n[2].f, n[3].f));
	    break;
	 case OPCODE_UNIFORM_3F:
	    CALL_Uniform3fARB(ctx->Exec, (n[1].i, n[2].f, n[3].f, n[4].f));
	    break;
	 case OPCODE_UNIFORM_4F:
	    CALL_Uniform4fARB(ctx->Exec,
                              (n[1].i, n[2].f, n[3].f, n[4].f, n[5].f));
	    break;
	 case OPCODE_UNIFORM_1FV:
	    CALL_Uniform1fvARB(ctx->Exec, (n[1].i, n[2].i, n[3].data));
	    break;
	 case OPCODE_UNIFORM_2FV:
	    CALL_Uniform2fvARB(ctx->Exec, (n[1].i, n[2].i, n[3].data));
	    break;
	 case OPCODE_UNIFORM_3FV:
	    CALL_Uniform3fvARB(ctx->Exec, (n[1].i, n[2].i, n[3].data));
	    break;
	 case OPCODE_UNIFORM_4FV:
	    CALL_Uniform4fvARB(ctx->Exec, (n[1].i, n[2].i, n[3].data));
	    break;
	 case OPCODE_UNIFORM_1I:
	    CALL_Uniform1iARB(ctx->Exec, (n[1].i, n[2].i));
	    break;
	 case OPCODE_UNIFORM_2I:
	    CALL_Uniform2iARB(ctx->Exec, (n[1].i, n[2].i, n[3].i));
	    break;
	 case OPCODE_UNIFORM_3I:
	    CALL_Uniform3iARB(ctx->Exec, (n[1].i, n[2].i, n[3].i, n[4].i));
	    break;
	 case OPCODE_UNIFORM_4I:
	    CALL_Uniform4iARB(ctx->Exec,
                              (n[1].i, n[2].i, n[3].i, n[4].i, n[5].i));
	    break;
	 case OPCODE_UNIFORM_1IV:
	    CALL_Uniform1ivARB(ctx->Exec, (n[1].i, n[2].i, n[3].data));
	    break;
	 case OPCODE_UNIFORM_2IV:
	    CALL_Uniform2ivARB(ctx->Exec, (n[1].i, n[2].i, n[3].data));
	    break;
	 case OPCODE_UNIFORM_3IV:
	    CALL_Uniform3ivARB(ctx->Exec, (n[1].i, n[2].i, n[3].data));
	    break;
	 case OPCODE_UNIFORM_4IV:
	    CALL_Uniform4ivARB(ctx->Exec, (n[1].i, n[2].i, n[3].data));
	    break;
	 case OPCODE_UNIFORM_1UI:
	    /*CALL_Uniform1uiARB(ctx->Exec, (n[1].i, n[2].i));*/
	    break;
	 case OPCODE_UNIFORM_2UI:
	    /*CALL_Uniform2uiARB(ctx->Exec, (n[1].i, n[2].i, n[3].i));*/
	    break;
	 case OPCODE_UNIFORM_3UI:
	    /*CALL_Uniform3uiARB(ctx->Exec, (n[1].i, n[2].i, n[3].i, n[4].i));*/
	    break;
	 case OPCODE_UNIFORM_4UI:
	    /*CALL_Uniform4uiARB(ctx->Exec,
                              (n[1].i, n[2].i, n[3].i, n[4].i, n[5].i));
            */
	    break;
	 case OPCODE_UNIFORM_1UIV:
	    /*CALL_Uniform1uivARB(ctx->Exec, (n[1].i, n[2].i, n[3].data));*/
	    break;
	 case OPCODE_UNIFORM_2UIV:
	    /*CALL_Uniform2uivARB(ctx->Exec, (n[1].i, n[2].i, n[3].data));*/
	    break;
	 case OPCODE_UNIFORM_3UIV:
	    /*CALL_Uniform3uivARB(ctx->Exec, (n[1].i, n[2].i, n[3].data));*/
	    break;
	 case OPCODE_UNIFORM_4UIV:
	    /*CALL_Uniform4uivARB(ctx->Exec, (n[1].i, n[2].i, n[3].data));*/
	    break;
	 case OPCODE_UNIFORM_MATRIX22:
	    CALL_UniformMatrix2fvARB(ctx->Exec,
                                     (n[1].i, n[2].i, n[3].b, n[4].data));
	    break;
	 case OPCODE_UNIFORM_MATRIX33:
	    CALL_UniformMatrix3fvARB(ctx->Exec,
                                     (n[1].i, n[2].i, n[3].b, n[4].data));
	    break;
	 case OPCODE_UNIFORM_MATRIX44:
	    CALL_UniformMatrix4fvARB(ctx->Exec,
                                     (n[1].i, n[2].i, n[3].b, n[4].data));
	    break;
	 case OPCODE_UNIFORM_MATRIX23:
	    CALL_UniformMatrix2x3fv(ctx->Exec,
                                    (n[1].i, n[2].i, n[3].b, n[4].data));
	    break;
	 case OPCODE_UNIFORM_MATRIX32:
	    CALL_UniformMatrix3x2fv(ctx->Exec,
                                    (n[1].i, n[2].i, n[3].b, n[4].data));
	    break;
	 case OPCODE_UNIFORM_MATRIX24:
	    CALL_UniformMatrix2x4fv(ctx->Exec,
                                    (n[1].i, n[2].i, n[3].b, n[4].data));
	    break;
	 case OPCODE_UNIFORM_MATRIX42:
	    CALL_UniformMatrix4x2fv(ctx->Exec,
                                    (n[1].i, n[2].i, n[3].b, n[4].data));
	    break;
	 case OPCODE_UNIFORM_MATRIX34:
	    CALL_UniformMatrix3x4fv(ctx->Exec,
                                    (n[1].i, n[2].i, n[3].b, n[4].data));
	    break;
	 case OPCODE_UNIFORM_MATRIX43:
	    CALL_UniformMatrix4x3fv(ctx->Exec,
                                    (n[1].i, n[2].i, n[3].b, n[4].data));
	    break;

         case OPCODE_CLAMP_COLOR:
            CALL_ClampColorARB(ctx->Exec, (n[1].e, n[2].e));
            break;

         case OPCODE_TEX_BUMP_PARAMETER_ATI:
            {
               GLfloat values[4];
               GLuint i, pname = n[1].ui;

               for (i = 0; i < 4; i++)
                  values[i] = n[1 + i].f;
               CALL_TexBumpParameterfvATI(ctx->Exec, (pname, values));
            }
            break;
#if FEATURE_ATI_fragment_shader
         case OPCODE_BIND_FRAGMENT_SHADER_ATI:
            CALL_BindFragmentShaderATI(ctx->Exec, (n[1].i));
            break;
         case OPCODE_SET_FRAGMENT_SHADER_CONSTANTS_ATI:
            {
               GLfloat values[4];
               GLuint i, dst = n[1].ui;

               for (i = 0; i < 4; i++)
                  values[i] = n[1 + i].f;
               CALL_SetFragmentShaderConstantATI(ctx->Exec, (dst, values));
            }
            break;
#endif
         case OPCODE_ATTR_1F_NV:
            CALL_VertexAttrib1fNV(ctx->Exec, (n[1].e, n[2].f));
            break;
         case OPCODE_ATTR_2F_NV:
            /* Really shouldn't have to do this - the Node structure
             * is convenient, but it would be better to store the data
             * packed appropriately so that it can be sent directly
             * on.  With x86_64 becoming common, this will start to
             * matter more.
             */
            if (sizeof(Node) == sizeof(GLfloat))
               CALL_VertexAttrib2fvNV(ctx->Exec, (n[1].e, &n[2].f));
            else
               CALL_VertexAttrib2fNV(ctx->Exec, (n[1].e, n[2].f, n[3].f));
            break;
         case OPCODE_ATTR_3F_NV:
            if (sizeof(Node) == sizeof(GLfloat))
               CALL_VertexAttrib3fvNV(ctx->Exec, (n[1].e, &n[2].f));
            else
               CALL_VertexAttrib3fNV(ctx->Exec, (n[1].e, n[2].f, n[3].f,
                                                 n[4].f));
            break;
         case OPCODE_ATTR_4F_NV:
            if (sizeof(Node) == sizeof(GLfloat))
               CALL_VertexAttrib4fvNV(ctx->Exec, (n[1].e, &n[2].f));
            else
               CALL_VertexAttrib4fNV(ctx->Exec, (n[1].e, n[2].f, n[3].f,
                                                 n[4].f, n[5].f));
            break;
         case OPCODE_ATTR_1F_ARB:
            CALL_VertexAttrib1fARB(ctx->Exec, (n[1].e, n[2].f));
            break;
         case OPCODE_ATTR_2F_ARB:
            /* Really shouldn't have to do this - the Node structure
             * is convenient, but it would be better to store the data
             * packed appropriately so that it can be sent directly
             * on.  With x86_64 becoming common, this will start to
             * matter more.
             */
            if (sizeof(Node) == sizeof(GLfloat))
               CALL_VertexAttrib2fvARB(ctx->Exec, (n[1].e, &n[2].f));
            else
               CALL_VertexAttrib2fARB(ctx->Exec, (n[1].e, n[2].f, n[3].f));
            break;
         case OPCODE_ATTR_3F_ARB:
            if (sizeof(Node) == sizeof(GLfloat))
               CALL_VertexAttrib3fvARB(ctx->Exec, (n[1].e, &n[2].f));
            else
               CALL_VertexAttrib3fARB(ctx->Exec, (n[1].e, n[2].f, n[3].f,
                                                  n[4].f));
            break;
         case OPCODE_ATTR_4F_ARB:
            if (sizeof(Node) == sizeof(GLfloat))
               CALL_VertexAttrib4fvARB(ctx->Exec, (n[1].e, &n[2].f));
            else
               CALL_VertexAttrib4fARB(ctx->Exec, (n[1].e, n[2].f, n[3].f,
                                                  n[4].f, n[5].f));
            break;
         case OPCODE_MATERIAL:
            if (sizeof(Node) == sizeof(GLfloat))
               CALL_Materialfv(ctx->Exec, (n[1].e, n[2].e, &n[3].f));
            else {
               GLfloat f[4];
               f[0] = n[3].f;
               f[1] = n[4].f;
               f[2] = n[5].f;
               f[3] = n[6].f;
               CALL_Materialfv(ctx->Exec, (n[1].e, n[2].e, f));
            }
            break;
         case OPCODE_BEGIN:
            CALL_Begin(ctx->Exec, (n[1].e));
            break;
         case OPCODE_END:
            CALL_End(ctx->Exec, ());
            break;
         case OPCODE_RECTF:
            CALL_Rectf(ctx->Exec, (n[1].f, n[2].f, n[3].f, n[4].f));
            break;
         case OPCODE_EVAL_C1:
            CALL_EvalCoord1f(ctx->Exec, (n[1].f));
            break;
         case OPCODE_EVAL_C2:
            CALL_EvalCoord2f(ctx->Exec, (n[1].f, n[2].f));
            break;
         case OPCODE_EVAL_P1:
            CALL_EvalPoint1(ctx->Exec, (n[1].i));
            break;
         case OPCODE_EVAL_P2:
            CALL_EvalPoint2(ctx->Exec, (n[1].i, n[2].i));
            break;

         /* GL_EXT_texture_integer */
         case OPCODE_CLEARCOLOR_I:
            CALL_ClearColorIiEXT(ctx->Exec, (n[1].i, n[2].i, n[3].i, n[4].i));
            break;
         case OPCODE_CLEARCOLOR_UI:
            CALL_ClearColorIuiEXT(ctx->Exec,
                                  (n[1].ui, n[2].ui, n[3].ui, n[4].ui));
            break;
         case OPCODE_TEXPARAMETER_I:
            {
               GLint params[4];
               params[0] = n[3].i;
               params[1] = n[4].i;
               params[2] = n[5].i;
               params[3] = n[6].i;
               CALL_TexParameterIivEXT(ctx->Exec, (n[1].e, n[2].e, params));
            }
            break;
         case OPCODE_TEXPARAMETER_UI:
            {
               GLuint params[4];
               params[0] = n[3].ui;
               params[1] = n[4].ui;
               params[2] = n[5].ui;
               params[3] = n[6].ui;
               CALL_TexParameterIuivEXT(ctx->Exec, (n[1].e, n[2].e, params));
            }
            break;

         case OPCODE_VERTEX_ATTRIB_DIVISOR:
            /* GL_ARB_instanced_arrays */
            CALL_VertexAttribDivisorARB(ctx->Exec, (n[1].ui, n[2].ui));
            break;

         case OPCODE_TEXTURE_BARRIER_NV:
            CALL_TextureBarrierNV(ctx->Exec, ());
            break;

         /* GL_EXT/ARB_transform_feedback */
         case OPCODE_BEGIN_TRANSFORM_FEEDBACK:
            CALL_BeginTransformFeedbackEXT(ctx->Exec, (n[1].e));
            break;
         case OPCODE_END_TRANSFORM_FEEDBACK:
            CALL_EndTransformFeedbackEXT(ctx->Exec, ());
            break;
         case OPCODE_BIND_TRANSFORM_FEEDBACK:
            CALL_BindTransformFeedback(ctx->Exec, (n[1].e, n[2].ui));
            break;
         case OPCODE_PAUSE_TRANSFORM_FEEDBACK:
            CALL_PauseTransformFeedback(ctx->Exec, ());
            break;
         case OPCODE_RESUME_TRANSFORM_FEEDBACK:
            CALL_ResumeTransformFeedback(ctx->Exec, ());
            break;
         case OPCODE_DRAW_TRANSFORM_FEEDBACK:
            CALL_DrawTransformFeedback(ctx->Exec, (n[1].e, n[2].ui));
            break;


         case OPCODE_BIND_SAMPLER:
            CALL_BindSampler(ctx->Exec, (n[1].ui, n[2].ui));
            break;
         case OPCODE_SAMPLER_PARAMETERIV:
            {
               GLint params[4];
               params[0] = n[3].i;
               params[1] = n[4].i;
               params[2] = n[5].i;
               params[3] = n[6].i;
               CALL_SamplerParameteriv(ctx->Exec, (n[1].ui, n[2].e, params));
            }
            break;
         case OPCODE_SAMPLER_PARAMETERFV:
            {
               GLfloat params[4];
               params[0] = n[3].f;
               params[1] = n[4].f;
               params[2] = n[5].f;
               params[3] = n[6].f;
               CALL_SamplerParameterfv(ctx->Exec, (n[1].ui, n[2].e, params));
            }
            break;
         case OPCODE_SAMPLER_PARAMETERIIV:
            {
               GLint params[4];
               params[0] = n[3].i;
               params[1] = n[4].i;
               params[2] = n[5].i;
               params[3] = n[6].i;
               CALL_SamplerParameterIiv(ctx->Exec, (n[1].ui, n[2].e, params));
            }
            break;
         case OPCODE_SAMPLER_PARAMETERUIV:
            {
               GLuint params[4];
               params[0] = n[3].ui;
               params[1] = n[4].ui;
               params[2] = n[5].ui;
               params[3] = n[6].ui;
               CALL_SamplerParameterIuiv(ctx->Exec, (n[1].ui, n[2].e, params));
            }
            break;

         /* GL_ARB_geometry_shader4 */
         case OPCODE_PROGRAM_PARAMETERI:
            CALL_ProgramParameteriARB(ctx->Exec, (n[1].ui, n[2].e, n[3].i));
            break;
         case OPCODE_FRAMEBUFFER_TEXTURE:
            CALL_FramebufferTextureARB(ctx->Exec, (n[1].e, n[2].e,
                                                   n[3].ui, n[4].i));
            break;
         case OPCODE_FRAMEBUFFER_TEXTURE_FACE:
            CALL_FramebufferTextureFaceARB(ctx->Exec, (n[1].e, n[2].e,
                                                       n[3].ui, n[4].i, n[5].e));
            break;

         /* GL_ARB_sync */
         case OPCODE_WAIT_SYNC:
            {
               union uint64_pair p;
               p.uint32[0] = n[3].ui;
               p.uint32[1] = n[4].ui;
               CALL_WaitSync(ctx->Exec, (n[1].data, n[2].bf, p.uint64));
            }
            break;

         /* GL_NV_conditional_render */
         case OPCODE_BEGIN_CONDITIONAL_RENDER:
            CALL_BeginConditionalRenderNV(ctx->Exec, (n[1].i, n[2].e));
            break;
         case OPCODE_END_CONDITIONAL_RENDER:
            CALL_EndConditionalRenderNV(ctx->Exec, ());
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
               _mesa_snprintf(msg, sizeof(msg), "Error in execute_list: opcode=%d",
                             (int) opcode);
               _mesa_problem(ctx, "%s", msg);
            }
            done = GL_TRUE;
         }

         /* increment n to point to next compiled command */
         if (opcode != OPCODE_CONTINUE) {
            n += InstSize[opcode];
         }
      }
   }

   if (ctx->Driver.EndCallList)
      ctx->Driver.EndCallList(ctx);

   ctx->ListState.CallDepth--;
}



/**********************************************************************/
/*                           GL functions                             */
/**********************************************************************/

/**
 * Test if a display list number is valid.
 */
static GLboolean GLAPIENTRY
_mesa_IsList(GLuint list)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);      /* must be called before assert */
   ASSERT_OUTSIDE_BEGIN_END_WITH_RETVAL(ctx, GL_FALSE);
   return islist(ctx, list);
}


/**
 * Delete a sequence of consecutive display lists.
 */
static void GLAPIENTRY
_mesa_DeleteLists(GLuint list, GLsizei range)
{
   GET_CURRENT_CONTEXT(ctx);
   GLuint i;
   FLUSH_VERTICES(ctx, 0);      /* must be called before assert */
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (range < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glDeleteLists");
      return;
   }
   for (i = list; i < list + range; i++) {
      destroy_list(ctx, i);
   }
}


/**
 * Return a display list number, n, such that lists n through n+range-1
 * are free.
 */
static GLuint GLAPIENTRY
_mesa_GenLists(GLsizei range)
{
   GET_CURRENT_CONTEXT(ctx);
   GLuint base;
   FLUSH_VERTICES(ctx, 0);      /* must be called before assert */
   ASSERT_OUTSIDE_BEGIN_END_WITH_RETVAL(ctx, 0);

   if (range < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glGenLists");
      return 0;
   }
   if (range == 0) {
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
      for (i = 0; i < range; i++) {
         _mesa_HashInsert(ctx->Shared->DisplayList, base + i,
                          make_list(base + i, 1));
      }
   }

   _glthread_UNLOCK_MUTEX(ctx->Shared->Mutex);

   return base;
}


/**
 * Begin a new display list.
 */
static void GLAPIENTRY
_mesa_NewList(GLuint name, GLenum mode)
{
   GET_CURRENT_CONTEXT(ctx);

   FLUSH_CURRENT(ctx, 0);       /* must be called before assert */
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx, "glNewList %u %s\n", name,
                  _mesa_lookup_enum_by_nr(mode));

   if (name == 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glNewList");
      return;
   }

   if (mode != GL_COMPILE && mode != GL_COMPILE_AND_EXECUTE) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glNewList");
      return;
   }

   if (ctx->ListState.CurrentList) {
      /* already compiling a display list */
      _mesa_error(ctx, GL_INVALID_OPERATION, "glNewList");
      return;
   }

   ctx->CompileFlag = GL_TRUE;
   ctx->ExecuteFlag = (mode == GL_COMPILE_AND_EXECUTE);

   /* Reset acumulated list state:
    */
   invalidate_saved_current_state( ctx );

   /* Allocate new display list */
   ctx->ListState.CurrentList = make_list(name, BLOCK_SIZE);
   ctx->ListState.CurrentBlock = ctx->ListState.CurrentList->Head;
   ctx->ListState.CurrentPos = 0;

   ctx->Driver.NewList(ctx, name, mode);

   ctx->CurrentDispatch = ctx->Save;
   _glapi_set_dispatch(ctx->CurrentDispatch);
}


/**
 * End definition of current display list. 
 */
static void GLAPIENTRY
_mesa_EndList(void)
{
   GET_CURRENT_CONTEXT(ctx);
   SAVE_FLUSH_VERTICES(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx, "glEndList\n");

   /* Check that a list is under construction */
   if (!ctx->ListState.CurrentList) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glEndList");
      return;
   }
   
   /* Call before emitting END_OF_LIST, in case the driver wants to
    * emit opcodes itself.
    */
   ctx->Driver.EndList(ctx);

   (void) alloc_instruction(ctx, OPCODE_END_OF_LIST, 0);

   /* Destroy old list, if any */
   destroy_list(ctx, ctx->ListState.CurrentList->Name);

   /* Install the new list */
   _mesa_HashInsert(ctx->Shared->DisplayList,
                    ctx->ListState.CurrentList->Name,
                    ctx->ListState.CurrentList);


   if (MESA_VERBOSE & VERBOSE_DISPLAY_LIST)
      mesa_print_display_list(ctx->ListState.CurrentList->Name);

   ctx->ListState.CurrentList = NULL;
   ctx->ExecuteFlag = GL_TRUE;
   ctx->CompileFlag = GL_FALSE;

   ctx->CurrentDispatch = ctx->Exec;
   _glapi_set_dispatch(ctx->CurrentDispatch);
}


void GLAPIENTRY
_mesa_CallList(GLuint list)
{
   GLboolean save_compile_flag;
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_CURRENT(ctx, 0);

   if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx, "glCallList %d\n", list);

   if (list == 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glCallList(list==0)");
      return;
   }

   if (0)
      mesa_print_display_list( list );

   /* VERY IMPORTANT:  Save the CompileFlag status, turn it off,
    * execute the display list, and restore the CompileFlag.
    */
   save_compile_flag = ctx->CompileFlag;
   if (save_compile_flag) {
      ctx->CompileFlag = GL_FALSE;
   }

   execute_list(ctx, list);
   ctx->CompileFlag = save_compile_flag;

   /* also restore API function pointers to point to "save" versions */
   if (save_compile_flag) {
      ctx->CurrentDispatch = ctx->Save;
      _glapi_set_dispatch(ctx->CurrentDispatch);
   }
}


/**
 * Execute glCallLists:  call multiple display lists.
 */
void GLAPIENTRY
_mesa_CallLists(GLsizei n, GLenum type, const GLvoid * lists)
{
   GET_CURRENT_CONTEXT(ctx);
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

   for (i = 0; i < n; i++) {
      GLuint list = (GLuint) (ctx->List.ListBase + translate_id(i, type, lists));
      execute_list(ctx, list);
   }

   ctx->CompileFlag = save_compile_flag;

   /* also restore API function pointers to point to "save" versions */
   if (save_compile_flag) {
      ctx->CurrentDispatch = ctx->Save;
      _glapi_set_dispatch(ctx->CurrentDispatch);
   }
}


/**
 * Set the offset added to list numbers in glCallLists.
 */
static void GLAPIENTRY
_mesa_ListBase(GLuint base)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);      /* must be called before assert */
   ASSERT_OUTSIDE_BEGIN_END(ctx);
   ctx->List.ListBase = base;
}


/* Can no longer assume ctx->Exec->Func is equal to _mesa_Func.
 */
static void GLAPIENTRY
exec_Finish(void)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   CALL_Finish(ctx->Exec, ());
}

static void GLAPIENTRY
exec_Flush(void)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   CALL_Flush(ctx->Exec, ());
}

static void GLAPIENTRY
exec_GetBooleanv(GLenum pname, GLboolean *params)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   CALL_GetBooleanv(ctx->Exec, (pname, params));
}

static void GLAPIENTRY
exec_GetClipPlane(GLenum plane, GLdouble * equation)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   CALL_GetClipPlane(ctx->Exec, (plane, equation));
}

static void GLAPIENTRY
exec_GetDoublev(GLenum pname, GLdouble *params)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   CALL_GetDoublev(ctx->Exec, (pname, params));
}

static GLenum GLAPIENTRY
exec_GetError(void)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   return CALL_GetError(ctx->Exec, ());
}

static void GLAPIENTRY
exec_GetFloatv(GLenum pname, GLfloat *params)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   CALL_GetFloatv(ctx->Exec, (pname, params));
}

static void GLAPIENTRY
exec_GetIntegerv(GLenum pname, GLint *params)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   CALL_GetIntegerv(ctx->Exec, (pname, params));
}

static void GLAPIENTRY
exec_GetLightfv(GLenum light, GLenum pname, GLfloat *params)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   CALL_GetLightfv(ctx->Exec, (light, pname, params));
}

static void GLAPIENTRY
exec_GetLightiv(GLenum light, GLenum pname, GLint *params)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   CALL_GetLightiv(ctx->Exec, (light, pname, params));
}

static void GLAPIENTRY
exec_GetMapdv(GLenum target, GLenum query, GLdouble * v)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   CALL_GetMapdv(ctx->Exec, (target, query, v));
}

static void GLAPIENTRY
exec_GetMapfv(GLenum target, GLenum query, GLfloat * v)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   CALL_GetMapfv(ctx->Exec, (target, query, v));
}

static void GLAPIENTRY
exec_GetMapiv(GLenum target, GLenum query, GLint * v)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   CALL_GetMapiv(ctx->Exec, (target, query, v));
}

static void GLAPIENTRY
exec_GetMaterialfv(GLenum face, GLenum pname, GLfloat *params)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   CALL_GetMaterialfv(ctx->Exec, (face, pname, params));
}

static void GLAPIENTRY
exec_GetMaterialiv(GLenum face, GLenum pname, GLint *params)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   CALL_GetMaterialiv(ctx->Exec, (face, pname, params));
}

static void GLAPIENTRY
exec_GetPixelMapfv(GLenum map, GLfloat *values)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   CALL_GetPixelMapfv(ctx->Exec, (map, values));
}

static void GLAPIENTRY
exec_GetPixelMapuiv(GLenum map, GLuint *values)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   CALL_GetPixelMapuiv(ctx->Exec, (map, values));
}

static void GLAPIENTRY
exec_GetPixelMapusv(GLenum map, GLushort *values)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   CALL_GetPixelMapusv(ctx->Exec, (map, values));
}

static void GLAPIENTRY
exec_GetPolygonStipple(GLubyte * dest)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   CALL_GetPolygonStipple(ctx->Exec, (dest));
}

static const GLubyte *GLAPIENTRY
exec_GetString(GLenum name)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   return CALL_GetString(ctx->Exec, (name));
}

static void GLAPIENTRY
exec_GetTexEnvfv(GLenum target, GLenum pname, GLfloat *params)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   CALL_GetTexEnvfv(ctx->Exec, (target, pname, params));
}

static void GLAPIENTRY
exec_GetTexEnviv(GLenum target, GLenum pname, GLint *params)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   CALL_GetTexEnviv(ctx->Exec, (target, pname, params));
}

static void GLAPIENTRY
exec_GetTexGendv(GLenum coord, GLenum pname, GLdouble *params)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   CALL_GetTexGendv(ctx->Exec, (coord, pname, params));
}

static void GLAPIENTRY
exec_GetTexGenfv(GLenum coord, GLenum pname, GLfloat *params)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   CALL_GetTexGenfv(ctx->Exec, (coord, pname, params));
}

static void GLAPIENTRY
exec_GetTexGeniv(GLenum coord, GLenum pname, GLint *params)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   CALL_GetTexGeniv(ctx->Exec, (coord, pname, params));
}

static void GLAPIENTRY
exec_GetTexImage(GLenum target, GLint level, GLenum format,
                 GLenum type, GLvoid * pixels)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   CALL_GetTexImage(ctx->Exec, (target, level, format, type, pixels));
}

static void GLAPIENTRY
exec_GetTexLevelParameterfv(GLenum target, GLint level,
                            GLenum pname, GLfloat *params)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   CALL_GetTexLevelParameterfv(ctx->Exec, (target, level, pname, params));
}

static void GLAPIENTRY
exec_GetTexLevelParameteriv(GLenum target, GLint level,
                            GLenum pname, GLint *params)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   CALL_GetTexLevelParameteriv(ctx->Exec, (target, level, pname, params));
}

static void GLAPIENTRY
exec_GetTexParameterfv(GLenum target, GLenum pname, GLfloat *params)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   CALL_GetTexParameterfv(ctx->Exec, (target, pname, params));
}

static void GLAPIENTRY
exec_GetTexParameteriv(GLenum target, GLenum pname, GLint *params)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   CALL_GetTexParameteriv(ctx->Exec, (target, pname, params));
}

static GLboolean GLAPIENTRY
exec_IsEnabled(GLenum cap)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   return CALL_IsEnabled(ctx->Exec, (cap));
}

static void GLAPIENTRY
exec_PixelStoref(GLenum pname, GLfloat param)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   CALL_PixelStoref(ctx->Exec, (pname, param));
}

static void GLAPIENTRY
exec_PixelStorei(GLenum pname, GLint param)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   CALL_PixelStorei(ctx->Exec, (pname, param));
}

static void GLAPIENTRY
exec_ReadPixels(GLint x, GLint y, GLsizei width, GLsizei height,
                GLenum format, GLenum type, GLvoid * pixels)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   CALL_ReadPixels(ctx->Exec, (x, y, width, height, format, type, pixels));
}

static GLint GLAPIENTRY
exec_RenderMode(GLenum mode)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   return CALL_RenderMode(ctx->Exec, (mode));
}

static void GLAPIENTRY
exec_FeedbackBuffer(GLsizei size, GLenum type, GLfloat * buffer)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   CALL_FeedbackBuffer(ctx->Exec, (size, type, buffer));
}

static void GLAPIENTRY
exec_SelectBuffer(GLsizei size, GLuint * buffer)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   CALL_SelectBuffer(ctx->Exec, (size, buffer));
}

static GLboolean GLAPIENTRY
exec_AreTexturesResident(GLsizei n, const GLuint * texName,
                         GLboolean * residences)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   return CALL_AreTexturesResident(ctx->Exec, (n, texName, residences));
}

static void GLAPIENTRY
exec_ColorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *ptr)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   CALL_ColorPointer(ctx->Exec, (size, type, stride, ptr));
}

static void GLAPIENTRY
exec_DeleteTextures(GLsizei n, const GLuint * texName)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   CALL_DeleteTextures(ctx->Exec, (n, texName));
}

static void GLAPIENTRY
exec_DisableClientState(GLenum cap)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   CALL_DisableClientState(ctx->Exec, (cap));
}

static void GLAPIENTRY
exec_EdgeFlagPointer(GLsizei stride, const GLvoid * vptr)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   CALL_EdgeFlagPointer(ctx->Exec, (stride, vptr));
}

static void GLAPIENTRY
exec_EnableClientState(GLenum cap)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   CALL_EnableClientState(ctx->Exec, (cap));
}

static void GLAPIENTRY
exec_GenTextures(GLsizei n, GLuint * texName)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   CALL_GenTextures(ctx->Exec, (n, texName));
}

static void GLAPIENTRY
exec_GetPointerv(GLenum pname, GLvoid **params)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   CALL_GetPointerv(ctx->Exec, (pname, params));
}

static void GLAPIENTRY
exec_IndexPointer(GLenum type, GLsizei stride, const GLvoid *ptr)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   CALL_IndexPointer(ctx->Exec, (type, stride, ptr));
}

static void GLAPIENTRY
exec_InterleavedArrays(GLenum format, GLsizei stride, const GLvoid * pointer)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   CALL_InterleavedArrays(ctx->Exec, (format, stride, pointer));
}

static GLboolean GLAPIENTRY
exec_IsTexture(GLuint texture)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   return CALL_IsTexture(ctx->Exec, (texture));
}

static void GLAPIENTRY
exec_NormalPointer(GLenum type, GLsizei stride, const GLvoid *ptr)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   CALL_NormalPointer(ctx->Exec, (type, stride, ptr));
}

static void GLAPIENTRY
exec_PopClientAttrib(void)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   CALL_PopClientAttrib(ctx->Exec, ());
}

static void GLAPIENTRY
exec_PushClientAttrib(GLbitfield mask)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   CALL_PushClientAttrib(ctx->Exec, (mask));
}

static void GLAPIENTRY
exec_TexCoordPointer(GLint size, GLenum type, GLsizei stride,
                     const GLvoid *ptr)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   CALL_TexCoordPointer(ctx->Exec, (size, type, stride, ptr));
}

static void GLAPIENTRY
exec_GetCompressedTexImageARB(GLenum target, GLint level, GLvoid * img)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   CALL_GetCompressedTexImageARB(ctx->Exec, (target, level, img));
}

static void GLAPIENTRY
exec_VertexPointer(GLint size, GLenum type, GLsizei stride,
                   const GLvoid *ptr)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   CALL_VertexPointer(ctx->Exec, (size, type, stride, ptr));
}

static void GLAPIENTRY
exec_CopyConvolutionFilter1D(GLenum target, GLenum internalFormat,
                             GLint x, GLint y, GLsizei width)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   CALL_CopyConvolutionFilter1D(ctx->Exec,
                                (target, internalFormat, x, y, width));
}

static void GLAPIENTRY
exec_CopyConvolutionFilter2D(GLenum target, GLenum internalFormat,
                             GLint x, GLint y, GLsizei width, GLsizei height)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   CALL_CopyConvolutionFilter2D(ctx->Exec,
                                (target, internalFormat, x, y, width,
                                 height));
}

static void GLAPIENTRY
exec_GetColorTable(GLenum target, GLenum format, GLenum type, GLvoid * data)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   CALL_GetColorTable(ctx->Exec, (target, format, type, data));
}

static void GLAPIENTRY
exec_GetColorTableParameterfv(GLenum target, GLenum pname, GLfloat *params)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   CALL_GetColorTableParameterfv(ctx->Exec, (target, pname, params));
}

static void GLAPIENTRY
exec_GetColorTableParameteriv(GLenum target, GLenum pname, GLint *params)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   CALL_GetColorTableParameteriv(ctx->Exec, (target, pname, params));
}

static void GLAPIENTRY
exec_GetConvolutionFilter(GLenum target, GLenum format, GLenum type,
                          GLvoid * image)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   CALL_GetConvolutionFilter(ctx->Exec, (target, format, type, image));
}

static void GLAPIENTRY
exec_GetConvolutionParameterfv(GLenum target, GLenum pname, GLfloat *params)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   CALL_GetConvolutionParameterfv(ctx->Exec, (target, pname, params));
}

static void GLAPIENTRY
exec_GetConvolutionParameteriv(GLenum target, GLenum pname, GLint *params)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   CALL_GetConvolutionParameteriv(ctx->Exec, (target, pname, params));
}

static void GLAPIENTRY
exec_GetHistogram(GLenum target, GLboolean reset, GLenum format,
                  GLenum type, GLvoid *values)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   CALL_GetHistogram(ctx->Exec, (target, reset, format, type, values));
}

static void GLAPIENTRY
exec_GetHistogramParameterfv(GLenum target, GLenum pname, GLfloat *params)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   CALL_GetHistogramParameterfv(ctx->Exec, (target, pname, params));
}

static void GLAPIENTRY
exec_GetHistogramParameteriv(GLenum target, GLenum pname, GLint *params)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   CALL_GetHistogramParameteriv(ctx->Exec, (target, pname, params));
}

static void GLAPIENTRY
exec_GetMinmax(GLenum target, GLboolean reset, GLenum format,
               GLenum type, GLvoid *values)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   CALL_GetMinmax(ctx->Exec, (target, reset, format, type, values));
}

static void GLAPIENTRY
exec_GetMinmaxParameterfv(GLenum target, GLenum pname, GLfloat *params)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   CALL_GetMinmaxParameterfv(ctx->Exec, (target, pname, params));
}

static void GLAPIENTRY
exec_GetMinmaxParameteriv(GLenum target, GLenum pname, GLint *params)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   CALL_GetMinmaxParameteriv(ctx->Exec, (target, pname, params));
}

static void GLAPIENTRY
exec_GetSeparableFilter(GLenum target, GLenum format, GLenum type,
                        GLvoid *row, GLvoid *column, GLvoid *span)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   CALL_GetSeparableFilter(ctx->Exec,
                           (target, format, type, row, column, span));
}

static void GLAPIENTRY
exec_SeparableFilter2D(GLenum target, GLenum internalFormat,
                       GLsizei width, GLsizei height, GLenum format,
                       GLenum type, const GLvoid *row, const GLvoid *column)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   CALL_SeparableFilter2D(ctx->Exec,
                          (target, internalFormat, width, height, format,
                           type, row, column));
}

static void GLAPIENTRY
exec_ColorPointerEXT(GLint size, GLenum type, GLsizei stride,
                     GLsizei count, const GLvoid *ptr)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   CALL_ColorPointerEXT(ctx->Exec, (size, type, stride, count, ptr));
}

static void GLAPIENTRY
exec_EdgeFlagPointerEXT(GLsizei stride, GLsizei count, const GLboolean *ptr)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   CALL_EdgeFlagPointerEXT(ctx->Exec, (stride, count, ptr));
}

static void GLAPIENTRY
exec_IndexPointerEXT(GLenum type, GLsizei stride, GLsizei count,
                     const GLvoid *ptr)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   CALL_IndexPointerEXT(ctx->Exec, (type, stride, count, ptr));
}

static void GLAPIENTRY
exec_NormalPointerEXT(GLenum type, GLsizei stride, GLsizei count,
                      const GLvoid *ptr)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   CALL_NormalPointerEXT(ctx->Exec, (type, stride, count, ptr));
}

static void GLAPIENTRY
exec_TexCoordPointerEXT(GLint size, GLenum type, GLsizei stride,
                        GLsizei count, const GLvoid *ptr)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   CALL_TexCoordPointerEXT(ctx->Exec, (size, type, stride, count, ptr));
}

static void GLAPIENTRY
exec_VertexPointerEXT(GLint size, GLenum type, GLsizei stride,
                      GLsizei count, const GLvoid *ptr)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   CALL_VertexPointerEXT(ctx->Exec, (size, type, stride, count, ptr));
}

static void GLAPIENTRY
exec_LockArraysEXT(GLint first, GLsizei count)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   CALL_LockArraysEXT(ctx->Exec, (first, count));
}

static void GLAPIENTRY
exec_UnlockArraysEXT(void)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   CALL_UnlockArraysEXT(ctx->Exec, ());
}

static void GLAPIENTRY
exec_ClientActiveTextureARB(GLenum target)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   CALL_ClientActiveTextureARB(ctx->Exec, (target));
}

static void GLAPIENTRY
exec_SecondaryColorPointerEXT(GLint size, GLenum type,
                              GLsizei stride, const GLvoid *ptr)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   CALL_SecondaryColorPointerEXT(ctx->Exec, (size, type, stride, ptr));
}

static void GLAPIENTRY
exec_FogCoordPointerEXT(GLenum type, GLsizei stride, const GLvoid *ptr)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   CALL_FogCoordPointerEXT(ctx->Exec, (type, stride, ptr));
}

/* GL_EXT_multi_draw_arrays */
static void GLAPIENTRY
exec_MultiDrawArraysEXT(GLenum mode, const GLint *first,
                        const GLsizei *count, GLsizei primcount)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   CALL_MultiDrawArraysEXT(ctx->Exec, (mode, first, count, primcount));
}

/* GL_IBM_multimode_draw_arrays */
static void GLAPIENTRY
exec_MultiModeDrawArraysIBM(const GLenum * mode, const GLint * first,
                            const GLsizei * count, GLsizei primcount,
                            GLint modestride)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   CALL_MultiModeDrawArraysIBM(ctx->Exec,
                               (mode, first, count, primcount, modestride));
}

/* GL_IBM_multimode_draw_arrays */
static void GLAPIENTRY
exec_MultiModeDrawElementsIBM(const GLenum * mode,
                              const GLsizei * count,
                              GLenum type,
                              const GLvoid * const *indices,
                              GLsizei primcount, GLint modestride)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   CALL_MultiModeDrawElementsIBM(ctx->Exec,
                                 (mode, count, type, indices, primcount,
                                  modestride));
}

/**
 * Setup the given dispatch table to point to Mesa's display list
 * building functions.
 *
 * This does not include any of the tnl functions - they are
 * initialized from _mesa_init_api_defaults and from the active vtxfmt
 * struct.
 */
struct _glapi_table *
_mesa_create_save_table(void)
{
   struct _glapi_table *table;

   table = _mesa_alloc_dispatch_table(_gloffset_COUNT);
   if (table == NULL)
      return NULL;

   _mesa_loopback_init_api_table(table);

   /* GL 1.0 */
   SET_Accum(table, save_Accum);
   SET_AlphaFunc(table, save_AlphaFunc);
   SET_Bitmap(table, save_Bitmap);
   SET_BlendFunc(table, save_BlendFunc);
   SET_CallList(table, save_CallList);
   SET_CallLists(table, save_CallLists);
   SET_Clear(table, save_Clear);
   SET_ClearAccum(table, save_ClearAccum);
   SET_ClearColor(table, save_ClearColor);
   SET_ClearDepth(table, save_ClearDepth);
   SET_ClearIndex(table, save_ClearIndex);
   SET_ClearStencil(table, save_ClearStencil);
   SET_ClipPlane(table, save_ClipPlane);
   SET_ColorMask(table, save_ColorMask);
   SET_ColorMaskIndexedEXT(table, save_ColorMaskIndexed);
   SET_ColorMaterial(table, save_ColorMaterial);
   SET_CopyPixels(table, save_CopyPixels);
   SET_CullFace(table, save_CullFace);
   SET_DeleteLists(table, _mesa_DeleteLists);
   SET_DepthFunc(table, save_DepthFunc);
   SET_DepthMask(table, save_DepthMask);
   SET_DepthRange(table, save_DepthRange);
   SET_Disable(table, save_Disable);
   SET_DisableIndexedEXT(table, save_DisableIndexed);
   SET_DrawBuffer(table, save_DrawBuffer);
   SET_DrawPixels(table, save_DrawPixels);
   SET_Enable(table, save_Enable);
   SET_EnableIndexedEXT(table, save_EnableIndexed);
   SET_EndList(table, _mesa_EndList);
   SET_EvalMesh1(table, save_EvalMesh1);
   SET_EvalMesh2(table, save_EvalMesh2);
   SET_Finish(table, exec_Finish);
   SET_Flush(table, exec_Flush);
   SET_Fogf(table, save_Fogf);
   SET_Fogfv(table, save_Fogfv);
   SET_Fogi(table, save_Fogi);
   SET_Fogiv(table, save_Fogiv);
   SET_FrontFace(table, save_FrontFace);
   SET_Frustum(table, save_Frustum);
   SET_GenLists(table, _mesa_GenLists);
   SET_GetBooleanv(table, exec_GetBooleanv);
   SET_GetClipPlane(table, exec_GetClipPlane);
   SET_GetDoublev(table, exec_GetDoublev);
   SET_GetError(table, exec_GetError);
   SET_GetFloatv(table, exec_GetFloatv);
   SET_GetIntegerv(table, exec_GetIntegerv);
   SET_GetLightfv(table, exec_GetLightfv);
   SET_GetLightiv(table, exec_GetLightiv);
   SET_GetMapdv(table, exec_GetMapdv);
   SET_GetMapfv(table, exec_GetMapfv);
   SET_GetMapiv(table, exec_GetMapiv);
   SET_GetMaterialfv(table, exec_GetMaterialfv);
   SET_GetMaterialiv(table, exec_GetMaterialiv);
   SET_GetPixelMapfv(table, exec_GetPixelMapfv);
   SET_GetPixelMapuiv(table, exec_GetPixelMapuiv);
   SET_GetPixelMapusv(table, exec_GetPixelMapusv);
   SET_GetPolygonStipple(table, exec_GetPolygonStipple);
   SET_GetString(table, exec_GetString);
   SET_GetTexEnvfv(table, exec_GetTexEnvfv);
   SET_GetTexEnviv(table, exec_GetTexEnviv);
   SET_GetTexGendv(table, exec_GetTexGendv);
   SET_GetTexGenfv(table, exec_GetTexGenfv);
   SET_GetTexGeniv(table, exec_GetTexGeniv);
   SET_GetTexImage(table, exec_GetTexImage);
   SET_GetTexLevelParameterfv(table, exec_GetTexLevelParameterfv);
   SET_GetTexLevelParameteriv(table, exec_GetTexLevelParameteriv);
   SET_GetTexParameterfv(table, exec_GetTexParameterfv);
   SET_GetTexParameteriv(table, exec_GetTexParameteriv);
   SET_Hint(table, save_Hint);
   SET_IndexMask(table, save_IndexMask);
   SET_InitNames(table, save_InitNames);
   SET_IsEnabled(table, exec_IsEnabled);
   SET_IsList(table, _mesa_IsList);
   SET_LightModelf(table, save_LightModelf);
   SET_LightModelfv(table, save_LightModelfv);
   SET_LightModeli(table, save_LightModeli);
   SET_LightModeliv(table, save_LightModeliv);
   SET_Lightf(table, save_Lightf);
   SET_Lightfv(table, save_Lightfv);
   SET_Lighti(table, save_Lighti);
   SET_Lightiv(table, save_Lightiv);
   SET_LineStipple(table, save_LineStipple);
   SET_LineWidth(table, save_LineWidth);
   SET_ListBase(table, save_ListBase);
   SET_LoadIdentity(table, save_LoadIdentity);
   SET_LoadMatrixd(table, save_LoadMatrixd);
   SET_LoadMatrixf(table, save_LoadMatrixf);
   SET_LoadName(table, save_LoadName);
   SET_LogicOp(table, save_LogicOp);
   SET_Map1d(table, save_Map1d);
   SET_Map1f(table, save_Map1f);
   SET_Map2d(table, save_Map2d);
   SET_Map2f(table, save_Map2f);
   SET_MapGrid1d(table, save_MapGrid1d);
   SET_MapGrid1f(table, save_MapGrid1f);
   SET_MapGrid2d(table, save_MapGrid2d);
   SET_MapGrid2f(table, save_MapGrid2f);
   SET_MatrixMode(table, save_MatrixMode);
   SET_MultMatrixd(table, save_MultMatrixd);
   SET_MultMatrixf(table, save_MultMatrixf);
   SET_NewList(table, save_NewList);
   SET_Ortho(table, save_Ortho);
   SET_PassThrough(table, save_PassThrough);
   SET_PixelMapfv(table, save_PixelMapfv);
   SET_PixelMapuiv(table, save_PixelMapuiv);
   SET_PixelMapusv(table, save_PixelMapusv);
   SET_PixelStoref(table, exec_PixelStoref);
   SET_PixelStorei(table, exec_PixelStorei);
   SET_PixelTransferf(table, save_PixelTransferf);
   SET_PixelTransferi(table, save_PixelTransferi);
   SET_PixelZoom(table, save_PixelZoom);
   SET_PointSize(table, save_PointSize);
   SET_PolygonMode(table, save_PolygonMode);
   SET_PolygonOffset(table, save_PolygonOffset);
   SET_PolygonStipple(table, save_PolygonStipple);
   SET_PopAttrib(table, save_PopAttrib);
   SET_PopMatrix(table, save_PopMatrix);
   SET_PopName(table, save_PopName);
   SET_PushAttrib(table, save_PushAttrib);
   SET_PushMatrix(table, save_PushMatrix);
   SET_PushName(table, save_PushName);
   SET_RasterPos2d(table, save_RasterPos2d);
   SET_RasterPos2dv(table, save_RasterPos2dv);
   SET_RasterPos2f(table, save_RasterPos2f);
   SET_RasterPos2fv(table, save_RasterPos2fv);
   SET_RasterPos2i(table, save_RasterPos2i);
   SET_RasterPos2iv(table, save_RasterPos2iv);
   SET_RasterPos2s(table, save_RasterPos2s);
   SET_RasterPos2sv(table, save_RasterPos2sv);
   SET_RasterPos3d(table, save_RasterPos3d);
   SET_RasterPos3dv(table, save_RasterPos3dv);
   SET_RasterPos3f(table, save_RasterPos3f);
   SET_RasterPos3fv(table, save_RasterPos3fv);
   SET_RasterPos3i(table, save_RasterPos3i);
   SET_RasterPos3iv(table, save_RasterPos3iv);
   SET_RasterPos3s(table, save_RasterPos3s);
   SET_RasterPos3sv(table, save_RasterPos3sv);
   SET_RasterPos4d(table, save_RasterPos4d);
   SET_RasterPos4dv(table, save_RasterPos4dv);
   SET_RasterPos4f(table, save_RasterPos4f);
   SET_RasterPos4fv(table, save_RasterPos4fv);
   SET_RasterPos4i(table, save_RasterPos4i);
   SET_RasterPos4iv(table, save_RasterPos4iv);
   SET_RasterPos4s(table, save_RasterPos4s);
   SET_RasterPos4sv(table, save_RasterPos4sv);
   SET_ReadBuffer(table, save_ReadBuffer);
   SET_ReadPixels(table, exec_ReadPixels);
   SET_RenderMode(table, exec_RenderMode);
   SET_Rotated(table, save_Rotated);
   SET_Rotatef(table, save_Rotatef);
   SET_Scaled(table, save_Scaled);
   SET_Scalef(table, save_Scalef);
   SET_Scissor(table, save_Scissor);
   SET_FeedbackBuffer(table, exec_FeedbackBuffer);
   SET_SelectBuffer(table, exec_SelectBuffer);
   SET_ShadeModel(table, save_ShadeModel);
   SET_StencilFunc(table, save_StencilFunc);
   SET_StencilMask(table, save_StencilMask);
   SET_StencilOp(table, save_StencilOp);
   SET_TexEnvf(table, save_TexEnvf);
   SET_TexEnvfv(table, save_TexEnvfv);
   SET_TexEnvi(table, save_TexEnvi);
   SET_TexEnviv(table, save_TexEnviv);
   SET_TexGend(table, save_TexGend);
   SET_TexGendv(table, save_TexGendv);
   SET_TexGenf(table, save_TexGenf);
   SET_TexGenfv(table, save_TexGenfv);
   SET_TexGeni(table, save_TexGeni);
   SET_TexGeniv(table, save_TexGeniv);
   SET_TexImage1D(table, save_TexImage1D);
   SET_TexImage2D(table, save_TexImage2D);
   SET_TexParameterf(table, save_TexParameterf);
   SET_TexParameterfv(table, save_TexParameterfv);
   SET_TexParameteri(table, save_TexParameteri);
   SET_TexParameteriv(table, save_TexParameteriv);
   SET_Translated(table, save_Translated);
   SET_Translatef(table, save_Translatef);
   SET_Viewport(table, save_Viewport);

   /* GL 1.1 */
   SET_AreTexturesResident(table, exec_AreTexturesResident);
   SET_BindTexture(table, save_BindTexture);
   SET_ColorPointer(table, exec_ColorPointer);
   SET_CopyTexImage1D(table, save_CopyTexImage1D);
   SET_CopyTexImage2D(table, save_CopyTexImage2D);
   SET_CopyTexSubImage1D(table, save_CopyTexSubImage1D);
   SET_CopyTexSubImage2D(table, save_CopyTexSubImage2D);
   SET_DeleteTextures(table, exec_DeleteTextures);
   SET_DisableClientState(table, exec_DisableClientState);
   SET_EdgeFlagPointer(table, exec_EdgeFlagPointer);
   SET_EnableClientState(table, exec_EnableClientState);
   SET_GenTextures(table, exec_GenTextures);
   SET_GetPointerv(table, exec_GetPointerv);
   SET_IndexPointer(table, exec_IndexPointer);
   SET_InterleavedArrays(table, exec_InterleavedArrays);
   SET_IsTexture(table, exec_IsTexture);
   SET_NormalPointer(table, exec_NormalPointer);
   SET_PopClientAttrib(table, exec_PopClientAttrib);
   SET_PrioritizeTextures(table, save_PrioritizeTextures);
   SET_PushClientAttrib(table, exec_PushClientAttrib);
   SET_TexCoordPointer(table, exec_TexCoordPointer);
   SET_TexSubImage1D(table, save_TexSubImage1D);
   SET_TexSubImage2D(table, save_TexSubImage2D);
   SET_VertexPointer(table, exec_VertexPointer);

   /* GL 1.2 */
   SET_CopyTexSubImage3D(table, save_CopyTexSubImage3D);
   SET_TexImage3D(table, save_TexImage3D);
   SET_TexSubImage3D(table, save_TexSubImage3D);

   /* GL 2.0 */
   SET_StencilFuncSeparate(table, save_StencilFuncSeparate);
   SET_StencilMaskSeparate(table, save_StencilMaskSeparate);
   SET_StencilOpSeparate(table, save_StencilOpSeparate);

   /* ATI_separate_stencil */ 
   SET_StencilFuncSeparateATI(table, save_StencilFuncSeparateATI);

   /* GL_ARB_imaging */
   /* Not all are supported */
   SET_BlendColor(table, save_BlendColor);
   SET_BlendEquation(table, save_BlendEquation);
   SET_ColorSubTable(table, save_ColorSubTable);
   SET_ColorTable(table, save_ColorTable);
   SET_ColorTableParameterfv(table, save_ColorTableParameterfv);
   SET_ColorTableParameteriv(table, save_ColorTableParameteriv);
   SET_ConvolutionFilter1D(table, save_ConvolutionFilter1D);
   SET_ConvolutionFilter2D(table, save_ConvolutionFilter2D);
   SET_ConvolutionParameterf(table, save_ConvolutionParameterf);
   SET_ConvolutionParameterfv(table, save_ConvolutionParameterfv);
   SET_ConvolutionParameteri(table, save_ConvolutionParameteri);
   SET_ConvolutionParameteriv(table, save_ConvolutionParameteriv);
   SET_CopyColorSubTable(table, save_CopyColorSubTable);
   SET_CopyColorTable(table, save_CopyColorTable);
   SET_CopyConvolutionFilter1D(table, exec_CopyConvolutionFilter1D);
   SET_CopyConvolutionFilter2D(table, exec_CopyConvolutionFilter2D);
   SET_GetColorTable(table, exec_GetColorTable);
   SET_GetColorTableParameterfv(table, exec_GetColorTableParameterfv);
   SET_GetColorTableParameteriv(table, exec_GetColorTableParameteriv);
   SET_GetConvolutionFilter(table, exec_GetConvolutionFilter);
   SET_GetConvolutionParameterfv(table, exec_GetConvolutionParameterfv);
   SET_GetConvolutionParameteriv(table, exec_GetConvolutionParameteriv);
   SET_GetHistogram(table, exec_GetHistogram);
   SET_GetHistogramParameterfv(table, exec_GetHistogramParameterfv);
   SET_GetHistogramParameteriv(table, exec_GetHistogramParameteriv);
   SET_GetMinmax(table, exec_GetMinmax);
   SET_GetMinmaxParameterfv(table, exec_GetMinmaxParameterfv);
   SET_GetMinmaxParameteriv(table, exec_GetMinmaxParameteriv);
   SET_GetSeparableFilter(table, exec_GetSeparableFilter);
   SET_Histogram(table, save_Histogram);
   SET_Minmax(table, save_Minmax);
   SET_ResetHistogram(table, save_ResetHistogram);
   SET_ResetMinmax(table, save_ResetMinmax);
   SET_SeparableFilter2D(table, exec_SeparableFilter2D);

   /* 2. GL_EXT_blend_color */
#if 0
   SET_BlendColorEXT(table, save_BlendColorEXT);
#endif

   /* 3. GL_EXT_polygon_offset */
   SET_PolygonOffsetEXT(table, save_PolygonOffsetEXT);

   /* 6. GL_EXT_texture3d */
#if 0
   SET_CopyTexSubImage3DEXT(table, save_CopyTexSubImage3D);
   SET_TexImage3DEXT(table, save_TexImage3DEXT);
   SET_TexSubImage3DEXT(table, save_TexSubImage3D);
#endif

   /* 14. GL_SGI_color_table */
#if 0
   SET_ColorTableSGI(table, save_ColorTable);
   SET_ColorSubTableSGI(table, save_ColorSubTable);
   SET_GetColorTableSGI(table, exec_GetColorTable);
   SET_GetColorTableParameterfvSGI(table, exec_GetColorTableParameterfv);
   SET_GetColorTableParameterivSGI(table, exec_GetColorTableParameteriv);
#endif

   /* 30. GL_EXT_vertex_array */
   SET_ColorPointerEXT(table, exec_ColorPointerEXT);
   SET_EdgeFlagPointerEXT(table, exec_EdgeFlagPointerEXT);
   SET_IndexPointerEXT(table, exec_IndexPointerEXT);
   SET_NormalPointerEXT(table, exec_NormalPointerEXT);
   SET_TexCoordPointerEXT(table, exec_TexCoordPointerEXT);
   SET_VertexPointerEXT(table, exec_VertexPointerEXT);

   /* 37. GL_EXT_blend_minmax */
#if 0
   SET_BlendEquationEXT(table, save_BlendEquationEXT);
#endif

   /* 54. GL_EXT_point_parameters */
   SET_PointParameterfEXT(table, save_PointParameterfEXT);
   SET_PointParameterfvEXT(table, save_PointParameterfvEXT);

   /* 97. GL_EXT_compiled_vertex_array */
   SET_LockArraysEXT(table, exec_LockArraysEXT);
   SET_UnlockArraysEXT(table, exec_UnlockArraysEXT);

   /* 145. GL_EXT_secondary_color */
   SET_SecondaryColorPointerEXT(table, exec_SecondaryColorPointerEXT);

   /* 148. GL_EXT_multi_draw_arrays */
   SET_MultiDrawArraysEXT(table, exec_MultiDrawArraysEXT);

   /* 149. GL_EXT_fog_coord */
   SET_FogCoordPointerEXT(table, exec_FogCoordPointerEXT);

   /* 173. GL_EXT_blend_func_separate */
   SET_BlendFuncSeparateEXT(table, save_BlendFuncSeparateEXT);

   /* 196. GL_MESA_resize_buffers */
   SET_ResizeBuffersMESA(table, _mesa_ResizeBuffersMESA);

   /* 197. GL_MESA_window_pos */
   SET_WindowPos2dMESA(table, save_WindowPos2dMESA);
   SET_WindowPos2dvMESA(table, save_WindowPos2dvMESA);
   SET_WindowPos2fMESA(table, save_WindowPos2fMESA);
   SET_WindowPos2fvMESA(table, save_WindowPos2fvMESA);
   SET_WindowPos2iMESA(table, save_WindowPos2iMESA);
   SET_WindowPos2ivMESA(table, save_WindowPos2ivMESA);
   SET_WindowPos2sMESA(table, save_WindowPos2sMESA);
   SET_WindowPos2svMESA(table, save_WindowPos2svMESA);
   SET_WindowPos3dMESA(table, save_WindowPos3dMESA);
   SET_WindowPos3dvMESA(table, save_WindowPos3dvMESA);
   SET_WindowPos3fMESA(table, save_WindowPos3fMESA);
   SET_WindowPos3fvMESA(table, save_WindowPos3fvMESA);
   SET_WindowPos3iMESA(table, save_WindowPos3iMESA);
   SET_WindowPos3ivMESA(table, save_WindowPos3ivMESA);
   SET_WindowPos3sMESA(table, save_WindowPos3sMESA);
   SET_WindowPos3svMESA(table, save_WindowPos3svMESA);
   SET_WindowPos4dMESA(table, save_WindowPos4dMESA);
   SET_WindowPos4dvMESA(table, save_WindowPos4dvMESA);
   SET_WindowPos4fMESA(table, save_WindowPos4fMESA);
   SET_WindowPos4fvMESA(table, save_WindowPos4fvMESA);
   SET_WindowPos4iMESA(table, save_WindowPos4iMESA);
   SET_WindowPos4ivMESA(table, save_WindowPos4ivMESA);
   SET_WindowPos4sMESA(table, save_WindowPos4sMESA);
   SET_WindowPos4svMESA(table, save_WindowPos4svMESA);

   /* 200. GL_IBM_multimode_draw_arrays */
   SET_MultiModeDrawArraysIBM(table, exec_MultiModeDrawArraysIBM);
   SET_MultiModeDrawElementsIBM(table, exec_MultiModeDrawElementsIBM);

#if FEATURE_NV_vertex_program
   /* 233. GL_NV_vertex_program */
   /* The following commands DO NOT go into display lists:
    * AreProgramsResidentNV, IsProgramNV, GenProgramsNV, DeleteProgramsNV,
    * VertexAttribPointerNV, GetProgram*, GetVertexAttrib*
    */
   SET_BindProgramNV(table, save_BindProgramNV);
   SET_DeleteProgramsNV(table, _mesa_DeletePrograms);
   SET_ExecuteProgramNV(table, save_ExecuteProgramNV);
   SET_GenProgramsNV(table, _mesa_GenPrograms);
   SET_AreProgramsResidentNV(table, _mesa_AreProgramsResidentNV);
   SET_RequestResidentProgramsNV(table, save_RequestResidentProgramsNV);
   SET_GetProgramParameterfvNV(table, _mesa_GetProgramParameterfvNV);
   SET_GetProgramParameterdvNV(table, _mesa_GetProgramParameterdvNV);
   SET_GetProgramivNV(table, _mesa_GetProgramivNV);
   SET_GetProgramStringNV(table, _mesa_GetProgramStringNV);
   SET_GetTrackMatrixivNV(table, _mesa_GetTrackMatrixivNV);
   SET_GetVertexAttribdvNV(table, _mesa_GetVertexAttribdvNV);
   SET_GetVertexAttribfvNV(table, _mesa_GetVertexAttribfvNV);
   SET_GetVertexAttribivNV(table, _mesa_GetVertexAttribivNV);
   SET_GetVertexAttribPointervNV(table, _mesa_GetVertexAttribPointervNV);
   SET_IsProgramNV(table, _mesa_IsProgramARB);
   SET_LoadProgramNV(table, save_LoadProgramNV);
   SET_ProgramEnvParameter4dARB(table, save_ProgramEnvParameter4dARB);
   SET_ProgramEnvParameter4dvARB(table, save_ProgramEnvParameter4dvARB);
   SET_ProgramEnvParameter4fARB(table, save_ProgramEnvParameter4fARB);
   SET_ProgramEnvParameter4fvARB(table, save_ProgramEnvParameter4fvARB);
   SET_ProgramParameters4dvNV(table, save_ProgramParameters4dvNV);
   SET_ProgramParameters4fvNV(table, save_ProgramParameters4fvNV);
   SET_TrackMatrixNV(table, save_TrackMatrixNV);
   SET_VertexAttribPointerNV(table, _mesa_VertexAttribPointerNV);
#endif

   /* 244. GL_ATI_envmap_bumpmap */
   SET_TexBumpParameterivATI(table, save_TexBumpParameterivATI);
   SET_TexBumpParameterfvATI(table, save_TexBumpParameterfvATI);

   /* 245. GL_ATI_fragment_shader */
#if FEATURE_ATI_fragment_shader
   SET_BindFragmentShaderATI(table, save_BindFragmentShaderATI);
   SET_SetFragmentShaderConstantATI(table, save_SetFragmentShaderConstantATI);
#endif

   /* 282. GL_NV_fragment_program */
#if FEATURE_NV_fragment_program
   SET_ProgramNamedParameter4fNV(table, save_ProgramNamedParameter4fNV);
   SET_ProgramNamedParameter4dNV(table, save_ProgramNamedParameter4dNV);
   SET_ProgramNamedParameter4fvNV(table, save_ProgramNamedParameter4fvNV);
   SET_ProgramNamedParameter4dvNV(table, save_ProgramNamedParameter4dvNV);
   SET_GetProgramNamedParameterfvNV(table,
                                    _mesa_GetProgramNamedParameterfvNV);
   SET_GetProgramNamedParameterdvNV(table,
                                    _mesa_GetProgramNamedParameterdvNV);
   SET_ProgramLocalParameter4dARB(table, save_ProgramLocalParameter4dARB);
   SET_ProgramLocalParameter4dvARB(table, save_ProgramLocalParameter4dvARB);
   SET_ProgramLocalParameter4fARB(table, save_ProgramLocalParameter4fARB);
   SET_ProgramLocalParameter4fvARB(table, save_ProgramLocalParameter4fvARB);
   SET_GetProgramLocalParameterdvARB(table,
                                     _mesa_GetProgramLocalParameterdvARB);
   SET_GetProgramLocalParameterfvARB(table,
                                     _mesa_GetProgramLocalParameterfvARB);
#endif

   /* 262. GL_NV_point_sprite */
   SET_PointParameteriNV(table, save_PointParameteriNV);
   SET_PointParameterivNV(table, save_PointParameterivNV);

   /* 268. GL_EXT_stencil_two_side */
   SET_ActiveStencilFaceEXT(table, save_ActiveStencilFaceEXT);

   /* 273. GL_APPLE_vertex_array_object */
   SET_BindVertexArrayAPPLE(table, _mesa_BindVertexArrayAPPLE);
   SET_DeleteVertexArraysAPPLE(table, _mesa_DeleteVertexArraysAPPLE);
   SET_GenVertexArraysAPPLE(table, _mesa_GenVertexArraysAPPLE);
   SET_IsVertexArrayAPPLE(table, _mesa_IsVertexArrayAPPLE);

   /* 310. GL_EXT_framebuffer_object */
   SET_GenFramebuffersEXT(table, _mesa_GenFramebuffersEXT);
   SET_BindFramebufferEXT(table, _mesa_BindFramebufferEXT);
   SET_DeleteFramebuffersEXT(table, _mesa_DeleteFramebuffersEXT);
   SET_CheckFramebufferStatusEXT(table, _mesa_CheckFramebufferStatusEXT);
   SET_GenRenderbuffersEXT(table, _mesa_GenRenderbuffersEXT);
   SET_BindRenderbufferEXT(table, _mesa_BindRenderbufferEXT);
   SET_DeleteRenderbuffersEXT(table, _mesa_DeleteRenderbuffersEXT);
   SET_RenderbufferStorageEXT(table, _mesa_RenderbufferStorageEXT);
   SET_FramebufferTexture1DEXT(table, _mesa_FramebufferTexture1DEXT);
   SET_FramebufferTexture2DEXT(table, _mesa_FramebufferTexture2DEXT);
   SET_FramebufferTexture3DEXT(table, _mesa_FramebufferTexture3DEXT);
   SET_FramebufferRenderbufferEXT(table, _mesa_FramebufferRenderbufferEXT);
   SET_GenerateMipmapEXT(table, _mesa_GenerateMipmapEXT);

   /* 317. GL_EXT_framebuffer_multisample */
   SET_RenderbufferStorageMultisample(table, _mesa_RenderbufferStorageMultisample);

   /* GL_ARB_vertex_array_object */
   SET_BindVertexArray(table, _mesa_BindVertexArray);
   SET_GenVertexArrays(table, _mesa_GenVertexArrays);

   /* ???. GL_EXT_depth_bounds_test */
   SET_DepthBoundsEXT(table, save_DepthBoundsEXT);

   /* ARB 1. GL_ARB_multitexture */
   SET_ActiveTextureARB(table, save_ActiveTextureARB);
   SET_ClientActiveTextureARB(table, exec_ClientActiveTextureARB);

   /* ARB 3. GL_ARB_transpose_matrix */
   SET_LoadTransposeMatrixdARB(table, save_LoadTransposeMatrixdARB);
   SET_LoadTransposeMatrixfARB(table, save_LoadTransposeMatrixfARB);
   SET_MultTransposeMatrixdARB(table, save_MultTransposeMatrixdARB);
   SET_MultTransposeMatrixfARB(table, save_MultTransposeMatrixfARB);

   /* ARB 5. GL_ARB_multisample */
   SET_SampleCoverageARB(table, save_SampleCoverageARB);

   /* ARB 12. GL_ARB_texture_compression */
   SET_CompressedTexImage3DARB(table, save_CompressedTexImage3DARB);
   SET_CompressedTexImage2DARB(table, save_CompressedTexImage2DARB);
   SET_CompressedTexImage1DARB(table, save_CompressedTexImage1DARB);
   SET_CompressedTexSubImage3DARB(table, save_CompressedTexSubImage3DARB);
   SET_CompressedTexSubImage2DARB(table, save_CompressedTexSubImage2DARB);
   SET_CompressedTexSubImage1DARB(table, save_CompressedTexSubImage1DARB);
   SET_GetCompressedTexImageARB(table, exec_GetCompressedTexImageARB);

   /* ARB 14. GL_ARB_point_parameters */
   /* aliased with EXT_point_parameters functions */

   /* ARB 25. GL_ARB_window_pos */
   /* aliased with MESA_window_pos functions */

   /* ARB 26. GL_ARB_vertex_program */
   /* ARB 27. GL_ARB_fragment_program */
#if FEATURE_ARB_vertex_program || FEATURE_ARB_fragment_program
   /* glVertexAttrib* functions alias the NV ones, handled elsewhere */
   SET_VertexAttribPointerARB(table, _mesa_VertexAttribPointerARB);
   SET_EnableVertexAttribArrayARB(table, _mesa_EnableVertexAttribArrayARB);
   SET_DisableVertexAttribArrayARB(table, _mesa_DisableVertexAttribArrayARB);
   SET_ProgramStringARB(table, save_ProgramStringARB);
   SET_BindProgramNV(table, save_BindProgramNV);
   SET_DeleteProgramsNV(table, _mesa_DeletePrograms);
   SET_GenProgramsNV(table, _mesa_GenPrograms);
   SET_IsProgramNV(table, _mesa_IsProgramARB);
   SET_GetVertexAttribdvARB(table, _mesa_GetVertexAttribdvARB);
   SET_GetVertexAttribfvARB(table, _mesa_GetVertexAttribfvARB);
   SET_GetVertexAttribivARB(table, _mesa_GetVertexAttribivARB);
   SET_GetVertexAttribPointervNV(table, _mesa_GetVertexAttribPointervNV);
   SET_ProgramEnvParameter4dARB(table, save_ProgramEnvParameter4dARB);
   SET_ProgramEnvParameter4dvARB(table, save_ProgramEnvParameter4dvARB);
   SET_ProgramEnvParameter4fARB(table, save_ProgramEnvParameter4fARB);
   SET_ProgramEnvParameter4fvARB(table, save_ProgramEnvParameter4fvARB);
   SET_ProgramLocalParameter4dARB(table, save_ProgramLocalParameter4dARB);
   SET_ProgramLocalParameter4dvARB(table, save_ProgramLocalParameter4dvARB);
   SET_ProgramLocalParameter4fARB(table, save_ProgramLocalParameter4fARB);
   SET_ProgramLocalParameter4fvARB(table, save_ProgramLocalParameter4fvARB);
   SET_GetProgramEnvParameterdvARB(table, _mesa_GetProgramEnvParameterdvARB);
   SET_GetProgramEnvParameterfvARB(table, _mesa_GetProgramEnvParameterfvARB);
   SET_GetProgramLocalParameterdvARB(table,
                                     _mesa_GetProgramLocalParameterdvARB);
   SET_GetProgramLocalParameterfvARB(table,
                                     _mesa_GetProgramLocalParameterfvARB);
   SET_GetProgramivARB(table, _mesa_GetProgramivARB);
   SET_GetProgramStringARB(table, _mesa_GetProgramStringARB);
#endif

   /* ARB 28. GL_ARB_vertex_buffer_object */
   /* None of the extension's functions get compiled */
   SET_BindBufferARB(table, _mesa_BindBufferARB);
   SET_BufferDataARB(table, _mesa_BufferDataARB);
   SET_BufferSubDataARB(table, _mesa_BufferSubDataARB);
   SET_DeleteBuffersARB(table, _mesa_DeleteBuffersARB);
   SET_GenBuffersARB(table, _mesa_GenBuffersARB);
   SET_GetBufferParameterivARB(table, _mesa_GetBufferParameterivARB);
   SET_GetBufferPointervARB(table, _mesa_GetBufferPointervARB);
   SET_GetBufferSubDataARB(table, _mesa_GetBufferSubDataARB);
   SET_IsBufferARB(table, _mesa_IsBufferARB);
   SET_MapBufferARB(table, _mesa_MapBufferARB);
   SET_UnmapBufferARB(table, _mesa_UnmapBufferARB);

#if FEATURE_queryobj
   _mesa_init_queryobj_dispatch(table); /* glGetQuery, etc */
   SET_BeginQueryARB(table, save_BeginQueryARB);
   SET_EndQueryARB(table, save_EndQueryARB);
#endif

   SET_DrawBuffersARB(table, save_DrawBuffersARB);

#if FEATURE_EXT_framebuffer_blit
   SET_BlitFramebufferEXT(table, save_BlitFramebufferEXT);
#endif

   /* GL_ARB_shader_objects */
   _mesa_init_shader_dispatch(table); /* Plug in glCreate/Delete/Get, etc */
   SET_UseProgramObjectARB(table, save_UseProgramObjectARB);
   SET_Uniform1fARB(table, save_Uniform1fARB);
   SET_Uniform2fARB(table, save_Uniform2fARB);
   SET_Uniform3fARB(table, save_Uniform3fARB);
   SET_Uniform4fARB(table, save_Uniform4fARB);
   SET_Uniform1fvARB(table, save_Uniform1fvARB);
   SET_Uniform2fvARB(table, save_Uniform2fvARB);
   SET_Uniform3fvARB(table, save_Uniform3fvARB);
   SET_Uniform4fvARB(table, save_Uniform4fvARB);
   SET_Uniform1iARB(table, save_Uniform1iARB);
   SET_Uniform2iARB(table, save_Uniform2iARB);
   SET_Uniform3iARB(table, save_Uniform3iARB);
   SET_Uniform4iARB(table, save_Uniform4iARB);
   SET_Uniform1ivARB(table, save_Uniform1ivARB);
   SET_Uniform2ivARB(table, save_Uniform2ivARB);
   SET_Uniform3ivARB(table, save_Uniform3ivARB);
   SET_Uniform4ivARB(table, save_Uniform4ivARB);
   SET_UniformMatrix2fvARB(table, save_UniformMatrix2fvARB);
   SET_UniformMatrix3fvARB(table, save_UniformMatrix3fvARB);
   SET_UniformMatrix4fvARB(table, save_UniformMatrix4fvARB);
   SET_UniformMatrix2x3fv(table, save_UniformMatrix2x3fv);
   SET_UniformMatrix3x2fv(table, save_UniformMatrix3x2fv);
   SET_UniformMatrix2x4fv(table, save_UniformMatrix2x4fv);
   SET_UniformMatrix4x2fv(table, save_UniformMatrix4x2fv);
   SET_UniformMatrix3x4fv(table, save_UniformMatrix3x4fv);
   SET_UniformMatrix4x3fv(table, save_UniformMatrix4x3fv);

   /* ARB 30/31/32. GL_ARB_shader_objects, GL_ARB_vertex/fragment_shader */
   SET_BindAttribLocationARB(table, exec_BindAttribLocationARB);
   SET_GetAttribLocationARB(table, exec_GetAttribLocationARB);
   SET_GetUniformLocationARB(table, exec_GetUniformLocationARB);
   /* XXX additional functions need to be implemented here! */

   /* 299. GL_EXT_blend_equation_separate */
   SET_BlendEquationSeparateEXT(table, save_BlendEquationSeparateEXT);

   /* GL_EXT_gpu_program_parameters */
#if FEATURE_ARB_vertex_program || FEATURE_ARB_fragment_program
   SET_ProgramEnvParameters4fvEXT(table, save_ProgramEnvParameters4fvEXT);
   SET_ProgramLocalParameters4fvEXT(table, save_ProgramLocalParameters4fvEXT);
#endif

   /* ARB 50. GL_ARB_map_buffer_range */
#if FEATURE_ARB_map_buffer_range
   SET_MapBufferRange(table, _mesa_MapBufferRange); /* no dlist save */
   SET_FlushMappedBufferRange(table, _mesa_FlushMappedBufferRange); /* no dl */
#endif

   /* ARB 59. GL_ARB_copy_buffer */
   SET_CopyBufferSubData(table, _mesa_CopyBufferSubData); /* no dlist save */

   /* 364. GL_EXT_provoking_vertex */
   SET_ProvokingVertexEXT(table, save_ProvokingVertexEXT);

   /* 371. GL_APPLE_object_purgeable */
#if FEATURE_APPLE_object_purgeable
   SET_ObjectPurgeableAPPLE(table, _mesa_ObjectPurgeableAPPLE);
   SET_ObjectUnpurgeableAPPLE(table, _mesa_ObjectUnpurgeableAPPLE);
   SET_GetObjectParameterivAPPLE(table, _mesa_GetObjectParameterivAPPLE);
#endif

   /* GL_EXT_texture_integer */
   SET_ClearColorIiEXT(table, save_ClearColorIi);
   SET_ClearColorIuiEXT(table, save_ClearColorIui);
   SET_TexParameterIivEXT(table, save_TexParameterIiv);
   SET_TexParameterIuivEXT(table, save_TexParameterIuiv);
   SET_GetTexParameterIivEXT(table, exec_GetTexParameterIiv);
   SET_GetTexParameterIuivEXT(table, exec_GetTexParameterIuiv);

   /* 377. GL_EXT_separate_shader_objects */
   SET_UseShaderProgramEXT(table, save_UseShaderProgramEXT);
   SET_ActiveProgramEXT(table, save_ActiveProgramEXT);

   /* GL_ARB_color_buffer_float */
   SET_ClampColorARB(table, save_ClampColorARB);
   SET_ClampColor(table, save_ClampColorARB);

   /* GL 3.0 */
   SET_ClearBufferiv(table, save_ClearBufferiv);
   SET_ClearBufferuiv(table, save_ClearBufferuiv);
   SET_ClearBufferfv(table, save_ClearBufferfv);
   SET_ClearBufferfi(table, save_ClearBufferfi);
#if 0
   SET_Uniform1ui(table, save_Uniform1ui);
   SET_Uniform2ui(table, save_Uniform2ui);
   SET_Uniform3ui(table, save_Uniform3ui);
   SET_Uniform4ui(table, save_Uniform4ui);
   SET_Uniform1uiv(table, save_Uniform1uiv);
   SET_Uniform2uiv(table, save_Uniform2uiv);
   SET_Uniform3uiv(table, save_Uniform3uiv);
   SET_Uniform4uiv(table, save_Uniform4uiv);
#else
   (void) save_Uniform1ui;
   (void) save_Uniform2ui;
   (void) save_Uniform3ui;
   (void) save_Uniform4ui;
   (void) save_Uniform1uiv;
   (void) save_Uniform2uiv;
   (void) save_Uniform3uiv;
   (void) save_Uniform4uiv;
#endif

#if FEATURE_EXT_transform_feedback
   /* These are not compiled into display lists: */
   SET_BindBufferBaseEXT(table, _mesa_BindBufferBase);
   SET_BindBufferOffsetEXT(table, _mesa_BindBufferOffsetEXT);
   SET_BindBufferRangeEXT(table, _mesa_BindBufferRange);
   SET_TransformFeedbackVaryingsEXT(table, _mesa_TransformFeedbackVaryings);
   /* These are: */
   SET_BeginTransformFeedbackEXT(table, save_BeginTransformFeedback);
   SET_EndTransformFeedbackEXT(table, save_EndTransformFeedback);
   SET_BindTransformFeedback(table, save_BindTransformFeedback);
   SET_PauseTransformFeedback(table, save_PauseTransformFeedback);
   SET_ResumeTransformFeedback(table, save_ResumeTransformFeedback);
   SET_DrawTransformFeedback(table, save_DrawTransformFeedback);
#endif

   /* GL_ARB_instanced_arrays */
   SET_VertexAttribDivisorARB(table, save_VertexAttribDivisor);

   /* GL_NV_texture_barrier */
   SET_TextureBarrierNV(table, save_TextureBarrierNV);

   /* GL_ARB_sampler_objects */
   _mesa_init_sampler_object_dispatch(table); /* plug in Gen/Get/etc functions */
   SET_BindSampler(table, save_BindSampler);
   SET_SamplerParameteri(table, save_SamplerParameteri);
   SET_SamplerParameterf(table, save_SamplerParameterf);
   SET_SamplerParameteriv(table, save_SamplerParameteriv);
   SET_SamplerParameterfv(table, save_SamplerParameterfv);
   SET_SamplerParameterIiv(table, save_SamplerParameterIiv);
   SET_SamplerParameterIuiv(table, save_SamplerParameterIuiv);

   /* GL_ARB_draw_buffer_blend */
   SET_BlendFunciARB(table, save_BlendFunci);
   SET_BlendFuncSeparateiARB(table, save_BlendFuncSeparatei);
   SET_BlendEquationiARB(table, save_BlendEquationi);
   SET_BlendEquationSeparateiARB(table, save_BlendEquationSeparatei);

   /* GL_ARB_geometry_shader4 */
   SET_ProgramParameteriARB(table, save_ProgramParameteri);
   SET_FramebufferTextureARB(table, save_FramebufferTexture);
   SET_FramebufferTextureFaceARB(table, save_FramebufferTextureFace);

   /* GL_NV_conditional_render */
   SET_BeginConditionalRenderNV(table, save_BeginConditionalRender);
   SET_EndConditionalRenderNV(table, save_EndConditionalRender);

   /* GL_ARB_sync */
   _mesa_init_sync_dispatch(table);
   SET_WaitSync(table, save_WaitSync);

   /* GL_ARB_texture_storage (no dlist support) */
   SET_TexStorage1D(table, _mesa_TexStorage1D);
   SET_TexStorage2D(table, _mesa_TexStorage2D);
   SET_TexStorage3D(table, _mesa_TexStorage3D);
   SET_TextureStorage1DEXT(table, _mesa_TextureStorage1DEXT);
   SET_TextureStorage2DEXT(table, _mesa_TextureStorage2DEXT);
   SET_TextureStorage3DEXT(table, _mesa_TextureStorage3DEXT);

   return table;
}



static const char *
enum_string(GLenum k)
{
   return _mesa_lookup_enum_by_nr(k);
}


/**
 * Print the commands in a display list.  For debugging only.
 * TODO: many commands aren't handled yet.
 */
static void GLAPIENTRY
print_list(struct gl_context *ctx, GLuint list)
{
   struct gl_display_list *dlist;
   Node *n;
   GLboolean done;

   if (!islist(ctx, list)) {
      printf("%u is not a display list ID\n", list);
      return;
   }

   dlist = lookup_list(ctx, list);
   if (!dlist)
      return;

   n = dlist->Head;

   printf("START-LIST %u, address %p\n", list, (void *) n);

   done = n ? GL_FALSE : GL_TRUE;
   while (!done) {
      const OpCode opcode = n[0].opcode;

      if (is_ext_opcode(opcode)) {
         n += ext_opcode_print(ctx, n);
      }
      else {
         switch (opcode) {
         case OPCODE_ACCUM:
            printf("Accum %s %g\n", enum_string(n[1].e), n[2].f);
            break;
         case OPCODE_BITMAP:
            printf("Bitmap %d %d %g %g %g %g %p\n", n[1].i, n[2].i,
                         n[3].f, n[4].f, n[5].f, n[6].f, (void *) n[7].data);
            break;
         case OPCODE_CALL_LIST:
            printf("CallList %d\n", (int) n[1].ui);
            break;
         case OPCODE_CALL_LIST_OFFSET:
            printf("CallList %d + offset %u = %u\n", (int) n[1].ui,
                         ctx->List.ListBase, ctx->List.ListBase + n[1].ui);
            break;
         case OPCODE_COLOR_TABLE_PARAMETER_FV:
            printf("ColorTableParameterfv %s %s %f %f %f %f\n",
                         enum_string(n[1].e), enum_string(n[2].e),
                         n[3].f, n[4].f, n[5].f, n[6].f);
            break;
         case OPCODE_COLOR_TABLE_PARAMETER_IV:
            printf("ColorTableParameteriv %s %s %d %d %d %d\n",
                         enum_string(n[1].e), enum_string(n[2].e),
                         n[3].i, n[4].i, n[5].i, n[6].i);
            break;
         case OPCODE_DISABLE:
            printf("Disable %s\n", enum_string(n[1].e));
            break;
         case OPCODE_ENABLE:
            printf("Enable %s\n", enum_string(n[1].e));
            break;
         case OPCODE_FRUSTUM:
            printf("Frustum %g %g %g %g %g %g\n",
                         n[1].f, n[2].f, n[3].f, n[4].f, n[5].f, n[6].f);
            break;
         case OPCODE_LINE_STIPPLE:
            printf("LineStipple %d %x\n", n[1].i, (int) n[2].us);
            break;
         case OPCODE_LOAD_IDENTITY:
            printf("LoadIdentity\n");
            break;
         case OPCODE_LOAD_MATRIX:
            printf("LoadMatrix\n");
            printf("  %8f %8f %8f %8f\n",
                         n[1].f, n[5].f, n[9].f, n[13].f);
            printf("  %8f %8f %8f %8f\n",
                         n[2].f, n[6].f, n[10].f, n[14].f);
            printf("  %8f %8f %8f %8f\n",
                         n[3].f, n[7].f, n[11].f, n[15].f);
            printf("  %8f %8f %8f %8f\n",
                         n[4].f, n[8].f, n[12].f, n[16].f);
            break;
         case OPCODE_MULT_MATRIX:
            printf("MultMatrix (or Rotate)\n");
            printf("  %8f %8f %8f %8f\n",
                         n[1].f, n[5].f, n[9].f, n[13].f);
            printf("  %8f %8f %8f %8f\n",
                         n[2].f, n[6].f, n[10].f, n[14].f);
            printf("  %8f %8f %8f %8f\n",
                         n[3].f, n[7].f, n[11].f, n[15].f);
            printf("  %8f %8f %8f %8f\n",
                         n[4].f, n[8].f, n[12].f, n[16].f);
            break;
         case OPCODE_ORTHO:
            printf("Ortho %g %g %g %g %g %g\n",
                         n[1].f, n[2].f, n[3].f, n[4].f, n[5].f, n[6].f);
            break;
         case OPCODE_POP_ATTRIB:
            printf("PopAttrib\n");
            break;
         case OPCODE_POP_MATRIX:
            printf("PopMatrix\n");
            break;
         case OPCODE_POP_NAME:
            printf("PopName\n");
            break;
         case OPCODE_PUSH_ATTRIB:
            printf("PushAttrib %x\n", n[1].bf);
            break;
         case OPCODE_PUSH_MATRIX:
            printf("PushMatrix\n");
            break;
         case OPCODE_PUSH_NAME:
            printf("PushName %d\n", (int) n[1].ui);
            break;
         case OPCODE_RASTER_POS:
            printf("RasterPos %g %g %g %g\n",
                         n[1].f, n[2].f, n[3].f, n[4].f);
            break;
         case OPCODE_ROTATE:
            printf("Rotate %g %g %g %g\n",
                         n[1].f, n[2].f, n[3].f, n[4].f);
            break;
         case OPCODE_SCALE:
            printf("Scale %g %g %g\n", n[1].f, n[2].f, n[3].f);
            break;
         case OPCODE_TRANSLATE:
            printf("Translate %g %g %g\n", n[1].f, n[2].f, n[3].f);
            break;
         case OPCODE_BIND_TEXTURE:
            printf("BindTexture %s %d\n",
                         _mesa_lookup_enum_by_nr(n[1].ui), n[2].ui);
            break;
         case OPCODE_SHADE_MODEL:
            printf("ShadeModel %s\n", _mesa_lookup_enum_by_nr(n[1].ui));
            break;
         case OPCODE_MAP1:
            printf("Map1 %s %.3f %.3f %d %d\n",
                         _mesa_lookup_enum_by_nr(n[1].ui),
                         n[2].f, n[3].f, n[4].i, n[5].i);
            break;
         case OPCODE_MAP2:
            printf("Map2 %s %.3f %.3f %.3f %.3f %d %d %d %d\n",
                         _mesa_lookup_enum_by_nr(n[1].ui),
                         n[2].f, n[3].f, n[4].f, n[5].f,
                         n[6].i, n[7].i, n[8].i, n[9].i);
            break;
         case OPCODE_MAPGRID1:
            printf("MapGrid1 %d %.3f %.3f\n", n[1].i, n[2].f, n[3].f);
            break;
         case OPCODE_MAPGRID2:
            printf("MapGrid2 %d %.3f %.3f, %d %.3f %.3f\n",
                         n[1].i, n[2].f, n[3].f, n[4].i, n[5].f, n[6].f);
            break;
         case OPCODE_EVALMESH1:
            printf("EvalMesh1 %d %d\n", n[1].i, n[2].i);
            break;
         case OPCODE_EVALMESH2:
            printf("EvalMesh2 %d %d %d %d\n",
                         n[1].i, n[2].i, n[3].i, n[4].i);
            break;

         case OPCODE_ATTR_1F_NV:
            printf("ATTR_1F_NV attr %d: %f\n", n[1].i, n[2].f);
            break;
         case OPCODE_ATTR_2F_NV:
            printf("ATTR_2F_NV attr %d: %f %f\n",
                         n[1].i, n[2].f, n[3].f);
            break;
         case OPCODE_ATTR_3F_NV:
            printf("ATTR_3F_NV attr %d: %f %f %f\n",
                         n[1].i, n[2].f, n[3].f, n[4].f);
            break;
         case OPCODE_ATTR_4F_NV:
            printf("ATTR_4F_NV attr %d: %f %f %f %f\n",
                         n[1].i, n[2].f, n[3].f, n[4].f, n[5].f);
            break;
         case OPCODE_ATTR_1F_ARB:
            printf("ATTR_1F_ARB attr %d: %f\n", n[1].i, n[2].f);
            break;
         case OPCODE_ATTR_2F_ARB:
            printf("ATTR_2F_ARB attr %d: %f %f\n",
                         n[1].i, n[2].f, n[3].f);
            break;
         case OPCODE_ATTR_3F_ARB:
            printf("ATTR_3F_ARB attr %d: %f %f %f\n",
                         n[1].i, n[2].f, n[3].f, n[4].f);
            break;
         case OPCODE_ATTR_4F_ARB:
            printf("ATTR_4F_ARB attr %d: %f %f %f %f\n",
                         n[1].i, n[2].f, n[3].f, n[4].f, n[5].f);
            break;

         case OPCODE_MATERIAL:
            printf("MATERIAL %x %x: %f %f %f %f\n",
                         n[1].i, n[2].i, n[3].f, n[4].f, n[5].f, n[6].f);
            break;
         case OPCODE_BEGIN:
            printf("BEGIN %x\n", n[1].i);
            break;
         case OPCODE_END:
            printf("END\n");
            break;
         case OPCODE_RECTF:
            printf("RECTF %f %f %f %f\n", n[1].f, n[2].f, n[3].f,
                         n[4].f);
            break;
         case OPCODE_EVAL_C1:
            printf("EVAL_C1 %f\n", n[1].f);
            break;
         case OPCODE_EVAL_C2:
            printf("EVAL_C2 %f %f\n", n[1].f, n[2].f);
            break;
         case OPCODE_EVAL_P1:
            printf("EVAL_P1 %d\n", n[1].i);
            break;
         case OPCODE_EVAL_P2:
            printf("EVAL_P2 %d %d\n", n[1].i, n[2].i);
            break;

         case OPCODE_PROVOKING_VERTEX:
            printf("ProvokingVertex %s\n",
                         _mesa_lookup_enum_by_nr(n[1].ui));
            break;

            /*
             * meta opcodes/commands
             */
         case OPCODE_ERROR:
            printf("Error: %s %s\n",
                         enum_string(n[1].e), (const char *) n[2].data);
            break;
         case OPCODE_CONTINUE:
            printf("DISPLAY-LIST-CONTINUE\n");
            n = (Node *) n[1].next;
            break;
         case OPCODE_END_OF_LIST:
            printf("END-LIST %u\n", list);
            done = GL_TRUE;
            break;
         default:
            if (opcode < 0 || opcode > OPCODE_END_OF_LIST) {
               printf
                  ("ERROR IN DISPLAY LIST: opcode = %d, address = %p\n",
                   opcode, (void *) n);
               return;
            }
            else {
               printf("command %d, %u operands\n", opcode,
                            InstSize[opcode]);
            }
         }
         /* increment n to point to next compiled command */
         if (opcode != OPCODE_CONTINUE) {
            n += InstSize[opcode];
         }
      }
   }
}



/**
 * Clients may call this function to help debug display list problems.
 * This function is _ONLY_FOR_DEBUGGING_PURPOSES_.  It may be removed,
 * changed, or break in the future without notice.
 */
void
mesa_print_display_list(GLuint list)
{
   GET_CURRENT_CONTEXT(ctx);
   print_list(ctx, list);
}


/**********************************************************************/
/*****                      Initialization                        *****/
/**********************************************************************/

void
_mesa_save_vtxfmt_init(GLvertexformat * vfmt)
{
   _MESA_INIT_ARRAYELT_VTXFMT(vfmt, _ae_);

   vfmt->Begin = save_Begin;

   _MESA_INIT_DLIST_VTXFMT(vfmt, save_);

   vfmt->Color3f = save_Color3f;
   vfmt->Color3fv = save_Color3fv;
   vfmt->Color4f = save_Color4f;
   vfmt->Color4fv = save_Color4fv;
   vfmt->EdgeFlag = save_EdgeFlag;
   vfmt->End = save_End;

   _MESA_INIT_EVAL_VTXFMT(vfmt, save_);

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
   vfmt->VertexAttrib1fARB = save_VertexAttrib1fARB;
   vfmt->VertexAttrib1fvARB = save_VertexAttrib1fvARB;
   vfmt->VertexAttrib2fARB = save_VertexAttrib2fARB;
   vfmt->VertexAttrib2fvARB = save_VertexAttrib2fvARB;
   vfmt->VertexAttrib3fARB = save_VertexAttrib3fARB;
   vfmt->VertexAttrib3fvARB = save_VertexAttrib3fvARB;
   vfmt->VertexAttrib4fARB = save_VertexAttrib4fARB;
   vfmt->VertexAttrib4fvARB = save_VertexAttrib4fvARB;

   vfmt->Rectf = save_Rectf;

   /* GL_ARB_draw_instanced */
   vfmt->DrawArraysInstanced = save_DrawArraysInstancedARB;
   vfmt->DrawElementsInstanced = save_DrawElementsInstancedARB;

   /* GL_ARB_draw_elements_base_vertex */
   vfmt->DrawElementsInstancedBaseVertex = save_DrawElementsInstancedBaseVertexARB;

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
   vfmt->MultiDrawElemementsEXT = 0;
   vfmt->DrawElementsBaseVertex = 0;
   vfmt->DrawRangeElementsBaseVertex = 0;
   vfmt->MultiDrawElemementsBaseVertex = 0;
#endif
}


void
_mesa_install_dlist_vtxfmt(struct _glapi_table *disp,
                           const GLvertexformat *vfmt)
{
   SET_CallList(disp, vfmt->CallList);
   SET_CallLists(disp, vfmt->CallLists);
}


void _mesa_init_dlist_dispatch(struct _glapi_table *disp)
{
   SET_CallList(disp, _mesa_CallList);
   SET_CallLists(disp, _mesa_CallLists);

   SET_DeleteLists(disp, _mesa_DeleteLists);
   SET_EndList(disp, _mesa_EndList);
   SET_GenLists(disp, _mesa_GenLists);
   SET_IsList(disp, _mesa_IsList);
   SET_ListBase(disp, _mesa_ListBase);
   SET_NewList(disp, _mesa_NewList);
}


#endif /* FEATURE_dlist */


/**
 * Initialize display list state for given context.
 */
void
_mesa_init_display_list(struct gl_context *ctx)
{
   static GLboolean tableInitialized = GL_FALSE;

   /* zero-out the instruction size table, just once */
   if (!tableInitialized) {
      memset(InstSize, 0, sizeof(InstSize));
      tableInitialized = GL_TRUE;
   }

   /* extension info */
   ctx->ListExt = CALLOC_STRUCT(gl_list_extensions);

   /* Display list */
   ctx->ListState.CallDepth = 0;
   ctx->ExecuteFlag = GL_TRUE;
   ctx->CompileFlag = GL_FALSE;
   ctx->ListState.CurrentBlock = NULL;
   ctx->ListState.CurrentPos = 0;

   /* Display List group */
   ctx->List.ListBase = 0;

#if FEATURE_dlist
   _mesa_save_vtxfmt_init(&ctx->ListState.ListVtxfmt);
#endif
}


void
_mesa_free_display_list_data(struct gl_context *ctx)
{
   free(ctx->ListExt);
   ctx->ListExt = NULL;
}

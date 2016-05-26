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

#include <precomp.h>

#include "config.h"

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
   OPCODE_BLEND_FUNC,

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
   OPCODE_COPY_PIXELS,
   OPCODE_COPY_TEX_IMAGE1D,
   OPCODE_COPY_TEX_IMAGE2D,
   OPCODE_COPY_TEX_SUB_IMAGE1D,
   OPCODE_COPY_TEX_SUB_IMAGE2D,
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
   OPCODE_TEX_SUB_IMAGE1D,
   OPCODE_TEX_SUB_IMAGE2D,
   OPCODE_TRANSLATE,
   OPCODE_VIEWPORT,
   OPCODE_WINDOW_POS,
   /* GL_ARB_multisample */
   OPCODE_SAMPLE_COVERAGE,
   /* GL_ARB_window_pos */
   OPCODE_WINDOW_POS_ARB,
   /* GL_EXT_depth_bounds_test */
   OPCODE_DEPTH_BOUNDS_EXT,

   /* Vertex attributes -- fallback for when optimized display
    * list build isn't active.
    */
   OPCODE_ATTR_1F_NV,
   OPCODE_ATTR_2F_NV,
   OPCODE_ATTR_3F_NV,
   OPCODE_ATTR_4F_NV,
   OPCODE_MATERIAL,
   OPCODE_BEGIN,
   OPCODE_END,
   OPCODE_RECTF,
   OPCODE_EVAL_C1,
   OPCODE_EVAL_C2,
   OPCODE_EVAL_P1,
   OPCODE_EVAL_P2,

   /* GL_EXT_texture_integer */
   OPCODE_CLEARCOLOR_I,
   OPCODE_CLEARCOLOR_UI,
   OPCODE_TEXPARAMETER_I,
   OPCODE_TEXPARAMETER_UI,

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
         case OPCODE_TEX_SUB_IMAGE1D:
            free(n[7].data);
            n += InstSize[n[0].opcode];
            break;
         case OPCODE_TEX_SUB_IMAGE2D:
            free(n[9].data);
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
   GLvoid *image;

   if (width <= 0 || height <= 0) {
      return NULL;
   }

   if (_mesa_bytes_per_pixel(format, type) < 0) {
      /* bad format and/or type */
      return NULL;
   }

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
save_BlendFunc(GLenum srcfactor, GLenum dstfactor)
{
   GET_CURRENT_CONTEXT(ctx);
   Node *n;
   ASSERT_OUTSIDE_SAVE_BEGIN_END_AND_FLUSH(ctx);
   n = alloc_instruction(ctx, OPCODE_BLEND_FUNC, 2);
   if (n) {
      n[1].e = srcfactor;
      n[2].e = dstfactor;
   }
   if (ctx->ExecuteFlag) {
      CALL_BlendFunc(ctx->Exec, (srcfactor, dstfactor));
   }
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
   save_Attr1fNV(VERT_ATTRIB_TEX, x);
}

static void GLAPIENTRY
save_TexCoord1fv(const GLfloat * v)
{
   save_Attr1fNV(VERT_ATTRIB_TEX, v[0]);
}

static void GLAPIENTRY
save_TexCoord2f(GLfloat x, GLfloat y)
{
   save_Attr2fNV(VERT_ATTRIB_TEX, x, y);
}

static void GLAPIENTRY
save_TexCoord2fv(const GLfloat * v)
{
   save_Attr2fNV(VERT_ATTRIB_TEX, v[0], v[1]);
}

static void GLAPIENTRY
save_TexCoord3f(GLfloat x, GLfloat y, GLfloat z)
{
   save_Attr3fNV(VERT_ATTRIB_TEX, x, y, z);
}

static void GLAPIENTRY
save_TexCoord3fv(const GLfloat * v)
{
   save_Attr3fNV(VERT_ATTRIB_TEX, v[0], v[1], v[2]);
}

static void GLAPIENTRY
save_TexCoord4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
   save_Attr4fNV(VERT_ATTRIB_TEX, x, y, z, w);
}

static void GLAPIENTRY
save_TexCoord4fv(const GLfloat * v)
{
   save_Attr4fNV(VERT_ATTRIB_TEX, v[0], v[1], v[2], v[3]);
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
   save_Attr3fNV(VERT_ATTRIB_COLOR, x, y, z);
}

static void GLAPIENTRY
save_Color3fv(const GLfloat * v)
{
   save_Attr3fNV(VERT_ATTRIB_COLOR, v[0], v[1], v[2]);
}

static void GLAPIENTRY
save_Color4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
   save_Attr4fNV(VERT_ATTRIB_COLOR, x, y, z, w);
}

static void GLAPIENTRY
save_Color4fv(const GLfloat * v)
{
   save_Attr4fNV(VERT_ATTRIB_COLOR, v[0], v[1], v[2], v[3]);
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
         case OPCODE_BLEND_FUNC:
            CALL_BlendFunc(ctx->Exec, (n[1].e, n[2].e));
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
         case OPCODE_COLOR_MATERIAL:
            CALL_ColorMaterial(ctx->Exec, (n[1].e, n[2].e));
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
         case OPCODE_STENCIL_FUNC:
            CALL_StencilFunc(ctx->Exec, (n[1].e, n[2].i, n[3].ui));
            break;
         case OPCODE_STENCIL_MASK:
            CALL_StencilMask(ctx->Exec, (n[1].ui));
            break;
         case OPCODE_STENCIL_OP:
            CALL_StencilOp(ctx->Exec, (n[1].e, n[2].e, n[3].e));
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
         case OPCODE_SAMPLE_COVERAGE:  /* GL_ARB_multisample */
            CALL_SampleCoverageARB(ctx->Exec, (n[1].f, n[2].b));
            break;
         case OPCODE_WINDOW_POS_ARB:   /* GL_ARB_window_pos */
            CALL_WindowPos3fMESA(ctx->Exec, (n[1].f, n[2].f, n[3].f));
            break;

         case OPCODE_DEPTH_BOUNDS_EXT:
            CALL_DepthBoundsEXT(ctx->Exec, (n[1].f, n[2].f));
            break;

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
         case OPCODE_MATERIAL:
            {
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
exec_VertexPointer(GLint size, GLenum type, GLsizei stride,
                   const GLvoid *ptr)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   CALL_VertexPointer(ctx->Exec, (size, type, stride, ptr));
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
exec_FogCoordPointerEXT(GLenum type, GLsizei stride, const GLvoid *ptr)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);
   CALL_FogCoordPointerEXT(ctx->Exec, (type, stride, ptr));
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
   SET_ColorMaterial(table, save_ColorMaterial);
   SET_CopyPixels(table, save_CopyPixels);
   SET_CullFace(table, save_CullFace);
   SET_DeleteLists(table, _mesa_DeleteLists);
   SET_DepthFunc(table, save_DepthFunc);
   SET_DepthMask(table, save_DepthMask);
   SET_DepthRange(table, save_DepthRange);
   SET_Disable(table, save_Disable);
   SET_DrawBuffer(table, save_DrawBuffer);
   SET_DrawPixels(table, save_DrawPixels);
   SET_Enable(table, save_Enable);
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

   /* 3. GL_EXT_polygon_offset */
   SET_PolygonOffsetEXT(table, save_PolygonOffsetEXT);

   /* 30. GL_EXT_vertex_array */
   SET_ColorPointerEXT(table, exec_ColorPointerEXT);
   SET_EdgeFlagPointerEXT(table, exec_EdgeFlagPointerEXT);
   SET_IndexPointerEXT(table, exec_IndexPointerEXT);
   SET_NormalPointerEXT(table, exec_NormalPointerEXT);
   SET_TexCoordPointerEXT(table, exec_TexCoordPointerEXT);
   SET_VertexPointerEXT(table, exec_VertexPointerEXT);

   /* 54. GL_EXT_point_parameters */
   SET_PointParameterfEXT(table, save_PointParameterfEXT);
   SET_PointParameterfvEXT(table, save_PointParameterfvEXT);

   /* 97. GL_EXT_compiled_vertex_array */
   SET_LockArraysEXT(table, exec_LockArraysEXT);
   SET_UnlockArraysEXT(table, exec_UnlockArraysEXT);

   /* 149. GL_EXT_fog_coord */
   SET_FogCoordPointerEXT(table, exec_FogCoordPointerEXT);

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

   /* 262. GL_NV_point_sprite */
   SET_PointParameteriNV(table, save_PointParameteriNV);
   SET_PointParameterivNV(table, save_PointParameterivNV);

   /* ???. GL_EXT_depth_bounds_test */
   SET_DepthBoundsEXT(table, save_DepthBoundsEXT);

   /* ARB 3. GL_ARB_transpose_matrix */
   SET_LoadTransposeMatrixdARB(table, save_LoadTransposeMatrixdARB);
   SET_LoadTransposeMatrixfARB(table, save_LoadTransposeMatrixfARB);
   SET_MultTransposeMatrixdARB(table, save_MultTransposeMatrixdARB);
   SET_MultTransposeMatrixfARB(table, save_MultTransposeMatrixfARB);

   /* ARB 5. GL_ARB_multisample */
   SET_SampleCoverageARB(table, save_SampleCoverageARB);

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

   /* ARB 50. GL_ARB_map_buffer_range */
#if FEATURE_ARB_map_buffer_range
   SET_MapBufferRange(table, _mesa_MapBufferRange); /* no dlist save */
   SET_FlushMappedBufferRange(table, _mesa_FlushMappedBufferRange); /* no dl */
#endif

   /* GL_EXT_texture_integer */
   SET_ClearColorIiEXT(table, save_ClearColorIi);
   SET_ClearColorIuiEXT(table, save_ClearColorIui);
   SET_TexParameterIivEXT(table, save_TexParameterIiv);
   SET_TexParameterIuivEXT(table, save_TexParameterIuiv);
   SET_GetTexParameterIivEXT(table, exec_GetTexParameterIiv);
   SET_GetTexParameterIuivEXT(table, exec_GetTexParameterIuiv);

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
   vfmt->Normal3f = save_Normal3f;
   vfmt->Normal3fv = save_Normal3fv;
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

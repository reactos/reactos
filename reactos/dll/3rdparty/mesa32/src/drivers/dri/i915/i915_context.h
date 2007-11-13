 /**************************************************************************
 * 
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
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

#ifndef I915CONTEXT_INC
#define I915CONTEXT_INC

#include "intel_context.h"

#define I915_FALLBACK_TEXTURE		 0x1000
#define I915_FALLBACK_COLORMASK		 0x2000
#define I915_FALLBACK_STENCIL		 0x4000
#define I915_FALLBACK_STIPPLE		 0x8000
#define I915_FALLBACK_PROGRAM		 0x10000
#define I915_FALLBACK_LOGICOP		 0x20000
#define I915_FALLBACK_POLYGON_SMOOTH	 0x40000
#define I915_FALLBACK_POINT_SMOOTH	 0x80000

#define I915_UPLOAD_CTX              0x1
#define I915_UPLOAD_BUFFERS          0x2
#define I915_UPLOAD_STIPPLE          0x4
#define I915_UPLOAD_PROGRAM          0x8
#define I915_UPLOAD_CONSTANTS        0x10
#define I915_UPLOAD_FOG              0x20
#define I915_UPLOAD_INVARIENT        0x40
#define I915_UPLOAD_TEX(i)           (0x00010000<<(i))
#define I915_UPLOAD_TEX_ALL          (0x00ff0000)
#define I915_UPLOAD_TEX_0_SHIFT      16


/* State structure offsets - these will probably disappear.
 */
#define I915_DESTREG_CBUFADDR0 0
#define I915_DESTREG_CBUFADDR1 1
#define I915_DESTREG_CBUFADDR2 2
#define I915_DESTREG_DBUFADDR0 3
#define I915_DESTREG_DBUFADDR1 4
#define I915_DESTREG_DBUFADDR2 5
#define I915_DESTREG_DV0 6
#define I915_DESTREG_DV1 7
#define I915_DESTREG_SENABLE 8
#define I915_DESTREG_SR0 9
#define I915_DESTREG_SR1 10
#define I915_DESTREG_SR2 11
#define I915_DEST_SETUP_SIZE 12

#define I915_CTXREG_STATE4		0
#define I915_CTXREG_LI	        	1
#define I915_CTXREG_LIS2		        2
#define I915_CTXREG_LIS4	        	3
#define I915_CTXREG_LIS5	        	4
#define I915_CTXREG_LIS6	         	5
#define I915_CTXREG_IAB   	 	6
#define I915_CTXREG_BLENDCOLOR0		7
#define I915_CTXREG_BLENDCOLOR1		8
#define I915_CTX_SETUP_SIZE		9

#define I915_FOGREG_COLOR		0
#define I915_FOGREG_MODE0		1
#define I915_FOGREG_MODE1		2
#define I915_FOGREG_MODE2		3
#define I915_FOGREG_MODE3		4
#define I915_FOG_SETUP_SIZE		5

#define I915_STPREG_ST0        0
#define I915_STPREG_ST1        1
#define I915_STP_SETUP_SIZE    2

#define I915_TEXREG_MS2        0
#define I915_TEXREG_MS3        1
#define I915_TEXREG_MS4        2
#define I915_TEXREG_SS2        3
#define I915_TEXREG_SS3        4
#define I915_TEXREG_SS4        5
#define I915_TEX_SETUP_SIZE    6

#define I915_MAX_CONSTANT      32
#define I915_CONSTANT_SIZE     (2+(4*I915_MAX_CONSTANT))


#define I915_PROGRAM_SIZE      192


/* Hardware version of a parsed fragment program.  "Derived" from the
 * mesa fragment_program struct.
 */
struct i915_fragment_program {
   struct gl_fragment_program FragProg;

   GLboolean translated;
   GLboolean params_uptodate;
   GLboolean on_hardware;
   GLboolean error;		/* If program is malformed for any reason. */

   GLuint nr_tex_indirect;
   GLuint nr_tex_insn;
   GLuint nr_alu_insn;
   GLuint nr_decl_insn;




   /* TODO: split between the stored representation of a program and
    * the state used to build that representation.
    */
   GLcontext *ctx;

   GLuint declarations[I915_PROGRAM_SIZE];
   GLuint program[I915_PROGRAM_SIZE];

   GLfloat constant[I915_MAX_CONSTANT][4];
   GLuint constant_flags[I915_MAX_CONSTANT];
   GLuint nr_constants;

   GLuint *csr;			/* Cursor, points into program.
				 */

   GLuint *decl;		/* Cursor, points into declarations.
				 */
   
   GLuint decl_s;		/* flags for which s regs need to be decl'd */
   GLuint decl_t;		/* flags for which t regs need to be decl'd */

   GLuint temp_flag;		/* Tracks temporary regs which are in
				 * use.
				 */

   GLuint utemp_flag;		/* Tracks TYPE_U temporary regs which are in
				 * use.
				 */



   /* Helpers for i915_fragprog.c:
    */
   GLuint wpos_tex;
   GLboolean depth_written;

   struct { 
      GLuint reg;		/* Hardware constant idx */
      const GLfloat *values; 	/* Pointer to tracked values */
   } param[I915_MAX_CONSTANT];
   GLuint nr_params;
      



   /* Helpers for i915_texprog.c:
    */
   GLuint src_texture;		/* Reg containing sampled texture color,
				 * else UREG_BAD.
				 */

   GLuint src_previous;		/* Reg containing color from previous 
				 * stage.  May need to be decl'd.
				 */

   GLuint last_tex_stage;	/* Number of last enabled texture unit */

   struct vertex_buffer *VB;
};






struct i915_texture_object
{
   struct intel_texture_object intel;
   GLenum lastTarget;
   GLboolean refs_border_color;
   GLuint Setup[I915_TEX_SETUP_SIZE];
};

#define I915_TEX_UNITS 8


struct i915_hw_state {
   GLuint Ctx[I915_CTX_SETUP_SIZE];
   GLuint Buffer[I915_DEST_SETUP_SIZE];
   GLuint Stipple[I915_STP_SETUP_SIZE];
   GLuint Fog[I915_FOG_SETUP_SIZE];
   GLuint Tex[I915_TEX_UNITS][I915_TEX_SETUP_SIZE];
   GLuint Constant[I915_CONSTANT_SIZE];
   GLuint ConstantSize;
   GLuint Program[I915_PROGRAM_SIZE];
   GLuint ProgramSize;
   GLuint active;		/* I915_UPLOAD_* */
   GLuint emitted;		/* I915_UPLOAD_* */
};

#define I915_FOG_PIXEL  2
#define I915_FOG_VERTEX 1
#define I915_FOG_NONE   0

struct i915_context 
{
   struct intel_context intel;

   GLuint last_ReallyEnabled;
   GLuint vertex_fog;

   struct i915_fragment_program tex_program;
   struct i915_fragment_program *current_program;

   struct i915_hw_state meta, initial, state, *current;
};


typedef struct i915_context *i915ContextPtr;
typedef struct i915_texture_object *i915TextureObjectPtr;

#define I915_CONTEXT(ctx)	((i915ContextPtr)(ctx))



#define I915_STATECHANGE(i915, flag)					\
do {									\
   if (0) fprintf(stderr, "I915_STATECHANGE %x in %s\n", flag, __FUNCTION__);	\
   INTEL_FIREVERTICES( &(i915)->intel );					\
   (i915)->state.emitted &= ~(flag);					\
} while (0)

#define I915_ACTIVESTATE(i915, flag, mode)			\
do {								\
   if (0) fprintf(stderr, "I915_ACTIVESTATE %x %d in %s\n",	\
		  flag, mode, __FUNCTION__);			\
   INTEL_FIREVERTICES( &(i915)->intel );				\
   if (mode)							\
      (i915)->state.active |= (flag);				\
   else								\
      (i915)->state.active &= ~(flag);				\
} while (0)


/*======================================================================
 * i915_vtbl.c
 */
extern void i915InitVtbl( i915ContextPtr i915 );



#define SZ_TO_HW(sz)  ((sz-2)&0x3)
#define EMIT_SZ(sz)   (EMIT_1F + (sz) - 1)
#define EMIT_ATTR( ATTR, STYLE, S4, SZ )				\
do {									\
   intel->vertex_attrs[intel->vertex_attr_count].attrib = (ATTR);	\
   intel->vertex_attrs[intel->vertex_attr_count].format = (STYLE);	\
   s4 |= S4;								\
   intel->vertex_attr_count++;						\
   offset += (SZ);							\
} while (0)

#define EMIT_PAD( N )							\
do {									\
   intel->vertex_attrs[intel->vertex_attr_count].attrib = 0;		\
   intel->vertex_attrs[intel->vertex_attr_count].format = EMIT_PAD;	\
   intel->vertex_attrs[intel->vertex_attr_count].offset = (N);		\
   intel->vertex_attr_count++;						\
   offset += (N);							\
} while (0)



/*======================================================================
 * i915_context.c
 */
extern GLboolean i915CreateContext( const __GLcontextModes *mesaVis,
				    __DRIcontextPrivate *driContextPriv,
				    void *sharedContextPrivate);


/*======================================================================
 * i915_texprog.c
 */
extern void i915ValidateTextureProgram( i915ContextPtr i915 );


/*======================================================================
 * i915_debug.c
 */
extern void i915_disassemble_program( const GLuint *program, GLuint sz );
extern void i915_print_ureg( const char *msg, GLuint ureg );


/*======================================================================
 * i915_state.c
 */
extern void i915InitStateFunctions( struct dd_function_table *functions );
extern void i915InitState( i915ContextPtr i915 );
extern void i915_update_fog(GLcontext *ctxx);


/*======================================================================
 * i915_tex.c
 */
extern void i915UpdateTextureState( intelContextPtr intel );
extern void i915InitTextureFuncs( struct dd_function_table *functions );
extern intelTextureObjectPtr i915AllocTexObj( struct gl_texture_object *texObj );

/*======================================================================
 * i915_metaops.c
 */
extern GLboolean
i915TryTextureReadPixels( GLcontext *ctx,
			  GLint x, GLint y, GLsizei width, GLsizei height,
			  GLenum format, GLenum type,
			  const struct gl_pixelstore_attrib *pack,
			  GLvoid *pixels );

extern GLboolean
i915TryTextureDrawPixels( GLcontext *ctx,
			  GLint x, GLint y, GLsizei width, GLsizei height,
			  GLenum format, GLenum type,
			  const struct gl_pixelstore_attrib *unpack,
			  const GLvoid *pixels );

extern void 
i915ClearWithTris( intelContextPtr intel, GLbitfield mask,
		   GLboolean all, GLint cx, GLint cy, GLint cw, GLint ch);


extern void
i915RotateWindow(intelContextPtr intel, __DRIdrawablePrivate *dPriv,
                 GLuint srcBuf);

/*======================================================================
 * i915_fragprog.c
 */
extern void i915ValidateFragmentProgram( i915ContextPtr i915 );
extern void i915InitFragProgFuncs( struct dd_function_table *functions );
	
#endif


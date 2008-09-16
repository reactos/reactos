/* $XFree86: xc/lib/GL/mesa/src/drv/ffb/ffb_context.h,v 1.2 2002/02/22 21:32:58 dawes Exp $ */

#ifndef _FFB_CONTEXT_H
#define _FFB_CONTEXT_H

#include "dri_util.h"
#include "drm.h"

#include "mtypes.h"

#include "ffb_xmesa.h"

typedef struct {
	GLfloat	alpha;
	GLfloat	red;
	GLfloat	green;
	GLfloat	blue;
} ffb_color;

#define FFB_GET_ALPHA(VTX)	\
	FFB_COLOR_FROM_FLOAT((VTX)->color[0].alpha)
#define FFB_GET_RED(VTX)	\
	FFB_COLOR_FROM_FLOAT((VTX)->color[0].red)
#define FFB_GET_GREEN(VTX)	\
	FFB_COLOR_FROM_FLOAT((VTX)->color[0].green)
#define FFB_GET_BLUE(VTX)	\
	FFB_COLOR_FROM_FLOAT((VTX)->color[0].blue)

typedef struct {
	GLfloat x, y, z;
	ffb_color	color[2];
} ffb_vertex;

#define FFB_DELAYED_VIEWPORT_VARS				\
	GLfloat		VP_SX = fmesa->hw_viewport[MAT_SX];	\
	GLfloat		VP_TX = fmesa->hw_viewport[MAT_TX];	\
	GLfloat		VP_SY = fmesa->hw_viewport[MAT_SY];	\
	GLfloat		VP_TY = fmesa->hw_viewport[MAT_TY];	\
	GLfloat		VP_SZ = fmesa->hw_viewport[MAT_SZ];	\
	GLfloat		VP_TZ = fmesa->hw_viewport[MAT_TZ];	\
	(void) VP_SX; (void) VP_SY; (void) VP_SZ; 		\
	(void) VP_TX; (void) VP_TY; (void) VP_TZ

#define FFB_GET_Z(VTX)			\
	FFB_Z_FROM_FLOAT(VP_SZ * (VTX)->z + VP_TZ)
#define FFB_GET_Y(VTX)			\
	FFB_XY_FROM_FLOAT(VP_SY * (VTX)->y + VP_TY)
#define FFB_GET_X(VTX)			\
	FFB_XY_FROM_FLOAT(VP_SX * (VTX)->x + VP_TX)

typedef void (*ffb_point_func)(GLcontext *, ffb_vertex *);
typedef void (*ffb_line_func)(GLcontext *, ffb_vertex *, ffb_vertex *);
typedef void (*ffb_tri_func)(GLcontext *, ffb_vertex *, ffb_vertex *,
			     ffb_vertex *);
typedef void (*ffb_quad_func)(GLcontext *, ffb_vertex *, ffb_vertex *,
			      ffb_vertex *, ffb_vertex *);

/* Immediate mode fast-path support. */
typedef struct {
	GLfloat		obj[4];
	GLfloat 	normal[4];
	GLfloat 	clip[4];
	GLuint 		mask;
	GLfloat		color[4];
	GLfloat		win[4];
	GLfloat		eye[4];
} ffbTnlVertex, *ffbTnlVertexPtr;

typedef void (*ffb_interp_func)(GLfloat t,
				ffbTnlVertex *O,
				const ffbTnlVertex *I,
				const ffbTnlVertex *J);

struct ffb_current_state {
	GLfloat color[4];
	GLfloat normal[4];
	GLfloat specular[4];
};

struct ffb_light_state {
	GLfloat base_color[3];
	GLfloat base_alpha;
};

struct ffb_vertex_state {
	struct ffb_current_state	current;
	struct ffb_light_state		light;
};

struct ffb_imm_vertex {
	ffbTnlVertex	vertices[8];
	ffbTnlVertex	*v0;
	ffbTnlVertex	*v1;
	ffbTnlVertex	*v2;
	ffbTnlVertex	*v3;

	void (*save_vertex)(GLcontext *ctx, ffbTnlVertex *v);
	void (*flush_vertex)(GLcontext *ctx, ffbTnlVertex *v);

	ffb_interp_func interp;

	GLuint prim, format;

	GLvertexformat vtxfmt;
};

typedef struct ffb_context_t {
	GLcontext		*glCtx;
	GLframebuffer		*glBuffer;

	ffb_fbcPtr		regs;
	volatile char		*sfb32;

	int			hw_locked;

	int			back_buffer;	/* 0 = bufferA, 1 = bufferB */

	/* Viewport matrix. */
	GLfloat			hw_viewport[16];
#define SUBPIXEL_X (-0.5F)
#define SUBPIXEL_Y (-0.5F + 0.125)

	/* Vertices in driver format. */
	ffb_vertex              *verts;

	/* Rasterization functions. */
	ffb_point_func draw_point;
	ffb_line_func draw_line;
	ffb_tri_func draw_tri;
	ffb_quad_func draw_quad;

	GLenum raster_primitive;
	GLenum render_primitive;

	GLfloat backface_sign;
	GLfloat depth_scale;

	GLfloat	ffb_2_30_fixed_scale;
	GLfloat	ffb_one_over_2_30_fixed_scale;
	GLfloat ffb_16_16_fixed_scale;
	GLfloat ffb_one_over_16_16_fixed_scale;
	GLfloat ffb_ubyte_color_scale;
	GLfloat ffb_zero;

	/* Immediate mode state. */
	struct ffb_vertex_state	vtx_state;
	struct ffb_imm_vertex	imm;

	/* Debugging knobs. */
	GLboolean debugFallbacks;

	/* This records state bits when a per-fragment attribute has
	 * been set which prevents us from rendering in hardware.
	 *
	 * As attributes change, some of these bits may clear as
	 * we move back within the chips capabilities.  If they
	 * all clear, we return to full hw rendering.
	 */
	unsigned int		bad_fragment_attrs;
#define FFB_BADATTR_FOG		0x00000001	/* Bad fog possible only when < FFB2 */
#define FFB_BADATTR_BLENDFUNC	0x00000002	/* Any non-const func based upon dst alpha */
#define FFB_BADATTR_BLENDROP	0x00000004	/* Blend enabled and LogicOP != GL_COPY */
#define FFB_BADATTR_BLENDEQN	0x00000008	/* Blend equation other than ADD */
#define FFB_BADATTR_STENCIL	0x00000010	/* Stencil enabled when < FFB2+ */
#define FFB_BADATTR_TEXTURE	0x00000020	/* Texture enabled */
#define FFB_BADATTR_SWONLY	0x00000040	/* Environment var set */

	unsigned int		state_dirty;
	unsigned int		state_fifo_ents;
#define FFB_STATE_FBC		0x00000001
#define FFB_STATE_PPC		0x00000002
#define FFB_STATE_DRAWOP	0x00000004
#define FFB_STATE_ROP		0x00000008
#define FFB_STATE_LPAT		0x00000010
#define FFB_STATE_PMASK		0x00000020
#define FFB_STATE_XPMASK	0x00000040
#define FFB_STATE_YPMASK	0x00000080
#define FFB_STATE_ZPMASK	0x00000100
#define FFB_STATE_XCLIP		0x00000200
#define FFB_STATE_CMP		0x00000400
#define FFB_STATE_MATCHAB	0x00000800
#define FFB_STATE_MAGNAB	0x00001000
#define FFB_STATE_MATCHC	0x00002000
#define FFB_STATE_MAGNC		0x00004000
#define FFB_STATE_DCUE		0x00008000
#define FFB_STATE_BLEND		0x00010000
#define FFB_STATE_CLIP		0x00020000
#define FFB_STATE_STENCIL	0x00040000
#define FFB_STATE_APAT		0x00080000
#define FFB_STATE_WID		0x00100000
#define FFB_STATE_ALL		0x001fffff

	unsigned int		state_all_fifo_ents;

#define FFB_MAKE_DIRTY(FMESA, STATE_MASK, FIFO_ENTS)	\
do {	if ((STATE_MASK) & ~((FMESA)->state_dirty)) {	\
		(FMESA)->state_dirty |= (STATE_MASK);	\
		(FMESA)->state_fifo_ents += FIFO_ENTS;	\
	}						\
} while (0)

	/* General hw reg state. */
	unsigned int		fbc;
	unsigned int		ppc;
	unsigned int		drawop;
	unsigned int		rop;

	unsigned int		lpat;
#define FFB_LPAT_BAD		0xffffffff
 
	unsigned int		wid;
	unsigned int		pmask;
	unsigned int		xpmask;
	unsigned int		ypmask;
	unsigned int		zpmask;
	unsigned int		xclip;
	unsigned int		cmp;
	unsigned int		matchab;
	unsigned int		magnab;
	unsigned int		matchc;
	unsigned int		magnc;

	/* Depth cue unit hw reg state. */
	unsigned int		dcss;	/* All FFB		*/
	unsigned int		dcsf;	/* All FFB		*/
	unsigned int		dcsb;	/* All FFB		*/
	unsigned int		dczf;	/* All FFB		*/
	unsigned int		dczb;	/* All FFB		*/
	unsigned int		dcss1;	/* >=FFB2 only		*/
	unsigned int		dcss2;	/* >=FFB2 only		*/
	unsigned int		dcss3;	/* >=FFB2 only		*/
	unsigned int		dcs2;	/* >=FFB2 only		*/
	unsigned int		dcs3;	/* >=FFB2 only		*/
	unsigned int		dcs4;	/* >=FFB2 only		*/
	unsigned int		dcd2;	/* >=FFB2 only		*/
	unsigned int		dcd3;	/* >=FFB2 only		*/
	unsigned int		dcd4;	/* >=FFB2 only		*/

	/* Blend unit hw reg state. */
	unsigned int		blendc;
	unsigned int		blendc1;
	unsigned int		blendc2;

	/* ViewPort clipping hw reg state. */
	unsigned int		vclipmin;
	unsigned int		vclipmax;
	unsigned int		vclipzmin;
	unsigned int		vclipzmax;
	struct {
		unsigned int	min;
		unsigned int	max;
	} aux_clips[4];

	/* Stencil control hw reg state.  >=FFB2+ only. */
	unsigned int		stencil;
	unsigned int		stencilctl;
	unsigned int		consty;		/* Stencil Ref */

	/* Area pattern (used for polygon stipples). */
	unsigned int		pattern[32];

	/* Fog state. */
	float			Znear, Zfar;

	drm_context_t		hHWContext;
	drm_hw_lock_t		*driHwLock;
	int			driFd;

	unsigned int		clear_pixel;
	unsigned int		clear_depth;
	unsigned int		clear_stencil;

	unsigned int		setupindex;
	unsigned int		setupnewinputs;
	unsigned int		new_gl_state;

	__DRIdrawablePrivate	*driDrawable;
	__DRIscreenPrivate	*driScreen;
	ffbScreenPrivate	*ffbScreen;
	ffb_dri_state_t		*ffb_sarea;
} ffbContextRec, *ffbContextPtr;

#define FFB_CONTEXT(ctx)	((ffbContextPtr)((ctx)->DriverCtx))

/* We want the depth values written during software rendering
 * to match what the hardware is going to put there when we
 * hw render.
 *
 * The Z buffer is 28 bits deep.  Smooth shaded primitives
 * specify a 2:30 signed fixed point Z value in the range 0.0
 * to 1.0 inclusive.
 *
 * So for example, when hw rendering, the largest Z value of
 * 1.0 would produce a value of 0x0fffffff in the actual Z
 * buffer, which is the maximum value.
 *
 * Mesa's depth type is a 32-bit uint, so we use the following macro
 * to convert to/from FFB hw Z values.  Note we also have to clear
 * out the top bits as that is where the Y (stencil) buffer is stored
 * and during hw Z buffer reads it is always there. (During writes
 * we tell the hw to discard those top 4 bits).
 */
#define Z_TO_MESA(VAL)		((GLuint)(((VAL) & 0x0fffffff) << (32 - 28)))
#define Z_FROM_MESA(VAL)	(((GLuint)((GLdouble)(VAL))) >> (32 - 28))

#endif /* !(_FFB_CONTEXT_H) */

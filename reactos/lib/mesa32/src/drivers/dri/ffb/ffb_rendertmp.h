/* $XFree86: xc/lib/GL/mesa/src/drv/ffb/ffb_rendertmp.h,v 1.2 2003/01/29 23:00:40 dawes Exp $ */

#define IMPL_LOCAL_VARS						\
	ffbContextPtr fmesa = FFB_CONTEXT(ctx);			\
	ffb_fbcPtr ffb = fmesa->regs;				\
	const GLuint * const elt = TNL_CONTEXT(ctx)->vb.Elts;	\
	FFB_DELAYED_VIEWPORT_VARS;				\
	(void) fmesa; (void) ffb; (void) elt

#if (IND & FFB_FLAT_BIT)
#define FFB_DECLARE_CACHED_COLOR(NAME)		\
	unsigned int NAME;
#define FFB_COMPUTE_CACHED_COLOR(NAME, VTX)	\
	NAME = FFB_PACK_CONST_UBYTE_ARGB_COLOR((VTX)->color[0])
#define FFB_CACHED_COLOR_SAME(NAME1, NAME2)	\
	((NAME1) == (NAME2))
#define FFB_CACHED_COLOR_SET(NAME)		\
	ffb->fg = (NAME)
#define FFB_CACHED_COLOR_UPDATE(NAME1, NAME2)	\
	ffb->fg = (NAME1) = (NAME2)
#define FFB_SET_PRIM_COLOR(COLOR_VERTEX)	\
	ffb->fg = FFB_PACK_CONST_UBYTE_ARGB_COLOR((COLOR_VERTEX)->color[0])
#define FFB_PRIM_COLOR_COST			1
#define FFB_SET_VERTEX_COLOR(VTX)		/**/
#define FFB_VERTEX_COLOR_COST			0
#else
#define FFB_DECLARE_CACHED_COLOR(NAME)		/**/
#define FFB_COMPUTE_CACHED_COLOR(NAME, VTX)	/**/
#define FFB_CACHED_COLOR_SAME(NAME1, NAME2)	0
#define FFB_CACHED_COLOR_SET(NAME1)		/**/
#define FFB_CACHED_COLOR_UPDATE(NAME1, NAME2)	/**/
#define FFB_SET_PRIM_COLOR(COLOR_VERTEX)	/**/
#define FFB_PRIM_COLOR_COST			0
#if (IND & FFB_ALPHA_BIT)
#define FFB_SET_VERTEX_COLOR(VTX)		\
	ffb->alpha = FFB_GET_ALPHA(VTX);	\
	ffb->red   = FFB_GET_RED(VTX);		\
	ffb->green = FFB_GET_GREEN(VTX);	\
	ffb->blue  = FFB_GET_BLUE(VTX)
#define FFB_VERTEX_COLOR_COST			4
#else
#define FFB_SET_VERTEX_COLOR(VTX)		\
	ffb->red   = FFB_GET_RED(VTX);		\
	ffb->green = FFB_GET_GREEN(VTX);	\
	ffb->blue  = FFB_GET_BLUE(VTX)
#define FFB_VERTEX_COLOR_COST			3
#endif
#endif

#define RESET_STIPPLE	ffb->lpat = fmesa->lpat;

#if !(IND & (FFB_TRI_CULL_BIT))
static void TAG(ffb_vb_points)(GLcontext *ctx, GLuint start, GLuint count, GLuint flags)
{
	GLint i;
	IMPL_LOCAL_VARS;

#ifdef FFB_RENDER_TRACE
	fprintf(stderr, "%s: start(%d) count(%d) flags(%x)\n",
		__FUNCTION__, start, count, flags);
#endif
	ffbRenderPrimitive(ctx, GL_POINTS);
	if (ctx->_TriangleCaps & DD_POINT_SMOOTH) {
		for (i = start; i < count; i++) {
			ffb_vertex *v0 = &fmesa->verts[ELT(i)];

			FFBFifo(fmesa, 4);
			ffb->fg = FFB_PACK_CONST_UBYTE_ARGB_COLOR(v0->color[0]);
			ffb->z = FFB_GET_Z(v0);
			ffb->y = FFB_GET_Y(v0) + 0x8000 /* FIX ME */;
			ffb->x = FFB_GET_X(v0) + 0x8000 /* FIX ME */;
		}
	} else {
		for (i = start; i < count; i++) {
			ffb_vertex *v0 = &fmesa->verts[ELT(i)];
			FFBFifo(fmesa, 4);
			ffb->fg = FFB_PACK_CONST_UBYTE_ARGB_COLOR(v0->color[0]);
			ffb->constz = Z_FROM_MESA(FFB_Z_TO_FLOAT(FFB_GET_Z(v0)));
			ffb->bh = FFB_GET_Y(v0) >> 16;
			ffb->bw = FFB_GET_X(v0) >> 16;
		}
	}

	fmesa->ffbScreen->rp_active = 1;
}

static void TAG(ffb_vb_lines)(GLcontext *ctx, GLuint start, GLuint count, GLuint flags)
{
	GLint i;
	IMPL_LOCAL_VARS;

#ifdef FFB_RENDER_TRACE
	fprintf(stderr, "%s: start(%d) count(%d) flags(%x)\n",
		__FUNCTION__, start, count, flags);
#endif
	ffbRenderPrimitive(ctx, GL_LINES);
	for (i = start + 1; i < count; i += 2) {
		ffb_vertex *v0 = &fmesa->verts[i - 1];
		ffb_vertex *v1 = &fmesa->verts[i - 0];

		FFBFifo(fmesa, (1 + FFB_PRIM_COLOR_COST +
				(FFB_VERTEX_COLOR_COST * 2) + 6));

		RESET_STIPPLE;

		FFB_SET_PRIM_COLOR(v1);

		FFB_SET_VERTEX_COLOR(v0);
		ffb->z   = FFB_GET_Z(v0);
		ffb->ryf = FFB_GET_Y(v0);
		ffb->rxf = FFB_GET_X(v0);

		FFB_SET_VERTEX_COLOR(v1);
		ffb->z   = FFB_GET_Z(v1);
		ffb->y   = FFB_GET_Y(v1);
		ffb->x   = FFB_GET_X(v1);
	}
}

static void TAG(ffb_vb_line_loop)(GLcontext *ctx, GLuint start, GLuint count, GLuint flags)
{
	GLint i;
	IMPL_LOCAL_VARS;

#ifdef FFB_RENDER_TRACE
	fprintf(stderr, "%s: start(%d) count(%d) flags(%x)\n",
		__FUNCTION__, start, count, flags);
#endif
	ffbRenderPrimitive(ctx, GL_LINE_LOOP);
	if ((flags & PRIM_BEGIN) != 0) {
		ffb_vertex *v0 = &fmesa->verts[ELT(start + 0)];
		ffb_vertex *v1 = &fmesa->verts[ELT(start + 1)];

		FFBFifo(fmesa, (1 + FFB_PRIM_COLOR_COST +
				((FFB_VERTEX_COLOR_COST * 2) + (3 * 2))));

		RESET_STIPPLE;

		FFB_SET_PRIM_COLOR(v1);

		FFB_SET_VERTEX_COLOR(v0);
		ffb->z   = FFB_GET_Z(v0);
		ffb->ryf = FFB_GET_Y(v0);
		ffb->rxf = FFB_GET_X(v0);

		FFB_SET_VERTEX_COLOR(v1);
		ffb->z   = FFB_GET_Z(v1);
		ffb->y   = FFB_GET_Y(v1);
		ffb->x   = FFB_GET_X(v1);
	}
	for (i = start + 2; i < count; i++) {
		ffb_vertex *v0 = &fmesa->verts[ELT(i)];

		FFBFifo(fmesa, (FFB_PRIM_COLOR_COST +
				(FFB_VERTEX_COLOR_COST + 3)));

		FFB_SET_PRIM_COLOR(v0);

		FFB_SET_VERTEX_COLOR(v0);
		ffb->z   = FFB_GET_Z(v0);
		ffb->y   = FFB_GET_Y(v0);
		ffb->x   = FFB_GET_X(v0);
	}
	if ((flags & PRIM_END) != 0) {
		ffb_vertex *v0 = &fmesa->verts[ELT(start)];

		FFBFifo(fmesa, (FFB_PRIM_COLOR_COST +
				(FFB_VERTEX_COLOR_COST + 3)));

		FFB_SET_PRIM_COLOR(v0);

		FFB_SET_VERTEX_COLOR(v0);
		ffb->z   = FFB_GET_Z(v0);
		ffb->y   = FFB_GET_Y(v0);
		ffb->x   = FFB_GET_X(v0);
	}

	fmesa->ffbScreen->rp_active = 1;
}

static void TAG(ffb_vb_line_strip)(GLcontext *ctx, GLuint start, GLuint count, GLuint flags)
{
	GLint i;
	FFB_DECLARE_CACHED_COLOR(cached_fg)
	IMPL_LOCAL_VARS;

#ifdef FFB_RENDER_TRACE
	fprintf(stderr, "%s: start(%d) count(%d) flags(%x)\n",
		__FUNCTION__, start, count, flags);
#endif
	ffbRenderPrimitive(ctx, GL_LINE_STRIP);
	FFBFifo(fmesa, (1 + FFB_PRIM_COLOR_COST +
			((FFB_VERTEX_COLOR_COST * 2) + (3 * 2))));

	RESET_STIPPLE;

	{
		ffb_vertex *v0 = &fmesa->verts[ELT(start + 0)];
		ffb_vertex *v1 = &fmesa->verts[ELT(start + 1)];

		FFB_COMPUTE_CACHED_COLOR(cached_fg, v0);
		FFB_CACHED_COLOR_SET(cached_fg);

		FFB_SET_VERTEX_COLOR(v0);
		ffb->z = FFB_GET_Z(v0);
		ffb->ryf = FFB_GET_Y(v0);
		ffb->rxf = FFB_GET_X(v0);

		FFB_SET_VERTEX_COLOR(v1);
		ffb->z = FFB_GET_Z(v1);
		ffb->y = FFB_GET_Y(v1);
		ffb->x = FFB_GET_X(v1);
	}

	for (i = start + 2; i < count; i++) {
		ffb_vertex *v1 = &fmesa->verts[ELT(i - 0)];
		FFB_DECLARE_CACHED_COLOR(new_fg)

		FFB_COMPUTE_CACHED_COLOR(new_fg, v1);
		if (FFB_CACHED_COLOR_SAME(cached_fg, new_fg)) {
			FFBFifo(fmesa, ((FFB_VERTEX_COLOR_COST * 1) + (3 * 1)));
		} else {
			FFBFifo(fmesa, (FFB_PRIM_COLOR_COST +
					(FFB_VERTEX_COLOR_COST * 1) + (3 * 1)));
			FFB_CACHED_COLOR_UPDATE(cached_fg, new_fg);
		}

		FFB_SET_VERTEX_COLOR(v1);
		ffb->z   = FFB_GET_Z(v1);
		ffb->y   = FFB_GET_Y(v1);
		ffb->x   = FFB_GET_X(v1);
	}

	fmesa->ffbScreen->rp_active = 1;
}
#endif /* !(IND & (FFB_TRI_CULL_BIT)) */

/* OK, now things start getting fun :-) */
#if (IND & (FFB_TRI_CULL_BIT))
#define FFB_AREA_DECLARE	GLfloat cc, ex, ey, fx, fy;
#define FFB_COMPUTE_AREA_TRI(V0, V1, V2)	\
{	ex = (V1)->x - (V0)->x;			\
	ey = (V1)->y - (V0)->y;			\
	fx = (V2)->x - (V0)->x;			\
	fy = (V2)->y - (V0)->y;			\
	cc = ex*fy-ey*fx;			\
}
#define FFB_COMPUTE_AREA_QUAD(V0, V1, V2, V3)	\
{	ex = (V2)->x - (V0)->x;			\
	ey = (V2)->y - (V0)->y;			\
	fx = (V3)->x - (V1)->x;			\
	fy = (V3)->y - (V1)->y;			\
	cc = ex*fy-ey*fx;			\
}
#else
#define FFB_AREA_DECLARE			/**/
#define FFB_COMPUTE_AREA_TRI(V0, V1, V2)	do { } while(0)
#define FFB_COMPUTE_AREA_QUAD(V0, V1, V2, V3)	do { } while(0)
#endif

#if (IND & FFB_TRI_CULL_BIT)
#define FFB_CULL_TRI(CULL_ACTION)		\
	if (cc * fmesa->backface_sign > fmesa->ffb_zero) {	\
		CULL_ACTION			\
	}
#define FFB_CULL_QUAD(CULL_ACTION)		\
	if (cc * fmesa->backface_sign > fmesa->ffb_zero) {	\
		CULL_ACTION			\
	}
#else
#define FFB_CULL_TRI(CULL_ACTION)	do { } while (0)
#define FFB_CULL_QUAD(CULL_ACTION)	do { } while (0)
#endif

static void TAG(ffb_vb_triangles)(GLcontext *ctx, GLuint start, GLuint count, GLuint flags)
{
	GLint i;
	IMPL_LOCAL_VARS;

#ifdef FFB_RENDER_TRACE
	fprintf(stderr, "%s: start(%d) count(%d) flags(%x)\n",
		__FUNCTION__, start, count, flags);
#endif
	ffbRenderPrimitive(ctx, GL_TRIANGLES);
	for (i = start + 2; i < count; i += 3) {
		ffb_vertex *v0 = &fmesa->verts[ELT(i - 2)];
		ffb_vertex *v1 = &fmesa->verts[ELT(i - 1)];
		ffb_vertex *v2 = &fmesa->verts[ELT(i - 0)];
		FFB_AREA_DECLARE

		FFB_COMPUTE_AREA_TRI(v0, v1, v2);
		FFB_CULL_TRI(continue;);

		FFBFifo(fmesa, (FFB_PRIM_COLOR_COST +
				(FFB_VERTEX_COLOR_COST * 3) + 9));
		FFB_SET_PRIM_COLOR(v2);

		FFB_SET_VERTEX_COLOR(v0);
		ffb->z   = FFB_GET_Z(v0);
		ffb->ryf = FFB_GET_Y(v0);
		ffb->rxf = FFB_GET_X(v0);

		FFB_SET_VERTEX_COLOR(v1);
		ffb->z   = FFB_GET_Z(v1);
		ffb->y   = FFB_GET_Y(v1);
		ffb->x   = FFB_GET_X(v1);

		FFB_SET_VERTEX_COLOR(v2);
		ffb->z   = FFB_GET_Z(v2);
		ffb->y   = FFB_GET_Y(v2);
		ffb->x   = FFB_GET_X(v2);
	}

	fmesa->ffbScreen->rp_active = 1;
}

static void TAG(ffb_vb_tri_strip)(GLcontext *ctx, GLuint start, GLuint count, GLuint flags)
{
	GLint i;
	GLint parity = 0;
	IMPL_LOCAL_VARS;

#ifdef FFB_RENDER_TRACE
	fprintf(stderr, "%s: start(%d) count(%d) flags(%x)\n",
		__FUNCTION__, start, count, flags);
#endif
	ffbRenderPrimitive(ctx, GL_TRIANGLE_STRIP);

	i = start + 2;
	goto something_clipped;

 something_clipped:
	for (; i < count; i++, parity ^= 1) {
		ffb_vertex *v0 = &fmesa->verts[ELT(i - 2 + parity)];
		ffb_vertex *v1 = &fmesa->verts[ELT(i - 1 - parity)];
		ffb_vertex *v2 = &fmesa->verts[ELT(i - 0)];
		FFB_AREA_DECLARE

		FFB_COMPUTE_AREA_TRI(v0, v1, v2);
		FFB_CULL_TRI(continue;);

		FFBFifo(fmesa, (FFB_PRIM_COLOR_COST +
				(FFB_VERTEX_COLOR_COST * 3) + 9));
		FFB_SET_PRIM_COLOR(v2);

		FFB_SET_VERTEX_COLOR(v0);
		ffb->z   = FFB_GET_Z(v0);
		ffb->ryf = FFB_GET_Y(v0);
		ffb->rxf = FFB_GET_X(v0);

		FFB_SET_VERTEX_COLOR(v1);
		ffb->z   = FFB_GET_Z(v1);
		ffb->y   = FFB_GET_Y(v1);
		ffb->x   = FFB_GET_X(v1);

		FFB_SET_VERTEX_COLOR(v2);
		ffb->z   = FFB_GET_Z(v2);
		ffb->y   = FFB_GET_Y(v2);
		ffb->x   = FFB_GET_X(v2);

		i++;
		parity ^= 1;
		break;
	}

	for (; i < count; i++, parity ^= 1) {
		ffb_vertex *v0 = &fmesa->verts[ELT(i - 2 + parity)];
		ffb_vertex *v1 = &fmesa->verts[ELT(i - 1 - parity)];
		ffb_vertex *v2 = &fmesa->verts[ELT(i - 0)];
		FFB_AREA_DECLARE
		(void) v0; (void) v1;

		FFB_COMPUTE_AREA_TRI(v0, v1, v2);
		FFB_CULL_TRI(i++; parity^=1; goto something_clipped;);

		FFBFifo(fmesa, (FFB_PRIM_COLOR_COST +
				(FFB_VERTEX_COLOR_COST * 1) + 3));
		FFB_SET_PRIM_COLOR(v2);

		FFB_SET_VERTEX_COLOR(v2);
		ffb->z   = FFB_GET_Z(v2);
		ffb->y   = FFB_GET_Y(v2);
		ffb->x   = FFB_GET_X(v2);
	}

	fmesa->ffbScreen->rp_active = 1;
}

static void TAG(ffb_vb_tri_fan)(GLcontext *ctx, GLuint start, GLuint count, GLuint flags)
{
	GLint i;
	IMPL_LOCAL_VARS;

#ifdef FFB_RENDER_TRACE
	fprintf(stderr, "%s: start(%d) count(%d) flags(%x)\n",
		__FUNCTION__, start, count, flags);
#endif
	ffbRenderPrimitive(ctx, GL_TRIANGLE_FAN);

	i = start + 2;
	goto something_clipped;

 something_clipped:
	for ( ; i < count; i++) {
		ffb_vertex *v0 = &fmesa->verts[ELT(start)];
		ffb_vertex *v1 = &fmesa->verts[ELT(i - 1)];
		ffb_vertex *v2 = &fmesa->verts[ELT(i - 0)];
		FFB_AREA_DECLARE

		FFB_COMPUTE_AREA_TRI(v0, v1, v2);
		FFB_CULL_TRI(continue;);

		FFBFifo(fmesa, (FFB_PRIM_COLOR_COST +
				(FFB_VERTEX_COLOR_COST * 3) + 9));
		FFB_SET_PRIM_COLOR(v2);

		FFB_SET_VERTEX_COLOR(v0);
		ffb->z   = FFB_GET_Z(v0);
		ffb->ryf = FFB_GET_Y(v0);
		ffb->rxf = FFB_GET_X(v0);

		FFB_SET_VERTEX_COLOR(v1);
		ffb->z   = FFB_GET_Z(v1);
		ffb->y   = FFB_GET_Y(v1);
		ffb->x   = FFB_GET_X(v1);

		FFB_SET_VERTEX_COLOR(v2);
		ffb->z   = FFB_GET_Z(v2);
		ffb->y   = FFB_GET_Y(v2);
		ffb->x   = FFB_GET_X(v2);

		i++;
		break;
	}

	for (; i < count; i++) {
		ffb_vertex *v0 = &fmesa->verts[ELT(start)];
		ffb_vertex *v1 = &fmesa->verts[ELT(i - 1)];
		ffb_vertex *v2 = &fmesa->verts[ELT(i - 0)];
		FFB_AREA_DECLARE
		(void) v0; (void) v1;

		FFB_COMPUTE_AREA_TRI(v0, v1, v2);
		FFB_CULL_TRI(i++; goto something_clipped;);

		FFBFifo(fmesa, (FFB_PRIM_COLOR_COST +
				(FFB_VERTEX_COLOR_COST * 1) + 3));
		FFB_SET_PRIM_COLOR(v2);

		FFB_SET_VERTEX_COLOR(v2);
		ffb->z    = FFB_GET_Z(v2);
		ffb->dmyf = FFB_GET_Y(v2);
		ffb->dmxf = FFB_GET_X(v2);
	}

	fmesa->ffbScreen->rp_active = 1;
}

static void TAG(ffb_vb_poly)(GLcontext *ctx, GLuint start, GLuint count, GLuint flags)
{
	GLint i;
	IMPL_LOCAL_VARS;

#ifdef FFB_RENDER_TRACE
	fprintf(stderr, "%s: start(%d) count(%d) flags(%x)\n",
		__FUNCTION__, start, count, flags);
#endif
	ffbRenderPrimitive(ctx, GL_POLYGON);

	/* XXX Optimize XXX */
	for (i = start + 2; i < count; i++) {
		ffb_vertex *v0 = &fmesa->verts[ELT(i - 1)];
		ffb_vertex *v1 = &fmesa->verts[ELT(i)];
		ffb_vertex *v2 = &fmesa->verts[ELT(start)];
		FFB_AREA_DECLARE

		FFB_COMPUTE_AREA_TRI(v0, v1, v2);
		FFB_CULL_TRI(continue;);

		FFBFifo(fmesa, (FFB_PRIM_COLOR_COST +
				(FFB_VERTEX_COLOR_COST * 3) + 9));
		FFB_SET_PRIM_COLOR(v2);

		FFB_SET_VERTEX_COLOR(v0);
		ffb->z   = FFB_GET_Z(v0);
		ffb->ryf = FFB_GET_Y(v0);
		ffb->rxf = FFB_GET_X(v0);

		FFB_SET_VERTEX_COLOR(v1);
		ffb->z   = FFB_GET_Z(v1);
		ffb->y   = FFB_GET_Y(v1);
		ffb->x   = FFB_GET_X(v1);

		FFB_SET_VERTEX_COLOR(v2);
		ffb->z   = FFB_GET_Z(v2);
		ffb->y   = FFB_GET_Y(v2);
		ffb->x   = FFB_GET_X(v2);
	}

	fmesa->ffbScreen->rp_active = 1;
}

static void TAG(ffb_vb_quads)(GLcontext *ctx, GLuint start, GLuint count, GLuint flags)
{
	GLint i;
	IMPL_LOCAL_VARS;

#ifdef FFB_RENDER_TRACE
	fprintf(stderr, "%s: start(%d) count(%d) flags(%x)\n",
		__FUNCTION__, start, count, flags);
#endif
	ffbRenderPrimitive(ctx, GL_QUADS);

	for (i = start + 3; i < count; i += 4) {
		ffb_vertex *v0 = &fmesa->verts[ELT(i - 3)];
		ffb_vertex *v1 = &fmesa->verts[ELT(i - 2)];
		ffb_vertex *v2 = &fmesa->verts[ELT(i - 1)];
		ffb_vertex *v3 = &fmesa->verts[ELT(i - 0)];
		FFB_AREA_DECLARE

		FFB_COMPUTE_AREA_QUAD(v0, v1, v2, v3);
		FFB_CULL_QUAD(continue;);

		FFBFifo(fmesa, (FFB_PRIM_COLOR_COST +
				(FFB_VERTEX_COLOR_COST * 4) + 12));
		FFB_SET_PRIM_COLOR(v3);

		FFB_SET_VERTEX_COLOR(v0);
		ffb->z    = FFB_GET_Z(v0);
		ffb->ryf  = FFB_GET_Y(v0);
		ffb->rxf  = FFB_GET_X(v0);

		FFB_SET_VERTEX_COLOR(v1);
		ffb->z    = FFB_GET_Z(v1);
		ffb->y    = FFB_GET_Y(v1);
		ffb->x    = FFB_GET_X(v1);

		FFB_SET_VERTEX_COLOR(v2);
		ffb->z    = FFB_GET_Z(v2);
		ffb->y    = FFB_GET_Y(v2);
		ffb->x    = FFB_GET_X(v2);

		FFB_SET_VERTEX_COLOR(v3);
		ffb->z    = FFB_GET_Z(v3);
		ffb->dmyf = FFB_GET_Y(v3);
		ffb->dmxf = FFB_GET_X(v3);
	}

	fmesa->ffbScreen->rp_active = 1;
}

static void TAG(ffb_vb_quad_strip)(GLcontext *ctx, GLuint start, GLuint count, GLuint flags)
{
	GLint i;
	IMPL_LOCAL_VARS;

#ifdef FFB_RENDER_TRACE
	fprintf(stderr, "%s: start(%d) count(%d) flags(%x)\n",
		__FUNCTION__, start, count, flags);
#endif
	ffbRenderPrimitive(ctx, GL_QUAD_STRIP);

	/* XXX Optimize XXX */
	for (i = start + 3; i < count; i += 2) {
		ffb_vertex *v0 = &fmesa->verts[ELT(i - 1)];
		ffb_vertex *v1 = &fmesa->verts[ELT(i - 3)];
		ffb_vertex *v2 = &fmesa->verts[ELT(i - 2)];
		ffb_vertex *v3 = &fmesa->verts[ELT(i - 0)];
		FFB_AREA_DECLARE

		FFB_COMPUTE_AREA_QUAD(v0, v1, v2, v3);
		FFB_CULL_QUAD(continue;);

		FFBFifo(fmesa, (FFB_PRIM_COLOR_COST +
				(FFB_VERTEX_COLOR_COST * 4) + 12));
		FFB_SET_PRIM_COLOR(v3);

		FFB_DUMP_VERTEX(v0);
		FFB_SET_VERTEX_COLOR(v0);
		ffb->z    = FFB_GET_Z(v0);
		ffb->ryf  = FFB_GET_Y(v0);
		ffb->rxf  = FFB_GET_X(v0);

		FFB_DUMP_VERTEX(v1);
		FFB_SET_VERTEX_COLOR(v1);
		ffb->z    = FFB_GET_Z(v1);
		ffb->y    = FFB_GET_Y(v1);
		ffb->x    = FFB_GET_X(v1);

		FFB_DUMP_VERTEX(v2);
		FFB_SET_VERTEX_COLOR(v2);
		ffb->z    = FFB_GET_Z(v2);
		ffb->y    = FFB_GET_Y(v2);
		ffb->x    = FFB_GET_X(v2);

		FFB_DUMP_VERTEX(v3);
		FFB_SET_VERTEX_COLOR(v3);
		ffb->z    = FFB_GET_Z(v3);
		ffb->dmyf = FFB_GET_Y(v3);
		ffb->dmxf = FFB_GET_X(v3);
	}

	fmesa->ffbScreen->rp_active = 1;
}

static void (*TAG(render_tab)[GL_POLYGON + 2])(GLcontext *, GLuint, GLuint, GLuint) =
{
#if !(IND & (FFB_TRI_CULL_BIT))
	TAG(ffb_vb_points),
	TAG(ffb_vb_lines),
	TAG(ffb_vb_line_loop),
	TAG(ffb_vb_line_strip),
#else
	NULL,
	NULL,
	NULL,
	NULL,
#endif
	TAG(ffb_vb_triangles),
	TAG(ffb_vb_tri_strip),
	TAG(ffb_vb_tri_fan),
	TAG(ffb_vb_quads),
	TAG(ffb_vb_quad_strip),
	TAG(ffb_vb_poly),
	ffb_vb_noop,
};

#undef IND
#undef TAG

#undef IMPL_LOCAL_VARS
#undef FFB_DECLARE_CACHED_COLOR
#undef FFB_COMPUTE_CACHED_COLOR
#undef FFB_CACHED_COLOR_SAME
#undef FFB_CACHED_COLOR_SET
#undef FFB_CACHED_COLOR_UPDATE
#undef FFB_SET_PRIM_COLOR
#undef FFB_PRIM_COLOR_COST
#undef FFB_SET_VERTEX_COLOR
#undef FFB_VERTEX_COLOR_COST
#undef RESET_STIPPLE
#undef FFB_AREA_DECLARE
#undef FFB_COMPUTE_AREA_TRI
#undef FFB_COMPUTE_AREA_QUAD
#undef FFB_CULL_TRI
#undef FFB_CULL_QUAD

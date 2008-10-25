/* $XFree86: xc/lib/GL/mesa/src/drv/ffb/ffb_vbtmp.h,v 1.1 2002/02/22 21:32:59 dawes Exp $ */

static void TAG(emit)(GLcontext *ctx, GLuint start, GLuint end)
{
	ffbContextPtr fmesa = FFB_CONTEXT(ctx);
#if defined(VB_DEBUG) || (IND & (FFB_VB_XYZ_BIT | FFB_VB_RGBA_BIT))
	struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;
#endif
#if (IND & (FFB_VB_RGBA_BIT))
	GLfloat (*col0)[4];
	GLuint col0_stride;
#if (IND & (FFB_VB_TWOSIDE_BIT))
	GLfloat (*col1)[4];
	GLuint col1_stride;
#endif
#endif
#if (IND & FFB_VB_XYZ_BIT)
	GLfloat (*proj)[4] = VB->NdcPtr->data;
	GLuint proj_stride = VB->NdcPtr->stride;
	const GLubyte *mask = VB->ClipMask;
#endif
	ffb_vertex *v = &fmesa->verts[start];
	int i;

#ifdef VB_DEBUG
	fprintf(stderr, "FFB: ffb_emit ["
#if (IND & (FFB_VB_XYZ_BIT))
		" XYZ"
#endif
#if (IND & (FFB_VB_RGBA_BIT))
		" RGBA"
#endif
#if (IND & (FFB_VB_TWOSIDE_BIT))
		" TWOSIDE"
#endif
		"] start(%d) end(%d) import(%d)\n",
		start, end,
		VB->importable_data);
#endif

#if (IND & (FFB_VB_RGBA_BIT))
	col0 = VB->ColorPtr[0]->data;
	col0_stride = VB->ColorPtr[0]->stride;
#if (IND & (FFB_VB_TWOSIDE_BIT))
	col1 = VB->ColorPtr[1]->data;
	col1_stride = VB->ColorPtr[1]->stride;
#endif
#endif

        {
		if (start) {
#if (IND & (FFB_VB_XYZ_BIT))
			proj = (GLfloat (*)[4])((GLubyte *)proj + start * proj_stride);
#endif
#if (IND & (FFB_VB_RGBA_BIT))
			col0 = (GLfloat (*)[4])((GLubyte *)col0 + start * col0_stride);
#if (IND & (FFB_VB_TWOSIDE_BIT))
			col1 = (GLfloat (*)[4])((GLubyte *)col1 + start * col1_stride);
#endif
#endif
		}
		for (i = start; i < end; i++, v++) {
#if (IND & (FFB_VB_XYZ_BIT))
			if (mask[i] == 0) {
				v->x = proj[0][0];
				v->y = proj[0][1];
				v->z = proj[0][2];
			}
			proj = (GLfloat (*)[4])((GLubyte *)proj + proj_stride);
#endif
#if (IND & (FFB_VB_RGBA_BIT))
			v->color[0].alpha = CLAMP(col0[0][3], 0.0f, 1.0f);
			v->color[0].red   = CLAMP(col0[0][0], 0.0f, 1.0f);
			v->color[0].green = CLAMP(col0[0][1], 0.0f, 1.0f);
			v->color[0].blue  = CLAMP(col0[0][2], 0.0f, 1.0f);
			col0 = (GLfloat (*)[4])((GLubyte *)col0 + col0_stride);
#if (IND & (FFB_VB_TWOSIDE_BIT))
			v->color[1].alpha = CLAMP(col1[0][3], 0.0f, 1.0f);
			v->color[1].red   = CLAMP(col1[0][0], 0.0f, 1.0f);
			v->color[1].green = CLAMP(col1[0][1], 0.0f, 1.0f);
			v->color[1].blue  = CLAMP(col1[0][2], 0.0f, 1.0f);
			col1 = (GLfloat (*)[4])((GLubyte *)col1 + col1_stride);
#endif
#endif
		}
	}
}

static void TAG(interp)(GLcontext *ctx, GLfloat t,
			GLuint edst, GLuint eout, GLuint ein,
			GLboolean force_boundary)
{
#if (IND & (FFB_VB_XYZ_BIT | FFB_VB_RGBA_BIT))
	ffbContextPtr fmesa = FFB_CONTEXT(ctx);
#endif
#if (IND & (FFB_VB_XYZ_BIT))
	struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;
	const GLfloat *dstclip = VB->ClipPtr->data[edst];
	GLfloat oow = 1.0 / dstclip[3];
#endif
#if (IND & (FFB_VB_XYZ_BIT | FFB_VB_RGBA_BIT))
	ffb_vertex *dst = &fmesa->verts[edst];
#endif
#if (IND & (FFB_VB_RGBA_BIT))
	ffb_vertex *in = &fmesa->verts[eout];
	ffb_vertex *out = &fmesa->verts[ein];
#endif

#ifdef VB_DEBUG
	fprintf(stderr, "FFB: ffb_interp ["
#if (IND & (FFB_VB_XYZ_BIT))
		" XYZ"
#endif
#if (IND & (FFB_VB_RGBA_BIT))
		" RGBA"
#endif
#if (IND & (FFB_VB_TWOSIDE_BIT))
		" TWOSIDE"
#endif
		"] edst(%d) eout(%d) ein(%d)\n",
		edst, eout, ein);
#endif

#if (IND & (FFB_VB_XYZ_BIT))
	dst->x = dstclip[0] * oow;
	dst->y = dstclip[1] * oow;
	dst->z = dstclip[2] * oow;
#endif

#if (IND & (FFB_VB_RGBA_BIT))
	INTERP_F(t, dst->color[0].alpha, out->color[0].alpha, in->color[0].alpha);
	INTERP_F(t, dst->color[0].red,   out->color[0].red,   in->color[0].red);
	INTERP_F(t, dst->color[0].green, out->color[0].green, in->color[0].green);
	INTERP_F(t, dst->color[0].blue,  out->color[0].blue,  in->color[0].blue);
#if (IND & (FFB_VB_TWOSIDE_BIT))
	INTERP_F(t, dst->color[1].alpha, out->color[1].alpha, in->color[1].alpha);
	INTERP_F(t, dst->color[1].red,   out->color[1].red,   in->color[1].red);
	INTERP_F(t, dst->color[1].green, out->color[1].green, in->color[1].green);
	INTERP_F(t, dst->color[1].blue,  out->color[1].blue,  in->color[1].blue);
#endif
#endif
}

static void TAG(init)(void)
{
	setup_tab[IND].emit = TAG(emit);
	setup_tab[IND].interp = TAG(interp);
}

#undef IND
#undef TAG

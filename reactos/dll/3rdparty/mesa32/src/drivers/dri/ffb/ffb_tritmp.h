/* $XFree86: xc/lib/GL/mesa/src/drv/ffb/ffb_tritmp.h,v 1.2 2002/02/22 21:32:59 dawes Exp $ */

static void TAG(ffb_triangle)( GLcontext *ctx, 	
			       ffb_vertex *v0,
			       ffb_vertex *v1,
			       ffb_vertex *v2 )
{
	ffbContextPtr fmesa = FFB_CONTEXT(ctx);
	ffb_fbcPtr ffb = fmesa->regs;
#if (IND & FFB_TRI_FLAT_BIT)
	GLuint const_fg;
#endif
	FFB_DELAYED_VIEWPORT_VARS;

#ifdef TRI_DEBUG
	fprintf(stderr, "FFB: ffb_triangle ["
#if (IND & FFB_TRI_CULL_BIT)
		" CULL"
#endif
#if (IND & FFB_TRI_FLAT_BIT)
		" FLAT"
#endif
#if (IND & FFB_TRI_ALPHA_BIT)
		" ALPHA"
#endif
		" ]\n");
#endif

#if (IND & FFB_TRI_CULL_BIT)
	{	/* NOTE: These are not viewport transformed yet. */
		GLfloat ex = v1->x - v0->x;
		GLfloat ey = v1->y - v0->y;
		GLfloat fx = v2->x - v0->x;
		GLfloat fy = v2->y - v0->y;
		GLfloat c = ex*fy-ey*fx;

		/* Culled... */
		if (c * fmesa->backface_sign > fmesa->ffb_zero)
			return;
	}
#endif

#if (IND & FFB_TRI_FLAT_BIT)
	const_fg = FFB_PACK_CONST_UBYTE_ARGB_COLOR( v2->color[0] );
#ifdef TRI_DEBUG
	fprintf(stderr, "FFB_tri: const_fg %08x (B[%f] G[%f] R[%f])\n",
		const_fg,
		FFB_2_30_FIXED_TO_FLOAT(v2->color[0].blue),
		FFB_2_30_FIXED_TO_FLOAT(v2->color[0].green),
		FFB_2_30_FIXED_TO_FLOAT(v2->color[0].red));
#endif
#endif


#if (IND & FFB_TRI_FLAT_BIT)
	FFBFifo(fmesa, 1);
	ffb->fg = const_fg;
#endif

#if (IND & FFB_TRI_FLAT_BIT)
	FFBFifo(fmesa, 9);
#else
#if (IND & FFB_TRI_ALPHA_BIT)
	FFBFifo(fmesa, 21);
#else
	FFBFifo(fmesa, 18);
#endif
#endif

	FFB_DUMP_VERTEX(v0);
#if !(IND & FFB_TRI_FLAT_BIT)
#if (IND & FFB_TRI_ALPHA_BIT)
	ffb->alpha = FFB_GET_ALPHA(v0);
#endif
	ffb->red   = FFB_GET_RED(v0);
	ffb->green = FFB_GET_GREEN(v0);
	ffb->blue  = FFB_GET_BLUE(v0);
#endif
	ffb->z     = FFB_GET_Z(v0);
	ffb->ryf   = FFB_GET_Y(v0);
	ffb->rxf   = FFB_GET_X(v0);

	FFB_DUMP_VERTEX(v1);
#if !(IND & FFB_TRI_FLAT_BIT)
#if (IND & FFB_TRI_ALPHA_BIT)
	ffb->alpha = FFB_GET_ALPHA(v1);
#endif
	ffb->red   = FFB_GET_RED(v1);
	ffb->green = FFB_GET_GREEN(v1);
	ffb->blue  = FFB_GET_BLUE(v1);
#endif
	ffb->z     = FFB_GET_Z(v1);
	ffb->y     = FFB_GET_Y(v1);
	ffb->x     = FFB_GET_X(v1);

	FFB_DUMP_VERTEX(v2);
#if !(IND & FFB_TRI_FLAT_BIT)
#if (IND & FFB_TRI_ALPHA_BIT)
	ffb->alpha = FFB_GET_ALPHA(v2);
#endif
	ffb->red   = FFB_GET_RED(v2);
	ffb->green = FFB_GET_GREEN(v2);
	ffb->blue  = FFB_GET_BLUE(v2);
#endif
	ffb->z     = FFB_GET_Z(v2);
	ffb->y     = FFB_GET_Y(v2);
	ffb->x     = FFB_GET_X(v2);

	fmesa->ffbScreen->rp_active = 1;
}


static void TAG(ffb_quad)(GLcontext *ctx, 			      
			  ffb_vertex *v0,
			  ffb_vertex *v1,
			  ffb_vertex *v2,
			  ffb_vertex *v3 )
{
	ffbContextPtr fmesa = FFB_CONTEXT(ctx);
	ffb_fbcPtr ffb = fmesa->regs;
#if (IND & FFB_TRI_FLAT_BIT)
	GLuint const_fg;
#endif
	FFB_DELAYED_VIEWPORT_VARS;

#ifdef TRI_DEBUG
	fprintf(stderr, "FFB: ffb_quad ["
#if (IND & FFB_TRI_CULL_BIT)
		" CULL"
#endif
#if (IND & FFB_TRI_FLAT_BIT)
		" FLAT"
#endif
#if (IND & FFB_TRI_ALPHA_BIT)
		" ALPHA"
#endif
		" ]\n");
#endif /* TRI_DEBUG */

#if (IND & FFB_TRI_CULL_BIT)
	{	/* NOTE: These are not viewport transformed yet. */
		GLfloat ex = v2->x - v0->x;
		GLfloat ey = v2->y - v0->y;
		GLfloat fx = v3->x - v1->x;
		GLfloat fy = v3->y - v1->y;
		GLfloat c = ex*fy-ey*fx;

		/* Culled... */
		if (c * fmesa->backface_sign > fmesa->ffb_zero)
			return;
	}
#endif

#if (IND & FFB_TRI_FLAT_BIT)
	const_fg = FFB_PACK_CONST_UBYTE_ARGB_COLOR( v3->color[0] );
#ifdef TRI_DEBUG
	fprintf(stderr, "FFB_quad: const_fg %08x (B[%f] G[%f] R[%f])\n",
		const_fg,
		FFB_2_30_FIXED_TO_FLOAT(v3->color[0].blue),
		FFB_2_30_FIXED_TO_FLOAT(v3->color[0].green),
		FFB_2_30_FIXED_TO_FLOAT(v3->color[0].red));
#endif
#endif


#if (IND & FFB_TRI_FLAT_BIT)
	FFBFifo(fmesa, 13);
	ffb->fg = const_fg;
#else
#if (IND & FFB_TRI_ALPHA_BIT)
	FFBFifo(fmesa, 28);
#else
	FFBFifo(fmesa, 24);
#endif
#endif

	FFB_DUMP_VERTEX(v0);
#if !(IND & FFB_TRI_FLAT_BIT)
#if (IND & FFB_TRI_ALPHA_BIT)
	ffb->alpha = FFB_GET_ALPHA(v0);
#endif
	ffb->red   = FFB_GET_RED(v0);
	ffb->green = FFB_GET_GREEN(v0);
	ffb->blue  = FFB_GET_BLUE(v0);
#endif
	ffb->z     = FFB_GET_Z(v0);
	ffb->ryf   = FFB_GET_Y(v0);
	ffb->rxf   = FFB_GET_X(v0);

	FFB_DUMP_VERTEX(v1);
#if !(IND & FFB_TRI_FLAT_BIT)
#if (IND & FFB_TRI_ALPHA_BIT)
	ffb->alpha = FFB_GET_ALPHA(v1);
#endif
	ffb->red   = FFB_GET_RED(v1);
	ffb->green = FFB_GET_GREEN(v1);
	ffb->blue  = FFB_GET_BLUE(v1);
#endif
	ffb->z     = FFB_GET_Z(v1);
	ffb->y     = FFB_GET_Y(v1);
	ffb->x     = FFB_GET_X(v1);

	FFB_DUMP_VERTEX(v2);
#if !(IND & FFB_TRI_FLAT_BIT)
#if (IND & FFB_TRI_ALPHA_BIT)
	ffb->alpha = FFB_GET_ALPHA(v2);
#endif
	ffb->red   = FFB_GET_RED(v2);
	ffb->green = FFB_GET_GREEN(v2);
	ffb->blue  = FFB_GET_BLUE(v2);
#endif
	ffb->z     = FFB_GET_Z(v2);
	ffb->y     = FFB_GET_Y(v2);
	ffb->x     = FFB_GET_X(v2);

	FFB_DUMP_VERTEX(v3);
#if !(IND & FFB_TRI_FLAT_BIT)
#if (IND & FFB_TRI_ALPHA_BIT)
	ffb->alpha = FFB_GET_ALPHA(v3);
#endif
	ffb->red   = FFB_GET_RED(v3);
	ffb->green = FFB_GET_GREEN(v3);
	ffb->blue  = FFB_GET_BLUE(v3);
#endif
	ffb->z     = FFB_GET_Z(v3);
	ffb->dmyf  = FFB_GET_Y(v3);
	ffb->dmxf  = FFB_GET_X(v3);

	fmesa->ffbScreen->rp_active = 1;
}

static void TAG(ffb_init)(void)
{
	ffb_tri_tab[IND]	= TAG(ffb_triangle);
	ffb_quad_tab[IND]	= TAG(ffb_quad);
}

#undef IND
#undef TAG

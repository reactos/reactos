/* $XFree86: xc/lib/GL/mesa/src/drv/ffb/ffb_linetmp.h,v 1.2 2002/02/22 21:32:58 dawes Exp $ */

static __inline void TAG(ffb_line)(GLcontext *ctx, ffb_vertex *v0, 
				   ffb_vertex *v1 )
{
	ffbContextPtr fmesa = FFB_CONTEXT(ctx);
	ffb_fbcPtr ffb = fmesa->regs;
#if (IND & FFB_LINE_FLAT_BIT)
	const GLuint const_fg = FFB_PACK_CONST_UBYTE_ARGB_COLOR( v1->color[0] );
#endif
	FFB_DELAYED_VIEWPORT_VARS;

#ifdef FFB_LINE_TRACE
	fprintf(stderr, "FFB: ffb_line ["
#if (IND & FFB_LINE_FLAT_BIT)
		" FLAT"
#endif
#if (IND & FFB_LINE_ALPHA_BIT)
		" ALPHA"
#endif
		" ]\n");
#endif

#if (IND & FFB_LINE_FLAT_BIT)
	FFBFifo(fmesa, 1);
	ffb->fg = const_fg;
#ifdef FFB_LINE_TRACE
	fprintf(stderr, "FFB: ffb_line confg_fg[%08x]\n", const_fg);
#endif
#endif

#if (IND & FFB_LINE_FLAT_BIT)
	/* (2 * 3) + 1 */
	FFBFifo(fmesa, 7);
#else
#if (IND & FFB_LINE_ALPHA_BIT)
	/* (2 * 7) + 1 */
	FFBFifo(fmesa, 15);
#else
	/* (2 * 6) + 1 */
	FFBFifo(fmesa, 13);
#endif
#endif

	/* Using DDLINE or AALINE, init the line pattern state. */
	ffb->lpat = fmesa->lpat;

#if !(IND & FFB_LINE_FLAT_BIT)
#if (IND & FFB_LINE_ALPHA_BIT)
	ffb->alpha	= FFB_GET_ALPHA(v0);
#endif
	ffb->red	= FFB_GET_RED(v0);
	ffb->green	= FFB_GET_GREEN(v0);
	ffb->blue	= FFB_GET_BLUE(v0);
#endif
	ffb->z		= FFB_GET_Z(v0);
	ffb->ryf	= FFB_GET_Y(v0);
	ffb->rxf	= FFB_GET_X(v0);

#if !(IND & FFB_LINE_FLAT_BIT)
#if (IND & FFB_LINE_ALPHA_BIT)
	ffb->alpha	= FFB_GET_ALPHA(v1);
#endif
	ffb->red	= FFB_GET_RED(v1);
	ffb->green	= FFB_GET_GREEN(v1);
	ffb->blue	= FFB_GET_BLUE(v1);
#endif
	ffb->z		= FFB_GET_Z(v1);
	ffb->y		= FFB_GET_Y(v1);
	ffb->x		= FFB_GET_X(v1);

	fmesa->ffbScreen->rp_active = 1;
}

static void TAG(init)(void)
{
	ffb_line_tab[IND] = TAG(ffb_line);
}

#undef IND
#undef TAG

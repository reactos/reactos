/* $XFree86: xc/lib/GL/mesa/src/drv/ffb/ffb_pointtmp.h,v 1.3 2002/02/22 21:32:59 dawes Exp $ */

static __inline void TAG(ffb_draw_point)(GLcontext *ctx, ffb_vertex *tmp )
{
	ffbContextPtr fmesa = FFB_CONTEXT(ctx);
	ffb_fbcPtr ffb = fmesa->regs;
	FFB_DELAYED_VIEWPORT_VARS;

#ifdef FFB_POINT_TRACE
	fprintf(stderr, "FFB: ffb_point ["
#if (IND & FFB_POINT_AA_BIT)
		"AA"
#endif
		"] X(%f) Y(%f) Z(%f)\n",
		tmp->x, tmp->y, tmp->z);
#endif

#if (IND & FFB_POINT_AA_BIT)
	FFBFifo(fmesa, 4);
			
	ffb->fg = FFB_PACK_CONST_UBYTE_ARGB_COLOR( tmp->color[0] );
	ffb->z = FFB_GET_Z(tmp);
	ffb->y = FFB_GET_Y(tmp) + 0x8000 /* FIX ME */;
	ffb->x = FFB_GET_X(tmp) + 0x8000 /* FIX ME */;
#else
	{
		unsigned int const_fg, const_z, h, w;

		const_fg = FFB_PACK_CONST_UBYTE_ARGB_COLOR( tmp->color[0] );
		const_z = Z_FROM_MESA(FFB_Z_TO_FLOAT(FFB_GET_Z(tmp)));
		h = FFB_GET_Y(tmp) >> 16;
		w = FFB_GET_X(tmp) >> 16;
#ifdef FFB_POINT_TRACE
		fprintf(stderr, "FFB: ffb_point fg(%08x) z(%08x) h(%08x) w(%08x)\n",
			const_fg, const_z, h, w);
#endif
		FFBFifo(fmesa, 4);
		ffb->fg = const_fg;
		ffb->constz = const_z;
		ffb->bh = h;
		ffb->bw = w;
	}
#endif

	fmesa->ffbScreen->rp_active = 1;
}


static void TAG(init)(void)
{
	ffb_point_tab[IND] = TAG(ffb_draw_point);
}

#undef IND
#undef TAG

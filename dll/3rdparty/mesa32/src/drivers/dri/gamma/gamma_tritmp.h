/* $XFree86: xc/extras/Mesa/src/mesa/drivers/dri/gamma/gamma_tritmp.h,v 1.2 2004/12/13 22:40:49 tsi Exp $ */

static void TAG(gamma_point)( gammaContextPtr gmesa, 
			     const gammaVertex *v0 )
{
    u_int32_t vColor;
    u_int32_t vBegin;

    vBegin = gmesa->Begin | B_PrimType_Points;

    CHECK_DMA_BUFFER(gmesa, 1);
    WRITE(gmesa->buf, Begin, vBegin);

#if (IND & GAMMA_RAST_ALPHA_BIT)
    vColor = (v0->v.color.alpha << 24) |
	     (v0->v.color.blue  << 16) |
	     (v0->v.color.green <<  8) |
	     (v0->v.color.red   <<  0);

    CHECK_DMA_BUFFER(gmesa, 1);
    WRITE(gmesa->buf, PackedColor4, vColor);
#else
    vColor = (v0->v.color.blue  << 16) |
	     (v0->v.color.green <<  8) |
	     (v0->v.color.red   <<  0);

    CHECK_DMA_BUFFER(gmesa, 1);
    WRITE(gmesa->buf, PackedColor3, vColor);
#endif

#if (IND & GAMMA_RAST_TEX_BIT)
    CHECK_DMA_BUFFER(gmesa, 6);
    WRITEF(gmesa->buf, Tt2, v0->v.u0);
    WRITEF(gmesa->buf, Ts2, v0->v.v0);
    WRITEF(gmesa->buf, Vw, v0->v.w);
    WRITEF(gmesa->buf, Vz, v0->v.z);
    WRITEF(gmesa->buf, Vy, v0->v.y);
    WRITEF(gmesa->buf, Vx4, v0->v.x);
#else
    CHECK_DMA_BUFFER(gmesa, 4);
    WRITEF(gmesa->buf, Vw, v0->v.w);
    WRITEF(gmesa->buf, Vz, v0->v.z);
    WRITEF(gmesa->buf, Vy, v0->v.y);
    WRITEF(gmesa->buf, Vx4, v0->v.x);
#endif

#if !(IND & GAMMA_RAST_FLAT_BIT)
    CHECK_DMA_BUFFER(gmesa, 1);
    WRITE(gmesa->buf, FlushSpan, 0);
#endif

    CHECK_DMA_BUFFER(gmesa, 1);
    WRITE(gmesa->buf, End, 0);
}

static void TAG(gamma_line)( gammaContextPtr gmesa, 
			     const gammaVertex *v0,
			     const gammaVertex *v1 )
{
    u_int32_t vColor;
    u_int32_t vBegin;

    vBegin = gmesa->Begin | B_PrimType_Lines;

    CHECK_DMA_BUFFER(gmesa, 1);
    WRITE(gmesa->buf, Begin, vBegin);

#if !(IND & GAMMA_RAST_FLAT_BIT)
#if (IND & GAMMA_RAST_ALPHA_BIT)
    vColor = (v0->v.color.alpha << 24) |
	     (v0->v.color.blue  << 16) |
	     (v0->v.color.green <<  8) |
	     (v0->v.color.red   <<  0);

    CHECK_DMA_BUFFER(gmesa, 1);
    WRITE(gmesa->buf, PackedColor4, vColor);
#else
    vColor = (v0->v.color.blue  << 16) |
	     (v0->v.color.green <<  8) |
	     (v0->v.color.red   <<  0);

    CHECK_DMA_BUFFER(gmesa, 1);
    WRITE(gmesa->buf, PackedColor3, vColor);
#endif
#else
#if (IND & GAMMA_RAST_ALPHA_BIT)
    vColor = (v1->v.color.alpha << 24) |
	     (v1->v.color.blue  << 16) |
	     (v1->v.color.green <<  8) |
	     (v1->v.color.red   <<  0);

    CHECK_DMA_BUFFER(gmesa, 1);
    WRITE(gmesa->buf, PackedColor4, vColor);
#else
    vColor = (v1->v.color.blue  << 16) |
	     (v1->v.color.green <<  8) |
	     (v1->v.color.red   <<  0);

    CHECK_DMA_BUFFER(gmesa, 1);
    WRITE(gmesa->buf, PackedColor3, vColor);
#endif
#endif

#if (IND & GAMMA_RAST_TEX_BIT)
    CHECK_DMA_BUFFER(gmesa, 6);
    WRITEF(gmesa->buf, Tt2, v0->v.u0);
    WRITEF(gmesa->buf, Ts2, v0->v.v0);
    WRITEF(gmesa->buf, Vw, v0->v.w);
    WRITEF(gmesa->buf, Vz, v0->v.z);
    WRITEF(gmesa->buf, Vy, v0->v.y);
    WRITEF(gmesa->buf, Vx4, v0->v.x);
#else
    CHECK_DMA_BUFFER(gmesa, 4);
    WRITEF(gmesa->buf, Vw, v0->v.w);
    WRITEF(gmesa->buf, Vz, v0->v.z);
    WRITEF(gmesa->buf, Vy, v0->v.y);
    WRITEF(gmesa->buf, Vx4, v0->v.x);
#endif

#if !(IND & GAMMA_RAST_FLAT_BIT)
#if (IND & GAMMA_RAST_ALPHA_BIT)
    vColor = (v1->v.color.alpha << 24) |
	     (v1->v.color.blue  << 16) |
	     (v1->v.color.green <<  8) |
	     (v1->v.color.red   <<  0);

    CHECK_DMA_BUFFER(gmesa, 1);
    WRITE(gmesa->buf, PackedColor4, vColor);
#else
    vColor = (v1->v.color.blue  << 16) |
	     (v1->v.color.green <<  8) |
	     (v1->v.color.red   <<  0);

    CHECK_DMA_BUFFER(gmesa, 1);
    WRITE(gmesa->buf, PackedColor3, vColor);
#endif
#endif

#if (IND & GAMMA_RAST_TEX_BIT)
    CHECK_DMA_BUFFER(gmesa, 6);
    WRITEF(gmesa->buf, Tt2, v1->v.u0);
    WRITEF(gmesa->buf, Ts2, v1->v.v0);
    WRITEF(gmesa->buf, Vw, v1->v.w);
    WRITEF(gmesa->buf, Vz, v1->v.z);
    WRITEF(gmesa->buf, Vy, v1->v.y);
    WRITEF(gmesa->buf, Vx4, v1->v.x);
#else
    CHECK_DMA_BUFFER(gmesa, 4);
    WRITEF(gmesa->buf, Vw, v1->v.w);
    WRITEF(gmesa->buf, Vz, v1->v.z);
    WRITEF(gmesa->buf, Vy, v1->v.y);
    WRITEF(gmesa->buf, Vx4, v1->v.x);
#endif

#if !(IND & GAMMA_RAST_FLAT_BIT)
    CHECK_DMA_BUFFER(gmesa, 1);
    WRITE(gmesa->buf, FlushSpan, 0);
#endif

    CHECK_DMA_BUFFER(gmesa, 1);
    WRITE(gmesa->buf, End, 0);
}

static void TAG(gamma_triangle)( gammaContextPtr gmesa,
				 const gammaVertex *v0,
				 const gammaVertex *v1, 
				 const gammaVertex *v2 )
{
    u_int32_t vColor;
    u_int32_t vBegin;

    vBegin = gmesa->Begin | B_PrimType_Triangles;

    CHECK_DMA_BUFFER(gmesa, 1);
    WRITE(gmesa->buf, Begin, vBegin);

#if !(IND & GAMMA_RAST_FLAT_BIT)
#if (IND & GAMMA_RAST_ALPHA_BIT)
    vColor = (v0->v.color.alpha << 24) |
	     (v0->v.color.blue  << 16) |
	     (v0->v.color.green <<  8) |
	     (v0->v.color.red   <<  0);

    CHECK_DMA_BUFFER(gmesa, 1);
    WRITE(gmesa->buf, PackedColor4, vColor);
#else
    vColor = (v0->v.color.blue  << 16) |
	     (v0->v.color.green <<  8) |
	     (v0->v.color.red   <<  0);

    CHECK_DMA_BUFFER(gmesa, 1);
    WRITE(gmesa->buf, PackedColor3, vColor);
#endif
#else
#if (IND & GAMMA_RAST_ALPHA_BIT)
    vColor = (v2->v.color.alpha << 24) |
	     (v2->v.color.blue  << 16) |
	     (v2->v.color.green <<  8) |
	     (v2->v.color.red   <<  0);

    CHECK_DMA_BUFFER(gmesa, 1);
    WRITE(gmesa->buf, PackedColor4, vColor);
#else
    vColor = (v2->v.color.blue  << 16) |
	     (v2->v.color.green <<  8) |
	     (v2->v.color.red   <<  0);

    CHECK_DMA_BUFFER(gmesa, 1);
    WRITE(gmesa->buf, PackedColor3, vColor);
#endif
#endif

#if (IND & GAMMA_RAST_TEX_BIT)
    CHECK_DMA_BUFFER(gmesa, 6);
    WRITEF(gmesa->buf, Tt2, v0->v.u0);
    WRITEF(gmesa->buf, Ts2, v0->v.v0);
    WRITEF(gmesa->buf, Vw, v0->v.w);
    WRITEF(gmesa->buf, Vz, v0->v.z);
    WRITEF(gmesa->buf, Vy, v0->v.y);
    WRITEF(gmesa->buf, Vx4, v0->v.x);
#else
    CHECK_DMA_BUFFER(gmesa, 4);
    WRITEF(gmesa->buf, Vw, v0->v.w);
    WRITEF(gmesa->buf, Vz, v0->v.z);
    WRITEF(gmesa->buf, Vy, v0->v.y);
    WRITEF(gmesa->buf, Vx4, v0->v.x);
#endif

#if !(IND & GAMMA_RAST_FLAT_BIT)
#if (IND & GAMMA_RAST_ALPHA_BIT)
    vColor = (v1->v.color.alpha << 24) |
	     (v1->v.color.blue  << 16) |
	     (v1->v.color.green <<  8) |
	     (v1->v.color.red   <<  0);

    CHECK_DMA_BUFFER(gmesa, 1);
    WRITE(gmesa->buf, PackedColor4, vColor);
#else
    vColor = (v1->v.color.blue  << 16) |
	     (v1->v.color.green <<  8) |
	     (v1->v.color.red   <<  0);

    CHECK_DMA_BUFFER(gmesa, 1);
    WRITE(gmesa->buf, PackedColor3, vColor);
#endif
#endif

#if (IND & GAMMA_RAST_TEX_BIT)
    CHECK_DMA_BUFFER(gmesa, 6);
    WRITEF(gmesa->buf, Tt2, v1->v.u0);
    WRITEF(gmesa->buf, Ts2, v1->v.v0);
    WRITEF(gmesa->buf, Vw, v1->v.w);
    WRITEF(gmesa->buf, Vz, v1->v.z);
    WRITEF(gmesa->buf, Vy, v1->v.y);
    WRITEF(gmesa->buf, Vx4, v1->v.x);
#else
    CHECK_DMA_BUFFER(gmesa, 4);
    WRITEF(gmesa->buf, Vw, v1->v.w);
    WRITEF(gmesa->buf, Vz, v1->v.z);
    WRITEF(gmesa->buf, Vy, v1->v.y);
    WRITEF(gmesa->buf, Vx4, v1->v.x);
#endif

#if !(IND & GAMMA_RAST_FLAT_BIT)
#if (IND & GAMMA_RAST_ALPHA_BIT)
    vColor = (v2->v.color.alpha << 24) |
	     (v2->v.color.blue  << 16) |
	     (v2->v.color.green <<  8) |
	     (v2->v.color.red   <<  0);

    CHECK_DMA_BUFFER(gmesa, 1);
    WRITE(gmesa->buf, PackedColor4, vColor);
#else
    vColor = (v2->v.color.blue  << 16) |
	     (v2->v.color.green <<  8) |
	     (v2->v.color.red   <<  0);

    CHECK_DMA_BUFFER(gmesa, 1);
    WRITE(gmesa->buf, PackedColor3, vColor);
#endif
#endif

#if (IND & GAMMA_RAST_TEX_BIT)
    CHECK_DMA_BUFFER(gmesa, 6);
    WRITEF(gmesa->buf, Tt2, v2->v.u0);
    WRITEF(gmesa->buf, Ts2, v2->v.v0);
    WRITEF(gmesa->buf, Vw, v2->v.w);
    WRITEF(gmesa->buf, Vz, v2->v.z);
    WRITEF(gmesa->buf, Vy, v2->v.y);
    WRITEF(gmesa->buf, Vx4, v2->v.x);
#else
    CHECK_DMA_BUFFER(gmesa, 4);
    WRITEF(gmesa->buf, Vw, v2->v.w);
    WRITEF(gmesa->buf, Vz, v2->v.z);
    WRITEF(gmesa->buf, Vy, v2->v.y);
    WRITEF(gmesa->buf, Vx4, v2->v.x);
#endif

#if !(IND & GAMMA_RAST_FLAT_BIT)
    CHECK_DMA_BUFFER(gmesa, 1);
    WRITE(gmesa->buf, FlushSpan, 0);
#endif

    CHECK_DMA_BUFFER(gmesa, 1);
    WRITE(gmesa->buf, End, 0);
}

static void TAG(gamma_quad)( gammaContextPtr gmesa,
			    const gammaVertex *v0,
			    const gammaVertex *v1,
			    const gammaVertex *v2,
			    const gammaVertex *v3 )
{
    u_int32_t vColor;
    u_int32_t vBegin;

    vBegin = gmesa->Begin | B_PrimType_Quads;

    CHECK_DMA_BUFFER(gmesa, 1);
    WRITE(gmesa->buf, Begin, vBegin);

#if !(IND & GAMMA_RAST_FLAT_BIT)
#if (IND & GAMMA_RAST_ALPHA_BIT)
    vColor = (v0->v.color.alpha << 24) |
	     (v0->v.color.blue  << 16) |
	     (v0->v.color.green <<  8) |
	     (v0->v.color.red   <<  0);

    CHECK_DMA_BUFFER(gmesa, 1);
    WRITE(gmesa->buf, PackedColor4, vColor);
#else
    vColor = (v0->v.color.blue  << 16) |
	     (v0->v.color.green <<  8) |
	     (v0->v.color.red   <<  0);

    CHECK_DMA_BUFFER(gmesa, 1);
    WRITE(gmesa->buf, PackedColor3, vColor);
#endif
#else
#if (IND & GAMMA_RAST_ALPHA_BIT)
    vColor = (v3->v.color.alpha << 24) |
	     (v3->v.color.blue  << 16) |
	     (v3->v.color.green <<  8) |
	     (v3->v.color.red   <<  0);

    CHECK_DMA_BUFFER(gmesa, 1);
    WRITE(gmesa->buf, PackedColor4, vColor);
#else
    vColor = (v3->v.color.blue  << 16) |
	     (v3->v.color.green <<  8) |
	     (v3->v.color.red   <<  0);

    CHECK_DMA_BUFFER(gmesa, 1);
    WRITE(gmesa->buf, PackedColor3, vColor);
#endif
#endif

#if (IND & GAMMA_RAST_TEX_BIT)
    CHECK_DMA_BUFFER(gmesa, 6);
    WRITEF(gmesa->buf, Tt2, v0->v.u0);
    WRITEF(gmesa->buf, Ts2, v0->v.v0);
    WRITEF(gmesa->buf, Vw, v0->v.w);
    WRITEF(gmesa->buf, Vz, v0->v.z);
    WRITEF(gmesa->buf, Vy, v0->v.y);
    WRITEF(gmesa->buf, Vx4, v0->v.x);
#else
    CHECK_DMA_BUFFER(gmesa, 4);
    WRITEF(gmesa->buf, Vw, v0->v.w);
    WRITEF(gmesa->buf, Vz, v0->v.z);
    WRITEF(gmesa->buf, Vy, v0->v.y);
    WRITEF(gmesa->buf, Vx4, v0->v.x);
#endif

#if !(IND & GAMMA_RAST_FLAT_BIT)
#if (IND & GAMMA_RAST_ALPHA_BIT)
    vColor = (v1->v.color.alpha << 24) |
	     (v1->v.color.blue  << 16) |
	     (v1->v.color.green <<  8) |
	     (v1->v.color.red   <<  0);

    CHECK_DMA_BUFFER(gmesa, 1);
    WRITE(gmesa->buf, PackedColor4, vColor);
#else
    vColor = (v1->v.color.blue  << 16) |
	     (v1->v.color.green <<  8) |
	     (v1->v.color.red   <<  0);

    CHECK_DMA_BUFFER(gmesa, 1);
    WRITE(gmesa->buf, PackedColor3, vColor);
#endif
#endif

#if (IND & GAMMA_RAST_TEX_BIT)
    CHECK_DMA_BUFFER(gmesa, 6);
    WRITEF(gmesa->buf, Tt2, v1->v.u0);
    WRITEF(gmesa->buf, Ts2, v1->v.v0);
    WRITEF(gmesa->buf, Vw, v1->v.w);
    WRITEF(gmesa->buf, Vz, v1->v.z);
    WRITEF(gmesa->buf, Vy, v1->v.y);
    WRITEF(gmesa->buf, Vx4, v1->v.x);
#else
    CHECK_DMA_BUFFER(gmesa, 4);
    WRITEF(gmesa->buf, Vw, v1->v.w);
    WRITEF(gmesa->buf, Vz, v1->v.z);
    WRITEF(gmesa->buf, Vy, v1->v.y);
    WRITEF(gmesa->buf, Vx4, v1->v.x);
#endif

#if !(IND & GAMMA_RAST_FLAT_BIT)
#if (IND & GAMMA_RAST_ALPHA_BIT)
    vColor = (v2->v.color.alpha << 24) |
	     (v2->v.color.blue  << 16) |
	     (v2->v.color.green <<  8) |
	     (v2->v.color.red   <<  0);

    CHECK_DMA_BUFFER(gmesa, 1);
    WRITE(gmesa->buf, PackedColor4, vColor);
#else
    vColor = (v2->v.color.blue  << 16) |
	     (v2->v.color.green <<  8) |
	     (v2->v.color.red   <<  0);

    CHECK_DMA_BUFFER(gmesa, 1);
    WRITE(gmesa->buf, PackedColor3, vColor);
#endif
#endif

#if (IND & GAMMA_RAST_TEX_BIT)
    CHECK_DMA_BUFFER(gmesa, 6);
    WRITEF(gmesa->buf, Tt2, v2->v.u0);
    WRITEF(gmesa->buf, Ts2, v2->v.v0);
    WRITEF(gmesa->buf, Vw, v2->v.w);
    WRITEF(gmesa->buf, Vz, v2->v.z);
    WRITEF(gmesa->buf, Vy, v2->v.y);
    WRITEF(gmesa->buf, Vx4, v2->v.x);
#else
    CHECK_DMA_BUFFER(gmesa, 4);
    WRITEF(gmesa->buf, Vw, v2->v.w);
    WRITEF(gmesa->buf, Vz, v2->v.z);
    WRITEF(gmesa->buf, Vy, v2->v.y);
    WRITEF(gmesa->buf, Vx4, v2->v.x);
#endif

#if !(IND & GAMMA_RAST_FLAT_BIT)
#if (IND & GAMMA_RAST_ALPHA_BIT)
    vColor = (v3->v.color.alpha << 24) |
	     (v3->v.color.blue  << 16) |
	     (v3->v.color.green <<  8) |
	     (v3->v.color.red   <<  0);

    CHECK_DMA_BUFFER(gmesa, 1);
    WRITE(gmesa->buf, PackedColor4, vColor);
#else
    vColor = (v3->v.color.blue  << 16) |
	     (v3->v.color.green <<  8) |
	     (v3->v.color.red   <<  0);

    CHECK_DMA_BUFFER(gmesa, 1);
    WRITE(gmesa->buf, PackedColor3, vColor);
#endif
#endif

#if (IND & GAMMA_RAST_TEX_BIT)
    CHECK_DMA_BUFFER(gmesa, 6);
    WRITEF(gmesa->buf, Tt2, v3->v.u0);
    WRITEF(gmesa->buf, Ts2, v3->v.v0);
    WRITEF(gmesa->buf, Vw, v3->v.w);
    WRITEF(gmesa->buf, Vz, v3->v.z);
    WRITEF(gmesa->buf, Vy, v3->v.y);
    WRITEF(gmesa->buf, Vx4, v3->v.x);
#else
    CHECK_DMA_BUFFER(gmesa, 4);
    WRITEF(gmesa->buf, Vw, v3->v.w);
    WRITEF(gmesa->buf, Vz, v3->v.z);
    WRITEF(gmesa->buf, Vy, v3->v.y);
    WRITEF(gmesa->buf, Vx4, v3->v.x);
#endif

#if !(IND & GAMMA_RAST_FLAT_BIT)
    CHECK_DMA_BUFFER(gmesa, 1);
    WRITE(gmesa->buf, FlushSpan, 0);
#endif

    CHECK_DMA_BUFFER(gmesa, 1);
    WRITE(gmesa->buf, End, 0);
}

static void TAG(gamma_init)(void)
{
	gamma_point_tab[IND]	= TAG(gamma_point);
	gamma_line_tab[IND]	= TAG(gamma_line);
	gamma_tri_tab[IND]	= TAG(gamma_triangle);
	gamma_quad_tab[IND]	= TAG(gamma_quad);
}

#undef IND
#undef TAG

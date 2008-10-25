/*
 * Author: Max Lingua <sunmax@libero.it>
 */

/**** MACROS start ****/

/* point/line macros */

#define LINE_VERT_VARS \
        SWvertex v[3]; \
	s3vVertex* vvv[2]; \
        int x[3], y[3], z[3]; \
        int idx[3]; \
        int dx01, dy01; \
        int delt02; \
        int deltzy, zstart; \
        int start02, end01; \
        int ystart, y01y12; \
        int i, tmp, tmp2, tmp3; \
        GLfloat ydiff, fy[3]
#define LINE_VERT_VARS_VOIDS \
        (void) v; (void) vvv; (void) x; (void) y; (void) z; (void) idx; \
        (void) dx01; (void) dy01; (void) delt02; (void) deltzy; \
        (void) zstart; (void) start02; (void) ystart; (void) y01y12; \
        (void) i; (void) tmp; (void) tmp2; (void) tmp3; (void) ydiff; (void) fy

#define LINE_FLAT_VARS \
        int arstart, gbstart; \
        int deltarx, deltgbx, deltary, deltgby; \
        GLubyte *(col)[3]
#define LINE_FLAT_VARS_VOIDS \
        (void) arstart; (void) gbstart; (void) deltarx; (void) deltgbx; \
        (void) deltary; (void) deltgby; (void) col

#define LINE_GOURAUD_VARS \
        int arstart, gbstart; \
        int deltary, deltgby; \
        int ctmp, ctmp2, ctmp3, ctmp4; \
        GLubyte *(col)[3]
#define LINE_GOURAUD_VARS_VOIDS \
        (void) arstart; (void) gbstart; (void) deltary; (void) deltgby; \
        (void) ctmp; (void) ctmp2; (void) ctmp3; (void) ctmp4; (void) col

#define SORT_LINE_VERT() \
do { \
	if(v[0].win[1] <= v[1].win[1]) { \
\
                idx[0] = 0; \
                idx[1] = 1; \
\
        } else if (v[0].win[1] > v[1].win[1]) { \
\
                idx[0] = 1; \
                idx[1] = 0; \
\
        } \
} while(0)

#define SET_LINE_VERT() \
do { \
        x[0] = (v[idx[0]].win[0] * 1024.0f * 1024.0f); /* 0x100000 */ \
        y[0] = fy[0] = dPriv->h - v[idx[0]].win[1]; \
        z[0] = (v[idx[0]].win[2]) * 1024.0f * 32.0f; /* 0x8000; */ \
\
        x[1] = (v[idx[1]].win[0] * 1024.0f * 1024.0f); /* 0x100000 */ \
        y[1] = dPriv->h - v[idx[1]].win[1]; \
        z[1] = (v[idx[1]].win[2]) * 1024.0f * 32.0f; /* 0x8000 */ \
} while(0)

#define SET_LINE_XY() \
do { \
	tmp = v[idx[0]].win[0]; \
        tmp2 = v[idx[1]].win[0]; \
\
	dx01 = x[0] - x[1]; \
        dy01 = y[0] - y[1]; \
\
        ydiff = fy[0] - (float)y[0]; \
        ystart = y[0]; \
        y01y12 = dy01 + 1; \
} while (0)

#define SET_LINE_DIR() \
do { \
        if (tmp2 > tmp) { \
                y01y12 |= 0x80000000; \
                tmp3 = tmp2-tmp; \
        } else { \
                tmp3 = tmp-tmp2; \
        } \
\
        end01 = ((tmp << 16) | tmp2); \
\
	if (dy01) \
                delt02 = -(dx01/dy01); \
        else \
		delt02 = 0; \
\
        if (dy01 > tmp3) { /* Y MAJ */ \
	/* NOTE: tmp3 always >=0 */ \
                start02 = x[0]; \
        } else if (delt02 >= 0){ /* X MAJ - positive delta */ \
                start02 = x[0] + delt02/2; \
                dy01 = tmp3; /* could be 0 */ \
        } else { /* X MAJ - negative delta */ \
                start02 = x[0] + delt02/2 + ((1 << 20) - 1); \
                dy01 = tmp3; /* could be 0 */ \
        } \
} while(0)

#define SET_LINE_Z() \
do { \
	zstart = z[0]; \
\
	if (dy01) { \
                deltzy = (z[1] - z[0])/dy01; \
        } else { \
                deltzy = 0; /* dy01 = tmp3 = 0 (it's a point)*/ \
        } \
} while (0)

#define SET_LINE_FLAT_COL() \
do { \
        col[0] = &(v[idx[0]].color[0]); \
        deltarx = deltary = deltgbx = deltgby = 0; \
        gbstart = (((col[0][1]) << 23) | ((col[0][2]) << 7)); \
        arstart = (((col[0][3]) << 23) | ((col[0][0]) << 7)); \
} while(0)

#define SET_LINE_GOURAUD_COL() \
do { \
        col[0] = &(v[idx[0]].color[0]); \
        col[1] = &(v[idx[1]].color[0]); \
\
        vvv[0] = _v0; \
        vvv[1] = _v1; \
\
        for (i=0; i<2; i++) { \
	/* FIXME: swapped ! */ \
                col[i][0] = vvv[!idx[i]]->v.color.red; \
                col[i][1] = vvv[!idx[i]]->v.color.green; \
                col[i][2] = vvv[!idx[i]]->v.color.blue; \
                col[i][3] = vvv[!idx[i]]->v.color.alpha; \
        } \
\
	if (dy01) { \
\
        ctmp = ((col[0][1] - col[1][1]) << 7) / dy01; \
        ctmp2 = ((col[0][2] - col[1][2]) << 7) / dy01; \
        deltgby = ((ctmp << 16) & 0xFFFF0000) | (ctmp2 & 0xFFFF); \
\
        ctmp3 = ((col[0][3] - col[1][3]) << 7) / dy01; \
        ctmp4 = ((col[0][0] - col[1][0]) << 7) / dy01; \
        deltary = ((ctmp3 << 16) & 0xFFFF0000) | (ctmp4 & 0xFFFF); \
        } else { \
        ctmp = ((col[1][1] - col[0][1]) << 7); \
        ctmp2 = ((col[1][2] - col[0][2]) << 7); \
        deltgby = ((ctmp << 16) & 0xFFFF0000) | (ctmp2 & 0xFFFF); \
\
        ctmp3 = ((col[1][3] - col[0][3]) << 7); \
        ctmp4 = ((col[1][0] - col[0][0]) << 7); \
        deltary = ((ctmp3 << 16) & 0xFFFF0000) | (ctmp4 & 0xFFFF); \
        deltgby = deltary = 0; \
        } \
\
	idx[0] = 1; /* FIXME: swapped */ \
\
        gbstart = \
	(((int)((ydiff * ctmp) + (col[idx[0]][1] << 7)) << 16) & 0x7FFF0000) \
	| ((int)((ydiff * ctmp2) + (col[idx[0]][2] << 7)) & 0x7FFF); \
        arstart = \
	(((int)((ydiff * ctmp3) + (col[idx[0]][3] << 7)) << 16) & 0x7FFF0000) \
	| ((int)((ydiff * ctmp4) + (col[idx[0]][0] << 7)) & 0x7FFF); \
} while(0)

#define SEND_LINE_COL() \
do { \
	DMAOUT(deltgby); \
	DMAOUT(deltary); \
	DMAOUT(gbstart); \
	DMAOUT(arstart); \
} while (0)

#define SEND_LINE_VERT() \
do { \
	DMAOUT(deltzy); \
	DMAOUT(zstart); \
	DMAOUT(0); \
	DMAOUT(0); \
	DMAOUT(0); \
	DMAOUT(end01); \
	DMAOUT(delt02); \
	DMAOUT(start02); \
	DMAOUT(ystart); \
	DMAOUT(y01y12); \
} while (0)


/* tri macros (mostly stolen from utah-glx...) */

#define VERT_VARS \
        SWvertex v[3]; \
        int x[3], y[3], z[3]; \
        int idx[3]; \
        int dx01, dy01; \
        int dx02, dy02; \
        int dx12, dy12; \
        int delt01, delt02, delt12; \
        int deltzx, deltzy, zstart; \
        int start02, end01, end12; \
        int ystart, y01y12; \
        int i, tmp, lr; \
        GLfloat ydiff, fy[3]
#define VERT_VARS_VOIDS \
        (void) v; (void) x; (void) y; (void) z; (void) idx; (void) dx01; \
        (void) dy01; (void) dx02; (void) dy02; (void) dx12; (void) dy12; \
        (void) delt01; (void) delt02; (void) delt12; (void) deltzx; \
        (void) deltzy; (void) zstart; (void) start02; (void) end01; \
        (void) end12; (void) ystart; (void) y01y12; (void) i; (void) tmp; \
        (void) lr; (void) ydiff; (void) fy

#define GOURAUD_VARS \
        int arstart, gbstart; \
        int deltarx, deltgbx, deltary, deltgby; \
        int ctmp, ctmp2, ctmp3, ctmp4; \
        GLubyte *(col)[3]
#define GOURAUD_VARS_VOIDS \
        (void) arstart; (void) gbstart; (void) deltarx; (void) deltgbx; \
        (void) deltary; (void) deltgby; (void) ctmp; (void) ctmp2; \
        (void) ctmp3; (void) ctmp4; (void) col

#define FLAT_VARS \
        int arstart, gbstart; \
        int deltarx, deltgbx, deltary, deltgby; \
        GLubyte *(col)[3]
#define FLAT_VARS_VOIDS \
        (void) arstart; (void) gbstart; (void) deltarx; (void) deltgbx; \
        (void) deltary; (void) deltgby; (void) col

#define TEX_VARS \
        int u0, u1, u2; \
        GLfloat ru0, ru1, ru2; \
        int v0, v1, v2; \
        GLfloat rv0, rv1, rv2; \
        GLfloat w0, w1, w2; \
        GLfloat rw0, rw1, rw2; \
        int baseu, basev; \
        int d0, d1, d2; \
        int deltdx, deltvx, deltux, deltdy, deltvy, deltuy; \
        int deltwx, deltwy; \
        int rbaseu, rbasev; \
        int dstart, ustart, wstart, vstart; \
        static int stmp = 0; \
        s3vTextureObjectPtr t
#define TEX_VARS_VOIDS \
        (void) u0; (void) u1; (void) u2; (void) ru0; (void) ru1; (void) ru2; \
        (void) v0; (void) v1; (void) v2; (void) rv0; (void) rv1; (void) rv2; \
        (void) w0; (void) w1; (void) w2; (void) rw0; (void) rw1; (void) rw2; \
        (void) baseu; (void) basev; (void) d0; (void) d1; (void) d2; \
        (void) deltdx; (void) deltvx; (void) deltux; (void) deltdy; \
        (void) deltuy; (void) deltwx; (void) deltwy; (void) rbaseu; \
        (void) rbasev; (void) dstart; (void) ustart; (void) wstart; \
        (void) vstart; (void) stmp; (void) t

#define SORT_VERT() \
do { \
	for (i=0; i<3; i++) \
		fy[i] = v[i].win[1]; \
\
		if (fy[1] > fy[0]) {  /* (fy[1] > fy[0]) */ \
\
			if (fy[2] > fy[0]) { \
				idx[0] = 0; \
				if (fy[1] > fy[2]) { \
					idx[1] = 2; \
					idx[2] = 1; \
				} else { \
					idx[1] = 1; \
					idx[2] = 2; \
				} \
			} else { \
				idx[0] = 2; \
				idx[1] = 0; \
				idx[2] = 1; \
			} \
		} else { /* (fy[1] < y[0]) */ \
			if (fy[2] > fy[0]) { \
				idx[0] = 1; \
				idx[1] = 0; \
				idx[2] = 2; \
		} else { \
			idx[2] = 0; \
			if (fy[2] > fy[1]) { \
				idx[0] = 1; \
				idx[1] = 2; \
			} else { \
				idx[0] = 2; \
				idx[1] = 1; \
			} \
		} \
	} \
} while(0)

#define SET_VERT() \
do { \
	for (i=0; i<3; i++) \
	{ \
		x[i] = ((v[idx[i]].win[0]) * /* 0x100000*/  1024.0 * 1024.0); \
		y[i] = fy[i] = (dPriv->h - v[idx[i]].win[1]); \
		z[i] = ((v[idx[i]].win[2]) * /* 0x8000 */ 1024.0 * 32.0); \
	} \
\
	ydiff = fy[0] - (float)y[0]; \
\
	ystart = y[0]; \
\
	dx12 = x[2] - x[1]; \
	dy12 = y[1] - y[2]; \
	dx01 = x[1] - x[0]; \
	dy01 = y[0] - y[1]; \
	dx02 = x[2] - x[0]; \
	dy02 = y[0] - y[2]; \
\
	delt01 = delt02 = delt12 = 0; \
} while (0)


#define SET_XY() \
do { \
	if (dy01) delt01 = dx01 / dy01; \
	if (dy12) delt12 = dx12 / dy12; \
	delt02 = dx02 / dy02; \
\
	start02 = x[0] + (ydiff * delt02); \
	end01 = x[0] + (ydiff * delt01); \
	end12 = x[1] + ((fy[1] - (GLfloat)y[1]) * delt12); \
} while (0)

#define SET_DIR() \
do { \
	tmp = x[1] - (dy01 * delt02 + x[0]); \
	if (tmp > 0) { \
		lr = 0x80000000; \
	} else { \
		tmp *= -1; \
		lr = 0; \
	} \
	tmp >>= 20; \
\
        y01y12 = ((((y[0] - y[1]) & 0x7FF) << 16) \
                 | ((y[1] - y[2]) & 0x7FF) | lr); \
} while (0)

#define SET_Z() \
do { \
	deltzy = (z[2] - z[0]) / dy02; \
	if (tmp) { \
		deltzx = (z[1] - (dy01 * deltzy + z[0])) / tmp; \
	} else { \
		deltzx = 0; \
	} \
	zstart = (deltzy * ydiff) + z[0]; \
} while (0)

#define SET_FLAT_COL() \
do { \
	col[0] = &(v[0].color[0]); \
	deltarx = deltary = deltgbx = deltgby = 0; \
	gbstart = (((col[0][1]) << 23) | ((col[0][2]) << 7)); \
	arstart = (((col[0][3]) << 23) | ((col[0][0]) << 7)); \
} while(0)

#define SET_GOURAUD_COL() \
do { \
	col[0] = &(v[idx[0]].color[0]); \
	col[1] = &(v[idx[1]].color[0]); \
	col[2] = &(v[idx[2]].color[0]); \
\
	ctmp = ((col[2][3] - col[0][3]) << 7) / dy02; \
	ctmp2 = ((col[2][0] - col[0][0]) << 7) / dy02; \
	deltary = ((ctmp << 16) & 0xFFFF0000) | (ctmp2 & 0xFFFF); \
	ctmp3 = ((col[2][1] - col[0][1]) << 7) / dy02; \
	ctmp4 = ((col[2][2] - col[0][2]) << 7) / dy02; \
	deltgby = ((ctmp3 << 16) & 0xFFFF0000) | (ctmp4 & 0xFFFF); \
	gbstart = \
	(((int)((ydiff * ctmp3) + (col[0][1] << 7)) << 16) & 0x7FFF0000) \
	| ((int)((ydiff * ctmp4) + (col[0][2] << 7)) & 0x7FFF); \
	arstart = \
	(((int)((ydiff * ctmp) + (col[0][3] << 7)) << 16) & 0x7FFF0000) \
	| ((int)((ydiff * ctmp2) + (col[0][0] << 7)) & 0x7FFF); \
	if (tmp) { \
	int ax, rx, gx, bx; \
	ax = ((col[1][3] << 7) - (dy01 * ctmp + (col[0][3] << 7))) / tmp; \
	rx = ((col[1][0] << 7) - (dy01 * ctmp2 + (col[0][0] << 7))) / tmp; \
	gx = ((col[1][1] << 7) - (dy01 * ctmp3 + (col[0][1] << 7))) / tmp; \
	bx = ((col[1][2] << 7) - (dy01 * ctmp4 + (col[0][2] << 7))) / tmp; \
	deltarx = ((ax << 16) & 0xFFFF0000) | (rx & 0xFFFF); \
	deltgbx = ((gx << 16) & 0xFFFF0000) | (bx & 0xFFFF); \
	} else { \
	deltgbx = deltarx = 0; \
	} \
} while (0)

#define SET_TEX_VERT() \
do { \
        t = ((s3vTextureObjectPtr) \
                ctx->Texture.Unit[0]._Current->DriverData); \
        deltwx = deltwy = wstart = deltdx = deltdy = dstart = 0; \
\
        u0 = (v[idx[0]].attrib[FRAG_ATTRIB_TEX0][0] \
                * (GLfloat)(t->image[0].image->Width) * 256.0); \
        u1 = (v[idx[1]].attrib[FRAG_ATTRIB_TEX0][0] \
                * (GLfloat)(t->globj->Image[0][0]->Width) * 256.0); \
        u2 = (v[idx[2]].attrib[FRAG_ATTRIB_TEX0][0] \
                * (GLfloat)(t->globj->Image[0][0]->Width) * 256.0); \
        v0 = (v[idx[0]].attrib[FRAG_ATTRIB_TEX0][1] \
                * (GLfloat)(t->globj->Image[0][0]->Height) * 256.0); \
        v1 = (v[idx[1]].attrib[FRAG_ATTRIB_TEX0][1] \
                * (GLfloat)(t->globj->Image[0][0]->Height) * 256.0); \
        v2 = (v[idx[2]].attrib[FRAG_ATTRIB_TEX0][1] \
                * (GLfloat)(t->globj->Image[0][0]->Height) * 256.0); \
\
        w0 = (v[idx[0]].win[3]); \
        w1 = (v[idx[1]].win[3]); \
        w2 = (v[idx[2]].win[3]); \
} while (0)

#define SET_BASEUV() \
do { \
        if (u0 < u1) { \
                if (u0 < u2) { \
                        baseu = u0; \
                } else { \
                        baseu = u2; \
                } \
        } else { \
                if (u1 < u2) { \
                        baseu = u1; \
                } else { \
                        baseu = u2; \
                } \
        } \
\
        if (v0 < v1) { \
                if (v0 < v2) { \
                        basev = v0; \
                } else { \
                        basev = v2; \
                } \
        } else { \
                if (v1 < v2) { \
                        basev = v1; \
                } else { \
                        basev = v2; \
                } \
        } \
} while (0)


#define SET_RW() \
do { \
        /* GLfloat minW; \
\
        if (w0 < w1) { \
                if (w0 < w2) { \
                        minW = w0; \
                } else { \
                        minW = w2; \
                } \
        } else { \
                if (w1 < w2) { \
                        minW = w1; \
                } else { \
                        minW = w2; \
                } \
        } */ \
\
        rw0 = (512.0 * w0); \
        rw1 = (512.0 * w1); \
        rw2 = (512.0 * w2); \
} while (0)

#define SET_D() \
do { \
        GLfloat sxy, suv; \
        int lev; \
\
        suv = (v[idx[0]].attrib[FRAG_ATTRIB_TEX0][0] - \
                v[idx[2]].attrib[FRAG_ATTRIB_TEX0][0]) * \
                (v[idx[1]].attrib[FRAG_ATTRIB_TEX0][1] - \
                v[idx[2]].attrib[FRAG_ATTRIB_TEX0][1]) - \
                (v[idx[1]].attrib[FRAG_ATTRIB_TEX0][0] - \
                v[idx[2]].attrib[FRAG_ATTRIB_TEX0][0]) * \
                (v[idx[0]].attrib[FRAG_ATTRIB_TEX0][1] - \
                v[idx[2]].attrib[FRAG_ATTRIB_TEX0][2]); \
\
        sxy = (v[idx[0]].attrib[FRAG_ATTRIB_TEX0][0] - \
                v[idx[2]].attrib[FRAG_ATTRIB_TEX0][0]) * \
                (v[idx[1]].attrib[FRAG_ATTRIB_TEX0][1] - \
                v[idx[2]].attrib[FRAG_ATTRIB_TEX0][1]) - \
                (v[idx[1]].attrib[FRAG_ATTRIB_TEX0][0] - \
                v[idx[2]].attrib[FRAG_ATTRIB_TEX0][0]) * \
                (v[idx[0]].attrib[FRAG_ATTRIB_TEX0][1] - \
                v[idx[2]].attrib[FRAG_ATTRIB_TEX0][2]); \
\
	if (sxy < 0) sxy *= -1.0; \
	if (suv < 0) suv *= -1.0; \
\
	lev = *(int*)&suv - *(int *)&sxy; \
	if (lev < 0) \
		lev = 0; \
	else \
		lev >>=23; \
	dstart = (lev << 27); \
} while (0)

#define SET_UVWD() \
do { \
        SET_BASEUV(); \
        SET_RW(); \
        SET_D(); \
        ru0 = (((u0 - baseu) * rw0)); \
        ru1 = (((u1 - baseu) * rw1)); \
        ru2 = (((u2 - baseu) * rw2)); \
        rv0 = (((v0 - basev) * rw0)); \
        rv1 = (((v1 - basev) * rw1)); \
        rv2 = (((v2 - basev) * rw2)); \
\
        while (baseu < 0) { baseu += (t->globj->Image[0][0]->Width << 8); } \
        while (basev < 0) { basev += (t->globj->Image[0][0]->Height << 8); } \
\
        if (!(baseu & 0xFF)) \
                { baseu = (baseu >> 8); } \
        else \
                { baseu = (baseu >> 8) + 1; } \
\
        if ((basev & 0x80) || !(basev & 0xFF)) \
                { basev = (basev >> 8); } \
        else \
                { basev = (basev >> 8) - 1; } \
\
        rbaseu = (baseu) << (16 - t->globj->Image[0][0]->WidthLog2); \
        rbasev = (basev) << (16 - t->globj->Image[0][0]->WidthLog2); \
        deltuy = (((ru2 - ru0) / dy02)); \
        deltvy = (((rv2 - rv0) / dy02)); \
        rw0 *= (1024.0 * 512.0); \
        rw1 *= (1024.0 * 512.0); \
        rw2 *= (1024.0 * 512.0); \
        deltwy = ((rw2 - rw0) / dy02); \
        if (tmp) { \
                deltux = ((ru1 - (dy01 * deltuy + ru0)) / tmp); \
                deltvx = ((rv1 - (dy01 * deltvy + rv0)) / tmp); \
                deltwx = ((rw1 - (dy01 * deltwy + rw0)) / tmp); \
        } else { deltux = deltvx = deltwx = 0; } \
                ustart = (deltuy * ydiff) + (ru0); \
                vstart = (deltvy * ydiff) + (rv0); \
                wstart = (deltwy * ydiff) + (rw0); \
} while (0)

#define SEND_UVWD() \
do { \
        DMAOUT((rbasev & 0xFFFF)); \
        DMAOUT((0xa0000000 | (rbaseu & 0xFFFF))); \
        DMAOUT(deltwx); \
        DMAOUT(deltwy); \
        DMAOUT(wstart); \
        DMAOUT(deltdx); \
        DMAOUT(deltvx); \
        DMAOUT(deltux); \
        DMAOUT(deltdy); \
        DMAOUT(deltvy); \
        DMAOUT(deltuy); \
        DMAOUT(dstart); \
        DMAOUT(vstart); \
        DMAOUT(ustart); \
} while (0)

#define SEND_VERT() \
do { \
	DMAOUT(deltzx); \
	DMAOUT(deltzy); \
	DMAOUT(zstart); \
	DMAOUT(delt12); \
	DMAOUT(end12); \
	DMAOUT(delt01); \
	DMAOUT(end01); \
	DMAOUT(delt02); \
	DMAOUT(start02); \
	DMAOUT(ystart); \
	DMAOUT(y01y12); \
} while (0)

#define SEND_COL() \
do { \
	DMAOUT(deltgbx); \
	DMAOUT(deltarx); \
	DMAOUT(deltgby); \
	DMAOUT(deltary); \
	DMAOUT(gbstart); \
	DMAOUT(arstart); \
} while (0)

/**** MACROS end ****/




static void TAG(s3v_point)( s3vContextPtr vmesa, 
			     const s3vVertex *_v0 )
{
}

static void TAG(s3v_line)( s3vContextPtr vmesa, 
			     const s3vVertex *_v0,
			     const s3vVertex *_v1 )
{
	GLcontext *ctx = vmesa->glCtx;
	__DRIdrawablePrivate *dPriv = vmesa->driDrawable;

	LINE_VERT_VARS;
#if (IND & S3V_RAST_FLAT_BIT)
	LINE_FLAT_VARS;
#else
	LINE_GOURAUD_VARS;
#endif
#if (IND & S3V_RAST_CULL_BIT)
	GLfloat cull;
        (void) cull;
#endif
	LINE_VERT_VARS_VOIDS;
#if (IND & S3V_RAST_FLAT_BIT)
	LINE_FLAT_VARS_VOIDS;
#else
	LINE_GOURAUD_VARS_VOIDS;
#endif

	DEBUG(("*** s3v_line: "));
#if (IND & S3V_RAST_CULL_BIT)
	DEBUG(("cull "));
#endif
#if (IND & S3V_RAST_FLAT_BIT)
        DEBUG(("flat "));
#endif

	DEBUG(("***\n"));

#if 0
	s3v_print_vertex(ctx, _v0);
	s3v_print_vertex(ctx, _v1);
#endif

	s3v_translate_vertex( ctx, _v0, &v[0] );
	s3v_translate_vertex( ctx, _v1, &v[1] );

#if (IND & S3V_RAST_CULL_BIT)
	/* FIXME: should we cull lines too? */
#endif
	(void)v; /* v[0]; v[1]; */

	SORT_LINE_VERT();
	SET_LINE_VERT();

	SET_LINE_XY();
	SET_LINE_DIR();
	SET_LINE_Z();

#if (IND & S3V_RAST_FLAT_BIT)
	SET_LINE_FLAT_COL();
#else
	SET_LINE_GOURAUD_COL();
#endif

	DMAOUT_CHECK(3DLINE_GBD, 15);
		SEND_LINE_COL();
		DMAOUT(0);
		SEND_LINE_VERT();
	DMAFINISH();
}

static void TAG(s3v_triangle)( s3vContextPtr vmesa,
				 const s3vVertex *_v0,
				 const s3vVertex *_v1, 
				 const s3vVertex *_v2 )
{
	GLcontext *ctx = vmesa->glCtx;
	__DRIdrawablePrivate *dPriv = vmesa->driDrawable;

	VERT_VARS;
#if (IND & S3v_RAST_FLAT_BIT)
	FLAT_VARS;
#else
	GOURAUD_VARS;
#endif
#if (IND & S3V_RAST_TEX_BIT)
	TEX_VARS;
#endif
#if (IND & S3V_RAST_CULL_BIT)
	GLfloat cull;
#endif
	VERT_VARS_VOIDS;
#if (IND & S3v_RAST_FLAT_BIT)
	FLAT_VARS_VOIDS;
#else
	GOURAUD_VARS_VOIDS;
#endif
#if (IND & S3V_RAST_TEX_BIT)
	TEX_VARS_VOIDS;
#endif

	DEBUG(("*** s3v_triangle: "));
#if (IND & S3V_RAST_CULL_BIT)
	DEBUG(("cull "));
#endif
#if (IND & S3V_RAST_FLAT_BIT)
        DEBUG(("flat "));
#endif
#if (IND & S3V_RAST_TEX_BIT)
        DEBUG(("tex "));
#endif

DEBUG(("***\n"));

#if 0
	s3v_print_vertex(ctx, _v0);
	s3v_print_vertex(ctx, _v1);
	s3v_print_vertex(ctx, _v2);
#endif

	s3v_translate_vertex( ctx, _v0, &v[0] );
	s3v_translate_vertex( ctx, _v1, &v[1] );
	s3v_translate_vertex( ctx, _v2, &v[2] );

#if (IND & S3V_RAST_CULL_BIT)
	cull = vmesa->backface_sign *
		((v[1].win[0] - v[0].win[0]) * (v[0].win[1] - v[2].win[1]) +
		 (v[1].win[1] - v[0].win[1]) * (v[2].win[0] - v[0].win[0]));

	if (cull < vmesa->cull_zero /* -0.02f */) return;
#endif

	(void)v; /* v[0]; v[1]; v[2]; */

	SORT_VERT();
	SET_VERT();

	if (dy02 == 0) return;

	SET_XY();
	SET_DIR();
	SET_Z();
	
#if (IND & S3V_RAST_TEX_BIT)
	SET_TEX_VERT();
	SET_UVWD();
#endif

#if (IND & S3V_RAST_FLAT_BIT)
	SET_FLAT_COL();
#else
	SET_GOURAUD_COL();
#endif

#if (IND & S3V_RAST_TEX_BIT)
        DMAOUT_CHECK(3DTRI_BASEV, 31);
		SEND_UVWD();
		SEND_COL();
		SEND_VERT();
	DMAFINISH();
#else
	DMAOUT_CHECK(3DTRI_GBX, 17);
		SEND_COL();
		SEND_VERT();
	DMAFINISH();
#endif
}

static void TAG(s3v_quad)( s3vContextPtr vmesa,
			    const s3vVertex *_v0,
			    const s3vVertex *_v1,
			    const s3vVertex *_v2,
			    const s3vVertex *_v3 )
{
	GLcontext *ctx = vmesa->glCtx;
        __DRIdrawablePrivate *dPriv = vmesa->driDrawable;

	SWvertex temp_v[4];
        VERT_VARS;
#if (IND & S3v_RAST_FLAT_BIT)
        FLAT_VARS;
#else
        GOURAUD_VARS;
#endif
#if (IND & S3V_RAST_TEX_BIT)
        TEX_VARS;
#endif
#if (IND & S3V_RAST_CULL_BIT)
        GLfloat cull;
#endif
        VERT_VARS_VOIDS;
#if (IND & S3v_RAST_FLAT_BIT)
        FLAT_VARS_VOIDS;
#else
        GOURAUD_VARS_VOIDS;
#endif
#if (IND & S3V_RAST_TEX_BIT)
        TEX_VARS_VOIDS;
#endif

	DEBUG(("*** s3v_quad: "));
#if (IND & S3V_RAST_CULL_BIT)
        DEBUG(("cull "));
		/* printf(""); */ /* speed trick */
#endif
#if (IND & S3V_RAST_FLAT_BIT)
        DEBUG(("flat "));
#endif
#if (IND & S3V_RAST_TEX_BIT)
        DEBUG(("tex "));
#endif

	DEBUG(("***\n"));

#if 0
	s3v_print_vertex(ctx, _v0);
	s3v_print_vertex(ctx, _v1);
	s3v_print_vertex(ctx, _v2);
	s3v_print_vertex(ctx, _v3);
#endif
	s3v_translate_vertex( ctx, _v0, &temp_v[0] );
	s3v_translate_vertex( ctx, _v1, &temp_v[1] );
	s3v_translate_vertex( ctx, _v2, &temp_v[2] );
	s3v_translate_vertex( ctx, _v3, &temp_v[3] );

	/* FIRST TRI (0,1,2) */

	/* ROMEO */
	/* printf(""); */ /* speed trick (a) [turn on if (a) is return]*/

	v[0] = temp_v[0];
	v[1] = temp_v[1];
	v[2] = temp_v[2];

#if (IND & S3V_RAST_CULL_BIT)
	cull = vmesa->backface_sign *
		((v[1].win[0] - v[0].win[0]) * (v[0].win[1] - v[2].win[1]) +
		 (v[1].win[1] - v[0].win[1]) * (v[2].win[0] - v[0].win[0]));

	if (cull < vmesa->cull_zero /* -0.02f */) goto second; /* return; */ /* (a) */
#endif
	
#if 0
        v[0] = temp_v[0];
	v[1] = temp_v[1];
	v[2] = temp_v[2];
#else
        (void) v;
#endif
	SORT_VERT();
	SET_VERT();

	if (dy02 == 0) goto second;

        SET_XY();
        SET_DIR();
        SET_Z();

#if (IND & S3V_RAST_TEX_BIT)
	SET_TEX_VERT();
	SET_UVWD();
#endif

#if (IND & S3V_RAST_FLAT_BIT)
	SET_FLAT_COL();
#else
	SET_GOURAUD_COL();
#endif

#if (IND & S3V_RAST_TEX_BIT)
	DMAOUT_CHECK(3DTRI_BASEV, 31);
		SEND_UVWD();
		SEND_COL();
		SEND_VERT();
	DMAFINISH();
#else
	DMAOUT_CHECK(3DTRI_GBX, 17);
		SEND_COL();
		SEND_VERT();
	DMAFINISH();
#endif

	/* SECOND TRI (0,2,3) */

second:
	v[0] = temp_v[0];
	v[1] = temp_v[2];
	v[2] = temp_v[3];

#if (IND & S3V_RAST_CULL_BIT)
	cull = vmesa->backface_sign *
		((v[1].win[0] - v[0].win[0]) * (v[0].win[1] - v[2].win[1]) +
		 (v[1].win[1] - v[0].win[1]) * (v[2].win[0] - v[0].win[0]));
		 
	if (cull < /* -0.02f */ vmesa->cull_zero) return;
#endif

/* second: */

	/* ROMEO */
	/* printf(""); */ /* speed trick */

	v[0] = temp_v[0];
	v[1] = temp_v[2];
	v[2] = temp_v[3];

	SORT_VERT();
	SET_VERT();

	if (dy02 == 0) return;

	SET_XY();
	SET_DIR();
	SET_Z();

#if (IND & S3V_RAST_TEX_BIT)
	SET_TEX_VERT();
	SET_UVWD();
#endif

#if (IND & S3V_RAST_FLAT_BIT)
	SET_FLAT_COL();
#else
	SET_GOURAUD_COL();
#endif

#if (IND & S3V_RAST_TEX_BIT)
	DMAOUT_CHECK(3DTRI_BASEV, 31);
		SEND_UVWD();
		SEND_COL();
		SEND_VERT();
	DMAFINISH();
#else
	DMAOUT_CHECK(3DTRI_GBX, 17);
		SEND_COL();
		SEND_VERT();
	DMAFINISH();
#endif
}

static void TAG(s3v_init)(void)
{
	s3v_point_tab[IND]	= TAG(s3v_point);
	s3v_line_tab[IND]	= TAG(s3v_line);
	s3v_tri_tab[IND]	= TAG(s3v_triangle);
	s3v_quad_tab[IND]	= TAG(s3v_quad);
}

#undef IND
#undef TAG

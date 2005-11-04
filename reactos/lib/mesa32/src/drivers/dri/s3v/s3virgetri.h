/*
 * Author: Max Lingua <sunmax@libero.it>
 */

#define LOCAL_VARS \
	int vert0, vert1, vert2; \
	GLfloat y0, y1, y2, ydiff; \
	int iy0, iy1, iy2; \
	int x0, x1, x2, z0, z1, z2; \
	int dy01, dy02, dy12, dx01, dx02, dx12; \
	int delt02, delt01, delt12, end01, end12, start02; \
	int zstart, arstart, gbstart; \
	int deltzy, deltzx, deltarx, deltgbx, deltary, deltgby; \
	GLubyte (*colours)[4]; \
	GLubyte (*scolours)[4]; \
	static int tp = 0; \
	int tmp, lr

#define LOCAL_TEX_VARS \
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
	s3virgeTextureObject_t *t

#define CULL_BACKFACE() \
	do { \
		GLfloat *w0 = VB->Win.data[e0]; \
		GLfloat *w1 = VB->Win.data[e1]; \
		GLfloat *w2 = VB->Win.data[e2]; \
		float cull; \
		cull = ctx->backface_sign * ((w1[0] - w0[0]) * (w0[1] - w2[1]) + \
			(w1[1] - w0[1]) * (w2[0] - w0[0])); \
		if (cull < 0) \
			return; \
	} while (0)
	
#define SORT_VERTICES() \
	do { \
		y0 = VB->Win.data[e0][1]; \
		y1 = VB->Win.data[e1][1]; \
		y2 = VB->Win.data[e2][1]; \
		if (y1 > y0) { \
			if (y2 > y0) {  \
				vert0 = e0; \
				if (y1 > y2) { vert2 = e1; vert1 = e2; } else { vert2 = e2; vert1 = e1; } \
			} else { vert0 = e2; vert1 = e0; vert2 = e1; } \
		} else { \
			if (y2 > y0) { vert0 = e1; vert1 = e0; vert2 = e2; } else { \
				vert2 = e0; \
				if (y2 > y1) { vert0 = e1; vert1 = e2; } else { vert0 = e2; vert1 = e1; } \
			} \
		} \
	} while (0)

#define SET_VARIABLES() \
	do { \
		iy0 = y0 = ((s3virgeDB->height - (VB->Win.data[vert0][1]))); \
		iy1 = y1 = ((s3virgeDB->height - (VB->Win.data[vert1][1]))); \
		iy2 = y2 = ((s3virgeDB->height - (VB->Win.data[vert2][1]))); \
		if (iy0 == iy2) { return; } \
		ydiff = y0 - (float)iy0; \
		x0 = ((VB->Win.data[vert0][0]) * 1024.0 * 1024.0); \
		x1 = ((VB->Win.data[vert1][0]) * 1024.0 * 1024.0); \
		x2 = ((VB->Win.data[vert2][0]) * 1024.0 * 1024.0); \
		z0 = (VB->Win.data[vert0][2] * 1024.0 * 32.0); \
		z1 = (VB->Win.data[vert1][2] * 1024.0 * 32.0); \
		z2 = (VB->Win.data[vert2][2] * 1024.0 * 32.0); \
		dx12 = x2 - x1; \
		dy12 = iy1 - iy2; \
		dx01 = x1 - x0; \
		dy01 = iy0 - iy1; \
		dx02 = x2 - x0; \
		dy02 = iy0 - iy2; \
		delt12 = delt02 = delt01 = 0; \
	} while (0)
	
#define SET_TEX_VARIABLES() \
	do { \
		t = ((s3virgeTextureObject_t *)ctx->Texture.Unit[0].Current->DriverData); \
		deltwx = deltwy = wstart = deltdx = deltdy = dstart = 0; \
		u0 = (VB->TexCoordPtr[0]->data[vert0][0] * (GLfloat)(t->tObj->Image[0]->Width) * 256.0); \
		u1 = (VB->TexCoordPtr[0]->data[vert1][0] * (GLfloat)(t->tObj->Image[0]->Width) * 256.0); \
		u2 = (VB->TexCoordPtr[0]->data[vert2][0] * (GLfloat)(t->tObj->Image[0]->Width) * 256.0); \
		v0 = (VB->TexCoordPtr[0]->data[vert0][1] * (GLfloat)(t->tObj->Image[0]->Height) * 256.0); \
		v1 = (VB->TexCoordPtr[0]->data[vert1][1] * (GLfloat)(t->tObj->Image[0]->Height) * 256.0); \
		v2 = (VB->TexCoordPtr[0]->data[vert2][1] * (GLfloat)(t->tObj->Image[0]->Height) * 256.0); \
		w0 = (VB->Win.data[vert0][3]); \
		w1 = (VB->Win.data[vert1][3]); \
		w2 = (VB->Win.data[vert2][3]); \
	} while (0)

#define FLATSHADE_COLORS() \
	do { \
		GLubyte *col = &(colours[pv][0]); \
		deltarx = deltary = deltgbx = deltgby = 0; \
		gbstart = (((col[1]) << 23) | ((col[2]) << 7)); \
		arstart = (((col[3]) << 23) | ((col[0]) << 7)); \
	} while (0)

#define GOURAUD_COLORS() \
	do { \
 		int ctmp, ctmp2, ctmp3, ctmp4; \
		GLubyte *col0, *col1, *col2; \
		col0 = &(colours[vert0][0]); \
		col1 = &(colours[vert1][0]); \
		col2 = &(colours[vert2][0]); \
		ctmp = ((col2[3] - col0[3]) << 7) / dy02; \
		ctmp2 = ((col2[0] - col0[0]) << 7) / dy02; \
		deltary = ((ctmp << 16) & 0xFFFF0000) | (ctmp2 & 0xFFFF); \
		ctmp3 = ((col2[1] - col0[1]) << 7) / dy02; \
		ctmp4 = ((col2[2] - col0[2]) << 7) / dy02; \
		deltgby = ((ctmp3 << 16) & 0xFFFF0000) | (ctmp4 & 0xFFFF); \
		gbstart = (((int)((ydiff * ctmp3) + (col0[1] << 7)) << 16) & 0x7FFF0000) | \
          		  ((int)((ydiff * ctmp4) + (col0[2] << 7)) & 0x7FFF); \
		arstart = (((int)((ydiff * ctmp) + (col0[3] << 7)) << 16) & 0x7FFF0000) | \
          		  ((int)((ydiff * ctmp2) + (col0[0] << 7)) & 0x7FFF); \
		if (tmp) { \
			int ax, rx, gx, bx; \
			ax = ((col1[3] << 7) - (dy01 * ctmp + (col0[3] << 7))) / tmp; \
			rx = ((col1[0] << 7) - (dy01 * ctmp2 + (col0[0] << 7))) / tmp; \
			gx = ((col1[1] << 7) - (dy01 * ctmp3 + (col0[1] << 7))) / tmp; \
			bx = ((col1[2] << 7) - (dy01 * ctmp4 + (col0[2] << 7))) / tmp; \
			deltarx = ((ax << 16) & 0xFFFF0000) | (rx & 0xFFFF); \
			deltgbx = ((gx << 16) & 0xFFFF0000) | (bx & 0xFFFF); \
		} else { \
			deltgbx = deltarx = 0; \
 		} \
 	} while (0)

#define SET_XY() \
	do { \
		delt02 = dx02 / dy02; \
		if (dy12) delt12 = dx12 / dy12; \
		if (dy01) delt01 = dx01 / dy01; \
		start02 = (ydiff * delt02) + x0; \
		end01 = (ydiff * delt01) + x0; \
		end12 = ((y1 - (GLfloat)iy1) * delt12) + x1; \
	} while (0)

#define SET_DIR() \
	do { \
		tmp = x1 - (dy01 * delt02 + x0); \
		if (tmp > 0) { \
			lr = 0x80000000; \
		} else { \
			tmp *= -1; \
			lr = 0; \
		} \
		tmp >>= 20; \
	} while (0)

#define SET_Z() \
	do { \
		deltzy = (z2 - z0) / dy02; \
		if (tmp) { \
			deltzx = (z1 - (dy01 * deltzy + z0)) / tmp; \
		} else { deltzx = 0; } \
		zstart = (deltzy * ydiff) + z0; \
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
		rw0 = (512.0 * w0); \
		rw1 = (512.0 * w1); \
		rw2 = (512.0 * w2); \
	} while (0)


#define SET_D() \
	do { \
		GLfloat sxy, suv; \
		int lev; \
		suv = (VB->TexCoordPtr[0]->data[vert0][0] - \
		       VB->TexCoordPtr[0]->data[vert2][0]) * \
		      (VB->TexCoordPtr[0]->data[vert1][1] - \
		       VB->TexCoordPtr[0]->data[vert2][1]) - \
		      (VB->TexCoordPtr[0]->data[vert1][0] - \
		       VB->TexCoordPtr[0]->data[vert2][0]) * \
		      (VB->TexCoordPtr[0]->data[vert0][1] - \
		       VB->TexCoordPtr[0]->data[vert2][2]); \
		sxy = (VB->Win.data[vert0][0] - \
		       VB->Win.data[vert2][0]) * \
		      (VB->Win.data[vert1][1] - \
		       VB->Win.data[vert2][1]) - \
		      (VB->Win.data[vert1][0] - \
		       VB->Win.data[vert2][0]) * \
		      (VB->Win.data[vert0][1] - \
		       VB->Win.data[vert2][2]); \
		if (sxy < 0) sxy *= -1.0; \
		if (suv < 0) suv *= -1.0; \
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
		while (baseu < 0) { baseu += (t->tObj->Image[0]->Width << 8); } \
		while (basev < 0) { basev += (t->tObj->Image[0]->Height << 8); } \
		if (!(baseu & 0xFF)) { baseu = (baseu >> 8); } else { baseu = (baseu >> 8) + 1; } \
		if ((basev & 0x80) || !(basev & 0xFF)) { basev = (basev >> 8); } else { basev = (basev >> 8) - 1; } \
		rbaseu = (baseu) << (16 - t->widthLog2); \
		rbasev = (basev) << (16 - t->widthLog2); \
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


#define SEND_COLORS() \
	do { \
		WAITFIFOEMPTY(6); \
		OUTREG( (S3VIRGE_3DTRI_REG | S3VIRGE_3DTRI_GBX), deltgbx); \
		OUTREG( (S3VIRGE_3DTRI_REG | S3VIRGE_3DTRI_ARX), deltarx); \
		OUTREG( (S3VIRGE_3DTRI_REG | S3VIRGE_3DTRI_GBY), deltgby); \
		OUTREG( (S3VIRGE_3DTRI_REG | S3VIRGE_3DTRI_ARY), deltary); \
		OUTREG( (S3VIRGE_3DTRI_REG | S3VIRGE_3DTRI_GS_BS), gbstart); \
		OUTREG( (S3VIRGE_3DTRI_REG | S3VIRGE_3DTRI_AS_RS), arstart); \
	} while (0)

#define SEND_VERTICES() \
	do { \
		WAITFIFOEMPTY(6); \
		OUTREG( (S3VIRGE_3DTRI_REG | S3VIRGE_3DTRI_ZSTART), zstart); \
		OUTREG( (S3VIRGE_3DTRI_REG | S3VIRGE_3DTRI_ZXD), deltzx); \
		OUTREG( (S3VIRGE_3DTRI_REG | S3VIRGE_3DTRI_ZYD), deltzy); \
		OUTREG( (S3VIRGE_3DTRI_REG | S3VIRGE_3DTRI_TXDELTA12), delt12); \
		OUTREG( (S3VIRGE_3DTRI_REG | S3VIRGE_3DTRI_TXEND12), end12); \
		OUTREG( (S3VIRGE_3DTRI_REG | S3VIRGE_3DTRI_TXDELTA01), delt01); \
		WAITFIFOEMPTY(5); \
		OUTREG( (S3VIRGE_3DTRI_REG | S3VIRGE_3DTRI_TXEND01), end01); \
		OUTREG( (S3VIRGE_3DTRI_REG | S3VIRGE_3DTRI_TXDELTA02), delt02); \
		OUTREG( (S3VIRGE_3DTRI_REG | S3VIRGE_3DTRI_TXSTART02), start02); \
		OUTREG( (S3VIRGE_3DTRI_REG | S3VIRGE_3DTRI_TYS), iy0); \
		OUTREG( (S3VIRGE_3DTRI_REG | S3VIRGE_3DTRI_TY01_Y12), \
				((((iy0 - iy1) & 0x7FF) << 16) | \
				((iy1 - iy2) & 0x7FF) | lr)); \
	} while (0)

#define SEND_UVWD() \
	do { \
		WAITFIFOEMPTY(7); \
		OUTREG( (S3VIRGE_3DTRI_REG | S3VIRGE_3DTRI_BASEV), (rbasev & 0xFFFF)); \
		OUTREG( (S3VIRGE_3DTRI_REG | S3VIRGE_3DTRI_BASEU), (0xa0000000 | (rbaseu & 0xFFFF))); \
		OUTREG( (S3VIRGE_3DTRI_REG | S3VIRGE_3DTRI_WXD), deltwx); \
		OUTREG( (S3VIRGE_3DTRI_REG | S3VIRGE_3DTRI_WYD), deltwy); \
		OUTREG( (S3VIRGE_3DTRI_REG | S3VIRGE_3DTRI_WSTART), wstart); \
		OUTREG( (S3VIRGE_3DTRI_REG | S3VIRGE_3DTRI_DXD), deltdx); \
		OUTREG( (S3VIRGE_3DTRI_REG | S3VIRGE_3DTRI_VXD), deltvx); \
		WAITFIFOEMPTY(7); \
		OUTREG( (S3VIRGE_3DTRI_REG | S3VIRGE_3DTRI_UXD), deltux); \
		OUTREG( (S3VIRGE_3DTRI_REG | S3VIRGE_3DTRI_DYD), deltdy); \
		OUTREG( (S3VIRGE_3DTRI_REG | S3VIRGE_3DTRI_VYD), deltvy); \
		OUTREG( (S3VIRGE_3DTRI_REG | S3VIRGE_3DTRI_UYD), deltuy); \
		OUTREG( (S3VIRGE_3DTRI_REG | S3VIRGE_3DTRI_DSTART), dstart); \
		OUTREG( (S3VIRGE_3DTRI_REG | S3VIRGE_3DTRI_VSTART), vstart); \
		OUTREG( (S3VIRGE_3DTRI_REG | S3VIRGE_3DTRI_USTART), ustart); \
	} while (0)

#define DMA_SEND_UVWD() \
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


#define DMA_SEND_COLORS() \
	do { \
		DMAOUT(deltgbx); \
		DMAOUT(deltarx); \
		DMAOUT(deltgby); \
		DMAOUT(deltary); \
		DMAOUT(gbstart); \
		DMAOUT(arstart); \
	} while (0)

#define DMA_SEND_VERTICES() \
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
		DMAOUT(iy0); \
		DMAOUT(((((iy0 - iy1) & 0x7FF) << 16) | \
			((iy1 - iy2) & 0x7FF) | lr)); \
	} while (0)


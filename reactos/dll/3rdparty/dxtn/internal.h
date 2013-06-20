/*
 * Texture compression
 * Version:  1.0
 *
 * Copyright (C) 2004  Daniel Borca   All Rights Reserved.
 *
 * this is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * this is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GNU Make; see the file COPYING.  If not, write to
 * the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.	
 */


#ifndef INTERNAL_H_included
#define INTERNAL_H_included

/*****************************************************************************\
 * DLL stuff
\*****************************************************************************/

#ifdef __WIN32__
#define TAPI __declspec(dllexport)
#define TAPIENTRY __stdcall
#else
#define TAPI
#define TAPIENTRY
#endif


/*****************************************************************************\
 * 64bit types on 32bit machine
\*****************************************************************************/

#if defined(__GNUC__) && !defined(__cplusplus)

typedef unsigned long long qword;

#define Q_MOV32(a, b) a = b
#define Q_OR32(a, b)  a |= b
#define Q_SHL(a, c)   a <<= c

#else  /* !__GNUC__ */

typedef struct {
    dword lo, hi;
} qword;

#define Q_MOV32(a, b) a.lo = b
#define Q_OR32(a, b)  a.lo |= b
#define Q_SHL(a, c)					\
    do {						\
	if ((c) >= 32) {				\
	    a.hi = a.lo << ((c) - 32);			\
	    a.lo = 0;					\
	} else {					\
	    a.hi = (a.hi << (c)) | (a.lo >> (32 - (c)));\
	    a.lo <<= c;					\
	}						\
    } while (0)

#endif /* !__GNUC__ */


/*****************************************************************************\
 * Config
\*****************************************************************************/

#define RCOMP 0
#define GCOMP 1
#define BCOMP 2
#define ACOMP 3


/*****************************************************************************\
 * Metric
\*****************************************************************************/

#define F(i) (float)1 /* can be used to obtain an oblong metric: 0.30 / 0.59 / 0.11 */
#define SAFECDOT 1 /* for paranoids */

#define MAKEIVEC(NV, NC, IV, B, V0, V1)	\
    do {				\
	/* compute interpolation vector */\
	float d2 = 0.0F;		\
	float rd2;			\
					\
	for (i = 0; i < NC; i++) {	\
	    IV[i] = (V1[i] - V0[i]) * F(i);\
	    d2 += IV[i] * IV[i];	\
	}				\
	rd2 = (float)NV / d2;		\
	B = 0;				\
	for (i = 0; i < NC; i++) {	\
	    IV[i] *= F(i);		\
	    B -= IV[i] * V0[i];		\
	    IV[i] *= rd2;		\
	}				\
	B = B * rd2 + 0.5F;		\
    } while (0)

#define CALCCDOT(TEXEL, NV, NC, IV, B, V)\
    do {				\
	float dot = 0.0F;		\
	for (i = 0; i < NC; i++) {	\
	    dot += V[i] * IV[i];	\
	}				\
	TEXEL = (int)(dot + B);		\
	if (SAFECDOT) {			\
	    if (TEXEL < 0) {		\
		TEXEL = 0;		\
	    } else if (TEXEL > NV) {	\
		TEXEL = NV;		\
	    }				\
	}				\
    } while (0)


/*****************************************************************************\
 * Utility functions
\*****************************************************************************/

void
_mesa_upscale_teximage2d (unsigned int inWidth, unsigned int inHeight,
			  unsigned int outWidth, unsigned int outHeight,
			  unsigned int comps,
			  const byte *src, int srcRowStride,
			  unsigned char *dest);

#endif

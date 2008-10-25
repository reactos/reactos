/*
 * Mesa 3-D graphics library
 * Version:  4.0
 *
 * Copyright (C) 1999-2001  Brian Paul   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/* Authors:
 *    David Bucciarelli
 *    Brian Paul
 *    Daryll Strauss
 *    Keith Whitwell
 *    Daniel Borca
 *    Hiroshi Morii
 */


#ifndef __FX_GLIDE_WARPER__
#define __FX_GLIDE_WARPER__


#include "fxg.h"

#ifndef FX_PACKEDCOLOR
#define FX_PACKEDCOLOR 1
#endif

#define MAX_NUM_SST             4

enum {
      GR_SSTTYPE_VOODOO  = 0,
      GR_SSTTYPE_SST96   = 1,
      GR_SSTTYPE_AT3D    = 2,
      GR_SSTTYPE_Voodoo2 = 3,
      GR_SSTTYPE_Banshee = 4,
      GR_SSTTYPE_Voodoo3 = 5,
      GR_SSTTYPE_Voodoo4 = 6,
      GR_SSTTYPE_Voodoo5 = 7
};

#define GrState                 void

typedef int GrSstType;

typedef struct GrTMUConfig_St {
        int tmuRev;		/* Rev of Texelfx chip */
        int tmuRam;		/* 1, 2, or 4 MB */
} GrTMUConfig_t;

typedef struct {
        int num_sst;		/* # of HW units in the system */
        struct SstCard_St {
               GrSstType type;	/* Which hardware is it? */
               int fbRam;	/* 1, 2, or 4 MB */
               int fbiRev;	/* Rev of Pixelfx chip */
               int nTexelfx;	/* How many texelFX chips are there? */
               int numChips;	/* Number of Voodoo chips */
               GrTMUConfig_t tmuConfig[GLIDE_NUM_TMU];	/* Configuration of the Texelfx chips */
               /* Glide3 extensions */
               FxBool HavePalExt;	/* PALETTE6666 */
               FxBool HavePixExt;	/* PIXEXT */
               FxBool HaveTexFmt;	/* TEXFMT */
               FxBool HaveCmbExt;	/* COMBINE */
               FxBool HaveMirExt;	/* TEXMIRROR */
               FxBool HaveTexUma;	/* TEXUMA */
        }
        SSTs[MAX_NUM_SST];	/* configuration for each board */
        struct tdfx_glide Glide;
} GrHwConfiguration;



typedef FxU32 GrHint_t;
#define GR_HINTTYPE_MIN         0
#define GR_HINT_STWHINT         0

typedef FxU32 GrSTWHint_t;
#define GR_STWHINT_W_DIFF_FBI   FXBIT(0)
#define GR_STWHINT_W_DIFF_TMU0  FXBIT(1)
#define GR_STWHINT_ST_DIFF_TMU0 FXBIT(2)
#define GR_STWHINT_W_DIFF_TMU1  FXBIT(3)
#define GR_STWHINT_ST_DIFF_TMU1 FXBIT(4)
#define GR_STWHINT_W_DIFF_TMU2  FXBIT(5)
#define GR_STWHINT_ST_DIFF_TMU2 FXBIT(6)

#define GR_CONTROL_ACTIVATE     1
#define GR_CONTROL_DEACTIVATE   0



/*
** move the vertex layout defintion to application
*/
typedef struct {
        float sow;		/* s texture ordinate (s over w) */
        float tow;		/* t texture ordinate (t over w) */
        float oow;		/* 1/w (used mipmapping - really 0xfff/w) */
} GrTmuVertex;

#if FX_PACKEDCOLOR
typedef struct {
        float x, y;		/* X and Y in screen space */
        float ooz;		/* 65535/Z (used for Z-buffering) */
        float oow;		/* 1/W (used for W-buffering, texturing) */
        unsigned char pargb[4];	/* B, G, R, A [0..255] */
        GrTmuVertex tmuvtx[GLIDE_NUM_TMU];
        float fog;		/* fog coordinate */
        unsigned char pspec[4];	/* B, G, R, A [0..255] */
        float psize;		/* point size */
        long pad[16 - 14];	/* ensure 64b structure */
} GrVertex;

#define GR_VERTEX_X_OFFSET              0
#define GR_VERTEX_Y_OFFSET              1
#define GR_VERTEX_OOZ_OFFSET            2
#define GR_VERTEX_OOW_OFFSET            3
#define GR_VERTEX_PARGB_OFFSET          4
#define GR_VERTEX_SOW_TMU0_OFFSET       5
#define GR_VERTEX_TOW_TMU0_OFFSET       6
#define GR_VERTEX_OOW_TMU0_OFFSET       7
#define GR_VERTEX_SOW_TMU1_OFFSET       8
#define GR_VERTEX_TOW_TMU1_OFFSET       9
#define GR_VERTEX_OOW_TMU1_OFFSET       10
#define GR_VERTEX_FOG_OFFSET            11
#define GR_VERTEX_PSPEC_OFFSET          12
#else  /* !FX_PACKEDCOLOR */
typedef struct {
        float x, y;		/* X and Y in screen space */
        float ooz;		/* 65535/Z (used for Z-buffering) */
        float oow;		/* 1/W (used for W-buffering, texturing) */
        float r, g, b, a;	/* R, G, B, A [0..255] */
        GrTmuVertex tmuvtx[GLIDE_NUM_TMU];
        float fog;		/* fog coordinate */
        float r1, g1, b1;	/* R, G, B [0..255] */
        float psize;		/* point size */
        long pad[20 - 19];	/* ensure multiple of 16 */
} GrVertex;

#define GR_VERTEX_X_OFFSET              0
#define GR_VERTEX_Y_OFFSET              1
#define GR_VERTEX_OOZ_OFFSET            2
#define GR_VERTEX_OOW_OFFSET            3
#define GR_VERTEX_RGB_OFFSET            4
#define GR_VERTEX_A_OFFSET              7
#define GR_VERTEX_SOW_TMU0_OFFSET       8
#define GR_VERTEX_TOW_TMU0_OFFSET       9
#define GR_VERTEX_OOW_TMU0_OFFSET       10
#define GR_VERTEX_SOW_TMU1_OFFSET       11
#define GR_VERTEX_TOW_TMU1_OFFSET       12
#define GR_VERTEX_OOW_TMU1_OFFSET       13
#define GR_VERTEX_FOG_OFFSET            14
#define GR_VERTEX_SPEC_OFFSET           15
#endif /* !FX_PACKEDCOLOR */



/*
 * For Lod/LodLog2 conversion.
 */
#define FX_largeLodLog2(info)		(info).largeLodLog2
#define FX_aspectRatioLog2(info)	(info).aspectRatioLog2
#define FX_smallLodLog2(info)		(info).smallLodLog2
#define FX_lodToValue(val)		((int)(GR_LOD_LOG2_256-val))
#define FX_largeLodValue(info)		((int)(GR_LOD_LOG2_256-(info).largeLodLog2))
#define FX_smallLodValue(info)		((int)(GR_LOD_LOG2_256-(info).smallLodLog2))
#define FX_valueToLod(val)		((GrLOD_t)(GR_LOD_LOG2_256-val))



/*
 * Query
 */
extern int FX_grSstScreenWidth(void);
extern int FX_grSstScreenHeight(void);
extern void FX_grSstPerfStats(GrSstPerfStats_t *st);
extern int FX_grSstQueryHardware(GrHwConfiguration *config);
#define FX_grGetInteger FX_grGetInteger_NoLock
extern FxI32 FX_grGetInteger_NoLock(FxU32 pname);



/*
 * GrHints
 */
#define FX_grHints FX_grHints_NoLock
extern void FX_grHints_NoLock(GrHint_t hintType, FxU32 hintMask);



/*
 * Needed for Glide3 only, to set up Glide2 compatible vertex layout.
 */
extern void FX_setupGrVertexLayout(void);



/*
 * grSstControl stuff
 */
extern FxBool FX_grSstControl(FxU32 code);

#define FX_grBufferClear(c, a, d)	\
  do {					\
    BEGIN_CLIP_LOOP();			\
    grBufferClear(c, a, d);		\
    END_CLIP_LOOP();			\
  } while (0)



#endif /* __FX_GLIDE_WARPER__ */

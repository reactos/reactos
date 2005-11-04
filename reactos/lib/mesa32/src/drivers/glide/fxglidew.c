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

/* fxsetup.c - 3Dfx VooDoo rendering mode setup functions */


#ifdef HAVE_CONFIG_H
#include "conf.h"
#endif

#if defined(FX)
#include "fxglidew.h"
#include "fxdrv.h"

#include <stdlib.h>
#include <string.h>

FxI32
FX_grGetInteger_NoLock(FxU32 pname)
{
 FxI32 result;

 if (grGet(pname, 4, &result)) {
    return result;
 }

 if (TDFX_DEBUG & VERBOSE_DRIVER) {
    fprintf(stderr, "FX_grGetInteger_NoLock: wrong parameter (%lx)\n", pname);
 }
 return -1;
}

FxBool
FX_grSstControl(FxU32 code)
{
   /* The glide 3 sources call for grEnable/grDisable to be called in exchange
    * for grSstControl. */
   switch (code) {
   case GR_CONTROL_ACTIVATE:
      grEnable(GR_PASSTHRU);
      break;
   case GR_CONTROL_DEACTIVATE:
      grDisable(GR_PASSTHRU);
      break;
   }
   /* Appearently GR_CONTROL_RESIZE can be ignored. */
   return 1;			/* OK? */
}


int
FX_grSstScreenWidth()
{
   FxI32 result[4];

   BEGIN_BOARD_LOCK();
   grGet(GR_VIEWPORT, sizeof(FxI32) * 4, result);
   END_BOARD_LOCK();

   return result[2];
}

int
FX_grSstScreenHeight()
{
   FxI32 result[4];

   BEGIN_BOARD_LOCK();
   grGet(GR_VIEWPORT, sizeof(FxI32) * 4, result);
   END_BOARD_LOCK();

   return result[3];
}

void
FX_grSstPerfStats(GrSstPerfStats_t * st)
{
   FxI32 n;
   grGet(GR_STATS_PIXELS_IN, 4, &n);
   st->pixelsIn = n;
   grGet(GR_STATS_PIXELS_CHROMA_FAIL, 4, &n);
   st->chromaFail = n;
   grGet(GR_STATS_PIXELS_DEPTHFUNC_FAIL, 4, &n);
   st->zFuncFail = n;
   grGet(GR_STATS_PIXELS_AFUNC_FAIL, 4, &n);
   st->aFuncFail = n;
   grGet(GR_STATS_PIXELS_OUT, 4, &n);
   st->pixelsOut = n;
}

void
FX_setupGrVertexLayout(void)
{
   BEGIN_BOARD_LOCK();
   grReset(GR_VERTEX_PARAMETER);

   grCoordinateSpace(GR_WINDOW_COORDS);
   grVertexLayout(GR_PARAM_XY, GR_VERTEX_X_OFFSET << 2, GR_PARAM_ENABLE);
#if FX_PACKEDCOLOR
   grVertexLayout(GR_PARAM_PARGB, GR_VERTEX_PARGB_OFFSET << 2, GR_PARAM_ENABLE);
#else  /* !FX_PACKEDCOLOR */
   grVertexLayout(GR_PARAM_RGB, GR_VERTEX_RGB_OFFSET << 2, GR_PARAM_ENABLE);
   grVertexLayout(GR_PARAM_A, GR_VERTEX_A_OFFSET << 2, GR_PARAM_ENABLE);
#endif /* !FX_PACKEDCOLOR */
   grVertexLayout(GR_PARAM_Q, GR_VERTEX_OOW_OFFSET << 2, GR_PARAM_ENABLE);
   grVertexLayout(GR_PARAM_Z, GR_VERTEX_OOZ_OFFSET << 2, GR_PARAM_ENABLE);
   grVertexLayout(GR_PARAM_ST0, GR_VERTEX_SOW_TMU0_OFFSET << 2,
		  GR_PARAM_ENABLE);
   grVertexLayout(GR_PARAM_Q0, GR_VERTEX_OOW_TMU0_OFFSET << 2,
		  GR_PARAM_DISABLE);
   grVertexLayout(GR_PARAM_ST1, GR_VERTEX_SOW_TMU1_OFFSET << 2,
		  GR_PARAM_DISABLE);
   grVertexLayout(GR_PARAM_Q1, GR_VERTEX_OOW_TMU1_OFFSET << 2,
		  GR_PARAM_DISABLE);
   END_BOARD_LOCK();
}

void
FX_grHints_NoLock(GrHint_t hintType, FxU32 hintMask)
{
   switch (hintType) {
   case GR_HINT_STWHINT:
      {
	 if (hintMask & GR_STWHINT_W_DIFF_TMU0)
	    grVertexLayout(GR_PARAM_Q0, GR_VERTEX_OOW_TMU0_OFFSET << 2,
			   GR_PARAM_ENABLE);
	 else
	    grVertexLayout(GR_PARAM_Q0, GR_VERTEX_OOW_TMU0_OFFSET << 2,
			   GR_PARAM_DISABLE);

	 if (hintMask & GR_STWHINT_ST_DIFF_TMU1)
	    grVertexLayout(GR_PARAM_ST1, GR_VERTEX_SOW_TMU1_OFFSET << 2,
			   GR_PARAM_ENABLE);
	 else
	    grVertexLayout(GR_PARAM_ST1, GR_VERTEX_SOW_TMU1_OFFSET << 2,
			   GR_PARAM_DISABLE);

	 if (hintMask & GR_STWHINT_W_DIFF_TMU1)
	    grVertexLayout(GR_PARAM_Q1, GR_VERTEX_OOW_TMU1_OFFSET << 2,
			   GR_PARAM_ENABLE);
	 else
	    grVertexLayout(GR_PARAM_Q1, GR_VERTEX_OOW_TMU1_OFFSET << 2,
			   GR_PARAM_DISABLE);

      }
   }
}

/*
 * Glide3 doesn't have the grSstQueryHardware function anymore.
 * Instead, we call grGet() and fill in the data structures ourselves.
 */
int
FX_grSstQueryHardware(GrHwConfiguration * config)
{
   int i, j;
   int numFB;

   BEGIN_BOARD_LOCK();

   grGet(GR_NUM_BOARDS, 4, (void *) &(config->num_sst));
   if (config->num_sst == 0)
      return 0;

   for (i = 0; i < config->num_sst; i++) {
      FxI32 result;
      const char *extension;

      grSstSelect(i);

      extension = grGetString(GR_HARDWARE);
      if (strstr(extension, "Rush")) {
         config->SSTs[i].type = GR_SSTTYPE_SST96;
      } else if (strstr(extension, "Voodoo2")) {
         config->SSTs[i].type = GR_SSTTYPE_Voodoo2;
      } else if (strstr(extension, "Voodoo Banshee")) {
         config->SSTs[i].type = GR_SSTTYPE_Banshee;
      } else if (strstr(extension, "Voodoo3")) {
         config->SSTs[i].type = GR_SSTTYPE_Voodoo3;
      } else if (strstr(extension, "Voodoo4")) {
         config->SSTs[i].type = GR_SSTTYPE_Voodoo4;
      } else if (strstr(extension, "Voodoo5")) {
         config->SSTs[i].type = GR_SSTTYPE_Voodoo5;
      } else {
         config->SSTs[i].type = GR_SSTTYPE_VOODOO;
      }

      grGet(GR_MEMORY_FB, 4, &result);
      config->SSTs[i].fbRam = result / (1024 * 1024);

      grGet(GR_NUM_TMU, 4, &result);
      config->SSTs[i].nTexelfx = result;

      grGet(GR_REVISION_FB, 4, &result);
      config->SSTs[i].fbiRev = result;

      for (j = 0; j < config->SSTs[i].nTexelfx; j++) {
	 grGet(GR_MEMORY_TMU, 4, &result);
	 config->SSTs[i].tmuConfig[j].tmuRam = result / (1024 * 1024);
	 grGet(GR_REVISION_TMU, 4, &result);
	 config->SSTs[i].tmuConfig[j].tmuRev = result;
      }

      extension = grGetString(GR_EXTENSION);
      config->SSTs[i].HavePalExt = (strstr(extension, " PALETTE6666 ") != NULL);
      config->SSTs[i].HavePixExt = (strstr(extension, " PIXEXT ") != NULL);
      config->SSTs[i].HaveTexFmt = (strstr(extension, " TEXFMT ") != NULL);
      config->SSTs[i].HaveCmbExt = (strstr(extension, " COMBINE ") != NULL);
      config->SSTs[i].HaveMirExt = (strstr(extension, " TEXMIRROR ") != NULL);
      config->SSTs[i].HaveTexUma = (strstr(extension, " TEXUMA ") != NULL);

      /* number of Voodoo chips */
      grGet(GR_NUM_FB, 4, (void *) &numFB);
      config->SSTs[i].numChips = numFB;

   }

   tdfx_hook_glide(&config->Glide, getenv("MESA_FX_POINTCAST") != NULL);

   END_BOARD_LOCK();
   return 1;
}



#else

/*
 * Need this to provide at least one external definition.
 */

extern int gl_fx_dummy_function_glidew(void);
int
gl_fx_dummy_function_glidew(void)
{
   return 0;
}

#endif /* FX */

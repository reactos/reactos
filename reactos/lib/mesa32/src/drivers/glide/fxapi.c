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


/* fxapi.c - public interface to FX/Mesa functions (fxmesa.h) */


#ifdef HAVE_CONFIG_H
#include "conf.h"
#endif

#if defined(FX)
#include "fxdrv.h"

#ifndef TDFX_DEBUG
int TDFX_DEBUG = (0 
/*		  | VERBOSE_VARRAY */
/*		  | VERBOSE_TEXTURE */
/*		  | VERBOSE_IMMEDIATE */
/*		  | VERBOSE_PIPELINE */
/*		  | VERBOSE_DRIVER */
/*		  | VERBOSE_STATE */
/*		  | VERBOSE_API */
/*		  | VERBOSE_DISPLAY_LIST */
/*		  | VERBOSE_LIGHTING */
/*		  | VERBOSE_PRIMS */
/*		  | VERBOSE_VERTS */
   );
#endif

static fxMesaContext fxMesaCurrentCtx = NULL;

/*
 * Status of 3Dfx hardware initialization
 */

static int glbGlideInitialized = 0;
static int glb3DfxPresent = 0;
static int glbTotNumCtx = 0;

static GrHwConfiguration glbHWConfig;
static int glbCurrentBoard = 0;


#if defined(__WIN32__)
static int
cleangraphics(void)
{
   glbTotNumCtx = 1;
   fxMesaDestroyContext(fxMesaCurrentCtx);

   return 0;
}
#elif defined(__linux__)
static void
cleangraphics(void)
{
   glbTotNumCtx = 1;
   fxMesaDestroyContext(fxMesaCurrentCtx);
}

static void
cleangraphics_handler(int s)
{
   fprintf(stderr, "fxmesa: ERROR: received a not handled signal %d\n", s);

   cleangraphics();
/*    abort(); */
   exit(1);
}
#endif


/*
 * Query 3Dfx hardware presence/kind
 */
static GLboolean GLAPIENTRY fxQueryHardware (void)
{
 if (TDFX_DEBUG & VERBOSE_DRIVER) {
    fprintf(stderr, "fxQueryHardware()\n");
 }

 if (!glbGlideInitialized) {
    grGlideInit();
    glb3DfxPresent = FX_grSstQueryHardware(&glbHWConfig);

    glbGlideInitialized = 1;

#if defined(__WIN32__)
    _onexit((_onexit_t) cleangraphics);
#elif defined(__linux__)
    /* Only register handler if environment variable is not defined. */
    if (!getenv("MESA_FX_NO_SIGNALS")) {
       atexit(cleangraphics);
    }
#endif
 }

 return glb3DfxPresent;
}


/*
 * Select the Voodoo board to use when creating
 * a new context.
 */
GLint GLAPIENTRY fxMesaSelectCurrentBoard (int n)
{
   fxQueryHardware();

   if ((n < 0) || (n >= glbHWConfig.num_sst))
      return -1;

   return glbHWConfig.SSTs[glbCurrentBoard = n].type;
}


void * GLAPIENTRY fxMesaGetCurrentContext (void)
{
 GET_CURRENT_CONTEXT(ctx);
 return ctx;
}


void GLAPIENTRY fxGetScreenGeometry (GLint *w, GLint *h)
{
 GLint width = 0;
 GLint height = 0;
 
 if (fxMesaCurrentCtx != NULL) {
    width = fxMesaCurrentCtx->screen_width;
    height = fxMesaCurrentCtx->screen_height;
 }

 if (w != NULL) {
    *w = width;
 }
 if (h != NULL) {
    *h = height;
 }
}


/*
 * The 3Dfx Global Palette extension for GLQuake.
 * More a trick than a real extesion, use the shared global
 * palette extension. 
 */
extern void GLAPIENTRY gl3DfxSetPaletteEXT(GLuint * pal);	/* silence warning */
void GLAPIENTRY
gl3DfxSetPaletteEXT(GLuint * pal)
{
   fxMesaContext fxMesa = fxMesaCurrentCtx;

   if (TDFX_DEBUG & VERBOSE_DRIVER) {
      int i;

      fprintf(stderr, "gl3DfxSetPaletteEXT(...)\n");

      for (i = 0; i < 256; i++) {
	 fprintf(stderr, "\t%x\n", pal[i]);
      }
   }

   if (fxMesa) {
      fxMesa->haveGlobalPaletteTexture = 1;

      grTexDownloadTable(GR_TEXTABLE_PALETTE, (GuTexPalette *) pal);
   }
}


static GrScreenResolution_t fxBestResolution (int width, int height)
{
 static int resolutions[][5] = {
#if 1 /* Voodoo^2 can't fullscreen these, because of Glide */
        { 320,  200, GR_RESOLUTION_320x200  },
        { 320,  240, GR_RESOLUTION_320x240  },
#endif
        { 512,  384, GR_RESOLUTION_512x384  },
        { 640,  400, GR_RESOLUTION_640x400  },
        { 640,  480, GR_RESOLUTION_640x480  },
        { 800,  600, GR_RESOLUTION_800x600  },
        {1024,  768, GR_RESOLUTION_1024x768 },
        {1280, 1024, GR_RESOLUTION_1280x1024},
        {1600, 1200, GR_RESOLUTION_1600x1200},
 };

 int i, NUM_RESOLUTIONS = sizeof(resolutions) / sizeof(resolutions[0]);
 int lastvalidres = 4;  /* set default to GR_RESOLUTION_640x480 */
 int min = 2048 * 2048; /* max is GR_RESOLUTION_2048x2048 */

 for (i = 0; i < NUM_RESOLUTIONS; i++) {
     if ((width <= resolutions[i][0]) && (height <= resolutions[i][1])) {
        if (min > (resolutions[i][0] * resolutions[i][1])) {
           min = resolutions[i][0] * resolutions[i][1];
           lastvalidres = i;
        }
     }
 }

 return resolutions[lastvalidres][2];
}


fxMesaContext GLAPIENTRY
fxMesaCreateBestContext(GLuint win, GLint width, GLint height,
			const GLint attribList[])
{
 int res = fxBestResolution(width, height);

 if (res == -1) {
    return NULL;
 }

 return fxMesaCreateContext(win, res, GR_REFRESH_60Hz, attribList);
}


/*
 * Create a new FX/Mesa context and return a handle to it.
 */
fxMesaContext GLAPIENTRY
fxMesaCreateContext(GLuint win,
		    GrScreenResolution_t res,
		    GrScreenRefresh_t ref, const GLint attribList[])
{
 fxMesaContext fxMesa = NULL;
 GLcontext *ctx = NULL, *shareCtx = NULL;

 int i;
 const char *str;
 int sliaa, numSLI, samplesPerChip, tmuRam, fbRam;
 struct SstCard_St *voodoo;
 struct tdfx_glide *Glide;

 GLboolean aux;
 GLboolean doubleBuffer;
 GLuint colDepth;
 GLuint depthSize, alphaSize, stencilSize, accumSize;
 GLuint redBits, greenBits, blueBits, alphaBits;
 GrPixelFormat_t pixFmt;
   
 GLboolean useBGR;

 if (TDFX_DEBUG & VERBOSE_DRIVER) {
    fprintf(stderr, "fxMesaCreateContext(...)\n");
 }

 /* Okay, first process the user flags */
 aux = GL_FALSE;
 doubleBuffer = GL_FALSE;
 colDepth = 16;
 depthSize = alphaSize = stencilSize = accumSize = 0;

 i = 0;
 while (attribList[i] != FXMESA_NONE) {
       switch (attribList[i]) {
              case FXMESA_COLORDEPTH:
	           colDepth = attribList[++i];
	           break;
              case FXMESA_DOUBLEBUFFER:
	           doubleBuffer = GL_TRUE;
	           break;
              case FXMESA_ALPHA_SIZE:
	           if ((alphaSize = attribList[++i])) {
	              aux = GL_TRUE;
                   }
	           break;
              case FXMESA_DEPTH_SIZE:
	           if ((depthSize = attribList[++i])) {
	              aux = GL_TRUE;
                   }
	           break;
              case FXMESA_STENCIL_SIZE:
	           stencilSize = attribList[++i];
	           break;
              case FXMESA_ACCUM_SIZE:
	           accumSize = attribList[++i];
	           break;
              /* XXX ugly hack here for sharing display lists */
              case FXMESA_SHARE_CONTEXT:
	           {
	            const void *vPtr = &attribList[++i];
	            GLcontext **ctx = (GLcontext **)vPtr;
	            shareCtx = *ctx;
                   }
	           break;
              default:
                   fprintf(stderr, "fxMesaCreateContext: ERROR: wrong parameter (%d) passed\n", attribList[i]);
	           return NULL;
       }
       i++;
 }

 if (!fxQueryHardware()) {
    str = "no Voodoo hardware!";
    goto errorhandler;
 }

 grSstSelect(glbCurrentBoard);
 /*grEnable(GR_OPENGL_MODE_EXT);*/ /* ZZZ: trick to make GL happy.
                                  Glide3 will unmap memory for card when grSstWinClose is called.
                                  This also forces the SLI band height to be 32 (above 1024x768) or 16
                                  and disables the splash screen due to y-origin swapping.
                                  Note: We only want the former. */
 voodoo = &glbHWConfig.SSTs[glbCurrentBoard];

 fxMesa = (fxMesaContext)CALLOC_STRUCT(tfxMesaContext);
 if (!fxMesa) {
    str = "private context";
    goto errorhandler;
 }

 if (getenv("MESA_FX_INFO")) {
    fxMesa->verbose = GL_TRUE;
 }

 fxMesa->type = voodoo->type;
 fxMesa->HavePalExt = voodoo->HavePalExt && !getenv("MESA_FX_IGNORE_PALEXT");
 fxMesa->HavePixExt = voodoo->HavePixExt && !getenv("MESA_FX_IGNORE_PIXEXT");
 fxMesa->HaveTexFmt = voodoo->HaveTexFmt && !getenv("MESA_FX_IGNORE_TEXFMT");
 fxMesa->HaveCmbExt = voodoo->HaveCmbExt && !getenv("MESA_FX_IGNORE_CMBEXT");
 fxMesa->HaveMirExt = voodoo->HaveMirExt && !getenv("MESA_FX_IGNORE_MIREXT");
 fxMesa->HaveTexUma = voodoo->HaveTexUma && !getenv("MESA_FX_IGNORE_TEXUMA");
 fxMesa->Glide = glbHWConfig.Glide;
 Glide = &fxMesa->Glide;
 fxMesa->HaveTexus2 = Glide->txImgQuantize &&
                      Glide->txMipQuantize &&
                      Glide->txPalToNcc && !getenv("MESA_FX_IGNORE_TEXUS2");

 /*
  * Pixel tables are used during pixel read-back
  * Either initialize them for RGB or BGR order;
  * However, 32bit capable cards have the right order.
  * As a consequence, 32bit read-back is not swizzled!
  * Also determine if we need vertex snapping.
  */
 /* number of SLI units and AA Samples per chip */
 sliaa = 0;
 switch (voodoo->type) {
        case GR_SSTTYPE_VOODOO:
        case GR_SSTTYPE_Banshee:
             useBGR = GL_TRUE;
             fxMesa->snapVertices = GL_TRUE;
             break;
        case GR_SSTTYPE_Voodoo2:
             useBGR = GL_TRUE;
             fxMesa->snapVertices = GL_FALSE;
             break;
        case GR_SSTTYPE_Voodoo4:
        case GR_SSTTYPE_Voodoo5:
             if ((str = Glide->grGetRegistryOrEnvironmentStringExt("SSTH3_SLI_AA_CONFIGURATION")) != NULL) {
                sliaa = atoi(str);
             }
        case GR_SSTTYPE_Voodoo3:
        default:
             useBGR = GL_FALSE;
             fxMesa->snapVertices = GL_FALSE;
             break;
 }
 /* ZZZ TO DO: Add the old SLI/AA settings for Napalm. */
 switch(voodoo->numChips) {
 case 4: /* 4 chips */
   switch(sliaa) {
   case 8: /* 8 Sample AA */
     numSLI         = 1;
     samplesPerChip = 2;
     break;
   case 7: /* 4 Sample AA */
     numSLI         = 1;
     samplesPerChip = 1;
     break;
   case 6: /* 2 Sample AA */
     numSLI         = 2;
     samplesPerChip = 1;
     break;
   default:
     numSLI         = 4;
     samplesPerChip = 1;
   }
   break;
 case 2: /* 2 chips */
   switch(sliaa) {
   case 4: /* 4 Sample AA */
     numSLI         = 1;
     samplesPerChip = 2;
     break;
   case 3: /* 2 Sample AA */
     numSLI         = 1;
     samplesPerChip = 1;
     break;
   default:
     numSLI         = 2;
     samplesPerChip = 1;
   }
   break;
 default: /* 1 chip */
   switch(sliaa) {
   case 1: /* 2 Sample AA */
     numSLI         = 1;
     samplesPerChip = 2;
     break;
   default:
     numSLI         = 1;
     samplesPerChip = 1;
   }
 }

 fxMesa->fsaa = samplesPerChip * voodoo->numChips / numSLI; /* 1:noFSAA, 2:2xFSAA, 4:4xFSAA, 8:8xFSAA */

 switch (fxMesa->colDepth = colDepth) {
   case 15:
     redBits   = 5;
     greenBits = 5;
     blueBits  = 5;
     alphaBits = depthSize ? 1 : 8;
     switch(fxMesa->fsaa) {
       case 8:
         pixFmt = GR_PIXFMT_AA_8_ARGB_1555;
         break;
       case 4:
         pixFmt = GR_PIXFMT_AA_4_ARGB_1555;
         break;
       case 2:
         pixFmt = GR_PIXFMT_AA_2_ARGB_1555;
         break;
       default:
         pixFmt = GR_PIXFMT_ARGB_1555;
     }
     break;
   case 16:
     redBits   = 5;
     greenBits = 6;
     blueBits  = 5;
     alphaBits = depthSize ? 0 : 8;
     switch(fxMesa->fsaa) {
       case 8:
         pixFmt = GR_PIXFMT_AA_8_RGB_565;
         break;
       case 4:
         pixFmt = GR_PIXFMT_AA_4_RGB_565;
         break;
       case 2:
         pixFmt = GR_PIXFMT_AA_2_RGB_565;
         break;
       default:
         pixFmt = GR_PIXFMT_RGB_565;
     }
     break;
   case 32:
     redBits   = 8;
     greenBits = 8;
     blueBits  = 8;
     alphaBits = 8;
     switch(fxMesa->fsaa) {
       case 8:
         pixFmt = GR_PIXFMT_AA_8_ARGB_8888;
         break;
       case 4:
         pixFmt = GR_PIXFMT_AA_4_ARGB_8888;
         break;
       case 2:
         pixFmt = GR_PIXFMT_AA_2_ARGB_8888;
         break;
       default:
         pixFmt = GR_PIXFMT_ARGB_8888;
     }
     break;
   default:
     str = "pixelFormat";
     goto errorhandler;
 }

 /* ZZZ TODO: check if there is enough fbRam */

 /* Tips:
  * 1. we don't bother setting/checking AUX for stencil, because we'll decide
  *    later whether we have HW stencil, based on depth buffer (thus AUX is
  *    properly set)
  * 2. when both DEPTH and ALPHA are enabled, depth should win. However, it is
  *    not clear whether 15bpp and 32bpp require AUX alpha buffer. Furthermore,
  *    alpha buffering is required only if destination alpha is used in alpha
  *    blending; alpha blending modes that do not use destination alpha can be
  *    used w/o alpha buffer.
  * 3. `alphaBits' is what we can provide
  *    `alphaSize' is what app requests
  *    if we cannot provide enough bits for alpha buffer, we should fallback to
  *    SW alpha. However, setting `alphaBits' to `alphaSize' might confuse some
  *    of the span functions...
  */

 fxMesa->haveHwAlpha = GL_FALSE;
 if (alphaSize && (alphaSize <= alphaBits)) {
    alphaSize = alphaBits;
    fxMesa->haveHwAlpha = GL_TRUE;
 }

 fxMesa->haveHwStencil = (fxMesa->HavePixExt && stencilSize && depthSize == 24);

 fxMesa->haveZBuffer = depthSize > 0;
 fxMesa->haveDoubleBuffer = doubleBuffer;
 fxMesa->haveGlobalPaletteTexture = GL_FALSE;
 fxMesa->board = glbCurrentBoard;

 fxMesa->haveTwoTMUs = (voodoo->nTexelfx > 1);

 if ((str = Glide->grGetRegistryOrEnvironmentStringExt("FX_GLIDE_NUM_TMU"))) {
    if (atoi(str) <= 1) {
       fxMesa->haveTwoTMUs = GL_FALSE;
    }
 }

 if ((str = Glide->grGetRegistryOrEnvironmentStringExt("FX_GLIDE_SWAPPENDINGCOUNT"))) {
    fxMesa->maxPendingSwapBuffers = atoi(str);
    if (fxMesa->maxPendingSwapBuffers > 3) {
       fxMesa->maxPendingSwapBuffers = 3;
    } else if (fxMesa->maxPendingSwapBuffers < 0) {
       fxMesa->maxPendingSwapBuffers = 0;
    }
 } else {
    fxMesa->maxPendingSwapBuffers = 2;
 }

 if ((str = Glide->grGetRegistryOrEnvironmentStringExt("FX_GLIDE_SWAPINTERVAL"))) {
    fxMesa->swapInterval = atoi(str);
 } else {
    fxMesa->swapInterval = 0;
 }

 BEGIN_BOARD_LOCK();
 if (fxMesa->HavePixExt) {
    fxMesa->glideContext = Glide->grSstWinOpenExt((FxU32)win, res, ref,
                                                  GR_COLORFORMAT_ABGR, GR_ORIGIN_LOWER_LEFT,
                                                  pixFmt,
                                                  2, aux);
 } else if (pixFmt == GR_PIXFMT_RGB_565) {
    fxMesa->glideContext = grSstWinOpen((FxU32)win, res, ref,
                                        GR_COLORFORMAT_ABGR, GR_ORIGIN_LOWER_LEFT,
                                        2, aux);
 } else {
    fxMesa->glideContext = 0;
 }
 END_BOARD_LOCK();
 if (!fxMesa->glideContext) {
    str = "grSstWinOpen";
    goto errorhandler;
 }

  /* Not that it matters, but tmuRam and fbRam change after grSstWinOpen. */
  tmuRam = voodoo->tmuConfig[GR_TMU0].tmuRam;
  fbRam  = voodoo->fbRam;
  BEGIN_BOARD_LOCK();
  {
    FxI32 result;
    grGet(GR_MEMORY_TMU, 4, &result);
    tmuRam = result / (1024 * 1024);
    grGet(GR_MEMORY_FB, 4, &result);
    fbRam = result / (1024 * 1024);
  }
  END_BOARD_LOCK();

  sprintf(fxMesa->rendererString, "Mesa %s v0.60 %s %dMB FB, %dMB TM, %d TMU, %s",
                           grGetString(GR_RENDERER),
                           grGetString(GR_HARDWARE),
                           fbRam,
                           tmuRam * voodoo->nTexelfx,
                           voodoo->nTexelfx,
                           (voodoo->numChips > 1) ? "SLI" : "NOSLI");

  fxMesa->bgrOrder = useBGR;

   /* screen */
   fxMesa->screen_width = FX_grSstScreenWidth();
   fxMesa->screen_height = FX_grSstScreenHeight();

   /* window inside screen */
   fxMesa->width = fxMesa->screen_width;
   fxMesa->height = fxMesa->screen_height;

   /* scissor inside window */
   fxMesa->clipMinX = 0;
   fxMesa->clipMaxX = fxMesa->width;
   fxMesa->clipMinY = 0;
   fxMesa->clipMaxY = fxMesa->height;

   if (fxMesa->verbose) {
      char buf[80];

      strcpy(buf, grGetString(GR_VERSION));
      fprintf(stderr, "Voodoo Using Glide %s\n", buf);
      fprintf(stderr, "Voodoo Number of boards: %d\n", glbHWConfig.num_sst);
      fprintf(stderr, "Voodoo Number of TMUs: %d\n", voodoo->nTexelfx);
      fprintf(stderr, "Voodoo fbRam: %d\n", voodoo->fbRam);
      fprintf(stderr, "Voodoo fbiRev: %d\n", voodoo->fbiRev);
      fprintf(stderr, "Voodoo chips detected: %d\n", voodoo->numChips);
      fprintf(stderr, "Voodoo pixel order = %s, vertex snapping = %d\n",
                      useBGR ? "BGR" : "RGB",
                      fxMesa->snapVertices);
      fprintf(stderr, "Voodoo screen: %dx%d:%d\n",
	              fxMesa->screen_width, fxMesa->screen_height, colDepth);
   }

   fxMesa->glVis = _mesa_create_visual(GL_TRUE,		/* RGB mode */
				       doubleBuffer,
				       GL_FALSE,	/* stereo */
				       redBits,		/* RGBA.R bits */
				       greenBits,	/* RGBA.G bits */
				       blueBits,	/* RGBA.B bits */
				       alphaSize,	/* RGBA.A bits */
				       0,		/* index bits */
				       depthSize,	/* depth_size */
				       stencilSize,	/* stencil_size */
				       accumSize,
				       accumSize,
				       accumSize,
				       alphaSize ? accumSize : 0,
                                       1);
   if (!fxMesa->glVis) {
      str = "_mesa_create_visual";
      goto errorhandler;
   }

   ctx = fxMesa->glCtx = _mesa_create_context(fxMesa->glVis,
                                              shareCtx,
					      (void *) fxMesa, GL_TRUE);
   if (!ctx) {
      str = "_mesa_create_context";
      goto errorhandler;
   }


   if (!fxDDInitFxMesaContext(fxMesa)) {
      str = "fxDDInitFxMesaContext";
      goto errorhandler;
   }


   fxMesa->glBuffer = _mesa_create_framebuffer(fxMesa->glVis,
					       GL_FALSE,	/* no software depth */
					       stencilSize && !fxMesa->haveHwStencil,
					       fxMesa->glVis->accumRedBits > 0,
					       alphaSize && !fxMesa->haveHwAlpha);
   if (!fxMesa->glBuffer) {
      str = "_mesa_create_framebuffer";
      goto errorhandler;
   }

   glbTotNumCtx++;

   /* install signal handlers */
#if defined(__linux__)
   /* Only install if environment var. is not set. */
   if (fxMesa->glCtx->CatchSignals && !getenv("MESA_FX_NO_SIGNALS")) {
      signal(SIGINT, cleangraphics_handler);
      signal(SIGHUP, cleangraphics_handler);
      signal(SIGPIPE, cleangraphics_handler);
      signal(SIGFPE, cleangraphics_handler);
      signal(SIGBUS, cleangraphics_handler);
      signal(SIGILL, cleangraphics_handler);
      signal(SIGSEGV, cleangraphics_handler);
      signal(SIGTERM, cleangraphics_handler);
   }
#endif

   return fxMesa;

errorhandler:
 if (fxMesa) {
    if (fxMesa->glideContext) {
       grSstWinClose(fxMesa->glideContext);
       fxMesa->glideContext = 0;
    }

    if (fxMesa->state) {
       FREE(fxMesa->state);
    }
    if (fxMesa->fogTable) {
       FREE(fxMesa->fogTable);
    }
    if (fxMesa->glBuffer) {
       _mesa_destroy_framebuffer(fxMesa->glBuffer);
    }
    if (fxMesa->glVis) {
       _mesa_destroy_visual(fxMesa->glVis);
    }
    if (fxMesa->glCtx) {
       _mesa_destroy_context(fxMesa->glCtx);
    }
    FREE(fxMesa);
 }

 fprintf(stderr, "fxMesaCreateContext: ERROR: %s\n", str);
 return NULL;
}


/*
 * Function to set the new window size in the context (mainly for the Voodoo Rush)
 */
void GLAPIENTRY
fxMesaUpdateScreenSize(fxMesaContext fxMesa)
{
   fxMesa->width = FX_grSstScreenWidth();
   fxMesa->height = FX_grSstScreenHeight();
}


/*
 * Destroy the given FX/Mesa context.
 */
void GLAPIENTRY
fxMesaDestroyContext(fxMesaContext fxMesa)
{
   if (TDFX_DEBUG & VERBOSE_DRIVER) {
      fprintf(stderr, "fxMesaDestroyContext(...)\n");
   }

   if (!fxMesa)
      return;

   if (fxMesa->verbose) {
      fprintf(stderr, "Misc Stats:\n");
      fprintf(stderr, "  # swap buffer: %u\n", fxMesa->stats.swapBuffer);

      if (!fxMesa->stats.swapBuffer)
	 fxMesa->stats.swapBuffer = 1;

      fprintf(stderr, "Textures Stats:\n");
      fprintf(stderr, "  Free texture memory on TMU0: %d:\n",
	      fxMesa->freeTexMem[FX_TMU0]);
      if (fxMesa->haveTwoTMUs)
	 fprintf(stderr, "  Free texture memory on TMU1: %d:\n",
		 fxMesa->freeTexMem[FX_TMU1]);
      fprintf(stderr, "  # request to TMM to upload a texture objects: %u\n",
	      fxMesa->stats.reqTexUpload);
      fprintf(stderr,
	      "  # request to TMM to upload a texture objects per swapbuffer: %.2f\n",
	      fxMesa->stats.reqTexUpload / (float) fxMesa->stats.swapBuffer);
      fprintf(stderr, "  # texture objects uploaded: %u\n",
	      fxMesa->stats.texUpload);
      fprintf(stderr, "  # texture objects uploaded per swapbuffer: %.2f\n",
	      fxMesa->stats.texUpload / (float) fxMesa->stats.swapBuffer);
      fprintf(stderr, "  # MBs uploaded to texture memory: %.2f\n",
	      fxMesa->stats.memTexUpload / (float) (1 << 20));
      fprintf(stderr,
	      "  # MBs uploaded to texture memory per swapbuffer: %.2f\n",
	      (fxMesa->stats.memTexUpload /
	       (float) fxMesa->stats.swapBuffer) / (float) (1 << 20));
   }

   glbTotNumCtx--;

   fxDDDestroyFxMesaContext(fxMesa);
   _mesa_destroy_visual(fxMesa->glVis);
   _mesa_destroy_context(fxMesa->glCtx);
   _mesa_destroy_framebuffer(fxMesa->glBuffer);

   grSstWinClose(fxMesa->glideContext);
   fxCloseHardware();

   FREE(fxMesa);

   if (fxMesa == fxMesaCurrentCtx)
      fxMesaCurrentCtx = NULL;
}


/*
 * Make the specified FX/Mesa context the current one.
 */
void GLAPIENTRY
fxMesaMakeCurrent(fxMesaContext fxMesa)
{
   if (!fxMesa) {
      _mesa_make_current(NULL, NULL);
      fxMesaCurrentCtx = NULL;

      if (TDFX_DEBUG & VERBOSE_DRIVER) {
	 fprintf(stderr, "fxMesaMakeCurrent(NULL)\n");
      }

      return;
   }

   /* if this context is already the current one, we can return early */
   if (fxMesaCurrentCtx == fxMesa
       && fxMesaCurrentCtx->glCtx == _mesa_get_current_context()) {
      if (TDFX_DEBUG & VERBOSE_DRIVER) {
	 fprintf(stderr, "fxMesaMakeCurrent(NOP)\n");
      }

      return;
   }

   if (TDFX_DEBUG & VERBOSE_DRIVER) {
      fprintf(stderr, "fxMesaMakeCurrent(...)\n");
   }

   if (fxMesaCurrentCtx)
      grGlideGetState((GrState *) fxMesaCurrentCtx->state);

   fxMesaCurrentCtx = fxMesa;

   grSstSelect(fxMesa->board);
   grGlideSetState((GrState *) fxMesa->state);

   _mesa_make_current(fxMesa->glCtx, fxMesa->glBuffer);

   fxSetupDDPointers(fxMesa->glCtx);

   /* The first time we call MakeCurrent we set the initial viewport size */
   if (fxMesa->glCtx->Viewport.Width == 0)
      _mesa_set_viewport(fxMesa->glCtx, 0, 0, fxMesa->width, fxMesa->height);
}


/*
 * Swap front/back buffers for current context if double buffered.
 */
void GLAPIENTRY
fxMesaSwapBuffers(void)
{
   if (TDFX_DEBUG & VERBOSE_DRIVER) {
      fprintf(stderr, "fxMesaSwapBuffers()\n");
   }

   if (fxMesaCurrentCtx) {
      _mesa_notifySwapBuffers(fxMesaCurrentCtx->glCtx);

      if (fxMesaCurrentCtx->haveDoubleBuffer) {

	 grBufferSwap(fxMesaCurrentCtx->swapInterval);

	 /*
	  * Don't allow swap buffer commands to build up!
	  */
	 while (FX_grGetInteger(GR_PENDING_BUFFERSWAPS) >
		fxMesaCurrentCtx->maxPendingSwapBuffers)
	    /* The driver is able to sleep when waiting for the completation
	       of multiple swapbuffer operations instead of wasting
	       CPU time (NOTE: you must uncomment the following line in the
	       in order to enable this option) */
	    /* usleep(10000); */
	    ;

	 fxMesaCurrentCtx->stats.swapBuffer++;
      }
   }
}


/*
 * Shutdown Glide library
 */
void GLAPIENTRY
fxCloseHardware(void)
{
   if (glbGlideInitialized) {
      if (fxMesaCurrentCtx && fxMesaCurrentCtx->verbose) {
	 GrSstPerfStats_t st;

	 FX_grSstPerfStats(&st);
	 fprintf(stderr, "Pixels Stats:\n");
	 fprintf(stderr, "  # pixels processed (minus buffer clears): %u\n",
		 (unsigned) st.pixelsIn);
	 fprintf(stderr,
		 "  # pixels not drawn due to chroma key test failure: %u\n",
		 (unsigned) st.chromaFail);
	 fprintf(stderr,
		 "  # pixels not drawn due to depth test failure: %u\n",
		 (unsigned) st.zFuncFail);
	 fprintf(stderr,
		 "  # pixels not drawn due to alpha test failure: %u\n",
		 (unsigned) st.aFuncFail);
	 fprintf(stderr,
		 "  # pixels drawn (including buffer clears and LFB writes): %u\n",
		 (unsigned) st.pixelsOut);
      }

      if (glbTotNumCtx == 0) {
	 grGlideShutdown();
	 glbGlideInitialized = 0;
      }
   }
}


#else


/*
 * Need this to provide at least one external definition.
 */
extern int gl_fx_dummy_function_api(void);
int
gl_fx_dummy_function_api(void)
{
   return 0;
}

#endif /* FX */

/*
 * Mesa 3-D graphics library
 * Version:  7.0.1
 *
 * Copyright (C) 1999-2007  Brian Paul   All Rights Reserved.
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


/*
 * This is an emulation of the GLX API which allows Mesa/GLX-based programs
 * to run on X servers which do not have the real GLX extension.
 *
 * Thanks to the contributors:
 *
 * Initial version:  Philip Brown (phil@bolthole.com)
 * Better glXGetConfig() support: Armin Liebchen (liebchen@asylum.cs.utah.edu)
 * Further visual-handling refinements: Wolfram Gloger
 *    (wmglo@Dent.MED.Uni-Muenchen.DE).
 *
 * Notes:
 *   Don't be fooled, stereo isn't supported yet.
 */



#include "glxheader.h"
#include "glxapi.h"
#include "GL/xmesa.h"
#include "context.h"
#include "config.h"
#include "macros.h"
#include "imports.h"
#include "mtypes.h"
#include "version.h"
#include "xfonts.h"
#include "xmesaP.h"

#ifdef __VMS
#define _mesa_sprintf sprintf
#endif

/* This indicates the client-side GLX API and GLX encoder version. */
#define CLIENT_MAJOR_VERSION 1
#define CLIENT_MINOR_VERSION 4  /* but don't have 1.3's pbuffers, etc yet */

/* This indicates the server-side GLX decoder version.
 * GLX 1.4 indicates OpenGL 1.3 support
 */
#define SERVER_MAJOR_VERSION 1
#define SERVER_MINOR_VERSION 4

/* This is appended onto the glXGetClient/ServerString version strings. */
#define MESA_GLX_VERSION "Mesa " MESA_VERSION_STRING

/* Who implemented this GLX? */
#define VENDOR "Brian Paul"

#define EXTENSIONS \
   "GLX_MESA_set_3dfx_mode " \
   "GLX_MESA_copy_sub_buffer " \
   "GLX_MESA_pixmap_colormap " \
   "GLX_MESA_release_buffers " \
   "GLX_ARB_get_proc_address " \
   "GLX_EXT_visual_info " \
   "GLX_EXT_visual_rating " \
   /*"GLX_SGI_video_sync "*/ \
   "GLX_SGIX_fbconfig " \
   "GLX_SGIX_pbuffer "

/*
 * Our fake GLX context will contain a "real" GLX context and an XMesa context.
 *
 * Note that a pointer to a __GLXcontext is a pointer to a fake_glx_context,
 * and vice versa.
 *
 * We really just need this structure in order to make the libGL functions
 * glXGetCurrentContext(), glXGetCurrentDrawable() and glXGetCurrentDisplay()
 * work correctly.
 */
struct fake_glx_context {
   __GLXcontext glxContext;   /* this MUST be first! */
   XMesaContext xmesaContext;
};



/**********************************************************************/
/***                       GLX Visual Code                          ***/
/**********************************************************************/

#define DONT_CARE -1


static XMesaVisual *VisualTable = NULL;
static int NumVisuals = 0;


/*
 * This struct and some code fragments borrowed
 * from Mark Kilgard's GLUT library.
 */
typedef struct _OverlayInfo {
  /* Avoid 64-bit portability problems by being careful to use
     longs due to the way XGetWindowProperty is specified. Note
     that these parameters are passed as CARD32s over X
     protocol. */
  unsigned long overlay_visual;
  long transparent_type;
  long value;
  long layer;
} OverlayInfo;



/* Macro to handle c_class vs class field name in XVisualInfo struct */
#if defined(__cplusplus) || defined(c_plusplus)
#define CLASS c_class
#else
#define CLASS class
#endif



/*
 * Test if the given XVisualInfo is usable for Mesa rendering.
 */
static GLboolean
is_usable_visual( XVisualInfo *vinfo )
{
   switch (vinfo->CLASS) {
      case StaticGray:
      case GrayScale:
         /* Any StaticGray/GrayScale visual works in RGB or CI mode */
         return GL_TRUE;
      case StaticColor:
      case PseudoColor:
	 /* Any StaticColor/PseudoColor visual of at least 4 bits */
	 if (vinfo->depth>=4) {
	    return GL_TRUE;
	 }
	 else {
	    return GL_FALSE;
	 }
      case TrueColor:
      case DirectColor:
	 /* Any depth of TrueColor or DirectColor works in RGB mode */
	 return GL_TRUE;
      default:
	 /* This should never happen */
	 return GL_FALSE;
   }
}



/**
 * Get an array OverlayInfo records for specified screen.
 * \param dpy  the display
 * \param screen  screen number
 * \param numOverlays  returns numver of OverlayInfo records
 * \return  pointer to OverlayInfo array, free with XFree()
 */
static OverlayInfo *
GetOverlayInfo(Display *dpy, int screen, int *numOverlays)
{
   Atom overlayVisualsAtom;
   Atom actualType;
   Status status;
   unsigned char *ovInfo;
   unsigned long sizeData, bytesLeft;
   int actualFormat;

   /*
    * The SERVER_OVERLAY_VISUALS property on the root window contains
    * a list of overlay visuals.  Get that list now.
    */
   overlayVisualsAtom = XInternAtom(dpy,"SERVER_OVERLAY_VISUALS", True);
   if (overlayVisualsAtom == None) {
      return 0;
   }

   status = XGetWindowProperty(dpy, RootWindow(dpy, screen),
                               overlayVisualsAtom, 0L, (long) 10000, False,
                               overlayVisualsAtom, &actualType, &actualFormat,
                               &sizeData, &bytesLeft,
                               &ovInfo);

   if (status != Success || actualType != overlayVisualsAtom ||
       actualFormat != 32 || sizeData < 4) {
      /* something went wrong */
      XFree((void *) ovInfo);
      *numOverlays = 0;
      return NULL;
   }

   *numOverlays = sizeData / 4;
   return (OverlayInfo *) ovInfo;
}



/**
 * Return the level (overlay, normal, underlay) of a given XVisualInfo.
 * Input:  dpy - the X display
 *         vinfo - the XVisualInfo to test
 * Return:  level of the visual:
 *             0 = normal planes
 *            >0 = overlay planes
 *            <0 = underlay planes
 */
static int
level_of_visual( Display *dpy, XVisualInfo *vinfo )
{
   OverlayInfo *overlay_info;
   int numOverlaysPerScreen, i;

   overlay_info = GetOverlayInfo(dpy, vinfo->screen, &numOverlaysPerScreen);
   if (!overlay_info) {
      return 0;
   }

   /* search the overlay visual list for the visual ID of interest */
   for (i = 0; i < numOverlaysPerScreen; i++) {
      const OverlayInfo *ov = overlay_info + i;
      if (ov->overlay_visual == vinfo->visualid) {
         /* found the visual */
         if (/*ov->transparent_type==1 &&*/ ov->layer!=0) {
            int level = ov->layer;
            XFree((void *) overlay_info);
            return level;
         }
         else {
            XFree((void *) overlay_info);
            return 0;
         }
      }
   }

   /* The visual ID was not found in the overlay list. */
   XFree((void *) overlay_info);
   return 0;
}




/*
 * Given an XVisualInfo and RGB, Double, and Depth buffer flags, save the
 * configuration in our list of GLX visuals.
 */
static XMesaVisual
save_glx_visual( Display *dpy, XVisualInfo *vinfo,
                 GLboolean rgbFlag, GLboolean alphaFlag, GLboolean dbFlag,
                 GLboolean stereoFlag,
                 GLint depth_size, GLint stencil_size,
                 GLint accumRedSize, GLint accumGreenSize,
                 GLint accumBlueSize, GLint accumAlphaSize,
                 GLint level, GLint numAuxBuffers )
{
   GLboolean ximageFlag = GL_TRUE;
   XMesaVisual xmvis;
   GLint i;
   GLboolean comparePointers;

   if (dbFlag) {
      /* Check if the MESA_BACK_BUFFER env var is set */
      char *backbuffer = _mesa_getenv("MESA_BACK_BUFFER");
      if (backbuffer) {
         if (backbuffer[0]=='p' || backbuffer[0]=='P') {
            ximageFlag = GL_FALSE;
         }
         else if (backbuffer[0]=='x' || backbuffer[0]=='X') {
            ximageFlag = GL_TRUE;
         }
         else {
            _mesa_warning(NULL, "Mesa: invalid value for MESA_BACK_BUFFER environment variable, using an XImage.");
         }
      }
   }

   if (stereoFlag) {
      /* stereo not supported */
      return NULL;
   }

   /* Comparing IDs uses less memory but sometimes fails. */
   /* XXX revisit this after 3.0 is finished. */
   if (_mesa_getenv("MESA_GLX_VISUAL_HACK"))
      comparePointers = GL_TRUE;
   else
      comparePointers = GL_FALSE;

   /* Force the visual to have an alpha channel */
   if (rgbFlag && _mesa_getenv("MESA_GLX_FORCE_ALPHA"))
      alphaFlag = GL_TRUE;

   /* First check if a matching visual is already in the list */
   for (i=0; i<NumVisuals; i++) {
      XMesaVisual v = VisualTable[i];
      if (v->display == dpy
          && v->mesa_visual.level == level
          && v->mesa_visual.numAuxBuffers == numAuxBuffers
          && v->ximage_flag == ximageFlag
          && v->mesa_visual.rgbMode == rgbFlag
          && v->mesa_visual.doubleBufferMode == dbFlag
          && v->mesa_visual.stereoMode == stereoFlag
          && (v->mesa_visual.alphaBits > 0) == alphaFlag
          && (v->mesa_visual.depthBits >= depth_size || depth_size == 0)
          && (v->mesa_visual.stencilBits >= stencil_size || stencil_size == 0)
          && (v->mesa_visual.accumRedBits >= accumRedSize || accumRedSize == 0)
          && (v->mesa_visual.accumGreenBits >= accumGreenSize || accumGreenSize == 0)
          && (v->mesa_visual.accumBlueBits >= accumBlueSize || accumBlueSize == 0)
          && (v->mesa_visual.accumAlphaBits >= accumAlphaSize || accumAlphaSize == 0)) {
         /* now either compare XVisualInfo pointers or visual IDs */
         if ((!comparePointers && v->visinfo->visualid == vinfo->visualid)
             || (comparePointers && v->vishandle == vinfo)) {
            return v;
         }
      }
   }

   /* Create a new visual and add it to the list. */

   xmvis = XMesaCreateVisual( dpy, vinfo, rgbFlag, alphaFlag, dbFlag,
                              stereoFlag, ximageFlag,
                              depth_size, stencil_size,
                              accumRedSize, accumBlueSize,
                              accumBlueSize, accumAlphaSize, 0, level,
                              GLX_NONE_EXT );
   if (xmvis) {
      /* Save a copy of the pointer now so we can find this visual again
       * if we need to search for it in find_glx_visual().
       */
      xmvis->vishandle = vinfo;
      /* Allocate more space for additional visual */
      VisualTable = (XMesaVisual *) _mesa_realloc( VisualTable, 
                                   sizeof(XMesaVisual) * NumVisuals, 
                                   sizeof(XMesaVisual) * (NumVisuals + 1));
      /* add xmvis to the list */
      VisualTable[NumVisuals] = xmvis;
      NumVisuals++;
      /* XXX minor hack, because XMesaCreateVisual doesn't support an
       * aux buffers parameter.
       */
      xmvis->mesa_visual.numAuxBuffers = numAuxBuffers;
   }
   return xmvis;
}


/**
 * Return the default number of bits for the Z buffer.
 * If defined, use the MESA_GLX_DEPTH_BITS env var value.
 * Otherwise, use the DEFAULT_SOFTWARE_DEPTH_BITS constant.
 * XXX probably do the same thing for stencil, accum, etc.
 */
static GLint
default_depth_bits(void)
{
   int zBits;
   const char *zEnv = _mesa_getenv("MESA_GLX_DEPTH_BITS");
   if (zEnv)
      zBits = _mesa_atoi(zEnv);
   else
      zBits = DEFAULT_SOFTWARE_DEPTH_BITS;
   return zBits;
}

static GLint
default_alpha_bits(void)
{
   int aBits;
   const char *aEnv = _mesa_getenv("MESA_GLX_ALPHA_BITS");
   if (aEnv)
      aBits = _mesa_atoi(aEnv);
   else
      aBits = 0;
   return aBits;
}

static GLint
default_accum_bits(void)
{
   return 16;
}



/*
 * Create a GLX visual from a regular XVisualInfo.
 * This is called when Fake GLX is given an XVisualInfo which wasn't
 * returned by glXChooseVisual.  Since this is the first time we're
 * considering this visual we'll take a guess at reasonable values
 * for depth buffer size, stencil size, accum size, etc.
 * This is the best we can do with a client-side emulation of GLX.
 */
static XMesaVisual
create_glx_visual( Display *dpy, XVisualInfo *visinfo )
{
   int vislevel;
   GLint zBits = default_depth_bits();
   GLint accBits = default_accum_bits();
   GLboolean alphaFlag = default_alpha_bits() > 0;

   vislevel = level_of_visual( dpy, visinfo );
   if (vislevel) {
      /* Configure this visual as a CI, single-buffered overlay */
      return save_glx_visual( dpy, visinfo,
                              GL_FALSE,  /* rgb */
                              GL_FALSE,  /* alpha */
                              GL_FALSE,  /* double */
                              GL_FALSE,  /* stereo */
                              0,         /* depth bits */
                              0,         /* stencil bits */
                              0,0,0,0,   /* accum bits */
                              vislevel,  /* level */
                              0          /* numAux */
                            );
   }
   else if (is_usable_visual( visinfo )) {
      if (_mesa_getenv("MESA_GLX_FORCE_CI")) {
         /* Configure this visual as a COLOR INDEX visual. */
         return save_glx_visual( dpy, visinfo,
                                 GL_FALSE,   /* rgb */
                                 GL_FALSE,  /* alpha */
                                 GL_TRUE,   /* double */
                                 GL_FALSE,  /* stereo */
                                 zBits,
                                 STENCIL_BITS,
                                 0, 0, 0, 0, /* accum bits */
                                 0,         /* level */
                                 0          /* numAux */
                               );
      }
      else {
         /* Configure this visual as RGB, double-buffered, depth-buffered. */
         /* This is surely wrong for some people's needs but what else */
         /* can be done?  They should use glXChooseVisual(). */
         return save_glx_visual( dpy, visinfo,
                                 GL_TRUE,   /* rgb */
                                 alphaFlag, /* alpha */
                                 GL_TRUE,   /* double */
                                 GL_FALSE,  /* stereo */
                                 zBits,
                                 STENCIL_BITS,
                                 accBits, /* r */
                                 accBits, /* g */
                                 accBits, /* b */
                                 accBits, /* a */
                                 0,         /* level */
                                 0          /* numAux */
                               );
      }
   }
   else {
      _mesa_warning(NULL, "Mesa: error in glXCreateContext: bad visual\n");
      return NULL;
   }
}



/*
 * Find the GLX visual associated with an XVisualInfo.
 */
static XMesaVisual
find_glx_visual( Display *dpy, XVisualInfo *vinfo )
{
   int i;

   /* try to match visual id */
   for (i=0;i<NumVisuals;i++) {
      if (VisualTable[i]->display==dpy
          && VisualTable[i]->visinfo->visualid == vinfo->visualid) {
         return VisualTable[i];
      }
   }

   /* if that fails, try to match pointers */
   for (i=0;i<NumVisuals;i++) {
      if (VisualTable[i]->display==dpy && VisualTable[i]->vishandle==vinfo) {
         return VisualTable[i];
      }
   }

   return NULL;
}



/**
 * Return the transparent pixel value for a GLX visual.
 * Input:  glxvis - the glx_visual
 * Return:  a pixel value or -1 if no transparent pixel
 */
static int
transparent_pixel( XMesaVisual glxvis )
{
   Display *dpy = glxvis->display;
   XVisualInfo *vinfo = glxvis->visinfo;
   OverlayInfo *overlay_info;
   int numOverlaysPerScreen, i;

   overlay_info = GetOverlayInfo(dpy, vinfo->screen, &numOverlaysPerScreen);
   if (!overlay_info) {
      return -1;
   }

   for (i = 0; i < numOverlaysPerScreen; i++) {
      const OverlayInfo *ov = overlay_info + i;
      if (ov->overlay_visual == vinfo->visualid) {
         /* found it! */
         if (ov->transparent_type == 0) {
            /* type 0 indicates no transparency */
            XFree((void *) overlay_info);
            return -1;
         }
         else {
            /* ov->value is the transparent pixel */
            XFree((void *) overlay_info);
            return ov->value;
         }
      }
   }

   /* The visual ID was not found in the overlay list. */
   XFree((void *) overlay_info);
   return -1;
}



/**
 * Try to get an X visual which matches the given arguments.
 */
static XVisualInfo *
get_visual( Display *dpy, int scr, unsigned int depth, int xclass )
{
   XVisualInfo temp, *vis;
   long mask;
   int n;
   unsigned int default_depth;
   int default_class;

   mask = VisualScreenMask | VisualDepthMask | VisualClassMask;
   temp.screen = scr;
   temp.depth = depth;
   temp.CLASS = xclass;

   default_depth = DefaultDepth(dpy,scr);
   default_class = DefaultVisual(dpy,scr)->CLASS;

   if (depth==default_depth && xclass==default_class) {
      /* try to get root window's visual */
      temp.visualid = DefaultVisual(dpy,scr)->visualid;
      mask |= VisualIDMask;
   }

   vis = XGetVisualInfo( dpy, mask, &temp, &n );

   /* In case bits/pixel > 24, make sure color channels are still <=8 bits.
    * An SGI Infinite Reality system, for example, can have 30bpp pixels:
    * 10 bits per color channel.  Mesa's limited to a max of 8 bits/channel.
    */
   if (vis && depth > 24 && (xclass==TrueColor || xclass==DirectColor)) {
      if (_mesa_bitcount((GLuint) vis->red_mask  ) <= 8 &&
          _mesa_bitcount((GLuint) vis->green_mask) <= 8 &&
          _mesa_bitcount((GLuint) vis->blue_mask ) <= 8) {
         return vis;
      }
      else {
         XFree((void *) vis);
         return NULL;
      }
   }

   return vis;
}



/*
 * Retrieve the value of the given environment variable and find
 * the X visual which matches it.
 * Input:  dpy - the display
 *         screen - the screen number
 *         varname - the name of the environment variable
 * Return:  an XVisualInfo pointer to NULL if error.
 */
static XVisualInfo *
get_env_visual(Display *dpy, int scr, const char *varname)
{
   char value[100], type[100];
   int depth, xclass = -1;
   XVisualInfo *vis;

   if (!_mesa_getenv( varname )) {
      return NULL;
   }

   _mesa_strncpy( value, _mesa_getenv(varname), 100 );
   value[99] = 0;

   sscanf( value, "%s %d", type, &depth );

   if (_mesa_strcmp(type,"TrueColor")==0)          xclass = TrueColor;
   else if (_mesa_strcmp(type,"DirectColor")==0)   xclass = DirectColor;
   else if (_mesa_strcmp(type,"PseudoColor")==0)   xclass = PseudoColor;
   else if (_mesa_strcmp(type,"StaticColor")==0)   xclass = StaticColor;
   else if (_mesa_strcmp(type,"GrayScale")==0)     xclass = GrayScale;
   else if (_mesa_strcmp(type,"StaticGray")==0)    xclass = StaticGray;

   if (xclass>-1 && depth>0) {
      vis = get_visual( dpy, scr, depth, xclass );
      if (vis) {
	 return vis;
      }
   }

   _mesa_warning(NULL, "GLX unable to find visual class=%s, depth=%d.",
                 type, depth);

   return NULL;
}



/*
 * Select an X visual which satisfies the RGBA/CI flag and minimum depth.
 * Input:  dpy, screen - X display and screen number
 *         rgba - GL_TRUE = RGBA mode, GL_FALSE = CI mode
 *         min_depth - minimum visual depth
 *         preferred_class - preferred GLX visual class or DONT_CARE
 * Return:  pointer to an XVisualInfo or NULL.
 */
static XVisualInfo *
choose_x_visual( Display *dpy, int screen, GLboolean rgba, int min_depth,
                 int preferred_class )
{
   XVisualInfo *vis;
   int xclass, visclass = 0;
   int depth;

   if (rgba) {
      Atom hp_cr_maps = XInternAtom(dpy, "_HP_RGB_SMOOTH_MAP_LIST", True);
      /* First see if the MESA_RGB_VISUAL env var is defined */
      vis = get_env_visual( dpy, screen, "MESA_RGB_VISUAL" );
      if (vis) {
	 return vis;
      }
      /* Otherwise, search for a suitable visual */
      if (preferred_class==DONT_CARE) {
         for (xclass=0;xclass<6;xclass++) {
            switch (xclass) {
               case 0:  visclass = TrueColor;    break;
               case 1:  visclass = DirectColor;  break;
               case 2:  visclass = PseudoColor;  break;
               case 3:  visclass = StaticColor;  break;
               case 4:  visclass = GrayScale;    break;
               case 5:  visclass = StaticGray;   break;
            }
            if (min_depth==0) {
               /* start with shallowest */
               for (depth=0;depth<=32;depth++) {
                  if (visclass==TrueColor && depth==8 && !hp_cr_maps) {
                     /* Special case:  try to get 8-bit PseudoColor before */
                     /* 8-bit TrueColor */
                     vis = get_visual( dpy, screen, 8, PseudoColor );
                     if (vis) {
                        return vis;
                     }
                  }
                  vis = get_visual( dpy, screen, depth, visclass );
                  if (vis) {
                     return vis;
                  }
               }
            }
            else {
               /* start with deepest */
               for (depth=32;depth>=min_depth;depth--) {
                  if (visclass==TrueColor && depth==8 && !hp_cr_maps) {
                     /* Special case:  try to get 8-bit PseudoColor before */
                     /* 8-bit TrueColor */
                     vis = get_visual( dpy, screen, 8, PseudoColor );
                     if (vis) {
                        return vis;
                     }
                  }
                  vis = get_visual( dpy, screen, depth, visclass );
                  if (vis) {
                     return vis;
                  }
               }
            }
         }
      }
      else {
         /* search for a specific visual class */
         switch (preferred_class) {
            case GLX_TRUE_COLOR_EXT:    visclass = TrueColor;    break;
            case GLX_DIRECT_COLOR_EXT:  visclass = DirectColor;  break;
            case GLX_PSEUDO_COLOR_EXT:  visclass = PseudoColor;  break;
            case GLX_STATIC_COLOR_EXT:  visclass = StaticColor;  break;
            case GLX_GRAY_SCALE_EXT:    visclass = GrayScale;    break;
            case GLX_STATIC_GRAY_EXT:   visclass = StaticGray;   break;
            default:   return NULL;
         }
         if (min_depth==0) {
            /* start with shallowest */
            for (depth=0;depth<=32;depth++) {
               vis = get_visual( dpy, screen, depth, visclass );
               if (vis) {
                  return vis;
               }
            }
         }
         else {
            /* start with deepest */
            for (depth=32;depth>=min_depth;depth--) {
               vis = get_visual( dpy, screen, depth, visclass );
               if (vis) {
                  return vis;
               }
            }
         }
      }
   }
   else {
      /* First see if the MESA_CI_VISUAL env var is defined */
      vis = get_env_visual( dpy, screen, "MESA_CI_VISUAL" );
      if (vis) {
	 return vis;
      }
      /* Otherwise, search for a suitable visual, starting with shallowest */
      if (preferred_class==DONT_CARE) {
         for (xclass=0;xclass<4;xclass++) {
            switch (xclass) {
               case 0:  visclass = PseudoColor;  break;
               case 1:  visclass = StaticColor;  break;
               case 2:  visclass = GrayScale;    break;
               case 3:  visclass = StaticGray;   break;
            }
            /* try 8-bit up through 16-bit */
            for (depth=8;depth<=16;depth++) {
               vis = get_visual( dpy, screen, depth, visclass );
               if (vis) {
                  return vis;
               }
            }
            /* try min_depth up to 8-bit */
            for (depth=min_depth;depth<8;depth++) {
               vis = get_visual( dpy, screen, depth, visclass );
               if (vis) {
                  return vis;
               }
            }
         }
      }
      else {
         /* search for a specific visual class */
         switch (preferred_class) {
            case GLX_TRUE_COLOR_EXT:    visclass = TrueColor;    break;
            case GLX_DIRECT_COLOR_EXT:  visclass = DirectColor;  break;
            case GLX_PSEUDO_COLOR_EXT:  visclass = PseudoColor;  break;
            case GLX_STATIC_COLOR_EXT:  visclass = StaticColor;  break;
            case GLX_GRAY_SCALE_EXT:    visclass = GrayScale;    break;
            case GLX_STATIC_GRAY_EXT:   visclass = StaticGray;   break;
            default:   return NULL;
         }
         /* try 8-bit up through 16-bit */
         for (depth=8;depth<=16;depth++) {
            vis = get_visual( dpy, screen, depth, visclass );
            if (vis) {
               return vis;
            }
         }
         /* try min_depth up to 8-bit */
         for (depth=min_depth;depth<8;depth++) {
            vis = get_visual( dpy, screen, depth, visclass );
            if (vis) {
               return vis;
            }
         }
      }
   }

   /* didn't find a visual */
   return NULL;
}



/*
 * Find the deepest X over/underlay visual of at least min_depth.
 * Input:  dpy, screen - X display and screen number
 *         level - the over/underlay level
 *         trans_type - transparent pixel type: GLX_NONE_EXT,
 *                      GLX_TRANSPARENT_RGB_EXT, GLX_TRANSPARENT_INDEX_EXT,
 *                      or DONT_CARE
 *         trans_value - transparent pixel value or DONT_CARE
 *         min_depth - minimum visual depth
 *         preferred_class - preferred GLX visual class or DONT_CARE
 * Return:  pointer to an XVisualInfo or NULL.
 */
static XVisualInfo *
choose_x_overlay_visual( Display *dpy, int scr, GLboolean rgbFlag,
                         int level, int trans_type, int trans_value,
                         int min_depth, int preferred_class )
{
   OverlayInfo *overlay_info;
   int numOverlaysPerScreen;
   int i;
   XVisualInfo *deepvis;
   int deepest;

   /*DEBUG int tt, tv; */

   switch (preferred_class) {
      case GLX_TRUE_COLOR_EXT:    preferred_class = TrueColor;    break;
      case GLX_DIRECT_COLOR_EXT:  preferred_class = DirectColor;  break;
      case GLX_PSEUDO_COLOR_EXT:  preferred_class = PseudoColor;  break;
      case GLX_STATIC_COLOR_EXT:  preferred_class = StaticColor;  break;
      case GLX_GRAY_SCALE_EXT:    preferred_class = GrayScale;    break;
      case GLX_STATIC_GRAY_EXT:   preferred_class = StaticGray;   break;
      default:                    preferred_class = DONT_CARE;
   }

   overlay_info = GetOverlayInfo(dpy, scr, &numOverlaysPerScreen);
   if (!overlay_info) {
      return NULL;
   }

   /* Search for the deepest overlay which satisifies all criteria. */
   deepest = min_depth;
   deepvis = NULL;

   for (i = 0; i < numOverlaysPerScreen; i++) {
      const OverlayInfo *ov = overlay_info + i;
      XVisualInfo *vislist, vistemplate;
      int count;

      if (ov->layer!=level) {
         /* failed overlay level criteria */
         continue;
      }
      if (!(trans_type==DONT_CARE
            || (trans_type==GLX_TRANSPARENT_INDEX_EXT
                && ov->transparent_type>0)
            || (trans_type==GLX_NONE_EXT && ov->transparent_type==0))) {
         /* failed transparent pixel type criteria */
         continue;
      }
      if (trans_value!=DONT_CARE && trans_value!=ov->value) {
         /* failed transparent pixel value criteria */
         continue;
      }

      /* get XVisualInfo and check the depth */
      vistemplate.visualid = ov->overlay_visual;
      vistemplate.screen = scr;
      vislist = XGetVisualInfo( dpy, VisualIDMask | VisualScreenMask,
                                &vistemplate, &count );

      if (count!=1) {
         /* something went wrong */
         continue;
      }
      if (preferred_class!=DONT_CARE && preferred_class!=vislist->CLASS) {
         /* wrong visual class */
         continue;
      }

      /* if RGB was requested, make sure we have True/DirectColor */
      if (rgbFlag && vislist->CLASS != TrueColor
          && vislist->CLASS != DirectColor)
         continue;

      /* if CI was requested, make sure we have a color indexed visual */
      if (!rgbFlag
          && (vislist->CLASS == TrueColor || vislist->CLASS == DirectColor))
         continue;

      if (deepvis==NULL || vislist->depth > deepest) {
         /* YES!  found a satisfactory visual */
         if (deepvis) {
            XFree( deepvis );
         }
         deepest = vislist->depth;
         deepvis = vislist;
         /* DEBUG  tt = ov->transparent_type;*/
         /* DEBUG  tv = ov->value; */
      }
   }

/*DEBUG
   if (deepvis) {
      printf("chose 0x%x:  layer=%d depth=%d trans_type=%d trans_value=%d\n",
             deepvis->visualid, level, deepvis->depth, tt, tv );
   }
*/
   return deepvis;
}


/**********************************************************************/
/***             Display-related functions                          ***/
/**********************************************************************/


/**
 * Free all XMesaVisuals which are associated with the given display.
 */
static void
destroy_visuals_on_display(Display *dpy)
{
   int i;
   for (i = 0; i < NumVisuals; i++) {
      if (VisualTable[i]->display == dpy) {
         /* remove this visual */
         int j;
         free(VisualTable[i]);
         for (j = i; j < NumVisuals - 1; j++)
            VisualTable[j] = VisualTable[j + 1];
         NumVisuals--;
      }
   }
}


/**
 * Called from XCloseDisplay() to let us free our display-related data.
 */
static int
close_display_callback(Display *dpy, XExtCodes *codes)
{
   destroy_visuals_on_display(dpy);
   xmesa_destroy_buffers_on_display(dpy);
   return 0;
}


/**
 * Look for the named extension on given display and return a pointer
 * to the _XExtension data, or NULL if extension not found.
 */
static _XExtension *
lookup_extension(Display *dpy, const char *extName)
{
   _XExtension *ext;
   for (ext = dpy->ext_procs; ext; ext = ext->next) {
      if (ext->name && strcmp(ext->name, extName) == 0) {
         return ext;
      }
   }
   return NULL;
}


/**
 * Whenever we're given a new Display pointer, call this function to
 * register our close_display_callback function.
 */
static void
register_with_display(Display *dpy)
{
   const char *extName = "MesaGLX";
   _XExtension *ext;

   ext = lookup_extension(dpy, extName);
   if (!ext) {
      XExtCodes *c = XAddExtension(dpy);
      ext = dpy->ext_procs;  /* new extension is at head of list */
      assert(c->extension == ext->codes.extension);
      ext->name = _mesa_strdup(extName);
      ext->close_display = close_display_callback;
   }
}


/**********************************************************************/
/***                  Begin Fake GLX API Functions                  ***/
/**********************************************************************/


/**
 * Helper used by glXChooseVisual and glXChooseFBConfig.
 * The fbConfig parameter must be GL_FALSE for the former and GL_TRUE for
 * the later.
 * In either case, the attribute list is terminated with the value 'None'.
 */
static XMesaVisual
choose_visual( Display *dpy, int screen, const int *list, GLboolean fbConfig )
{
   const GLboolean rgbModeDefault = fbConfig;
   const int *parselist;
   XVisualInfo *vis;
   int min_ci = 0;
   int min_red=0, min_green=0, min_blue=0;
   GLboolean rgb_flag = rgbModeDefault;
   GLboolean alpha_flag = GL_FALSE;
   GLboolean double_flag = GL_FALSE;
   GLboolean stereo_flag = GL_FALSE;
   GLint depth_size = 0;
   GLint stencil_size = 0;
   GLint accumRedSize = 0;
   GLint accumGreenSize = 0;
   GLint accumBlueSize = 0;
   GLint accumAlphaSize = 0;
   int level = 0;
   int visual_type = DONT_CARE;
   int trans_type = DONT_CARE;
   int trans_value = DONT_CARE;
   GLint caveat = DONT_CARE;
   XMesaVisual xmvis = NULL;
   int desiredVisualID = -1;
   int numAux = 0;

   parselist = list;

   while (*parselist) {

      switch (*parselist) {
	 case GLX_USE_GL:
            if (fbConfig) {
               /* invalid token */
               return NULL;
            }
            else {
               /* skip */
               parselist++;
            }
	    break;
	 case GLX_BUFFER_SIZE:
	    parselist++;
	    min_ci = *parselist++;
	    break;
	 case GLX_LEVEL:
	    parselist++;
            level = *parselist++;
	    break;
	 case GLX_RGBA:
            if (fbConfig) {
               /* invalid token */
               return NULL;
            }
            else {
               rgb_flag = GL_TRUE;
               parselist++;
            }
	    break;
	 case GLX_DOUBLEBUFFER:
            parselist++;
            if (fbConfig) {
               double_flag = *parselist++;
            }
            else {
               double_flag = GL_TRUE;
            }
	    break;
	 case GLX_STEREO:
            parselist++;
            if (fbConfig) {
               stereo_flag = *parselist++;
            }
            else {
               stereo_flag = GL_TRUE;
            }
            break;
	 case GLX_AUX_BUFFERS:
	    parselist++;
            numAux = *parselist++;
            if (numAux > MAX_AUX_BUFFERS)
               return NULL;
	    break;
	 case GLX_RED_SIZE:
	    parselist++;
	    min_red = *parselist++;
	    break;
	 case GLX_GREEN_SIZE:
	    parselist++;
	    min_green = *parselist++;
	    break;
	 case GLX_BLUE_SIZE:
	    parselist++;
	    min_blue = *parselist++;
	    break;
	 case GLX_ALPHA_SIZE:
	    parselist++;
            {
               GLint size = *parselist++;
               alpha_flag = size ? GL_TRUE : GL_FALSE;
            }
	    break;
	 case GLX_DEPTH_SIZE:
	    parselist++;
	    depth_size = *parselist++;
	    break;
	 case GLX_STENCIL_SIZE:
	    parselist++;
	    stencil_size = *parselist++;
	    break;
	 case GLX_ACCUM_RED_SIZE:
	    parselist++;
            {
               GLint size = *parselist++;
               accumRedSize = MAX2( accumRedSize, size );
            }
            break;
	 case GLX_ACCUM_GREEN_SIZE:
	    parselist++;
            {
               GLint size = *parselist++;
               accumGreenSize = MAX2( accumGreenSize, size );
            }
            break;
	 case GLX_ACCUM_BLUE_SIZE:
	    parselist++;
            {
               GLint size = *parselist++;
               accumBlueSize = MAX2( accumBlueSize, size );
            }
            break;
	 case GLX_ACCUM_ALPHA_SIZE:
	    parselist++;
            {
               GLint size = *parselist++;
               accumAlphaSize = MAX2( accumAlphaSize, size );
            }
	    break;

         /*
          * GLX_EXT_visual_info extension
          */
         case GLX_X_VISUAL_TYPE_EXT:
            parselist++;
            visual_type = *parselist++;
            break;
         case GLX_TRANSPARENT_TYPE_EXT:
            parselist++;
            trans_type = *parselist++;
            break;
         case GLX_TRANSPARENT_INDEX_VALUE_EXT:
            parselist++;
            trans_value = *parselist++;
            break;
         case GLX_TRANSPARENT_RED_VALUE_EXT:
         case GLX_TRANSPARENT_GREEN_VALUE_EXT:
         case GLX_TRANSPARENT_BLUE_VALUE_EXT:
         case GLX_TRANSPARENT_ALPHA_VALUE_EXT:
	    /* ignore */
	    parselist++;
	    parselist++;
	    break;

         /*
          * GLX_EXT_visual_info extension
          */
         case GLX_VISUAL_CAVEAT_EXT:
            parselist++;
            caveat = *parselist++; /* ignored for now */
            break;

         /*
          * GLX_ARB_multisample
          */
         case GLX_SAMPLE_BUFFERS_ARB:
            /* ms not supported */
            return NULL;
         case GLX_SAMPLES_ARB:
            /* ms not supported */
            return NULL;

         /*
          * FBConfig attribs.
          */
         case GLX_RENDER_TYPE:
            if (!fbConfig)
               return NULL;
            parselist++;
            if (*parselist == GLX_RGBA_BIT) {
               rgb_flag = GL_TRUE;
            }
            else if (*parselist == GLX_COLOR_INDEX_BIT) {
               rgb_flag = GL_FALSE;
            }
            else if (*parselist == 0) {
               rgb_flag = GL_TRUE;
            }
            parselist++;
            break;
         case GLX_DRAWABLE_TYPE:
            if (!fbConfig)
               return NULL;
            parselist++;
            if (*parselist & ~(GLX_WINDOW_BIT | GLX_PIXMAP_BIT | GLX_PBUFFER_BIT)) {
               return NULL; /* bad bit */
            }
            parselist++;
            break;
         case GLX_FBCONFIG_ID:
            if (!fbConfig)
               return NULL;
            parselist++;
            desiredVisualID = *parselist++;
            break;
         case GLX_X_RENDERABLE:
            if (!fbConfig)
               return NULL;
            parselist += 2;
            /* ignore */
            break;

	 case None:
            /* end of list */
	    break;

	 default:
	    /* undefined attribute */
            _mesa_warning(NULL, "unexpected attrib 0x%x in choose_visual()",
                          *parselist);
	    return NULL;
      }
   }

   (void) caveat;

   /*
    * Since we're only simulating the GLX extension this function will never
    * find any real GL visuals.  Instead, all we can do is try to find an RGB
    * or CI visual of appropriate depth.  Other requested attributes such as
    * double buffering, depth buffer, etc. will be associated with the X
    * visual and stored in the VisualTable[].
    */
   if (desiredVisualID != -1) {
      /* try to get a specific visual, by visualID */
      XVisualInfo temp;
      int n;
      temp.visualid = desiredVisualID;
      temp.screen = screen;
      vis = XGetVisualInfo(dpy, VisualIDMask | VisualScreenMask, &temp, &n);
      if (vis) {
         /* give the visual some useful GLX attributes */
         double_flag = GL_TRUE;
         if (vis->depth > 8)
            rgb_flag = GL_TRUE;
         depth_size = default_depth_bits();
         stencil_size = STENCIL_BITS;
         /* XXX accum??? */
      }
   }
   else if (level==0) {
      /* normal color planes */
      if (rgb_flag) {
         /* Get an RGB visual */
         int min_rgb = min_red + min_green + min_blue;
         if (min_rgb>1 && min_rgb<8) {
            /* a special case to be sure we can get a monochrome visual */
            min_rgb = 1;
         }
         vis = choose_x_visual( dpy, screen, rgb_flag, min_rgb, visual_type );
      }
      else {
         /* Get a color index visual */
         vis = choose_x_visual( dpy, screen, rgb_flag, min_ci, visual_type );
         accumRedSize = accumGreenSize = accumBlueSize = accumAlphaSize = 0;
      }
   }
   else {
      /* over/underlay planes */
      if (rgb_flag) {
         /* rgba overlay */
         int min_rgb = min_red + min_green + min_blue;
         if (min_rgb>1 && min_rgb<8) {
            /* a special case to be sure we can get a monochrome visual */
            min_rgb = 1;
         }
         vis = choose_x_overlay_visual( dpy, screen, rgb_flag, level,
                              trans_type, trans_value, min_rgb, visual_type );
      }
      else {
         /* color index overlay */
         vis = choose_x_overlay_visual( dpy, screen, rgb_flag, level,
                              trans_type, trans_value, min_ci, visual_type );
      }
   }

   if (vis) {
      /* Note: we're not exactly obeying the glXChooseVisual rules here.
       * When GLX_DEPTH_SIZE = 1 is specified we're supposed to choose the
       * largest depth buffer size, which is 32bits/value.  Instead, we
       * return 16 to maintain performance with earlier versions of Mesa.
       */
      if (depth_size > 24)
         depth_size = 32;
      else if (depth_size > 16)
         depth_size = 24;
      else if (depth_size > 0) {
         depth_size = default_depth_bits();
      }

      if (!alpha_flag) {
         alpha_flag = default_alpha_bits() > 0;
      }

      /* we only support one size of stencil and accum buffers. */
      if (stencil_size > 0)
         stencil_size = STENCIL_BITS;
      if (accumRedSize > 0 || accumGreenSize > 0 || accumBlueSize > 0 ||
          accumAlphaSize > 0) {
         accumRedSize = 
         accumGreenSize = 
         accumBlueSize = default_accum_bits();
         accumAlphaSize = alpha_flag ? accumRedSize : 0;
      }

      xmvis = save_glx_visual( dpy, vis, rgb_flag, alpha_flag, double_flag,
                               stereo_flag, depth_size, stencil_size,
                               accumRedSize, accumGreenSize,
                               accumBlueSize, accumAlphaSize, level, numAux );
   }

   return xmvis;
}


static XVisualInfo *
Fake_glXChooseVisual( Display *dpy, int screen, int *list )
{
   XMesaVisual xmvis;

   /* register ourselves as an extension on this display */
   register_with_display(dpy);

   xmvis = choose_visual(dpy, screen, list, GL_FALSE);
   if (xmvis) {
#if 0
      return xmvis->vishandle;
#else
      /* create a new vishandle - the cached one may be stale */
      xmvis->vishandle = (XVisualInfo *) _mesa_malloc(sizeof(XVisualInfo));
      if (xmvis->vishandle) {
         _mesa_memcpy(xmvis->vishandle, xmvis->visinfo, sizeof(XVisualInfo));
      }
      return xmvis->vishandle;
#endif
   }
   else
      return NULL;
}


static GLXContext
Fake_glXCreateContext( Display *dpy, XVisualInfo *visinfo,
                       GLXContext share_list, Bool direct )
{
   XMesaVisual xmvis;
   struct fake_glx_context *glxCtx;
   struct fake_glx_context *shareCtx = (struct fake_glx_context *) share_list;

   if (!dpy || !visinfo)
      return 0;

   glxCtx = CALLOC_STRUCT(fake_glx_context);
   if (!glxCtx)
      return 0;

   /* deallocate unused windows/buffers */
#if 0
   XMesaGarbageCollect();
#endif

   xmvis = find_glx_visual( dpy, visinfo );
   if (!xmvis) {
      /* This visual wasn't found with glXChooseVisual() */
      xmvis = create_glx_visual( dpy, visinfo );
      if (!xmvis) {
         /* unusable visual */
         _mesa_free(glxCtx);
         return NULL;
      }
   }

   glxCtx->xmesaContext = XMesaCreateContext(xmvis,
                                   shareCtx ? shareCtx->xmesaContext : NULL);
   if (!glxCtx->xmesaContext) {
      _mesa_free(glxCtx);
      return NULL;
   }

   glxCtx->xmesaContext->direct = GL_FALSE;
   glxCtx->glxContext.isDirect = GL_FALSE;
   glxCtx->glxContext.currentDpy = dpy;
   glxCtx->glxContext.xid = (XID) glxCtx;  /* self pointer */

   assert((void *) glxCtx == (void *) &(glxCtx->glxContext));

   return (GLXContext) glxCtx;
}


/* XXX these may have to be removed due to thread-safety issues. */
static GLXContext MakeCurrent_PrevContext = 0;
static GLXDrawable MakeCurrent_PrevDrawable = 0;
static GLXDrawable MakeCurrent_PrevReadable = 0;
static XMesaBuffer MakeCurrent_PrevDrawBuffer = 0;
static XMesaBuffer MakeCurrent_PrevReadBuffer = 0;


/* GLX 1.3 and later */
static Bool
Fake_glXMakeContextCurrent( Display *dpy, GLXDrawable draw,
                            GLXDrawable read, GLXContext ctx )
{
   struct fake_glx_context *glxCtx = (struct fake_glx_context *) ctx;

   if (ctx && draw && read) {
      XMesaBuffer drawBuffer, readBuffer;
      XMesaContext xmctx = glxCtx->xmesaContext;

      /* Find the XMesaBuffer which corresponds to the GLXDrawable 'draw' */
      if (ctx == MakeCurrent_PrevContext
          && draw == MakeCurrent_PrevDrawable) {
         drawBuffer = MakeCurrent_PrevDrawBuffer;
      }
      else {
         drawBuffer = XMesaFindBuffer( dpy, draw );
      }
      if (!drawBuffer) {
         /* drawable must be a new window! */
         drawBuffer = XMesaCreateWindowBuffer( xmctx->xm_visual, draw );
         if (!drawBuffer) {
            /* Out of memory, or context/drawable depth mismatch */
            return False;
         }
#ifdef FX
         FXcreateContext( xmctx->xm_visual, draw, xmctx, drawBuffer );
#endif
      }

      /* Find the XMesaBuffer which corresponds to the GLXDrawable 'read' */
      if (ctx == MakeCurrent_PrevContext
          && read == MakeCurrent_PrevReadable) {
         readBuffer = MakeCurrent_PrevReadBuffer;
      }
      else {
         readBuffer = XMesaFindBuffer( dpy, read );
      }
      if (!readBuffer) {
         /* drawable must be a new window! */
         readBuffer = XMesaCreateWindowBuffer( xmctx->xm_visual, read );
         if (!readBuffer) {
            /* Out of memory, or context/drawable depth mismatch */
            return False;
         }
#ifdef FX
         FXcreateContext( xmctx->xm_visual, read, xmctx, readBuffer );
#endif
      }

      MakeCurrent_PrevContext = ctx;
      MakeCurrent_PrevDrawable = draw;
      MakeCurrent_PrevReadable = read;
      MakeCurrent_PrevDrawBuffer = drawBuffer;
      MakeCurrent_PrevReadBuffer = readBuffer;

      /* Now make current! */
      if (XMesaMakeCurrent2(xmctx, drawBuffer, readBuffer)) {
         ((__GLXcontext *) ctx)->currentDpy = dpy;
         ((__GLXcontext *) ctx)->currentDrawable = draw;
         ((__GLXcontext *) ctx)->currentReadable = read;
         return True;
      }
      else {
         return False;
      }
   }
   else if (!ctx && !draw && !read) {
      /* release current context w/out assigning new one. */
      XMesaMakeCurrent( NULL, NULL );
      MakeCurrent_PrevContext = 0;
      MakeCurrent_PrevDrawable = 0;
      MakeCurrent_PrevReadable = 0;
      MakeCurrent_PrevDrawBuffer = 0;
      MakeCurrent_PrevReadBuffer = 0;
      return True;
   }
   else {
      /* The args must either all be non-zero or all zero.
       * This is an error.
       */
      return False;
   }
}


static Bool
Fake_glXMakeCurrent( Display *dpy, GLXDrawable drawable, GLXContext ctx )
{
   return Fake_glXMakeContextCurrent( dpy, drawable, drawable, ctx );
}


static GLXPixmap
Fake_glXCreateGLXPixmap( Display *dpy, XVisualInfo *visinfo, Pixmap pixmap )
{
   XMesaVisual v;
   XMesaBuffer b;

   v = find_glx_visual( dpy, visinfo );
   if (!v) {
      v = create_glx_visual( dpy, visinfo );
      if (!v) {
         /* unusable visual */
         return 0;
      }
   }

   b = XMesaCreatePixmapBuffer( v, pixmap, 0 );
   if (!b) {
      return 0;
   }
   return b->frontxrb->pixmap;
}


/*** GLX_MESA_pixmap_colormap ***/

static GLXPixmap
Fake_glXCreateGLXPixmapMESA( Display *dpy, XVisualInfo *visinfo,
                             Pixmap pixmap, Colormap cmap )
{
   XMesaVisual v;
   XMesaBuffer b;

   v = find_glx_visual( dpy, visinfo );
   if (!v) {
      v = create_glx_visual( dpy, visinfo );
      if (!v) {
         /* unusable visual */
         return 0;
      }
   }

   b = XMesaCreatePixmapBuffer( v, pixmap, cmap );
   if (!b) {
      return 0;
   }
   return b->frontxrb->pixmap;
}


static void
Fake_glXDestroyGLXPixmap( Display *dpy, GLXPixmap pixmap )
{
   XMesaBuffer b = XMesaFindBuffer(dpy, pixmap);
   if (b) {
      XMesaDestroyBuffer(b);
   }
   else if (_mesa_getenv("MESA_DEBUG")) {
      _mesa_warning(NULL, "Mesa: glXDestroyGLXPixmap: invalid pixmap\n");
   }
}


static void
Fake_glXCopyContext( Display *dpy, GLXContext src, GLXContext dst,
                     unsigned long mask )
{
   struct fake_glx_context *fakeSrc = (struct fake_glx_context *) src;
   struct fake_glx_context *fakeDst = (struct fake_glx_context *) dst;
   XMesaContext xm_src = fakeSrc->xmesaContext;
   XMesaContext xm_dst = fakeDst->xmesaContext;
   (void) dpy;
   if (MakeCurrent_PrevContext == src) {
      _mesa_Flush();
   }
   _mesa_copy_context( &(xm_src->mesa), &(xm_dst->mesa), (GLuint) mask );
}


static Bool
Fake_glXQueryExtension( Display *dpy, int *errorb, int *event )
{
   /* Mesa's GLX isn't really an X extension but we try to act like one. */
   (void) dpy;
   (void) errorb;
   (void) event;
   return True;
}


extern void _kw_ungrab_all( Display *dpy );
void _kw_ungrab_all( Display *dpy )
{
   XUngrabPointer( dpy, CurrentTime );
   XUngrabKeyboard( dpy, CurrentTime );
}


static void
Fake_glXDestroyContext( Display *dpy, GLXContext ctx )
{
   struct fake_glx_context *glxCtx = (struct fake_glx_context *) ctx;
   (void) dpy;
   MakeCurrent_PrevContext = 0;
   MakeCurrent_PrevDrawable = 0;
   MakeCurrent_PrevReadable = 0;
   MakeCurrent_PrevDrawBuffer = 0;
   MakeCurrent_PrevReadBuffer = 0;
   XMesaDestroyContext( glxCtx->xmesaContext );
   XMesaGarbageCollect();
   _mesa_free(glxCtx);
}


static Bool
Fake_glXIsDirect( Display *dpy, GLXContext ctx )
{
   struct fake_glx_context *glxCtx = (struct fake_glx_context *) ctx;
   (void) dpy;
   return glxCtx->xmesaContext->direct;
}



static void
Fake_glXSwapBuffers( Display *dpy, GLXDrawable drawable )
{
   XMesaBuffer buffer = XMesaFindBuffer( dpy, drawable );

   if (buffer) {
      XMesaSwapBuffers(buffer);
   }
   else if (_mesa_getenv("MESA_DEBUG")) {
      _mesa_warning(NULL, "glXSwapBuffers: invalid drawable 0x%x\n",
                    (int) drawable);
   }
}



/*** GLX_MESA_copy_sub_buffer ***/

static void
Fake_glXCopySubBufferMESA( Display *dpy, GLXDrawable drawable,
                           int x, int y, int width, int height )
{
   XMesaBuffer buffer = XMesaFindBuffer( dpy, drawable );
   if (buffer) {
      XMesaCopySubBuffer(buffer, x, y, width, height);
   }
   else if (_mesa_getenv("MESA_DEBUG")) {
      _mesa_warning(NULL, "Mesa: glXCopySubBufferMESA: invalid drawable\n");
   }
}


static Bool
Fake_glXQueryVersion( Display *dpy, int *maj, int *min )
{
   (void) dpy;
   /* Return GLX version, not Mesa version */
   assert(CLIENT_MAJOR_VERSION == SERVER_MAJOR_VERSION);
   *maj = CLIENT_MAJOR_VERSION;
   *min = MIN2( CLIENT_MINOR_VERSION, SERVER_MINOR_VERSION );
   return True;
}


/*
 * Query the GLX attributes of the given XVisualInfo.
 */
static int
get_config( XMesaVisual xmvis, int attrib, int *value, GLboolean fbconfig )
{
   ASSERT(xmvis);
   switch(attrib) {
      case GLX_USE_GL:
         if (fbconfig)
            return GLX_BAD_ATTRIBUTE;
         *value = (int) True;
	 return 0;
      case GLX_BUFFER_SIZE:
	 *value = xmvis->visinfo->depth;
	 return 0;
      case GLX_LEVEL:
	 *value = xmvis->mesa_visual.level;
	 return 0;
      case GLX_RGBA:
         if (fbconfig)
            return GLX_BAD_ATTRIBUTE;
	 if (xmvis->mesa_visual.rgbMode) {
	    *value = True;
	 }
	 else {
	    *value = False;
	 }
	 return 0;
      case GLX_DOUBLEBUFFER:
	 *value = (int) xmvis->mesa_visual.doubleBufferMode;
	 return 0;
      case GLX_STEREO:
	 *value = (int) xmvis->mesa_visual.stereoMode;
	 return 0;
      case GLX_AUX_BUFFERS:
	 *value = xmvis->mesa_visual.numAuxBuffers;
	 return 0;
      case GLX_RED_SIZE:
         *value = xmvis->mesa_visual.redBits;
	 return 0;
      case GLX_GREEN_SIZE:
         *value = xmvis->mesa_visual.greenBits;
	 return 0;
      case GLX_BLUE_SIZE:
         *value = xmvis->mesa_visual.blueBits;
	 return 0;
      case GLX_ALPHA_SIZE:
         *value = xmvis->mesa_visual.alphaBits;
	 return 0;
      case GLX_DEPTH_SIZE:
         *value = xmvis->mesa_visual.depthBits;
	 return 0;
      case GLX_STENCIL_SIZE:
	 *value = xmvis->mesa_visual.stencilBits;
	 return 0;
      case GLX_ACCUM_RED_SIZE:
	 *value = xmvis->mesa_visual.accumRedBits;
	 return 0;
      case GLX_ACCUM_GREEN_SIZE:
	 *value = xmvis->mesa_visual.accumGreenBits;
	 return 0;
      case GLX_ACCUM_BLUE_SIZE:
	 *value = xmvis->mesa_visual.accumBlueBits;
	 return 0;
      case GLX_ACCUM_ALPHA_SIZE:
         *value = xmvis->mesa_visual.accumAlphaBits;
	 return 0;

      /*
       * GLX_EXT_visual_info extension
       */
      case GLX_X_VISUAL_TYPE_EXT:
         switch (xmvis->visinfo->CLASS) {
            case StaticGray:   *value = GLX_STATIC_GRAY_EXT;   return 0;
            case GrayScale:    *value = GLX_GRAY_SCALE_EXT;    return 0;
            case StaticColor:  *value = GLX_STATIC_GRAY_EXT;   return 0;
            case PseudoColor:  *value = GLX_PSEUDO_COLOR_EXT;  return 0;
            case TrueColor:    *value = GLX_TRUE_COLOR_EXT;    return 0;
            case DirectColor:  *value = GLX_DIRECT_COLOR_EXT;  return 0;
         }
         return 0;
      case GLX_TRANSPARENT_TYPE_EXT:
         if (xmvis->mesa_visual.level==0) {
            /* normal planes */
            *value = GLX_NONE_EXT;
         }
         else if (xmvis->mesa_visual.level>0) {
            /* overlay */
            if (xmvis->mesa_visual.rgbMode) {
               *value = GLX_TRANSPARENT_RGB_EXT;
            }
            else {
               *value = GLX_TRANSPARENT_INDEX_EXT;
            }
         }
         else if (xmvis->mesa_visual.level<0) {
            /* underlay */
            *value = GLX_NONE_EXT;
         }
         return 0;
      case GLX_TRANSPARENT_INDEX_VALUE_EXT:
         {
            int pixel = transparent_pixel( xmvis );
            if (pixel>=0) {
               *value = pixel;
            }
            /* else undefined */
         }
         return 0;
      case GLX_TRANSPARENT_RED_VALUE_EXT:
         /* undefined */
         return 0;
      case GLX_TRANSPARENT_GREEN_VALUE_EXT:
         /* undefined */
         return 0;
      case GLX_TRANSPARENT_BLUE_VALUE_EXT:
         /* undefined */
         return 0;
      case GLX_TRANSPARENT_ALPHA_VALUE_EXT:
         /* undefined */
         return 0;

      /*
       * GLX_EXT_visual_info extension
       */
      case GLX_VISUAL_CAVEAT_EXT:
         /* test for zero, just in case */
         if (xmvis->mesa_visual.visualRating > 0)
            *value = xmvis->mesa_visual.visualRating;
         else
            *value = GLX_NONE_EXT;
         return 0;

      /*
       * GLX_ARB_multisample
       */
      case GLX_SAMPLE_BUFFERS_ARB:
         *value = 0;
         return 0;
      case GLX_SAMPLES_ARB:
         *value = 0;
         return 0;

      /*
       * For FBConfigs:
       */
      case GLX_SCREEN_EXT:
         if (!fbconfig)
            return GLX_BAD_ATTRIBUTE;
         *value = xmvis->visinfo->screen;
         break;
      case GLX_DRAWABLE_TYPE: /*SGIX too */
         if (!fbconfig)
            return GLX_BAD_ATTRIBUTE;
         *value = GLX_WINDOW_BIT | GLX_PIXMAP_BIT | GLX_PBUFFER_BIT;
         break;
      case GLX_RENDER_TYPE_SGIX:
         if (!fbconfig)
            return GLX_BAD_ATTRIBUTE;
         if (xmvis->mesa_visual.rgbMode)
            *value = GLX_RGBA_BIT;
         else
            *value = GLX_COLOR_INDEX_BIT;
         break;
      case GLX_X_RENDERABLE_SGIX:
         if (!fbconfig)
            return GLX_BAD_ATTRIBUTE;
         *value = True; /* XXX really? */
         break;
      case GLX_FBCONFIG_ID_SGIX:
         if (!fbconfig)
            return GLX_BAD_ATTRIBUTE;
         *value = xmvis->visinfo->visualid;
         break;
      case GLX_MAX_PBUFFER_WIDTH:
         if (!fbconfig)
            return GLX_BAD_ATTRIBUTE;
         /* XXX or MAX_WIDTH? */
         *value = DisplayWidth(xmvis->display, xmvis->visinfo->screen);
         break;
      case GLX_MAX_PBUFFER_HEIGHT:
         if (!fbconfig)
            return GLX_BAD_ATTRIBUTE;
         *value = DisplayHeight(xmvis->display, xmvis->visinfo->screen);
         break;
      case GLX_MAX_PBUFFER_PIXELS:
         if (!fbconfig)
            return GLX_BAD_ATTRIBUTE;
         *value = DisplayWidth(xmvis->display, xmvis->visinfo->screen) *
                  DisplayHeight(xmvis->display, xmvis->visinfo->screen);
         break;
      case GLX_VISUAL_ID:
         if (!fbconfig)
            return GLX_BAD_ATTRIBUTE;
         *value = xmvis->visinfo->visualid;
         break;

      default:
	 return GLX_BAD_ATTRIBUTE;
   }
   return Success;
}


static int
Fake_glXGetConfig( Display *dpy, XVisualInfo *visinfo,
                   int attrib, int *value )
{
   XMesaVisual xmvis;
   int k;
   if (!dpy || !visinfo)
      return GLX_BAD_ATTRIBUTE;

   xmvis = find_glx_visual( dpy, visinfo );
   if (!xmvis) {
      /* this visual wasn't obtained with glXChooseVisual */
      xmvis = create_glx_visual( dpy, visinfo );
      if (!xmvis) {
	 /* this visual can't be used for GL rendering */
	 if (attrib==GLX_USE_GL) {
	    *value = (int) False;
	    return 0;
	 }
	 else {
	    return GLX_BAD_VISUAL;
	 }
      }
   }

   k = get_config(xmvis, attrib, value, GL_FALSE);
   return k;
}


static void
Fake_glXWaitGL( void )
{
   XMesaContext xmesa = XMesaGetCurrentContext();
   XMesaFlush( xmesa );
}



static void
Fake_glXWaitX( void )
{
   XMesaContext xmesa = XMesaGetCurrentContext();
   XMesaFlush( xmesa );
}


static const char *
get_extensions( void )
{
#ifdef FX
   const char *fx = _mesa_getenv("MESA_GLX_FX");
   if (fx && fx[0] != 'd') {
      return EXTENSIONS;
   }
#endif
   return EXTENSIONS + 23; /* skip "GLX_MESA_set_3dfx_mode" */
}



/* GLX 1.1 and later */
static const char *
Fake_glXQueryExtensionsString( Display *dpy, int screen )
{
   (void) dpy;
   (void) screen;
   return get_extensions();
}



/* GLX 1.1 and later */
static const char *
Fake_glXQueryServerString( Display *dpy, int screen, int name )
{
   static char version[1000];
   _mesa_sprintf(version, "%d.%d %s",
                 SERVER_MAJOR_VERSION, SERVER_MINOR_VERSION, MESA_GLX_VERSION);

   (void) dpy;
   (void) screen;

   switch (name) {
      case GLX_EXTENSIONS:
         return get_extensions();
      case GLX_VENDOR:
	 return VENDOR;
      case GLX_VERSION:
	 return version;
      default:
         return NULL;
   }
}



/* GLX 1.1 and later */
static const char *
Fake_glXGetClientString( Display *dpy, int name )
{
   static char version[1000];
   _mesa_sprintf(version, "%d.%d %s", CLIENT_MAJOR_VERSION,
                 CLIENT_MINOR_VERSION, MESA_GLX_VERSION);

   (void) dpy;

   switch (name) {
      case GLX_EXTENSIONS:
         return get_extensions();
      case GLX_VENDOR:
	 return VENDOR;
      case GLX_VERSION:
	 return version;
      default:
         return NULL;
   }
}



/*
 * GLX 1.3 and later
 */


static int
Fake_glXGetFBConfigAttrib( Display *dpy, GLXFBConfig config,
                           int attribute, int *value )
{
   XMesaVisual v = (XMesaVisual) config;
   (void) dpy;
   (void) config;

   if (!dpy || !config || !value)
      return -1;

   return get_config(v, attribute, value, GL_TRUE);
}


static GLXFBConfig *
Fake_glXGetFBConfigs( Display *dpy, int screen, int *nelements )
{
   XVisualInfo *visuals, visTemplate;
   const long visMask = VisualScreenMask;
   int i;

   /* Get list of all X visuals */
   visTemplate.screen = screen;
   visuals = XGetVisualInfo(dpy, visMask, &visTemplate, nelements);
   if (*nelements > 0) {
      XMesaVisual *results;
      results = (XMesaVisual *) _mesa_malloc(*nelements * sizeof(XMesaVisual));
      if (!results) {
         *nelements = 0;
         return NULL;
      }
      for (i = 0; i < *nelements; i++) {
         results[i] = create_glx_visual(dpy, visuals + i);
      }
      return (GLXFBConfig *) results;
   }
   return NULL;
}


static GLXFBConfig *
Fake_glXChooseFBConfig( Display *dpy, int screen,
                        const int *attribList, int *nitems )
{
   XMesaVisual xmvis;

   if (!attribList || !attribList[0]) {
      /* return list of all configs (per GLX_SGIX_fbconfig spec) */
      return Fake_glXGetFBConfigs(dpy, screen, nitems);
   }

   xmvis = choose_visual(dpy, screen, attribList, GL_TRUE);
   if (xmvis) {
      GLXFBConfig *config = (GLXFBConfig *) _mesa_malloc(sizeof(XMesaVisual));
      if (!config) {
         *nitems = 0;
         return NULL;
      }
      *nitems = 1;
      config[0] = (GLXFBConfig) xmvis;
      return (GLXFBConfig *) config;
   }
   else {
      *nitems = 0;
      return NULL;
   }
}


static XVisualInfo *
Fake_glXGetVisualFromFBConfig( Display *dpy, GLXFBConfig config )
{
   if (dpy && config) {
      XMesaVisual xmvis = (XMesaVisual) config;
#if 0      
      return xmvis->vishandle;
#else
      /* create a new vishandle - the cached one may be stale */
      xmvis->vishandle = (XVisualInfo *) _mesa_malloc(sizeof(XVisualInfo));
      if (xmvis->vishandle) {
         _mesa_memcpy(xmvis->vishandle, xmvis->visinfo, sizeof(XVisualInfo));
      }
      return xmvis->vishandle;
#endif
   }
   else {
      return NULL;
   }
}


static GLXWindow
Fake_glXCreateWindow( Display *dpy, GLXFBConfig config, Window win,
                      const int *attribList )
{
   XMesaVisual xmvis = (XMesaVisual) config;
   XMesaBuffer xmbuf;
   if (!xmvis)
      return 0;

   xmbuf = XMesaCreateWindowBuffer(xmvis, win);
   if (!xmbuf)
      return 0;

#ifdef FX
   /* XXX this will segfault if actually called */
   FXcreateContext(xmvis, win, NULL, xmbuf);
#endif

   (void) dpy;
   (void) attribList;  /* Ignored in GLX 1.3 */

   return win;  /* A hack for now */
}


static void
Fake_glXDestroyWindow( Display *dpy, GLXWindow window )
{
   XMesaBuffer b = XMesaFindBuffer(dpy, (XMesaDrawable) window);
   if (b)
      XMesaDestroyBuffer(b);
   /* don't destroy X window */
}


/* XXX untested */
static GLXPixmap
Fake_glXCreatePixmap( Display *dpy, GLXFBConfig config, Pixmap pixmap,
                      const int *attribList )
{
   XMesaVisual v = (XMesaVisual) config;
   XMesaBuffer b;

   (void) dpy;
   (void) config;
   (void) pixmap;
   (void) attribList;  /* Ignored in GLX 1.3 */

   if (!dpy || !config || !pixmap)
      return 0;

   b = XMesaCreatePixmapBuffer( v, pixmap, 0 );
   if (!b) {
      return 0;
   }

   return pixmap;
}


static void
Fake_glXDestroyPixmap( Display *dpy, GLXPixmap pixmap )
{
   XMesaBuffer b = XMesaFindBuffer(dpy, (XMesaDrawable)pixmap);
   if (b)
      XMesaDestroyBuffer(b);
   /* don't destroy X pixmap */
}


static GLXPbuffer
Fake_glXCreatePbuffer( Display *dpy, GLXFBConfig config,
                       const int *attribList )
{
   XMesaVisual xmvis = (XMesaVisual) config;
   XMesaBuffer xmbuf;
   const int *attrib;
   int width = 0, height = 0;
   GLboolean useLargest = GL_FALSE, preserveContents = GL_FALSE;

   (void) dpy;

   for (attrib = attribList; *attrib; attrib++) {
      switch (*attrib) {
         case GLX_PBUFFER_WIDTH:
            attrib++;
            width = *attrib;
            break;
         case GLX_PBUFFER_HEIGHT:
            attrib++;
            height = *attrib;
            break;
         case GLX_PRESERVED_CONTENTS:
            attrib++;
            preserveContents = *attrib; /* ignored */
            break;
         case GLX_LARGEST_PBUFFER:
            attrib++;
            useLargest = *attrib; /* ignored */
            break;
         default:
            return 0;
      }
   }

   /* not used at this time */
   (void) useLargest;
   (void) preserveContents;

   if (width == 0 || height == 0)
      return 0;

   xmbuf = XMesaCreatePBuffer( xmvis, 0, width, height);
   /* A GLXPbuffer handle must be an X Drawable because that's what
    * glXMakeCurrent takes.
    */
   if (xmbuf)
      return (GLXPbuffer) xmbuf->frontxrb->pixmap;
   else
      return 0;
}


static void
Fake_glXDestroyPbuffer( Display *dpy, GLXPbuffer pbuf )
{
   XMesaBuffer b = XMesaFindBuffer(dpy, pbuf);
   if (b) {
      XMesaDestroyBuffer(b);
   }
}


static void
Fake_glXQueryDrawable( Display *dpy, GLXDrawable draw, int attribute,
                       unsigned int *value )
{
   XMesaBuffer xmbuf = XMesaFindBuffer(dpy, draw);
   if (!xmbuf)
      return;

   switch (attribute) {
      case GLX_WIDTH:
         *value = xmbuf->mesa_buffer.Width;
         break;
      case GLX_HEIGHT:
         *value = xmbuf->mesa_buffer.Height;
         break;
      case GLX_PRESERVED_CONTENTS:
         *value = True;
         break;
      case GLX_LARGEST_PBUFFER:
         *value = xmbuf->mesa_buffer.Width * xmbuf->mesa_buffer.Height;
         break;
      case GLX_FBCONFIG_ID:
         *value = xmbuf->xm_visual->visinfo->visualid;
         return;
      default:
         return;  /* GLX_BAD_ATTRIBUTE? */
   }
}


static GLXContext
Fake_glXCreateNewContext( Display *dpy, GLXFBConfig config,
                          int renderType, GLXContext shareList, Bool direct )
{
   struct fake_glx_context *glxCtx;
   struct fake_glx_context *shareCtx = (struct fake_glx_context *) shareList;
   XMesaVisual xmvis = (XMesaVisual) config;

   if (!dpy || !config ||
       (renderType != GLX_RGBA_TYPE && renderType != GLX_COLOR_INDEX_TYPE))
      return 0;

   glxCtx = CALLOC_STRUCT(fake_glx_context);
   if (!glxCtx)
      return 0;

   /* deallocate unused windows/buffers */
   XMesaGarbageCollect();

   glxCtx->xmesaContext = XMesaCreateContext(xmvis,
                                   shareCtx ? shareCtx->xmesaContext : NULL);
   if (!glxCtx->xmesaContext) {
      _mesa_free(glxCtx);
      return NULL;
   }

   glxCtx->xmesaContext->direct = GL_FALSE;
   glxCtx->glxContext.isDirect = GL_FALSE;
   glxCtx->glxContext.currentDpy = dpy;
   glxCtx->glxContext.xid = (XID) glxCtx;  /* self pointer */

   assert((void *) glxCtx == (void *) &(glxCtx->glxContext));

   return (GLXContext) glxCtx;
}


static int
Fake_glXQueryContext( Display *dpy, GLXContext ctx, int attribute, int *value )
{
   struct fake_glx_context *glxCtx = (struct fake_glx_context *) ctx;
   XMesaContext xmctx = glxCtx->xmesaContext;

   (void) dpy;
   (void) ctx;

   switch (attribute) {
   case GLX_FBCONFIG_ID:
      *value = xmctx->xm_visual->visinfo->visualid;
      break;
   case GLX_RENDER_TYPE:
      if (xmctx->xm_visual->mesa_visual.rgbMode)
         *value = GLX_RGBA_BIT;
      else
         *value = GLX_COLOR_INDEX_BIT;
      break;
   case GLX_SCREEN:
      *value = 0;
      return Success;
   default:
      return GLX_BAD_ATTRIBUTE;
   }
   return 0;
}


static void
Fake_glXSelectEvent( Display *dpy, GLXDrawable drawable, unsigned long mask )
{
   XMesaBuffer xmbuf = XMesaFindBuffer(dpy, drawable);
   if (xmbuf)
      xmbuf->selectedEvents = mask;
}


static void
Fake_glXGetSelectedEvent( Display *dpy, GLXDrawable drawable,
                          unsigned long *mask )
{
   XMesaBuffer xmbuf = XMesaFindBuffer(dpy, drawable);
   if (xmbuf)
      *mask = xmbuf->selectedEvents;
   else
      *mask = 0;
}



/*** GLX_SGI_swap_control ***/

static int
Fake_glXSwapIntervalSGI(int interval)
{
   (void) interval;
   return 0;
}



/*** GLX_SGI_video_sync ***/

static unsigned int FrameCounter = 0;

static int
Fake_glXGetVideoSyncSGI(unsigned int *count)
{
   /* this is a bogus implementation */
   *count = FrameCounter++;
   return 0;
}

static int
Fake_glXWaitVideoSyncSGI(int divisor, int remainder, unsigned int *count)
{
   if (divisor <= 0 || remainder < 0)
      return GLX_BAD_VALUE;
   /* this is a bogus implementation */
   FrameCounter++;
   while (FrameCounter % divisor != remainder)
      FrameCounter++;
   *count = FrameCounter;
   return 0;
}



/*** GLX_SGI_make_current_read ***/

static Bool
Fake_glXMakeCurrentReadSGI(Display *dpy, GLXDrawable draw, GLXDrawable read, GLXContext ctx)
{
   return Fake_glXMakeContextCurrent( dpy, draw, read, ctx );
}

/* not used
static GLXDrawable
Fake_glXGetCurrentReadDrawableSGI(void)
{
   return 0;
}
*/


/*** GLX_SGIX_video_source ***/
#if defined(_VL_H)

static GLXVideoSourceSGIX
Fake_glXCreateGLXVideoSourceSGIX(Display *dpy, int screen, VLServer server, VLPath path, int nodeClass, VLNode drainNode)
{
   (void) dpy;
   (void) screen;
   (void) server;
   (void) path;
   (void) nodeClass;
   (void) drainNode;
   return 0;
}

static void
Fake_glXDestroyGLXVideoSourceSGIX(Display *dpy, GLXVideoSourceSGIX src)
{
   (void) dpy;
   (void) src;
}

#endif


/*** GLX_EXT_import_context ***/

static void
Fake_glXFreeContextEXT(Display *dpy, GLXContext context)
{
   (void) dpy;
   (void) context;
}

static GLXContextID
Fake_glXGetContextIDEXT(const GLXContext context)
{
   (void) context;
   return 0;
}

static GLXContext
Fake_glXImportContextEXT(Display *dpy, GLXContextID contextID)
{
   (void) dpy;
   (void) contextID;
   return 0;
}

static int
Fake_glXQueryContextInfoEXT(Display *dpy, GLXContext context, int attribute, int *value)
{
   (void) dpy;
   (void) context;
   (void) attribute;
   (void) value;
   return 0;
}



/*** GLX_SGIX_fbconfig ***/

static int
Fake_glXGetFBConfigAttribSGIX(Display *dpy, GLXFBConfigSGIX config, int attribute, int *value)
{
   return Fake_glXGetFBConfigAttrib(dpy, config, attribute, value);
}

static GLXFBConfigSGIX *
Fake_glXChooseFBConfigSGIX(Display *dpy, int screen, int *attrib_list, int *nelements)
{
   return (GLXFBConfig *) Fake_glXChooseFBConfig(dpy, screen, attrib_list, nelements);
}


static GLXPixmap
Fake_glXCreateGLXPixmapWithConfigSGIX(Display *dpy, GLXFBConfigSGIX config, Pixmap pixmap)
{
   XMesaVisual xmvis = (XMesaVisual) config;
   XMesaBuffer xmbuf = XMesaCreatePixmapBuffer(xmvis, pixmap, 0);
   return xmbuf->frontxrb->pixmap; /* need to return an X ID */
}


static GLXContext
Fake_glXCreateContextWithConfigSGIX(Display *dpy, GLXFBConfigSGIX config, int render_type, GLXContext share_list, Bool direct)
{
   XMesaVisual xmvis = (XMesaVisual) config;
   struct fake_glx_context *glxCtx;
   struct fake_glx_context *shareCtx = (struct fake_glx_context *) share_list;

   glxCtx = CALLOC_STRUCT(fake_glx_context);
   if (!glxCtx)
      return 0;

   /* deallocate unused windows/buffers */
   XMesaGarbageCollect();

   glxCtx->xmesaContext = XMesaCreateContext(xmvis,
                                   shareCtx ? shareCtx->xmesaContext : NULL);
   if (!glxCtx->xmesaContext) {
      _mesa_free(glxCtx);
      return NULL;
   }

   glxCtx->xmesaContext->direct = GL_FALSE;
   glxCtx->glxContext.isDirect = GL_FALSE;
   glxCtx->glxContext.currentDpy = dpy;
   glxCtx->glxContext.xid = (XID) glxCtx;  /* self pointer */

   assert((void *) glxCtx == (void *) &(glxCtx->glxContext));

   return (GLXContext) glxCtx;
}


static XVisualInfo *
Fake_glXGetVisualFromFBConfigSGIX(Display *dpy, GLXFBConfigSGIX config)
{
   return Fake_glXGetVisualFromFBConfig(dpy, config);
}


static GLXFBConfigSGIX
Fake_glXGetFBConfigFromVisualSGIX(Display *dpy, XVisualInfo *vis)
{
   XMesaVisual xmvis = find_glx_visual(dpy, vis);
   if (!xmvis) {
      /* This visual wasn't found with glXChooseVisual() */
      xmvis = create_glx_visual(dpy, vis);
   }

   return (GLXFBConfigSGIX) xmvis;
}



/*** GLX_SGIX_pbuffer ***/

static GLXPbufferSGIX
Fake_glXCreateGLXPbufferSGIX(Display *dpy, GLXFBConfigSGIX config,
                             unsigned int width, unsigned int height,
                             int *attribList)
{
   XMesaVisual xmvis = (XMesaVisual) config;
   XMesaBuffer xmbuf;
   const int *attrib;
   GLboolean useLargest = GL_FALSE, preserveContents = GL_FALSE;

   (void) dpy;

   for (attrib = attribList; attrib && *attrib; attrib++) {
      switch (*attrib) {
         case GLX_PRESERVED_CONTENTS_SGIX:
            attrib++;
            preserveContents = *attrib; /* ignored */
            break;
         case GLX_LARGEST_PBUFFER_SGIX:
            attrib++;
            useLargest = *attrib; /* ignored */
            break;
         default:
            return 0;
      }
   }

   /* not used at this time */
   (void) useLargest;
   (void) preserveContents;

   xmbuf = XMesaCreatePBuffer( xmvis, 0, width, height);
   /* A GLXPbuffer handle must be an X Drawable because that's what
    * glXMakeCurrent takes.
    */
   return (GLXPbuffer) xmbuf->frontxrb->pixmap;
}


static void
Fake_glXDestroyGLXPbufferSGIX(Display *dpy, GLXPbufferSGIX pbuf)
{
   XMesaBuffer xmbuf = XMesaFindBuffer(dpy, pbuf);
   if (xmbuf) {
      XMesaDestroyBuffer(xmbuf);
   }
}


static int
Fake_glXQueryGLXPbufferSGIX(Display *dpy, GLXPbufferSGIX pbuf, int attribute, unsigned int *value)
{
   const XMesaBuffer xmbuf = XMesaFindBuffer(dpy, pbuf);

   if (!xmbuf) {
      /* Generate GLXBadPbufferSGIX for bad pbuffer */
      return 0;
   }

   switch (attribute) {
      case GLX_PRESERVED_CONTENTS_SGIX:
         *value = True;
         break;
      case GLX_LARGEST_PBUFFER_SGIX:
         *value = xmbuf->mesa_buffer.Width * xmbuf->mesa_buffer.Height;
         break;
      case GLX_WIDTH_SGIX:
         *value = xmbuf->mesa_buffer.Width;
         break;
      case GLX_HEIGHT_SGIX:
         *value = xmbuf->mesa_buffer.Height;
         break;
      case GLX_EVENT_MASK_SGIX:
         *value = 0;  /* XXX might be wrong */
         break;
      default:
         *value = 0;
   }
   return 0;
}


static void
Fake_glXSelectEventSGIX(Display *dpy, GLXDrawable drawable, unsigned long mask)
{
   XMesaBuffer xmbuf = XMesaFindBuffer(dpy, drawable);
   if (xmbuf) {
      /* Note: we'll never generate clobber events */
      xmbuf->selectedEvents = mask;
   }
}


static void
Fake_glXGetSelectedEventSGIX(Display *dpy, GLXDrawable drawable, unsigned long *mask)
{
   XMesaBuffer xmbuf = XMesaFindBuffer(dpy, drawable);
   if (xmbuf) {
      *mask = xmbuf->selectedEvents;
   }
   else {
      *mask = 0;
   }
}



/*** GLX_SGI_cushion ***/

static void
Fake_glXCushionSGI(Display *dpy, Window win, float cushion)
{
   (void) dpy;
   (void) win;
   (void) cushion;
}



/*** GLX_SGIX_video_resize ***/

static int
Fake_glXBindChannelToWindowSGIX(Display *dpy, int screen, int channel , Window window)
{
   (void) dpy;
   (void) screen;
   (void) channel;
   (void) window;
   return 0;
}

static int
Fake_glXChannelRectSGIX(Display *dpy, int screen, int channel, int x, int y, int w, int h)
{
   (void) dpy;
   (void) screen;
   (void) channel;
   (void) x;
   (void) y;
   (void) w;
   (void) h;
   return 0;
}

static int
Fake_glXQueryChannelRectSGIX(Display *dpy, int screen, int channel, int *x, int *y, int *w, int *h)
{
   (void) dpy;
   (void) screen;
   (void) channel;
   (void) x;
   (void) y;
   (void) w;
   (void) h;
   return 0;
}

static int
Fake_glXQueryChannelDeltasSGIX(Display *dpy, int screen, int channel, int *dx, int *dy, int *dw, int *dh)
{
   (void) dpy;
   (void) screen;
   (void) channel;
   (void) dx;
   (void) dy;
   (void) dw;
   (void) dh;
   return 0;
}

static int
Fake_glXChannelRectSyncSGIX(Display *dpy, int screen, int channel, GLenum synctype)
{
   (void) dpy;
   (void) screen;
   (void) channel;
   (void) synctype;
   return 0;
}



/*** GLX_SGIX_dmbuffer **/

#if defined(_DM_BUFFER_H_)
static Bool
Fake_glXAssociateDMPbufferSGIX(Display *dpy, GLXPbufferSGIX pbuffer, DMparams *params, DMbuffer dmbuffer)
{
   (void) dpy;
   (void) pbuffer;
   (void) params;
   (void) dmbuffer;
   return False;
}
#endif


/*** GLX_SGIX_swap_group ***/

static void
Fake_glXJoinSwapGroupSGIX(Display *dpy, GLXDrawable drawable, GLXDrawable member)
{
   (void) dpy;
   (void) drawable;
   (void) member;
}



/*** GLX_SGIX_swap_barrier ***/

static void
Fake_glXBindSwapBarrierSGIX(Display *dpy, GLXDrawable drawable, int barrier)
{
   (void) dpy;
   (void) drawable;
   (void) barrier;
}

static Bool
Fake_glXQueryMaxSwapBarriersSGIX(Display *dpy, int screen, int *max)
{
   (void) dpy;
   (void) screen;
   (void) max;
   return False;
}



/*** GLX_SUN_get_transparent_index ***/

static Status
Fake_glXGetTransparentIndexSUN(Display *dpy, Window overlay, Window underlay, long *pTransparent)
{
   (void) dpy;
   (void) overlay;
   (void) underlay;
   (void) pTransparent;
   return 0;
}



/*** GLX_MESA_release_buffers ***/

/*
 * Release the depth, stencil, accum buffers attached to a GLXDrawable
 * (a window or pixmap) prior to destroying the GLXDrawable.
 */
static Bool
Fake_glXReleaseBuffersMESA( Display *dpy, GLXDrawable d )
{
   XMesaBuffer b = XMesaFindBuffer(dpy, d);
   if (b) {
      XMesaDestroyBuffer(b);
      return True;
   }
   return False;
}



/*** GLX_MESA_set_3dfx_mode ***/

static Bool
Fake_glXSet3DfxModeMESA( int mode )
{
   return XMesaSetFXmode( mode );
}



/*** GLX_NV_vertex_array range ***/
static void *
Fake_glXAllocateMemoryNV( GLsizei size,
                          GLfloat readFrequency,
                          GLfloat writeFrequency,
                          GLfloat priority )
{
   (void) size;
   (void) readFrequency;
   (void) writeFrequency;
   (void) priority;
   return NULL;
}


static void 
Fake_glXFreeMemoryNV( GLvoid *pointer )
{
   (void) pointer;
}


/*** GLX_MESA_agp_offset ***/

static GLuint
Fake_glXGetAGPOffsetMESA( const GLvoid *pointer )
{
   (void) pointer;
   return ~0;
}


/* silence warning */
extern struct _glxapi_table *_mesa_GetGLXDispatchTable(void);


/**
 * Create a new GLX API dispatch table with its function pointers
 * initialized to point to Mesa's "fake" GLX API functions.
 * Note: there's a similar function (_real_GetGLXDispatchTable) that
 * returns a new dispatch table with all pointers initalized to point
 * to "real" GLX functions (which understand GLX wire protocol, etc).
 */
struct _glxapi_table *
_mesa_GetGLXDispatchTable(void)
{
   static struct _glxapi_table glx;

   /* be sure our dispatch table size <= libGL's table */
   {
      GLuint size = sizeof(struct _glxapi_table) / sizeof(void *);
      (void) size;
      assert(_glxapi_get_dispatch_table_size() >= size);
   }

   /* initialize the whole table to no-ops */
   _glxapi_set_no_op_table(&glx);

   /* now initialize the table with the functions I implement */
   glx.ChooseVisual = Fake_glXChooseVisual;
   glx.CopyContext = Fake_glXCopyContext;
   glx.CreateContext = Fake_glXCreateContext;
   glx.CreateGLXPixmap = Fake_glXCreateGLXPixmap;
   glx.DestroyContext = Fake_glXDestroyContext;
   glx.DestroyGLXPixmap = Fake_glXDestroyGLXPixmap;
   glx.GetConfig = Fake_glXGetConfig;
   /*glx.GetCurrentContext = Fake_glXGetCurrentContext;*/
   /*glx.GetCurrentDrawable = Fake_glXGetCurrentDrawable;*/
   glx.IsDirect = Fake_glXIsDirect;
   glx.MakeCurrent = Fake_glXMakeCurrent;
   glx.QueryExtension = Fake_glXQueryExtension;
   glx.QueryVersion = Fake_glXQueryVersion;
   glx.SwapBuffers = Fake_glXSwapBuffers;
   glx.UseXFont = Fake_glXUseXFont;
   glx.WaitGL = Fake_glXWaitGL;
   glx.WaitX = Fake_glXWaitX;

   /*** GLX_VERSION_1_1 ***/
   glx.GetClientString = Fake_glXGetClientString;
   glx.QueryExtensionsString = Fake_glXQueryExtensionsString;
   glx.QueryServerString = Fake_glXQueryServerString;

   /*** GLX_VERSION_1_2 ***/
   /*glx.GetCurrentDisplay = Fake_glXGetCurrentDisplay;*/

   /*** GLX_VERSION_1_3 ***/
   glx.ChooseFBConfig = Fake_glXChooseFBConfig;
   glx.CreateNewContext = Fake_glXCreateNewContext;
   glx.CreatePbuffer = Fake_glXCreatePbuffer;
   glx.CreatePixmap = Fake_glXCreatePixmap;
   glx.CreateWindow = Fake_glXCreateWindow;
   glx.DestroyPbuffer = Fake_glXDestroyPbuffer;
   glx.DestroyPixmap = Fake_glXDestroyPixmap;
   glx.DestroyWindow = Fake_glXDestroyWindow;
   /*glx.GetCurrentReadDrawable = Fake_glXGetCurrentReadDrawable;*/
   glx.GetFBConfigAttrib = Fake_glXGetFBConfigAttrib;
   glx.GetFBConfigs = Fake_glXGetFBConfigs;
   glx.GetSelectedEvent = Fake_glXGetSelectedEvent;
   glx.GetVisualFromFBConfig = Fake_glXGetVisualFromFBConfig;
   glx.MakeContextCurrent = Fake_glXMakeContextCurrent;
   glx.QueryContext = Fake_glXQueryContext;
   glx.QueryDrawable = Fake_glXQueryDrawable;
   glx.SelectEvent = Fake_glXSelectEvent;

   /*** GLX_SGI_swap_control ***/
   glx.SwapIntervalSGI = Fake_glXSwapIntervalSGI;

   /*** GLX_SGI_video_sync ***/
   glx.GetVideoSyncSGI = Fake_glXGetVideoSyncSGI;
   glx.WaitVideoSyncSGI = Fake_glXWaitVideoSyncSGI;

   /*** GLX_SGI_make_current_read ***/
   glx.MakeCurrentReadSGI = Fake_glXMakeCurrentReadSGI;
   /*glx.GetCurrentReadDrawableSGI = Fake_glXGetCurrentReadDrawableSGI;*/

/*** GLX_SGIX_video_source ***/
#if defined(_VL_H)
   glx.CreateGLXVideoSourceSGIX = Fake_glXCreateGLXVideoSourceSGIX;
   glx.DestroyGLXVideoSourceSGIX = Fake_glXDestroyGLXVideoSourceSGIX;
#endif

   /*** GLX_EXT_import_context ***/
   glx.FreeContextEXT = Fake_glXFreeContextEXT;
   glx.GetContextIDEXT = Fake_glXGetContextIDEXT;
   /*glx.GetCurrentDisplayEXT = Fake_glXGetCurrentDisplayEXT;*/
   glx.ImportContextEXT = Fake_glXImportContextEXT;
   glx.QueryContextInfoEXT = Fake_glXQueryContextInfoEXT;

   /*** GLX_SGIX_fbconfig ***/
   glx.GetFBConfigAttribSGIX = Fake_glXGetFBConfigAttribSGIX;
   glx.ChooseFBConfigSGIX = Fake_glXChooseFBConfigSGIX;
   glx.CreateGLXPixmapWithConfigSGIX = Fake_glXCreateGLXPixmapWithConfigSGIX;
   glx.CreateContextWithConfigSGIX = Fake_glXCreateContextWithConfigSGIX;
   glx.GetVisualFromFBConfigSGIX = Fake_glXGetVisualFromFBConfigSGIX;
   glx.GetFBConfigFromVisualSGIX = Fake_glXGetFBConfigFromVisualSGIX;

   /*** GLX_SGIX_pbuffer ***/
   glx.CreateGLXPbufferSGIX = Fake_glXCreateGLXPbufferSGIX;
   glx.DestroyGLXPbufferSGIX = Fake_glXDestroyGLXPbufferSGIX;
   glx.QueryGLXPbufferSGIX = Fake_glXQueryGLXPbufferSGIX;
   glx.SelectEventSGIX = Fake_glXSelectEventSGIX;
   glx.GetSelectedEventSGIX = Fake_glXGetSelectedEventSGIX;

   /*** GLX_SGI_cushion ***/
   glx.CushionSGI = Fake_glXCushionSGI;

   /*** GLX_SGIX_video_resize ***/
   glx.BindChannelToWindowSGIX = Fake_glXBindChannelToWindowSGIX;
   glx.ChannelRectSGIX = Fake_glXChannelRectSGIX;
   glx.QueryChannelRectSGIX = Fake_glXQueryChannelRectSGIX;
   glx.QueryChannelDeltasSGIX = Fake_glXQueryChannelDeltasSGIX;
   glx.ChannelRectSyncSGIX = Fake_glXChannelRectSyncSGIX;

   /*** GLX_SGIX_dmbuffer **/
#if defined(_DM_BUFFER_H_)
   glx.AssociateDMPbufferSGIX = NULL;
#endif

   /*** GLX_SGIX_swap_group ***/
   glx.JoinSwapGroupSGIX = Fake_glXJoinSwapGroupSGIX;

   /*** GLX_SGIX_swap_barrier ***/
   glx.BindSwapBarrierSGIX = Fake_glXBindSwapBarrierSGIX;
   glx.QueryMaxSwapBarriersSGIX = Fake_glXQueryMaxSwapBarriersSGIX;

   /*** GLX_SUN_get_transparent_index ***/
   glx.GetTransparentIndexSUN = Fake_glXGetTransparentIndexSUN;

   /*** GLX_MESA_copy_sub_buffer ***/
   glx.CopySubBufferMESA = Fake_glXCopySubBufferMESA;

   /*** GLX_MESA_release_buffers ***/
   glx.ReleaseBuffersMESA = Fake_glXReleaseBuffersMESA;

   /*** GLX_MESA_pixmap_colormap ***/
   glx.CreateGLXPixmapMESA = Fake_glXCreateGLXPixmapMESA;

   /*** GLX_MESA_set_3dfx_mode ***/
   glx.Set3DfxModeMESA = Fake_glXSet3DfxModeMESA;

   /*** GLX_NV_vertex_array_range ***/
   glx.AllocateMemoryNV = Fake_glXAllocateMemoryNV;
   glx.FreeMemoryNV = Fake_glXFreeMemoryNV;

   /*** GLX_MESA_agp_offset ***/
   glx.GetAGPOffsetMESA = Fake_glXGetAGPOffsetMESA;

   return &glx;
}

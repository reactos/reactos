/**************************************************************************

Copyright 2000 Silicon Integrated Systems Corp, Inc., HsinChu, Taiwan.
Copyright 2003 Eric Anholt
All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
on the rights to use, copy, modify, merge, publish, distribute, sub
license, and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice (including the next
paragraph) shall be included in all copies or substantial portions of the
Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
ERIC ANHOLT OR SILICON INTEGRATED SYSTEMS CORP BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/
/* $XFree86$ */

/*
 * Authors:
 *   Sung-Ching Lin <sclin@sis.com.tw>
 *   Eric Anholt <anholt@FreeBSD.org>
 */

#ifndef _sis_ctx_h_
#define _sis_ctx_h_

#include "context.h"
#include "dri_util.h"
#include "drm.h"
#include "drm_sarea.h"
#include "xmlconfig.h"
#include "tnl/t_vertex.h"

#include "sis_screen.h"
#include "sis_reg.h"
#include "sis6326_reg.h"
#include "sis_dri.h"

/* for GLboolean */
#include <GL/gl.h>

#define PCI_CHIP_SIS300		0x0300
#define PCI_CHIP_SIS630		0x6300
#define PCI_CHIP_SIS540		0x5300

#define NEW_TEXTURING		0x1
#define NEW_TEXTURE_ENV		0x2

/* Flags for software fallback cases:
 */
#define SIS_FALLBACK_TEXTURE		0x0001
#define SIS_FALLBACK_TEXTURE0		0x0002
#define SIS_FALLBACK_TEXTURE1		0x0004
#define SIS_FALLBACK_TEXENV0		0x0008
#define SIS_FALLBACK_TEXENV1		0x0010
#define SIS_FALLBACK_DRAW_BUFFER	0x0020
#define SIS_FALLBACK_STENCIL		0x0040
#define SIS_FALLBACK_WRITEMASK		0x0080
#define SIS_FALLBACK_DISABLE		0x0100

/* Flags for hardware state that needs to be updated */
#define GFLAG_ENABLESETTING		0x00000001
#define GFLAG_ENABLESETTING2		0x00000002
#define GFLAG_ZSETTING			0x00000004
#define GFLAG_ALPHASETTING		0x00000008
#define GFLAG_DESTSETTING		0x00000010
#define GFLAG_LINESETTING		0x00000020
#define GFLAG_STENCILSETTING		0x00000040
#define GFLAG_FOGSETTING		0x00000080
#define GFLAG_DSTBLEND			0x00000100
#define GFLAG_CLIPPING			0x00000200
#define CFLAG_TEXTURERESET		0x00000400
#define GFLAG_TEXTUREMIPMAP		0x00000800
#define GFLAG_TEXBORDERCOLOR		0x00001000
#define GFLAG_TEXTUREADDRESS		0x00002000
#define GFLAG_TEXTUREENV		0x00004000
#define CFLAG_TEXTURERESET_1		0x00008000
#define GFLAG_TEXTUREMIPMAP_1		0x00010000
#define GFLAG_TEXBORDERCOLOR_1		0x00020000
#define GFLAG_TEXTUREADDRESS_1		0x00040000
#define GFLAG_TEXTUREENV_1		0x00080000
#define GFLAG_ALL			0x000fffff

#define GFLAG_TEXTURE_STATES (CFLAG_TEXTURERESET | GFLAG_TEXTUREMIPMAP | \
			      GFLAG_TEXBORDERCOLOR | GFLAG_TEXTUREADDRESS | \
			      CFLAG_TEXTURERESET_1 | GFLAG_TEXTUREMIPMAP_1 | \
			      GFLAG_TEXBORDERCOLOR_1 | \
			      GFLAG_TEXTUREADDRESS_1 | \
			      GFLAG_TEXTUREENV | GFLAG_TEXTUREENV_1)


#define GFLAG_RENDER_STATES  (GFLAG_ENABLESETTING | GFLAG_ENABLESETTING2 | \
			      GFLAG_ZSETTING | GFLAG_ALPHASETTING | \
			      GFLAG_DESTSETTING | GFLAG_FOGSETTING | \
			      GFLAG_STENCILSETTING | GFLAG_DSTBLEND | \
			      GFLAG_CLIPPING)

/* Use the templated vertex format:
 */
#define TAG(x) sis##x
#include "tnl_dd/t_dd_vertex.h"
#undef TAG

/* Subpixel offsets for window coordinates (triangles):
 */
#define SUBPIXEL_X  (-0.5F)
#define SUBPIXEL_Y  (-0.5F)

#define SIS_MAX_TEXTURE_SIZE 2048
#define SIS_MAX_TEXTURES 2
#define SIS_MAX_TEXTURE_LEVELS		11
#define SIS_MAX_FRAME_LENGTH 3

typedef struct {
   GLubyte *Data;		/* Pointer to texture in offscreen */
   GLuint memType;		/* VIDEO_TYPE or AGP_TYPE */
   void *handle;		/* Handle for sisFree*() */
   GLuint pitch;
   GLuint size;
} sisTexImage;

typedef struct sis_tex_obj {
   sisTexImage image[SIS_MAX_TEXTURE_LEVELS];	/* Image data for each mipmap
						 * level */
   GLenum format;		/* One of GL_ALPHA, GL_INTENSITY, GL_LUMINANCE,
				 * GL_LUMINANCE_ALPHA, GL_RGB, GL_RGBA
				 * MESA_YCBCR */
   GLint hwformat;              /* One of the TEXEL_ defines */
   GLint numImages;             /* Number of images loaded into .image */
} sisTexObj, *sisTexObjPtr;

/*
 ** Device dependent context state
 */
typedef struct __GLSiSTextureRec
{
  GLint hwTextureSet;
  GLint hwTextureMip;
  GLint hwTextureClrHigh;
  GLint hwTextureClrLow;
  GLint hwTexWidthHeight;	/* 6326: Texture Blending Setting */
  GLint hwTextureBorderColor;

  GLint texOffset0;
  GLint texOffset1;
  GLint texOffset2;
  GLint texOffset3;
  GLint texOffset4;
  GLint texOffset5;
  GLint texOffset6;
  GLint texOffset7;
  GLint texOffset8;
  GLint texOffset9;
  GLint texOffset10;
  GLint texOffset11;

  GLint texPitch01;
  GLint texPitch23;
  GLint texPitch45;
  GLint texPitch67;
  GLint texPitch89;
  GLint texPitch10;
} __GLSiSTexture;

typedef struct __GLSiSHardwareRec
{
  GLint hwCapEnable, hwCapEnable2;	/*  Enable Setting */

  GLint hwOffsetZ, hwZ;		/* Z Setting */

  GLint hwZBias, hwZMask;	/* Z Setting */

  GLint hwAlpha;		/* Alpha Setting */

  GLint hwDstSet, hwDstMask;	/* Destination Setting */

  GLint hwOffsetDest;		/* Destination Setting */

  GLint hwLinePattern;		/* Line Setting */

  GLint hwFog;			/* Fog Setting */

  GLint hwFogFar, hwFogInverse;	/* Fog Distance setting */

  GLint hwFogDensity;		/* Fog factor & density */

  GLint hwStSetting, hwStSetting2;	/* Stencil Setting */

  GLint hwStOffset;		/* Stencil Setting */

  GLint hwDstSrcBlend;		/* Blending mode Setting */

  GLint clipTopBottom;		/* Clip for Top & Bottom */

  GLint clipLeftRight;		/* Clip for Left & Right */

  struct __GLSiSTextureRec texture[2];

  GLint hwTexEnvColor;		/* Texture Blending Setting */

  GLint hwTexBlendSet;		/* 6326 */
  GLint hwTexBlendColor0;
  GLint hwTexBlendColor1;
  GLint hwTexBlendAlpha0;
  GLint hwTexBlendAlpha1;

}
__GLSiSHardware;

typedef struct sis_context sisContextRec;
typedef struct sis_context *sisContextPtr;

typedef void (*sis_quad_func)( sisContextPtr, 
			       sisVertex *,
			       sisVertex *,
			       sisVertex *,
			       sisVertex * );

typedef void (*sis_tri_func)( sisContextPtr, 
			      sisVertex *,
			      sisVertex *,
			      sisVertex * );

typedef void (*sis_line_func)( sisContextPtr, 
			       sisVertex *,
			       sisVertex * );

typedef void (*sis_point_func)( sisContextPtr,
				sisVertex * );

/**
 * Derived from gl_renderbuffer.
 */
struct sis_renderbuffer {
   struct gl_renderbuffer Base;  /* must be first! */
   drmSize size;
   GLuint offset;
   void *handle;
   GLuint pitch;
   GLuint bpp;
   char *map;
};

/* Device dependent context state */

struct sis_context
{
  /* This must be first in this structure */
  GLcontext *glCtx;

  /* Vertex state */
  GLuint vertex_size;
  struct tnl_attr_map vertex_attrs[VERT_ATTRIB_MAX];
  GLuint vertex_attr_count;
  char *verts;			/* points to tnl->clipspace.vertex_buf */

  /* Vertex buffer (in system memory or AGP) state. */
  unsigned char *vb;		/* Beginning of vertex buffer */
  unsigned char *vb_cur;	/* Current write location in vertex buffer */
  unsigned char *vb_last;	/* Last written location in vertex buffer */
  unsigned char *vb_end;	/* End of vertex buffer */
  void *vb_agp_handle;
  GLuint vb_agp_offset;
  GLboolean using_agp;
  GLint coloroffset;		/* Offset in vertex format of current color */
  GLint specoffset;		/* Offset in vertex format of specular color */

  GLuint NewGLState;
  GLuint Fallback;
  GLuint RenderIndex;
  GLfloat hw_viewport[16];
  GLfloat depth_scale;

  unsigned int virtualX, virtualY;
  unsigned int bytesPerPixel;
  unsigned char *IOBase;
  unsigned char *FbBase;
  unsigned int displayWidth;

  /* HW RGBA layout */
  unsigned int redMask, greenMask, blueMask, alphaMask;
  unsigned int colorFormat;

  /* Z format */
  unsigned int zFormat;

  /* Clear patterns, 4 bytes */
  unsigned int clearColorPattern;
  unsigned int clearZStencilPattern;

  /* Fallback rasterization functions 
   */
  sis_point_func draw_point;
  sis_line_func draw_line;
  sis_tri_func draw_tri;
  sis_quad_func draw_quad;

  GLuint hw_primitive;
  GLenum raster_primitive;
  GLenum render_primitive;

  /* DRM fd */
  int driFd;
  
  /* AGP Memory */
  unsigned int AGPSize;
  unsigned char *AGPBase;
  unsigned int AGPAddr;
  
  /* register 0x89F4 */
  GLint AGPParseSet;

  /* register 0x89F8 */
  GLint dwPrimitiveSet;

  __GLSiSHardware prev, current;

  int Chipset;
  GLboolean is6326;

  GLint drawableID;

  GLint GlobalFlag;
  DECLARE_RENDERINPUTS(last_tcl_state_bitset);

  /* Stereo */
  GLboolean useStereo;
  GLboolean stereoEnabled;
  int stereo_drawIndex;
  int stereo_drawSide;
  GLboolean irqEnabled;

  GLboolean clearTexCache;

  GLuint TexStates[SIS_MAX_TEXTURES];
  GLuint PrevTexFormat[SIS_MAX_TEXTURES];

  int *CurrentQueueLenPtr;
  unsigned int *FrameCountPtr;

  /* Front/back/depth buffer info */
  GLuint width, height;			/* size of buffers */
  GLint bottom;				/* used for FLIP macro */
  /* XXX These don't belong here.  They should be per-drawable state. */
  struct sis_renderbuffer front;
  struct sis_renderbuffer back;
  struct sis_renderbuffer depth;
  struct sis_renderbuffer stencil; /* mirrors depth */

  /* Mirrors of some DRI state
   */
  __DRIcontextPrivate	*driContext;	/* DRI context */
  __DRIscreenPrivate	*driScreen;	/* DRI screen */
  __DRIdrawablePrivate	*driDrawable;	/* DRI drawable bound to this ctx */

  unsigned int lastStamp;	        /* mirror driDrawable->lastStamp */

  drm_context_t hHWContext;
  drm_hw_lock_t *driHwLock;

  sisScreenPtr sisScreen;		/* Screen private DRI data */
  SISSAREAPrivPtr sarea;		/* Private SAREA data */

   /* Configuration cache */
   driOptionCache optionCache;
    GLint texture_depth;
};

#define SIS_CONTEXT(ctx)		((sisContextPtr)(ctx->DriverCtx))

/* Macros */
#define GET_IOBase(x) ((x)->IOBase)

#define Y_FLIP(Y)  (smesa->bottom - (Y))

#define SISPACKCOLOR565( r, g, b )					\
   ((((r) & 0xf8) << 8) | (((g) & 0xfc) << 3) | (((b) & 0xf8) >> 3))

#define SISPACKCOLOR8888( r, g, b, a )					\
   (((a) << 24) | ((r) << 16) | ((g) << 8) | (b))

#define SIS_VERBOSE 0


#define MMIO(reg, value) \
{\
   *(volatile GLint *)(smesa->IOBase + (reg)) = value;			\
}

#define MMIO_READ(reg) *(volatile GLint *)(smesa->IOBase + (reg))
#define MMIO_READf(reg) *(volatile GLfloat *)(smesa->IOBase + (reg))

#if defined(__i386__) || defined(__amd64__)
#define MMIO_WMB()	__asm __volatile("" : : : "memory")
#else
#error platform needs WMB
#endif

#define mEndPrimitive()  \
{       \
   *(volatile GLubyte *)(smesa->IOBase + REG_3D_EndPrimitiveList) = 0xff; \
   *(volatile GLuint *)(smesa->IOBase + 0x8b60) = 0xffffffff;		\
}

#define sis_fatal_error(...)						\
do {									\
	fprintf(stderr, "[%s:%d]:", __FILE__, __LINE__);		\
	fprintf(stderr, __VA_ARGS__);					\
	exit(-1);							\
} while (0)

/* Lock required */
#define mWait3DCmdQueue(wLen)						\
/* Update the mirrored queue pointer if it doesn't indicate enough space */ \
if (*(smesa->CurrentQueueLenPtr) < (wLen)) {				\
   *(smesa->CurrentQueueLenPtr) =					\
      (*(GLint *)(GET_IOBase(smesa) + REG_CommandQueue) & MASK_QueueLen) - 20; \
   /* Spin and wait if the queue is actually too full */		\
   if (*(smesa->CurrentQueueLenPtr) < (wLen))				\
      WaitingFor3dIdle(smesa, wLen);					\
   *(smesa->CurrentQueueLenPtr) -= wLen;				\
}

enum _sis_verbose {
	VERBOSE_SIS_BUFFER  = 0x1,
	VERBOSE_SIS_MEMORY  = 0x2
};

extern GLboolean sisCreateContext( const __GLcontextModes *glVisual,
				   __DRIcontextPrivate *driContextPriv,
                                   void *sharedContextPrivate );
extern void sisDestroyContext( __DRIcontextPrivate * );

void sisReAllocateBuffers(GLcontext *ctx, GLframebuffer *drawbuffer,
                          GLuint width, GLuint height);

extern GLboolean sisMakeCurrent( __DRIcontextPrivate *driContextPriv,
                                  __DRIdrawablePrivate *driDrawPriv,
                                  __DRIdrawablePrivate *driReadPriv );

extern GLboolean sisUnbindContext( __DRIcontextPrivate *driContextPriv );

void WaitEngIdle (sisContextPtr smesa);
void Wait2DEngIdle (sisContextPtr smesa);
void WaitingFor3dIdle(sisContextPtr smesa, int wLen);

/* update to hw */
extern void sis_update_texture_state( sisContextPtr smesa );
extern void sis_update_render_state( sisContextPtr smesa );
extern void sis6326_update_texture_state( sisContextPtr smesa );
extern void sis6326_update_render_state( sisContextPtr smesa );

/* ================================================================
 * Debugging:
 */
#define DO_DEBUG		1

#if DO_DEBUG
extern int SIS_DEBUG;
#else
#define SIS_DEBUG		0
#endif

#define DEBUG_FALLBACKS		0x01

#endif /* _sis_ctx_h_ */

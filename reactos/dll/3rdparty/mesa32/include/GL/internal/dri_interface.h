/*
 * Copyright 1998-1999 Precision Insight, Inc., Cedar Park, Texas.
 * Copyright 2007-2008 Red Hat, Inc.
 * (C) Copyright IBM Corporation 2004
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDERS AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/**
 * \file dri_interface.h
 *
 * This file contains all the types and functions that define the interface
 * between a DRI driver and driver loader.  Currently, the most common driver
 * loader is the XFree86 libGL.so.  However, other loaders do exist, and in
 * the future the server-side libglx.a will also be a loader.
 * 
 * \author Kevin E. Martin <kevin@precisioninsight.com>
 * \author Ian Romanick <idr@us.ibm.com>
 * \author Kristian Høgsberg <krh@redhat.com>
 */

#ifndef DRI_INTERFACE_H
#define DRI_INTERFACE_H

/* Make this something other than __APPLE__ for other arcs with no drm.h */
#ifndef __APPLE__
#include <drm.h>
#else
typedef unsigned int drm_context_t;
typedef unsigned int drm_drawable_t;
typedef struct drm_clip_rect drm_clip_rect_t;
#endif

/**
 * \name DRI interface structures
 *
 * The following structures define the interface between the GLX client
 * side library and the DRI (direct rendering infrastructure).
 */
/*@{*/
typedef struct __DRIdisplayRec		__DRIdisplay;
typedef struct __DRIscreenRec		__DRIscreen;
typedef struct __DRIcontextRec		__DRIcontext;
typedef struct __DRIdrawableRec		__DRIdrawable;
typedef struct __DRIconfigRec		__DRIconfig;
typedef struct __DRIframebufferRec	__DRIframebuffer;
typedef struct __DRIversionRec		__DRIversion;

typedef struct __DRIcoreExtensionRec		__DRIcoreExtension;
typedef struct __DRIextensionRec		__DRIextension;
typedef struct __DRIcopySubBufferExtensionRec	__DRIcopySubBufferExtension;
typedef struct __DRIswapControlExtensionRec	__DRIswapControlExtension;
typedef struct __DRIallocateExtensionRec	__DRIallocateExtension;
typedef struct __DRIframeTrackingExtensionRec	__DRIframeTrackingExtension;
typedef struct __DRImediaStreamCounterExtensionRec	__DRImediaStreamCounterExtension;
typedef struct __DRItexOffsetExtensionRec	__DRItexOffsetExtension;
typedef struct __DRItexBufferExtensionRec	__DRItexBufferExtension;
typedef struct __DRIlegacyExtensionRec		__DRIlegacyExtension;
typedef struct __DRIswrastExtensionRec		__DRIswrastExtension;
/*@}*/


/**
 * Extension struct.  Drivers 'inherit' from this struct by embedding
 * it as the first element in the extension struct.
 *
 * We never break API in for a DRI extension.  If we need to change
 * the way things work in a non-backwards compatible manner, we
 * introduce a new extension.  During a transition period, we can
 * leave both the old and the new extension in the driver, which
 * allows us to move to the new interface without having to update the
 * loader(s) in lock step.
 *
 * However, we can add entry points to an extension over time as long
 * as we don't break the old ones.  As we add entry points to an
 * extension, we increase the version number.  The corresponding
 * #define can be used to guard code that accesses the new entry
 * points at compile time and the version field in the extension
 * struct can be used at run-time to determine how to use the
 * extension.
 */
struct __DRIextensionRec {
    const char *name;
    int version;
};

/**
 * The first set of extension are the screen extensions, returned by
 * __DRIcore::getExtensions().  This entry point will return a list of
 * extensions and the loader can use the ones it knows about by
 * casting them to more specific extensions and advertising any GLX
 * extensions the DRI extensions enables.
 */

/**
 * Used by drivers to indicate support for setting the read drawable.
 */
#define __DRI_READ_DRAWABLE "DRI_ReadDrawable"
#define __DRI_READ_DRAWABLE_VERSION 1

/**
 * Used by drivers that implement the GLX_MESA_copy_sub_buffer extension.
 */
#define __DRI_COPY_SUB_BUFFER "DRI_CopySubBuffer"
#define __DRI_COPY_SUB_BUFFER_VERSION 1
struct __DRIcopySubBufferExtensionRec {
    __DRIextension base;
    void (*copySubBuffer)(__DRIdrawable *drawable, int x, int y, int w, int h);
};

/**
 * Used by drivers that implement the GLX_SGI_swap_control or
 * GLX_MESA_swap_control extension.
 */
#define __DRI_SWAP_CONTROL "DRI_SwapControl"
#define __DRI_SWAP_CONTROL_VERSION 1
struct __DRIswapControlExtensionRec {
    __DRIextension base;
    void (*setSwapInterval)(__DRIdrawable *drawable, unsigned int inteval);
    unsigned int (*getSwapInterval)(__DRIdrawable *drawable);
};

/**
 * Used by drivers that implement the GLX_MESA_allocate_memory.
 */
#define __DRI_ALLOCATE "DRI_Allocate"
#define __DRI_ALLOCATE_VERSION 1
struct __DRIallocateExtensionRec {
    __DRIextension base;

    void *(*allocateMemory)(__DRIscreen *screen, GLsizei size,
			    GLfloat readfreq, GLfloat writefreq,
			    GLfloat priority);
   
    void (*freeMemory)(__DRIscreen *screen, GLvoid *pointer);
   
    GLuint (*memoryOffset)(__DRIscreen *screen, const GLvoid *pointer);
};

/**
 * Used by drivers that implement the GLX_MESA_swap_frame_usage extension.
 */
#define __DRI_FRAME_TRACKING "DRI_FrameTracking"
#define __DRI_FRAME_TRACKING_VERSION 1
struct __DRIframeTrackingExtensionRec {
    __DRIextension base;

    /**
     * Enable or disable frame usage tracking.
     * 
     * \since Internal API version 20030317.
     */
    int (*frameTracking)(__DRIdrawable *drawable, GLboolean enable);

    /**
     * Retrieve frame usage information.
     * 
     * \since Internal API version 20030317.
     */
    int (*queryFrameTracking)(__DRIdrawable *drawable,
			      int64_t * sbc, int64_t * missedFrames,
			      float * lastMissedUsage, float * usage);
};


/**
 * Used by drivers that implement the GLX_SGI_video_sync extension.
 */
#define __DRI_MEDIA_STREAM_COUNTER "DRI_MediaStreamCounter"
#define __DRI_MEDIA_STREAM_COUNTER_VERSION 1
struct __DRImediaStreamCounterExtensionRec {
    __DRIextension base;

    /**
     * Wait for the MSC to equal target_msc, or, if that has already passed,
     * the next time (MSC % divisor) is equal to remainder.  If divisor is
     * zero, the function will return as soon as MSC is greater than or equal
     * to target_msc.
     */
    int (*waitForMSC)(__DRIdrawable *drawable,
		      int64_t target_msc, int64_t divisor, int64_t remainder,
		      int64_t * msc, int64_t * sbc);

    /**
     * Get the number of vertical refreshes since some point in time before
     * this function was first called (i.e., system start up).
     */
    int (*getDrawableMSC)(__DRIscreen *screen, __DRIdrawable *drawable,
			  int64_t *msc);
};


#define __DRI_TEX_OFFSET "DRI_TexOffset"
#define __DRI_TEX_OFFSET_VERSION 1
struct __DRItexOffsetExtensionRec {
    __DRIextension base;

    /**
     * Method to override base texture image with a driver specific 'offset'.
     * The depth passed in allows e.g. to ignore the alpha channel of texture
     * images where the non-alpha components don't occupy a whole texel.
     *
     * For GLX_EXT_texture_from_pixmap with AIGLX.
     */
    void (*setTexOffset)(__DRIcontext *pDRICtx, GLint texname,
			 unsigned long long offset, GLint depth, GLuint pitch);
};


#define __DRI_TEX_BUFFER "DRI_TexBuffer"
#define __DRI_TEX_BUFFER_VERSION 1
struct __DRItexBufferExtensionRec {
    __DRIextension base;

    /**
     * Method to override base texture image with the contents of a
     * __DRIdrawable. 
     *
     * For GLX_EXT_texture_from_pixmap with AIGLX.
     */
    void (*setTexBuffer)(__DRIcontext *pDRICtx,
			 GLint target,
			 __DRIdrawable *pDraw);
};


/**
 * XML document describing the configuration options supported by the
 * driver.
 */
extern const char __driConfigOptions[];

/*@}*/

/**
 * The following extensions describe loader features that the DRI
 * driver can make use of.  Some of these are mandatory, such as the
 * getDrawableInfo extension for DRI and the DRI Loader extensions for
 * DRI2, while others are optional, and if present allow the driver to
 * expose certain features.  The loader pass in a NULL terminated
 * array of these extensions to the driver in the createNewScreen
 * constructor.
 */

typedef struct __DRIgetDrawableInfoExtensionRec __DRIgetDrawableInfoExtension;
typedef struct __DRIsystemTimeExtensionRec __DRIsystemTimeExtension;
typedef struct __DRIdamageExtensionRec __DRIdamageExtension;
typedef struct __DRIloaderExtensionRec __DRIloaderExtension;
typedef struct __DRIswrastLoaderExtensionRec __DRIswrastLoaderExtension;


/**
 * Callback to getDrawableInfo protocol
 */
#define __DRI_GET_DRAWABLE_INFO "DRI_GetDrawableInfo"
#define __DRI_GET_DRAWABLE_INFO_VERSION 1
struct __DRIgetDrawableInfoExtensionRec {
    __DRIextension base;

    /**
     * This function is used to get information about the position, size, and
     * clip rects of a drawable.
     */
    GLboolean (* getDrawableInfo) ( __DRIdrawable *drawable,
	unsigned int * index, unsigned int * stamp,
        int * x, int * y, int * width, int * height,
        int * numClipRects, drm_clip_rect_t ** pClipRects,
        int * backX, int * backY,
	int * numBackClipRects, drm_clip_rect_t ** pBackClipRects,
	void *loaderPrivate);
};

/**
 * Callback to get system time for media stream counter extensions.
 */
#define __DRI_SYSTEM_TIME "DRI_SystemTime"
#define __DRI_SYSTEM_TIME_VERSION 1
struct __DRIsystemTimeExtensionRec {
    __DRIextension base;

    /**
     * Get the 64-bit unadjusted system time (UST).
     */
    int (*getUST)(int64_t * ust);

    /**
     * Get the media stream counter (MSC) rate.
     * 
     * Matching the definition in GLX_OML_sync_control, this function returns
     * the rate of the "media stream counter".  In practical terms, this is
     * the frame refresh rate of the display.
     */
    GLboolean (*getMSCRate)(__DRIdrawable *draw,
			    int32_t * numerator, int32_t * denominator,
			    void *loaderPrivate);
};

/**
 * Damage reporting
 */
#define __DRI_DAMAGE "DRI_Damage"
#define __DRI_DAMAGE_VERSION 1
struct __DRIdamageExtensionRec {
    __DRIextension base;

    /**
     * Reports areas of the given drawable which have been modified by the
     * driver.
     *
     * \param drawable which the drawing was done to.
     * \param rects rectangles affected, with the drawable origin as the
     *	      origin.
     * \param x X offset of the drawable within the screen (used in the
     *	      front_buffer case)
     * \param y Y offset of the drawable within the screen.
     * \param front_buffer boolean flag for whether the drawing to the
     * 	      drawable was actually done directly to the front buffer (instead
     *	      of backing storage, for example)
     * \param loaderPrivate the data passed in at createNewDrawable time
     */
    void (*reportDamage)(__DRIdrawable *draw,
			 int x, int y,
			 drm_clip_rect_t *rects, int num_rects,
			 GLboolean front_buffer,
			 void *loaderPrivate);
};

/**
 * DRI2 Loader extension.  This extension describes the basic
 * functionality the loader needs to provide for the DRI driver.
 */
#define __DRI_LOADER "DRI_Loader"
#define __DRI_LOADER_VERSION 1
struct __DRIloaderExtensionRec {
    __DRIextension base;

    /**
     * Ping the windowing system to get it to reemit info for the
     * specified drawable in the DRI2 event buffer.
     *
     * \param draw the drawable for which to request info
     * \param tail the new event buffer tail pointer
     */
    void (*reemitDrawableInfo)(__DRIdrawable *draw, unsigned int *tail,
			       void *loaderPrivate);

    void (*postDamage)(__DRIdrawable *draw, struct drm_clip_rect *rects,
		       int num_rects, void *loaderPrivate);
};

#define __DRI_SWRAST_IMAGE_OP_DRAW	1
#define __DRI_SWRAST_IMAGE_OP_CLEAR	2
#define __DRI_SWRAST_IMAGE_OP_SWAP	3

/**
 * SWRast Loader extension.
 */
#define __DRI_SWRAST_LOADER "DRI_SWRastLoader"
#define __DRI_SWRAST_LOADER_VERSION 1
struct __DRIswrastLoaderExtensionRec {
    __DRIextension base;

    /*
     * Drawable position and size
     */
    void (*getDrawableInfo)(__DRIdrawable *drawable,
			    int *x, int *y, int *width, int *height,
			    void *loaderPrivate);

    /**
     * Put image to drawable
     */
    void (*putImage)(__DRIdrawable *drawable, int op,
		     int x, int y, int width, int height, char *data,
		     void *loaderPrivate);

    /**
     * Get image from drawable
     */
    void (*getImage)(__DRIdrawable *drawable,
		     int x, int y, int width, int height, char *data,
		     void *loaderPrivate);
};

/**
 * The remaining extensions describe driver extensions, immediately
 * available interfaces provided by the driver.  To start using the
 * driver, dlsym() for the __DRI_DRIVER_EXTENSIONS symbol and look for
 * the extension you need in the array.
 */
#define __DRI_DRIVER_EXTENSIONS "__driDriverExtensions"

/**
 * Tokens for __DRIconfig attribs.  A number of attributes defined by
 * GLX or EGL standards are not in the table, as they must be provided
 * by the loader.  For example, FBConfig ID or visual ID, drawable type.
 */

#define __DRI_ATTRIB_BUFFER_SIZE		 1
#define __DRI_ATTRIB_LEVEL			 2
#define __DRI_ATTRIB_RED_SIZE			 3
#define __DRI_ATTRIB_GREEN_SIZE			 4
#define __DRI_ATTRIB_BLUE_SIZE			 5
#define __DRI_ATTRIB_LUMINANCE_SIZE		 6
#define __DRI_ATTRIB_ALPHA_SIZE			 7
#define __DRI_ATTRIB_ALPHA_MASK_SIZE		 8
#define __DRI_ATTRIB_DEPTH_SIZE			 9
#define __DRI_ATTRIB_STENCIL_SIZE		10
#define __DRI_ATTRIB_ACCUM_RED_SIZE		11
#define __DRI_ATTRIB_ACCUM_GREEN_SIZE		12
#define __DRI_ATTRIB_ACCUM_BLUE_SIZE		13
#define __DRI_ATTRIB_ACCUM_ALPHA_SIZE		14
#define __DRI_ATTRIB_SAMPLE_BUFFERS		15
#define __DRI_ATTRIB_SAMPLES			16
#define __DRI_ATTRIB_RENDER_TYPE		17
#define __DRI_ATTRIB_CONFIG_CAVEAT		18
#define __DRI_ATTRIB_CONFORMANT			19
#define __DRI_ATTRIB_DOUBLE_BUFFER		20
#define __DRI_ATTRIB_STEREO			21
#define __DRI_ATTRIB_AUX_BUFFERS		22
#define __DRI_ATTRIB_TRANSPARENT_TYPE		23
#define __DRI_ATTRIB_TRANSPARENT_INDEX_VALUE	24
#define __DRI_ATTRIB_TRANSPARENT_RED_VALUE	25
#define __DRI_ATTRIB_TRANSPARENT_GREEN_VALUE	26
#define __DRI_ATTRIB_TRANSPARENT_BLUE_VALUE	27
#define __DRI_ATTRIB_TRANSPARENT_ALPHA_VALUE	28
#define __DRI_ATTRIB_FLOAT_MODE			29
#define __DRI_ATTRIB_RED_MASK			30
#define __DRI_ATTRIB_GREEN_MASK			31
#define __DRI_ATTRIB_BLUE_MASK			32
#define __DRI_ATTRIB_ALPHA_MASK			33
#define __DRI_ATTRIB_MAX_PBUFFER_WIDTH		34
#define __DRI_ATTRIB_MAX_PBUFFER_HEIGHT		35
#define __DRI_ATTRIB_MAX_PBUFFER_PIXELS		36
#define __DRI_ATTRIB_OPTIMAL_PBUFFER_WIDTH	37
#define __DRI_ATTRIB_OPTIMAL_PBUFFER_HEIGHT	38
#define __DRI_ATTRIB_VISUAL_SELECT_GROUP	39
#define __DRI_ATTRIB_SWAP_METHOD		40
#define __DRI_ATTRIB_MAX_SWAP_INTERVAL		41
#define __DRI_ATTRIB_MIN_SWAP_INTERVAL		42
#define __DRI_ATTRIB_BIND_TO_TEXTURE_RGB	43
#define __DRI_ATTRIB_BIND_TO_TEXTURE_RGBA	44
#define __DRI_ATTRIB_BIND_TO_MIPMAP_TEXTURE	45
#define __DRI_ATTRIB_BIND_TO_TEXTURE_TARGETS	46
#define __DRI_ATTRIB_YINVERTED			47

/* __DRI_ATTRIB_RENDER_TYPE */
#define __DRI_ATTRIB_RGBA_BIT			0x01	
#define __DRI_ATTRIB_COLOR_INDEX_BIT		0x02
#define __DRI_ATTRIB_LUMINANCE_BIT		0x04

/* __DRI_ATTRIB_CONFIG_CAVEAT */
#define __DRI_ATTRIB_SLOW_BIT			0x01
#define __DRI_ATTRIB_NON_CONFORMANT_CONFIG	0x02

/* __DRI_ATTRIB_TRANSPARENT_TYPE */
#define __DRI_ATTRIB_TRANSPARENT_RGB		0x00
#define __DRI_ATTRIB_TRANSPARENT_INDEX		0x01

/* __DRI_ATTRIB_BIND_TO_TEXTURE_TARGETS	 */
#define __DRI_ATTRIB_TEXTURE_1D_BIT		0x01
#define __DRI_ATTRIB_TEXTURE_2D_BIT		0x02
#define __DRI_ATTRIB_TEXTURE_RECTANGLE_BIT	0x04

/**
 * This extension defines the core DRI functionality.
 */
#define __DRI_CORE "DRI_Core"
#define __DRI_CORE_VERSION 1

struct __DRIcoreExtensionRec {
    __DRIextension base;

    __DRIscreen *(*createNewScreen)(int screen, int fd,
				    unsigned int sarea_handle,
				    const __DRIextension **extensions,
				    const __DRIconfig ***driverConfigs,
				    void *loaderPrivate);

    void (*destroyScreen)(__DRIscreen *screen);

    const __DRIextension **(*getExtensions)(__DRIscreen *screen);

    int (*getConfigAttrib)(const __DRIconfig *config,
			   unsigned int attrib,
			   unsigned int *value);

    int (*indexConfigAttrib)(const __DRIconfig *config, int index,
			     unsigned int *attrib, unsigned int *value);

    __DRIdrawable *(*createNewDrawable)(__DRIscreen *screen,
					const __DRIconfig *config,
					unsigned int drawable_id,
					unsigned int head,
					void *loaderPrivate);

    void (*destroyDrawable)(__DRIdrawable *drawable);

    void (*swapBuffers)(__DRIdrawable *drawable);

    __DRIcontext *(*createNewContext)(__DRIscreen *screen,
				      const __DRIconfig *config,
				      __DRIcontext *shared,
				      void *loaderPrivate);

    int (*copyContext)(__DRIcontext *dest,
		       __DRIcontext *src,
		       unsigned long mask);

    void (*destroyContext)(__DRIcontext *context);

    int (*bindContext)(__DRIcontext *ctx,
		       __DRIdrawable *pdraw,
		       __DRIdrawable *pread);

    int (*unbindContext)(__DRIcontext *ctx);
};

/**
 * Stored version of some component (i.e., server-side DRI module, kernel-side
 * DRM, etc.).
 * 
 * \todo
 * There are several data structures that explicitly store a major version,
 * minor version, and patch level.  These structures should be modified to
 * have a \c __DRIversionRec instead.
 */
struct __DRIversionRec {
    int    major;        /**< Major version number. */
    int    minor;        /**< Minor version number. */
    int    patch;        /**< Patch-level. */
};

/**
 * Framebuffer information record.  Used by libGL to communicate information
 * about the framebuffer to the driver's \c __driCreateNewScreen function.
 * 
 * In XFree86, most of this information is derrived from data returned by
 * calling \c XF86DRIGetDeviceInfo.
 *
 * \sa XF86DRIGetDeviceInfo __DRIdisplayRec::createNewScreen
 *     __driUtilCreateNewScreen CallCreateNewScreen
 *
 * \bug This structure could be better named.
 */
struct __DRIframebufferRec {
    unsigned char *base;    /**< Framebuffer base address in the CPU's
			     * address space.  This value is calculated by
			     * calling \c drmMap on the framebuffer handle
			     * returned by \c XF86DRIGetDeviceInfo (or a
			     * similar function).
			     */
    int size;               /**< Framebuffer size, in bytes. */
    int stride;             /**< Number of bytes from one line to the next. */
    int width;              /**< Pixel width of the framebuffer. */
    int height;             /**< Pixel height of the framebuffer. */
    int dev_priv_size;      /**< Size of the driver's dev-priv structure. */
    void *dev_priv;         /**< Pointer to the driver's dev-priv structure. */
};


/**
 * This extension provides alternative screen, drawable and context
 * constructors for legacy DRI functionality.  This is used in
 * conjunction with the core extension.
 */
#define __DRI_LEGACY "DRI_Legacy"
#define __DRI_LEGACY_VERSION 1

struct __DRIlegacyExtensionRec {
    __DRIextension base;

    __DRIscreen *(*createNewScreen)(int screen,
				    const __DRIversion *ddx_version,
				    const __DRIversion *dri_version,
				    const __DRIversion *drm_version,
				    const __DRIframebuffer *frame_buffer,
				    void *pSAREA, int fd, 
				    const __DRIextension **extensions,
				    const __DRIconfig ***driver_configs,
				    void *loaderPrivate);

    __DRIdrawable *(*createNewDrawable)(__DRIscreen *screen,
					const __DRIconfig *config,
					drm_drawable_t hwDrawable,
					int renderType, const int *attrs,
					void *loaderPrivate);

    __DRIcontext *(*createNewContext)(__DRIscreen *screen,
				      const __DRIconfig *config,
				      int render_type,
				      __DRIcontext *shared,
				      drm_context_t hwContext,
				      void *loaderPrivate);
};

/**
 * This extension provides alternative screen, drawable and context
 * constructors for swrast DRI functionality.  This is used in
 * conjunction with the core extension.
 */
#define __DRI_SWRAST "DRI_SWRast"
#define __DRI_SWRAST_VERSION 1

struct __DRIswrastExtensionRec {
    __DRIextension base;

    __DRIscreen *(*createNewScreen)(int screen,
				    const __DRIextension **extensions,
				    const __DRIconfig ***driver_configs,
				    void *loaderPrivate);

    __DRIdrawable *(*createNewDrawable)(__DRIscreen *screen,
					const __DRIconfig *config,
					void *loaderPrivate);
};

#endif

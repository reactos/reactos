/*
 * Functions to use the XRender extension
 *
 * Copyright 2001, 2002 Huw D M Davies for CodeWeavers
 * Copyright 2009 Roderick Colenbrander
 *
 * Some parts also:
 * Copyright 2000 Keith Packard, member of The XFree86 Project, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */
#include "config.h"
#include "wine/port.h"

#include <assert.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#include "windef.h"
#include "winbase.h"
#include "x11drv.h"
#include "winternl.h"
#include "wine/library.h"
#include "wine/unicode.h"
#include "wine/debug.h"

int using_client_side_fonts = FALSE;

WINE_DEFAULT_DEBUG_CHANNEL(xrender);

#ifdef SONAME_LIBXRENDER

static BOOL X11DRV_XRender_Installed = FALSE;

#include <X11/Xlib.h>
#include <X11/extensions/Xrender.h>

#ifndef RepeatNone  /* added in 0.10 */
#define RepeatNone    0
#define RepeatNormal  1
#define RepeatPad     2
#define RepeatReflect 3
#endif

typedef enum wine_xrformat
{
  WXR_FORMAT_MONO,
  WXR_FORMAT_GRAY,
  WXR_FORMAT_X1R5G5B5,
  WXR_FORMAT_X1B5G5R5,
  WXR_FORMAT_R5G6B5,
  WXR_FORMAT_B5G6R5,
  WXR_FORMAT_R8G8B8,
  WXR_FORMAT_B8G8R8,
  WXR_FORMAT_A8R8G8B8,
  WXR_FORMAT_B8G8R8A8,
  WXR_FORMAT_X8R8G8B8,
  WXR_FORMAT_B8G8R8X8,
  WXR_NB_FORMATS
} WXRFormat;

typedef struct wine_xrender_format_template
{
    WXRFormat wxr_format;
    unsigned int depth;
    unsigned int alpha;
    unsigned int alphaMask;
    unsigned int red;
    unsigned int redMask;
    unsigned int green;
    unsigned int greenMask;
    unsigned int blue;
    unsigned int blueMask;
} WineXRenderFormatTemplate;

static const WineXRenderFormatTemplate wxr_formats_template[WXR_NB_FORMATS] =
{
    /* Format               depth   alpha   mask    red     mask    green   mask    blue    mask*/
    {WXR_FORMAT_MONO,       1,      0,      0x01,   0,      0,      0,      0,      0,      0       },
    {WXR_FORMAT_GRAY,       8,      0,      0xff,   0,      0,      0,      0,      0,      0       },
    {WXR_FORMAT_X1R5G5B5,   16,     0,      0,      10,     0x1f,   5,      0x1f,   0,      0x1f    },
    {WXR_FORMAT_X1B5G5R5,   16,     0,      0,      0,      0x1f,   5,      0x1f,   10,     0x1f    },
    {WXR_FORMAT_R5G6B5,     16,     0,      0,      11,     0x1f,   5,      0x3f,   0,      0x1f    },
    {WXR_FORMAT_B5G6R5,     16,     0,      0,      0,      0x1f,   5,      0x3f,   11,     0x1f    },
    {WXR_FORMAT_R8G8B8,     24,     0,      0,      16,     0xff,   8,      0xff,   0,      0xff    },
    {WXR_FORMAT_B8G8R8,     24,     0,      0,      0,      0xff,   8,      0xff,   16,     0xff    },
    {WXR_FORMAT_A8R8G8B8,   32,     24,     0xff,   16,     0xff,   8,      0xff,   0,      0xff    },
    {WXR_FORMAT_B8G8R8A8,   32,     0,      0xff,   8,      0xff,   16,     0xff,   24,     0xff    },
    {WXR_FORMAT_X8R8G8B8,   32,     0,      0,      16,     0xff,   8,      0xff,   0,      0xff    },
    {WXR_FORMAT_B8G8R8X8,   32,     0,      0,      8,      0xff,   16,     0xff,   24,     0xff    },
};

typedef struct wine_xrender_format
{
    WXRFormat               format;
    XRenderPictFormat       *pict_format;
} WineXRenderFormat;

static WineXRenderFormat wxr_formats[WXR_NB_FORMATS];
static int WineXRenderFormatsListSize = 0;
static WineXRenderFormat *default_format = NULL;

typedef struct
{
    LOGFONTW lf;
    XFORM    xform;
    SIZE     devsize;  /* size in device coords */
    DWORD    hash;
} LFANDSIZE;

#define INITIAL_REALIZED_BUF_SIZE 128

typedef enum { AA_None = 0, AA_Grey, AA_RGB, AA_BGR, AA_VRGB, AA_VBGR, AA_MAXVALUE } AA_Type;

typedef struct
{
    GlyphSet glyphset;
    const WineXRenderFormat *font_format;
    int nrealized;
    BOOL *realized;
    void **bitmaps;
    XGlyphInfo *gis;
} gsCacheEntryFormat;

typedef struct
{
    LFANDSIZE lfsz;
    AA_Type aa_default;
    gsCacheEntryFormat * format[AA_MAXVALUE];
    INT count;
    INT next;
} gsCacheEntry;

struct xrender_info
{
    int                cache_index;
    Picture            pict;
    Picture            pict_src;
    const WineXRenderFormat *format;
};

static gsCacheEntry *glyphsetCache = NULL;
static DWORD glyphsetCacheSize = 0;
static INT lastfree = -1;
static INT mru = -1;

#define INIT_CACHE_SIZE 10

static int antialias = 1;

static void *xrender_handle;

#define MAKE_FUNCPTR(f) static typeof(f) * p##f;
MAKE_FUNCPTR(XRenderAddGlyphs)
MAKE_FUNCPTR(XRenderComposite)
MAKE_FUNCPTR(XRenderCompositeString8)
MAKE_FUNCPTR(XRenderCompositeString16)
MAKE_FUNCPTR(XRenderCompositeString32)
MAKE_FUNCPTR(XRenderCompositeText16)
MAKE_FUNCPTR(XRenderCreateGlyphSet)
MAKE_FUNCPTR(XRenderCreatePicture)
MAKE_FUNCPTR(XRenderFillRectangle)
MAKE_FUNCPTR(XRenderFindFormat)
MAKE_FUNCPTR(XRenderFindVisualFormat)
MAKE_FUNCPTR(XRenderFreeGlyphSet)
MAKE_FUNCPTR(XRenderFreePicture)
MAKE_FUNCPTR(XRenderSetPictureClipRectangles)
#ifdef HAVE_XRENDERSETPICTURETRANSFORM
MAKE_FUNCPTR(XRenderSetPictureTransform)
#endif
MAKE_FUNCPTR(XRenderQueryExtension)

#ifdef SONAME_LIBFONTCONFIG
#include <fontconfig/fontconfig.h>
MAKE_FUNCPTR(FcConfigSubstitute)
MAKE_FUNCPTR(FcDefaultSubstitute)
MAKE_FUNCPTR(FcFontMatch)
MAKE_FUNCPTR(FcInit)
MAKE_FUNCPTR(FcPatternCreate)
MAKE_FUNCPTR(FcPatternDestroy)
MAKE_FUNCPTR(FcPatternAddInteger)
MAKE_FUNCPTR(FcPatternAddString)
MAKE_FUNCPTR(FcPatternGetBool)
MAKE_FUNCPTR(FcPatternGetInteger)
MAKE_FUNCPTR(FcPatternGetString)
static void *fontconfig_handle;
static BOOL fontconfig_installed;
#endif

#undef MAKE_FUNCPTR

static CRITICAL_SECTION xrender_cs;
static CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &xrender_cs,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": xrender_cs") }
};
static CRITICAL_SECTION xrender_cs = { &critsect_debug, -1, 0, 0, 0, 0 };

#define MS_MAKE_TAG( _x1, _x2, _x3, _x4 ) \
          ( ( (ULONG)_x4 << 24 ) |     \
            ( (ULONG)_x3 << 16 ) |     \
            ( (ULONG)_x2 <<  8 ) |     \
              (ULONG)_x1         )

#define MS_GASP_TAG MS_MAKE_TAG('g', 'a', 's', 'p')

#define GASP_GRIDFIT 0x01
#define GASP_DOGRAY  0x02

#ifdef WORDS_BIGENDIAN
#define get_be_word(x) (x)
#define NATIVE_BYTE_ORDER MSBFirst
#else
#define get_be_word(x) RtlUshortByteSwap(x)
#define NATIVE_BYTE_ORDER LSBFirst
#endif

static WXRFormat get_format_without_alpha( WXRFormat format )
{
    switch (format)
    {
    case WXR_FORMAT_A8R8G8B8: return WXR_FORMAT_X8R8G8B8;
    case WXR_FORMAT_B8G8R8A8: return WXR_FORMAT_B8G8R8X8;
    default: return format;
    }
}

static BOOL get_xrender_template(const WineXRenderFormatTemplate *fmt, XRenderPictFormat *templ, unsigned long *mask)
{
    templ->id = 0;
    templ->type = PictTypeDirect;
    templ->depth = fmt->depth;
    templ->direct.alpha = fmt->alpha;
    templ->direct.alphaMask = fmt->alphaMask;
    templ->direct.red = fmt->red;
    templ->direct.redMask = fmt->redMask;
    templ->direct.green = fmt->green;
    templ->direct.greenMask = fmt->greenMask;
    templ->direct.blue = fmt->blue;
    templ->direct.blueMask = fmt->blueMask;
    templ->colormap = 0;

    *mask = PictFormatType | PictFormatDepth | PictFormatAlpha | PictFormatAlphaMask | PictFormatRed | PictFormatRedMask | PictFormatGreen | PictFormatGreenMask | PictFormatBlue | PictFormatBlueMask;

    return TRUE;
}

static BOOL is_wxrformat_compatible_with_default_visual(const WineXRenderFormatTemplate *fmt)
{
    if(fmt->depth != screen_depth)
        return FALSE;
    if( (fmt->redMask << fmt->red) != visual->red_mask)
        return FALSE;
    if( (fmt->greenMask << fmt->green) != visual->green_mask)
        return FALSE;
    if( (fmt->blueMask << fmt->blue) != visual->blue_mask)
        return FALSE;

    /* We never select a default ARGB visual */
    if(fmt->alphaMask)
        return FALSE;

    return TRUE;
}

static int load_xrender_formats(void)
{
    unsigned int i;
    for(i = 0; i < (sizeof(wxr_formats_template) / sizeof(wxr_formats_template[0])); i++)
    {
        XRenderPictFormat templ, *pict_format;

        if(is_wxrformat_compatible_with_default_visual(&wxr_formats_template[i]))
        {
            wine_tsx11_lock();
            pict_format = pXRenderFindVisualFormat(gdi_display, visual);
            if(!pict_format)
            {
                /* Xrender doesn't like DirectColor visuals, try to find a TrueColor one instead */
                if (visual->class == DirectColor)
                {
                    XVisualInfo info;
                    if (XMatchVisualInfo( gdi_display, DefaultScreen(gdi_display),
                                          screen_depth, TrueColor, &info ))
                    {
                        pict_format = pXRenderFindVisualFormat(gdi_display, info.visual);
                        if (pict_format) visual = info.visual;
                    }
                }
            }
            wine_tsx11_unlock();

            if(pict_format)
            {
                wxr_formats[WineXRenderFormatsListSize].format = wxr_formats_template[i].wxr_format;
                wxr_formats[WineXRenderFormatsListSize].pict_format = pict_format;
                default_format = &wxr_formats[WineXRenderFormatsListSize];
                WineXRenderFormatsListSize++;
                TRACE("Loaded pict_format with id=%#lx for wxr_format=%#x\n", pict_format->id, wxr_formats_template[i].wxr_format);
            }
        }
        else
        {
            unsigned long mask = 0;
            get_xrender_template(&wxr_formats_template[i], &templ, &mask);

            wine_tsx11_lock();
            pict_format = pXRenderFindFormat(gdi_display, mask, &templ, 0);
            wine_tsx11_unlock();

            if(pict_format)
            {
                wxr_formats[WineXRenderFormatsListSize].format = wxr_formats_template[i].wxr_format;
                wxr_formats[WineXRenderFormatsListSize].pict_format = pict_format;
                WineXRenderFormatsListSize++;
                TRACE("Loaded pict_format with id=%#lx for wxr_format=%#x\n", pict_format->id, wxr_formats_template[i].wxr_format);
            }
        }
    }
    return WineXRenderFormatsListSize;
}

/***********************************************************************
 *   X11DRV_XRender_Init
 *
 * Let's see if our XServer has the extension available
 *
 */
void X11DRV_XRender_Init(void)
{
    int event_base, i;

    if (client_side_with_render &&
	wine_dlopen(SONAME_LIBX11, RTLD_NOW|RTLD_GLOBAL, NULL, 0) &&
	wine_dlopen(SONAME_LIBXEXT, RTLD_NOW|RTLD_GLOBAL, NULL, 0) && 
	(xrender_handle = wine_dlopen(SONAME_LIBXRENDER, RTLD_NOW, NULL, 0)))
    {

#define LOAD_FUNCPTR(f) if((p##f = wine_dlsym(xrender_handle, #f, NULL, 0)) == NULL) goto sym_not_found;
LOAD_FUNCPTR(XRenderAddGlyphs)
LOAD_FUNCPTR(XRenderComposite)
LOAD_FUNCPTR(XRenderCompositeString8)
LOAD_FUNCPTR(XRenderCompositeString16)
LOAD_FUNCPTR(XRenderCompositeString32)
LOAD_FUNCPTR(XRenderCompositeText16)
LOAD_FUNCPTR(XRenderCreateGlyphSet)
LOAD_FUNCPTR(XRenderCreatePicture)
LOAD_FUNCPTR(XRenderFillRectangle)
LOAD_FUNCPTR(XRenderFindFormat)
LOAD_FUNCPTR(XRenderFindVisualFormat)
LOAD_FUNCPTR(XRenderFreeGlyphSet)
LOAD_FUNCPTR(XRenderFreePicture)
LOAD_FUNCPTR(XRenderSetPictureClipRectangles)
LOAD_FUNCPTR(XRenderQueryExtension)
#undef LOAD_FUNCPTR
#ifdef HAVE_XRENDERSETPICTURETRANSFORM
#define LOAD_OPTIONAL_FUNCPTR(f) p##f = wine_dlsym(xrender_handle, #f, NULL, 0);
LOAD_OPTIONAL_FUNCPTR(XRenderSetPictureTransform)
#undef LOAD_OPTIONAL_FUNCPTR
#endif

        wine_tsx11_lock();
        X11DRV_XRender_Installed = pXRenderQueryExtension(gdi_display, &event_base, &xrender_error_base);
        wine_tsx11_unlock();
        if(X11DRV_XRender_Installed) {
            TRACE("Xrender is up and running error_base = %d\n", xrender_error_base);
            if(!load_xrender_formats()) /* This fails in buggy versions of libXrender.so */
            {
                wine_tsx11_unlock();
                WINE_MESSAGE(
                    "Wine has detected that you probably have a buggy version\n"
                    "of libXrender.so .  Because of this client side font rendering\n"
                    "will be disabled.  Please upgrade this library.\n");
                X11DRV_XRender_Installed = FALSE;
                return;
            }

            if (!visual->red_mask || !visual->green_mask || !visual->blue_mask) {
                WARN("one or more of the colour masks are 0, disabling XRENDER. Try running in 16-bit mode or higher.\n");
                X11DRV_XRender_Installed = FALSE;
            }
        }
    }

#ifdef SONAME_LIBFONTCONFIG
    if ((fontconfig_handle = wine_dlopen(SONAME_LIBFONTCONFIG, RTLD_NOW, NULL, 0)))
    {
#define LOAD_FUNCPTR(f) if((p##f = wine_dlsym(fontconfig_handle, #f, NULL, 0)) == NULL){WARN("Can't find symbol %s\n", #f); goto sym_not_found;}
        LOAD_FUNCPTR(FcConfigSubstitute);
        LOAD_FUNCPTR(FcDefaultSubstitute);
        LOAD_FUNCPTR(FcFontMatch);
        LOAD_FUNCPTR(FcInit);
        LOAD_FUNCPTR(FcPatternCreate);
        LOAD_FUNCPTR(FcPatternDestroy);
        LOAD_FUNCPTR(FcPatternAddInteger);
        LOAD_FUNCPTR(FcPatternAddString);
        LOAD_FUNCPTR(FcPatternGetBool);
        LOAD_FUNCPTR(FcPatternGetInteger);
        LOAD_FUNCPTR(FcPatternGetString);
#undef LOAD_FUNCPTR
        fontconfig_installed = pFcInit();
    }
    else TRACE( "cannot find the fontconfig library " SONAME_LIBFONTCONFIG "\n" );
#endif

sym_not_found:
    if(X11DRV_XRender_Installed || client_side_with_core)
    {
	glyphsetCache = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
				  sizeof(*glyphsetCache) * INIT_CACHE_SIZE);

	glyphsetCacheSize = INIT_CACHE_SIZE;
	lastfree = 0;
	for(i = 0; i < INIT_CACHE_SIZE; i++) {
	  glyphsetCache[i].next = i + 1;
	  glyphsetCache[i].count = -1;
	}
	glyphsetCache[i-1].next = -1;
	using_client_side_fonts = 1;

	if(!X11DRV_XRender_Installed) {
	    TRACE("Xrender is not available on your XServer, client side rendering with the core protocol instead.\n");
	    if(screen_depth <= 8 || !client_side_antialias_with_core)
	        antialias = 0;
	} else {
	    if(screen_depth <= 8 || !client_side_antialias_with_render)
	        antialias = 0;
	}
    }
    else TRACE("Using X11 core fonts\n");
}

/* Helper function to convert from a color packed in a 32-bit integer to a XRenderColor */
static void get_xrender_color(const WineXRenderFormat *wxr_format, int src_color, XRenderColor *dst_color)
{
    XRenderPictFormat *pf = wxr_format->pict_format;

    if(pf->direct.redMask)
        dst_color->red = ((src_color >> pf->direct.red) & pf->direct.redMask) * 65535/pf->direct.redMask;
    else
       dst_color->red = 0;

    if(pf->direct.greenMask)
        dst_color->green = ((src_color >> pf->direct.green) & pf->direct.greenMask) * 65535/pf->direct.greenMask;
    else
        dst_color->green = 0;

    if(pf->direct.blueMask)
        dst_color->blue = ((src_color >> pf->direct.blue) & pf->direct.blueMask) * 65535/pf->direct.blueMask;
    else
        dst_color->blue = 0;

    dst_color->alpha = 0xffff;
}

static const WineXRenderFormat *get_xrender_format(WXRFormat format)
{
    int i;
    for(i=0; i<WineXRenderFormatsListSize; i++)
    {
        if(wxr_formats[i].format == format)
        {
            TRACE("Returning wxr_format=%#x\n", format);
            return &wxr_formats[i];
        }
    }
    return NULL;
}

static const WineXRenderFormat *get_xrender_format_from_color_shifts(int depth, ColorShifts *shifts)
{
    int redMask, greenMask, blueMask;
    unsigned int i;

    if(depth == 1)
        return get_xrender_format(WXR_FORMAT_MONO);

    /* physDevs of a depth <=8, don't have color_shifts set and XRender can't handle those except for 1-bit */
    if(!shifts)
        return default_format;

    redMask   = shifts->physicalRed.max << shifts->physicalRed.shift;
    greenMask = shifts->physicalGreen.max << shifts->physicalGreen.shift;
    blueMask  = shifts->physicalBlue.max << shifts->physicalBlue.shift;

    /* Try to locate a format which matches the specification of the dibsection. */
    for(i = 0; i < (sizeof(wxr_formats_template) / sizeof(wxr_formats_template[0])); i++)
    {
        if( depth     == wxr_formats_template[i].depth &&
            redMask   == (wxr_formats_template[i].redMask << wxr_formats_template[i].red) &&
            greenMask == (wxr_formats_template[i].greenMask << wxr_formats_template[i].green) &&
            blueMask  == (wxr_formats_template[i].blueMask << wxr_formats_template[i].blue) )

        {
            /* When we reach this stage the format was found in our template table but this doesn't mean that
            * the Xserver also supports this format (e.g. its depth might be too low). The call below verifies that.
            */
            return get_xrender_format(wxr_formats_template[i].wxr_format);
        }
    }

    /* This should not happen because when we reach 'shifts' must have been set and we only allows shifts which are backed by X */
    ERR("No XRender format found!\n");
    return NULL;
}

/* Set the x/y scaling and x/y offsets in the transformation matrix of the source picture */
static void set_xrender_transformation(Picture src_pict, double xscale, double yscale, int xoffset, int yoffset)
{
#ifdef HAVE_XRENDERSETPICTURETRANSFORM
    XTransform xform = {{
        { XDoubleToFixed(xscale), XDoubleToFixed(0), XDoubleToFixed(xoffset) },
        { XDoubleToFixed(0), XDoubleToFixed(yscale), XDoubleToFixed(yoffset) },
        { XDoubleToFixed(0), XDoubleToFixed(0), XDoubleToFixed(1) }
    }};

    pXRenderSetPictureTransform(gdi_display, src_pict, &xform);
#endif
}

/* check if we can use repeating instead of scaling for the specified source DC */
static BOOL use_source_repeat( X11DRV_PDEVICE *physDev )
{
    return (physDev->bitmap &&
            physDev->drawable_rect.right - physDev->drawable_rect.left == 1 &&
            physDev->drawable_rect.bottom - physDev->drawable_rect.top == 1);
}

static struct xrender_info *get_xrender_info(X11DRV_PDEVICE *physDev)
{
    if(!physDev->xrender)
    {
        physDev->xrender = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*physDev->xrender));

        if(!physDev->xrender)
        {
            ERR("Unable to allocate XRENDERINFO!\n");
            return NULL;
        }
        physDev->xrender->cache_index = -1;
    }
    if (!physDev->xrender->format)
        physDev->xrender->format = get_xrender_format_from_color_shifts(physDev->depth, physDev->color_shifts);

    return physDev->xrender;
}

static Picture get_xrender_picture(X11DRV_PDEVICE *physDev)
{
    struct xrender_info *info = get_xrender_info(physDev);
    if (!info) return 0;

    if (!info->pict && info->format)
    {
        XRenderPictureAttributes pa;
        RGNDATA *clip = X11DRV_GetRegionData( physDev->region, 0 );

        wine_tsx11_lock();
        pa.subwindow_mode = IncludeInferiors;
        info->pict = pXRenderCreatePicture(gdi_display, physDev->drawable, info->format->pict_format,
                                           CPSubwindowMode, &pa);
        if (info->pict && clip)
            pXRenderSetPictureClipRectangles( gdi_display, info->pict,
                                              physDev->dc_rect.left, physDev->dc_rect.top,
                                              (XRectangle *)clip->Buffer, clip->rdh.nCount );
        wine_tsx11_unlock();
        TRACE("Allocing pict=%lx dc=%p drawable=%08lx\n", info->pict, physDev->hdc, physDev->drawable);
        HeapFree( GetProcessHeap(), 0, clip );
    }

    return info->pict;
}

static Picture get_xrender_picture_source(X11DRV_PDEVICE *physDev, BOOL repeat)
{
    struct xrender_info *info = get_xrender_info(physDev);
    if (!info) return 0;

    if (!info->pict_src && info->format)
    {
        XRenderPictureAttributes pa;

        wine_tsx11_lock();
        pa.subwindow_mode = IncludeInferiors;
        pa.repeat = repeat ? RepeatNormal : RepeatNone;
        info->pict_src = pXRenderCreatePicture(gdi_display, physDev->drawable, info->format->pict_format,
                                               CPSubwindowMode|CPRepeat, &pa);
        wine_tsx11_unlock();

        TRACE("Allocing pict_src=%lx dc=%p drawable=%08lx repeat=%u\n",
              info->pict_src, physDev->hdc, physDev->drawable, pa.repeat);
    }

    return info->pict_src;
}

/* return a mask picture used to force alpha to 0 */
static Picture get_no_alpha_mask(void)
{
    static Pixmap pixmap;
    static Picture pict;

    wine_tsx11_lock();
    if (!pict)
    {
        const WineXRenderFormat *fmt = get_xrender_format( WXR_FORMAT_A8R8G8B8 );
        XRenderPictureAttributes pa;
        XRenderColor col;

        pixmap = XCreatePixmap( gdi_display, root_window, 1, 1, 32 );
        pa.repeat = RepeatNormal;
        pa.component_alpha = True;
        pict = pXRenderCreatePicture( gdi_display, pixmap, fmt->pict_format,
                                      CPRepeat|CPComponentAlpha, &pa );
        col.red = col.green = col.blue = 0xffff;
        col.alpha = 0;
        pXRenderFillRectangle( gdi_display, PictOpSrc, pict, &col, 0, 0, 1, 1 );
    }
    wine_tsx11_unlock();
    return pict;
}

static BOOL fontcmp(LFANDSIZE *p1, LFANDSIZE *p2)
{
  if(p1->hash != p2->hash) return TRUE;
  if(memcmp(&p1->devsize, &p2->devsize, sizeof(p1->devsize))) return TRUE;
  if(memcmp(&p1->xform, &p2->xform, sizeof(p1->xform))) return TRUE;
  if(memcmp(&p1->lf, &p2->lf, offsetof(LOGFONTW, lfFaceName))) return TRUE;
  return strcmpiW(p1->lf.lfFaceName, p2->lf.lfFaceName);
}

#if 0
static void walk_cache(void)
{
  int i;

  EnterCriticalSection(&xrender_cs);
  for(i=mru; i >= 0; i = glyphsetCache[i].next)
    TRACE("item %d\n", i);
  LeaveCriticalSection(&xrender_cs);
}
#endif

static int LookupEntry(LFANDSIZE *plfsz)
{
  int i, prev_i = -1;

  for(i = mru; i >= 0; i = glyphsetCache[i].next) {
    TRACE("%d\n", i);
    if(glyphsetCache[i].count == -1) { /* reached free list so stop */
      i = -1;
      break;
    }

    if(!fontcmp(&glyphsetCache[i].lfsz, plfsz)) {
      glyphsetCache[i].count++;
      if(prev_i >= 0) {
	glyphsetCache[prev_i].next = glyphsetCache[i].next;
	glyphsetCache[i].next = mru;
	mru = i;
      }
      TRACE("found font in cache %d\n", i);
      return i;
    }
    prev_i = i;
  }
  TRACE("font not in cache\n");
  return -1;
}

static void FreeEntry(int entry)
{
    int i, format;
  
    for(format = 0; format < AA_MAXVALUE; format++) {
        gsCacheEntryFormat * formatEntry;

        if( !glyphsetCache[entry].format[format] )
            continue;

        formatEntry = glyphsetCache[entry].format[format];

        if(formatEntry->glyphset) {
            wine_tsx11_lock();
            pXRenderFreeGlyphSet(gdi_display, formatEntry->glyphset);
            wine_tsx11_unlock();
            formatEntry->glyphset = 0;
        }
        if(formatEntry->nrealized) {
            HeapFree(GetProcessHeap(), 0, formatEntry->realized);
            formatEntry->realized = NULL;
            if(formatEntry->bitmaps) {
                for(i = 0; i < formatEntry->nrealized; i++)
                    HeapFree(GetProcessHeap(), 0, formatEntry->bitmaps[i]);
                HeapFree(GetProcessHeap(), 0, formatEntry->bitmaps);
                formatEntry->bitmaps = NULL;
            }
            HeapFree(GetProcessHeap(), 0, formatEntry->gis);
            formatEntry->gis = NULL;
            formatEntry->nrealized = 0;
        }

        HeapFree(GetProcessHeap(), 0, formatEntry);
        glyphsetCache[entry].format[format] = NULL;
    }
}

static int AllocEntry(void)
{
  int best = -1, prev_best = -1, i, prev_i = -1;

  if(lastfree >= 0) {
    assert(glyphsetCache[lastfree].count == -1);
    glyphsetCache[lastfree].count = 1;
    best = lastfree;
    lastfree = glyphsetCache[lastfree].next;
    assert(best != mru);
    glyphsetCache[best].next = mru;
    mru = best;

    TRACE("empty space at %d, next lastfree = %d\n", mru, lastfree);
    return mru;
  }

  for(i = mru; i >= 0; i = glyphsetCache[i].next) {
    if(glyphsetCache[i].count == 0) {
      best = i;
      prev_best = prev_i;
    }
    prev_i = i;
  }

  if(best >= 0) {
    TRACE("freeing unused glyphset at cache %d\n", best);
    FreeEntry(best);
    glyphsetCache[best].count = 1;
    if(prev_best >= 0) {
      glyphsetCache[prev_best].next = glyphsetCache[best].next;
      glyphsetCache[best].next = mru;
      mru = best;
    } else {
      assert(mru == best);
    }
    return mru;
  }

  TRACE("Growing cache\n");
  
  if (glyphsetCache)
    glyphsetCache = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
			      glyphsetCache,
			      (glyphsetCacheSize + INIT_CACHE_SIZE)
			      * sizeof(*glyphsetCache));
  else
    glyphsetCache = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
			      (glyphsetCacheSize + INIT_CACHE_SIZE)
			      * sizeof(*glyphsetCache));

  for(best = i = glyphsetCacheSize; i < glyphsetCacheSize + INIT_CACHE_SIZE;
      i++) {
    glyphsetCache[i].next = i + 1;
    glyphsetCache[i].count = -1;
  }
  glyphsetCache[i-1].next = -1;
  glyphsetCacheSize += INIT_CACHE_SIZE;

  lastfree = glyphsetCache[best].next;
  glyphsetCache[best].count = 1;
  glyphsetCache[best].next = mru;
  mru = best;
  TRACE("new free cache slot at %d\n", mru);
  return mru;
}

static BOOL get_gasp_flags(X11DRV_PDEVICE *physDev, WORD *flags)
{
    DWORD size;
    WORD *gasp, *buffer;
    WORD num_recs;
    DWORD ppem;
    TEXTMETRICW tm;

    *flags = 0;

    size = GetFontData(physDev->hdc, MS_GASP_TAG,  0, NULL, 0);
    if(size == GDI_ERROR)
        return FALSE;

    gasp = buffer = HeapAlloc(GetProcessHeap(), 0, size);
    GetFontData(physDev->hdc, MS_GASP_TAG,  0, gasp, size);

    GetTextMetricsW(physDev->hdc, &tm);
    ppem = abs(X11DRV_YWStoDS(physDev, tm.tmAscent + tm.tmDescent - tm.tmInternalLeading));

    gasp++;
    num_recs = get_be_word(*gasp);
    gasp++;
    while(num_recs--)
    {
        *flags = get_be_word(*(gasp + 1));
        if(ppem <= get_be_word(*gasp))
            break;
        gasp += 2;
    }
    TRACE("got flags %04x for ppem %d\n", *flags, ppem);

    HeapFree(GetProcessHeap(), 0, buffer);
    return TRUE;
}

static AA_Type get_antialias_type( X11DRV_PDEVICE *physDev, BOOL subpixel, BOOL hinter)
{
    AA_Type ret;
    WORD flags;
    UINT font_smoothing_type, font_smoothing_orientation;

    if (X11DRV_XRender_Installed && subpixel &&
        SystemParametersInfoW( SPI_GETFONTSMOOTHINGTYPE, 0, &font_smoothing_type, 0) &&
        font_smoothing_type == FE_FONTSMOOTHINGCLEARTYPE)
    {
        if ( SystemParametersInfoW( SPI_GETFONTSMOOTHINGORIENTATION, 0,
                                    &font_smoothing_orientation, 0) &&
             font_smoothing_orientation == FE_FONTSMOOTHINGORIENTATIONBGR)
        {
            ret = AA_BGR;
        }
        else
            ret = AA_RGB;
        /*FIXME
          If the monitor is in portrait mode, ClearType is disabled in the MS Windows (MSDN).
          But, Wine's subpixel rendering can support the portrait mode.
         */
    }
    else if (!hinter || !get_gasp_flags(physDev, &flags) || flags & GASP_DOGRAY)
        ret = AA_Grey;
    else
        ret = AA_None;

    return ret;
}

static int GetCacheEntry(X11DRV_PDEVICE *physDev, LFANDSIZE *plfsz)
{
    int ret;
    int format;
    gsCacheEntry *entry;
    static int hinter = -1;
    static int subpixel = -1;
    BOOL font_smoothing;

    if((ret = LookupEntry(plfsz)) != -1) return ret;

    ret = AllocEntry();
    entry = glyphsetCache + ret;
    entry->lfsz = *plfsz;
    for( format = 0; format < AA_MAXVALUE; format++ ) {
        assert( !entry->format[format] );
    }

    if(antialias && plfsz->lf.lfQuality != NONANTIALIASED_QUALITY)
    {
        if(hinter == -1 || subpixel == -1)
        {
            RASTERIZER_STATUS status;
            GetRasterizerCaps(&status, sizeof(status));
            hinter = status.wFlags & WINE_TT_HINTER_ENABLED;
            subpixel = status.wFlags & WINE_TT_SUBPIXEL_RENDERING_ENABLED;
        }

        switch (plfsz->lf.lfQuality)
        {
            case ANTIALIASED_QUALITY:
                entry->aa_default = get_antialias_type( physDev, FALSE, hinter );
                return ret;  /* ignore further configuration */
            case CLEARTYPE_QUALITY:
            case CLEARTYPE_NATURAL_QUALITY:
                entry->aa_default = get_antialias_type( physDev, subpixel, hinter );
                break;
            case DEFAULT_QUALITY:
            case DRAFT_QUALITY:
            case PROOF_QUALITY:
            default:
                if ( SystemParametersInfoW( SPI_GETFONTSMOOTHING, 0, &font_smoothing, 0) &&
                     font_smoothing)
                {
                    entry->aa_default = get_antialias_type( physDev, subpixel, hinter );
                }
                else
                    entry->aa_default = AA_None;
                break;
        }

        font_smoothing = TRUE;  /* default to enabled */
#ifdef SONAME_LIBFONTCONFIG
        if (fontconfig_installed)
        {
            FcPattern *match, *pattern = pFcPatternCreate();
            FcResult result;
            char family[LF_FACESIZE * 4];

            WideCharToMultiByte( CP_UTF8, 0, plfsz->lf.lfFaceName, -1, family, sizeof(family), NULL, NULL );
            pFcPatternAddString( pattern, FC_FAMILY, (FcChar8 *)family );
            if (plfsz->lf.lfWeight != FW_DONTCARE)
            {
                int weight;
                switch (plfsz->lf.lfWeight)
                {
                case FW_THIN:       weight = FC_WEIGHT_THIN; break;
                case FW_EXTRALIGHT: weight = FC_WEIGHT_EXTRALIGHT; break;
                case FW_LIGHT:      weight = FC_WEIGHT_LIGHT; break;
                case FW_NORMAL:     weight = FC_WEIGHT_NORMAL; break;
                case FW_MEDIUM:     weight = FC_WEIGHT_MEDIUM; break;
                case FW_SEMIBOLD:   weight = FC_WEIGHT_SEMIBOLD; break;
                case FW_BOLD:       weight = FC_WEIGHT_BOLD; break;
                case FW_EXTRABOLD:  weight = FC_WEIGHT_EXTRABOLD; break;
                case FW_HEAVY:      weight = FC_WEIGHT_HEAVY; break;
                default:            weight = (plfsz->lf.lfWeight - 80) / 4; break;
                }
                pFcPatternAddInteger( pattern, FC_WEIGHT, weight );
            }
            pFcPatternAddInteger( pattern, FC_SLANT, plfsz->lf.lfItalic ? FC_SLANT_ITALIC : FC_SLANT_ROMAN );
            pFcConfigSubstitute( NULL, pattern, FcMatchPattern );
            pFcDefaultSubstitute( pattern );
            if ((match = pFcFontMatch( NULL, pattern, &result )))
            {
                int rgba;
                FcBool antialias;

                if (pFcPatternGetBool( match, FC_ANTIALIAS, 0, &antialias ) != FcResultMatch)
                    antialias = TRUE;
                if (pFcPatternGetInteger( match, FC_RGBA, 0, &rgba ) == FcResultMatch)
                {
                    FcChar8 *file;
                    if (pFcPatternGetString( match, FC_FILE, 0, &file ) != FcResultMatch) file = NULL;

                    TRACE( "fontconfig returned rgba %u antialias %u for font %s file %s\n",
                           rgba, antialias, debugstr_w(plfsz->lf.lfFaceName), debugstr_a((char *)file) );

                    switch (rgba)
                    {
                    case FC_RGBA_RGB:  entry->aa_default = AA_RGB; break;
                    case FC_RGBA_BGR:  entry->aa_default = AA_BGR; break;
                    case FC_RGBA_VRGB: entry->aa_default = AA_VRGB; break;
                    case FC_RGBA_VBGR: entry->aa_default = AA_VBGR; break;
                    case FC_RGBA_NONE: entry->aa_default = AA_Grey; break;
                    }
                }
                if (!antialias) font_smoothing = FALSE;
                pFcPatternDestroy( match );
            }
            pFcPatternDestroy( pattern );
        }
#endif  /* SONAME_LIBFONTCONFIG */

        /* now check Xft resources */
        {
            char *value;
            BOOL antialias = TRUE;

            wine_tsx11_lock();
            if ((value = XGetDefault( gdi_display, "Xft", "antialias" )))
            {
                if (tolower(value[0]) == 'f' || tolower(value[0]) == 'n' ||
                    value[0] == '0' || !strcasecmp( value, "off" ))
                    antialias = FALSE;
            }
            if ((value = XGetDefault( gdi_display, "Xft", "rgba" )))
            {
                TRACE( "Xft resource returned rgba '%s' antialias %u\n", value, antialias );
                if (!strcmp( value, "rgb" )) entry->aa_default = AA_RGB;
                else if (!strcmp( value, "bgr" )) entry->aa_default = AA_BGR;
                else if (!strcmp( value, "vrgb" )) entry->aa_default = AA_VRGB;
                else if (!strcmp( value, "vbgr" )) entry->aa_default = AA_VBGR;
                else if (!strcmp( value, "none" )) entry->aa_default = AA_Grey;
            }
            wine_tsx11_unlock();
            if (!antialias) font_smoothing = FALSE;
        }

        if (!font_smoothing) entry->aa_default = AA_None;

        /* we can't support subpixel without xrender */
        if (!X11DRV_XRender_Installed && entry->aa_default > AA_Grey) entry->aa_default = AA_Grey;
    }
    else
        entry->aa_default = AA_None;

    return ret;
}

static void dec_ref_cache(int index)
{
    assert(index >= 0);
    TRACE("dec'ing entry %d to %d\n", index, glyphsetCache[index].count - 1);
    assert(glyphsetCache[index].count > 0);
    glyphsetCache[index].count--;
}

static void lfsz_calc_hash(LFANDSIZE *plfsz)
{
  DWORD hash = 0, *ptr, two_chars;
  WORD *pwc;
  int i;

  hash ^= plfsz->devsize.cx;
  hash ^= plfsz->devsize.cy;
  for(i = 0, ptr = (DWORD*)&plfsz->xform; i < sizeof(XFORM)/sizeof(DWORD); i++, ptr++)
    hash ^= *ptr;
  for(i = 0, ptr = (DWORD*)&plfsz->lf; i < 7; i++, ptr++)
    hash ^= *ptr;
  for(i = 0, ptr = (DWORD*)plfsz->lf.lfFaceName; i < LF_FACESIZE/2; i++, ptr++) {
    two_chars = *ptr;
    pwc = (WCHAR *)&two_chars;
    if(!*pwc) break;
    *pwc = toupperW(*pwc);
    pwc++;
    *pwc = toupperW(*pwc);
    hash ^= two_chars;
    if(!*pwc) break;
  }
  plfsz->hash = hash;
  return;
}

/***********************************************************************
 *   X11DRV_XRender_Finalize
 */
void X11DRV_XRender_Finalize(void)
{
    int i;

    EnterCriticalSection(&xrender_cs);
    for(i = mru; i >= 0; i = glyphsetCache[i].next)
	FreeEntry(i);
    LeaveCriticalSection(&xrender_cs);
}


/***********************************************************************
 *   X11DRV_XRender_SelectFont
 */
BOOL X11DRV_XRender_SelectFont(X11DRV_PDEVICE *physDev, HFONT hfont)
{
    LFANDSIZE lfsz;
    struct xrender_info *info;

    GetObjectW(hfont, sizeof(lfsz.lf), &lfsz.lf);
    TRACE("h=%d w=%d weight=%d it=%d charset=%d name=%s\n",
	  lfsz.lf.lfHeight, lfsz.lf.lfWidth, lfsz.lf.lfWeight,
	  lfsz.lf.lfItalic, lfsz.lf.lfCharSet, debugstr_w(lfsz.lf.lfFaceName));
    lfsz.lf.lfWidth = abs( lfsz.lf.lfWidth );
    lfsz.devsize.cx = X11DRV_XWStoDS( physDev, lfsz.lf.lfWidth );
    lfsz.devsize.cy = X11DRV_YWStoDS( physDev, lfsz.lf.lfHeight );
    GetWorldTransform( physDev->hdc, &lfsz.xform );
    lfsz_calc_hash(&lfsz);

    info = get_xrender_info(physDev);
    if (!info) return 0;

    EnterCriticalSection(&xrender_cs);
    if(info->cache_index != -1)
        dec_ref_cache(info->cache_index);
    info->cache_index = GetCacheEntry(physDev, &lfsz);
    LeaveCriticalSection(&xrender_cs);
    return 0;
}

/***********************************************************************
*   X11DRV_XRender_SetDeviceClipping
*/
void X11DRV_XRender_SetDeviceClipping(X11DRV_PDEVICE *physDev, const RGNDATA *data)
{
    if (physDev->xrender->pict)
    {
        wine_tsx11_lock();
        pXRenderSetPictureClipRectangles( gdi_display, physDev->xrender->pict,
                                          physDev->dc_rect.left, physDev->dc_rect.top,
                                          (XRectangle *)data->Buffer, data->rdh.nCount );
        wine_tsx11_unlock();
    }
}

/***********************************************************************
 *   X11DRV_XRender_DeleteDC
 */
void X11DRV_XRender_DeleteDC(X11DRV_PDEVICE *physDev)
{
    X11DRV_XRender_UpdateDrawable(physDev);

    EnterCriticalSection(&xrender_cs);
    if(physDev->xrender->cache_index != -1)
        dec_ref_cache(physDev->xrender->cache_index);
    LeaveCriticalSection(&xrender_cs);

    HeapFree(GetProcessHeap(), 0, physDev->xrender);
    physDev->xrender = NULL;
    return;
}

BOOL X11DRV_XRender_SetPhysBitmapDepth(X_PHYSBITMAP *physBitmap, int bits_pixel, const DIBSECTION *dib)
{
    const WineXRenderFormat *fmt;
    ColorShifts shifts;

    /* When XRender is not around we can only use the screen_depth and when needed we perform depth conversion
     * in software. Further we also return the screen depth for paletted formats or TrueColor formats with a low
     * number of bits because XRender can't handle paletted formats and 8-bit TrueColor does not exist for XRender. */
    if (!X11DRV_XRender_Installed || bits_pixel <= 8)
        return FALSE;

    if (dib)
    {
        X11DRV_PALETTE_ComputeColorShifts(&shifts, dib->dsBitfields[0], dib->dsBitfields[1], dib->dsBitfields[2]);
        fmt = get_xrender_format_from_color_shifts(dib->dsBm.bmBitsPixel, &shifts);

        /* Common formats should be in our picture format table. */
        if (!fmt)
        {
            TRACE("Unhandled dibsection format bpp=%d, redMask=%x, greenMask=%x, blueMask=%x\n",
                dib->dsBm.bmBitsPixel, dib->dsBitfields[0], dib->dsBitfields[1], dib->dsBitfields[2]);
            return FALSE;
        }
    }
    else
    {
        int red_mask, green_mask, blue_mask;

        /* We are dealing with a DDB */
        switch (bits_pixel)
        {
            case 16:
                fmt = get_xrender_format(WXR_FORMAT_R5G6B5);
                break;
            case 24:
                fmt = get_xrender_format(WXR_FORMAT_R8G8B8);
                break;
            case 32:
                fmt = get_xrender_format(WXR_FORMAT_A8R8G8B8);
                break;
            default:
                fmt = NULL;
        }

        if (!fmt)
        {
            TRACE("Unhandled DDB bits_pixel=%d\n", bits_pixel);
            return FALSE;
        }

        red_mask = fmt->pict_format->direct.redMask << fmt->pict_format->direct.red;
        green_mask = fmt->pict_format->direct.greenMask << fmt->pict_format->direct.green;
        blue_mask = fmt->pict_format->direct.blueMask << fmt->pict_format->direct.blue;
        X11DRV_PALETTE_ComputeColorShifts(&shifts, red_mask, green_mask, blue_mask);
    }

    physBitmap->pixmap_depth = fmt->pict_format->depth;
    physBitmap->trueColor = TRUE;
    physBitmap->pixmap_color_shifts = shifts;
    return TRUE;
}

/***********************************************************************
 *   X11DRV_XRender_UpdateDrawable
 *
 * Deletes the pict and tile when the drawable changes.
 */
void X11DRV_XRender_UpdateDrawable(X11DRV_PDEVICE *physDev)
{
    struct xrender_info *info = physDev->xrender;

    if (info->pict || info->pict_src)
    {
        wine_tsx11_lock();
        XFlush( gdi_display );
        if (info->pict)
        {
            TRACE("freeing pict = %lx dc = %p\n", info->pict, physDev->hdc);
            pXRenderFreePicture(gdi_display, info->pict);
            info->pict = 0;
        }
        if(info->pict_src)
        {
            TRACE("freeing pict = %lx dc = %p\n", info->pict_src, physDev->hdc);
            pXRenderFreePicture(gdi_display, info->pict_src);
            info->pict_src = 0;
        }
        wine_tsx11_unlock();
    }

    info->format = NULL;
}

/************************************************************************
 *   UploadGlyph
 *
 * Helper to ExtTextOut.  Must be called inside xrender_cs
 */
static void UploadGlyph(X11DRV_PDEVICE *physDev, int glyph, AA_Type format)
{
    unsigned int buflen;
    char *buf;
    Glyph gid;
    GLYPHMETRICS gm;
    XGlyphInfo gi;
    gsCacheEntry *entry = glyphsetCache + physDev->xrender->cache_index;
    gsCacheEntryFormat *formatEntry;
    UINT ggo_format = GGO_GLYPH_INDEX;
    WXRFormat wxr_format;
    static const char zero[4];
    static const MAT2 identity = { {0,1},{0,0},{0,0},{0,1} };

    switch(format) {
    case AA_Grey:
	ggo_format |= WINE_GGO_GRAY16_BITMAP;
	break;
    case AA_RGB:
	ggo_format |= WINE_GGO_HRGB_BITMAP;
	break;
    case AA_BGR:
	ggo_format |= WINE_GGO_HBGR_BITMAP;
	break;
    case AA_VRGB:
	ggo_format |= WINE_GGO_VRGB_BITMAP;
	break;
    case AA_VBGR:
	ggo_format |= WINE_GGO_VBGR_BITMAP;
	break;

    default:
        ERR("aa = %d - not implemented\n", format);
    case AA_None:
        ggo_format |= GGO_BITMAP;
	break;
    }

    buflen = GetGlyphOutlineW(physDev->hdc, glyph, ggo_format, &gm, 0, NULL, &identity);
    if(buflen == GDI_ERROR) {
        if(format != AA_None) {
            format = AA_None;
            entry->aa_default = AA_None;
            ggo_format = GGO_GLYPH_INDEX | GGO_BITMAP;
            buflen = GetGlyphOutlineW(physDev->hdc, glyph, ggo_format, &gm, 0, NULL, &identity);
        }
        if(buflen == GDI_ERROR) {
            WARN("GetGlyphOutlineW failed using default glyph\n");
            buflen = GetGlyphOutlineW(physDev->hdc, 0, ggo_format, &gm, 0, NULL, &identity);
            if(buflen == GDI_ERROR) {
                WARN("GetGlyphOutlineW failed for default glyph trying for space\n");
                buflen = GetGlyphOutlineW(physDev->hdc, 0x20, ggo_format, &gm, 0, NULL, &identity);
                if(buflen == GDI_ERROR) {
                    ERR("GetGlyphOutlineW for all attempts unable to upload a glyph\n");
                    return;
                }
            }
        }
        TRACE("Turning off antialiasing for this monochrome font\n");
    }

    /* If there is nothing for the current type, we create the entry. */
    if( !entry->format[format] ) {
        entry->format[format] = HeapAlloc(GetProcessHeap(),
                                          HEAP_ZERO_MEMORY,
                                          sizeof(gsCacheEntryFormat));
    }
    formatEntry = entry->format[format];

    if(formatEntry->nrealized <= glyph) {
        formatEntry->nrealized = (glyph / 128 + 1) * 128;

	if (formatEntry->realized)
	    formatEntry->realized = HeapReAlloc(GetProcessHeap(),
				      HEAP_ZERO_MEMORY,
				      formatEntry->realized,
				      formatEntry->nrealized * sizeof(BOOL));
	else
	    formatEntry->realized = HeapAlloc(GetProcessHeap(),
				      HEAP_ZERO_MEMORY,
				      formatEntry->nrealized * sizeof(BOOL));

	if(!X11DRV_XRender_Installed) {
	  if (formatEntry->bitmaps)
	    formatEntry->bitmaps = HeapReAlloc(GetProcessHeap(),
				      HEAP_ZERO_MEMORY,
				      formatEntry->bitmaps,
				      formatEntry->nrealized * sizeof(formatEntry->bitmaps[0]));
	  else
	    formatEntry->bitmaps = HeapAlloc(GetProcessHeap(),
				      HEAP_ZERO_MEMORY,
				      formatEntry->nrealized * sizeof(formatEntry->bitmaps[0]));
        }
        if (formatEntry->gis)
	    formatEntry->gis = HeapReAlloc(GetProcessHeap(),
				   HEAP_ZERO_MEMORY,
				   formatEntry->gis,
				   formatEntry->nrealized * sizeof(formatEntry->gis[0]));
        else
	    formatEntry->gis = HeapAlloc(GetProcessHeap(),
				   HEAP_ZERO_MEMORY,
                                   formatEntry->nrealized * sizeof(formatEntry->gis[0]));
    }


    if(formatEntry->glyphset == 0 && X11DRV_XRender_Installed) {
        switch(format) {
            case AA_Grey:
                wxr_format = WXR_FORMAT_GRAY;
                break;

            case AA_RGB:
            case AA_BGR:
            case AA_VRGB:
            case AA_VBGR:
                wxr_format = WXR_FORMAT_A8R8G8B8;
                break;

            default:
                ERR("aa = %d - not implemented\n", format);
            case AA_None:
                wxr_format = WXR_FORMAT_MONO;
                break;
        }

        wine_tsx11_lock();
        formatEntry->font_format = get_xrender_format(wxr_format);
        formatEntry->glyphset = pXRenderCreateGlyphSet(gdi_display, formatEntry->font_format->pict_format);
        wine_tsx11_unlock();
    }


    buf = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, buflen);
    GetGlyphOutlineW(physDev->hdc, glyph, ggo_format, &gm, buflen, buf, &identity);
    formatEntry->realized[glyph] = TRUE;

    TRACE("buflen = %d. Got metrics: %dx%d adv=%d,%d origin=%d,%d\n",
	  buflen,
	  gm.gmBlackBoxX, gm.gmBlackBoxY, gm.gmCellIncX, gm.gmCellIncY,
	  gm.gmptGlyphOrigin.x, gm.gmptGlyphOrigin.y);

    gi.width = gm.gmBlackBoxX;
    gi.height = gm.gmBlackBoxY;
    gi.x = -gm.gmptGlyphOrigin.x;
    gi.y = gm.gmptGlyphOrigin.y;
    gi.xOff = gm.gmCellIncX;
    gi.yOff = gm.gmCellIncY;

    if(TRACE_ON(xrender)) {
        int pitch, i, j;
	char output[300];
	unsigned char *line;

	if(format == AA_None) {
	    pitch = ((gi.width + 31) / 32) * 4;
	    for(i = 0; i < gi.height; i++) {
	        line = (unsigned char*) buf + i * pitch;
		output[0] = '\0';
		for(j = 0; j < pitch * 8; j++) {
	            strcat(output, (line[j / 8] & (1 << (7 - (j % 8)))) ? "#" : " ");
		}
		TRACE("%s\n", output);
	    }
	} else {
	    static const char blks[] = " .:;!o*#";
	    char str[2];

	    str[1] = '\0';
	    pitch = ((gi.width + 3) / 4) * 4;
	    for(i = 0; i < gi.height; i++) {
	        line = (unsigned char*) buf + i * pitch;
		output[0] = '\0';
		for(j = 0; j < pitch; j++) {
		    str[0] = blks[line[j] >> 5];
		    strcat(output, str);
		}
		TRACE("%s\n", output);
	    }
	}
    }


    if(formatEntry->glyphset) {
        if(format == AA_None && BitmapBitOrder(gdi_display) != MSBFirst) {
	    unsigned char *byte = (unsigned char*) buf, c;
	    int i = buflen;

	    while(i--) {
	        c = *byte;

		/* magic to flip bit order */
		c = ((c << 1) & 0xaa) | ((c >> 1) & 0x55);
		c = ((c << 2) & 0xcc) | ((c >> 2) & 0x33);
		c = ((c << 4) & 0xf0) | ((c >> 4) & 0x0f);

		*byte++ = c;
	    }
	}
        else if ( format != AA_Grey &&
                  ImageByteOrder (gdi_display) != NATIVE_BYTE_ORDER)
        {
            unsigned int i, *data = (unsigned int *)buf;
            for (i = buflen / sizeof(int); i; i--, data++) *data = RtlUlongByteSwap(*data);
        }
	gid = glyph;

        /*
          XRenderCompositeText seems to ignore 0x0 glyphs when
          AA_None, which means we lose the advance width of glyphs
          like the space.  We'll pretend that such glyphs are 1x1
          bitmaps.
        */

        if(buflen == 0)
            gi.width = gi.height = 1;

        wine_tsx11_lock();
	pXRenderAddGlyphs(gdi_display, formatEntry->glyphset, &gid, &gi, 1,
                          buflen ? buf : zero, buflen ? buflen : sizeof(zero));
	wine_tsx11_unlock();
	HeapFree(GetProcessHeap(), 0, buf);
    } else {
        formatEntry->bitmaps[glyph] = buf;
    }

    formatEntry->gis[glyph] = gi;
}

static void SharpGlyphMono(X11DRV_PDEVICE *physDev, INT x, INT y,
			    void *bitmap, XGlyphInfo *gi)
{
    unsigned char   *srcLine = bitmap, *src;
    unsigned char   bits, bitsMask;
    int             width = gi->width;
    int             stride = ((width + 31) & ~31) >> 3;
    int             height = gi->height;
    int             w;
    int             xspan, lenspan;

    TRACE("%d, %d\n", x, y);
    x -= gi->x;
    y -= gi->y;
    while (height--)
    {
        src = srcLine;
        srcLine += stride;
        w = width;
        
        bitsMask = 0x80;    /* FreeType is always MSB first */
        bits = *src++;
        
        xspan = x;
        while (w)
        {
            if (bits & bitsMask)
            {
                lenspan = 0;
                do
                {
                    lenspan++;
                    if (lenspan == w)
                        break;
                    bitsMask = bitsMask >> 1;
                    if (!bitsMask)
                    {
                        bits = *src++;
                        bitsMask = 0x80;
                    }
                } while (bits & bitsMask);
                XFillRectangle (gdi_display, physDev->drawable, 
                                physDev->gc, xspan, y, lenspan, 1);
                xspan += lenspan;
                w -= lenspan;
            }
            else
            {
                do
                {
                    w--;
                    xspan++;
                    if (!w)
                        break;
                    bitsMask = bitsMask >> 1;
                    if (!bitsMask)
                    {
                        bits = *src++;
                        bitsMask = 0x80;
                    }
                } while (!(bits & bitsMask));
            }
        }
        y++;
    }
}

static void SharpGlyphGray(X11DRV_PDEVICE *physDev, INT x, INT y,
			    void *bitmap, XGlyphInfo *gi)
{
    unsigned char   *srcLine = bitmap, *src, bits;
    int             width = gi->width;
    int             stride = ((width + 3) & ~3);
    int             height = gi->height;
    int             w;
    int             xspan, lenspan;

    x -= gi->x;
    y -= gi->y;
    while (height--)
    {
        src = srcLine;
        srcLine += stride;
        w = width;
        
        bits = *src++;
        xspan = x;
        while (w)
        {
            if (bits >= 0x80)
            {
                lenspan = 0;
                do
                {
                    lenspan++;
                    if (lenspan == w)
                        break;
                    bits = *src++;
                } while (bits >= 0x80);
                XFillRectangle (gdi_display, physDev->drawable, 
                                physDev->gc, xspan, y, lenspan, 1);
                xspan += lenspan;
                w -= lenspan;
            }
            else
            {
                do
                {
                    w--;
                    xspan++;
                    if (!w)
                        break;
                    bits = *src++;
                } while (bits < 0x80);
            }
        }
        y++;
    }
}


static void ExamineBitfield (DWORD mask, int *shift, int *len)
{
    int s, l;

    s = 0;
    while ((mask & 1) == 0)
    {
        mask >>= 1;
        s++;
    }
    l = 0;
    while ((mask & 1) == 1)
    {
        mask >>= 1;
        l++;
    }
    *shift = s;
    *len = l;
}

static DWORD GetField (DWORD pixel, int shift, int len)
{
    pixel = pixel & (((1 << (len)) - 1) << shift);
    pixel = pixel << (32 - (shift + len)) >> 24;
    while (len < 8)
    {
        pixel |= (pixel >> len);
        len <<= 1;
    }
    return pixel;
}


static DWORD PutField (DWORD pixel, int shift, int len)
{
    shift = shift - (8 - len);
    if (len <= 8)
        pixel &= (((1 << len) - 1) << (8 - len));
    if (shift < 0)
        pixel >>= -shift;
    else
        pixel <<= shift;
    return pixel;
}

static void SmoothGlyphGray(XImage *image, int x, int y, void *bitmap, XGlyphInfo *gi,
			    int color)
{
    int             r_shift, r_len;
    int             g_shift, g_len;
    int             b_shift, b_len;
    BYTE            *maskLine, *mask, m;
    int             maskStride;
    DWORD           pixel;
    int             width, height;
    int             w, tx;
    BYTE            src_r, src_g, src_b;

    x -= gi->x;
    y -= gi->y;
    width = gi->width;
    height = gi->height;

    maskLine = bitmap;
    maskStride = (width + 3) & ~3;

    ExamineBitfield (image->red_mask, &r_shift, &r_len);
    ExamineBitfield (image->green_mask, &g_shift, &g_len);
    ExamineBitfield (image->blue_mask, &b_shift, &b_len);

    src_r = GetField(color, r_shift, r_len);
    src_g = GetField(color, g_shift, g_len);
    src_b = GetField(color, b_shift, b_len);
    
    for(; height--; y++)
    {
        mask = maskLine;
        maskLine += maskStride;
        w = width;
        tx = x;

	if(y < 0) continue;
	if(y >= image->height) break;

        for(; w--; tx++)
        {
	    if(tx >= image->width) break;

            m = *mask++;
	    if(tx < 0) continue;

	    if (m == 0xff)
		XPutPixel (image, tx, y, color);
	    else if (m)
	    {
	        BYTE r, g, b;

		pixel = XGetPixel (image, tx, y);

		r = GetField(pixel, r_shift, r_len);
		r = ((BYTE)~m * (WORD)r + (BYTE)m * (WORD)src_r) >> 8;
		g = GetField(pixel, g_shift, g_len);
		g = ((BYTE)~m * (WORD)g + (BYTE)m * (WORD)src_g) >> 8;
		b = GetField(pixel, b_shift, b_len);
		b = ((BYTE)~m * (WORD)b + (BYTE)m * (WORD)src_b) >> 8;

		pixel = (PutField (r, r_shift, r_len) |
			 PutField (g, g_shift, g_len) |
			 PutField (b, b_shift, b_len));
		XPutPixel (image, tx, y, pixel);
	    }
        }
    }
}

/*************************************************************
 *                 get_tile_pict
 *
 * Returns an appropriate Picture for tiling the text colour.
 * Call and use result within the xrender_cs
 */
static Picture get_tile_pict(const WineXRenderFormat *wxr_format, int text_pixel)
{
    static struct
    {
        Pixmap xpm;
        Picture pict;
        int current_color;
    } tiles[WXR_NB_FORMATS], *tile;
    XRenderColor col;

    tile = &tiles[wxr_format->format];

    if(!tile->xpm)
    {
        XRenderPictureAttributes pa;

        wine_tsx11_lock();
        tile->xpm = XCreatePixmap(gdi_display, root_window, 1, 1, wxr_format->pict_format->depth);

        pa.repeat = RepeatNormal;
        tile->pict = pXRenderCreatePicture(gdi_display, tile->xpm, wxr_format->pict_format, CPRepeat, &pa);
        wine_tsx11_unlock();

        /* init current_color to something different from text_pixel */
        tile->current_color = ~text_pixel;

        if(wxr_format->format == WXR_FORMAT_MONO)
        {
            /* for a 1bpp bitmap we always need a 1 in the tile */
            col.red = col.green = col.blue = 0;
            col.alpha = 0xffff;
            wine_tsx11_lock();
            pXRenderFillRectangle(gdi_display, PictOpSrc, tile->pict, &col, 0, 0, 1, 1);
            wine_tsx11_unlock();
        }
    }

    if(text_pixel != tile->current_color && wxr_format->format != WXR_FORMAT_MONO)
    {
        get_xrender_color(wxr_format, text_pixel, &col);
        wine_tsx11_lock();
        pXRenderFillRectangle(gdi_display, PictOpSrc, tile->pict, &col, 0, 0, 1, 1);
        wine_tsx11_unlock();
        tile->current_color = text_pixel;
    }
    return tile->pict;
}

/*************************************************************
 *                 get_mask_pict
 *
 * Returns an appropriate Picture for masking with the specified alpha.
 * Call and use result within the xrender_cs
 */
static Picture get_mask_pict( int alpha )
{
    static Pixmap pixmap;
    static Picture pict;
    static int current_alpha;

    if (alpha == 0xffff) return 0;  /* don't need a mask for alpha==1.0 */

    if (!pixmap)
    {
        const WineXRenderFormat *fmt = get_xrender_format( WXR_FORMAT_A8R8G8B8 );
        XRenderPictureAttributes pa;

        wine_tsx11_lock();
        pixmap = XCreatePixmap( gdi_display, root_window, 1, 1, 32 );
        pa.repeat = RepeatNormal;
        pict = pXRenderCreatePicture( gdi_display, pixmap, fmt->pict_format, CPRepeat, &pa );
        wine_tsx11_unlock();
        current_alpha = -1;
    }

    if (alpha != current_alpha)
    {
        XRenderColor col;
        col.red = col.green = col.blue = 0;
        col.alpha = current_alpha = alpha;
        wine_tsx11_lock();
        pXRenderFillRectangle( gdi_display, PictOpSrc, pict, &col, 0, 0, 1, 1 );
        wine_tsx11_unlock();
    }
    return pict;
}

static int XRenderErrorHandler(Display *dpy, XErrorEvent *event, void *arg)
{
    return 1;
}

/***********************************************************************
 *   X11DRV_XRender_ExtTextOut
 */
BOOL X11DRV_XRender_ExtTextOut( X11DRV_PDEVICE *physDev, INT x, INT y, UINT flags,
				const RECT *lprect, LPCWSTR wstr, UINT count,
				const INT *lpDx )
{
    XGCValues xgcval;
    gsCacheEntry *entry;
    gsCacheEntryFormat *formatEntry;
    BOOL retv = FALSE;
    int textPixel, backgroundPixel;
    HRGN saved_region = 0;
    BOOL disable_antialias = FALSE;
    AA_Type aa_type = AA_None;
    DIBSECTION bmp;
    unsigned int idx;
    const WineXRenderFormat *dst_format = get_xrender_format_from_color_shifts(physDev->depth, physDev->color_shifts);
    Picture tile_pict = 0;

    /* Do we need to disable antialiasing because of palette mode? */
    if( !physDev->bitmap || GetObjectW( physDev->bitmap->hbitmap, sizeof(bmp), &bmp ) != sizeof(bmp) ) {
        TRACE("bitmap is not a DIB\n");
    }
    else if (bmp.dsBmih.biBitCount <= 8) {
        TRACE("Disabling antialiasing\n");
        disable_antialias = TRUE;
    }

    xgcval.function = GXcopy;
    xgcval.background = physDev->backgroundPixel;
    xgcval.fill_style = FillSolid;
    wine_tsx11_lock();
    XChangeGC( gdi_display, physDev->gc, GCFunction | GCBackground | GCFillStyle, &xgcval );
    wine_tsx11_unlock();

    X11DRV_LockDIBSection( physDev, DIB_Status_GdiMod );

    if(physDev->depth == 1) {
        if((physDev->textPixel & 0xffffff) == 0) {
	    textPixel = 0;
	    backgroundPixel = 1;
	} else {
	    textPixel = 1;
	    backgroundPixel = 0;
	}
    } else {
        textPixel = physDev->textPixel;
	backgroundPixel = physDev->backgroundPixel;
    }

    if(flags & ETO_OPAQUE)
    {
        wine_tsx11_lock();
        XSetForeground( gdi_display, physDev->gc, backgroundPixel );
        XFillRectangle( gdi_display, physDev->drawable, physDev->gc,
                        physDev->dc_rect.left + lprect->left, physDev->dc_rect.top + lprect->top,
                        lprect->right - lprect->left, lprect->bottom - lprect->top );
        wine_tsx11_unlock();
    }

    if(count == 0)
    {
	retv = TRUE;
        goto done_unlock;
    }

    if (flags & ETO_CLIPPED)
    {
        HRGN clip_region;

        clip_region = CreateRectRgnIndirect( lprect );
        /* make a copy of the current device region */
        saved_region = CreateRectRgn( 0, 0, 0, 0 );
        CombineRgn( saved_region, physDev->region, 0, RGN_COPY );
        X11DRV_SetDeviceClipping( physDev, saved_region, clip_region );
        DeleteObject( clip_region );
    }

    EnterCriticalSection(&xrender_cs);

    entry = glyphsetCache + physDev->xrender->cache_index;
    if( disable_antialias == FALSE )
        aa_type = entry->aa_default;
    formatEntry = entry->format[aa_type];

    for(idx = 0; idx < count; idx++) {
        if( !formatEntry ) {
	    UploadGlyph(physDev, wstr[idx], aa_type);
            /* re-evaluate antialias since aa_default may have changed */
            if( disable_antialias == FALSE )
                aa_type = entry->aa_default;
            formatEntry = entry->format[aa_type];
        } else if( wstr[idx] >= formatEntry->nrealized || formatEntry->realized[wstr[idx]] == FALSE) {
	    UploadGlyph(physDev, wstr[idx], aa_type);
	}
    }
    if (!formatEntry)
    {
        WARN("could not upload requested glyphs\n");
        LeaveCriticalSection(&xrender_cs);
        goto done_unlock;
    }

    TRACE("Writing %s at %d,%d\n", debugstr_wn(wstr,count),
          physDev->dc_rect.left + x, physDev->dc_rect.top + y);

    if(X11DRV_XRender_Installed)
    {
        XGlyphElt16 *elts = HeapAlloc(GetProcessHeap(), 0, sizeof(XGlyphElt16) * count);
        POINT offset = {0, 0};
        POINT desired, current;
        int render_op = PictOpOver;
        Picture pict = get_xrender_picture(physDev);

        /* There's a bug in XRenderCompositeText that ignores the xDst and yDst parameters.
           So we pass zeros to the function and move to our starting position using the first
           element of the elts array. */

        desired.x = physDev->dc_rect.left + x;
        desired.y = physDev->dc_rect.top + y;
        current.x = current.y = 0;

        tile_pict = get_tile_pict(dst_format, physDev->textPixel);

	/* FIXME the mapping of Text/BkColor onto 1 or 0 needs investigation.
	 */
	if((dst_format->format == WXR_FORMAT_MONO) && (textPixel == 0))
	    render_op = PictOpOutReverse; /* This gives us 'black' text */

        for(idx = 0; idx < count; idx++)
        {
            elts[idx].glyphset = formatEntry->glyphset;
            elts[idx].chars = wstr + idx;
            elts[idx].nchars = 1;
            elts[idx].xOff = desired.x - current.x;
            elts[idx].yOff = desired.y - current.y;

            current.x += (elts[idx].xOff + formatEntry->gis[wstr[idx]].xOff);
            current.y += (elts[idx].yOff + formatEntry->gis[wstr[idx]].yOff);

            if(!lpDx)
            {
                desired.x += formatEntry->gis[wstr[idx]].xOff;
                desired.y += formatEntry->gis[wstr[idx]].yOff;
            }
            else
            {
                if(flags & ETO_PDY)
                {
                    offset.x += lpDx[idx * 2];
                    offset.y += lpDx[idx * 2 + 1];
                }
                else
                    offset.x += lpDx[idx];
                desired.x = physDev->dc_rect.left + x + offset.x;
                desired.y = physDev->dc_rect.top  + y + offset.y;
            }
        }
        wine_tsx11_lock();
        /* Make sure we don't have any transforms set from a previous call */
        set_xrender_transformation(pict, 1, 1, 0, 0);
        pXRenderCompositeText16(gdi_display, render_op,
                                tile_pict,
                                pict,
                                formatEntry->font_format->pict_format,
                                0, 0, 0, 0, elts, count);
        wine_tsx11_unlock();
        HeapFree(GetProcessHeap(), 0, elts);
    } else {
        POINT offset = {0, 0};
        wine_tsx11_lock();
	XSetForeground( gdi_display, physDev->gc, textPixel );

        if(aa_type == AA_None || physDev->depth == 1)
        {
            void (* sharp_glyph_fn)(X11DRV_PDEVICE *, INT, INT, void *, XGlyphInfo *);

            if(aa_type == AA_None)
                sharp_glyph_fn = SharpGlyphMono;
            else
                sharp_glyph_fn = SharpGlyphGray;

	    for(idx = 0; idx < count; idx++) {
                sharp_glyph_fn(physDev,
                               physDev->dc_rect.left + x + offset.x,
                               physDev->dc_rect.top  + y + offset.y,
			       formatEntry->bitmaps[wstr[idx]],
			       &formatEntry->gis[wstr[idx]]);
                if(lpDx)
                {
                    if(flags & ETO_PDY)
                    {
                        offset.x += lpDx[idx * 2];
                        offset.y += lpDx[idx * 2 + 1];
                    }
                    else
                        offset.x += lpDx[idx];
                }
                else
                {
                    offset.x += formatEntry->gis[wstr[idx]].xOff;
                    offset.y += formatEntry->gis[wstr[idx]].yOff;
		}
	    }
	} else {
	    XImage *image;
	    int image_x, image_y, image_off_x, image_off_y, image_w, image_h;
	    RECT extents = {0, 0, 0, 0};
	    POINT cur = {0, 0};
            int w = physDev->drawable_rect.right - physDev->drawable_rect.left;
            int h = physDev->drawable_rect.bottom - physDev->drawable_rect.top;

	    TRACE("drawable %dx%d\n", w, h);

	    for(idx = 0; idx < count; idx++) {
	        if(extents.left > cur.x - formatEntry->gis[wstr[idx]].x)
		    extents.left = cur.x - formatEntry->gis[wstr[idx]].x;
		if(extents.top > cur.y - formatEntry->gis[wstr[idx]].y)
		    extents.top = cur.y - formatEntry->gis[wstr[idx]].y;
		if(extents.right < cur.x - formatEntry->gis[wstr[idx]].x + formatEntry->gis[wstr[idx]].width)
		    extents.right = cur.x - formatEntry->gis[wstr[idx]].x + formatEntry->gis[wstr[idx]].width;
		if(extents.bottom < cur.y - formatEntry->gis[wstr[idx]].y + formatEntry->gis[wstr[idx]].height)
		    extents.bottom = cur.y - formatEntry->gis[wstr[idx]].y + formatEntry->gis[wstr[idx]].height;

                if(lpDx)
                {
                    if(flags & ETO_PDY)
                    {
                        cur.x += lpDx[idx * 2];
                        cur.y += lpDx[idx * 2 + 1];
                    }
                    else
                        cur.x += lpDx[idx];
                }
                else
                {
                    cur.x += formatEntry->gis[wstr[idx]].xOff;
                    cur.y += formatEntry->gis[wstr[idx]].yOff;
		}
	    }
	    TRACE("glyph extents %d,%d - %d,%d drawable x,y %d,%d\n", extents.left, extents.top,
		  extents.right, extents.bottom, physDev->dc_rect.left + x, physDev->dc_rect.top + y);

	    if(physDev->dc_rect.left + x + extents.left >= 0) {
	        image_x = physDev->dc_rect.left + x + extents.left;
		image_off_x = 0;
	    } else {
	        image_x = 0;
		image_off_x = physDev->dc_rect.left + x + extents.left;
	    }
	    if(physDev->dc_rect.top + y + extents.top >= 0) {
	        image_y = physDev->dc_rect.top + y + extents.top;
		image_off_y = 0;
	    } else {
	        image_y = 0;
		image_off_y = physDev->dc_rect.top + y + extents.top;
	    }
	    if(physDev->dc_rect.left + x + extents.right < w)
	        image_w = physDev->dc_rect.left + x + extents.right - image_x;
	    else
	        image_w = w - image_x;
	    if(physDev->dc_rect.top + y + extents.bottom < h)
	        image_h = physDev->dc_rect.top + y + extents.bottom - image_y;
	    else
	        image_h = h - image_y;

	    if(image_w <= 0 || image_h <= 0) goto no_image;

	    X11DRV_expect_error(gdi_display, XRenderErrorHandler, NULL);
	    image = XGetImage(gdi_display, physDev->drawable,
			      image_x, image_y, image_w, image_h,
			      AllPlanes, ZPixmap);
	    X11DRV_check_error();

	    TRACE("XGetImage(%p, %x, %d, %d, %d, %d, %lx, %x) depth = %d rets %p\n",
		  gdi_display, (int)physDev->drawable, image_x, image_y,
		  image_w, image_h, AllPlanes, ZPixmap,
		  physDev->depth, image);
	    if(!image) {
	        Pixmap xpm = XCreatePixmap(gdi_display, root_window, image_w, image_h,
					   physDev->depth);
		GC gc;
		XGCValues gcv;

		gcv.graphics_exposures = False;
		gc = XCreateGC(gdi_display, xpm, GCGraphicsExposures, &gcv);
		XCopyArea(gdi_display, physDev->drawable, xpm, gc, image_x, image_y,
			  image_w, image_h, 0, 0);
		XFreeGC(gdi_display, gc);
		X11DRV_expect_error(gdi_display, XRenderErrorHandler, NULL);
		image = XGetImage(gdi_display, xpm, 0, 0, image_w, image_h, AllPlanes,
				  ZPixmap);
		X11DRV_check_error();
		XFreePixmap(gdi_display, xpm);
	    }
	    if(!image) goto no_image;

	    image->red_mask = visual->red_mask;
	    image->green_mask = visual->green_mask;
	    image->blue_mask = visual->blue_mask;

	    for(idx = 0; idx < count; idx++) {
	        SmoothGlyphGray(image,
                                offset.x + image_off_x - extents.left,
                                offset.y + image_off_y - extents.top,
				formatEntry->bitmaps[wstr[idx]],
				&formatEntry->gis[wstr[idx]],
				physDev->textPixel);
                if(lpDx)
                {
                    if(flags & ETO_PDY)
                    {
                        offset.x += lpDx[idx * 2];
                        offset.y += lpDx[idx * 2 + 1];
                    }
                    else
                        offset.x += lpDx[idx];
                }
                else
                {
                    offset.x += formatEntry->gis[wstr[idx]].xOff;
                    offset.y += formatEntry->gis[wstr[idx]].yOff;
                }
	    }
	    XPutImage(gdi_display, physDev->drawable, physDev->gc, image, 0, 0,
		      image_x, image_y, image_w, image_h);
	    XDestroyImage(image);
	}
    no_image:
	wine_tsx11_unlock();
    }
    LeaveCriticalSection(&xrender_cs);

    if (flags & ETO_CLIPPED)
    {
        /* restore the device region */
        X11DRV_SetDeviceClipping( physDev, saved_region, 0 );
        DeleteObject( saved_region );
    }

    retv = TRUE;

done_unlock:
    X11DRV_UnlockDIBSection( physDev, TRUE );
    return retv;
}

/* Helper function for (stretched) blitting using xrender */
static void xrender_blit( int op, Picture src_pict, Picture mask_pict, Picture dst_pict,
                          int x_src, int y_src, int x_dst, int y_dst,
                          double xscale, double yscale, int width, int height )
{
    int x_offset, y_offset;

    /* When we need to scale we perform scaling and source_x / source_y translation using a transformation matrix.
     * This is needed because XRender is inaccurate in combination with scaled source coordinates passed to XRenderComposite.
     * In all other cases we do use XRenderComposite for translation as it is faster than using a transformation matrix. */
    if(xscale != 1.0 || yscale != 1.0)
    {
        /* In case of mirroring we need a source x- and y-offset because without the pixels will be
         * in the wrong quadrant of the x-y plane.
         */
        x_offset = (xscale < 0) ? -width : 0;
        y_offset = (yscale < 0) ? -height : 0;
        set_xrender_transformation(src_pict, xscale, yscale, x_src, y_src);
    }
    else
    {
        x_offset = x_src;
        y_offset = y_src;
        set_xrender_transformation(src_pict, 1, 1, 0, 0);
    }
    pXRenderComposite( gdi_display, op, src_pict, mask_pict, dst_pict,
                       x_offset, y_offset, 0, 0, x_dst, y_dst, width, height );
}

/* Helper function for (stretched) mono->color blitting using xrender */
static void xrender_mono_blit( Picture src_pict, Picture mask_pict, Picture dst_pict,
                               int x_src, int y_src, double xscale, double yscale, int width, int height )
{
    int x_offset, y_offset;

    /* When doing a mono->color blit, 'src_pict' contains a 1x1 picture for tiling, the actual
     * source data is in mask_pict.  The 'src_pict' data effectively acts as an alpha channel to the
     * tile data. We need PictOpOver for correct rendering.
     * Note since the 'source data' is in the mask picture, we have to pass x_src / y_src using
     * mask_x / mask_y
     */
    if (xscale != 1.0 || yscale != 1.0)
    {
        /* In case of mirroring we need a source x- and y-offset because without the pixels will be
         * in the wrong quadrant of the x-y plane.
         */
        x_offset = (xscale < 0) ? -width : 0;
        y_offset = (yscale < 0) ? -height : 0;
        set_xrender_transformation(mask_pict, xscale, yscale, x_src, y_src);
    }
    else
    {
        x_offset = x_src;
        y_offset = y_src;
        set_xrender_transformation(mask_pict, 1, 1, 0, 0);
    }
    pXRenderComposite(gdi_display, PictOpOver, src_pict, mask_pict, dst_pict,
                      0, 0, x_offset, y_offset, 0, 0, width, height);
}

/******************************************************************************
 * AlphaBlend
 */
BOOL XRender_AlphaBlend( X11DRV_PDEVICE *devDst, X11DRV_PDEVICE *devSrc,
                         struct bitblt_coords *dst, struct bitblt_coords *src, BLENDFUNCTION blendfn )
{
    Picture dst_pict, src_pict = 0, mask_pict = 0, tmp_pict = 0;
    struct xrender_info *src_info = get_xrender_info( devSrc );
    double xscale, yscale;
    BOOL use_repeat;

    if(!X11DRV_XRender_Installed) {
        FIXME("Unable to AlphaBlend without Xrender\n");
        return FALSE;
    }

    if (devSrc != devDst) X11DRV_LockDIBSection( devSrc, DIB_Status_GdiMod );
    X11DRV_LockDIBSection( devDst, DIB_Status_GdiMod );

    dst_pict = get_xrender_picture( devDst );

    use_repeat = use_source_repeat( devSrc );
    if (!use_repeat)
    {
        xscale = src->width / (double)dst->width;
        yscale = src->height / (double)dst->height;
    }
    else xscale = yscale = 1;  /* no scaling needed with a repeating source */

    if (!(blendfn.AlphaFormat & AC_SRC_ALPHA) && src_info->format)
    {
        /* we need a source picture with no alpha */
        WXRFormat format = get_format_without_alpha( src_info->format->format );
        if (format != src_info->format->format)
        {
            XRenderPictureAttributes pa;
            const WineXRenderFormat *fmt = get_xrender_format( format );

            wine_tsx11_lock();
            pa.subwindow_mode = IncludeInferiors;
            pa.repeat = use_repeat ? RepeatNormal : RepeatNone;
            tmp_pict = pXRenderCreatePicture( gdi_display, devSrc->drawable, fmt->pict_format,
                                              CPSubwindowMode|CPRepeat, &pa );
            wine_tsx11_unlock();
            src_pict = tmp_pict;
        }
    }

    if (!src_pict) src_pict = get_xrender_picture_source( devSrc, use_repeat );

    EnterCriticalSection( &xrender_cs );
    mask_pict = get_mask_pict( blendfn.SourceConstantAlpha * 257 );

    wine_tsx11_lock();
    xrender_blit( PictOpOver, src_pict, mask_pict, dst_pict,
                  devSrc->dc_rect.left + src->visrect.left, devSrc->dc_rect.top + src->visrect.top,
                  devDst->dc_rect.left + dst->visrect.left, devDst->dc_rect.top + dst->visrect.top,
                  xscale, yscale,
                  dst->visrect.right - dst->visrect.left, dst->visrect.bottom - dst->visrect.top );
    if (tmp_pict) pXRenderFreePicture( gdi_display, tmp_pict );
    wine_tsx11_unlock();

    LeaveCriticalSection( &xrender_cs );
    if (devSrc != devDst) X11DRV_UnlockDIBSection( devSrc, FALSE );
    X11DRV_UnlockDIBSection( devDst, TRUE );
    return TRUE;
}


void X11DRV_XRender_CopyBrush(X11DRV_PDEVICE *physDev, X_PHYSBITMAP *physBitmap, int width, int height)
{
    /* At depths >1, the depth of physBitmap and physDev might not be the same e.g. the physbitmap might be a 16-bit DIB while the physdev uses 24-bit */
    int depth = physBitmap->pixmap_depth == 1 ? 1 : physDev->depth;
    const WineXRenderFormat *src_format = get_xrender_format_from_color_shifts(physBitmap->pixmap_depth, &physBitmap->pixmap_color_shifts);
    const WineXRenderFormat *dst_format = get_xrender_format_from_color_shifts(physDev->depth, physDev->color_shifts);

    wine_tsx11_lock();
    physDev->brush.pixmap = XCreatePixmap(gdi_display, root_window, width, height, depth);

    /* Use XCopyArea when the physBitmap and brush.pixmap have the same format. */
    if( (physBitmap->pixmap_depth == 1) || (!X11DRV_XRender_Installed && physDev->depth == physBitmap->pixmap_depth) ||
        (src_format->format == dst_format->format) )
    {
        XCopyArea( gdi_display, physBitmap->pixmap, physDev->brush.pixmap,
                   get_bitmap_gc(physBitmap->pixmap_depth), 0, 0, width, height, 0, 0 );
    }
    else /* We need depth conversion */
    {
        Picture src_pict, dst_pict;
        XRenderPictureAttributes pa;
        pa.subwindow_mode = IncludeInferiors;
        pa.repeat = RepeatNone;

        src_pict = pXRenderCreatePicture(gdi_display, physBitmap->pixmap, src_format->pict_format, CPSubwindowMode|CPRepeat, &pa);
        dst_pict = pXRenderCreatePicture(gdi_display, physDev->brush.pixmap, dst_format->pict_format, CPSubwindowMode|CPRepeat, &pa);

        xrender_blit(PictOpSrc, src_pict, 0, dst_pict, 0, 0, 0, 0, 1.0, 1.0, width, height);
        pXRenderFreePicture(gdi_display, src_pict);
        pXRenderFreePicture(gdi_display, dst_pict);
    }
    wine_tsx11_unlock();
}

BOOL X11DRV_XRender_GetSrcAreaStretch(X11DRV_PDEVICE *physDevSrc, X11DRV_PDEVICE *physDevDst,
                                      Pixmap pixmap, GC gc,
                                      const struct bitblt_coords *src, const struct bitblt_coords *dst )
{
    BOOL stretch = (src->width != dst->width) || (src->height != dst->height);
    int width = dst->visrect.right - dst->visrect.left;
    int height = dst->visrect.bottom - dst->visrect.top;
    int x_src = physDevSrc->dc_rect.left + src->visrect.left;
    int y_src = physDevSrc->dc_rect.top + src->visrect.top;
    struct xrender_info *src_info = get_xrender_info(physDevSrc);
    const WineXRenderFormat *dst_format = get_xrender_format_from_color_shifts(physDevDst->depth, physDevDst->color_shifts);
    Picture src_pict=0, dst_pict=0, mask_pict=0;
    BOOL use_repeat;
    double xscale, yscale;

    XRenderPictureAttributes pa;
    pa.subwindow_mode = IncludeInferiors;
    pa.repeat = RepeatNone;

    TRACE("src depth=%d widthSrc=%d heightSrc=%d xSrc=%d ySrc=%d\n",
          physDevSrc->depth, src->width, src->height, x_src, y_src);
    TRACE("dst depth=%d widthDst=%d heightDst=%d\n", physDevDst->depth, dst->width, dst->height);

    if(!X11DRV_XRender_Installed)
    {
        TRACE("Not using XRender since it is not available or disabled\n");
        return FALSE;
    }

    /* XRender can't handle palettes, so abort */
    if(X11DRV_PALETTE_XPixelToPalette)
        return FALSE;

    /* XRender is of no use in this case */
    if((physDevDst->depth == 1) && (physDevSrc->depth > 1))
        return FALSE;

    /* Just use traditional X copy when the formats match and we don't need stretching */
    if((src_info->format->format == dst_format->format) && !stretch)
    {
        TRACE("Source and destination depth match and no stretching needed falling back to XCopyArea\n");
        wine_tsx11_lock();
        XCopyArea( gdi_display, physDevSrc->drawable, pixmap, gc, x_src, y_src, width, height, 0, 0);
        wine_tsx11_unlock();
        return TRUE;
    }

    use_repeat = use_source_repeat( physDevSrc );
    if (!use_repeat)
    {
        xscale = src->width / (double)dst->width;
        yscale = src->height / (double)dst->height;
    }
    else xscale = yscale = 1;  /* no scaling needed with a repeating source */

    /* mono -> color */
    if(physDevSrc->depth == 1 && physDevDst->depth > 1)
    {
        XRenderColor col;
        get_xrender_color(dst_format, physDevDst->textPixel, &col);

        /* We use the source drawable as a mask */
        mask_pict = get_xrender_picture_source( physDevSrc, use_repeat );

        /* Use backgroundPixel as the foreground color */
        EnterCriticalSection( &xrender_cs );
        src_pict = get_tile_pict(dst_format, physDevDst->backgroundPixel);

        /* Create a destination picture and fill it with textPixel color as the background color */
        wine_tsx11_lock();
        dst_pict = pXRenderCreatePicture(gdi_display, pixmap, dst_format->pict_format, CPSubwindowMode|CPRepeat, &pa);
        pXRenderFillRectangle(gdi_display, PictOpSrc, dst_pict, &col, 0, 0, width, height);

        xrender_mono_blit(src_pict, mask_pict, dst_pict, x_src, y_src, xscale, yscale, width, height);

        if(dst_pict) pXRenderFreePicture(gdi_display, dst_pict);
        wine_tsx11_unlock();
        LeaveCriticalSection( &xrender_cs );
    }
    else /* color -> color (can be at different depths) or mono -> mono */
    {
        if (physDevDst->depth == 32 && physDevSrc->depth < 32) mask_pict = get_no_alpha_mask();
        src_pict = get_xrender_picture_source( physDevSrc, use_repeat );

        wine_tsx11_lock();
        dst_pict = pXRenderCreatePicture(gdi_display,
                                          pixmap, dst_format->pict_format,
                                          CPSubwindowMode|CPRepeat, &pa);

        xrender_blit(PictOpSrc, src_pict, mask_pict, dst_pict,
                     x_src, y_src, 0, 0, xscale, yscale, width, height);

        if(dst_pict) pXRenderFreePicture(gdi_display, dst_pict);
        wine_tsx11_unlock();
    }
    return TRUE;
}

#else /* SONAME_LIBXRENDER */

void X11DRV_XRender_Init(void)
{
    TRACE("XRender support not compiled in.\n");
    return;
}

void X11DRV_XRender_Finalize(void)
{
}

BOOL X11DRV_XRender_SelectFont(X11DRV_PDEVICE *physDev, HFONT hfont)
{
  assert(0);
  return FALSE;
}

void X11DRV_XRender_DeleteDC(X11DRV_PDEVICE *physDev)
{
  assert(0);
  return;
}

void X11DRV_XRender_SetDeviceClipping(X11DRV_PDEVICE *physDev, const RGNDATA *data)
{
    assert(0);
    return;
}

BOOL X11DRV_XRender_ExtTextOut( X11DRV_PDEVICE *physDev, INT x, INT y, UINT flags,
				const RECT *lprect, LPCWSTR wstr, UINT count,
				const INT *lpDx )
{
  assert(0);
  return FALSE;
}

void X11DRV_XRender_UpdateDrawable(X11DRV_PDEVICE *physDev)
{
  assert(0);
  return;
}

BOOL XRender_AlphaBlend( X11DRV_PDEVICE *devDst, X11DRV_PDEVICE *devSrc,
                         struct bitblt_coords *dst, struct bitblt_coords *src, BLENDFUNCTION blendfn )
{
  FIXME("not supported - XRENDER headers were missing at compile time\n");
  return FALSE;
}

void X11DRV_XRender_CopyBrush(X11DRV_PDEVICE *physDev, X_PHYSBITMAP *physBitmap, int width, int height)
{
    wine_tsx11_lock();
    physDev->brush.pixmap = XCreatePixmap(gdi_display, root_window, width, height, physBitmap->pixmap_depth);

    XCopyArea( gdi_display, physBitmap->pixmap, physDev->brush.pixmap,
               get_bitmap_gc(physBitmap->pixmap_depth), 0, 0, width, height, 0, 0 );
    wine_tsx11_unlock();
}

BOOL X11DRV_XRender_SetPhysBitmapDepth(X_PHYSBITMAP *physBitmap, int bits_pixel, const DIBSECTION *dib)
{
    return FALSE;
}

BOOL X11DRV_XRender_GetSrcAreaStretch(X11DRV_PDEVICE *physDevSrc, X11DRV_PDEVICE *physDevDst,
                                      Pixmap pixmap, GC gc,
                                      const struct bitblt_coords *src, const struct bitblt_coords *dst )
{
    return FALSE;
}
#endif /* SONAME_LIBXRENDER */

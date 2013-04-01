/*
 * Mesa 3-D graphics library
 * Version:  7.7
 *
 * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
 * Copyright (c) 2008-2009  VMware, Inc.
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


#include "imports.h"
#include "formats.h"
#include "mfeatures.h"
#include "macros.h"


/**
 * Information about texture formats.
 */
struct gl_format_info
{
   gl_format Name;

   /** text name for debugging */
   const char *StrName;

   /**
    * Base format is one of GL_RED, GL_RG, GL_RGB, GL_RGBA, GL_ALPHA,
    * GL_LUMINANCE, GL_LUMINANCE_ALPHA, GL_INTENSITY, GL_YCBCR_MESA,
    * GL_DEPTH_COMPONENT, GL_STENCIL_INDEX, GL_DEPTH_STENCIL, GL_DUDV_ATI.
    */
   GLenum BaseFormat;

   /**
    * Logical data type: one of  GL_UNSIGNED_NORMALIZED, GL_SIGNED_NORMALIZED,
    * GL_UNSIGNED_INT, GL_INT, GL_FLOAT.
    */
   GLenum DataType;

   GLubyte RedBits;
   GLubyte GreenBits;
   GLubyte BlueBits;
   GLubyte AlphaBits;
   GLubyte LuminanceBits;
   GLubyte IntensityBits;
   GLubyte IndexBits;
   GLubyte DepthBits;
   GLubyte StencilBits;

   /**
    * To describe compressed formats.  If not compressed, Width=Height=1.
    */
   GLubyte BlockWidth, BlockHeight;
   GLubyte BytesPerBlock;
};


/**
 * Info about each format.
 * These must be in the same order as the MESA_FORMAT_* enums so that
 * we can do lookups without searching.
 */
static struct gl_format_info format_info[MESA_FORMAT_COUNT] =
{
   {
      MESA_FORMAT_NONE,            /* Name */
      "MESA_FORMAT_NONE",          /* StrName */
      GL_NONE,                     /* BaseFormat */
      GL_NONE,                     /* DataType */
      0, 0, 0, 0,                  /* Red/Green/Blue/AlphaBits */
      0, 0, 0, 0, 0,               /* Lum/Int/Index/Depth/StencilBits */
      0, 0, 0                      /* BlockWidth/Height,Bytes */
   },
   {
      MESA_FORMAT_RGBA8888,        /* Name */
      "MESA_FORMAT_RGBA8888",      /* StrName */
      GL_RGBA,                     /* BaseFormat */
      GL_UNSIGNED_NORMALIZED,      /* DataType */
      8, 8, 8, 8,                  /* Red/Green/Blue/AlphaBits */
      0, 0, 0, 0, 0,               /* Lum/Int/Index/Depth/StencilBits */
      1, 1, 4                      /* BlockWidth/Height,Bytes */
   },
   {
      MESA_FORMAT_RGBA8888_REV,    /* Name */
      "MESA_FORMAT_RGBA8888_REV",  /* StrName */
      GL_RGBA,                     /* BaseFormat */
      GL_UNSIGNED_NORMALIZED,      /* DataType */
      8, 8, 8, 8,                  /* Red/Green/Blue/AlphaBits */
      0, 0, 0, 0, 0,               /* Lum/Int/Index/Depth/StencilBits */
      1, 1, 4                      /* BlockWidth/Height,Bytes */
   },
   {
      MESA_FORMAT_ARGB8888,        /* Name */
      "MESA_FORMAT_ARGB8888",      /* StrName */
      GL_RGBA,                     /* BaseFormat */
      GL_UNSIGNED_NORMALIZED,      /* DataType */
      8, 8, 8, 8,                  /* Red/Green/Blue/AlphaBits */
      0, 0, 0, 0, 0,               /* Lum/Int/Index/Depth/StencilBits */
      1, 1, 4                      /* BlockWidth/Height,Bytes */
   },
   {
      MESA_FORMAT_ARGB8888_REV,    /* Name */
      "MESA_FORMAT_ARGB8888_REV",  /* StrName */
      GL_RGBA,                     /* BaseFormat */
      GL_UNSIGNED_NORMALIZED,      /* DataType */
      8, 8, 8, 8,                  /* Red/Green/Blue/AlphaBits */
      0, 0, 0, 0, 0,               /* Lum/Int/Index/Depth/StencilBits */
      1, 1, 4                      /* BlockWidth/Height,Bytes */
   },
   {
      MESA_FORMAT_RGBX8888,        /* Name */
      "MESA_FORMAT_RGBX8888",      /* StrName */
      GL_RGB,                      /* BaseFormat */
      GL_UNSIGNED_NORMALIZED,      /* DataType */
      8, 8, 8, 0,                  /* Red/Green/Blue/AlphaBits */
      0, 0, 0, 0, 0,               /* Lum/Int/Index/Depth/StencilBits */
      1, 1, 4                      /* BlockWidth/Height,Bytes */
   },
   {
      MESA_FORMAT_RGBX8888_REV,    /* Name */
      "MESA_FORMAT_RGBX8888_REV",  /* StrName */
      GL_RGB,                      /* BaseFormat */
      GL_UNSIGNED_NORMALIZED,      /* DataType */
      8, 8, 8, 0,                  /* Red/Green/Blue/AlphaBits */
      0, 0, 0, 0, 0,               /* Lum/Int/Index/Depth/StencilBits */
      1, 1, 4                      /* BlockWidth/Height,Bytes */
   },
   {
      MESA_FORMAT_XRGB8888,        /* Name */
      "MESA_FORMAT_XRGB8888",      /* StrName */
      GL_RGB,                      /* BaseFormat */
      GL_UNSIGNED_NORMALIZED,      /* DataType */
      8, 8, 8, 0,                  /* Red/Green/Blue/AlphaBits */
      0, 0, 0, 0, 0,               /* Lum/Int/Index/Depth/StencilBits */
      1, 1, 4                      /* BlockWidth/Height,Bytes */
   },
   {
      MESA_FORMAT_XRGB8888_REV,    /* Name */
      "MESA_FORMAT_XRGB8888_REV",  /* StrName */
      GL_RGB,                      /* BaseFormat */
      GL_UNSIGNED_NORMALIZED,      /* DataType */
      8, 8, 8, 0,                  /* Red/Green/Blue/AlphaBits */
      0, 0, 0, 0, 0,               /* Lum/Int/Index/Depth/StencilBits */
      1, 1, 4                      /* BlockWidth/Height,Bytes */
   },
   {
      MESA_FORMAT_RGB888,          /* Name */
      "MESA_FORMAT_RGB888",        /* StrName */
      GL_RGB,                      /* BaseFormat */
      GL_UNSIGNED_NORMALIZED,      /* DataType */
      8, 8, 8, 0,                  /* Red/Green/Blue/AlphaBits */
      0, 0, 0, 0, 0,               /* Lum/Int/Index/Depth/StencilBits */
      1, 1, 3                      /* BlockWidth/Height,Bytes */
   },
   {
      MESA_FORMAT_BGR888,          /* Name */
      "MESA_FORMAT_BGR888",        /* StrName */
      GL_RGB,                      /* BaseFormat */
      GL_UNSIGNED_NORMALIZED,      /* DataType */
      8, 8, 8, 0,                  /* Red/Green/Blue/AlphaBits */
      0, 0, 0, 0, 0,               /* Lum/Int/Index/Depth/StencilBits */
      1, 1, 3                      /* BlockWidth/Height,Bytes */
   },
   {
      MESA_FORMAT_RGB565,          /* Name */
      "MESA_FORMAT_RGB565",        /* StrName */
      GL_RGB,                      /* BaseFormat */
      GL_UNSIGNED_NORMALIZED,      /* DataType */
      5, 6, 5, 0,                  /* Red/Green/Blue/AlphaBits */
      0, 0, 0, 0, 0,               /* Lum/Int/Index/Depth/StencilBits */
      1, 1, 2                      /* BlockWidth/Height,Bytes */
   },
   {
      MESA_FORMAT_RGB565_REV,      /* Name */
      "MESA_FORMAT_RGB565_REV",    /* StrName */
      GL_RGB,                      /* BaseFormat */
      GL_UNSIGNED_NORMALIZED,      /* DataType */
      5, 6, 5, 0,                  /* Red/Green/Blue/AlphaBits */
      0, 0, 0, 0, 0,               /* Lum/Int/Index/Depth/StencilBits */
      1, 1, 2                      /* BlockWidth/Height,Bytes */
   },
   {
      MESA_FORMAT_ARGB4444,        /* Name */
      "MESA_FORMAT_ARGB4444",      /* StrName */
      GL_RGBA,                     /* BaseFormat */
      GL_UNSIGNED_NORMALIZED,      /* DataType */
      4, 4, 4, 4,                  /* Red/Green/Blue/AlphaBits */
      0, 0, 0, 0, 0,               /* Lum/Int/Index/Depth/StencilBits */
      1, 1, 2                      /* BlockWidth/Height,Bytes */
   },
   {
      MESA_FORMAT_ARGB4444_REV,    /* Name */
      "MESA_FORMAT_ARGB4444_REV",  /* StrName */
      GL_RGBA,                     /* BaseFormat */
      GL_UNSIGNED_NORMALIZED,      /* DataType */
      4, 4, 4, 4,                  /* Red/Green/Blue/AlphaBits */
      0, 0, 0, 0, 0,               /* Lum/Int/Index/Depth/StencilBits */
      1, 1, 2                      /* BlockWidth/Height,Bytes */
   },
   {
      MESA_FORMAT_RGBA5551,        /* Name */
      "MESA_FORMAT_RGBA5551",      /* StrName */
      GL_RGBA,                     /* BaseFormat */
      GL_UNSIGNED_NORMALIZED,      /* DataType */
      5, 5, 5, 1,                  /* Red/Green/Blue/AlphaBits */
      0, 0, 0, 0, 0,               /* Lum/Int/Index/Depth/StencilBits */
      1, 1, 2                      /* BlockWidth/Height,Bytes */
   },
   {
      MESA_FORMAT_ARGB1555,        /* Name */
      "MESA_FORMAT_ARGB1555",      /* StrName */
      GL_RGBA,                     /* BaseFormat */
      GL_UNSIGNED_NORMALIZED,      /* DataType */
      5, 5, 5, 1,                  /* Red/Green/Blue/AlphaBits */
      0, 0, 0, 0, 0,               /* Lum/Int/Index/Depth/StencilBits */
      1, 1, 2                      /* BlockWidth/Height,Bytes */
   },
   {
      MESA_FORMAT_ARGB1555_REV,    /* Name */
      "MESA_FORMAT_ARGB1555_REV",  /* StrName */
      GL_RGBA,                     /* BaseFormat */
      GL_UNSIGNED_NORMALIZED,      /* DataType */
      5, 5, 5, 1,                  /* Red/Green/Blue/AlphaBits */
      0, 0, 0, 0, 0,               /* Lum/Int/Index/Depth/StencilBits */
      1, 1, 2                      /* BlockWidth/Height,Bytes */
   },
   {
      MESA_FORMAT_AL44,            /* Name */
      "MESA_FORMAT_AL44",          /* StrName */
      GL_LUMINANCE_ALPHA,          /* BaseFormat */
      GL_UNSIGNED_NORMALIZED,      /* DataType */
      0, 0, 0, 4,                  /* Red/Green/Blue/AlphaBits */
      4, 0, 0, 0, 0,               /* Lum/Int/Index/Depth/StencilBits */
      1, 1, 1                      /* BlockWidth/Height,Bytes */
   },
   {
      MESA_FORMAT_AL88,            /* Name */
      "MESA_FORMAT_AL88",          /* StrName */
      GL_LUMINANCE_ALPHA,          /* BaseFormat */
      GL_UNSIGNED_NORMALIZED,      /* DataType */
      0, 0, 0, 8,                  /* Red/Green/Blue/AlphaBits */
      8, 0, 0, 0, 0,               /* Lum/Int/Index/Depth/StencilBits */
      1, 1, 2                      /* BlockWidth/Height,Bytes */
   },
   {
      MESA_FORMAT_AL88_REV,        /* Name */
      "MESA_FORMAT_AL88_REV",      /* StrName */
      GL_LUMINANCE_ALPHA,          /* BaseFormat */
      GL_UNSIGNED_NORMALIZED,      /* DataType */
      0, 0, 0, 8,                  /* Red/Green/Blue/AlphaBits */
      8, 0, 0, 0, 0,               /* Lum/Int/Index/Depth/StencilBits */
      1, 1, 2                      /* BlockWidth/Height,Bytes */
   },
   {
      MESA_FORMAT_AL1616,          /* Name */
      "MESA_FORMAT_AL1616",        /* StrName */
      GL_LUMINANCE_ALPHA,          /* BaseFormat */
      GL_UNSIGNED_NORMALIZED,      /* DataType */
      0, 0, 0, 16,                 /* Red/Green/Blue/AlphaBits */
      16, 0, 0, 0, 0,              /* Lum/Int/Index/Depth/StencilBits */
      1, 1, 4                      /* BlockWidth/Height,Bytes */
   },
   {
      MESA_FORMAT_AL1616_REV,      /* Name */
      "MESA_FORMAT_AL1616_REV",    /* StrName */
      GL_LUMINANCE_ALPHA,          /* BaseFormat */
      GL_UNSIGNED_NORMALIZED,      /* DataType */
      0, 0, 0, 16,                 /* Red/Green/Blue/AlphaBits */
      16, 0, 0, 0, 0,              /* Lum/Int/Index/Depth/StencilBits */
      1, 1, 4                      /* BlockWidth/Height,Bytes */
   },
   {
      MESA_FORMAT_RGB332,          /* Name */
      "MESA_FORMAT_RGB332",        /* StrName */
      GL_RGB,                      /* BaseFormat */
      GL_UNSIGNED_NORMALIZED,      /* DataType */
      3, 3, 2, 0,                  /* Red/Green/Blue/AlphaBits */
      0, 0, 0, 0, 0,               /* Lum/Int/Index/Depth/StencilBits */
      1, 1, 1                      /* BlockWidth/Height,Bytes */
   },
   {
      MESA_FORMAT_A8,              /* Name */
      "MESA_FORMAT_A8",            /* StrName */
      GL_ALPHA,                    /* BaseFormat */
      GL_UNSIGNED_NORMALIZED,      /* DataType */
      0, 0, 0, 8,                  /* Red/Green/Blue/AlphaBits */
      0, 0, 0, 0, 0,               /* Lum/Int/Index/Depth/StencilBits */
      1, 1, 1                      /* BlockWidth/Height,Bytes */
   },
   {
      MESA_FORMAT_A16,             /* Name */
      "MESA_FORMAT_A16",           /* StrName */
      GL_ALPHA,                    /* BaseFormat */
      GL_UNSIGNED_NORMALIZED,      /* DataType */
      0, 0, 0, 16,                 /* Red/Green/Blue/AlphaBits */
      0, 0, 0, 0, 0,               /* Lum/Int/Index/Depth/StencilBits */
      1, 1, 2                      /* BlockWidth/Height,Bytes */
   },
   {
      MESA_FORMAT_L8,              /* Name */
      "MESA_FORMAT_L8",            /* StrName */
      GL_LUMINANCE,                /* BaseFormat */
      GL_UNSIGNED_NORMALIZED,      /* DataType */
      0, 0, 0, 0,                  /* Red/Green/Blue/AlphaBits */
      8, 0, 0, 0, 0,               /* Lum/Int/Index/Depth/StencilBits */
      1, 1, 1                      /* BlockWidth/Height,Bytes */
   },
   {
      MESA_FORMAT_L16,             /* Name */
      "MESA_FORMAT_L16",           /* StrName */
      GL_LUMINANCE,                /* BaseFormat */
      GL_UNSIGNED_NORMALIZED,      /* DataType */
      0, 0, 0, 0,                  /* Red/Green/Blue/AlphaBits */
      16, 0, 0, 0, 0,              /* Lum/Int/Index/Depth/StencilBits */
      1, 1, 2                      /* BlockWidth/Height,Bytes */
   },
   {
      MESA_FORMAT_I8,              /* Name */
      "MESA_FORMAT_I8",            /* StrName */
      GL_INTENSITY,                /* BaseFormat */
      GL_UNSIGNED_NORMALIZED,      /* DataType */
      0, 0, 0, 0,                  /* Red/Green/Blue/AlphaBits */
      0, 8, 0, 0, 0,               /* Lum/Int/Index/Depth/StencilBits */
      1, 1, 1                      /* BlockWidth/Height,Bytes */
   },
   {
      MESA_FORMAT_I16,             /* Name */
      "MESA_FORMAT_I16",           /* StrName */
      GL_INTENSITY,                /* BaseFormat */
      GL_UNSIGNED_NORMALIZED,      /* DataType */
      0, 0, 0, 0,                  /* Red/Green/Blue/AlphaBits */
      0, 16, 0, 0, 0,              /* Lum/Int/Index/Depth/StencilBits */
      1, 1, 2                      /* BlockWidth/Height,Bytes */
   },
   {
      MESA_FORMAT_YCBCR,           /* Name */
      "MESA_FORMAT_YCBCR",         /* StrName */
      GL_YCBCR_MESA,               /* BaseFormat */
      GL_UNSIGNED_NORMALIZED,      /* DataType */
      0, 0, 0, 0,                  /* Red/Green/Blue/AlphaBits */
      0, 0, 0, 0, 0,               /* Lum/Int/Index/Depth/StencilBits */
      1, 1, 2                      /* BlockWidth/Height,Bytes */
   },
   {
      MESA_FORMAT_YCBCR_REV,       /* Name */
      "MESA_FORMAT_YCBCR_REV",     /* StrName */
      GL_YCBCR_MESA,               /* BaseFormat */
      GL_UNSIGNED_NORMALIZED,      /* DataType */
      0, 0, 0, 0,                  /* Red/Green/Blue/AlphaBits */
      0, 0, 0, 0, 0,               /* Lum/Int/Index/Depth/StencilBits */
      1, 1, 2                      /* BlockWidth/Height,Bytes */
   },
   {
      MESA_FORMAT_R8,
      "MESA_FORMAT_R8",
      GL_RED,
      GL_UNSIGNED_NORMALIZED,
      8, 0, 0, 0,
      0, 0, 0, 0, 0,
      1, 1, 1
   },
   {
      MESA_FORMAT_GR88,
      "MESA_FORMAT_GR88",
      GL_RG,
      GL_UNSIGNED_NORMALIZED,
      8, 8, 0, 0,
      0, 0, 0, 0, 0,
      1, 1, 2
   },
   {
      MESA_FORMAT_RG88,
      "MESA_FORMAT_RG88",
      GL_RG,
      GL_UNSIGNED_NORMALIZED,
      8, 8, 0, 0,
      0, 0, 0, 0, 0,
      1, 1, 2
   },
   {
      MESA_FORMAT_R16,
      "MESA_FORMAT_R16",
      GL_RED,
      GL_UNSIGNED_NORMALIZED,
      16, 0, 0, 0,
      0, 0, 0, 0, 0,
      1, 1, 2
   },
   {
      MESA_FORMAT_RG1616,
      "MESA_FORMAT_RG1616",
      GL_RG,
      GL_UNSIGNED_NORMALIZED,
      16, 16, 0, 0,
      0, 0, 0, 0, 0,
      1, 1, 4
   },
   {
      MESA_FORMAT_RG1616_REV,
      "MESA_FORMAT_RG1616_REV",
      GL_RG,
      GL_UNSIGNED_NORMALIZED,
      16, 16, 0, 0,
      0, 0, 0, 0, 0,
      1, 1, 4
   },
   {
      MESA_FORMAT_ARGB2101010,
      "MESA_FORMAT_ARGB2101010",
      GL_RGBA,
      GL_UNSIGNED_NORMALIZED,
      10, 10, 10, 2,
      0, 0, 0, 0, 0,
      1, 1, 4
   },
   {
      MESA_FORMAT_Z24_S8,          /* Name */
      "MESA_FORMAT_Z24_S8",        /* StrName */
      GL_DEPTH_STENCIL,            /* BaseFormat */
      GL_UNSIGNED_NORMALIZED,      /* DataType */
      0, 0, 0, 0,                  /* Red/Green/Blue/AlphaBits */
      0, 0, 0, 24, 8,              /* Lum/Int/Index/Depth/StencilBits */
      1, 1, 4                      /* BlockWidth/Height,Bytes */
   },
   {
      MESA_FORMAT_S8_Z24,          /* Name */
      "MESA_FORMAT_S8_Z24",        /* StrName */
      GL_DEPTH_STENCIL,            /* BaseFormat */
      GL_UNSIGNED_NORMALIZED,      /* DataType */
      0, 0, 0, 0,                  /* Red/Green/Blue/AlphaBits */
      0, 0, 0, 24, 8,              /* Lum/Int/Index/Depth/StencilBits */
      1, 1, 4                      /* BlockWidth/Height,Bytes */
   },
   {
      MESA_FORMAT_Z16,             /* Name */
      "MESA_FORMAT_Z16",           /* StrName */
      GL_DEPTH_COMPONENT,          /* BaseFormat */
      GL_UNSIGNED_NORMALIZED,      /* DataType */
      0, 0, 0, 0,                  /* Red/Green/Blue/AlphaBits */
      0, 0, 0, 16, 0,              /* Lum/Int/Index/Depth/StencilBits */
      1, 1, 2                      /* BlockWidth/Height,Bytes */
   },
   {
      MESA_FORMAT_X8_Z24,          /* Name */
      "MESA_FORMAT_X8_Z24",        /* StrName */
      GL_DEPTH_COMPONENT,          /* BaseFormat */
      GL_UNSIGNED_NORMALIZED,      /* DataType */
      0, 0, 0, 0,                  /* Red/Green/Blue/AlphaBits */
      0, 0, 0, 24, 0,              /* Lum/Int/Index/Depth/StencilBits */
      1, 1, 4                      /* BlockWidth/Height,Bytes */
   },
   {
      MESA_FORMAT_Z24_X8,          /* Name */
      "MESA_FORMAT_Z24_X8",        /* StrName */
      GL_DEPTH_COMPONENT,          /* BaseFormat */
      GL_UNSIGNED_NORMALIZED,      /* DataType */
      0, 0, 0, 0,                  /* Red/Green/Blue/AlphaBits */
      0, 0, 0, 24, 0,              /* Lum/Int/Index/Depth/StencilBits */
      1, 1, 4                      /* BlockWidth/Height,Bytes */
   },
   {
      MESA_FORMAT_Z32,             /* Name */
      "MESA_FORMAT_Z32",           /* StrName */
      GL_DEPTH_COMPONENT,          /* BaseFormat */
      GL_UNSIGNED_NORMALIZED,      /* DataType */
      0, 0, 0, 0,                  /* Red/Green/Blue/AlphaBits */
      0, 0, 0, 32, 0,              /* Lum/Int/Index/Depth/StencilBits */
      1, 1, 4                      /* BlockWidth/Height,Bytes */
   },
   {
      MESA_FORMAT_S8,              /* Name */
      "MESA_FORMAT_S8",            /* StrName */
      GL_STENCIL_INDEX,            /* BaseFormat */
      GL_UNSIGNED_INT,             /* DataType */
      0, 0, 0, 0,                  /* Red/Green/Blue/AlphaBits */
      0, 0, 0, 0, 8,               /* Lum/Int/Index/Depth/StencilBits */
      1, 1, 1                      /* BlockWidth/Height,Bytes */
   },
   {
      MESA_FORMAT_SRGB8,
      "MESA_FORMAT_SRGB8",
      GL_RGB,
      GL_UNSIGNED_NORMALIZED,
      8, 8, 8, 0,
      0, 0, 0, 0, 0,
      1, 1, 3
   },
   {
      MESA_FORMAT_SRGBA8,
      "MESA_FORMAT_SRGBA8",
      GL_RGBA,
      GL_UNSIGNED_NORMALIZED,    
      8, 8, 8, 8,
      0, 0, 0, 0, 0,
      1, 1, 4
   },
   {
      MESA_FORMAT_SARGB8,
      "MESA_FORMAT_SARGB8",
      GL_RGBA,
      GL_UNSIGNED_NORMALIZED,    
      8, 8, 8, 8,
      0, 0, 0, 0, 0,
      1, 1, 4
   },
   {
      MESA_FORMAT_SL8,
      "MESA_FORMAT_SL8",
      GL_LUMINANCE,
      GL_UNSIGNED_NORMALIZED,    
      0, 0, 0, 0,
      8, 0, 0, 0, 0,
      1, 1, 1
   },
   {
      MESA_FORMAT_SLA8,
      "MESA_FORMAT_SLA8",
      GL_LUMINANCE_ALPHA,
      GL_UNSIGNED_NORMALIZED,    
      0, 0, 0, 8,
      8, 0, 0, 0, 0,
      1, 1, 2
   },
   {
      MESA_FORMAT_SRGB_DXT1,       /* Name */
      "MESA_FORMAT_SRGB_DXT1",     /* StrName */
      GL_RGB,                      /* BaseFormat */
      GL_UNSIGNED_NORMALIZED,      /* DataType */
      4, 4, 4, 0,                  /* approx Red/Green/Blue/AlphaBits */
      0, 0, 0, 0, 0,               /* Lum/Int/Index/Depth/StencilBits */
      4, 4, 8                      /* 8 bytes per 4x4 block */
   },
   {
      MESA_FORMAT_SRGBA_DXT1,
      "MESA_FORMAT_SRGBA_DXT1",
      GL_RGBA,
      GL_UNSIGNED_NORMALIZED,
      4, 4, 4, 4,
      0, 0, 0, 0, 0,
      4, 4, 8                      /* 8 bytes per 4x4 block */
   },
   {
      MESA_FORMAT_SRGBA_DXT3,
      "MESA_FORMAT_SRGBA_DXT3",
      GL_RGBA,
      GL_UNSIGNED_NORMALIZED,
      4, 4, 4, 4,
      0, 0, 0, 0, 0,
      4, 4, 16                     /* 16 bytes per 4x4 block */
   },
   {
      MESA_FORMAT_SRGBA_DXT5,
      "MESA_FORMAT_SRGBA_DXT5",
      GL_RGBA,
      GL_UNSIGNED_NORMALIZED,
      4, 4, 4, 4,
      0, 0, 0, 0, 0,
      4, 4, 16                     /* 16 bytes per 4x4 block */
   },

   {
      MESA_FORMAT_RGB_FXT1,
      "MESA_FORMAT_RGB_FXT1",
      GL_RGB,
      GL_UNSIGNED_NORMALIZED,
      4, 4, 4, 0,                  /* approx Red/Green/BlueBits */
      0, 0, 0, 0, 0,
      8, 4, 16                     /* 16 bytes per 8x4 block */
   },
   {
      MESA_FORMAT_RGBA_FXT1,
      "MESA_FORMAT_RGBA_FXT1",
      GL_RGBA,
      GL_UNSIGNED_NORMALIZED,
      4, 4, 4, 1,                  /* approx Red/Green/Blue/AlphaBits */
      0, 0, 0, 0, 0,
      8, 4, 16                     /* 16 bytes per 8x4 block */
   },

   {
      MESA_FORMAT_RGB_DXT1,        /* Name */
      "MESA_FORMAT_RGB_DXT1",      /* StrName */
      GL_RGB,                      /* BaseFormat */
      GL_UNSIGNED_NORMALIZED,      /* DataType */
      4, 4, 4, 0,                  /* approx Red/Green/Blue/AlphaBits */
      0, 0, 0, 0, 0,               /* Lum/Int/Index/Depth/StencilBits */
      4, 4, 8                      /* 8 bytes per 4x4 block */
   },
   {
      MESA_FORMAT_RGBA_DXT1,
      "MESA_FORMAT_RGBA_DXT1",
      GL_RGBA,
      GL_UNSIGNED_NORMALIZED,    
      4, 4, 4, 4,
      0, 0, 0, 0, 0,
      4, 4, 8                      /* 8 bytes per 4x4 block */
   },
   {
      MESA_FORMAT_RGBA_DXT3,
      "MESA_FORMAT_RGBA_DXT3",
      GL_RGBA,
      GL_UNSIGNED_NORMALIZED,    
      4, 4, 4, 4,
      0, 0, 0, 0, 0,
      4, 4, 16                     /* 16 bytes per 4x4 block */
   },
   {
      MESA_FORMAT_RGBA_DXT5,
      "MESA_FORMAT_RGBA_DXT5",
      GL_RGBA,
      GL_UNSIGNED_NORMALIZED,    
      4, 4, 4, 4,
      0, 0, 0, 0, 0,
      4, 4, 16                     /* 16 bytes per 4x4 block */
   },
   {
      MESA_FORMAT_RGBA_FLOAT32,
      "MESA_FORMAT_RGBA_FLOAT32",
      GL_RGBA,
      GL_FLOAT,
      32, 32, 32, 32,
      0, 0, 0, 0, 0,
      1, 1, 16
   },
   {
      MESA_FORMAT_RGBA_FLOAT16,
      "MESA_FORMAT_RGBA_FLOAT16",
      GL_RGBA,
      GL_FLOAT,
      16, 16, 16, 16,
      0, 0, 0, 0, 0,
      1, 1, 8
   },
   {
      MESA_FORMAT_RGB_FLOAT32,
      "MESA_FORMAT_RGB_FLOAT32",
      GL_RGB,
      GL_FLOAT,
      32, 32, 32, 0,
      0, 0, 0, 0, 0,
      1, 1, 12
   },
   {
      MESA_FORMAT_RGB_FLOAT16,
      "MESA_FORMAT_RGB_FLOAT16",
      GL_RGB,
      GL_FLOAT,
      16, 16, 16, 0,
      0, 0, 0, 0, 0,
      1, 1, 6
   },
   {
      MESA_FORMAT_ALPHA_FLOAT32,
      "MESA_FORMAT_ALPHA_FLOAT32",
      GL_ALPHA,
      GL_FLOAT,
      0, 0, 0, 32,
      0, 0, 0, 0, 0,
      1, 1, 4
   },
   {
      MESA_FORMAT_ALPHA_FLOAT16,
      "MESA_FORMAT_ALPHA_FLOAT16",
      GL_ALPHA,
      GL_FLOAT,
      0, 0, 0, 16,
      0, 0, 0, 0, 0,
      1, 1, 2
   },
   {
      MESA_FORMAT_LUMINANCE_FLOAT32,
      "MESA_FORMAT_LUMINANCE_FLOAT32",
      GL_LUMINANCE,
      GL_FLOAT,
      0, 0, 0, 0,
      32, 0, 0, 0, 0,
      1, 1, 4
   },
   {
      MESA_FORMAT_LUMINANCE_FLOAT16,
      "MESA_FORMAT_LUMINANCE_FLOAT16",
      GL_LUMINANCE,
      GL_FLOAT,
      0, 0, 0, 0,
      16, 0, 0, 0, 0,
      1, 1, 2
   },
   {
      MESA_FORMAT_LUMINANCE_ALPHA_FLOAT32,
      "MESA_FORMAT_LUMINANCE_ALPHA_FLOAT32",
      GL_LUMINANCE_ALPHA,
      GL_FLOAT,
      0, 0, 0, 32,
      32, 0, 0, 0, 0,
      1, 1, 8
   },
   {
      MESA_FORMAT_LUMINANCE_ALPHA_FLOAT16,
      "MESA_FORMAT_LUMINANCE_ALPHA_FLOAT16",
      GL_LUMINANCE_ALPHA,
      GL_FLOAT,
      0, 0, 0, 16,
      16, 0, 0, 0, 0,
      1, 1, 4
   },
   {
      MESA_FORMAT_INTENSITY_FLOAT32,
      "MESA_FORMAT_INTENSITY_FLOAT32",
      GL_INTENSITY,
      GL_FLOAT,
      0, 0, 0, 0,
      0, 32, 0, 0, 0,
      1, 1, 4
   },
   {
      MESA_FORMAT_INTENSITY_FLOAT16,
      "MESA_FORMAT_INTENSITY_FLOAT16",
      GL_INTENSITY,
      GL_FLOAT,
      0, 0, 0, 0,
      0, 16, 0, 0, 0,
      1, 1, 2
   },
   {
      MESA_FORMAT_R_FLOAT32,
      "MESA_FORMAT_R_FLOAT32",
      GL_RED,
      GL_FLOAT,
      32, 0, 0, 0,
      0, 0, 0, 0, 0,
      1, 1, 4
   },
   {
      MESA_FORMAT_R_FLOAT16,
      "MESA_FORMAT_R_FLOAT16",
      GL_RED,
      GL_FLOAT,
      16, 0, 0, 0,
      0, 0, 0, 0, 0,
      1, 1, 2
   },
   {
      MESA_FORMAT_RG_FLOAT32,
      "MESA_FORMAT_RG_FLOAT32",
      GL_RG,
      GL_FLOAT,
      32, 32, 0, 0,
      0, 0, 0, 0, 0,
      1, 1, 8
   },
   {
      MESA_FORMAT_RG_FLOAT16,
      "MESA_FORMAT_RG_FLOAT16",
      GL_RG,
      GL_FLOAT,
      16, 16, 0, 0,
      0, 0, 0, 0, 0,
      1, 1, 4
   },

   /* unnormalized signed int formats */
   /* unnormalized unsigned int formats */
   {
      MESA_FORMAT_ALPHA_UINT8,
      "MESA_FORMAT_ALPHA_UINT8",
      GL_ALPHA,
      GL_UNSIGNED_INT,
      0, 0, 0, 8,
      0, 0, 0, 0, 0,
      1, 1, 1
   },
   {
      MESA_FORMAT_ALPHA_UINT16,
      "MESA_FORMAT_ALPHA_UINT16",
      GL_ALPHA,
      GL_UNSIGNED_INT,
      0, 0, 0, 16,
      0, 0, 0, 0, 0,
      1, 1, 2
   },
   {
      MESA_FORMAT_ALPHA_UINT32,
      "MESA_FORMAT_ALPHA_UINT32",
      GL_ALPHA,
      GL_UNSIGNED_INT,
      0, 0, 0, 32,
      0, 0, 0, 0, 0,
      1, 1, 4
   },
   {
      MESA_FORMAT_ALPHA_INT8,
      "MESA_FORMAT_ALPHA_INT8",
      GL_ALPHA,
      GL_INT,
      0, 0, 0, 8,
      0, 0, 0, 0, 0,
      1, 1, 1
   },
   {
      MESA_FORMAT_ALPHA_INT16,
      "MESA_FORMAT_ALPHA_INT16",
      GL_ALPHA,
      GL_INT,
      0, 0, 0, 16,
      0, 0, 0, 0, 0,
      1, 1, 2
   },
   {
      MESA_FORMAT_ALPHA_INT32,
      "MESA_FORMAT_ALPHA_INT32",
      GL_ALPHA,
      GL_INT,
      0, 0, 0, 32,
      0, 0, 0, 0, 0,
      1, 1, 4
   },
   {
      MESA_FORMAT_INTENSITY_UINT8,
      "MESA_FORMAT_INTENSITY_UINT8",
      GL_INTENSITY,
      GL_UNSIGNED_INT,
      0, 0, 0, 0,
      0, 8, 0, 0, 0,
      1, 1, 1
   },
   {
      MESA_FORMAT_INTENSITY_UINT16,
      "MESA_FORMAT_INTENSITY_UINT16",
      GL_INTENSITY,
      GL_UNSIGNED_INT,
      0, 0, 0, 0,
      0, 16, 0, 0, 0,
      1, 1, 2
   },
   {
      MESA_FORMAT_INTENSITY_UINT32,
      "MESA_FORMAT_INTENSITY_UINT32",
      GL_INTENSITY,
      GL_UNSIGNED_INT,
      0, 0, 0, 0,
      0, 32, 0, 0, 0,
      1, 1, 4
   },
   {
      MESA_FORMAT_INTENSITY_INT8,
      "MESA_FORMAT_INTENSITY_INT8",
      GL_INTENSITY,
      GL_INT,
      0, 0, 0, 0,
      0, 8, 0, 0, 0,
      1, 1, 1
   },
   {
      MESA_FORMAT_INTENSITY_INT16,
      "MESA_FORMAT_INTENSITY_INT16",
      GL_INTENSITY,
      GL_INT,
      0, 0, 0, 0,
      0, 16, 0, 0, 0,
      1, 1, 2
   },
   {
      MESA_FORMAT_INTENSITY_INT32,
      "MESA_FORMAT_INTENSITY_INT32",
      GL_INTENSITY,
      GL_INT,
      0, 0, 0, 0,
      0, 32, 0, 0, 0,
      1, 1, 4
   },
   {
      MESA_FORMAT_LUMINANCE_UINT8,
      "MESA_FORMAT_LUMINANCE_UINT8",
      GL_LUMINANCE,
      GL_UNSIGNED_INT,
      0, 0, 0, 0,
      8, 0, 0, 0, 0,
      1, 1, 1
   },
   {
      MESA_FORMAT_LUMINANCE_UINT16,
      "MESA_FORMAT_LUMINANCE_UINT16",
      GL_LUMINANCE,
      GL_UNSIGNED_INT,
      0, 0, 0, 0,
      16, 0, 0, 0, 0,
      1, 1, 2
   },
   {
      MESA_FORMAT_LUMINANCE_UINT32,
      "MESA_FORMAT_LUMINANCE_UINT32",
      GL_LUMINANCE,
      GL_UNSIGNED_INT,
      0, 0, 0, 0,
      32, 0, 0, 0, 0,
      1, 1, 4
   },
   {
      MESA_FORMAT_LUMINANCE_INT8,
      "MESA_FORMAT_LUMINANCE_INT8",
      GL_LUMINANCE,
      GL_INT,
      0, 0, 0, 0,
      8, 0, 0, 0, 0,
      1, 1, 1
   },
   {
      MESA_FORMAT_LUMINANCE_INT16,
      "MESA_FORMAT_LUMINANCE_INT16",
      GL_LUMINANCE,
      GL_INT,
      0, 0, 0, 0,
      16, 0, 0, 0, 0,
      1, 1, 2
   },
   {
      MESA_FORMAT_LUMINANCE_INT32,
      "MESA_FORMAT_LUMINANCE_INT32",
      GL_LUMINANCE,
      GL_INT,
      0, 0, 0, 0,
      32, 0, 0, 0, 0,
      1, 1, 4
   },
   {
      MESA_FORMAT_LUMINANCE_ALPHA_UINT8,
      "MESA_FORMAT_LUMINANCE_ALPHA_UINT8",
      GL_LUMINANCE_ALPHA,
      GL_UNSIGNED_INT,
      0, 0, 0, 8,
      8, 0, 0, 0, 0,
      1, 1, 2
   },
   {
      MESA_FORMAT_LUMINANCE_ALPHA_UINT16,
      "MESA_FORMAT_LUMINANCE_ALPHA_UINT16",
      GL_LUMINANCE_ALPHA,
      GL_UNSIGNED_INT,
      0, 0, 0, 16,
      16, 0, 0, 0, 0,
      1, 1, 4
   },
   {
      MESA_FORMAT_LUMINANCE_ALPHA_UINT32,
      "MESA_FORMAT_LUMINANCE_ALPHA_UINT32",
      GL_LUMINANCE_ALPHA,
      GL_UNSIGNED_INT,
      0, 0, 0, 32,
      32, 0, 0, 0, 0,
      1, 1, 8
   },
   {
      MESA_FORMAT_LUMINANCE_ALPHA_INT8,
      "MESA_FORMAT_LUMINANCE_ALPHA_INT8",
      GL_LUMINANCE_ALPHA,
      GL_INT,
      0, 0, 0, 8,
      8, 0, 0, 0, 0,
      1, 1, 2
   },
   {
      MESA_FORMAT_LUMINANCE_ALPHA_INT16,
      "MESA_FORMAT_LUMINANCE_ALPHA_INT16",
      GL_LUMINANCE_ALPHA,
      GL_INT,
      0, 0, 0, 16,
      16, 0, 0, 0, 0,
      1, 1, 4
   },
   {
      MESA_FORMAT_LUMINANCE_ALPHA_INT32,
      "MESA_FORMAT_LUMINANCE_ALPHA_INT32",
      GL_LUMINANCE_ALPHA,
      GL_INT,
      0, 0, 0, 32,
      32, 0, 0, 0, 0,
      1, 1, 8
   },

   {
      MESA_FORMAT_R_INT8,
      "MESA_FORMAT_R_INT8",
      GL_RED,
      GL_INT,
      8, 0, 0, 0,
      0, 0, 0, 0, 0,
      1, 1, 1
   },
   {
      MESA_FORMAT_RG_INT8,
      "MESA_FORMAT_RG_INT8",
      GL_RG,
      GL_INT,
      8, 8, 0, 0,
      0, 0, 0, 0, 0,
      1, 1, 2
   },
   {
      MESA_FORMAT_RGB_INT8,
      "MESA_FORMAT_RGB_INT8",
      GL_RGB,
      GL_INT,
      8, 8, 8, 0,
      0, 0, 0, 0, 0,
      1, 1, 3
   },
   {
      MESA_FORMAT_RGBA_INT8,
      "MESA_FORMAT_RGBA_INT8",
      GL_RGBA,
      GL_INT,
      8, 8, 8, 8,
      0, 0, 0, 0, 0,
      1, 1, 4
   },
   {
      MESA_FORMAT_R_INT16,
      "MESA_FORMAT_R_INT16",
      GL_RED,
      GL_INT,
      16, 0, 0, 0,
      0, 0, 0, 0, 0,
      1, 1, 2
   },
   {
      MESA_FORMAT_RG_INT16,
      "MESA_FORMAT_RG_INT16",
      GL_RG,
      GL_INT,
      16, 16, 0, 0,
      0, 0, 0, 0, 0,
      1, 1, 4
   },
   {
      MESA_FORMAT_RGB_INT16,
      "MESA_FORMAT_RGB_INT16",
      GL_RGB,
      GL_INT,
      16, 16, 16, 0,
      0, 0, 0, 0, 0,
      1, 1, 6
   },
   {
      MESA_FORMAT_RGBA_INT16,
      "MESA_FORMAT_RGBA_INT16",
      GL_RGBA,
      GL_INT,
      16, 16, 16, 16,
      0, 0, 0, 0, 0,
      1, 1, 8
   },
   {
      MESA_FORMAT_R_INT32,
      "MESA_FORMAT_R_INT32",
      GL_RED,
      GL_INT,
      32, 0, 0, 0,
      0, 0, 0, 0, 0,
      1, 1, 4
   },
   {
      MESA_FORMAT_RG_INT32,
      "MESA_FORMAT_RG_INT32",
      GL_RG,
      GL_INT,
      32, 32, 0, 0,
      0, 0, 0, 0, 0,
      1, 1, 8
   },
   {
      MESA_FORMAT_RGB_INT32,
      "MESA_FORMAT_RGB_INT32",
      GL_RGB,
      GL_INT,
      32, 32, 32, 0,
      0, 0, 0, 0, 0,
      1, 1, 12
   },
   {
      MESA_FORMAT_RGBA_INT32,
      "MESA_FORMAT_RGBA_INT32",
      GL_RGBA,
      GL_INT,
      32, 32, 32, 32,
      0, 0, 0, 0, 0,
      1, 1, 16
   },
   {
      MESA_FORMAT_R_UINT8,
      "MESA_FORMAT_R_UINT8",
      GL_RED,
      GL_UNSIGNED_INT,
      8, 0, 0, 0,
      0, 0, 0, 0, 0,
      1, 1, 1
   },
   {
      MESA_FORMAT_RG_UINT8,
      "MESA_FORMAT_RG_UINT8",
      GL_RG,
      GL_UNSIGNED_INT,
      8, 8, 0, 0,
      0, 0, 0, 0, 0,
      1, 1, 2
   },
   {
      MESA_FORMAT_RGB_UINT8,
      "MESA_FORMAT_RGB_UINT8",
      GL_RGB,
      GL_UNSIGNED_INT,
      8, 8, 8, 0,
      0, 0, 0, 0, 0,
      1, 1, 3
   },
   {
      MESA_FORMAT_RGBA_UINT8,
      "MESA_FORMAT_RGBA_UINT8",
      GL_RGBA,
      GL_UNSIGNED_INT,
      8, 8, 8, 8,
      0, 0, 0, 0, 0,
      1, 1, 4
   },
   {
      MESA_FORMAT_R_UINT16,
      "MESA_FORMAT_R_UINT16",
      GL_RED,
      GL_UNSIGNED_INT,
      16, 0, 0, 0,
      0, 0, 0, 0, 0,
      1, 1, 2
   },
   {
      MESA_FORMAT_RG_UINT16,
      "MESA_FORMAT_RG_UINT16",
      GL_RG,
      GL_UNSIGNED_INT,
      16, 16, 0, 0,
      0, 0, 0, 0, 0,
      1, 1, 4
   },
   {
      MESA_FORMAT_RGB_UINT16,
      "MESA_FORMAT_RGB_UINT16",
      GL_RGB,
      GL_UNSIGNED_INT,
      16, 16, 16, 0,
      0, 0, 0, 0, 0,
      1, 1, 6
   },
   {
      MESA_FORMAT_RGBA_UINT16,
      "MESA_FORMAT_RGBA_UINT16",
      GL_RGBA,
      GL_UNSIGNED_INT,
      16, 16, 16, 16,
      0, 0, 0, 0, 0,
      1, 1, 8
   },
   {
      MESA_FORMAT_R_UINT32,
      "MESA_FORMAT_R_UINT32",
      GL_RED,
      GL_UNSIGNED_INT,
      32, 0, 0, 0,
      0, 0, 0, 0, 0,
      1, 1, 4
   },
   {
      MESA_FORMAT_RG_UINT32,
      "MESA_FORMAT_RG_UINT32",
      GL_RG,
      GL_UNSIGNED_INT,
      32, 32, 0, 0,
      0, 0, 0, 0, 0,
      1, 1, 8
   },
   {
      MESA_FORMAT_RGB_UINT32,
      "MESA_FORMAT_RGB_UINT32",
      GL_RGB,
      GL_UNSIGNED_INT,
      32, 32, 32, 0,
      0, 0, 0, 0, 0,
      1, 1, 12
   },
   {
      MESA_FORMAT_RGBA_UINT32,
      "MESA_FORMAT_RGBA_UINT32",
      GL_RGBA,
      GL_UNSIGNED_INT,
      32, 32, 32, 32,
      0, 0, 0, 0, 0,
      1, 1, 16
   },


   {
      MESA_FORMAT_DUDV8,
      "MESA_FORMAT_DUDV8",
      GL_DUDV_ATI,
      GL_SIGNED_NORMALIZED,
      0, 0, 0, 0,
      0, 0, 0, 0, 0,
      1, 1, 2
   },

   /* Signed 8 bits / channel */
   {
      MESA_FORMAT_SIGNED_R8,        /* Name */
      "MESA_FORMAT_SIGNED_R8",      /* StrName */
      GL_RED,                       /* BaseFormat */
      GL_SIGNED_NORMALIZED,         /* DataType */
      8, 0, 0, 0,                   /* Red/Green/Blue/AlphaBits */
      0, 0, 0, 0, 0,                /* Lum/Int/Index/Depth/StencilBits */
      1, 1, 1                       /* BlockWidth/Height,Bytes */
   },
   {
      MESA_FORMAT_SIGNED_RG88_REV,
      "MESA_FORMAT_SIGNED_RG88_REV",
      GL_RG,
      GL_SIGNED_NORMALIZED,
      8, 8, 0, 0,
      0, 0, 0, 0, 0,
      1, 1, 2
   },
   {
      MESA_FORMAT_SIGNED_RGBX8888,
      "MESA_FORMAT_SIGNED_RGBX8888",
      GL_RGB,
      GL_SIGNED_NORMALIZED,
      8, 8, 8, 0,
      0, 0, 0, 0, 0,
      1, 1, 4                       /* 4 bpp, but no alpha */
   },
   {
      MESA_FORMAT_SIGNED_RGBA8888,
      "MESA_FORMAT_SIGNED_RGBA8888",
      GL_RGBA,
      GL_SIGNED_NORMALIZED,
      8, 8, 8, 8,
      0, 0, 0, 0, 0,
      1, 1, 4
   },
   {
      MESA_FORMAT_SIGNED_RGBA8888_REV,
      "MESA_FORMAT_SIGNED_RGBA8888_REV",
      GL_RGBA,
      GL_SIGNED_NORMALIZED,
      8, 8, 8, 8,
      0, 0, 0, 0, 0,
      1, 1, 4
   },

   /* Signed 16 bits / channel */
   {
      MESA_FORMAT_SIGNED_R16,
      "MESA_FORMAT_SIGNED_R16",
      GL_RED,
      GL_SIGNED_NORMALIZED,
      16, 0, 0, 0,
      0, 0, 0, 0, 0,
      1, 1, 2
   },
   {
      MESA_FORMAT_SIGNED_GR1616,
      "MESA_FORMAT_SIGNED_GR1616",
      GL_RG,
      GL_SIGNED_NORMALIZED,
      16, 16, 0, 0,
      0, 0, 0, 0, 0,
      1, 1, 4
   },
   {
      MESA_FORMAT_SIGNED_RGB_16,
      "MESA_FORMAT_SIGNED_RGB_16",
      GL_RGB,
      GL_SIGNED_NORMALIZED,
      16, 16, 16, 0,
      0, 0, 0, 0, 0,
      1, 1, 6
   },
   {
      MESA_FORMAT_SIGNED_RGBA_16,
      "MESA_FORMAT_SIGNED_RGBA_16",
      GL_RGBA,
      GL_SIGNED_NORMALIZED,
      16, 16, 16, 16,
      0, 0, 0, 0, 0,
      1, 1, 8
   },
   {
      MESA_FORMAT_RGBA_16,
      "MESA_FORMAT_RGBA_16",
      GL_RGBA,
      GL_UNSIGNED_NORMALIZED,
      16, 16, 16, 16,
      0, 0, 0, 0, 0,
      1, 1, 8
   },
   {
     MESA_FORMAT_RED_RGTC1,
     "MESA_FORMAT_RED_RGTC1",
     GL_RED,
     GL_UNSIGNED_NORMALIZED,
     4, 0, 0, 0,
     0, 0, 0, 0, 0,
     4, 4, 8                     /* 8 bytes per 4x4 block */
   },
   {
     MESA_FORMAT_SIGNED_RED_RGTC1,
     "MESA_FORMAT_SIGNED_RED_RGTC1",
     GL_RED,
     GL_SIGNED_NORMALIZED,
     4, 0, 0, 0,
     0, 0, 0, 0, 0,
     4, 4, 8                     /* 8 bytes per 4x4 block */
   },
   {
     MESA_FORMAT_RG_RGTC2,
     "MESA_FORMAT_RG_RGTC2",
     GL_RG,
     GL_UNSIGNED_NORMALIZED,
     4, 4, 0, 0,
     0, 0, 0, 0, 0,
     4, 4, 16                     /* 16 bytes per 4x4 block */
   },
   {
     MESA_FORMAT_SIGNED_RG_RGTC2,
     "MESA_FORMAT_SIGNED_RG_RGTC2",
     GL_RG,
     GL_SIGNED_NORMALIZED,
     4, 4, 0, 0,
     0, 0, 0, 0, 0,
     4, 4, 16                     /* 16 bytes per 4x4 block */
   },
   {
     MESA_FORMAT_L_LATC1,
     "MESA_FORMAT_L_LATC1",
     GL_LUMINANCE,
     GL_UNSIGNED_NORMALIZED,
     0, 0, 0, 0,
     4, 0, 0, 0, 0,
     4, 4, 8                     /* 8 bytes per 4x4 block */
   },
   {
     MESA_FORMAT_SIGNED_L_LATC1,
     "MESA_FORMAT_SIGNED_L_LATC1",
     GL_LUMINANCE,
     GL_SIGNED_NORMALIZED,
     0, 0, 0, 0,
     4, 0, 0, 0, 0,
     4, 4, 8                     /* 8 bytes per 4x4 block */
   },
   {
     MESA_FORMAT_LA_LATC2,
     "MESA_FORMAT_LA_LATC2",
     GL_LUMINANCE_ALPHA,
     GL_UNSIGNED_NORMALIZED,
     0, 0, 0, 4,
     4, 0, 0, 0, 0,
     4, 4, 16                     /* 16 bytes per 4x4 block */
   },
   {
     MESA_FORMAT_SIGNED_LA_LATC2,
     "MESA_FORMAT_SIGNED_LA_LATC2",
     GL_LUMINANCE_ALPHA,
     GL_SIGNED_NORMALIZED,
     0, 0, 0, 4,
     4, 0, 0, 0, 0,
     4, 4, 16                     /* 16 bytes per 4x4 block */
   },

   {
      MESA_FORMAT_ETC1_RGB8,
      "MESA_FORMAT_ETC1_RGB8",
      GL_RGB,
      GL_UNSIGNED_NORMALIZED,
      8, 8, 8, 0,
      0, 0, 0, 0, 0,
      4, 4, 8                     /* 8 bytes per 4x4 block */
   },

   /* Signed formats from EXT_texture_snorm that are not in GL3.1 */
   {
      MESA_FORMAT_SIGNED_A8,
      "MESA_FORMAT_SIGNED_A8",
      GL_ALPHA,
      GL_SIGNED_NORMALIZED,
      0, 0, 0, 8,
      0, 0, 0, 0, 0,
      1, 1, 1
   },
   {
      MESA_FORMAT_SIGNED_L8,
      "MESA_FORMAT_SIGNED_L8",
      GL_LUMINANCE,
      GL_SIGNED_NORMALIZED,
      0, 0, 0, 0,
      8, 0, 0, 0, 0,
      1, 1, 1
   },
   {
      MESA_FORMAT_SIGNED_AL88,
      "MESA_FORMAT_SIGNED_AL88",
      GL_LUMINANCE_ALPHA,
      GL_SIGNED_NORMALIZED,
      0, 0, 0, 8,
      8, 0, 0, 0, 0,
      1, 1, 2
   },
   {
      MESA_FORMAT_SIGNED_I8,
      "MESA_FORMAT_SIGNED_I8",
      GL_INTENSITY,
      GL_SIGNED_NORMALIZED,
      0, 0, 0, 0,
      0, 8, 0, 0, 0,
      1, 1, 1
   },
   {
      MESA_FORMAT_SIGNED_A16,
      "MESA_FORMAT_SIGNED_A16",
      GL_ALPHA,
      GL_SIGNED_NORMALIZED,
      0, 0, 0, 16,
      0, 0, 0, 0, 0,
      1, 1, 2
   },
   {
      MESA_FORMAT_SIGNED_L16,
      "MESA_FORMAT_SIGNED_L16",
      GL_LUMINANCE,
      GL_SIGNED_NORMALIZED,
      0, 0, 0, 0,
      16, 0, 0, 0, 0,
      1, 1, 2
   },
   {
      MESA_FORMAT_SIGNED_AL1616,
      "MESA_FORMAT_SIGNED_AL1616",
      GL_LUMINANCE_ALPHA,
      GL_SIGNED_NORMALIZED,
      0, 0, 0, 16,
      16, 0, 0, 0, 0,
      1, 1, 4
   },
   {
      MESA_FORMAT_SIGNED_I16,
      "MESA_FORMAT_SIGNED_I16",
      GL_INTENSITY,
      GL_SIGNED_NORMALIZED,
      0, 0, 0, 0,
      0, 16, 0, 0, 0,
      1, 1, 2
   },
   {
      MESA_FORMAT_RGB9_E5_FLOAT,
      "MESA_FORMAT_RGB9_E5",
      GL_RGB,
      GL_FLOAT,
      9, 9, 9, 0,
      0, 0, 0, 0, 0,
      1, 1, 4
   },
   {
      MESA_FORMAT_R11_G11_B10_FLOAT,
      "MESA_FORMAT_R11_G11_B10_FLOAT",
      GL_RGB,
      GL_FLOAT,
      11, 11, 10, 0,
      0, 0, 0, 0, 0,
      1, 1, 4
   },
   /* ARB_depth_buffer_float */
   {
      MESA_FORMAT_Z32_FLOAT,       /* Name */
      "MESA_FORMAT_Z32_FLOAT",     /* StrName */
      GL_DEPTH_COMPONENT,          /* BaseFormat */
      GL_FLOAT,                    /* DataType */
      0, 0, 0, 0,                  /* Red/Green/Blue/AlphaBits */
      0, 0, 0, 32, 0,              /* Lum/Int/Index/Depth/StencilBits */
      1, 1, 4                      /* BlockWidth/Height,Bytes */
   },
   {
      MESA_FORMAT_Z32_FLOAT_X24S8, /* Name */
      "MESA_FORMAT_Z32_FLOAT_X24S8", /* StrName */
      GL_DEPTH_STENCIL,            /* BaseFormat */
      /* DataType here is used to answer GL_TEXTURE_DEPTH_TYPE queries, and is
       * never used for stencil because stencil is always GL_UNSIGNED_INT.
       */
      GL_FLOAT,                    /* DataType */
      0, 0, 0, 0,                  /* Red/Green/Blue/AlphaBits */
      0, 0, 0, 32, 8,              /* Lum/Int/Index/Depth/StencilBits */
      1, 1, 8                      /* BlockWidth/Height,Bytes */
   },
   {
      MESA_FORMAT_ARGB2101010_UINT,
      "MESA_FORMAT_ARGB2101010_UINT",
      GL_RGBA,
      GL_UNSIGNED_INT,
      10, 10, 10, 2,
      0, 0, 0, 0, 0,
      1, 1, 4
   },
};



static const struct gl_format_info *
_mesa_get_format_info(gl_format format)
{
   const struct gl_format_info *info = &format_info[format];
   assert(info->Name == format);
   return info;
}


/** Return string name of format (for debugging) */
const char *
_mesa_get_format_name(gl_format format)
{
   const struct gl_format_info *info = _mesa_get_format_info(format);
   return info->StrName;
}



/**
 * Return bytes needed to store a block of pixels in the given format.
 * Normally, a block is 1x1 (a single pixel).  But for compressed formats
 * a block may be 4x4 or 8x4, etc.
 *
 * Note: not GLuint, so as not to coerce math to unsigned. cf. fdo #37351
 */
GLint
_mesa_get_format_bytes(gl_format format)
{
   const struct gl_format_info *info = _mesa_get_format_info(format);
   ASSERT(info->BytesPerBlock);
   ASSERT(info->BytesPerBlock <= MAX_PIXEL_BYTES ||
          _mesa_is_format_compressed(format));
   return info->BytesPerBlock;
}


/**
 * Return bits per component for the given format.
 * \param format  one of MESA_FORMAT_x
 * \param pname  the component, such as GL_RED_BITS, GL_TEXTURE_BLUE_BITS, etc.
 */
GLint
_mesa_get_format_bits(gl_format format, GLenum pname)
{
   const struct gl_format_info *info = _mesa_get_format_info(format);

   switch (pname) {
   case GL_RED_BITS:
   case GL_TEXTURE_RED_SIZE:
   case GL_RENDERBUFFER_RED_SIZE_EXT:
   case GL_FRAMEBUFFER_ATTACHMENT_RED_SIZE:
      return info->RedBits;
   case GL_GREEN_BITS:
   case GL_TEXTURE_GREEN_SIZE:
   case GL_RENDERBUFFER_GREEN_SIZE_EXT:
   case GL_FRAMEBUFFER_ATTACHMENT_GREEN_SIZE:
      return info->GreenBits;
   case GL_BLUE_BITS:
   case GL_TEXTURE_BLUE_SIZE:
   case GL_RENDERBUFFER_BLUE_SIZE_EXT:
   case GL_FRAMEBUFFER_ATTACHMENT_BLUE_SIZE:
      return info->BlueBits;
   case GL_ALPHA_BITS:
   case GL_TEXTURE_ALPHA_SIZE:
   case GL_RENDERBUFFER_ALPHA_SIZE_EXT:
   case GL_FRAMEBUFFER_ATTACHMENT_ALPHA_SIZE:
      return info->AlphaBits;
   case GL_TEXTURE_INTENSITY_SIZE:
      return info->IntensityBits;
   case GL_TEXTURE_LUMINANCE_SIZE:
      return info->LuminanceBits;
   case GL_INDEX_BITS:
      return info->IndexBits;
   case GL_DEPTH_BITS:
   case GL_TEXTURE_DEPTH_SIZE_ARB:
   case GL_RENDERBUFFER_DEPTH_SIZE_EXT:
   case GL_FRAMEBUFFER_ATTACHMENT_DEPTH_SIZE:
      return info->DepthBits;
   case GL_STENCIL_BITS:
   case GL_TEXTURE_STENCIL_SIZE_EXT:
   case GL_RENDERBUFFER_STENCIL_SIZE_EXT:
   case GL_FRAMEBUFFER_ATTACHMENT_STENCIL_SIZE:
      return info->StencilBits;
   default:
      _mesa_problem(NULL, "bad pname in _mesa_get_format_bits()");
      return 0;
   }
}


GLuint
_mesa_get_format_max_bits(gl_format format)
{
   const struct gl_format_info *info = _mesa_get_format_info(format);
   GLuint max = MAX2(info->RedBits, info->GreenBits);
   max = MAX2(max, info->BlueBits);
   max = MAX2(max, info->AlphaBits);
   max = MAX2(max, info->LuminanceBits);
   max = MAX2(max, info->IntensityBits);
   max = MAX2(max, info->DepthBits);
   max = MAX2(max, info->StencilBits);
   return max;
}


/**
 * Return the data type (or more specifically, the data representation)
 * for the given format.
 * The return value will be one of:
 *    GL_UNSIGNED_NORMALIZED = unsigned int representing [0,1]
 *    GL_SIGNED_NORMALIZED = signed int representing [-1, 1]
 *    GL_UNSIGNED_INT = an ordinary unsigned integer
 *    GL_INT = an ordinary signed integer
 *    GL_FLOAT = an ordinary float
 */
GLenum
_mesa_get_format_datatype(gl_format format)
{
   const struct gl_format_info *info = _mesa_get_format_info(format);
   return info->DataType;
}


/**
 * Return the basic format for the given type.  The result will be one of
 * GL_RGB, GL_RGBA, GL_ALPHA, GL_LUMINANCE, GL_LUMINANCE_ALPHA, GL_INTENSITY,
 * GL_YCBCR_MESA, GL_DEPTH_COMPONENT, GL_STENCIL_INDEX, GL_DEPTH_STENCIL.
 */
GLenum
_mesa_get_format_base_format(gl_format format)
{
   const struct gl_format_info *info = _mesa_get_format_info(format);
   return info->BaseFormat;
}


/**
 * Return the block size (in pixels) for the given format.  Normally
 * the block size is 1x1.  But compressed formats will have block sizes
 * of 4x4 or 8x4 pixels, etc.
 * \param bw  returns block width in pixels
 * \param bh  returns block height in pixels
 */
void
_mesa_get_format_block_size(gl_format format, GLuint *bw, GLuint *bh)
{
   const struct gl_format_info *info = _mesa_get_format_info(format);
   *bw = info->BlockWidth;
   *bh = info->BlockHeight;
}


/** Is the given format a compressed format? */
GLboolean
_mesa_is_format_compressed(gl_format format)
{
   const struct gl_format_info *info = _mesa_get_format_info(format);
   return info->BlockWidth > 1 || info->BlockHeight > 1;
}


/**
 * Determine if the given format represents a packed depth/stencil buffer.
 */
GLboolean
_mesa_is_format_packed_depth_stencil(gl_format format)
{
   const struct gl_format_info *info = _mesa_get_format_info(format);

   return info->BaseFormat == GL_DEPTH_STENCIL;
}


/**
 * Is the given format a signed/unsigned integer color format?
 */
GLboolean
_mesa_is_format_integer_color(gl_format format)
{
   const struct gl_format_info *info = _mesa_get_format_info(format);
   return (info->DataType == GL_INT || info->DataType == GL_UNSIGNED_INT) &&
      info->BaseFormat != GL_DEPTH_COMPONENT &&
      info->BaseFormat != GL_DEPTH_STENCIL &&
      info->BaseFormat != GL_STENCIL_INDEX;
}


/**
 * Return color encoding for given format.
 * \return GL_LINEAR or GL_SRGB
 */
GLenum
_mesa_get_format_color_encoding(gl_format format)
{
   /* XXX this info should be encoded in gl_format_info */
   switch (format) {
   case MESA_FORMAT_SRGB8:
   case MESA_FORMAT_SRGBA8:
   case MESA_FORMAT_SARGB8:
   case MESA_FORMAT_SL8:
   case MESA_FORMAT_SLA8:
   case MESA_FORMAT_SRGB_DXT1:
   case MESA_FORMAT_SRGBA_DXT1:
   case MESA_FORMAT_SRGBA_DXT3:
   case MESA_FORMAT_SRGBA_DXT5:
      return GL_SRGB;
   default:
      return GL_LINEAR;
   }
}


/**
 * For an sRGB format, return the corresponding linear color space format.
 * For non-sRGB formats, return the format as-is.
 */
gl_format
_mesa_get_srgb_format_linear(gl_format format)
{
   switch (format) {
   case MESA_FORMAT_SRGB8:
      format = MESA_FORMAT_RGB888;
      break;
   case MESA_FORMAT_SRGBA8:
      format = MESA_FORMAT_RGBA8888;
      break;
   case MESA_FORMAT_SARGB8:
      format = MESA_FORMAT_ARGB8888;
      break;
   case MESA_FORMAT_SL8:
      format = MESA_FORMAT_L8;
      break;
   case MESA_FORMAT_SLA8:
      format = MESA_FORMAT_AL88;
      break;
   case MESA_FORMAT_SRGB_DXT1:
      format = MESA_FORMAT_RGB_DXT1;
      break;
   case MESA_FORMAT_SRGBA_DXT1:
      format = MESA_FORMAT_RGBA_DXT1;
      break;
   case MESA_FORMAT_SRGBA_DXT3:
      format = MESA_FORMAT_RGBA_DXT3;
      break;
   case MESA_FORMAT_SRGBA_DXT5:
      format = MESA_FORMAT_RGBA_DXT5;
      break;
   default:
      break;
   }
   return format;
}


/**
 * If the given format is a compressed format, return a corresponding
 * uncompressed format.
 */
gl_format
_mesa_get_uncompressed_format(gl_format format)
{
   switch (format) {
   case MESA_FORMAT_RGB_FXT1:
      return MESA_FORMAT_RGB888;
   case MESA_FORMAT_RGBA_FXT1:
      return MESA_FORMAT_RGBA8888;
   case MESA_FORMAT_RGB_DXT1:
   case MESA_FORMAT_SRGB_DXT1:
      return MESA_FORMAT_RGB888;
   case MESA_FORMAT_RGBA_DXT1:
   case MESA_FORMAT_SRGBA_DXT1:
      return MESA_FORMAT_RGBA8888;
   case MESA_FORMAT_RGBA_DXT3:
   case MESA_FORMAT_SRGBA_DXT3:
      return MESA_FORMAT_RGBA8888;
   case MESA_FORMAT_RGBA_DXT5:
   case MESA_FORMAT_SRGBA_DXT5:
      return MESA_FORMAT_RGBA8888;
   case MESA_FORMAT_RED_RGTC1:
      return MESA_FORMAT_R8;
   case MESA_FORMAT_SIGNED_RED_RGTC1:
      return MESA_FORMAT_SIGNED_R8;
   case MESA_FORMAT_RG_RGTC2:
      return MESA_FORMAT_GR88;
   case MESA_FORMAT_SIGNED_RG_RGTC2:
      return MESA_FORMAT_SIGNED_RG88_REV;
   case MESA_FORMAT_L_LATC1:
      return MESA_FORMAT_L8;
   case MESA_FORMAT_SIGNED_L_LATC1:
      return MESA_FORMAT_SIGNED_L8;
   case MESA_FORMAT_LA_LATC2:
      return MESA_FORMAT_AL88;
   case MESA_FORMAT_SIGNED_LA_LATC2:
      return MESA_FORMAT_SIGNED_AL88;
   case MESA_FORMAT_ETC1_RGB8:
      return MESA_FORMAT_RGB888;
   default:
#ifdef DEBUG
      assert(!_mesa_is_format_compressed(format));
#endif
      return format;
   }
}


GLuint
_mesa_format_num_components(gl_format format)
{
   const struct gl_format_info *info = _mesa_get_format_info(format);
   return ((info->RedBits > 0) +
           (info->GreenBits > 0) +
           (info->BlueBits > 0) +
           (info->AlphaBits > 0) +
           (info->LuminanceBits > 0) +
           (info->IntensityBits > 0) +
           (info->DepthBits > 0) +
           (info->StencilBits > 0));
}


/**
 * Return number of bytes needed to store an image of the given size
 * in the given format.
 */
GLuint
_mesa_format_image_size(gl_format format, GLsizei width,
                        GLsizei height, GLsizei depth)
{
   const struct gl_format_info *info = _mesa_get_format_info(format);
   /* Strictly speaking, a conditional isn't needed here */
   if (info->BlockWidth > 1 || info->BlockHeight > 1) {
      /* compressed format (2D only for now) */
      const GLuint bw = info->BlockWidth, bh = info->BlockHeight;
      const GLuint wblocks = (width + bw - 1) / bw;
      const GLuint hblocks = (height + bh - 1) / bh;
      const GLuint sz = wblocks * hblocks * info->BytesPerBlock;
      assert(depth == 1);
      return sz;
   }
   else {
      /* non-compressed */
      const GLuint sz = width * height * depth * info->BytesPerBlock;
      return sz;
   }
}


/**
 * Same as _mesa_format_image_size() but returns a 64-bit value to
 * accomodate very large textures.
 */
uint64_t
_mesa_format_image_size64(gl_format format, GLsizei width,
                          GLsizei height, GLsizei depth)
{
   const struct gl_format_info *info = _mesa_get_format_info(format);
   /* Strictly speaking, a conditional isn't needed here */
   if (info->BlockWidth > 1 || info->BlockHeight > 1) {
      /* compressed format (2D only for now) */
      const uint64_t bw = info->BlockWidth, bh = info->BlockHeight;
      const uint64_t wblocks = (width + bw - 1) / bw;
      const uint64_t hblocks = (height + bh - 1) / bh;
      const uint64_t sz = wblocks * hblocks * info->BytesPerBlock;
      assert(depth == 1);
      return sz;
   }
   else {
      /* non-compressed */
      const uint64_t sz = ((uint64_t) width *
                           (uint64_t) height *
                           (uint64_t) depth *
                           info->BytesPerBlock);
      return sz;
   }
}



GLint
_mesa_format_row_stride(gl_format format, GLsizei width)
{
   const struct gl_format_info *info = _mesa_get_format_info(format);
   /* Strictly speaking, a conditional isn't needed here */
   if (info->BlockWidth > 1 || info->BlockHeight > 1) {
      /* compressed format */
      const GLuint bw = info->BlockWidth;
      const GLuint wblocks = (width + bw - 1) / bw;
      const GLint stride = wblocks * info->BytesPerBlock;
      return stride;
   }
   else {
      const GLint stride = width * info->BytesPerBlock;
      return stride;
   }
}


/**
 * Debug/test: check that all formats are handled in the
 * _mesa_format_to_type_and_comps() function.  When new pixel formats
 * are added to Mesa, that function needs to be updated.
 * This is a no-op after the first call.
 */
static void
check_format_to_type_and_comps(void)
{
   gl_format f;

   for (f = MESA_FORMAT_NONE + 1; f < MESA_FORMAT_COUNT; f++) {
      GLenum datatype = 0;
      GLuint comps = 0;
      /* This function will emit a problem/warning if the format is
       * not handled.
       */
      _mesa_format_to_type_and_comps(f, &datatype, &comps);
   }
}


/**
 * Do sanity checking of the format info table.
 */
void
_mesa_test_formats(void)
{
   GLuint i;

   STATIC_ASSERT(Elements(format_info) == MESA_FORMAT_COUNT);

   for (i = 0; i < MESA_FORMAT_COUNT; i++) {
      const struct gl_format_info *info = _mesa_get_format_info(i);
      assert(info);

      assert(info->Name == i);

      if (info->Name == MESA_FORMAT_NONE)
         continue;

      if (info->BlockWidth == 1 && info->BlockHeight == 1) {
         if (info->RedBits > 0) {
            GLuint t = info->RedBits + info->GreenBits
               + info->BlueBits + info->AlphaBits;
            assert(t / 8 <= info->BytesPerBlock);
            (void) t;
         }
      }

      assert(info->DataType == GL_UNSIGNED_NORMALIZED ||
             info->DataType == GL_SIGNED_NORMALIZED ||
             info->DataType == GL_UNSIGNED_INT ||
             info->DataType == GL_INT ||
             info->DataType == GL_FLOAT ||
             /* Z32_FLOAT_X24S8 has DataType of GL_NONE */
             info->DataType == GL_NONE);

      if (info->BaseFormat == GL_RGB) {
         assert(info->RedBits > 0);
         assert(info->GreenBits > 0);
         assert(info->BlueBits > 0);
         assert(info->AlphaBits == 0);
         assert(info->LuminanceBits == 0);
         assert(info->IntensityBits == 0);
      }
      else if (info->BaseFormat == GL_RGBA) {
         assert(info->RedBits > 0);
         assert(info->GreenBits > 0);
         assert(info->BlueBits > 0);
         assert(info->AlphaBits > 0);
         assert(info->LuminanceBits == 0);
         assert(info->IntensityBits == 0);
      }
      else if (info->BaseFormat == GL_RG) {
         assert(info->RedBits > 0);
         assert(info->GreenBits > 0);
         assert(info->BlueBits == 0);
         assert(info->AlphaBits == 0);
         assert(info->LuminanceBits == 0);
         assert(info->IntensityBits == 0);
      }
      else if (info->BaseFormat == GL_RED) {
         assert(info->RedBits > 0);
         assert(info->GreenBits == 0);
         assert(info->BlueBits == 0);
         assert(info->AlphaBits == 0);
         assert(info->LuminanceBits == 0);
         assert(info->IntensityBits == 0);
      }
      else if (info->BaseFormat == GL_LUMINANCE) {
         assert(info->RedBits == 0);
         assert(info->GreenBits == 0);
         assert(info->BlueBits == 0);
         assert(info->AlphaBits == 0);
         assert(info->LuminanceBits > 0);
         assert(info->IntensityBits == 0);
      }
      else if (info->BaseFormat == GL_INTENSITY) {
         assert(info->RedBits == 0);
         assert(info->GreenBits == 0);
         assert(info->BlueBits == 0);
         assert(info->AlphaBits == 0);
         assert(info->LuminanceBits == 0);
         assert(info->IntensityBits > 0);
      }
   }

   check_format_to_type_and_comps();
}



/**
 * Return datatype and number of components per texel for the given gl_format.
 * Only used for mipmap generation code.
 */
void
_mesa_format_to_type_and_comps(gl_format format,
                               GLenum *datatype, GLuint *comps)
{
   switch (format) {
   case MESA_FORMAT_RGBA8888:
   case MESA_FORMAT_RGBA8888_REV:
   case MESA_FORMAT_ARGB8888:
   case MESA_FORMAT_ARGB8888_REV:
   case MESA_FORMAT_RGBX8888:
   case MESA_FORMAT_RGBX8888_REV:
   case MESA_FORMAT_XRGB8888:
   case MESA_FORMAT_XRGB8888_REV:
      *datatype = GL_UNSIGNED_BYTE;
      *comps = 4;
      return;
   case MESA_FORMAT_RGB888:
   case MESA_FORMAT_BGR888:
      *datatype = GL_UNSIGNED_BYTE;
      *comps = 3;
      return;
   case MESA_FORMAT_RGB565:
   case MESA_FORMAT_RGB565_REV:
      *datatype = GL_UNSIGNED_SHORT_5_6_5;
      *comps = 3;
      return;

   case MESA_FORMAT_ARGB4444:
   case MESA_FORMAT_ARGB4444_REV:
      *datatype = GL_UNSIGNED_SHORT_4_4_4_4;
      *comps = 4;
      return;

   case MESA_FORMAT_ARGB1555:
   case MESA_FORMAT_ARGB1555_REV:
      *datatype = GL_UNSIGNED_SHORT_1_5_5_5_REV;
      *comps = 4;
      return;

   case MESA_FORMAT_ARGB2101010:
      *datatype = GL_UNSIGNED_INT_2_10_10_10_REV;
      *comps = 4;
      return;

   case MESA_FORMAT_RGBA5551:
      *datatype = GL_UNSIGNED_SHORT_5_5_5_1;
      *comps = 4;
      return;

   case MESA_FORMAT_AL44:
      *datatype = MESA_UNSIGNED_BYTE_4_4;
      *comps = 2;
      return;

   case MESA_FORMAT_AL88:
   case MESA_FORMAT_AL88_REV:
   case MESA_FORMAT_GR88:
   case MESA_FORMAT_RG88:
      *datatype = GL_UNSIGNED_BYTE;
      *comps = 2;
      return;

   case MESA_FORMAT_AL1616:
   case MESA_FORMAT_AL1616_REV:
   case MESA_FORMAT_RG1616:
   case MESA_FORMAT_RG1616_REV:
      *datatype = GL_UNSIGNED_SHORT;
      *comps = 2;
      return;

   case MESA_FORMAT_R16:
   case MESA_FORMAT_A16:
   case MESA_FORMAT_L16:
   case MESA_FORMAT_I16:
      *datatype = GL_UNSIGNED_SHORT;
      *comps = 1;
      return;

   case MESA_FORMAT_RGB332:
      *datatype = GL_UNSIGNED_BYTE_3_3_2;
      *comps = 3;
      return;

   case MESA_FORMAT_A8:
   case MESA_FORMAT_L8:
   case MESA_FORMAT_I8:
   case MESA_FORMAT_R8:
   case MESA_FORMAT_S8:
      *datatype = GL_UNSIGNED_BYTE;
      *comps = 1;
      return;

   case MESA_FORMAT_YCBCR:
   case MESA_FORMAT_YCBCR_REV:
      *datatype = GL_UNSIGNED_SHORT;
      *comps = 2;
      return;

   case MESA_FORMAT_Z24_S8:
      *datatype = GL_UNSIGNED_INT_24_8_MESA;
      *comps = 2;
      return;

   case MESA_FORMAT_S8_Z24:
      *datatype = GL_UNSIGNED_INT_8_24_REV_MESA;
      *comps = 2;
      return;

   case MESA_FORMAT_Z16:
      *datatype = GL_UNSIGNED_SHORT;
      *comps = 1;
      return;

   case MESA_FORMAT_X8_Z24:
      *datatype = GL_UNSIGNED_INT;
      *comps = 1;
      return;

   case MESA_FORMAT_Z24_X8:
      *datatype = GL_UNSIGNED_INT;
      *comps = 1;
      return;

   case MESA_FORMAT_Z32:
      *datatype = GL_UNSIGNED_INT;
      *comps = 1;
      return;

   case MESA_FORMAT_Z32_FLOAT:
      *datatype = GL_FLOAT;
      *comps = 1;
      return;

   case MESA_FORMAT_Z32_FLOAT_X24S8:
      *datatype = GL_FLOAT_32_UNSIGNED_INT_24_8_REV;
      *comps = 1;
      return;

   case MESA_FORMAT_DUDV8:
      *datatype = GL_BYTE;
      *comps = 2;
      return;

   case MESA_FORMAT_SIGNED_R8:
   case MESA_FORMAT_SIGNED_A8:
   case MESA_FORMAT_SIGNED_L8:
   case MESA_FORMAT_SIGNED_I8:
      *datatype = GL_BYTE;
      *comps = 1;
      return;
   case MESA_FORMAT_SIGNED_RG88_REV:
   case MESA_FORMAT_SIGNED_AL88:
      *datatype = GL_BYTE;
      *comps = 2;
      return;
   case MESA_FORMAT_SIGNED_RGBA8888:
   case MESA_FORMAT_SIGNED_RGBA8888_REV:
   case MESA_FORMAT_SIGNED_RGBX8888:
      *datatype = GL_BYTE;
      *comps = 4;
      return;

   case MESA_FORMAT_RGBA_16:
      *datatype = GL_UNSIGNED_SHORT;
      *comps = 4;
      return;

   case MESA_FORMAT_SIGNED_R16:
   case MESA_FORMAT_SIGNED_A16:
   case MESA_FORMAT_SIGNED_L16:
   case MESA_FORMAT_SIGNED_I16:
      *datatype = GL_SHORT;
      *comps = 1;
      return;
   case MESA_FORMAT_SIGNED_GR1616:
   case MESA_FORMAT_SIGNED_AL1616:
      *datatype = GL_SHORT;
      *comps = 2;
      return;
   case MESA_FORMAT_SIGNED_RGB_16:
      *datatype = GL_SHORT;
      *comps = 3;
      return;
   case MESA_FORMAT_SIGNED_RGBA_16:
      *datatype = GL_SHORT;
      *comps = 4;
      return;

#if FEATURE_EXT_texture_sRGB
   case MESA_FORMAT_SRGB8:
      *datatype = GL_UNSIGNED_BYTE;
      *comps = 3;
      return;
   case MESA_FORMAT_SRGBA8:
   case MESA_FORMAT_SARGB8:
      *datatype = GL_UNSIGNED_BYTE;
      *comps = 4;
      return;
   case MESA_FORMAT_SL8:
      *datatype = GL_UNSIGNED_BYTE;
      *comps = 1;
      return;
   case MESA_FORMAT_SLA8:
      *datatype = GL_UNSIGNED_BYTE;
      *comps = 2;
      return;
#endif

#if FEATURE_texture_fxt1
   case MESA_FORMAT_RGB_FXT1:
   case MESA_FORMAT_RGBA_FXT1:
#endif
#if FEATURE_texture_s3tc
   case MESA_FORMAT_RGB_DXT1:
   case MESA_FORMAT_RGBA_DXT1:
   case MESA_FORMAT_RGBA_DXT3:
   case MESA_FORMAT_RGBA_DXT5:
#if FEATURE_EXT_texture_sRGB
   case MESA_FORMAT_SRGB_DXT1:
   case MESA_FORMAT_SRGBA_DXT1:
   case MESA_FORMAT_SRGBA_DXT3:
   case MESA_FORMAT_SRGBA_DXT5:
#endif
#endif
   case MESA_FORMAT_RED_RGTC1:
   case MESA_FORMAT_SIGNED_RED_RGTC1:
   case MESA_FORMAT_RG_RGTC2:
   case MESA_FORMAT_SIGNED_RG_RGTC2:
   case MESA_FORMAT_L_LATC1:
   case MESA_FORMAT_SIGNED_L_LATC1:
   case MESA_FORMAT_LA_LATC2:
   case MESA_FORMAT_SIGNED_LA_LATC2:
   case MESA_FORMAT_ETC1_RGB8:
      /* XXX generate error instead? */
      *datatype = GL_UNSIGNED_BYTE;
      *comps = 0;
      return;

   case MESA_FORMAT_RGBA_FLOAT32:
      *datatype = GL_FLOAT;
      *comps = 4;
      return;
   case MESA_FORMAT_RGBA_FLOAT16:
      *datatype = GL_HALF_FLOAT_ARB;
      *comps = 4;
      return;
   case MESA_FORMAT_RGB_FLOAT32:
      *datatype = GL_FLOAT;
      *comps = 3;
      return;
   case MESA_FORMAT_RGB_FLOAT16:
      *datatype = GL_HALF_FLOAT_ARB;
      *comps = 3;
      return;
   case MESA_FORMAT_LUMINANCE_ALPHA_FLOAT32:
   case MESA_FORMAT_RG_FLOAT32:
      *datatype = GL_FLOAT;
      *comps = 2;
      return;
   case MESA_FORMAT_LUMINANCE_ALPHA_FLOAT16:
   case MESA_FORMAT_RG_FLOAT16:
      *datatype = GL_HALF_FLOAT_ARB;
      *comps = 2;
      return;
   case MESA_FORMAT_ALPHA_FLOAT32:
   case MESA_FORMAT_LUMINANCE_FLOAT32:
   case MESA_FORMAT_INTENSITY_FLOAT32:
   case MESA_FORMAT_R_FLOAT32:
      *datatype = GL_FLOAT;
      *comps = 1;
      return;
   case MESA_FORMAT_ALPHA_FLOAT16:
   case MESA_FORMAT_LUMINANCE_FLOAT16:
   case MESA_FORMAT_INTENSITY_FLOAT16:
   case MESA_FORMAT_R_FLOAT16:
      *datatype = GL_HALF_FLOAT_ARB;
      *comps = 1;
      return;

   case MESA_FORMAT_ALPHA_UINT8:
   case MESA_FORMAT_LUMINANCE_UINT8:
   case MESA_FORMAT_INTENSITY_UINT8:
      *datatype = GL_UNSIGNED_BYTE;
      *comps = 1;
      return;
   case MESA_FORMAT_LUMINANCE_ALPHA_UINT8:
      *datatype = GL_UNSIGNED_BYTE;
      *comps = 2;
      return;

   case MESA_FORMAT_ALPHA_UINT16:
   case MESA_FORMAT_LUMINANCE_UINT16:
   case MESA_FORMAT_INTENSITY_UINT16:
      *datatype = GL_UNSIGNED_SHORT;
      *comps = 1;
      return;
   case MESA_FORMAT_LUMINANCE_ALPHA_UINT16:
      *datatype = GL_UNSIGNED_SHORT;
      *comps = 2;
      return;
   case MESA_FORMAT_ALPHA_UINT32:
   case MESA_FORMAT_LUMINANCE_UINT32:
   case MESA_FORMAT_INTENSITY_UINT32:
      *datatype = GL_UNSIGNED_INT;
      *comps = 1;
      return;
   case MESA_FORMAT_LUMINANCE_ALPHA_UINT32:
      *datatype = GL_UNSIGNED_INT;
      *comps = 2;
      return;
   case MESA_FORMAT_ALPHA_INT8:
   case MESA_FORMAT_LUMINANCE_INT8:
   case MESA_FORMAT_INTENSITY_INT8:
      *datatype = GL_BYTE;
      *comps = 1;
      return;
   case MESA_FORMAT_LUMINANCE_ALPHA_INT8:
      *datatype = GL_BYTE;
      *comps = 2;
      return;

   case MESA_FORMAT_ALPHA_INT16:
   case MESA_FORMAT_LUMINANCE_INT16:
   case MESA_FORMAT_INTENSITY_INT16:
      *datatype = GL_SHORT;
      *comps = 1;
      return;
   case MESA_FORMAT_LUMINANCE_ALPHA_INT16:
      *datatype = GL_SHORT;
      *comps = 2;
      return;

   case MESA_FORMAT_ALPHA_INT32:
   case MESA_FORMAT_LUMINANCE_INT32:
   case MESA_FORMAT_INTENSITY_INT32:
      *datatype = GL_INT;
      *comps = 1;
      return;
   case MESA_FORMAT_LUMINANCE_ALPHA_INT32:
      *datatype = GL_INT;
      *comps = 2;
      return;

   case MESA_FORMAT_R_INT8:
      *datatype = GL_BYTE;
      *comps = 1;
      return;
   case MESA_FORMAT_RG_INT8:
      *datatype = GL_BYTE;
      *comps = 2;
      return;
   case MESA_FORMAT_RGB_INT8:
      *datatype = GL_BYTE;
      *comps = 3;
      return;
   case MESA_FORMAT_RGBA_INT8:
      *datatype = GL_BYTE;
      *comps = 4;
      return;
   case MESA_FORMAT_R_INT16:
      *datatype = GL_SHORT;
      *comps = 1;
      return;
   case MESA_FORMAT_RG_INT16:
      *datatype = GL_SHORT;
      *comps = 2;
      return;
   case MESA_FORMAT_RGB_INT16:
      *datatype = GL_SHORT;
      *comps = 3;
      return;
   case MESA_FORMAT_RGBA_INT16:
      *datatype = GL_SHORT;
      *comps = 4;
      return;
   case MESA_FORMAT_R_INT32:
      *datatype = GL_INT;
      *comps = 1;
      return;
   case MESA_FORMAT_RG_INT32:
      *datatype = GL_INT;
      *comps = 2;
      return;
   case MESA_FORMAT_RGB_INT32:
      *datatype = GL_INT;
      *comps = 3;
      return;
   case MESA_FORMAT_RGBA_INT32:
      *datatype = GL_INT;
      *comps = 4;
      return;

   /**
    * \name Non-normalized unsigned integer formats.
    */
   case MESA_FORMAT_R_UINT8:
      *datatype = GL_UNSIGNED_BYTE;
      *comps = 1;
      return;
   case MESA_FORMAT_RG_UINT8:
      *datatype = GL_UNSIGNED_BYTE;
      *comps = 2;
      return;
   case MESA_FORMAT_RGB_UINT8:
      *datatype = GL_UNSIGNED_BYTE;
      *comps = 3;
      return;
   case MESA_FORMAT_RGBA_UINT8:
      *datatype = GL_UNSIGNED_BYTE;
      *comps = 4;
      return;
   case MESA_FORMAT_R_UINT16:
      *datatype = GL_UNSIGNED_SHORT;
      *comps = 1;
      return;
   case MESA_FORMAT_RG_UINT16:
      *datatype = GL_UNSIGNED_SHORT;
      *comps = 2;
      return;
   case MESA_FORMAT_RGB_UINT16:
      *datatype = GL_UNSIGNED_SHORT;
      *comps = 3;
      return;
   case MESA_FORMAT_RGBA_UINT16:
      *datatype = GL_UNSIGNED_SHORT;
      *comps = 4;
      return;
   case MESA_FORMAT_R_UINT32:
      *datatype = GL_UNSIGNED_INT;
      *comps = 1;
      return;
   case MESA_FORMAT_RG_UINT32:
      *datatype = GL_UNSIGNED_INT;
      *comps = 2;
      return;
   case MESA_FORMAT_RGB_UINT32:
      *datatype = GL_UNSIGNED_INT;
      *comps = 3;
      return;
   case MESA_FORMAT_RGBA_UINT32:
      *datatype = GL_UNSIGNED_INT;
      *comps = 4;
      return;

   case MESA_FORMAT_RGB9_E5_FLOAT:
      *datatype = GL_UNSIGNED_INT_5_9_9_9_REV;
      *comps = 3;
      return;

   case MESA_FORMAT_R11_G11_B10_FLOAT:
      *datatype = GL_UNSIGNED_INT_10F_11F_11F_REV;
      *comps = 3;
      return;

   case MESA_FORMAT_ARGB2101010_UINT:
      *datatype = GL_UNSIGNED_INT_2_10_10_10_REV;
      *comps = 4;
      return;

   case MESA_FORMAT_COUNT:
      assert(0);
      return;

   case MESA_FORMAT_NONE:
   /* For debug builds, warn if any formats are not handled */
#ifdef DEBUG
   default:
#endif
      _mesa_problem(NULL, "bad format %s in _mesa_format_to_type_and_comps",
                    _mesa_get_format_name(format));
      *datatype = 0;
      *comps = 1;
   }
}

/**
 * Check if a gl_format exactly matches a GL formaat/type combination
 * such that we can use memcpy() from one to the other.
 *
 * Note: this matching assumes that GL_PACK/UNPACK_SWAP_BYTES is unset.
 *
 * \return GL_TRUE if the formats match, GL_FALSE otherwise.
 */
GLboolean
_mesa_format_matches_format_and_type(gl_format gl_format,
				     GLenum format, GLenum type)
{
   const GLboolean littleEndian = _mesa_little_endian();

   /* Note: When reading a GL format/type combination, the format lists channel
    * assignments from most significant channel in the type to least
    * significant.  A type with _REV indicates that the assignments are
    * swapped, so they are listed from least significant to most significant.
    *
    * For sanity, please keep this switch statement ordered the same as the
    * enums in formats.h.
    */

   switch (gl_format) {

   case MESA_FORMAT_NONE:
   case MESA_FORMAT_COUNT:
      return GL_FALSE;

   case MESA_FORMAT_RGBA8888:
      return ((format == GL_RGBA && (type == GL_UNSIGNED_INT_8_8_8_8 ||
				     (type == GL_UNSIGNED_BYTE && !littleEndian))) ||
	      (format == GL_ABGR_EXT && (type == GL_UNSIGNED_INT_8_8_8_8_REV ||
					 (type == GL_UNSIGNED_BYTE && littleEndian))));

   case MESA_FORMAT_RGBA8888_REV:
      return ((format == GL_RGBA && type == GL_UNSIGNED_INT_8_8_8_8_REV));

   case MESA_FORMAT_ARGB8888:
      return ((format == GL_BGRA && (type == GL_UNSIGNED_INT_8_8_8_8_REV ||
				     (type == GL_UNSIGNED_BYTE && littleEndian))));

   case MESA_FORMAT_ARGB8888_REV:
      return ((format == GL_BGRA && (type == GL_UNSIGNED_INT_8_8_8_8 ||
				     (type == GL_UNSIGNED_BYTE && !littleEndian))));

   case MESA_FORMAT_RGBX8888:
   case MESA_FORMAT_RGBX8888_REV:
      return GL_FALSE;

   case MESA_FORMAT_XRGB8888:
   case MESA_FORMAT_XRGB8888_REV:
      return GL_FALSE;

   case MESA_FORMAT_RGB888:
      return format == GL_BGR && type == GL_UNSIGNED_BYTE && littleEndian;

   case MESA_FORMAT_BGR888:
      return format == GL_RGB && type == GL_UNSIGNED_BYTE && littleEndian;

   case MESA_FORMAT_RGB565:
      return format == GL_RGB && type == GL_UNSIGNED_SHORT_5_6_5;
   case MESA_FORMAT_RGB565_REV:
      /* Some of the 16-bit MESA_FORMATs that would seem to correspond to
       * GL_UNSIGNED_SHORT_* are byte-swapped instead of channel-reversed,
       * according to formats.h, so they can't be matched.
       */
      return GL_FALSE;

   case MESA_FORMAT_ARGB4444:
      return format == GL_BGRA && type == GL_UNSIGNED_SHORT_4_4_4_4_REV;
   case MESA_FORMAT_ARGB4444_REV:
      return GL_FALSE;

   case MESA_FORMAT_RGBA5551:
      return format == GL_RGBA && type == GL_UNSIGNED_SHORT_5_5_5_1;

   case MESA_FORMAT_ARGB1555:
      return format == GL_BGRA && type == GL_UNSIGNED_SHORT_1_5_5_5_REV;
   case MESA_FORMAT_ARGB1555_REV:
      return GL_FALSE;

   case MESA_FORMAT_AL44:
      return GL_FALSE;
   case MESA_FORMAT_AL88:
      return format == GL_LUMINANCE_ALPHA && type == GL_UNSIGNED_BYTE && littleEndian;
   case MESA_FORMAT_AL88_REV:
      return GL_FALSE;

   case MESA_FORMAT_AL1616:
      return format == GL_LUMINANCE_ALPHA && type == GL_UNSIGNED_SHORT && littleEndian;
   case MESA_FORMAT_AL1616_REV:
      return GL_FALSE;

   case MESA_FORMAT_RGB332:
      return format == GL_RGB && type == GL_UNSIGNED_BYTE_3_3_2;

   case MESA_FORMAT_A8:
      return format == GL_ALPHA && type == GL_UNSIGNED_BYTE;
   case MESA_FORMAT_A16:
      return format == GL_ALPHA && type == GL_UNSIGNED_SHORT && littleEndian;
   case MESA_FORMAT_L8:
      return format == GL_LUMINANCE && type == GL_UNSIGNED_BYTE;
   case MESA_FORMAT_L16:
      return format == GL_LUMINANCE && type == GL_UNSIGNED_SHORT && littleEndian;
   case MESA_FORMAT_I8:
      return format == GL_INTENSITY && type == GL_UNSIGNED_BYTE;
   case MESA_FORMAT_I16:
      return format == GL_INTENSITY && type == GL_UNSIGNED_SHORT && littleEndian;

   case MESA_FORMAT_YCBCR:
   case MESA_FORMAT_YCBCR_REV:
      return GL_FALSE;

   case MESA_FORMAT_R8:
      return format == GL_RED && type == GL_UNSIGNED_BYTE;
   case MESA_FORMAT_GR88:
      return format == GL_RG && type == GL_UNSIGNED_BYTE && littleEndian;
   case MESA_FORMAT_RG88:
      return GL_FALSE;

   case MESA_FORMAT_R16:
      return format == GL_RED && type == GL_UNSIGNED_SHORT && littleEndian;
   case MESA_FORMAT_RG1616:
      return format == GL_RG && type == GL_UNSIGNED_SHORT && littleEndian;
   case MESA_FORMAT_RG1616_REV:
      return GL_FALSE;

   case MESA_FORMAT_ARGB2101010:
      return format == GL_BGRA && type == GL_UNSIGNED_INT_2_10_10_10_REV;

   case MESA_FORMAT_Z24_S8:
      return format == GL_DEPTH_STENCIL && type == GL_UNSIGNED_INT_24_8;
   case MESA_FORMAT_Z24_X8:
   case MESA_FORMAT_S8_Z24:
      return GL_FALSE;

   case MESA_FORMAT_Z16:
      return format == GL_DEPTH_COMPONENT && type == GL_UNSIGNED_SHORT;

   case MESA_FORMAT_X8_Z24:
      return GL_FALSE;

   case MESA_FORMAT_Z32:
      return format == GL_DEPTH_COMPONENT && type == GL_UNSIGNED_INT;

   case MESA_FORMAT_S8:
      return GL_FALSE;

   case MESA_FORMAT_SRGB8:
   case MESA_FORMAT_SRGBA8:
   case MESA_FORMAT_SARGB8:
   case MESA_FORMAT_SL8:
   case MESA_FORMAT_SLA8:
   case MESA_FORMAT_SRGB_DXT1:
   case MESA_FORMAT_SRGBA_DXT1:
   case MESA_FORMAT_SRGBA_DXT3:
   case MESA_FORMAT_SRGBA_DXT5:
      return GL_FALSE;

   case MESA_FORMAT_RGB_FXT1:
   case MESA_FORMAT_RGBA_FXT1:
   case MESA_FORMAT_RGB_DXT1:
   case MESA_FORMAT_RGBA_DXT1:
   case MESA_FORMAT_RGBA_DXT3:
   case MESA_FORMAT_RGBA_DXT5:
      return GL_FALSE;

   case MESA_FORMAT_RGBA_FLOAT32:
      return format == GL_RGBA && type == GL_FLOAT;
   case MESA_FORMAT_RGBA_FLOAT16:
      return format == GL_RGBA && type == GL_HALF_FLOAT;

   case MESA_FORMAT_RGB_FLOAT32:
      return format == GL_RGB && type == GL_FLOAT;
   case MESA_FORMAT_RGB_FLOAT16:
      return format == GL_RGB && type == GL_HALF_FLOAT;

   case MESA_FORMAT_ALPHA_FLOAT32:
      return format == GL_ALPHA && type == GL_FLOAT;
   case MESA_FORMAT_ALPHA_FLOAT16:
      return format == GL_ALPHA && type == GL_HALF_FLOAT;

   case MESA_FORMAT_LUMINANCE_FLOAT32:
      return format == GL_LUMINANCE && type == GL_FLOAT;
   case MESA_FORMAT_LUMINANCE_FLOAT16:
      return format == GL_LUMINANCE && type == GL_HALF_FLOAT;

   case MESA_FORMAT_LUMINANCE_ALPHA_FLOAT32:
      return format == GL_LUMINANCE_ALPHA && type == GL_FLOAT;
   case MESA_FORMAT_LUMINANCE_ALPHA_FLOAT16:
      return format == GL_LUMINANCE_ALPHA && type == GL_HALF_FLOAT;

   case MESA_FORMAT_INTENSITY_FLOAT32:
      return format == GL_INTENSITY && type == GL_FLOAT;
   case MESA_FORMAT_INTENSITY_FLOAT16:
      return format == GL_INTENSITY && type == GL_HALF_FLOAT;

   case MESA_FORMAT_R_FLOAT32:
      return format == GL_RED && type == GL_FLOAT;
   case MESA_FORMAT_R_FLOAT16:
      return format == GL_RED && type == GL_HALF_FLOAT;

   case MESA_FORMAT_RG_FLOAT32:
      return format == GL_RG && type == GL_FLOAT;
   case MESA_FORMAT_RG_FLOAT16:
      return format == GL_RG && type == GL_HALF_FLOAT;

      /* FINISHME: What do we want to do for GL_EXT_texture_integer? */
   case MESA_FORMAT_ALPHA_UINT8:
   case MESA_FORMAT_ALPHA_UINT16:
   case MESA_FORMAT_ALPHA_UINT32:
   case MESA_FORMAT_ALPHA_INT8:
   case MESA_FORMAT_ALPHA_INT16:
   case MESA_FORMAT_ALPHA_INT32:
      return GL_FALSE;

   case MESA_FORMAT_INTENSITY_UINT8:
   case MESA_FORMAT_INTENSITY_UINT16:
   case MESA_FORMAT_INTENSITY_UINT32:
   case MESA_FORMAT_INTENSITY_INT8:
   case MESA_FORMAT_INTENSITY_INT16:
   case MESA_FORMAT_INTENSITY_INT32:
      return GL_FALSE;

   case MESA_FORMAT_LUMINANCE_UINT8:
   case MESA_FORMAT_LUMINANCE_UINT16:
   case MESA_FORMAT_LUMINANCE_UINT32:
   case MESA_FORMAT_LUMINANCE_INT8:
   case MESA_FORMAT_LUMINANCE_INT16:
   case MESA_FORMAT_LUMINANCE_INT32:
      return GL_FALSE;

   case MESA_FORMAT_LUMINANCE_ALPHA_UINT8:
   case MESA_FORMAT_LUMINANCE_ALPHA_UINT16:
   case MESA_FORMAT_LUMINANCE_ALPHA_UINT32:
   case MESA_FORMAT_LUMINANCE_ALPHA_INT8:
   case MESA_FORMAT_LUMINANCE_ALPHA_INT16:
   case MESA_FORMAT_LUMINANCE_ALPHA_INT32:
      return GL_FALSE;

   case MESA_FORMAT_R_INT8:
   case MESA_FORMAT_RG_INT8:
   case MESA_FORMAT_RGB_INT8:
   case MESA_FORMAT_RGBA_INT8:
   case MESA_FORMAT_R_INT16:
   case MESA_FORMAT_RG_INT16:
   case MESA_FORMAT_RGB_INT16:
   case MESA_FORMAT_RGBA_INT16:
   case MESA_FORMAT_R_INT32:
   case MESA_FORMAT_RG_INT32:
   case MESA_FORMAT_RGB_INT32:
   case MESA_FORMAT_RGBA_INT32:
      return GL_FALSE;

   case MESA_FORMAT_R_UINT8:
   case MESA_FORMAT_RG_UINT8:
   case MESA_FORMAT_RGB_UINT8:
   case MESA_FORMAT_RGBA_UINT8:
   case MESA_FORMAT_R_UINT16:
   case MESA_FORMAT_RG_UINT16:
   case MESA_FORMAT_RGB_UINT16:
   case MESA_FORMAT_RGBA_UINT16:
   case MESA_FORMAT_R_UINT32:
   case MESA_FORMAT_RG_UINT32:
   case MESA_FORMAT_RGB_UINT32:
   case MESA_FORMAT_RGBA_UINT32:
      return GL_FALSE;

   case MESA_FORMAT_DUDV8:
   case MESA_FORMAT_SIGNED_R8:
   case MESA_FORMAT_SIGNED_RG88_REV:
   case MESA_FORMAT_SIGNED_RGBX8888:
   case MESA_FORMAT_SIGNED_RGBA8888:
   case MESA_FORMAT_SIGNED_RGBA8888_REV:
   case MESA_FORMAT_SIGNED_R16:
   case MESA_FORMAT_SIGNED_GR1616:
   case MESA_FORMAT_SIGNED_RGB_16:
   case MESA_FORMAT_SIGNED_RGBA_16:
   case MESA_FORMAT_RGBA_16:
      /* FINISHME: SNORM */
      return GL_FALSE;

   case MESA_FORMAT_RED_RGTC1:
   case MESA_FORMAT_SIGNED_RED_RGTC1:
   case MESA_FORMAT_RG_RGTC2:
   case MESA_FORMAT_SIGNED_RG_RGTC2:
      return GL_FALSE;

   case MESA_FORMAT_L_LATC1:
   case MESA_FORMAT_SIGNED_L_LATC1:
   case MESA_FORMAT_LA_LATC2:
   case MESA_FORMAT_SIGNED_LA_LATC2:
      return GL_FALSE;

   case MESA_FORMAT_ETC1_RGB8:
      return GL_FALSE;

   case MESA_FORMAT_SIGNED_A8:
   case MESA_FORMAT_SIGNED_L8:
   case MESA_FORMAT_SIGNED_AL88:
   case MESA_FORMAT_SIGNED_I8:
   case MESA_FORMAT_SIGNED_A16:
   case MESA_FORMAT_SIGNED_L16:
   case MESA_FORMAT_SIGNED_AL1616:
   case MESA_FORMAT_SIGNED_I16:
      /* FINISHME: SNORM */
      return GL_FALSE;

   case MESA_FORMAT_ARGB2101010_UINT:
      return GL_FALSE;

   case MESA_FORMAT_RGB9_E5_FLOAT:
      return format == GL_RGB && type == GL_UNSIGNED_INT_5_9_9_9_REV;
   case MESA_FORMAT_R11_G11_B10_FLOAT:
      return format == GL_RGB && type == GL_UNSIGNED_INT_10F_11F_11F_REV;

   case MESA_FORMAT_Z32_FLOAT:
      return format == GL_DEPTH_COMPONENT && type == GL_FLOAT;

   case MESA_FORMAT_Z32_FLOAT_X24S8:
      return GL_FALSE;
   }

   return GL_FALSE;
}

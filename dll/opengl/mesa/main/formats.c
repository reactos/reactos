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

#include <precomp.h>

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
    * GL_DEPTH_COMPONENT, GL_STENCIL_INDEX, GL_DEPTH_STENCIL.
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
   }
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
   ASSERT(info->BytesPerBlock <= MAX_PIXEL_BYTES);
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


/**
 * Is the given format a signed/unsigned integer color format?
 */
GLboolean
_mesa_is_format_integer_color(gl_format format)
{
   const struct gl_format_info *info = _mesa_get_format_info(format);
   return (info->DataType == GL_INT || info->DataType == GL_UNSIGNED_INT) &&
      info->BaseFormat != GL_DEPTH_COMPONENT &&
      info->BaseFormat != GL_STENCIL_INDEX;
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
      *datatype = GL_UNSIGNED_BYTE;
      *comps = 2;
      return;

   case MESA_FORMAT_AL1616:
   case MESA_FORMAT_AL1616_REV:
      *datatype = GL_UNSIGNED_SHORT;
      *comps = 2;
      return;

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
   case MESA_FORMAT_S8:
      *datatype = GL_UNSIGNED_BYTE;
      *comps = 1;
      return;

   case MESA_FORMAT_YCBCR:
   case MESA_FORMAT_YCBCR_REV:
      *datatype = GL_UNSIGNED_SHORT;
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

   case MESA_FORMAT_RGBA_16:
      *datatype = GL_UNSIGNED_SHORT;
      *comps = 4;
      return;

   case MESA_FORMAT_SIGNED_RGBA_16:
      *datatype = GL_SHORT;
      *comps = 4;
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

   case MESA_FORMAT_RGB_INT8:
      *datatype = GL_BYTE;
      *comps = 3;
      return;
   case MESA_FORMAT_RGBA_INT8:
      *datatype = GL_BYTE;
      *comps = 4;
      return;
   case MESA_FORMAT_RGB_INT16:
      *datatype = GL_SHORT;
      *comps = 3;
      return;
   case MESA_FORMAT_RGBA_INT16:
      *datatype = GL_SHORT;
      *comps = 4;
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
   case MESA_FORMAT_RGB_UINT8:
      *datatype = GL_UNSIGNED_BYTE;
      *comps = 3;
      return;
   case MESA_FORMAT_RGBA_UINT8:
      *datatype = GL_UNSIGNED_BYTE;
      *comps = 4;
      return;
   case MESA_FORMAT_RGB_UINT16:
      *datatype = GL_UNSIGNED_SHORT;
      *comps = 3;
      return;
   case MESA_FORMAT_RGBA_UINT16:
      *datatype = GL_UNSIGNED_SHORT;
      *comps = 4;
      return;
   case MESA_FORMAT_RGB_UINT32:
      *datatype = GL_UNSIGNED_INT;
      *comps = 3;
      return;
   case MESA_FORMAT_RGBA_UINT32:
      *datatype = GL_UNSIGNED_INT;
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

   case MESA_FORMAT_Z24_X8:
      return GL_FALSE;

   case MESA_FORMAT_Z16:
      return format == GL_DEPTH_COMPONENT && type == GL_UNSIGNED_SHORT;

   case MESA_FORMAT_X8_Z24:
      return GL_FALSE;

   case MESA_FORMAT_Z32:
      return format == GL_DEPTH_COMPONENT && type == GL_UNSIGNED_INT;

   case MESA_FORMAT_S8:
      return GL_FALSE;

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

   case MESA_FORMAT_RGB_INT8:
   case MESA_FORMAT_RGBA_INT8:
   case MESA_FORMAT_RGB_INT16:
   case MESA_FORMAT_RGBA_INT16:
   case MESA_FORMAT_RGB_INT32:
   case MESA_FORMAT_RGBA_INT32:
      return GL_FALSE;

   case MESA_FORMAT_RGB_UINT8:
   case MESA_FORMAT_RGBA_UINT8:
   case MESA_FORMAT_RGB_UINT16:
   case MESA_FORMAT_RGBA_UINT16:
   case MESA_FORMAT_RGB_UINT32:
   case MESA_FORMAT_RGBA_UINT32:
      return GL_FALSE;

   case MESA_FORMAT_SIGNED_RGBA_16:
   case MESA_FORMAT_RGBA_16:
      /* FINISHME: SNORM */
      return GL_FALSE;
   }

   return GL_FALSE;
}

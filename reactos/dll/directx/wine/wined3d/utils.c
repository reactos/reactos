/*
 * Utility functions for the WineD3D Library
 *
 * Copyright 2002-2004 Jason Edmeades
 * Copyright 2003-2004 Raphael Junqueira
 * Copyright 2004 Christian Costa
 * Copyright 2005 Oliver Stieber
 * Copyright 2006-2007 Henri Verbeet
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
#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d);

#define GLINFO_LOCATION This->adapter->gl_info

/*****************************************************************************
 * Pixel format array
 */
static const StaticPixelFormatDesc formats[] = {
  /*{WINED3DFORMAT          ,alphamask  ,redmask    ,greenmask  ,bluemask   ,bpp    ,depth  ,stencil,    isFourcc*/
    {WINED3DFMT_UNKNOWN     ,0x0        ,0x0        ,0x0        ,0x0        ,1      ,0      ,0          ,FALSE },
    /* FourCC formats, kept here to have WINED3DFMT_R8G8B8(=20) at position 20 */
    {WINED3DFMT_UYVY        ,0x0        ,0x0        ,0x0        ,0x0        ,1/*?*/ ,0      ,0          ,TRUE  },
    {WINED3DFMT_YUY2        ,0x0        ,0x0        ,0x0        ,0x0        ,1/*?*/ ,0      ,0          ,TRUE  },
    {WINED3DFMT_DXT1        ,0x0        ,0x0        ,0x0        ,0x0        ,1      ,0      ,0          ,TRUE  },
    {WINED3DFMT_DXT2        ,0x0        ,0x0        ,0x0        ,0x0        ,1      ,0      ,0          ,TRUE  },
    {WINED3DFMT_DXT3        ,0x0        ,0x0        ,0x0        ,0x0        ,1      ,0      ,0          ,TRUE  },
    {WINED3DFMT_DXT4        ,0x0        ,0x0        ,0x0        ,0x0        ,1      ,0      ,0          ,TRUE  },
    {WINED3DFMT_DXT5        ,0x0        ,0x0        ,0x0        ,0x0        ,1      ,0      ,0          ,TRUE  },
    {WINED3DFMT_MULTI2_ARGB8,0x0        ,0x0        ,0x0        ,0x0        ,1/*?*/ ,0      ,0          ,TRUE  },
    {WINED3DFMT_G8R8_G8B8   ,0x0        ,0x0        ,0x0        ,0x0        ,1/*?*/ ,0      ,0          ,TRUE  },
    {WINED3DFMT_R8G8_B8G8   ,0x0        ,0x0        ,0x0        ,0x0        ,1/*?*/ ,0      ,0          ,TRUE  },
    /* IEEE formats */
    {WINED3DFMT_R32F        ,0x0        ,0x0        ,0x0        ,0x0        ,4      ,0      ,0          ,FALSE },
    {WINED3DFMT_G32R32F     ,0x0        ,0x0        ,0x0        ,0x0        ,8      ,0      ,0          ,FALSE },
    {WINED3DFMT_A32B32G32R32F,0x0       ,0x0        ,0x0        ,0x0        ,16     ,0      ,0          ,FALSE },
    /* Hmm? */
    {WINED3DFMT_CxV8U8      ,0x0        ,0x0        ,0x0        ,0x0        ,2      ,0      ,0          ,FALSE },
    /* Float */
    {WINED3DFMT_R16F        ,0x0        ,0x0        ,0x0        ,0x0        ,2      ,0      ,0          ,FALSE },
    {WINED3DFMT_G16R16F     ,0x0        ,0x0        ,0x0        ,0x0        ,4      ,0      ,0          ,FALSE },
    {WINED3DFMT_A16B16G16R16F,0x0       ,0x0        ,0x0        ,0x0        ,8      ,0      ,0          ,FALSE },
    /* Palettized formats */
    {WINED3DFMT_A8P8        ,0x0000ff00 ,0x0        ,0x0        ,0x0        ,2      ,0      ,0          ,FALSE },
    {WINED3DFMT_P8          ,0x0        ,0x0        ,0x0        ,0x0        ,1      ,0      ,0          ,FALSE },
    /* Standard ARGB formats. Keep WINED3DFMT_R8G8B8(=20) at position 20 */
    {WINED3DFMT_R8G8B8      ,0x0        ,0x00ff0000 ,0x0000ff00 ,0x000000ff ,3      ,0      ,0          ,FALSE },
    {WINED3DFMT_A8R8G8B8    ,0xff000000 ,0x00ff0000 ,0x0000ff00 ,0x000000ff ,4      ,0      ,0          ,FALSE },
    {WINED3DFMT_X8R8G8B8    ,0x0        ,0x00ff0000 ,0x0000ff00 ,0x000000ff ,4      ,0      ,0          ,FALSE },
    {WINED3DFMT_R5G6B5      ,0x0        ,0x0000F800 ,0x000007e0 ,0x0000001f ,2      ,0      ,0          ,FALSE },
    {WINED3DFMT_X1R5G5B5    ,0x0        ,0x00007c00 ,0x000003e0 ,0x0000001f ,2      ,0      ,0          ,FALSE },
    {WINED3DFMT_A1R5G5B5    ,0x00008000 ,0x00007c00 ,0x000003e0 ,0x0000001f ,2      ,0      ,0          ,FALSE },
    {WINED3DFMT_A4R4G4B4    ,0x0000f000 ,0x00000f00 ,0x000000f0 ,0x0000000f ,2      ,0      ,0          ,FALSE },
    {WINED3DFMT_R3G3B2      ,0x0        ,0x000000e0 ,0x0000001c ,0x00000003 ,1      ,0      ,0          ,FALSE },
    {WINED3DFMT_A8          ,0x000000ff ,0x0        ,0x0        ,0x0        ,1      ,0      ,0          ,FALSE },
    {WINED3DFMT_A8R3G3B2    ,0x0000ff00 ,0x000000e0 ,0x0000001c ,0x00000003 ,2      ,0      ,0          ,FALSE },
    {WINED3DFMT_X4R4G4B4    ,0x0        ,0x00000f00 ,0x000000f0 ,0x0000000f ,2      ,0      ,0          ,FALSE },
    {WINED3DFMT_A2B10G10R10 ,0xb0000000 ,0x000003ff ,0x000ffc00 ,0x3ff00000 ,4      ,0      ,0          ,FALSE },
    {WINED3DFMT_A8B8G8R8    ,0xff000000 ,0x000000ff ,0x0000ff00 ,0x00ff0000 ,4      ,0      ,0          ,FALSE },
    {WINED3DFMT_X8B8G8R8    ,0x0        ,0x000000ff ,0x0000ff00 ,0x00ff0000 ,4      ,0      ,0          ,FALSE },
    {WINED3DFMT_G16R16      ,0x0        ,0x0000ffff ,0xffff0000 ,0x0        ,4      ,0      ,0          ,FALSE },
    {WINED3DFMT_A2R10G10B10 ,0xb0000000 ,0x3ff00000 ,0x000ffc00 ,0x000003ff ,4      ,0      ,0          ,FALSE },
    {WINED3DFMT_A16B16G16R16,0x0        ,0x0000ffff ,0xffff0000 ,0x0        ,8      ,0      ,0          ,FALSE },
    /* Luminance */
    {WINED3DFMT_L8          ,0x0        ,0x0        ,0x0        ,0x0        ,1      ,0      ,0          ,FALSE },
    {WINED3DFMT_A8L8        ,0x0000ff00 ,0x0        ,0x0        ,0x0        ,2      ,0      ,0          ,FALSE },
    {WINED3DFMT_A4L4        ,0x000000f0 ,0x0        ,0x0        ,0x0        ,1      ,0      ,0          ,FALSE },
    /* Bump mapping stuff */
    {WINED3DFMT_V8U8        ,0x0        ,0x0        ,0x0        ,0x0        ,2      ,0      ,0          ,FALSE },
    {WINED3DFMT_L6V5U5      ,0x0        ,0x0        ,0x0        ,0x0        ,2      ,0      ,0          ,FALSE },
    {WINED3DFMT_X8L8V8U8    ,0x0        ,0x0        ,0x0        ,0x0        ,4      ,0      ,0          ,FALSE },
    {WINED3DFMT_Q8W8V8U8    ,0x0        ,0x0        ,0x0        ,0x0        ,4      ,0      ,0          ,FALSE },
    {WINED3DFMT_V16U16      ,0x0        ,0x0        ,0x0        ,0x0        ,4      ,0      ,0          ,FALSE },
    {WINED3DFMT_W11V11U10   ,0x0        ,0x0        ,0x0        ,0x0        ,4      ,0      ,0          ,FALSE },
    {WINED3DFMT_A2W10V10U10 ,0xb0000000 ,0x0        ,0x0        ,0x0        ,4      ,0      ,0          ,FALSE },
    /* Depth stencil formats */
    {WINED3DFMT_D16_LOCKABLE,0x0        ,0x0        ,0x0        ,0x0        ,2      ,16     ,0          ,FALSE },
    {WINED3DFMT_D32         ,0x0        ,0x0        ,0x0        ,0x0        ,4      ,32     ,0          ,FALSE },
    {WINED3DFMT_D15S1       ,0x0        ,0x0        ,0x0        ,0x0        ,2      ,15     ,1          ,FALSE },
    {WINED3DFMT_D24S8       ,0x0        ,0x0        ,0x0        ,0x0        ,4      ,24     ,8          ,FALSE },
    {WINED3DFMT_D24X8       ,0x0        ,0x0        ,0x0        ,0x0        ,4      ,24     ,0          ,FALSE },
    {WINED3DFMT_D24X4S4     ,0x0        ,0x0        ,0x0        ,0x0        ,4      ,24     ,4          ,FALSE },
    {WINED3DFMT_D16         ,0x0        ,0x0        ,0x0        ,0x0        ,2      ,16     ,0          ,FALSE },
    {WINED3DFMT_L16         ,0x0        ,0x0        ,0x0        ,0x0        ,2      ,16      ,0          ,FALSE },
    {WINED3DFMT_D32F_LOCKABLE,0x0       ,0x0        ,0x0        ,0x0        ,4      ,32     ,0          ,FALSE },
    {WINED3DFMT_D24FS8      ,0x0        ,0x0        ,0x0        ,0x0        ,4      ,24     ,8          ,FALSE },
    /* Is this a vertex buffer? */
    {WINED3DFMT_VERTEXDATA  ,0x0        ,0x0        ,0x0        ,0x0        ,0      ,0      ,0          ,FALSE },
    {WINED3DFMT_INDEX16     ,0x0        ,0x0        ,0x0        ,0x0        ,2      ,0      ,0          ,FALSE },
    {WINED3DFMT_INDEX32     ,0x0        ,0x0        ,0x0        ,0x0        ,4      ,0      ,0          ,FALSE },
    {WINED3DFMT_Q16W16V16U16,0x0        ,0x0        ,0x0        ,0x0        ,8      ,0      ,0          ,FALSE },
};

typedef struct {
    WINED3DFORMAT           fmt;
    GLint                   glInternal, glGammaInternal, glFormat, glType;
} GlPixelFormatDescTemplate;

/*****************************************************************************
 * OpenGL format template. Contains unexciting formats which do not need
 * extension checks. The order in this table is independent of the order in
 * the table StaticPixelFormatDesc above. Not all formats have to be in this
 * table.
 */
static const GlPixelFormatDescTemplate gl_formats_template[] = {
  /*{                           internal                         ,srgbInternal                           ,format                    ,type                           }*/
    {WINED3DFMT_UNKNOWN        ,0                                ,0                                      ,0                         ,0                              },
    /* FourCC formats */
    {WINED3DFMT_UYVY           ,0                                ,0                                      ,0                         ,0                              },
    {WINED3DFMT_YUY2           ,0                                ,0                                      ,0                         ,0                              },
    {WINED3DFMT_DXT1           ,GL_COMPRESSED_RGBA_S3TC_DXT1_EXT ,GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT ,GL_RGBA                   ,GL_UNSIGNED_BYTE               },
    {WINED3DFMT_DXT2           ,GL_COMPRESSED_RGBA_S3TC_DXT3_EXT ,GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT ,GL_RGBA                   ,GL_UNSIGNED_BYTE               },
    {WINED3DFMT_DXT3           ,GL_COMPRESSED_RGBA_S3TC_DXT3_EXT ,GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT ,GL_RGBA                   ,GL_UNSIGNED_BYTE               },
    {WINED3DFMT_DXT4           ,GL_COMPRESSED_RGBA_S3TC_DXT5_EXT ,GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT ,GL_RGBA                   ,GL_UNSIGNED_BYTE               },
    {WINED3DFMT_DXT5           ,GL_COMPRESSED_RGBA_S3TC_DXT5_EXT ,GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT ,GL_RGBA                   ,GL_UNSIGNED_BYTE               },
    {WINED3DFMT_MULTI2_ARGB8   ,0                                ,0                                      ,0                         ,0                              },
    {WINED3DFMT_G8R8_G8B8      ,0                                ,0                                      ,0                         ,0                              },
    {WINED3DFMT_R8G8_B8G8      ,0                                ,0                                      ,0                         ,0                              },
    /* IEEE formats */
    {WINED3DFMT_R32F           ,GL_RGB32F_ARB                    ,GL_RGB32F_ARB                          ,GL_RED                    ,GL_FLOAT                       },
    {WINED3DFMT_G32R32F        ,0                                ,0                                      ,0                         ,0                              },
    {WINED3DFMT_A32B32G32R32F  ,GL_RGBA32F_ARB                   ,GL_RGBA32F_ARB                         ,GL_RGBA                   ,GL_FLOAT                       },
    /* Hmm? */
    {WINED3DFMT_CxV8U8         ,0                                ,0                                      ,0                         ,0                              },
    /* Float */
    {WINED3DFMT_R16F           ,GL_RGB16F_ARB                    ,GL_RGB16F_ARB                          ,GL_RED                    ,GL_HALF_FLOAT_ARB              },
    {WINED3DFMT_G16R16F        ,0                                ,0                                      ,0                         ,0                              },
    {WINED3DFMT_A16B16G16R16F  ,GL_RGBA16F_ARB                   ,GL_RGBA16F_ARB                         ,GL_RGBA                   ,GL_HALF_FLOAT_ARB              },
    /* Palettized formats */
    {WINED3DFMT_A8P8,           0                                ,0                                      ,0                         ,0                              },
    {WINED3DFMT_P8,             GL_COLOR_INDEX8_EXT              ,GL_COLOR_INDEX8_EXT                    ,GL_COLOR_INDEX            ,GL_UNSIGNED_BYTE               },
    /* Standard ARGB formats */
    {WINED3DFMT_R8G8B8         ,GL_RGB8                          ,GL_RGB8                                ,GL_BGR                    ,GL_UNSIGNED_BYTE               },
    {WINED3DFMT_A8R8G8B8       ,GL_RGBA8                         ,GL_SRGB8_ALPHA8_EXT                    ,GL_BGRA                   ,GL_UNSIGNED_INT_8_8_8_8_REV    },
    {WINED3DFMT_X8R8G8B8       ,GL_RGB8                          ,GL_SRGB8_EXT                           ,GL_BGRA                   ,GL_UNSIGNED_INT_8_8_8_8_REV    },
    {WINED3DFMT_R5G6B5         ,GL_RGB5                          ,GL_RGB5                                ,GL_RGB                    ,GL_UNSIGNED_SHORT_5_6_5        },
    {WINED3DFMT_X1R5G5B5       ,GL_RGB5_A1                       ,GL_RGB5_A1                             ,GL_BGRA                   ,GL_UNSIGNED_SHORT_1_5_5_5_REV  },
    {WINED3DFMT_A1R5G5B5       ,GL_RGB5_A1                       ,GL_RGB5_A1                             ,GL_BGRA                   ,GL_UNSIGNED_SHORT_1_5_5_5_REV  },
    {WINED3DFMT_A4R4G4B4       ,GL_RGBA4                         ,GL_SRGB8_ALPHA8_EXT                    ,GL_BGRA                   ,GL_UNSIGNED_SHORT_4_4_4_4_REV  },
    {WINED3DFMT_R3G3B2         ,GL_R3_G3_B2                      ,GL_R3_G3_B2                            ,GL_RGB                    ,GL_UNSIGNED_BYTE_3_3_2         },
    {WINED3DFMT_A8             ,GL_ALPHA8                        ,GL_ALPHA8                              ,GL_ALPHA                  ,GL_UNSIGNED_BYTE               },
    {WINED3DFMT_A8R3G3B2       ,0                                ,0                                      ,0                         ,0                              },
    {WINED3DFMT_X4R4G4B4       ,GL_RGB4                          ,GL_RGB4                                ,GL_BGRA                   ,GL_UNSIGNED_SHORT_4_4_4_4_REV  },
    {WINED3DFMT_A2B10G10R10    ,GL_RGB                           ,GL_RGB                                 ,GL_RGBA                   ,GL_UNSIGNED_INT_2_10_10_10_REV },
    {WINED3DFMT_A8B8G8R8       ,GL_RGBA8                         ,GL_RGBA8                               ,GL_RGBA                   ,GL_UNSIGNED_INT_8_8_8_8_REV    },
    {WINED3DFMT_X8B8G8R8       ,GL_RGB8                          ,GL_RGB8                                ,GL_RGBA                   ,GL_UNSIGNED_INT_8_8_8_8_REV    },
    {WINED3DFMT_G16R16         ,0                                ,0                                      ,0                         ,0                              },
    {WINED3DFMT_A2R10G10B10    ,GL_RGBA                          ,GL_RGBA                                ,GL_BGRA                   ,GL_UNSIGNED_INT_2_10_10_10_REV },
    {WINED3DFMT_A16B16G16R16   ,GL_RGBA16_EXT                    ,GL_RGBA16_EXT                          ,GL_RGBA                   ,GL_UNSIGNED_SHORT              },
    /* Luminance */
    {WINED3DFMT_L8             ,GL_LUMINANCE8                    ,GL_SLUMINANCE8_EXT                     ,GL_LUMINANCE              ,GL_UNSIGNED_BYTE               },
    {WINED3DFMT_A8L8           ,GL_LUMINANCE8_ALPHA8             ,GL_SLUMINANCE8_ALPHA8_EXT              ,GL_LUMINANCE_ALPHA        ,GL_UNSIGNED_BYTE               },
    {WINED3DFMT_A4L4           ,GL_LUMINANCE4_ALPHA4             ,GL_LUMINANCE4_ALPHA4                   ,GL_LUMINANCE_ALPHA        ,GL_UNSIGNED_BYTE               },
    /* Bump mapping stuff */
    {WINED3DFMT_V8U8           ,GL_DSDT8_NV                      ,GL_DSDT8_NV                            ,GL_DSDT_NV                ,GL_BYTE                        },
    {WINED3DFMT_L6V5U5         ,GL_COLOR_INDEX8_EXT              ,GL_COLOR_INDEX8_EXT                    ,GL_COLOR_INDEX            ,GL_UNSIGNED_SHORT_5_5_5_1      },
    {WINED3DFMT_X8L8V8U8       ,GL_DSDT8_MAG8_INTENSITY8_NV      ,GL_DSDT8_MAG8_INTENSITY8_NV            ,GL_DSDT_MAG_INTENSITY_NV  ,GL_BYTE                        },
    {WINED3DFMT_Q8W8V8U8       ,GL_SIGNED_RGBA8_NV               ,GL_SIGNED_RGBA8_NV                     ,GL_RGBA                   ,GL_BYTE                        },
    {WINED3DFMT_V16U16         ,GL_SIGNED_HILO16_NV              ,GL_SIGNED_HILO16_NV                    ,GL_HILO_NV                ,GL_SHORT                       },
    {WINED3DFMT_W11V11U10      ,0                                ,0                                      ,0                         ,0                              },
    {WINED3DFMT_A2W10V10U10    ,0                                ,0                                      ,0                         ,0                              },
    /* Depth stencil formats */
    {WINED3DFMT_D16_LOCKABLE   ,GL_DEPTH_COMPONENT24_ARB         ,GL_DEPTH_COMPONENT24_ARB               ,GL_DEPTH_COMPONENT        ,GL_UNSIGNED_SHORT              },
    {WINED3DFMT_D32            ,GL_DEPTH_COMPONENT32_ARB         ,GL_DEPTH_COMPONENT32_ARB               ,GL_DEPTH_COMPONENT        ,GL_UNSIGNED_INT                },
    {WINED3DFMT_D15S1          ,GL_DEPTH_COMPONENT24_ARB         ,GL_DEPTH_COMPONENT24_ARB               ,GL_DEPTH_COMPONENT        ,GL_UNSIGNED_SHORT              },
    {WINED3DFMT_D24S8          ,GL_DEPTH_COMPONENT24_ARB         ,GL_DEPTH_COMPONENT24_ARB               ,GL_DEPTH_COMPONENT        ,GL_UNSIGNED_INT                },
    {WINED3DFMT_D24X8          ,GL_DEPTH_COMPONENT24_ARB         ,GL_DEPTH_COMPONENT24_ARB               ,GL_DEPTH_COMPONENT        ,GL_UNSIGNED_INT                },
    {WINED3DFMT_D24X4S4        ,GL_DEPTH_COMPONENT24_ARB         ,GL_DEPTH_COMPONENT24_ARB               ,GL_DEPTH_COMPONENT        ,GL_UNSIGNED_INT                },
    {WINED3DFMT_D16            ,GL_DEPTH_COMPONENT24_ARB         ,GL_DEPTH_COMPONENT24_ARB               ,GL_DEPTH_COMPONENT        ,GL_UNSIGNED_SHORT              },
    {WINED3DFMT_L16            ,GL_LUMINANCE16_EXT               ,GL_LUMINANCE16_EXT                     ,GL_LUMINANCE              ,GL_UNSIGNED_SHORT              },
    {WINED3DFMT_D32F_LOCKABLE  ,GL_DEPTH_COMPONENT32_ARB         ,GL_DEPTH_COMPONENT32_ARB               ,GL_DEPTH_COMPONENT        ,GL_FLOAT                       },
    {WINED3DFMT_D24FS8         ,GL_DEPTH_COMPONENT24_ARB         ,GL_DEPTH_COMPONENT24_ARB               ,GL_DEPTH_COMPONENT        ,GL_FLOAT                       },
    /* Is this a vertex buffer? */
    {WINED3DFMT_VERTEXDATA     ,0                                ,0                                      ,0                         ,0                              },
    {WINED3DFMT_INDEX16        ,0                                ,0                                      ,0                         ,0                              },
    {WINED3DFMT_INDEX32        ,0                                ,0                                      ,0                         ,0                              },
    {WINED3DFMT_Q16W16V16U16   ,GL_COLOR_INDEX                   ,GL_COLOR_INDEX                         ,GL_COLOR_INDEX            ,GL_UNSIGNED_SHORT              }
};

static inline int getFmtIdx(WINED3DFORMAT fmt) {
    /* First check if the format is at the position of its value.
     * This will catch the argb formats before the loop is entered
     */
    if(fmt < (sizeof(formats) / sizeof(formats[0])) && formats[fmt].format == fmt) {
        return fmt;
    } else {
        unsigned int i;
        for(i = 0; i < (sizeof(formats) / sizeof(formats[0])); i++) {
            if(formats[i].format == fmt) {
                return i;
            }
        }
    }
    return -1;
}

BOOL initPixelFormats(WineD3D_GL_Info *gl_info)
{
    unsigned int src;

    gl_info->gl_formats = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                                    sizeof(formats) / sizeof(formats[0]) * sizeof(gl_info->gl_formats[0]));
    if(!gl_info->gl_formats) return FALSE;

    /* If a format depends on some extensions, remove them from the table above and initialize them
     * after this loop
     */
    for(src = 0; src < sizeof(gl_formats_template) / sizeof(gl_formats_template[0]); src++) {
        int dst = getFmtIdx(gl_formats_template[src].fmt);
        gl_info->gl_formats[dst].glInternal      = gl_formats_template[src].glInternal;
        gl_info->gl_formats[dst].glGammaInternal = gl_formats_template[src].glGammaInternal;
        gl_info->gl_formats[dst].glFormat        = gl_formats_template[src].glFormat;
        gl_info->gl_formats[dst].glType          = gl_formats_template[src].glType;
    }

    return TRUE;
}

const StaticPixelFormatDesc *getFormatDescEntry(WINED3DFORMAT fmt, WineD3D_GL_Info *gl_info, const GlPixelFormatDesc **glDesc)
{
    int idx = getFmtIdx(fmt);

    if(idx == -1) {
        FIXME("Can't find format %s(%d) in the format lookup table\n", debug_d3dformat(fmt), fmt);
        /* Get the caller a valid pointer */
        idx = getFmtIdx(WINED3DFMT_UNKNOWN);
    }
    if(glDesc) {
        if(!gl_info) {
            ERR("OpenGL pixel format information was requested, but no gl info structure passed\n");
            return NULL;
        }
        *glDesc = &gl_info->gl_formats[idx];
    }
    return &formats[idx];
}

/*****************************************************************************
 * Trace formatting of useful values
 */
const char* debug_d3dformat(WINED3DFORMAT fmt) {
  switch (fmt) {
#define FMT_TO_STR(fmt) case fmt: return #fmt
    FMT_TO_STR(WINED3DFMT_UNKNOWN);
    FMT_TO_STR(WINED3DFMT_R8G8B8);
    FMT_TO_STR(WINED3DFMT_A8R8G8B8);
    FMT_TO_STR(WINED3DFMT_X8R8G8B8);
    FMT_TO_STR(WINED3DFMT_R5G6B5);
    FMT_TO_STR(WINED3DFMT_X1R5G5B5);
    FMT_TO_STR(WINED3DFMT_A1R5G5B5);
    FMT_TO_STR(WINED3DFMT_A4R4G4B4);
    FMT_TO_STR(WINED3DFMT_R3G3B2);
    FMT_TO_STR(WINED3DFMT_A8);
    FMT_TO_STR(WINED3DFMT_A8R3G3B2);
    FMT_TO_STR(WINED3DFMT_X4R4G4B4);
    FMT_TO_STR(WINED3DFMT_A2B10G10R10);
    FMT_TO_STR(WINED3DFMT_A8B8G8R8);
    FMT_TO_STR(WINED3DFMT_X8B8G8R8);
    FMT_TO_STR(WINED3DFMT_G16R16);
    FMT_TO_STR(WINED3DFMT_A2R10G10B10);
    FMT_TO_STR(WINED3DFMT_A16B16G16R16);
    FMT_TO_STR(WINED3DFMT_A8P8);
    FMT_TO_STR(WINED3DFMT_P8);
    FMT_TO_STR(WINED3DFMT_L8);
    FMT_TO_STR(WINED3DFMT_A8L8);
    FMT_TO_STR(WINED3DFMT_A4L4);
    FMT_TO_STR(WINED3DFMT_V8U8);
    FMT_TO_STR(WINED3DFMT_L6V5U5);
    FMT_TO_STR(WINED3DFMT_X8L8V8U8);
    FMT_TO_STR(WINED3DFMT_Q8W8V8U8);
    FMT_TO_STR(WINED3DFMT_V16U16);
    FMT_TO_STR(WINED3DFMT_W11V11U10);
    FMT_TO_STR(WINED3DFMT_A2W10V10U10);
    FMT_TO_STR(WINED3DFMT_UYVY);
    FMT_TO_STR(WINED3DFMT_YUY2);
    FMT_TO_STR(WINED3DFMT_DXT1);
    FMT_TO_STR(WINED3DFMT_DXT2);
    FMT_TO_STR(WINED3DFMT_DXT3);
    FMT_TO_STR(WINED3DFMT_DXT4);
    FMT_TO_STR(WINED3DFMT_DXT5);
    FMT_TO_STR(WINED3DFMT_MULTI2_ARGB8);
    FMT_TO_STR(WINED3DFMT_G8R8_G8B8);
    FMT_TO_STR(WINED3DFMT_R8G8_B8G8);
    FMT_TO_STR(WINED3DFMT_D16_LOCKABLE);
    FMT_TO_STR(WINED3DFMT_D32);
    FMT_TO_STR(WINED3DFMT_D15S1);
    FMT_TO_STR(WINED3DFMT_D24S8);
    FMT_TO_STR(WINED3DFMT_D24X8);
    FMT_TO_STR(WINED3DFMT_D24X4S4);
    FMT_TO_STR(WINED3DFMT_D16);
    FMT_TO_STR(WINED3DFMT_L16);
    FMT_TO_STR(WINED3DFMT_D32F_LOCKABLE);
    FMT_TO_STR(WINED3DFMT_D24FS8);
    FMT_TO_STR(WINED3DFMT_VERTEXDATA);
    FMT_TO_STR(WINED3DFMT_INDEX16);
    FMT_TO_STR(WINED3DFMT_INDEX32);
    FMT_TO_STR(WINED3DFMT_Q16W16V16U16);
    FMT_TO_STR(WINED3DFMT_R16F);
    FMT_TO_STR(WINED3DFMT_G16R16F);
    FMT_TO_STR(WINED3DFMT_A16B16G16R16F);
    FMT_TO_STR(WINED3DFMT_R32F);
    FMT_TO_STR(WINED3DFMT_G32R32F);
    FMT_TO_STR(WINED3DFMT_A32B32G32R32F);
    FMT_TO_STR(WINED3DFMT_CxV8U8);
#undef FMT_TO_STR
  default:
    {
      char fourcc[5];
      fourcc[0] = (char)(fmt);
      fourcc[1] = (char)(fmt >> 8);
      fourcc[2] = (char)(fmt >> 16);
      fourcc[3] = (char)(fmt >> 24);
      fourcc[4] = 0;
      if( isprint(fourcc[0]) && isprint(fourcc[1]) && isprint(fourcc[2]) && isprint(fourcc[3]) )
        FIXME("Unrecognized %u (as fourcc: %s) WINED3DFORMAT!\n", fmt, fourcc);
      else
        FIXME("Unrecognized %u WINED3DFORMAT!\n", fmt);
    }
    return "unrecognized";
  }
}

const char* debug_d3ddevicetype(WINED3DDEVTYPE devtype) {
  switch (devtype) {
#define DEVTYPE_TO_STR(dev) case dev: return #dev
    DEVTYPE_TO_STR(WINED3DDEVTYPE_HAL);
    DEVTYPE_TO_STR(WINED3DDEVTYPE_REF);
    DEVTYPE_TO_STR(WINED3DDEVTYPE_SW);
#undef DEVTYPE_TO_STR
  default:
    FIXME("Unrecognized %u WINED3DDEVTYPE!\n", devtype);
    return "unrecognized";
  }
}

const char* debug_d3dusage(DWORD usage) {
  switch (usage & WINED3DUSAGE_MASK) {
#define WINED3DUSAGE_TO_STR(u) case u: return #u
    WINED3DUSAGE_TO_STR(WINED3DUSAGE_RENDERTARGET);
    WINED3DUSAGE_TO_STR(WINED3DUSAGE_DEPTHSTENCIL);
    WINED3DUSAGE_TO_STR(WINED3DUSAGE_WRITEONLY);
    WINED3DUSAGE_TO_STR(WINED3DUSAGE_SOFTWAREPROCESSING);
    WINED3DUSAGE_TO_STR(WINED3DUSAGE_DONOTCLIP);
    WINED3DUSAGE_TO_STR(WINED3DUSAGE_POINTS);
    WINED3DUSAGE_TO_STR(WINED3DUSAGE_RTPATCHES);
    WINED3DUSAGE_TO_STR(WINED3DUSAGE_NPATCHES);
    WINED3DUSAGE_TO_STR(WINED3DUSAGE_DYNAMIC);
    WINED3DUSAGE_TO_STR(WINED3DUSAGE_AUTOGENMIPMAP);
    WINED3DUSAGE_TO_STR(WINED3DUSAGE_DMAP);
#undef WINED3DUSAGE_TO_STR
  case 0: return "none";
  default:
    FIXME("Unrecognized %u Usage!\n", usage);
    return "unrecognized";
  }
}

const char* debug_d3dusagequery(DWORD usagequery) {
  switch (usagequery & WINED3DUSAGE_QUERY_MASK) {
#define WINED3DUSAGEQUERY_TO_STR(u) case u: return #u
    WINED3DUSAGEQUERY_TO_STR(WINED3DUSAGE_QUERY_FILTER);
    WINED3DUSAGEQUERY_TO_STR(WINED3DUSAGE_QUERY_LEGACYBUMPMAP);
    WINED3DUSAGEQUERY_TO_STR(WINED3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING);
    WINED3DUSAGEQUERY_TO_STR(WINED3DUSAGE_QUERY_SRGBREAD);
    WINED3DUSAGEQUERY_TO_STR(WINED3DUSAGE_QUERY_SRGBWRITE);
    WINED3DUSAGEQUERY_TO_STR(WINED3DUSAGE_QUERY_VERTEXTEXTURE);
    WINED3DUSAGEQUERY_TO_STR(WINED3DUSAGE_QUERY_WRAPANDMIP);
#undef WINED3DUSAGEQUERY_TO_STR
  case 0: return "none";
  default:
    FIXME("Unrecognized %u Usage Query!\n", usagequery);
    return "unrecognized";
  }
}

const char* debug_d3ddeclmethod(WINED3DDECLMETHOD method) {
    switch (method) {
#define WINED3DDECLMETHOD_TO_STR(u) case u: return #u
        WINED3DDECLMETHOD_TO_STR(WINED3DDECLMETHOD_DEFAULT);
        WINED3DDECLMETHOD_TO_STR(WINED3DDECLMETHOD_PARTIALU);
        WINED3DDECLMETHOD_TO_STR(WINED3DDECLMETHOD_PARTIALV);
        WINED3DDECLMETHOD_TO_STR(WINED3DDECLMETHOD_CROSSUV);
        WINED3DDECLMETHOD_TO_STR(WINED3DDECLMETHOD_UV);
        WINED3DDECLMETHOD_TO_STR(WINED3DDECLMETHOD_LOOKUP);
        WINED3DDECLMETHOD_TO_STR(WINED3DDECLMETHOD_LOOKUPPRESAMPLED);
#undef WINED3DDECLMETHOD_TO_STR
        default:
            FIXME("Unrecognized %u declaration method!\n", method);
            return "unrecognized";
    }
}

const char* debug_d3ddecltype(WINED3DDECLTYPE type) {
    switch (type) {
#define WINED3DDECLTYPE_TO_STR(u) case u: return #u
        WINED3DDECLTYPE_TO_STR(WINED3DDECLTYPE_FLOAT1);
        WINED3DDECLTYPE_TO_STR(WINED3DDECLTYPE_FLOAT2);
        WINED3DDECLTYPE_TO_STR(WINED3DDECLTYPE_FLOAT3);
        WINED3DDECLTYPE_TO_STR(WINED3DDECLTYPE_FLOAT4);
        WINED3DDECLTYPE_TO_STR(WINED3DDECLTYPE_D3DCOLOR);
        WINED3DDECLTYPE_TO_STR(WINED3DDECLTYPE_UBYTE4);
        WINED3DDECLTYPE_TO_STR(WINED3DDECLTYPE_SHORT2);
        WINED3DDECLTYPE_TO_STR(WINED3DDECLTYPE_SHORT4);
        WINED3DDECLTYPE_TO_STR(WINED3DDECLTYPE_UBYTE4N);
        WINED3DDECLTYPE_TO_STR(WINED3DDECLTYPE_SHORT2N);
        WINED3DDECLTYPE_TO_STR(WINED3DDECLTYPE_SHORT4N);
        WINED3DDECLTYPE_TO_STR(WINED3DDECLTYPE_USHORT2N);
        WINED3DDECLTYPE_TO_STR(WINED3DDECLTYPE_USHORT4N);
        WINED3DDECLTYPE_TO_STR(WINED3DDECLTYPE_UDEC3);
        WINED3DDECLTYPE_TO_STR(WINED3DDECLTYPE_DEC3N);
        WINED3DDECLTYPE_TO_STR(WINED3DDECLTYPE_FLOAT16_2);
        WINED3DDECLTYPE_TO_STR(WINED3DDECLTYPE_FLOAT16_4);
        WINED3DDECLTYPE_TO_STR(WINED3DDECLTYPE_UNUSED);
#undef WINED3DDECLTYPE_TO_STR
        default:
            FIXME("Unrecognized %u declaration type!\n", type);
            return "unrecognized";
    }
}

const char* debug_d3ddeclusage(BYTE usage) {
    switch (usage) {
#define WINED3DDECLUSAGE_TO_STR(u) case u: return #u
        WINED3DDECLUSAGE_TO_STR(WINED3DDECLUSAGE_POSITION);
        WINED3DDECLUSAGE_TO_STR(WINED3DDECLUSAGE_BLENDWEIGHT);
        WINED3DDECLUSAGE_TO_STR(WINED3DDECLUSAGE_BLENDINDICES);
        WINED3DDECLUSAGE_TO_STR(WINED3DDECLUSAGE_NORMAL);
        WINED3DDECLUSAGE_TO_STR(WINED3DDECLUSAGE_PSIZE);
        WINED3DDECLUSAGE_TO_STR(WINED3DDECLUSAGE_TEXCOORD);
        WINED3DDECLUSAGE_TO_STR(WINED3DDECLUSAGE_TANGENT);
        WINED3DDECLUSAGE_TO_STR(WINED3DDECLUSAGE_BINORMAL);
        WINED3DDECLUSAGE_TO_STR(WINED3DDECLUSAGE_TESSFACTOR);
        WINED3DDECLUSAGE_TO_STR(WINED3DDECLUSAGE_POSITIONT);
        WINED3DDECLUSAGE_TO_STR(WINED3DDECLUSAGE_COLOR);
        WINED3DDECLUSAGE_TO_STR(WINED3DDECLUSAGE_FOG);
        WINED3DDECLUSAGE_TO_STR(WINED3DDECLUSAGE_DEPTH);
        WINED3DDECLUSAGE_TO_STR(WINED3DDECLUSAGE_SAMPLE);
#undef WINED3DDECLUSAGE_TO_STR
        default:
            FIXME("Unrecognized %u declaration usage!\n", usage);
            return "unrecognized";
    }
}

const char* debug_d3dresourcetype(WINED3DRESOURCETYPE res) {
  switch (res) {
#define RES_TO_STR(res) case res: return #res;
    RES_TO_STR(WINED3DRTYPE_SURFACE);
    RES_TO_STR(WINED3DRTYPE_VOLUME);
    RES_TO_STR(WINED3DRTYPE_TEXTURE);
    RES_TO_STR(WINED3DRTYPE_VOLUMETEXTURE);
    RES_TO_STR(WINED3DRTYPE_CUBETEXTURE);
    RES_TO_STR(WINED3DRTYPE_VERTEXBUFFER);
    RES_TO_STR(WINED3DRTYPE_INDEXBUFFER);
#undef  RES_TO_STR
  default:
    FIXME("Unrecognized %u WINED3DRESOURCETYPE!\n", res);
    return "unrecognized";
  }
}

const char* debug_d3dprimitivetype(WINED3DPRIMITIVETYPE PrimitiveType) {
  switch (PrimitiveType) {
#define PRIM_TO_STR(prim) case prim: return #prim;
    PRIM_TO_STR(WINED3DPT_POINTLIST);
    PRIM_TO_STR(WINED3DPT_LINELIST);
    PRIM_TO_STR(WINED3DPT_LINESTRIP);
    PRIM_TO_STR(WINED3DPT_TRIANGLELIST);
    PRIM_TO_STR(WINED3DPT_TRIANGLESTRIP);
    PRIM_TO_STR(WINED3DPT_TRIANGLEFAN);
#undef  PRIM_TO_STR
  default:
    FIXME("Unrecognized %u WINED3DPRIMITIVETYPE!\n", PrimitiveType);
    return "unrecognized";
  }
}

const char* debug_d3drenderstate(DWORD state) {
  switch (state) {
#define D3DSTATE_TO_STR(u) case u: return #u
    D3DSTATE_TO_STR(WINED3DRS_TEXTUREHANDLE             );
    D3DSTATE_TO_STR(WINED3DRS_ANTIALIAS                 );
    D3DSTATE_TO_STR(WINED3DRS_TEXTUREADDRESS            );
    D3DSTATE_TO_STR(WINED3DRS_TEXTUREPERSPECTIVE        );
    D3DSTATE_TO_STR(WINED3DRS_WRAPU                     );
    D3DSTATE_TO_STR(WINED3DRS_WRAPV                     );
    D3DSTATE_TO_STR(WINED3DRS_ZENABLE                   );
    D3DSTATE_TO_STR(WINED3DRS_FILLMODE                  );
    D3DSTATE_TO_STR(WINED3DRS_SHADEMODE                 );
    D3DSTATE_TO_STR(WINED3DRS_LINEPATTERN               );
    D3DSTATE_TO_STR(WINED3DRS_MONOENABLE                );
    D3DSTATE_TO_STR(WINED3DRS_ROP2                      );
    D3DSTATE_TO_STR(WINED3DRS_PLANEMASK                 );
    D3DSTATE_TO_STR(WINED3DRS_ZWRITEENABLE              );
    D3DSTATE_TO_STR(WINED3DRS_ALPHATESTENABLE           );
    D3DSTATE_TO_STR(WINED3DRS_LASTPIXEL                 );
    D3DSTATE_TO_STR(WINED3DRS_TEXTUREMAG                );
    D3DSTATE_TO_STR(WINED3DRS_TEXTUREMIN                );
    D3DSTATE_TO_STR(WINED3DRS_SRCBLEND                  );
    D3DSTATE_TO_STR(WINED3DRS_DESTBLEND                 );
    D3DSTATE_TO_STR(WINED3DRS_TEXTUREMAPBLEND           );
    D3DSTATE_TO_STR(WINED3DRS_CULLMODE                  );
    D3DSTATE_TO_STR(WINED3DRS_ZFUNC                     );
    D3DSTATE_TO_STR(WINED3DRS_ALPHAREF                  );
    D3DSTATE_TO_STR(WINED3DRS_ALPHAFUNC                 );
    D3DSTATE_TO_STR(WINED3DRS_DITHERENABLE              );
    D3DSTATE_TO_STR(WINED3DRS_ALPHABLENDENABLE          );
    D3DSTATE_TO_STR(WINED3DRS_FOGENABLE                 );
    D3DSTATE_TO_STR(WINED3DRS_SPECULARENABLE            );
    D3DSTATE_TO_STR(WINED3DRS_ZVISIBLE                  );
    D3DSTATE_TO_STR(WINED3DRS_SUBPIXEL                  );
    D3DSTATE_TO_STR(WINED3DRS_SUBPIXELX                 );
    D3DSTATE_TO_STR(WINED3DRS_STIPPLEDALPHA             );
    D3DSTATE_TO_STR(WINED3DRS_FOGCOLOR                  );
    D3DSTATE_TO_STR(WINED3DRS_FOGTABLEMODE              );
    D3DSTATE_TO_STR(WINED3DRS_FOGSTART                  );
    D3DSTATE_TO_STR(WINED3DRS_FOGEND                    );
    D3DSTATE_TO_STR(WINED3DRS_FOGDENSITY                );
    D3DSTATE_TO_STR(WINED3DRS_STIPPLEENABLE             );
    D3DSTATE_TO_STR(WINED3DRS_EDGEANTIALIAS             );
    D3DSTATE_TO_STR(WINED3DRS_COLORKEYENABLE            );
    D3DSTATE_TO_STR(WINED3DRS_BORDERCOLOR               );
    D3DSTATE_TO_STR(WINED3DRS_TEXTUREADDRESSU           );
    D3DSTATE_TO_STR(WINED3DRS_TEXTUREADDRESSV           );
    D3DSTATE_TO_STR(WINED3DRS_MIPMAPLODBIAS             );
    D3DSTATE_TO_STR(WINED3DRS_ZBIAS                     );
    D3DSTATE_TO_STR(WINED3DRS_RANGEFOGENABLE            );
    D3DSTATE_TO_STR(WINED3DRS_ANISOTROPY                );
    D3DSTATE_TO_STR(WINED3DRS_FLUSHBATCH                );
    D3DSTATE_TO_STR(WINED3DRS_TRANSLUCENTSORTINDEPENDENT);
    D3DSTATE_TO_STR(WINED3DRS_STENCILENABLE             );
    D3DSTATE_TO_STR(WINED3DRS_STENCILFAIL               );
    D3DSTATE_TO_STR(WINED3DRS_STENCILZFAIL              );
    D3DSTATE_TO_STR(WINED3DRS_STENCILPASS               );
    D3DSTATE_TO_STR(WINED3DRS_STENCILFUNC               );
    D3DSTATE_TO_STR(WINED3DRS_STENCILREF                );
    D3DSTATE_TO_STR(WINED3DRS_STENCILMASK               );
    D3DSTATE_TO_STR(WINED3DRS_STENCILWRITEMASK          );
    D3DSTATE_TO_STR(WINED3DRS_TEXTUREFACTOR             );
    D3DSTATE_TO_STR(WINED3DRS_STIPPLEPATTERN00          );
    D3DSTATE_TO_STR(WINED3DRS_STIPPLEPATTERN01          );
    D3DSTATE_TO_STR(WINED3DRS_STIPPLEPATTERN02          );
    D3DSTATE_TO_STR(WINED3DRS_STIPPLEPATTERN03          );
    D3DSTATE_TO_STR(WINED3DRS_STIPPLEPATTERN04          );
    D3DSTATE_TO_STR(WINED3DRS_STIPPLEPATTERN05          );
    D3DSTATE_TO_STR(WINED3DRS_STIPPLEPATTERN06          );
    D3DSTATE_TO_STR(WINED3DRS_STIPPLEPATTERN07          );
    D3DSTATE_TO_STR(WINED3DRS_STIPPLEPATTERN08          );
    D3DSTATE_TO_STR(WINED3DRS_STIPPLEPATTERN09          );
    D3DSTATE_TO_STR(WINED3DRS_STIPPLEPATTERN10          );
    D3DSTATE_TO_STR(WINED3DRS_STIPPLEPATTERN11          );
    D3DSTATE_TO_STR(WINED3DRS_STIPPLEPATTERN12          );
    D3DSTATE_TO_STR(WINED3DRS_STIPPLEPATTERN13          );
    D3DSTATE_TO_STR(WINED3DRS_STIPPLEPATTERN14          );
    D3DSTATE_TO_STR(WINED3DRS_STIPPLEPATTERN15          );
    D3DSTATE_TO_STR(WINED3DRS_STIPPLEPATTERN16          );
    D3DSTATE_TO_STR(WINED3DRS_STIPPLEPATTERN17          );
    D3DSTATE_TO_STR(WINED3DRS_STIPPLEPATTERN18          );
    D3DSTATE_TO_STR(WINED3DRS_STIPPLEPATTERN19          );
    D3DSTATE_TO_STR(WINED3DRS_STIPPLEPATTERN20          );
    D3DSTATE_TO_STR(WINED3DRS_STIPPLEPATTERN21          );
    D3DSTATE_TO_STR(WINED3DRS_STIPPLEPATTERN22          );
    D3DSTATE_TO_STR(WINED3DRS_STIPPLEPATTERN23          );
    D3DSTATE_TO_STR(WINED3DRS_STIPPLEPATTERN24          );
    D3DSTATE_TO_STR(WINED3DRS_STIPPLEPATTERN25          );
    D3DSTATE_TO_STR(WINED3DRS_STIPPLEPATTERN26          );
    D3DSTATE_TO_STR(WINED3DRS_STIPPLEPATTERN27          );
    D3DSTATE_TO_STR(WINED3DRS_STIPPLEPATTERN28          );
    D3DSTATE_TO_STR(WINED3DRS_STIPPLEPATTERN29          );
    D3DSTATE_TO_STR(WINED3DRS_STIPPLEPATTERN30          );
    D3DSTATE_TO_STR(WINED3DRS_STIPPLEPATTERN31          );
    D3DSTATE_TO_STR(WINED3DRS_WRAP0                     );
    D3DSTATE_TO_STR(WINED3DRS_WRAP1                     );
    D3DSTATE_TO_STR(WINED3DRS_WRAP2                     );
    D3DSTATE_TO_STR(WINED3DRS_WRAP3                     );
    D3DSTATE_TO_STR(WINED3DRS_WRAP4                     );
    D3DSTATE_TO_STR(WINED3DRS_WRAP5                     );
    D3DSTATE_TO_STR(WINED3DRS_WRAP6                     );
    D3DSTATE_TO_STR(WINED3DRS_WRAP7                     );
    D3DSTATE_TO_STR(WINED3DRS_CLIPPING                  );
    D3DSTATE_TO_STR(WINED3DRS_LIGHTING                  );
    D3DSTATE_TO_STR(WINED3DRS_EXTENTS                   );
    D3DSTATE_TO_STR(WINED3DRS_AMBIENT                   );
    D3DSTATE_TO_STR(WINED3DRS_FOGVERTEXMODE             );
    D3DSTATE_TO_STR(WINED3DRS_COLORVERTEX               );
    D3DSTATE_TO_STR(WINED3DRS_LOCALVIEWER               );
    D3DSTATE_TO_STR(WINED3DRS_NORMALIZENORMALS          );
    D3DSTATE_TO_STR(WINED3DRS_COLORKEYBLENDENABLE       );
    D3DSTATE_TO_STR(WINED3DRS_DIFFUSEMATERIALSOURCE     );
    D3DSTATE_TO_STR(WINED3DRS_SPECULARMATERIALSOURCE    );
    D3DSTATE_TO_STR(WINED3DRS_AMBIENTMATERIALSOURCE     );
    D3DSTATE_TO_STR(WINED3DRS_EMISSIVEMATERIALSOURCE    );
    D3DSTATE_TO_STR(WINED3DRS_VERTEXBLEND               );
    D3DSTATE_TO_STR(WINED3DRS_CLIPPLANEENABLE           );
    D3DSTATE_TO_STR(WINED3DRS_SOFTWAREVERTEXPROCESSING  );
    D3DSTATE_TO_STR(WINED3DRS_POINTSIZE                 );
    D3DSTATE_TO_STR(WINED3DRS_POINTSIZE_MIN             );
    D3DSTATE_TO_STR(WINED3DRS_POINTSPRITEENABLE         );
    D3DSTATE_TO_STR(WINED3DRS_POINTSCALEENABLE          );
    D3DSTATE_TO_STR(WINED3DRS_POINTSCALE_A              );
    D3DSTATE_TO_STR(WINED3DRS_POINTSCALE_B              );
    D3DSTATE_TO_STR(WINED3DRS_POINTSCALE_C              );
    D3DSTATE_TO_STR(WINED3DRS_MULTISAMPLEANTIALIAS      );
    D3DSTATE_TO_STR(WINED3DRS_MULTISAMPLEMASK           );
    D3DSTATE_TO_STR(WINED3DRS_PATCHEDGESTYLE            );
    D3DSTATE_TO_STR(WINED3DRS_PATCHSEGMENTS             );
    D3DSTATE_TO_STR(WINED3DRS_DEBUGMONITORTOKEN         );
    D3DSTATE_TO_STR(WINED3DRS_POINTSIZE_MAX             );
    D3DSTATE_TO_STR(WINED3DRS_INDEXEDVERTEXBLENDENABLE  );
    D3DSTATE_TO_STR(WINED3DRS_COLORWRITEENABLE          );
    D3DSTATE_TO_STR(WINED3DRS_TWEENFACTOR               );
    D3DSTATE_TO_STR(WINED3DRS_BLENDOP                   );
    D3DSTATE_TO_STR(WINED3DRS_POSITIONDEGREE            );
    D3DSTATE_TO_STR(WINED3DRS_NORMALDEGREE              );
    D3DSTATE_TO_STR(WINED3DRS_SCISSORTESTENABLE         );
    D3DSTATE_TO_STR(WINED3DRS_SLOPESCALEDEPTHBIAS       );
    D3DSTATE_TO_STR(WINED3DRS_ANTIALIASEDLINEENABLE     );
    D3DSTATE_TO_STR(WINED3DRS_MINTESSELLATIONLEVEL      );
    D3DSTATE_TO_STR(WINED3DRS_MAXTESSELLATIONLEVEL      );
    D3DSTATE_TO_STR(WINED3DRS_ADAPTIVETESS_X            );
    D3DSTATE_TO_STR(WINED3DRS_ADAPTIVETESS_Y            );
    D3DSTATE_TO_STR(WINED3DRS_ADAPTIVETESS_Z            );
    D3DSTATE_TO_STR(WINED3DRS_ADAPTIVETESS_W            );
    D3DSTATE_TO_STR(WINED3DRS_ENABLEADAPTIVETESSELLATION);
    D3DSTATE_TO_STR(WINED3DRS_TWOSIDEDSTENCILMODE       );
    D3DSTATE_TO_STR(WINED3DRS_CCW_STENCILFAIL           );
    D3DSTATE_TO_STR(WINED3DRS_CCW_STENCILZFAIL          );
    D3DSTATE_TO_STR(WINED3DRS_CCW_STENCILPASS           );
    D3DSTATE_TO_STR(WINED3DRS_CCW_STENCILFUNC           );
    D3DSTATE_TO_STR(WINED3DRS_COLORWRITEENABLE1         );
    D3DSTATE_TO_STR(WINED3DRS_COLORWRITEENABLE2         );
    D3DSTATE_TO_STR(WINED3DRS_COLORWRITEENABLE3         );
    D3DSTATE_TO_STR(WINED3DRS_BLENDFACTOR               );
    D3DSTATE_TO_STR(WINED3DRS_SRGBWRITEENABLE           );
    D3DSTATE_TO_STR(WINED3DRS_DEPTHBIAS                 );
    D3DSTATE_TO_STR(WINED3DRS_WRAP8                     );
    D3DSTATE_TO_STR(WINED3DRS_WRAP9                     );
    D3DSTATE_TO_STR(WINED3DRS_WRAP10                    );
    D3DSTATE_TO_STR(WINED3DRS_WRAP11                    );
    D3DSTATE_TO_STR(WINED3DRS_WRAP12                    );
    D3DSTATE_TO_STR(WINED3DRS_WRAP13                    );
    D3DSTATE_TO_STR(WINED3DRS_WRAP14                    );
    D3DSTATE_TO_STR(WINED3DRS_WRAP15                    );
    D3DSTATE_TO_STR(WINED3DRS_SEPARATEALPHABLENDENABLE  );
    D3DSTATE_TO_STR(WINED3DRS_SRCBLENDALPHA             );
    D3DSTATE_TO_STR(WINED3DRS_DESTBLENDALPHA            );
    D3DSTATE_TO_STR(WINED3DRS_BLENDOPALPHA              );
#undef D3DSTATE_TO_STR
  default:
    FIXME("Unrecognized %u render state!\n", state);
    return "unrecognized";
  }
}

const char* debug_d3dsamplerstate(DWORD state) {
  switch (state) {
#define D3DSTATE_TO_STR(u) case u: return #u
    D3DSTATE_TO_STR(WINED3DSAMP_BORDERCOLOR  );
    D3DSTATE_TO_STR(WINED3DSAMP_ADDRESSU     );
    D3DSTATE_TO_STR(WINED3DSAMP_ADDRESSV     );
    D3DSTATE_TO_STR(WINED3DSAMP_ADDRESSW     );
    D3DSTATE_TO_STR(WINED3DSAMP_MAGFILTER    );
    D3DSTATE_TO_STR(WINED3DSAMP_MINFILTER    );
    D3DSTATE_TO_STR(WINED3DSAMP_MIPFILTER    );
    D3DSTATE_TO_STR(WINED3DSAMP_MIPMAPLODBIAS);
    D3DSTATE_TO_STR(WINED3DSAMP_MAXMIPLEVEL  );
    D3DSTATE_TO_STR(WINED3DSAMP_MAXANISOTROPY);
    D3DSTATE_TO_STR(WINED3DSAMP_SRGBTEXTURE  );
    D3DSTATE_TO_STR(WINED3DSAMP_ELEMENTINDEX );
    D3DSTATE_TO_STR(WINED3DSAMP_DMAPOFFSET   );
#undef D3DSTATE_TO_STR
  default:
    FIXME("Unrecognized %u sampler state!\n", state);
    return "unrecognized";
  }
}

const char *debug_d3dtexturefiltertype(WINED3DTEXTUREFILTERTYPE filter_type) {
    switch (filter_type) {
#define D3DTEXTUREFILTERTYPE_TO_STR(u) case u: return #u
        D3DTEXTUREFILTERTYPE_TO_STR(WINED3DTEXF_NONE);
        D3DTEXTUREFILTERTYPE_TO_STR(WINED3DTEXF_POINT);
        D3DTEXTUREFILTERTYPE_TO_STR(WINED3DTEXF_LINEAR);
        D3DTEXTUREFILTERTYPE_TO_STR(WINED3DTEXF_ANISOTROPIC);
        D3DTEXTUREFILTERTYPE_TO_STR(WINED3DTEXF_FLATCUBIC);
        D3DTEXTUREFILTERTYPE_TO_STR(WINED3DTEXF_GAUSSIANCUBIC);
        D3DTEXTUREFILTERTYPE_TO_STR(WINED3DTEXF_PYRAMIDALQUAD);
        D3DTEXTUREFILTERTYPE_TO_STR(WINED3DTEXF_GAUSSIANQUAD);
#undef D3DTEXTUREFILTERTYPE_TO_STR
        default:
            FIXME("Unrecognied texture filter type 0x%08x\n", filter_type);
            return "unrecognized";
    }
}

const char* debug_d3dtexturestate(DWORD state) {
  switch (state) {
#define D3DSTATE_TO_STR(u) case u: return #u
    D3DSTATE_TO_STR(WINED3DTSS_COLOROP               );
    D3DSTATE_TO_STR(WINED3DTSS_COLORARG1             );
    D3DSTATE_TO_STR(WINED3DTSS_COLORARG2             );
    D3DSTATE_TO_STR(WINED3DTSS_ALPHAOP               );
    D3DSTATE_TO_STR(WINED3DTSS_ALPHAARG1             );
    D3DSTATE_TO_STR(WINED3DTSS_ALPHAARG2             );
    D3DSTATE_TO_STR(WINED3DTSS_BUMPENVMAT00          );
    D3DSTATE_TO_STR(WINED3DTSS_BUMPENVMAT01          );
    D3DSTATE_TO_STR(WINED3DTSS_BUMPENVMAT10          );
    D3DSTATE_TO_STR(WINED3DTSS_BUMPENVMAT11          );
    D3DSTATE_TO_STR(WINED3DTSS_TEXCOORDINDEX         );
    D3DSTATE_TO_STR(WINED3DTSS_BUMPENVLSCALE         );
    D3DSTATE_TO_STR(WINED3DTSS_BUMPENVLOFFSET        );
    D3DSTATE_TO_STR(WINED3DTSS_TEXTURETRANSFORMFLAGS );
    D3DSTATE_TO_STR(WINED3DTSS_ADDRESSW              );
    D3DSTATE_TO_STR(WINED3DTSS_COLORARG0             );
    D3DSTATE_TO_STR(WINED3DTSS_ALPHAARG0             );
    D3DSTATE_TO_STR(WINED3DTSS_RESULTARG             );
    D3DSTATE_TO_STR(WINED3DTSS_CONSTANT              );
#undef D3DSTATE_TO_STR
  case 12:
    /* Note WINED3DTSS are not consecutive, so skip these */
    return "unused";
  default:
    FIXME("Unrecognized %u texture state!\n", state);
    return "unrecognized";
  }
}

static const char* debug_d3dtop(WINED3DTEXTUREOP d3dtop) {
    switch (d3dtop) {
#define D3DTOP_TO_STR(u) case u: return #u
        D3DTOP_TO_STR(WINED3DTOP_DISABLE);
        D3DTOP_TO_STR(WINED3DTOP_SELECTARG1);
        D3DTOP_TO_STR(WINED3DTOP_SELECTARG2);
        D3DTOP_TO_STR(WINED3DTOP_MODULATE);
        D3DTOP_TO_STR(WINED3DTOP_MODULATE2X);
        D3DTOP_TO_STR(WINED3DTOP_MODULATE4X);
        D3DTOP_TO_STR(WINED3DTOP_ADD);
        D3DTOP_TO_STR(WINED3DTOP_ADDSIGNED);
        D3DTOP_TO_STR(WINED3DTOP_SUBTRACT);
        D3DTOP_TO_STR(WINED3DTOP_ADDSMOOTH);
        D3DTOP_TO_STR(WINED3DTOP_BLENDDIFFUSEALPHA);
        D3DTOP_TO_STR(WINED3DTOP_BLENDTEXTUREALPHA);
        D3DTOP_TO_STR(WINED3DTOP_BLENDFACTORALPHA);
        D3DTOP_TO_STR(WINED3DTOP_BLENDTEXTUREALPHAPM);
        D3DTOP_TO_STR(WINED3DTOP_BLENDCURRENTALPHA);
        D3DTOP_TO_STR(WINED3DTOP_PREMODULATE);
        D3DTOP_TO_STR(WINED3DTOP_MODULATEALPHA_ADDCOLOR);
        D3DTOP_TO_STR(WINED3DTOP_MODULATECOLOR_ADDALPHA);
        D3DTOP_TO_STR(WINED3DTOP_MODULATEINVALPHA_ADDCOLOR);
        D3DTOP_TO_STR(WINED3DTOP_MODULATEINVCOLOR_ADDALPHA);
        D3DTOP_TO_STR(WINED3DTOP_BUMPENVMAP);
        D3DTOP_TO_STR(WINED3DTOP_BUMPENVMAPLUMINANCE);
        D3DTOP_TO_STR(WINED3DTOP_DOTPRODUCT3);
        D3DTOP_TO_STR(WINED3DTOP_MULTIPLYADD);
        D3DTOP_TO_STR(WINED3DTOP_LERP);
#undef D3DTOP_TO_STR
        default:
            FIXME("Unrecognized %u WINED3DTOP\n", d3dtop);
            return "unrecognized";
    }
}

const char* debug_d3dtstype(WINED3DTRANSFORMSTATETYPE tstype) {
    switch (tstype) {
#define TSTYPE_TO_STR(tstype) case tstype: return #tstype
    TSTYPE_TO_STR(WINED3DTS_VIEW);
    TSTYPE_TO_STR(WINED3DTS_PROJECTION);
    TSTYPE_TO_STR(WINED3DTS_TEXTURE0);
    TSTYPE_TO_STR(WINED3DTS_TEXTURE1);
    TSTYPE_TO_STR(WINED3DTS_TEXTURE2);
    TSTYPE_TO_STR(WINED3DTS_TEXTURE3);
    TSTYPE_TO_STR(WINED3DTS_TEXTURE4);
    TSTYPE_TO_STR(WINED3DTS_TEXTURE5);
    TSTYPE_TO_STR(WINED3DTS_TEXTURE6);
    TSTYPE_TO_STR(WINED3DTS_TEXTURE7);
    TSTYPE_TO_STR(WINED3DTS_WORLDMATRIX(0));
#undef TSTYPE_TO_STR
    default:
        if (tstype > 256 && tstype < 512) {
            FIXME("WINED3DTS_WORLDMATRIX(%u). 1..255 not currently supported\n", tstype);
            return ("WINED3DTS_WORLDMATRIX > 0");
        }
        FIXME("Unrecognized %u WINED3DTS\n", tstype);
        return "unrecognized";
    }
}

const char* debug_d3dpool(WINED3DPOOL Pool) {
  switch (Pool) {
#define POOL_TO_STR(p) case p: return #p;
    POOL_TO_STR(WINED3DPOOL_DEFAULT);
    POOL_TO_STR(WINED3DPOOL_MANAGED);
    POOL_TO_STR(WINED3DPOOL_SYSTEMMEM);
    POOL_TO_STR(WINED3DPOOL_SCRATCH);
#undef  POOL_TO_STR
  default:
    FIXME("Unrecognized %u WINED3DPOOL!\n", Pool);
    return "unrecognized";
  }
}

const char *debug_fbostatus(GLenum status) {
    switch(status) {
#define FBOSTATUS_TO_STR(u) case u: return #u
        FBOSTATUS_TO_STR(GL_FRAMEBUFFER_COMPLETE_EXT);
        FBOSTATUS_TO_STR(GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT);
        FBOSTATUS_TO_STR(GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT);
        FBOSTATUS_TO_STR(GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT);
        FBOSTATUS_TO_STR(GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT);
        FBOSTATUS_TO_STR(GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT);
        FBOSTATUS_TO_STR(GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT);
        FBOSTATUS_TO_STR(GL_FRAMEBUFFER_UNSUPPORTED_EXT);
#undef FBOSTATUS_TO_STR
        default:
            FIXME("Unrecognied FBO status 0x%08x\n", status);
            return "unrecognized";
    }
}

const char *debug_glerror(GLenum error) {
    switch(error) {
#define GLERROR_TO_STR(u) case u: return #u
        GLERROR_TO_STR(GL_NO_ERROR);
        GLERROR_TO_STR(GL_INVALID_ENUM);
        GLERROR_TO_STR(GL_INVALID_VALUE);
        GLERROR_TO_STR(GL_INVALID_OPERATION);
        GLERROR_TO_STR(GL_STACK_OVERFLOW);
        GLERROR_TO_STR(GL_STACK_UNDERFLOW);
        GLERROR_TO_STR(GL_OUT_OF_MEMORY);
        GLERROR_TO_STR(GL_INVALID_FRAMEBUFFER_OPERATION_EXT);
#undef GLERROR_TO_STR
        default:
            FIXME("Unrecognied GL error 0x%08x\n", error);
            return "unrecognized";
    }
}

const char *debug_d3dbasis(WINED3DBASISTYPE basis) {
    switch(basis) {
        case WINED3DBASIS_BEZIER:       return "WINED3DBASIS_BEZIER";
        case WINED3DBASIS_BSPLINE:      return "WINED3DBASIS_BSPLINE";
        case WINED3DBASIS_INTERPOLATE:  return "WINED3DBASIS_INTERPOLATE";
        default:                        return "unrecognized";
    }
}

const char *debug_d3ddegree(WINED3DDEGREETYPE degree) {
    switch(degree) {
        case WINED3DDEGREE_LINEAR:      return "WINED3DDEGREE_LINEAR";
        case WINED3DDEGREE_QUADRATIC:   return "WINED3DDEGREE_QUADRATIC";
        case WINED3DDEGREE_CUBIC:       return "WINED3DDEGREE_CUBIC";
        case WINED3DDEGREE_QUINTIC:     return "WINED3DDEGREE_QUINTIC";
        default:                        return "unrecognized";
    }
}

/*****************************************************************************
 * Useful functions mapping GL <-> D3D values
 */
GLenum StencilOp(DWORD op) {
    switch(op) {
    case WINED3DSTENCILOP_KEEP    : return GL_KEEP;
    case WINED3DSTENCILOP_ZERO    : return GL_ZERO;
    case WINED3DSTENCILOP_REPLACE : return GL_REPLACE;
    case WINED3DSTENCILOP_INCRSAT : return GL_INCR;
    case WINED3DSTENCILOP_DECRSAT : return GL_DECR;
    case WINED3DSTENCILOP_INVERT  : return GL_INVERT;
    case WINED3DSTENCILOP_INCR    : return GL_INCR_WRAP_EXT;
    case WINED3DSTENCILOP_DECR    : return GL_DECR_WRAP_EXT;
    default:
        FIXME("Unrecognized stencil op %d\n", op);
        return GL_KEEP;
    }
}

GLenum CompareFunc(DWORD func) {
    switch ((WINED3DCMPFUNC)func) {
    case WINED3DCMP_NEVER        : return GL_NEVER;
    case WINED3DCMP_LESS         : return GL_LESS;
    case WINED3DCMP_EQUAL        : return GL_EQUAL;
    case WINED3DCMP_LESSEQUAL    : return GL_LEQUAL;
    case WINED3DCMP_GREATER      : return GL_GREATER;
    case WINED3DCMP_NOTEQUAL     : return GL_NOTEQUAL;
    case WINED3DCMP_GREATEREQUAL : return GL_GEQUAL;
    case WINED3DCMP_ALWAYS       : return GL_ALWAYS;
    default:
        FIXME("Unrecognized WINED3DCMPFUNC value %d\n", func);
        return 0;
    }
}

static GLenum d3dta_to_combiner_input(DWORD d3dta, DWORD stage, INT texture_idx) {
    switch (d3dta) {
        case WINED3DTA_DIFFUSE:
            return GL_PRIMARY_COLOR_NV;

        case WINED3DTA_CURRENT:
            if (stage) return GL_SPARE0_NV;
            else return GL_PRIMARY_COLOR_NV;

        case WINED3DTA_TEXTURE:
            if (texture_idx > -1) return GL_TEXTURE0_ARB + texture_idx;
            else return GL_PRIMARY_COLOR_NV;

        case WINED3DTA_TFACTOR:
            return GL_CONSTANT_COLOR0_NV;

        case WINED3DTA_SPECULAR:
            return GL_SECONDARY_COLOR_NV;

        case WINED3DTA_TEMP:
            /* TODO: Support WINED3DTSS_RESULTARG */
            FIXME("WINED3DTA_TEMP, not properly supported.\n");
            return GL_SPARE1_NV;

        case WINED3DTA_CONSTANT:
            /* TODO: Support per stage constants (WINED3DTSS_CONSTANT, NV_register_combiners2) */
            FIXME("WINED3DTA_CONSTANT, not properly supported.\n");
            return GL_CONSTANT_COLOR1_NV;

        default:
            FIXME("Unrecognized texture arg %#x\n", d3dta);
            return GL_TEXTURE;
    }
}

static GLenum invert_mapping(GLenum mapping) {
    if (mapping == GL_UNSIGNED_INVERT_NV) return GL_SIGNED_IDENTITY_NV;
    else if (mapping == GL_SIGNED_IDENTITY_NV) return GL_UNSIGNED_INVERT_NV;

    FIXME("Unhandled mapping %#x\n", mapping);
    return mapping;
}

static void get_src_and_opr_nvrc(DWORD stage, DWORD arg, BOOL is_alpha, GLenum* input, GLenum* mapping, GLenum *component_usage, INT texture_idx) {
    /* The WINED3DTA_COMPLEMENT flag specifies the complement of the input should
     * be used. */
    if (arg & WINED3DTA_COMPLEMENT) *mapping = GL_UNSIGNED_INVERT_NV;
    else *mapping = GL_SIGNED_IDENTITY_NV;

    /* The WINED3DTA_ALPHAREPLICATE flag specifies the alpha component of the input
     * should be used for all input components. */
    if (is_alpha || arg & WINED3DTA_ALPHAREPLICATE) *component_usage = GL_ALPHA;
    else *component_usage = GL_RGB;

    *input = d3dta_to_combiner_input(arg & WINED3DTA_SELECTMASK, stage, texture_idx);
}

typedef struct {
    GLenum input[3];
    GLenum mapping[3];
    GLenum component_usage[3];
} tex_op_args;

static BOOL is_invalid_op(IWineD3DDeviceImpl *This, int stage, WINED3DTEXTUREOP op, DWORD arg1, DWORD arg2, DWORD arg3) {
    if (op == WINED3DTOP_DISABLE) return FALSE;
    if (This->stateBlock->textures[stage]) return FALSE;

    if ((arg1 & WINED3DTA_SELECTMASK) == WINED3DTA_TEXTURE
            && op != WINED3DTOP_SELECTARG2) return TRUE;
    if ((arg2 & WINED3DTA_SELECTMASK) == WINED3DTA_TEXTURE
            && op != WINED3DTOP_SELECTARG1) return TRUE;
    if ((arg3 & WINED3DTA_SELECTMASK) == WINED3DTA_TEXTURE
            && (op == WINED3DTOP_MULTIPLYADD || op == WINED3DTOP_LERP)) return TRUE;

    return FALSE;
}

void set_tex_op_nvrc(IWineD3DDevice *iface, BOOL is_alpha, int stage, WINED3DTEXTUREOP op, DWORD arg1, DWORD arg2, DWORD arg3, INT texture_idx) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl*)iface;
    tex_op_args tex_op_args = {{0}, {0}, {0}};
    GLenum portion = is_alpha ? GL_ALPHA : GL_RGB;
    GLenum target = GL_COMBINER0_NV + stage;

    TRACE("stage %d, is_alpha %d, op %s, arg1 %#x, arg2 %#x, arg3 %#x, texture_idx %d\n",
            stage, is_alpha, debug_d3dtop(op), arg1, arg2, arg3, texture_idx);

    /* If a texture stage references an invalid texture unit the stage just
     * passes through the result from the previous stage */
    if (is_invalid_op(This, stage, op, arg1, arg2, arg3)) {
        arg1 = WINED3DTA_CURRENT;
        op = WINED3DTOP_SELECTARG1;
    }

    get_src_and_opr_nvrc(stage, arg1, is_alpha, &tex_op_args.input[0],
            &tex_op_args.mapping[0], &tex_op_args.component_usage[0], texture_idx);
    get_src_and_opr_nvrc(stage, arg2, is_alpha, &tex_op_args.input[1],
            &tex_op_args.mapping[1], &tex_op_args.component_usage[1], texture_idx);
    get_src_and_opr_nvrc(stage, arg3, is_alpha, &tex_op_args.input[2],
            &tex_op_args.mapping[2], &tex_op_args.component_usage[2], texture_idx);


    /* This is called by a state handler which has the gl lock held and a context for the thread */
    switch(op)
    {
        case WINED3DTOP_DISABLE:
            /* Only for alpha */
            if (!is_alpha) ERR("Shouldn't be called for WINED3DTSS_COLOROP (WINED3DTOP_DISABLE)\n");
            /* Input, prev_alpha*1 */
            GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_A_NV,
                    GL_SPARE0_NV, GL_UNSIGNED_IDENTITY_NV, GL_ALPHA));
            GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_B_NV,
                    GL_ZERO, GL_UNSIGNED_INVERT_NV, GL_ALPHA));

            /* Output */
            GL_EXTCALL(glCombinerOutputNV(target, portion, GL_SPARE0_NV, GL_DISCARD_NV,
                    GL_DISCARD_NV, GL_NONE, GL_NONE, GL_FALSE, GL_FALSE, GL_FALSE));
            break;

        case WINED3DTOP_SELECTARG1:
        case WINED3DTOP_SELECTARG2:
            /* Input, arg*1 */
            if (op == WINED3DTOP_SELECTARG1) {
                GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_A_NV,
                        tex_op_args.input[0], tex_op_args.mapping[0], tex_op_args.component_usage[0]));
            } else {
                GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_A_NV,
                        tex_op_args.input[1], tex_op_args.mapping[1], tex_op_args.component_usage[1]));
            }
            GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_B_NV,
                    GL_ZERO, GL_UNSIGNED_INVERT_NV, portion));

            /* Output */
            GL_EXTCALL(glCombinerOutputNV(target, portion, GL_SPARE0_NV, GL_DISCARD_NV,
                    GL_DISCARD_NV, GL_NONE, GL_NONE, GL_FALSE, GL_FALSE, GL_FALSE));
            break;

        case WINED3DTOP_MODULATE:
        case WINED3DTOP_MODULATE2X:
        case WINED3DTOP_MODULATE4X:
            /* Input, arg1*arg2 */
            GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_A_NV,
                    tex_op_args.input[0], tex_op_args.mapping[0], tex_op_args.component_usage[0]));
            GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_B_NV,
                    tex_op_args.input[1], tex_op_args.mapping[1], tex_op_args.component_usage[1]));

            /* Output */
            if (op == WINED3DTOP_MODULATE) {
                GL_EXTCALL(glCombinerOutputNV(target, portion, GL_SPARE0_NV, GL_DISCARD_NV,
                        GL_DISCARD_NV, GL_NONE, GL_NONE, GL_FALSE, GL_FALSE, GL_FALSE));
            } else if (op == WINED3DTOP_MODULATE2X) {
                GL_EXTCALL(glCombinerOutputNV(target, portion, GL_SPARE0_NV, GL_DISCARD_NV,
                        GL_DISCARD_NV, GL_SCALE_BY_TWO_NV, GL_NONE, GL_FALSE, GL_FALSE, GL_FALSE));
            } else if (op == WINED3DTOP_MODULATE4X) {
                GL_EXTCALL(glCombinerOutputNV(target, portion, GL_SPARE0_NV, GL_DISCARD_NV,
                        GL_DISCARD_NV, GL_SCALE_BY_FOUR_NV, GL_NONE, GL_FALSE, GL_FALSE, GL_FALSE));
            }
            break;

        case WINED3DTOP_ADD:
        case WINED3DTOP_ADDSIGNED:
        case WINED3DTOP_ADDSIGNED2X:
            /* Input, arg1*1+arg2*1 */
            GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_A_NV,
                    tex_op_args.input[0], tex_op_args.mapping[0], tex_op_args.component_usage[0]));
            GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_B_NV,
                    GL_ZERO, GL_UNSIGNED_INVERT_NV, portion));
            GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_C_NV,
                    tex_op_args.input[1], tex_op_args.mapping[1], tex_op_args.component_usage[1]));
            GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_D_NV,
                    GL_ZERO, GL_UNSIGNED_INVERT_NV, portion));

            /* Output */
            if (op == WINED3DTOP_ADD) {
                GL_EXTCALL(glCombinerOutputNV(target, portion, GL_DISCARD_NV, GL_DISCARD_NV,
                        GL_SPARE0_NV, GL_NONE, GL_NONE, GL_FALSE, GL_FALSE, GL_FALSE));
            } else if (op == WINED3DTOP_ADDSIGNED) {
                GL_EXTCALL(glCombinerOutputNV(target, portion, GL_DISCARD_NV, GL_DISCARD_NV,
                        GL_SPARE0_NV, GL_NONE, GL_BIAS_BY_NEGATIVE_ONE_HALF_NV, GL_FALSE, GL_FALSE, GL_FALSE));
            } else if (op == WINED3DTOP_ADDSIGNED2X) {
                GL_EXTCALL(glCombinerOutputNV(target, portion, GL_DISCARD_NV, GL_DISCARD_NV,
                        GL_SPARE0_NV, GL_SCALE_BY_TWO_NV, GL_BIAS_BY_NEGATIVE_ONE_HALF_NV, GL_FALSE, GL_FALSE, GL_FALSE));
            }
            break;

        case WINED3DTOP_SUBTRACT:
            /* Input, arg1*1+-arg2*1 */
            GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_A_NV,
                    tex_op_args.input[0], tex_op_args.mapping[0], tex_op_args.component_usage[0]));
            GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_B_NV,
                    GL_ZERO, GL_UNSIGNED_INVERT_NV, portion));
            GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_C_NV,
                    tex_op_args.input[1], GL_SIGNED_NEGATE_NV, tex_op_args.component_usage[1]));
            GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_D_NV,
                    GL_ZERO, GL_UNSIGNED_INVERT_NV, portion));

            /* Output */
            GL_EXTCALL(glCombinerOutputNV(target, portion, GL_DISCARD_NV, GL_DISCARD_NV,
                    GL_SPARE0_NV, GL_NONE, GL_NONE, GL_FALSE, GL_FALSE, GL_FALSE));
            break;

        case WINED3DTOP_ADDSMOOTH:
            /* Input, arg1*1+(1-arg1)*arg2 */
            GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_A_NV,
                    tex_op_args.input[0], tex_op_args.mapping[0], tex_op_args.component_usage[0]));
            GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_B_NV,
                    GL_ZERO, GL_UNSIGNED_INVERT_NV, portion));
            GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_C_NV,
                    tex_op_args.input[0], invert_mapping(tex_op_args.mapping[0]), tex_op_args.component_usage[0]));
            GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_D_NV,
                    tex_op_args.input[1], tex_op_args.mapping[1], tex_op_args.component_usage[1]));

            /* Output */
            GL_EXTCALL(glCombinerOutputNV(target, portion, GL_DISCARD_NV, GL_DISCARD_NV,
                    GL_SPARE0_NV, GL_NONE, GL_NONE, GL_FALSE, GL_FALSE, GL_FALSE));
            break;

        case WINED3DTOP_BLENDDIFFUSEALPHA:
        case WINED3DTOP_BLENDTEXTUREALPHA:
        case WINED3DTOP_BLENDFACTORALPHA:
        case WINED3DTOP_BLENDTEXTUREALPHAPM:
        case WINED3DTOP_BLENDCURRENTALPHA:
        {
            GLenum alpha_src = GL_PRIMARY_COLOR_NV;
            if (op == WINED3DTOP_BLENDDIFFUSEALPHA) alpha_src = d3dta_to_combiner_input(WINED3DTA_DIFFUSE, stage, texture_idx);
            else if (op == WINED3DTOP_BLENDTEXTUREALPHA) alpha_src = d3dta_to_combiner_input(WINED3DTA_TEXTURE, stage, texture_idx);
            else if (op == WINED3DTOP_BLENDFACTORALPHA) alpha_src = d3dta_to_combiner_input(WINED3DTA_TFACTOR, stage, texture_idx);
            else if (op == WINED3DTOP_BLENDTEXTUREALPHAPM) alpha_src = d3dta_to_combiner_input(WINED3DTA_TEXTURE, stage, texture_idx);
            else if (op == WINED3DTOP_BLENDCURRENTALPHA) alpha_src = d3dta_to_combiner_input(WINED3DTA_CURRENT, stage, texture_idx);
            else FIXME("Unhandled WINED3DTOP %s, shouldn't happen\n", debug_d3dtop(op));

            /* Input, arg1*alpha_src+arg2*(1-alpha_src) */
            GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_A_NV,
                    tex_op_args.input[0], tex_op_args.mapping[0], tex_op_args.component_usage[0]));
            if (op == WINED3DTOP_BLENDTEXTUREALPHAPM)
            {
                GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_B_NV,
                        GL_ZERO, GL_UNSIGNED_INVERT_NV, portion));
            } else {
                GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_B_NV,
                        alpha_src, GL_UNSIGNED_IDENTITY_NV, GL_ALPHA));
            }
            GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_C_NV,
                    tex_op_args.input[1], tex_op_args.mapping[1], tex_op_args.component_usage[1]));
            GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_D_NV,
                    alpha_src, GL_UNSIGNED_INVERT_NV, GL_ALPHA));

            /* Output */
            GL_EXTCALL(glCombinerOutputNV(target, portion, GL_DISCARD_NV, GL_DISCARD_NV,
                    GL_SPARE0_NV, GL_NONE, GL_NONE, GL_FALSE, GL_FALSE, GL_FALSE));
            break;
        }

        case WINED3DTOP_MODULATEALPHA_ADDCOLOR:
            /* Input, arg1_alpha*arg2_rgb+arg1_rgb*1 */
            if (is_alpha) ERR("Only supported for WINED3DTSS_COLOROP (WINED3DTOP_MODULATEALPHA_ADDCOLOR)\n");
            GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_A_NV,
                    tex_op_args.input[0], tex_op_args.mapping[0], GL_ALPHA));
            GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_B_NV,
                    tex_op_args.input[1], tex_op_args.mapping[1], tex_op_args.component_usage[1]));
            GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_C_NV,
                    tex_op_args.input[0], tex_op_args.mapping[0], tex_op_args.component_usage[0]));
            GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_D_NV,
                    GL_ZERO, GL_UNSIGNED_INVERT_NV, portion));

            /* Output */
            GL_EXTCALL(glCombinerOutputNV(target, portion, GL_DISCARD_NV, GL_DISCARD_NV,
                    GL_SPARE0_NV, GL_NONE, GL_NONE, GL_FALSE, GL_FALSE, GL_FALSE));
            break;

        case WINED3DTOP_MODULATECOLOR_ADDALPHA:
            /* Input, arg1_rgb*arg2_rgb+arg1_alpha*1 */
            if (is_alpha) ERR("Only supported for WINED3DTSS_COLOROP (WINED3DTOP_MODULATECOLOR_ADDALPHA)\n");
            GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_A_NV,
                    tex_op_args.input[0], tex_op_args.mapping[0], tex_op_args.component_usage[0]));
            GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_B_NV,
                    tex_op_args.input[1], tex_op_args.mapping[1], tex_op_args.component_usage[1]));
            GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_C_NV,
                    tex_op_args.input[0], tex_op_args.mapping[0], GL_ALPHA));
            GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_D_NV,
                    GL_ZERO, GL_UNSIGNED_INVERT_NV, portion));

            /* Output */
            GL_EXTCALL(glCombinerOutputNV(target, portion, GL_DISCARD_NV, GL_DISCARD_NV,
                    GL_SPARE0_NV, GL_NONE, GL_NONE, GL_FALSE, GL_FALSE, GL_FALSE));
            break;

        case WINED3DTOP_MODULATEINVALPHA_ADDCOLOR:
            /* Input, (1-arg1_alpha)*arg2_rgb+arg1_rgb*1 */
            if (is_alpha) ERR("Only supported for WINED3DTSS_COLOROP (WINED3DTOP_MODULATEINVALPHA_ADDCOLOR)\n");
            GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_A_NV,
                    tex_op_args.input[0], invert_mapping(tex_op_args.mapping[0]), GL_ALPHA));
            GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_B_NV,
                    tex_op_args.input[1], tex_op_args.mapping[1], tex_op_args.component_usage[1]));
            GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_C_NV,
                    tex_op_args.input[0], tex_op_args.mapping[0], tex_op_args.component_usage[0]));
            GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_D_NV,
                    GL_ZERO, GL_UNSIGNED_INVERT_NV, portion));

            /* Output */
            GL_EXTCALL(glCombinerOutputNV(target, portion, GL_DISCARD_NV, GL_DISCARD_NV,
                    GL_SPARE0_NV, GL_NONE, GL_NONE, GL_FALSE, GL_FALSE, GL_FALSE));
            break;

        case WINED3DTOP_MODULATEINVCOLOR_ADDALPHA:
            /* Input, (1-arg1_rgb)*arg2_rgb+arg1_alpha*1 */
            if (is_alpha) ERR("Only supported for WINED3DTSS_COLOROP (WINED3DTOP_MODULATEINVCOLOR_ADDALPHA)\n");
            GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_A_NV,
                    tex_op_args.input[0], invert_mapping(tex_op_args.mapping[0]), tex_op_args.component_usage[0]));
            GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_B_NV,
                    tex_op_args.input[1], tex_op_args.mapping[1], tex_op_args.component_usage[1]));
            GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_C_NV,
                    tex_op_args.input[0], tex_op_args.mapping[0], GL_ALPHA));
            GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_D_NV,
                    GL_ZERO, GL_UNSIGNED_INVERT_NV, portion));

            /* Output */
            GL_EXTCALL(glCombinerOutputNV(target, portion, GL_DISCARD_NV, GL_DISCARD_NV,
                    GL_SPARE0_NV, GL_NONE, GL_NONE, GL_FALSE, GL_FALSE, GL_FALSE));
            break;

        case WINED3DTOP_DOTPRODUCT3:
            /* Input, arg1 . arg2 */
            /* FIXME: DX7 uses a different calculation? */
            GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_A_NV,
                    tex_op_args.input[0], GL_EXPAND_NORMAL_NV, tex_op_args.component_usage[0]));
            GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_B_NV,
                    tex_op_args.input[1], GL_EXPAND_NORMAL_NV, tex_op_args.component_usage[1]));

            /* Output */
            GL_EXTCALL(glCombinerOutputNV(target, portion, GL_SPARE0_NV, GL_DISCARD_NV,
                    GL_DISCARD_NV, GL_NONE, GL_NONE, GL_TRUE, GL_FALSE, GL_FALSE));
            break;

        case WINED3DTOP_MULTIPLYADD:
            /* Input, arg1*1+arg2*arg3 */
            GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_A_NV,
                    tex_op_args.input[0], tex_op_args.mapping[0], tex_op_args.component_usage[0]));
            GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_B_NV,
                    GL_ZERO, GL_UNSIGNED_INVERT_NV, portion));
            GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_C_NV,
                    tex_op_args.input[1], tex_op_args.mapping[1], tex_op_args.component_usage[1]));
            GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_D_NV,
                    tex_op_args.input[2], tex_op_args.mapping[2], tex_op_args.component_usage[2]));

            /* Output */
            GL_EXTCALL(glCombinerOutputNV(target, portion, GL_DISCARD_NV, GL_DISCARD_NV,
                    GL_SPARE0_NV, GL_NONE, GL_NONE, GL_FALSE, GL_FALSE, GL_FALSE));
            break;

        case WINED3DTOP_LERP:
            /* Input, arg1*arg2+(1-arg1)*arg3 */
            GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_A_NV,
                    tex_op_args.input[0], tex_op_args.mapping[0], tex_op_args.component_usage[0]));
            GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_B_NV,
                    tex_op_args.input[1], tex_op_args.mapping[1], tex_op_args.component_usage[1]));
            GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_C_NV,
                    tex_op_args.input[0], invert_mapping(tex_op_args.mapping[0]), tex_op_args.component_usage[0]));
            GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_D_NV,
                    tex_op_args.input[2], tex_op_args.mapping[2], tex_op_args.component_usage[2]));

            /* Output */
            GL_EXTCALL(glCombinerOutputNV(target, portion, GL_DISCARD_NV, GL_DISCARD_NV,
                    GL_SPARE0_NV, GL_NONE, GL_NONE, GL_FALSE, GL_FALSE, GL_FALSE));
            break;

        case WINED3DTOP_BUMPENVMAPLUMINANCE:
        case WINED3DTOP_BUMPENVMAP:
            if(GL_SUPPORT(NV_TEXTURE_SHADER)) {
                /* The bump map stage itself isn't exciting, just read the texture. But tell the next stage to
                 * perform bump mapping and source from the current stage. Pretty much a SELECTARG2.
                 * ARG2 is passed through unmodified(apps will most likely use D3DTA_CURRENT for arg2, arg1
                 * (which will most likely be D3DTA_TEXTURE) is available as a texture shader input for the next stage
                 */
                GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_A_NV,
                        tex_op_args.input[1], tex_op_args.mapping[1], tex_op_args.component_usage[1]));
                GL_EXTCALL(glCombinerInputNV(target, portion, GL_VARIABLE_B_NV,
                        GL_ZERO, GL_UNSIGNED_INVERT_NV, portion));
                GL_EXTCALL(glCombinerOutputNV(target, portion, GL_SPARE0_NV, GL_DISCARD_NV,
                        GL_DISCARD_NV, GL_NONE, GL_NONE, GL_FALSE, GL_FALSE, GL_FALSE));
                break;
            }

        default:
            FIXME("Unhandled WINED3DTOP: stage %d, is_alpha %d, op %s (%#x), arg1 %#x, arg2 %#x, arg3 %#x, texture_idx %d\n",
                    stage, is_alpha, debug_d3dtop(op), op, arg1, arg2, arg3, texture_idx);
    }

    checkGLcall("set_tex_op_nvrc()\n");

}

static void get_src_and_opr(DWORD arg, BOOL is_alpha, GLenum* source, GLenum* operand) {
    /* The WINED3DTA_ALPHAREPLICATE flag specifies the alpha component of the
     * input should be used for all input components. The WINED3DTA_COMPLEMENT
     * flag specifies the complement of the input should be used. */
    BOOL from_alpha = is_alpha || arg & WINED3DTA_ALPHAREPLICATE;
    BOOL complement = arg & WINED3DTA_COMPLEMENT;

    /* Calculate the operand */
    if (complement) {
        if (from_alpha) *operand = GL_ONE_MINUS_SRC_ALPHA;
        else *operand = GL_ONE_MINUS_SRC_COLOR;
    } else {
        if (from_alpha) *operand = GL_SRC_ALPHA;
        else *operand = GL_SRC_COLOR;
    }

    /* Calculate the source */
    switch (arg & WINED3DTA_SELECTMASK) {
        case WINED3DTA_CURRENT: *source = GL_PREVIOUS_EXT; break;
        case WINED3DTA_DIFFUSE: *source = GL_PRIMARY_COLOR_EXT; break;
        case WINED3DTA_TEXTURE: *source = GL_TEXTURE; break;
        case WINED3DTA_TFACTOR: *source = GL_CONSTANT_EXT; break;
        case WINED3DTA_SPECULAR:
            /*
             * According to the GL_ARB_texture_env_combine specs, SPECULAR is
             * 'Secondary color' and isn't supported until base GL supports it
             * There is no concept of temp registers as far as I can tell
             */
            FIXME("Unhandled texture arg WINED3DTA_SPECULAR\n");
            *source = GL_TEXTURE;
            break;
        default:
            FIXME("Unrecognized texture arg %#x\n", arg);
            *source = GL_TEXTURE;
            break;
    }
}

/* Set texture operations up - The following avoids lots of ifdefs in this routine!*/
#if defined (GL_VERSION_1_3)
# define useext(A) A
# define combine_ext 1
#elif defined (GL_EXT_texture_env_combine)
# define useext(A) A##_EXT
# define combine_ext 1
#elif defined (GL_ARB_texture_env_combine)
# define useext(A) A##_ARB
# define combine_ext 1
#else
# undef combine_ext
#endif

#if !defined(combine_ext)
void set_tex_op(IWineD3DDevice *iface, BOOL isAlpha, int Stage, WINED3DTEXTUREOP op, DWORD arg1, DWORD arg2, DWORD arg3)
{
        FIXME("Requires opengl combine extensions to work\n");
        return;
}
#else
/* Setup the texture operations texture stage states */
void set_tex_op(IWineD3DDevice *iface, BOOL isAlpha, int Stage, WINED3DTEXTUREOP op, DWORD arg1, DWORD arg2, DWORD arg3)
{
        GLenum src1, src2, src3;
        GLenum opr1, opr2, opr3;
        GLenum comb_target;
        GLenum src0_target, src1_target, src2_target;
        GLenum opr0_target, opr1_target, opr2_target;
        GLenum scal_target;
        GLenum opr=0, invopr, src3_target, opr3_target;
        BOOL Handled = FALSE;
        IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

        TRACE("Alpha?(%d), Stage:%d Op(%s), a1(%d), a2(%d), a3(%d)\n", isAlpha, Stage, debug_d3dtop(op), arg1, arg2, arg3);

        /* This is called by a state handler which has the gl lock held and a context for the thread */

        /* Note: Operations usually involve two ars, src0 and src1 and are operations of
           the form (a1 <operation> a2). However, some of the more complex operations
           take 3 parameters. Instead of the (sensible) addition of a3, Microsoft added
           in a third parameter called a0. Therefore these are operations of the form
           a0 <operation> a1 <operation> a2, ie the new parameter goes to the front.

           However, below we treat the new (a0) parameter as src2/opr2, so in the actual
           functions below, expect their syntax to differ slightly to those listed in the
           manuals, ie replace arg1 with arg3, arg2 with arg1 and arg3 with arg2
           This affects WINED3DTOP_MULTIPLYADD and WINED3DTOP_LERP                     */

        if (isAlpha) {
                comb_target = useext(GL_COMBINE_ALPHA);
                src0_target = useext(GL_SOURCE0_ALPHA);
                src1_target = useext(GL_SOURCE1_ALPHA);
                src2_target = useext(GL_SOURCE2_ALPHA);
                opr0_target = useext(GL_OPERAND0_ALPHA);
                opr1_target = useext(GL_OPERAND1_ALPHA);
                opr2_target = useext(GL_OPERAND2_ALPHA);
                scal_target = GL_ALPHA_SCALE;
        }
        else {
                comb_target = useext(GL_COMBINE_RGB);
                src0_target = useext(GL_SOURCE0_RGB);
                src1_target = useext(GL_SOURCE1_RGB);
                src2_target = useext(GL_SOURCE2_RGB);
                opr0_target = useext(GL_OPERAND0_RGB);
                opr1_target = useext(GL_OPERAND1_RGB);
                opr2_target = useext(GL_OPERAND2_RGB);
                scal_target = useext(GL_RGB_SCALE);
        }

        /* If a texture stage references an invalid texture unit the stage just
         * passes through the result from the previous stage */
        if (is_invalid_op(This, Stage, op, arg1, arg2, arg3)) {
            arg1 = WINED3DTA_CURRENT;
            op = WINED3DTOP_SELECTARG1;
        }

        /* From MSDN (WINED3DTSS_ALPHAARG1) :
           The default argument is WINED3DTA_TEXTURE. If no texture is set for this stage,
                   then the default argument is WINED3DTA_DIFFUSE.
                   FIXME? If texture added/removed, may need to reset back as well?    */
        if (isAlpha && This->stateBlock->textures[Stage] == NULL && arg1 == WINED3DTA_TEXTURE) {
            get_src_and_opr(WINED3DTA_DIFFUSE, isAlpha, &src1, &opr1);
        } else {
            get_src_and_opr(arg1, isAlpha, &src1, &opr1);
        }
        get_src_and_opr(arg2, isAlpha, &src2, &opr2);
        get_src_and_opr(arg3, isAlpha, &src3, &opr3);

        TRACE("ct(%x), 1:(%x,%x), 2:(%x,%x), 3:(%x,%x)\n", comb_target, src1, opr1, src2, opr2, src3, opr3);

        Handled = TRUE; /* Assume will be handled */

        /* Other texture operations require special extensions: */
        if (GL_SUPPORT(NV_TEXTURE_ENV_COMBINE4)) {
          if (isAlpha) {
            opr = GL_SRC_ALPHA;
            invopr = GL_ONE_MINUS_SRC_ALPHA;
            src3_target = GL_SOURCE3_ALPHA_NV;
            opr3_target = GL_OPERAND3_ALPHA_NV;
          } else {
            opr = GL_SRC_COLOR;
            invopr = GL_ONE_MINUS_SRC_COLOR;
            src3_target = GL_SOURCE3_RGB_NV;
            opr3_target = GL_OPERAND3_RGB_NV;
          }
          switch (op) {
          case WINED3DTOP_DISABLE: /* Only for alpha */
            glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_ADD);
            checkGLcall("GL_TEXTURE_ENV, comb_target, GL_REPLACE");
            glTexEnvi(GL_TEXTURE_ENV, src0_target, GL_PREVIOUS_EXT);
            checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
            glTexEnvi(GL_TEXTURE_ENV, opr0_target, GL_SRC_ALPHA);
            checkGLcall("GL_TEXTURE_ENV, opr0_target, opr1");
            glTexEnvi(GL_TEXTURE_ENV, src1_target, GL_ZERO);
            checkGLcall("GL_TEXTURE_ENV, src1_target, GL_ZERO");
            glTexEnvi(GL_TEXTURE_ENV, opr1_target, invopr);
            checkGLcall("GL_TEXTURE_ENV, opr1_target, invopr");
            glTexEnvi(GL_TEXTURE_ENV, src2_target, GL_ZERO);
            checkGLcall("GL_TEXTURE_ENV, src2_target, GL_ZERO");
            glTexEnvi(GL_TEXTURE_ENV, opr2_target, opr);
            checkGLcall("GL_TEXTURE_ENV, opr2_target, opr");
            glTexEnvi(GL_TEXTURE_ENV, src3_target, GL_ZERO);
            checkGLcall("GL_TEXTURE_ENV, src3_target, GL_ZERO");
            glTexEnvi(GL_TEXTURE_ENV, opr3_target, opr);
            checkGLcall("GL_TEXTURE_ENV, opr3_target, opr");
            break;
          case WINED3DTOP_SELECTARG1:                                          /* = a1 * 1 + 0 * 0 */
          case WINED3DTOP_SELECTARG2:                                          /* = a2 * 1 + 0 * 0 */
            glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_ADD);
            checkGLcall("GL_TEXTURE_ENV, comb_target, GL_ADD");
            if (op == WINED3DTOP_SELECTARG1) {
              glTexEnvi(GL_TEXTURE_ENV, src0_target, src1);
              checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
              glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr1);
              checkGLcall("GL_TEXTURE_ENV, opr0_target, opr1");
            } else {
              glTexEnvi(GL_TEXTURE_ENV, src0_target, src2);
              checkGLcall("GL_TEXTURE_ENV, src0_target, src2");
              glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr2);
              checkGLcall("GL_TEXTURE_ENV, opr0_target, opr2");
            }
            glTexEnvi(GL_TEXTURE_ENV, src1_target, GL_ZERO);
            checkGLcall("GL_TEXTURE_ENV, src1_target, GL_ZERO");
            glTexEnvi(GL_TEXTURE_ENV, opr1_target, invopr);
            checkGLcall("GL_TEXTURE_ENV, opr1_target, invopr");
            glTexEnvi(GL_TEXTURE_ENV, src2_target, GL_ZERO);
            checkGLcall("GL_TEXTURE_ENV, src2_target, GL_ZERO");
            glTexEnvi(GL_TEXTURE_ENV, opr2_target, opr);
            checkGLcall("GL_TEXTURE_ENV, opr2_target, opr");
            glTexEnvi(GL_TEXTURE_ENV, src3_target, GL_ZERO);
            checkGLcall("GL_TEXTURE_ENV, src3_target, GL_ZERO");
            glTexEnvi(GL_TEXTURE_ENV, opr3_target, opr);
            checkGLcall("GL_TEXTURE_ENV, opr3_target, opr");
            break;

          case WINED3DTOP_MODULATE:
            glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_ADD);
            checkGLcall("GL_TEXTURE_ENV, comb_target, GL_ADD"); /* Add = a0*a1 + a2*a3 */
            glTexEnvi(GL_TEXTURE_ENV, src0_target, src1);
            checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
            glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr1);
            checkGLcall("GL_TEXTURE_ENV, opr0_target, opr1");
            glTexEnvi(GL_TEXTURE_ENV, src1_target, src2);
            checkGLcall("GL_TEXTURE_ENV, src1_target, GL_ZERO");
            glTexEnvi(GL_TEXTURE_ENV, opr1_target, opr2);
            checkGLcall("GL_TEXTURE_ENV, opr1_target, invopr");
            glTexEnvi(GL_TEXTURE_ENV, src2_target, GL_ZERO);
            checkGLcall("GL_TEXTURE_ENV, src2_target, src2");
            glTexEnvi(GL_TEXTURE_ENV, opr2_target, opr);
            checkGLcall("GL_TEXTURE_ENV, opr2_target, opr2");
            glTexEnvi(GL_TEXTURE_ENV, src3_target, GL_ZERO);
            checkGLcall("GL_TEXTURE_ENV, src3_target, GL_ZERO");
            glTexEnvi(GL_TEXTURE_ENV, opr3_target, opr);
            checkGLcall("GL_TEXTURE_ENV, opr3_target, opr1");
            glTexEnvi(GL_TEXTURE_ENV, scal_target, 1);
            checkGLcall("GL_TEXTURE_ENV, scal_target, 1");
            break;
          case WINED3DTOP_MODULATE2X:
            glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_ADD);
            checkGLcall("GL_TEXTURE_ENV, comb_target, GL_ADD"); /* Add = a0*a1 + a2*a3 */
            glTexEnvi(GL_TEXTURE_ENV, src0_target, src1);
            checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
            glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr1);
            checkGLcall("GL_TEXTURE_ENV, opr0_target, opr1");
            glTexEnvi(GL_TEXTURE_ENV, src1_target, src2);
            checkGLcall("GL_TEXTURE_ENV, src1_target, GL_ZERO");
            glTexEnvi(GL_TEXTURE_ENV, opr1_target, opr2);
            checkGLcall("GL_TEXTURE_ENV, opr1_target, invopr");
            glTexEnvi(GL_TEXTURE_ENV, src2_target, GL_ZERO);
            checkGLcall("GL_TEXTURE_ENV, src2_target, src2");
            glTexEnvi(GL_TEXTURE_ENV, opr2_target, opr);
            checkGLcall("GL_TEXTURE_ENV, opr2_target, opr2");
            glTexEnvi(GL_TEXTURE_ENV, src3_target, GL_ZERO);
            checkGLcall("GL_TEXTURE_ENV, src3_target, GL_ZERO");
            glTexEnvi(GL_TEXTURE_ENV, opr3_target, opr);
            checkGLcall("GL_TEXTURE_ENV, opr3_target, opr1");
            glTexEnvi(GL_TEXTURE_ENV, scal_target, 2);
            checkGLcall("GL_TEXTURE_ENV, scal_target, 2");
            break;
          case WINED3DTOP_MODULATE4X:
            glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_ADD);
            checkGLcall("GL_TEXTURE_ENV, comb_target, GL_ADD"); /* Add = a0*a1 + a2*a3 */
            glTexEnvi(GL_TEXTURE_ENV, src0_target, src1);
            checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
            glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr1);
            checkGLcall("GL_TEXTURE_ENV, opr0_target, opr1");
            glTexEnvi(GL_TEXTURE_ENV, src1_target, src2);
            checkGLcall("GL_TEXTURE_ENV, src1_target, GL_ZERO");
            glTexEnvi(GL_TEXTURE_ENV, opr1_target, opr2);
            checkGLcall("GL_TEXTURE_ENV, opr1_target, invopr");
            glTexEnvi(GL_TEXTURE_ENV, src2_target, GL_ZERO);
            checkGLcall("GL_TEXTURE_ENV, src2_target, src2");
            glTexEnvi(GL_TEXTURE_ENV, opr2_target, opr);
            checkGLcall("GL_TEXTURE_ENV, opr2_target, opr2");
            glTexEnvi(GL_TEXTURE_ENV, src3_target, GL_ZERO);
            checkGLcall("GL_TEXTURE_ENV, src3_target, GL_ZERO");
            glTexEnvi(GL_TEXTURE_ENV, opr3_target, opr);
            checkGLcall("GL_TEXTURE_ENV, opr3_target, opr1");
            glTexEnvi(GL_TEXTURE_ENV, scal_target, 4);
            checkGLcall("GL_TEXTURE_ENV, scal_target, 4");
            break;

          case WINED3DTOP_ADD:
            glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_ADD);
            checkGLcall("GL_TEXTURE_ENV, comb_target, GL_ADD");
            glTexEnvi(GL_TEXTURE_ENV, src0_target, src1);
            checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
            glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr1);
            checkGLcall("GL_TEXTURE_ENV, opr0_target, opr1");
            glTexEnvi(GL_TEXTURE_ENV, src1_target, GL_ZERO);
            checkGLcall("GL_TEXTURE_ENV, src1_target, GL_ZERO");
            glTexEnvi(GL_TEXTURE_ENV, opr1_target, invopr);
            checkGLcall("GL_TEXTURE_ENV, opr1_target, invopr");
            glTexEnvi(GL_TEXTURE_ENV, src2_target, src2);
            checkGLcall("GL_TEXTURE_ENV, src2_target, src2");
            glTexEnvi(GL_TEXTURE_ENV, opr2_target, opr2);
            checkGLcall("GL_TEXTURE_ENV, opr2_target, opr2");
            glTexEnvi(GL_TEXTURE_ENV, src3_target, GL_ZERO);
            checkGLcall("GL_TEXTURE_ENV, src3_target, GL_ZERO");
            glTexEnvi(GL_TEXTURE_ENV, opr3_target, invopr);
            checkGLcall("GL_TEXTURE_ENV, opr3_target, invopr");
            glTexEnvi(GL_TEXTURE_ENV, scal_target, 1);
            checkGLcall("GL_TEXTURE_ENV, scal_target, 1");
            break;

          case WINED3DTOP_ADDSIGNED:
            glTexEnvi(GL_TEXTURE_ENV, comb_target, useext(GL_ADD_SIGNED));
            checkGLcall("GL_TEXTURE_ENV, comb_target, useext(GL_ADD_SIGNED)");
            glTexEnvi(GL_TEXTURE_ENV, src0_target, src1);
            checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
            glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr1);
            checkGLcall("GL_TEXTURE_ENV, opr0_target, opr1");
            glTexEnvi(GL_TEXTURE_ENV, src1_target, GL_ZERO);
            checkGLcall("GL_TEXTURE_ENV, src1_target, GL_ZERO");
            glTexEnvi(GL_TEXTURE_ENV, opr1_target, invopr);
            checkGLcall("GL_TEXTURE_ENV, opr1_target, invopr");
            glTexEnvi(GL_TEXTURE_ENV, src2_target, src2);
            checkGLcall("GL_TEXTURE_ENV, src2_target, src2");
            glTexEnvi(GL_TEXTURE_ENV, opr2_target, opr2);
            checkGLcall("GL_TEXTURE_ENV, opr2_target, opr2");
            glTexEnvi(GL_TEXTURE_ENV, src3_target, GL_ZERO);
            checkGLcall("GL_TEXTURE_ENV, src3_target, GL_ZERO");
            glTexEnvi(GL_TEXTURE_ENV, opr3_target, invopr);
            checkGLcall("GL_TEXTURE_ENV, opr3_target, invopr");
            glTexEnvi(GL_TEXTURE_ENV, scal_target, 1);
            checkGLcall("GL_TEXTURE_ENV, scal_target, 1");
            break;

          case WINED3DTOP_ADDSIGNED2X:
            glTexEnvi(GL_TEXTURE_ENV, comb_target, useext(GL_ADD_SIGNED));
            checkGLcall("GL_TEXTURE_ENV, comb_target, useext(GL_ADD_SIGNED)");
            glTexEnvi(GL_TEXTURE_ENV, src0_target, src1);
            checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
            glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr1);
            checkGLcall("GL_TEXTURE_ENV, opr0_target, opr1");
            glTexEnvi(GL_TEXTURE_ENV, src1_target, GL_ZERO);
            checkGLcall("GL_TEXTURE_ENV, src1_target, GL_ZERO");
            glTexEnvi(GL_TEXTURE_ENV, opr1_target, invopr);
            checkGLcall("GL_TEXTURE_ENV, opr1_target, invopr");
            glTexEnvi(GL_TEXTURE_ENV, src2_target, src2);
            checkGLcall("GL_TEXTURE_ENV, src2_target, src2");
            glTexEnvi(GL_TEXTURE_ENV, opr2_target, opr2);
            checkGLcall("GL_TEXTURE_ENV, opr2_target, opr2");
            glTexEnvi(GL_TEXTURE_ENV, src3_target, GL_ZERO);
            checkGLcall("GL_TEXTURE_ENV, src3_target, GL_ZERO");
            glTexEnvi(GL_TEXTURE_ENV, opr3_target, invopr);
            checkGLcall("GL_TEXTURE_ENV, opr3_target, invopr");
            glTexEnvi(GL_TEXTURE_ENV, scal_target, 2);
            checkGLcall("GL_TEXTURE_ENV, scal_target, 2");
            break;

          case WINED3DTOP_ADDSMOOTH:
            glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_ADD);
            checkGLcall("GL_TEXTURE_ENV, comb_target, GL_ADD");
            glTexEnvi(GL_TEXTURE_ENV, src0_target, src1);
            checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
            glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr1);
            checkGLcall("GL_TEXTURE_ENV, opr0_target, opr1");
            glTexEnvi(GL_TEXTURE_ENV, src1_target, GL_ZERO);
            checkGLcall("GL_TEXTURE_ENV, src1_target, GL_ZERO");
            glTexEnvi(GL_TEXTURE_ENV, opr1_target, invopr);
            checkGLcall("GL_TEXTURE_ENV, opr1_target, invopr");
            glTexEnvi(GL_TEXTURE_ENV, src2_target, src2);
            checkGLcall("GL_TEXTURE_ENV, src2_target, src2");
            glTexEnvi(GL_TEXTURE_ENV, opr2_target, opr2);
            checkGLcall("GL_TEXTURE_ENV, opr2_target, opr2");
            glTexEnvi(GL_TEXTURE_ENV, src3_target, src1);
            checkGLcall("GL_TEXTURE_ENV, src3_target, src1");
            switch (opr1) {
            case GL_SRC_COLOR: opr = GL_ONE_MINUS_SRC_COLOR; break;
            case GL_ONE_MINUS_SRC_COLOR: opr = GL_SRC_COLOR; break;
            case GL_SRC_ALPHA: opr = GL_ONE_MINUS_SRC_ALPHA; break;
            case GL_ONE_MINUS_SRC_ALPHA: opr = GL_SRC_ALPHA; break;
            }
            glTexEnvi(GL_TEXTURE_ENV, opr3_target, opr);
            checkGLcall("GL_TEXTURE_ENV, opr3_target, opr");
            glTexEnvi(GL_TEXTURE_ENV, scal_target, 1);
            checkGLcall("GL_TEXTURE_ENV, scal_target, 1");
            break;

          case WINED3DTOP_BLENDDIFFUSEALPHA:
            glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_ADD);
            checkGLcall("GL_TEXTURE_ENV, comb_target, GL_ADD");
            glTexEnvi(GL_TEXTURE_ENV, src0_target, src1);
            checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
            glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr1);
            checkGLcall("GL_TEXTURE_ENV, opr0_target, opr1");
            glTexEnvi(GL_TEXTURE_ENV, src1_target, useext(GL_PRIMARY_COLOR));
            checkGLcall("GL_TEXTURE_ENV, src1_target, useext(GL_PRIMARY_COLOR)");
            glTexEnvi(GL_TEXTURE_ENV, opr1_target, invopr);
            checkGLcall("GL_TEXTURE_ENV, opr1_target, invopr");
            glTexEnvi(GL_TEXTURE_ENV, src2_target, src2);
            checkGLcall("GL_TEXTURE_ENV, src2_target, src2");
            glTexEnvi(GL_TEXTURE_ENV, opr2_target, opr2);
            checkGLcall("GL_TEXTURE_ENV, opr2_target, opr2");
            glTexEnvi(GL_TEXTURE_ENV, src3_target, useext(GL_PRIMARY_COLOR));
            checkGLcall("GL_TEXTURE_ENV, src3_target, useext(GL_PRIMARY_COLOR)");
            glTexEnvi(GL_TEXTURE_ENV, opr3_target, GL_ONE_MINUS_SRC_ALPHA);
            checkGLcall("GL_TEXTURE_ENV, opr3_target, GL_ONE_MINUS_SRC_ALPHA");
            glTexEnvi(GL_TEXTURE_ENV, scal_target, 1);
            checkGLcall("GL_TEXTURE_ENV, scal_target, 1");
            break;
          case WINED3DTOP_BLENDTEXTUREALPHA:
            glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_ADD);
            checkGLcall("GL_TEXTURE_ENV, comb_target, GL_ADD");
            glTexEnvi(GL_TEXTURE_ENV, src0_target, src1);
            checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
            glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr1);
            checkGLcall("GL_TEXTURE_ENV, opr0_target, opr1");
            glTexEnvi(GL_TEXTURE_ENV, src1_target, GL_TEXTURE);
            checkGLcall("GL_TEXTURE_ENV, src1_target, GL_TEXTURE");
            glTexEnvi(GL_TEXTURE_ENV, opr1_target, invopr);
            checkGLcall("GL_TEXTURE_ENV, opr1_target, invopr");
            glTexEnvi(GL_TEXTURE_ENV, src2_target, src2);
            checkGLcall("GL_TEXTURE_ENV, src2_target, src2");
            glTexEnvi(GL_TEXTURE_ENV, opr2_target, opr2);
            checkGLcall("GL_TEXTURE_ENV, opr2_target, opr2");
            glTexEnvi(GL_TEXTURE_ENV, src3_target, GL_TEXTURE);
            checkGLcall("GL_TEXTURE_ENV, src3_target, GL_TEXTURE");
            glTexEnvi(GL_TEXTURE_ENV, opr3_target, GL_ONE_MINUS_SRC_ALPHA);
            checkGLcall("GL_TEXTURE_ENV, opr3_target, GL_ONE_MINUS_SRC_ALPHA");
            glTexEnvi(GL_TEXTURE_ENV, scal_target, 1);
            checkGLcall("GL_TEXTURE_ENV, scal_target, 1");
            break;
          case WINED3DTOP_BLENDFACTORALPHA:
            glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_ADD);
            checkGLcall("GL_TEXTURE_ENV, comb_target, GL_ADD");
            glTexEnvi(GL_TEXTURE_ENV, src0_target, src1);
            checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
            glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr1);
            checkGLcall("GL_TEXTURE_ENV, opr0_target, opr1");
            glTexEnvi(GL_TEXTURE_ENV, src1_target, useext(GL_CONSTANT));
            checkGLcall("GL_TEXTURE_ENV, src1_target, useext(GL_CONSTANT)");
            glTexEnvi(GL_TEXTURE_ENV, opr1_target, invopr);
            checkGLcall("GL_TEXTURE_ENV, opr1_target, invopr");
            glTexEnvi(GL_TEXTURE_ENV, src2_target, src2);
            checkGLcall("GL_TEXTURE_ENV, src2_target, src2");
            glTexEnvi(GL_TEXTURE_ENV, opr2_target, opr2);
            checkGLcall("GL_TEXTURE_ENV, opr2_target, opr2");
            glTexEnvi(GL_TEXTURE_ENV, src3_target, useext(GL_CONSTANT));
            checkGLcall("GL_TEXTURE_ENV, src3_target, useext(GL_CONSTANT)");
            glTexEnvi(GL_TEXTURE_ENV, opr3_target, GL_ONE_MINUS_SRC_ALPHA);
            checkGLcall("GL_TEXTURE_ENV, opr3_target, GL_ONE_MINUS_SRC_ALPHA");
            glTexEnvi(GL_TEXTURE_ENV, scal_target, 1);
            checkGLcall("GL_TEXTURE_ENV, scal_target, 1");
            break;
          case WINED3DTOP_BLENDTEXTUREALPHAPM:
            glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_ADD);
            checkGLcall("GL_TEXTURE_ENV, comb_target, GL_ADD");
            glTexEnvi(GL_TEXTURE_ENV, src0_target, src1);
            checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
            glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr1);
            checkGLcall("GL_TEXTURE_ENV, opr0_target, opr1");
            glTexEnvi(GL_TEXTURE_ENV, src1_target, GL_ZERO);
            checkGLcall("GL_TEXTURE_ENV, src1_target, GL_ZERO");
            glTexEnvi(GL_TEXTURE_ENV, opr1_target, invopr);
            checkGLcall("GL_TEXTURE_ENV, opr1_target, invopr");
            glTexEnvi(GL_TEXTURE_ENV, src2_target, src2);
            checkGLcall("GL_TEXTURE_ENV, src2_target, src2");
            glTexEnvi(GL_TEXTURE_ENV, opr2_target, opr2);
            checkGLcall("GL_TEXTURE_ENV, opr2_target, opr2");
            glTexEnvi(GL_TEXTURE_ENV, src3_target, GL_TEXTURE);
            checkGLcall("GL_TEXTURE_ENV, src3_target, GL_TEXTURE");
            glTexEnvi(GL_TEXTURE_ENV, opr3_target, GL_ONE_MINUS_SRC_ALPHA);
            checkGLcall("GL_TEXTURE_ENV, opr3_target, GL_ONE_MINUS_SRC_ALPHA");
            glTexEnvi(GL_TEXTURE_ENV, scal_target, 1);
            checkGLcall("GL_TEXTURE_ENV, scal_target, 1");
            break;
          case WINED3DTOP_MODULATEALPHA_ADDCOLOR:
            glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_ADD);
            checkGLcall("GL_TEXTURE_ENV, comb_target, GL_ADD");  /* Add = a0*a1 + a2*a3 */
            glTexEnvi(GL_TEXTURE_ENV, src0_target, src1);        /*   a0 = src1/opr1    */
            checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
            glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr1);
            checkGLcall("GL_TEXTURE_ENV, opr0_target, opr1");    /*   a1 = 1 (see docs) */
            glTexEnvi(GL_TEXTURE_ENV, src1_target, GL_ZERO);
            checkGLcall("GL_TEXTURE_ENV, src1_target, GL_ZERO");
            glTexEnvi(GL_TEXTURE_ENV, opr1_target, invopr);
            checkGLcall("GL_TEXTURE_ENV, opr1_target, invopr");
            glTexEnvi(GL_TEXTURE_ENV, src2_target, src2);        /*   a2 = arg2         */
            checkGLcall("GL_TEXTURE_ENV, src2_target, src2");
            glTexEnvi(GL_TEXTURE_ENV, opr2_target, opr2);
            checkGLcall("GL_TEXTURE_ENV, opr2_target, opr2");     /*  a3 = src1 alpha   */
            glTexEnvi(GL_TEXTURE_ENV, src3_target, src1);
            checkGLcall("GL_TEXTURE_ENV, src3_target, src1");
            switch (opr) {
            case GL_SRC_COLOR: opr = GL_SRC_ALPHA; break;
            case GL_ONE_MINUS_SRC_COLOR: opr = GL_ONE_MINUS_SRC_ALPHA; break;
            }
            glTexEnvi(GL_TEXTURE_ENV, opr3_target, opr);
            checkGLcall("GL_TEXTURE_ENV, opr3_target, opr");
            glTexEnvi(GL_TEXTURE_ENV, scal_target, 1);
            checkGLcall("GL_TEXTURE_ENV, scal_target, 1");
            break;
          case WINED3DTOP_MODULATECOLOR_ADDALPHA:
            glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_ADD);
            checkGLcall("GL_TEXTURE_ENV, comb_target, GL_ADD");
            glTexEnvi(GL_TEXTURE_ENV, src0_target, src1);
            checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
            glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr1);
            checkGLcall("GL_TEXTURE_ENV, opr0_target, opr1");
            glTexEnvi(GL_TEXTURE_ENV, src1_target, src2);
            checkGLcall("GL_TEXTURE_ENV, src1_target, src2");
            glTexEnvi(GL_TEXTURE_ENV, opr1_target, opr2);
            checkGLcall("GL_TEXTURE_ENV, opr1_target, opr2");
            glTexEnvi(GL_TEXTURE_ENV, src2_target, src1);
            checkGLcall("GL_TEXTURE_ENV, src2_target, src1");
            switch (opr1) {
            case GL_SRC_COLOR: opr = GL_SRC_ALPHA; break;
            case GL_ONE_MINUS_SRC_COLOR: opr = GL_ONE_MINUS_SRC_ALPHA; break;
            }
            glTexEnvi(GL_TEXTURE_ENV, opr2_target, opr);
            checkGLcall("GL_TEXTURE_ENV, opr2_target, opr");
            glTexEnvi(GL_TEXTURE_ENV, src3_target, GL_ZERO);
            checkGLcall("GL_TEXTURE_ENV, src3_target, GL_ZERO");
            glTexEnvi(GL_TEXTURE_ENV, opr3_target, invopr);
            checkGLcall("GL_TEXTURE_ENV, opr3_target, invopr");
            glTexEnvi(GL_TEXTURE_ENV, scal_target, 1);
            checkGLcall("GL_TEXTURE_ENV, scal_target, 1");
            break;
          case WINED3DTOP_MODULATEINVALPHA_ADDCOLOR:
            glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_ADD);
            checkGLcall("GL_TEXTURE_ENV, comb_target, GL_ADD");
            glTexEnvi(GL_TEXTURE_ENV, src0_target, src1);
            checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
            glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr1);
            checkGLcall("GL_TEXTURE_ENV, opr0_target, opr1");
            glTexEnvi(GL_TEXTURE_ENV, src1_target, GL_ZERO);
            checkGLcall("GL_TEXTURE_ENV, src1_target, GL_ZERO");
            glTexEnvi(GL_TEXTURE_ENV, opr1_target, invopr);
            checkGLcall("GL_TEXTURE_ENV, opr1_target, invopr");
            glTexEnvi(GL_TEXTURE_ENV, src2_target, src2);
            checkGLcall("GL_TEXTURE_ENV, src2_target, src2");
            glTexEnvi(GL_TEXTURE_ENV, opr2_target, opr2);
            checkGLcall("GL_TEXTURE_ENV, opr2_target, opr2");
            glTexEnvi(GL_TEXTURE_ENV, src3_target, src1);
            checkGLcall("GL_TEXTURE_ENV, src3_target, src1");
            switch (opr1) {
            case GL_SRC_COLOR: opr = GL_ONE_MINUS_SRC_ALPHA; break;
            case GL_ONE_MINUS_SRC_COLOR: opr = GL_SRC_ALPHA; break;
            case GL_SRC_ALPHA: opr = GL_ONE_MINUS_SRC_ALPHA; break;
            case GL_ONE_MINUS_SRC_ALPHA: opr = GL_SRC_ALPHA; break;
            }
            glTexEnvi(GL_TEXTURE_ENV, opr3_target, opr);
            checkGLcall("GL_TEXTURE_ENV, opr3_target, opr");
            glTexEnvi(GL_TEXTURE_ENV, scal_target, 1);
            checkGLcall("GL_TEXTURE_ENV, scal_target, 1");
            break;
          case WINED3DTOP_MODULATEINVCOLOR_ADDALPHA:
            glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_ADD);
            checkGLcall("GL_TEXTURE_ENV, comb_target, GL_ADD");
            glTexEnvi(GL_TEXTURE_ENV, src0_target, src1);
            checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
            switch (opr1) {
            case GL_SRC_COLOR: opr = GL_ONE_MINUS_SRC_COLOR; break;
            case GL_ONE_MINUS_SRC_COLOR: opr = GL_SRC_COLOR; break;
            case GL_SRC_ALPHA: opr = GL_ONE_MINUS_SRC_ALPHA; break;
            case GL_ONE_MINUS_SRC_ALPHA: opr = GL_SRC_ALPHA; break;
            }
            glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr);
            checkGLcall("GL_TEXTURE_ENV, opr0_target, opr");
            glTexEnvi(GL_TEXTURE_ENV, src1_target, src2);
            checkGLcall("GL_TEXTURE_ENV, src1_target, src2");
            glTexEnvi(GL_TEXTURE_ENV, opr1_target, opr2);
            checkGLcall("GL_TEXTURE_ENV, opr1_target, opr2");
            glTexEnvi(GL_TEXTURE_ENV, src2_target, src1);
            checkGLcall("GL_TEXTURE_ENV, src2_target, src1");
            switch (opr1) {
            case GL_SRC_COLOR: opr = GL_SRC_ALPHA; break;
            case GL_ONE_MINUS_SRC_COLOR: opr = GL_ONE_MINUS_SRC_ALPHA; break;
            }
            glTexEnvi(GL_TEXTURE_ENV, opr2_target, opr);
            checkGLcall("GL_TEXTURE_ENV, opr2_target, opr");
            glTexEnvi(GL_TEXTURE_ENV, src3_target, GL_ZERO);
            checkGLcall("GL_TEXTURE_ENV, src3_target, GL_ZERO");
            glTexEnvi(GL_TEXTURE_ENV, opr3_target, invopr);
            checkGLcall("GL_TEXTURE_ENV, opr3_target, invopr");
            glTexEnvi(GL_TEXTURE_ENV, scal_target, 1);
            checkGLcall("GL_TEXTURE_ENV, scal_target, 1");
            break;
          case WINED3DTOP_MULTIPLYADD:
            glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_ADD);
            checkGLcall("GL_TEXTURE_ENV, comb_target, GL_ADD");
            glTexEnvi(GL_TEXTURE_ENV, src0_target, src3);
            checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
            glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr3);
            checkGLcall("GL_TEXTURE_ENV, opr0_target, opr1");
            glTexEnvi(GL_TEXTURE_ENV, src1_target, GL_ZERO);
            checkGLcall("GL_TEXTURE_ENV, src1_target, GL_ZERO");
            glTexEnvi(GL_TEXTURE_ENV, opr1_target, invopr);
            checkGLcall("GL_TEXTURE_ENV, opr1_target, invopr");
            glTexEnvi(GL_TEXTURE_ENV, src2_target, src1);
            checkGLcall("GL_TEXTURE_ENV, src2_target, src2");
            glTexEnvi(GL_TEXTURE_ENV, opr2_target, opr1);
            checkGLcall("GL_TEXTURE_ENV, opr2_target, opr2");
            glTexEnvi(GL_TEXTURE_ENV, src3_target, src2);
            checkGLcall("GL_TEXTURE_ENV, src3_target, src3");
            glTexEnvi(GL_TEXTURE_ENV, opr3_target, opr2);
            checkGLcall("GL_TEXTURE_ENV, opr3_target, opr3");
            glTexEnvi(GL_TEXTURE_ENV, scal_target, 1);
            checkGLcall("GL_TEXTURE_ENV, scal_target, 1");
            break;

          case WINED3DTOP_BUMPENVMAP:
            {
            }

          case WINED3DTOP_BUMPENVMAPLUMINANCE:
                FIXME("Implement bump environment mapping in GL_NV_texture_env_combine4 path\n");

          default:
            Handled = FALSE;
          }
          if (Handled) {
            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE4_NV);
            checkGLcall("GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE4_NV");

            return;
          }
        } /* GL_NV_texture_env_combine4 */

        Handled = TRUE; /* Again, assume handled */
        switch (op) {
        case WINED3DTOP_DISABLE: /* Only for alpha */
                glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_REPLACE);
                checkGLcall("GL_TEXTURE_ENV, comb_target, GL_REPLACE");
                glTexEnvi(GL_TEXTURE_ENV, src0_target, GL_PREVIOUS_EXT);
                checkGLcall("GL_TEXTURE_ENV, src0_target, GL_PREVIOUS_EXT");
                glTexEnvi(GL_TEXTURE_ENV, opr0_target, GL_SRC_ALPHA);
                checkGLcall("GL_TEXTURE_ENV, opr0_target, GL_SRC_ALPHA");
                glTexEnvi(GL_TEXTURE_ENV, scal_target, 1);
                checkGLcall("GL_TEXTURE_ENV, scal_target, 1");
                break;
        case WINED3DTOP_SELECTARG1:
                glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_REPLACE);
                checkGLcall("GL_TEXTURE_ENV, comb_target, GL_REPLACE");
                glTexEnvi(GL_TEXTURE_ENV, src0_target, src1);
                checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
                glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr1);
                checkGLcall("GL_TEXTURE_ENV, opr0_target, opr1");
                glTexEnvi(GL_TEXTURE_ENV, scal_target, 1);
                checkGLcall("GL_TEXTURE_ENV, scal_target, 1");
                break;
        case WINED3DTOP_SELECTARG2:
                glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_REPLACE);
                checkGLcall("GL_TEXTURE_ENV, comb_target, GL_REPLACE");
                glTexEnvi(GL_TEXTURE_ENV, src0_target, src2);
                checkGLcall("GL_TEXTURE_ENV, src0_target, src2");
                glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr2);
                checkGLcall("GL_TEXTURE_ENV, opr0_target, opr2");
                glTexEnvi(GL_TEXTURE_ENV, scal_target, 1);
                checkGLcall("GL_TEXTURE_ENV, scal_target, 1");
                break;
        case WINED3DTOP_MODULATE:
                glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_MODULATE);
                checkGLcall("GL_TEXTURE_ENV, comb_target, GL_MODULATE");
                glTexEnvi(GL_TEXTURE_ENV, src0_target, src1);
                checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
                glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr1);
                checkGLcall("GL_TEXTURE_ENV, opr0_target, opr1");
                glTexEnvi(GL_TEXTURE_ENV, src1_target, src2);
                checkGLcall("GL_TEXTURE_ENV, src1_target, src2");
                glTexEnvi(GL_TEXTURE_ENV, opr1_target, opr2);
                checkGLcall("GL_TEXTURE_ENV, opr1_target, opr2");
                glTexEnvi(GL_TEXTURE_ENV, scal_target, 1);
                checkGLcall("GL_TEXTURE_ENV, scal_target, 1");
                break;
        case WINED3DTOP_MODULATE2X:
                glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_MODULATE);
                checkGLcall("GL_TEXTURE_ENV, comb_target, GL_MODULATE");
                glTexEnvi(GL_TEXTURE_ENV, src0_target, src1);
                checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
                glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr1);
                checkGLcall("GL_TEXTURE_ENV, opr0_target, opr1");
                glTexEnvi(GL_TEXTURE_ENV, src1_target, src2);
                checkGLcall("GL_TEXTURE_ENV, src1_target, src2");
                glTexEnvi(GL_TEXTURE_ENV, opr1_target, opr2);
                checkGLcall("GL_TEXTURE_ENV, opr1_target, opr2");
                glTexEnvi(GL_TEXTURE_ENV, scal_target, 2);
                checkGLcall("GL_TEXTURE_ENV, scal_target, 2");
                break;
        case WINED3DTOP_MODULATE4X:
                glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_MODULATE);
                checkGLcall("GL_TEXTURE_ENV, comb_target, GL_MODULATE");
                glTexEnvi(GL_TEXTURE_ENV, src0_target, src1);
                checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
                glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr1);
                checkGLcall("GL_TEXTURE_ENV, opr0_target, opr1");
                glTexEnvi(GL_TEXTURE_ENV, src1_target, src2);
                checkGLcall("GL_TEXTURE_ENV, src1_target, src2");
                glTexEnvi(GL_TEXTURE_ENV, opr1_target, opr2);
                checkGLcall("GL_TEXTURE_ENV, opr1_target, opr2");
                glTexEnvi(GL_TEXTURE_ENV, scal_target, 4);
                checkGLcall("GL_TEXTURE_ENV, scal_target, 4");
                break;
        case WINED3DTOP_ADD:
                glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_ADD);
                checkGLcall("GL_TEXTURE_ENV, comb_target, GL_ADD");
                glTexEnvi(GL_TEXTURE_ENV, src0_target, src1);
                checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
                glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr1);
                checkGLcall("GL_TEXTURE_ENV, opr0_target, opr1");
                glTexEnvi(GL_TEXTURE_ENV, src1_target, src2);
                checkGLcall("GL_TEXTURE_ENV, src1_target, src2");
                glTexEnvi(GL_TEXTURE_ENV, opr1_target, opr2);
                checkGLcall("GL_TEXTURE_ENV, opr1_target, opr2");
                glTexEnvi(GL_TEXTURE_ENV, scal_target, 1);
                checkGLcall("GL_TEXTURE_ENV, scal_target, 1");
                break;
        case WINED3DTOP_ADDSIGNED:
                glTexEnvi(GL_TEXTURE_ENV, comb_target, useext(GL_ADD_SIGNED));
                checkGLcall("GL_TEXTURE_ENV, comb_target, useext((GL_ADD_SIGNED)");
                glTexEnvi(GL_TEXTURE_ENV, src0_target, src1);
                checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
                glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr1);
                checkGLcall("GL_TEXTURE_ENV, opr0_target, opr1");
                glTexEnvi(GL_TEXTURE_ENV, src1_target, src2);
                checkGLcall("GL_TEXTURE_ENV, src1_target, src2");
                glTexEnvi(GL_TEXTURE_ENV, opr1_target, opr2);
                checkGLcall("GL_TEXTURE_ENV, opr1_target, opr2");
                glTexEnvi(GL_TEXTURE_ENV, scal_target, 1);
                checkGLcall("GL_TEXTURE_ENV, scal_target, 1");
                break;
        case WINED3DTOP_ADDSIGNED2X:
                glTexEnvi(GL_TEXTURE_ENV, comb_target, useext(GL_ADD_SIGNED));
                checkGLcall("GL_TEXTURE_ENV, comb_target, useext(GL_ADD_SIGNED)");
                glTexEnvi(GL_TEXTURE_ENV, src0_target, src1);
                checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
                glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr1);
                checkGLcall("GL_TEXTURE_ENV, opr0_target, opr1");
                glTexEnvi(GL_TEXTURE_ENV, src1_target, src2);
                checkGLcall("GL_TEXTURE_ENV, src1_target, src2");
                glTexEnvi(GL_TEXTURE_ENV, opr1_target, opr2);
                checkGLcall("GL_TEXTURE_ENV, opr1_target, opr2");
                glTexEnvi(GL_TEXTURE_ENV, scal_target, 2);
                checkGLcall("GL_TEXTURE_ENV, scal_target, 2");
                break;
        case WINED3DTOP_SUBTRACT:
          if (GL_SUPPORT(ARB_TEXTURE_ENV_COMBINE)) {
                glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_SUBTRACT);
                checkGLcall("GL_TEXTURE_ENV, comb_target, useext(GL_SUBTRACT)");
                glTexEnvi(GL_TEXTURE_ENV, src0_target, src1);
                checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
                glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr1);
                checkGLcall("GL_TEXTURE_ENV, opr0_target, opr1");
                glTexEnvi(GL_TEXTURE_ENV, src1_target, src2);
                checkGLcall("GL_TEXTURE_ENV, src1_target, src2");
                glTexEnvi(GL_TEXTURE_ENV, opr1_target, opr2);
                checkGLcall("GL_TEXTURE_ENV, opr1_target, opr2");
                glTexEnvi(GL_TEXTURE_ENV, scal_target, 1);
                checkGLcall("GL_TEXTURE_ENV, scal_target, 1");
          } else {
                FIXME("This version of opengl does not support GL_SUBTRACT\n");
          }
          break;

        case WINED3DTOP_BLENDDIFFUSEALPHA:
                glTexEnvi(GL_TEXTURE_ENV, comb_target, useext(GL_INTERPOLATE));
                checkGLcall("GL_TEXTURE_ENV, comb_target, useext(GL_INTERPOLATE)");
                glTexEnvi(GL_TEXTURE_ENV, src0_target, src1);
                checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
                glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr1);
                checkGLcall("GL_TEXTURE_ENV, opr0_target, opr1");
                glTexEnvi(GL_TEXTURE_ENV, src1_target, src2);
                checkGLcall("GL_TEXTURE_ENV, src1_target, src2");
                glTexEnvi(GL_TEXTURE_ENV, opr1_target, opr2);
                checkGLcall("GL_TEXTURE_ENV, opr1_target, opr2");
                glTexEnvi(GL_TEXTURE_ENV, src2_target, useext(GL_PRIMARY_COLOR));
                checkGLcall("GL_TEXTURE_ENV, src2_target, GL_PRIMARY_COLOR");
                glTexEnvi(GL_TEXTURE_ENV, opr2_target, GL_SRC_ALPHA);
                checkGLcall("GL_TEXTURE_ENV, opr2_target, GL_SRC_ALPHA");
                glTexEnvi(GL_TEXTURE_ENV, scal_target, 1);
                checkGLcall("GL_TEXTURE_ENV, scal_target, 1");
                break;
        case WINED3DTOP_BLENDTEXTUREALPHA:
                glTexEnvi(GL_TEXTURE_ENV, comb_target, useext(GL_INTERPOLATE));
                checkGLcall("GL_TEXTURE_ENV, comb_target, useext(GL_INTERPOLATE)");
                glTexEnvi(GL_TEXTURE_ENV, src0_target, src1);
                checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
                glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr1);
                checkGLcall("GL_TEXTURE_ENV, opr0_target, opr1");
                glTexEnvi(GL_TEXTURE_ENV, src1_target, src2);
                checkGLcall("GL_TEXTURE_ENV, src1_target, src2");
                glTexEnvi(GL_TEXTURE_ENV, opr1_target, opr2);
                checkGLcall("GL_TEXTURE_ENV, opr1_target, opr2");
                glTexEnvi(GL_TEXTURE_ENV, src2_target, GL_TEXTURE);
                checkGLcall("GL_TEXTURE_ENV, src2_target, GL_TEXTURE");
                glTexEnvi(GL_TEXTURE_ENV, opr2_target, GL_SRC_ALPHA);
                checkGLcall("GL_TEXTURE_ENV, opr2_target, GL_SRC_ALPHA");
                glTexEnvi(GL_TEXTURE_ENV, scal_target, 1);
                checkGLcall("GL_TEXTURE_ENV, scal_target, 1");
                break;
        case WINED3DTOP_BLENDFACTORALPHA:
                glTexEnvi(GL_TEXTURE_ENV, comb_target, useext(GL_INTERPOLATE));
                checkGLcall("GL_TEXTURE_ENV, comb_target, useext(GL_INTERPOLATE)");
                glTexEnvi(GL_TEXTURE_ENV, src0_target, src1);
                checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
                glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr1);
                checkGLcall("GL_TEXTURE_ENV, opr0_target, opr1");
                glTexEnvi(GL_TEXTURE_ENV, src1_target, src2);
                checkGLcall("GL_TEXTURE_ENV, src1_target, src2");
                glTexEnvi(GL_TEXTURE_ENV, opr1_target, opr2);
                checkGLcall("GL_TEXTURE_ENV, opr1_target, opr2");
                glTexEnvi(GL_TEXTURE_ENV, src2_target, useext(GL_CONSTANT));
                checkGLcall("GL_TEXTURE_ENV, src2_target, GL_CONSTANT");
                glTexEnvi(GL_TEXTURE_ENV, opr2_target, GL_SRC_ALPHA);
                checkGLcall("GL_TEXTURE_ENV, opr2_target, GL_SRC_ALPHA");
                glTexEnvi(GL_TEXTURE_ENV, scal_target, 1);
                checkGLcall("GL_TEXTURE_ENV, scal_target, 1");
                break;
        case WINED3DTOP_BLENDCURRENTALPHA:
                glTexEnvi(GL_TEXTURE_ENV, comb_target, useext(GL_INTERPOLATE));
                checkGLcall("GL_TEXTURE_ENV, comb_target, useext(GL_INTERPOLATE)");
                glTexEnvi(GL_TEXTURE_ENV, src0_target, src1);
                checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
                glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr1);
                checkGLcall("GL_TEXTURE_ENV, opr0_target, opr1");
                glTexEnvi(GL_TEXTURE_ENV, src1_target, src2);
                checkGLcall("GL_TEXTURE_ENV, src1_target, src2");
                glTexEnvi(GL_TEXTURE_ENV, opr1_target, opr2);
                checkGLcall("GL_TEXTURE_ENV, opr1_target, opr2");
                glTexEnvi(GL_TEXTURE_ENV, src2_target, useext(GL_PREVIOUS));
                checkGLcall("GL_TEXTURE_ENV, src2_target, GL_PREVIOUS");
                glTexEnvi(GL_TEXTURE_ENV, opr2_target, GL_SRC_ALPHA);
                checkGLcall("GL_TEXTURE_ENV, opr2_target, GL_SRC_ALPHA");
                glTexEnvi(GL_TEXTURE_ENV, scal_target, 1);
                checkGLcall("GL_TEXTURE_ENV, scal_target, 1");
                break;
        case WINED3DTOP_DOTPRODUCT3:
                if (GL_SUPPORT(ARB_TEXTURE_ENV_DOT3)) {
                  glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_DOT3_RGBA_ARB);
                  checkGLcall("GL_TEXTURE_ENV, comb_target, GL_DOT3_RGBA_ARB");
                } else if (GL_SUPPORT(EXT_TEXTURE_ENV_DOT3)) {
                  glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_DOT3_RGBA_EXT);
                  checkGLcall("GL_TEXTURE_ENV, comb_target, GL_DOT3_RGBA_EXT");
                } else {
                  FIXME("This version of opengl does not support GL_DOT3\n");
                }
                glTexEnvi(GL_TEXTURE_ENV, src0_target, src1);
                checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
                glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr1);
                checkGLcall("GL_TEXTURE_ENV, opr0_target, opr1");
                glTexEnvi(GL_TEXTURE_ENV, src1_target, src2);
                checkGLcall("GL_TEXTURE_ENV, src1_target, src2");
                glTexEnvi(GL_TEXTURE_ENV, opr1_target, opr2);
                checkGLcall("GL_TEXTURE_ENV, opr1_target, opr2");
                glTexEnvi(GL_TEXTURE_ENV, scal_target, 1);
                checkGLcall("GL_TEXTURE_ENV, scal_target, 1");
                break;
        case WINED3DTOP_LERP:
                glTexEnvi(GL_TEXTURE_ENV, comb_target, useext(GL_INTERPOLATE));
                checkGLcall("GL_TEXTURE_ENV, comb_target, useext(GL_INTERPOLATE)");
                glTexEnvi(GL_TEXTURE_ENV, src0_target, src1);
                checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
                glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr1);
                checkGLcall("GL_TEXTURE_ENV, opr0_target, opr1");
                glTexEnvi(GL_TEXTURE_ENV, src1_target, src2);
                checkGLcall("GL_TEXTURE_ENV, src1_target, src2");
                glTexEnvi(GL_TEXTURE_ENV, opr1_target, opr2);
                checkGLcall("GL_TEXTURE_ENV, opr1_target, opr2");
                glTexEnvi(GL_TEXTURE_ENV, src2_target, src3);
                checkGLcall("GL_TEXTURE_ENV, src2_target, src3");
                glTexEnvi(GL_TEXTURE_ENV, opr2_target, opr3);
                checkGLcall("GL_TEXTURE_ENV, opr2_target, opr3");
                glTexEnvi(GL_TEXTURE_ENV, scal_target, 1);
                checkGLcall("GL_TEXTURE_ENV, scal_target, 1");
                break;
        case WINED3DTOP_ADDSMOOTH:
                if (GL_SUPPORT(ATI_TEXTURE_ENV_COMBINE3)) {
                  glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_MODULATE_ADD_ATI);
                  checkGLcall("GL_TEXTURE_ENV, comb_target, GL_MODULATE_ADD_ATI");
                  glTexEnvi(GL_TEXTURE_ENV, src0_target, src1);
                  checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
                  switch (opr1) {
                  case GL_SRC_COLOR: opr = GL_ONE_MINUS_SRC_COLOR; break;
                  case GL_ONE_MINUS_SRC_COLOR: opr = GL_SRC_COLOR; break;
                  case GL_SRC_ALPHA: opr = GL_ONE_MINUS_SRC_ALPHA; break;
                  case GL_ONE_MINUS_SRC_ALPHA: opr = GL_SRC_ALPHA; break;
                  }
                  glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr);
                  checkGLcall("GL_TEXTURE_ENV, opr0_target, opr");
                  glTexEnvi(GL_TEXTURE_ENV, src1_target, src1);
                  checkGLcall("GL_TEXTURE_ENV, src1_target, src1");
                  glTexEnvi(GL_TEXTURE_ENV, opr1_target, opr1);
                  checkGLcall("GL_TEXTURE_ENV, opr1_target, opr1");
                  glTexEnvi(GL_TEXTURE_ENV, src2_target, src2);
                  checkGLcall("GL_TEXTURE_ENV, src2_target, src2");
                  glTexEnvi(GL_TEXTURE_ENV, opr2_target, opr2);
                  checkGLcall("GL_TEXTURE_ENV, opr2_target, opr2");
                  glTexEnvi(GL_TEXTURE_ENV, scal_target, 1);
                  checkGLcall("GL_TEXTURE_ENV, scal_target, 1");
                } else
                  Handled = FALSE;
                break;
        case WINED3DTOP_BLENDTEXTUREALPHAPM:
                if (GL_SUPPORT(ATI_TEXTURE_ENV_COMBINE3)) {
                  glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_MODULATE_ADD_ATI);
                  checkGLcall("GL_TEXTURE_ENV, comb_target, GL_MODULATE_ADD_ATI");
                  glTexEnvi(GL_TEXTURE_ENV, src0_target, GL_TEXTURE);
                  checkGLcall("GL_TEXTURE_ENV, src0_target, GL_TEXTURE");
                  glTexEnvi(GL_TEXTURE_ENV, opr0_target, GL_ONE_MINUS_SRC_ALPHA);
                  checkGLcall("GL_TEXTURE_ENV, opr0_target, GL_ONE_MINUS_SRC_APHA");
                  glTexEnvi(GL_TEXTURE_ENV, src1_target, src1);
                  checkGLcall("GL_TEXTURE_ENV, src1_target, src1");
                  glTexEnvi(GL_TEXTURE_ENV, opr1_target, opr1);
                  checkGLcall("GL_TEXTURE_ENV, opr1_target, opr1");
                  glTexEnvi(GL_TEXTURE_ENV, src2_target, src2);
                  checkGLcall("GL_TEXTURE_ENV, src2_target, src2");
                  glTexEnvi(GL_TEXTURE_ENV, opr2_target, opr2);
                  checkGLcall("GL_TEXTURE_ENV, opr2_target, opr2");
                  glTexEnvi(GL_TEXTURE_ENV, scal_target, 1);
                  checkGLcall("GL_TEXTURE_ENV, scal_target, 1");
                } else
                  Handled = FALSE;
                break;
        case WINED3DTOP_MODULATEALPHA_ADDCOLOR:
                if (GL_SUPPORT(ATI_TEXTURE_ENV_COMBINE3)) {
                  glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_MODULATE_ADD_ATI);
                  checkGLcall("GL_TEXTURE_ENV, comb_target, GL_MODULATE_ADD_ATI");
                  glTexEnvi(GL_TEXTURE_ENV, src0_target, src1);
                  checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
                  switch (opr1) {
                  case GL_SRC_COLOR: opr = GL_SRC_ALPHA; break;
                  case GL_ONE_MINUS_SRC_COLOR: opr = GL_ONE_MINUS_SRC_ALPHA; break;
                  case GL_SRC_ALPHA: opr = GL_SRC_ALPHA; break;
                  case GL_ONE_MINUS_SRC_ALPHA: opr = GL_ONE_MINUS_SRC_ALPHA; break;
                  }
                  glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr);
                  checkGLcall("GL_TEXTURE_ENV, opr0_target, opr");
                  glTexEnvi(GL_TEXTURE_ENV, src1_target, src1);
                  checkGLcall("GL_TEXTURE_ENV, src1_target, src1");
                  glTexEnvi(GL_TEXTURE_ENV, opr1_target, opr1);
                  checkGLcall("GL_TEXTURE_ENV, opr1_target, opr1");
                  glTexEnvi(GL_TEXTURE_ENV, src2_target, src2);
                  checkGLcall("GL_TEXTURE_ENV, src2_target, src2");
                  glTexEnvi(GL_TEXTURE_ENV, opr2_target, opr2);
                  checkGLcall("GL_TEXTURE_ENV, opr2_target, opr2");
                  glTexEnvi(GL_TEXTURE_ENV, scal_target, 1);
                  checkGLcall("GL_TEXTURE_ENV, scal_target, 1");
                } else
                  Handled = FALSE;
                break;
        case WINED3DTOP_MODULATECOLOR_ADDALPHA:
                if (GL_SUPPORT(ATI_TEXTURE_ENV_COMBINE3)) {
                  glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_MODULATE_ADD_ATI);
                  checkGLcall("GL_TEXTURE_ENV, comb_target, GL_MODULATE_ADD_ATI");
                  glTexEnvi(GL_TEXTURE_ENV, src0_target, src1);
                  checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
                  glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr1);
                  checkGLcall("GL_TEXTURE_ENV, opr0_target, opr1");
                  glTexEnvi(GL_TEXTURE_ENV, src1_target, src1);
                  checkGLcall("GL_TEXTURE_ENV, src1_target, src1");
                  switch (opr1) {
                  case GL_SRC_COLOR: opr = GL_SRC_ALPHA; break;
                  case GL_ONE_MINUS_SRC_COLOR: opr = GL_ONE_MINUS_SRC_ALPHA; break;
                  case GL_SRC_ALPHA: opr = GL_SRC_ALPHA; break;
                  case GL_ONE_MINUS_SRC_ALPHA: opr = GL_ONE_MINUS_SRC_ALPHA; break;
                  }
                  glTexEnvi(GL_TEXTURE_ENV, opr1_target, opr);
                  checkGLcall("GL_TEXTURE_ENV, opr1_target, opr");
                  glTexEnvi(GL_TEXTURE_ENV, src2_target, src2);
                  checkGLcall("GL_TEXTURE_ENV, src2_target, src2");
                  glTexEnvi(GL_TEXTURE_ENV, opr2_target, opr2);
                  checkGLcall("GL_TEXTURE_ENV, opr2_target, opr2");
                  glTexEnvi(GL_TEXTURE_ENV, scal_target, 1);
                  checkGLcall("GL_TEXTURE_ENV, scal_target, 1");
                } else
                  Handled = FALSE;
                break;
        case WINED3DTOP_MODULATEINVALPHA_ADDCOLOR:
                if (GL_SUPPORT(ATI_TEXTURE_ENV_COMBINE3)) {
                  glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_MODULATE_ADD_ATI);
                  checkGLcall("GL_TEXTURE_ENV, comb_target, GL_MODULATE_ADD_ATI");
                  glTexEnvi(GL_TEXTURE_ENV, src0_target, src1);
                  checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
                  switch (opr1) {
                  case GL_SRC_COLOR: opr = GL_ONE_MINUS_SRC_ALPHA; break;
                  case GL_ONE_MINUS_SRC_COLOR: opr = GL_SRC_ALPHA; break;
                  case GL_SRC_ALPHA: opr = GL_ONE_MINUS_SRC_ALPHA; break;
                  case GL_ONE_MINUS_SRC_ALPHA: opr = GL_SRC_ALPHA; break;
                  }
                  glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr);
                  checkGLcall("GL_TEXTURE_ENV, opr0_target, opr");
                  glTexEnvi(GL_TEXTURE_ENV, src1_target, src1);
                  checkGLcall("GL_TEXTURE_ENV, src1_target, src1");
                  glTexEnvi(GL_TEXTURE_ENV, opr1_target, opr1);
                  checkGLcall("GL_TEXTURE_ENV, opr1_target, opr1");
                  glTexEnvi(GL_TEXTURE_ENV, src2_target, src2);
                  checkGLcall("GL_TEXTURE_ENV, src2_target, src2");
                  glTexEnvi(GL_TEXTURE_ENV, opr2_target, opr2);
                  checkGLcall("GL_TEXTURE_ENV, opr2_target, opr2");
                  glTexEnvi(GL_TEXTURE_ENV, scal_target, 1);
                  checkGLcall("GL_TEXTURE_ENV, scal_target, 1");
                } else
                  Handled = FALSE;
                break;
        case WINED3DTOP_MODULATEINVCOLOR_ADDALPHA:
                if (GL_SUPPORT(ATI_TEXTURE_ENV_COMBINE3)) {
                  glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_MODULATE_ADD_ATI);
                  checkGLcall("GL_TEXTURE_ENV, comb_target, GL_MODULATE_ADD_ATI");
                  glTexEnvi(GL_TEXTURE_ENV, src0_target, src1);
                  checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
                  switch (opr1) {
                  case GL_SRC_COLOR: opr = GL_ONE_MINUS_SRC_COLOR; break;
                  case GL_ONE_MINUS_SRC_COLOR: opr = GL_SRC_COLOR; break;
                  case GL_SRC_ALPHA: opr = GL_ONE_MINUS_SRC_ALPHA; break;
                  case GL_ONE_MINUS_SRC_ALPHA: opr = GL_SRC_ALPHA; break;
                  }
                  glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr);
                  checkGLcall("GL_TEXTURE_ENV, opr0_target, opr");
                  glTexEnvi(GL_TEXTURE_ENV, src1_target, src1);
                  checkGLcall("GL_TEXTURE_ENV, src1_target, src1");
                  switch (opr1) {
                  case GL_SRC_COLOR: opr = GL_SRC_ALPHA; break;
                  case GL_ONE_MINUS_SRC_COLOR: opr = GL_ONE_MINUS_SRC_ALPHA; break;
                  case GL_SRC_ALPHA: opr = GL_SRC_ALPHA; break;
                  case GL_ONE_MINUS_SRC_ALPHA: opr = GL_ONE_MINUS_SRC_ALPHA; break;
                  }
                  glTexEnvi(GL_TEXTURE_ENV, opr1_target, opr);
                  checkGLcall("GL_TEXTURE_ENV, opr1_target, opr");
                  glTexEnvi(GL_TEXTURE_ENV, src2_target, src2);
                  checkGLcall("GL_TEXTURE_ENV, src2_target, src2");
                  glTexEnvi(GL_TEXTURE_ENV, opr2_target, opr2);
                  checkGLcall("GL_TEXTURE_ENV, opr2_target, opr2");
                  glTexEnvi(GL_TEXTURE_ENV, scal_target, 1);
                  checkGLcall("GL_TEXTURE_ENV, scal_target, 1");
                } else
                  Handled = FALSE;
                break;
        case WINED3DTOP_MULTIPLYADD:
                if (GL_SUPPORT(ATI_TEXTURE_ENV_COMBINE3)) {
                  glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_MODULATE_ADD_ATI);
                  checkGLcall("GL_TEXTURE_ENV, comb_target, GL_MODULATE_ADD_ATI");
                  glTexEnvi(GL_TEXTURE_ENV, src0_target, src3);
                  checkGLcall("GL_TEXTURE_ENV, src0_target, src3");
                  glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr3);
                  checkGLcall("GL_TEXTURE_ENV, opr0_target, opr3");
                  glTexEnvi(GL_TEXTURE_ENV, src1_target, src1);
                  checkGLcall("GL_TEXTURE_ENV, src1_target, src1");
                  glTexEnvi(GL_TEXTURE_ENV, opr1_target, opr1);
                  checkGLcall("GL_TEXTURE_ENV, opr1_target, opr1");
                  glTexEnvi(GL_TEXTURE_ENV, src2_target, src2);
                  checkGLcall("GL_TEXTURE_ENV, src2_target, src2");
                  glTexEnvi(GL_TEXTURE_ENV, opr2_target, opr2);
                  checkGLcall("GL_TEXTURE_ENV, opr2_target, opr2");
                  glTexEnvi(GL_TEXTURE_ENV, scal_target, 1);
                  checkGLcall("GL_TEXTURE_ENV, scal_target, 1");
                } else
                  Handled = FALSE;
                break;
        case WINED3DTOP_BUMPENVMAPLUMINANCE:
                if(GL_SUPPORT(ATI_ENVMAP_BUMPMAP)) {
                    /* Some apps use BUMPENVMAPLUMINANCE instead of D3DTOP_BUMPENVMAP, although
                     * they check for the non-luminance cap flag. Well, give them what they asked
                     * for :-)
                     */
                    WARN("Application uses WINED3DTOP_BUMPENVMAPLUMINANCE\n");
                } else {
                    Handled = FALSE;
                    break;
                }
                /* Fall through */
        case WINED3DTOP_BUMPENVMAP:
                if(GL_SUPPORT(ATI_ENVMAP_BUMPMAP)) {
                    TRACE("Using ati bumpmap on stage %d, target %d\n", Stage, Stage + 1);
                    glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_BUMP_ENVMAP_ATI);
                    checkGLcall("glTexEnvi(GL_TEXTURE_ENV, comb_target, GL_BUMP_ENVMAP_ATI)");
                    glTexEnvi(GL_TEXTURE_ENV, GL_BUMP_TARGET_ATI, GL_TEXTURE0_ARB + Stage + 1);
                    checkGLcall("glTexEnvi(GL_TEXTURE_ENV, GL_BUMP_TARGET_ATI, GL_TEXTURE0_ARB + Stage + 1)");
                    glTexEnvi(GL_TEXTURE_ENV, src0_target, src3);
                    checkGLcall("GL_TEXTURE_ENV, src0_target, src3");
                    glTexEnvi(GL_TEXTURE_ENV, opr0_target, opr3);
                    checkGLcall("GL_TEXTURE_ENV, opr0_target, opr3");
                    glTexEnvi(GL_TEXTURE_ENV, src1_target, src1);
                    checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
                    glTexEnvi(GL_TEXTURE_ENV, opr1_target, opr1);
                    checkGLcall("GL_TEXTURE_ENV, opr1_target, opr1");
                    glTexEnvi(GL_TEXTURE_ENV, src2_target, src2);
                    checkGLcall("GL_TEXTURE_ENV, src0_target, src1");
                    glTexEnvi(GL_TEXTURE_ENV, opr2_target, opr2);
                    checkGLcall("GL_TEXTURE_ENV, opr2_target, opr2");

                    Handled = TRUE;
                    break;
                } else if(GL_SUPPORT(NV_TEXTURE_SHADER2)) {
                    /* Technically texture shader support without register combiners is possible, but not expected to occur
                     * on real world cards, so for now a fixme should be enough
                     */
                    FIXME("Implement bump mapping with GL_NV_texture_shader in non register combiner path\n");
                }
        default:
                Handled = FALSE;
        }

        if (Handled) {
          BOOL  combineOK = TRUE;
          if (GL_SUPPORT(NV_TEXTURE_ENV_COMBINE4)) {
            DWORD op2;

            if (isAlpha) {
              op2 = This->stateBlock->textureState[Stage][WINED3DTSS_COLOROP];
            } else {
              op2 = This->stateBlock->textureState[Stage][WINED3DTSS_ALPHAOP];
            }

            /* Note: If COMBINE4 in effect can't go back to combine! */
            switch (op2) {
            case WINED3DTOP_ADDSMOOTH:
            case WINED3DTOP_BLENDTEXTUREALPHAPM:
            case WINED3DTOP_MODULATEALPHA_ADDCOLOR:
            case WINED3DTOP_MODULATECOLOR_ADDALPHA:
            case WINED3DTOP_MODULATEINVALPHA_ADDCOLOR:
            case WINED3DTOP_MODULATEINVCOLOR_ADDALPHA:
            case WINED3DTOP_MULTIPLYADD:
              /* Ignore those implemented in both cases */
              switch (op) {
              case WINED3DTOP_SELECTARG1:
              case WINED3DTOP_SELECTARG2:
                combineOK = FALSE;
                Handled   = FALSE;
                break;
              default:
                FIXME("Can't use COMBINE4 and COMBINE together, thisop=%s, otherop=%s, isAlpha(%d)\n", debug_d3dtop(op), debug_d3dtop(op2), isAlpha);
                return;
              }
            }
          }

          if (combineOK) {
            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, useext(GL_COMBINE));
            checkGLcall("GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, useext(GL_COMBINE)");

            return;
          }
        }

        /* After all the extensions, if still unhandled, report fixme */
        FIXME("Unhandled texture operation %s\n", debug_d3dtop(op));
        #undef GLINFO_LOCATION
}
#endif

/* Setup this textures matrix according to the texture flags*/
void set_texture_matrix(const float *smat, DWORD flags, BOOL calculatedCoords)
{
    float mat[16];

    glMatrixMode(GL_TEXTURE);
    checkGLcall("glMatrixMode(GL_TEXTURE)");

    if (flags == WINED3DTTFF_DISABLE) {
        glLoadIdentity();
        checkGLcall("glLoadIdentity()");
        return;
    }

    if (flags == (WINED3DTTFF_COUNT1|WINED3DTTFF_PROJECTED)) {
        ERR("Invalid texture transform flags: WINED3DTTFF_COUNT1|WINED3DTTFF_PROJECTED\n");
        return;
    }

    memcpy(mat, smat, 16 * sizeof(float));

    switch (flags & ~WINED3DTTFF_PROJECTED) {
    case WINED3DTTFF_COUNT1: mat[1] = mat[5] = mat[13] = 0;
    case WINED3DTTFF_COUNT2: mat[2] = mat[6] = mat[10] = mat[14] = 0;
    default: mat[3] = mat[7] = mat[11] = 0, mat[15] = 1;
    }

    if (flags & WINED3DTTFF_PROJECTED) {
        switch (flags & ~WINED3DTTFF_PROJECTED) {
        case WINED3DTTFF_COUNT2:
            mat[3] = mat[1], mat[7] = mat[5], mat[11] = mat[9], mat[15] = mat[13];
            mat[1] = mat[5] = mat[9] = mat[13] = 0;
            break;
        case WINED3DTTFF_COUNT3:
            mat[3] = mat[2], mat[7] = mat[6], mat[11] = mat[10], mat[15] = mat[14];
            mat[2] = mat[6] = mat[10] = mat[14] = 0;
            break;
        }
    } else if(!calculatedCoords) { /* under directx the R/Z coord can be used for translation, under opengl we use the Q coord instead */
        mat[12] = mat[8];
        mat[13] = mat[9];
    }

    glLoadMatrixf(mat);
    checkGLcall("glLoadMatrixf(mat)");
}

#define GLINFO_LOCATION ((IWineD3DImpl *)(This->wineD3D))->gl_info

/* This small helper function is used to convert a bitmask into the number of masked bits */
unsigned int count_bits(unsigned int mask)
{
    unsigned int count;
    for (count = 0; mask; ++count)
    {
        mask &= mask - 1;
    }
    return count;
}

/* Helper function for retrieving color info for ChoosePixelFormat and wglChoosePixelFormatARB.
 * The later function requires individual color components. */
BOOL getColorBits(WINED3DFORMAT fmt, short *redSize, short *greenSize, short *blueSize, short *alphaSize, short *totalSize)
{
    const StaticPixelFormatDesc *desc;

    TRACE("fmt: %s\n", debug_d3dformat(fmt));
    switch(fmt)
    {
        case WINED3DFMT_X8R8G8B8:
        case WINED3DFMT_R8G8B8:
        case WINED3DFMT_A8R8G8B8:
        case WINED3DFMT_A2R10G10B10:
        case WINED3DFMT_X1R5G5B5:
        case WINED3DFMT_A1R5G5B5:
        case WINED3DFMT_R5G6B5:
        case WINED3DFMT_R3G3B2:
        case WINED3DFMT_A8P8:
        case WINED3DFMT_P8:
            break;
        default:
            ERR("Unsupported format: %s\n", debug_d3dformat(fmt));
            return FALSE;
    }

    desc = getFormatDescEntry(fmt, NULL, NULL);
    if(!desc)
    {
        ERR("Unable to look up format: 0x%x\n", fmt);
        return FALSE;
    }
    *redSize = count_bits(desc->redMask);
    *greenSize = count_bits(desc->greenMask);
    *blueSize = count_bits(desc->blueMask);
    *alphaSize = count_bits(desc->alphaMask);
    *totalSize = *redSize + *greenSize + *blueSize + *alphaSize;

    TRACE("Returning red:  %d, green: %d, blue: %d, alpha: %d, total: %d for fmt=%s\n", *redSize, *greenSize, *blueSize, *alphaSize, *totalSize, debug_d3dformat(fmt));
    return TRUE;
}

/* Helper function for retrieving depth/stencil info for ChoosePixelFormat and wglChoosePixelFormatARB */
BOOL getDepthStencilBits(WINED3DFORMAT fmt, short *depthSize, short *stencilSize)
{
    const StaticPixelFormatDesc *desc;

    TRACE("fmt: %s\n", debug_d3dformat(fmt));
    switch(fmt)
    {
        case WINED3DFMT_D16_LOCKABLE:
        case WINED3DFMT_D16:
        case WINED3DFMT_D15S1:
        case WINED3DFMT_D24X8:
        case WINED3DFMT_D24X4S4:
        case WINED3DFMT_D24S8:
        case WINED3DFMT_D24FS8:
        case WINED3DFMT_D32:
            break;
        default:
            FIXME("Unsupported stencil format: %s\n", debug_d3dformat(fmt));
            return FALSE;
    }

    desc = getFormatDescEntry(fmt, NULL, NULL);
    if(!desc)
    {
        ERR("Unable to look up format: 0x%x\n", fmt);
        return FALSE;
    }
    *depthSize = desc->depthSize;
    *stencilSize = desc->stencilSize;

    TRACE("Returning depthSize: %d and stencilSize: %d for fmt=%s\n", *depthSize, *stencilSize, debug_d3dformat(fmt));
    return TRUE;
}

#undef GLINFO_LOCATION

/* DirectDraw stuff */
WINED3DFORMAT pixelformat_for_depth(DWORD depth) {
    switch(depth) {
        case 8:  return WINED3DFMT_P8;
        case 15: return WINED3DFMT_X1R5G5B5;
        case 16: return WINED3DFMT_R5G6B5;
        case 24: return WINED3DFMT_R8G8B8;
        case 32: return WINED3DFMT_X8R8G8B8;
        default: return WINED3DFMT_UNKNOWN;
    }
}

void multiply_matrix(WINED3DMATRIX *dest, const WINED3DMATRIX *src1, const WINED3DMATRIX *src2) {
    WINED3DMATRIX temp;

    /* Now do the multiplication 'by hand'.
       I know that all this could be optimised, but this will be done later :-) */
    temp.u.s._11 = (src1->u.s._11 * src2->u.s._11) + (src1->u.s._21 * src2->u.s._12) + (src1->u.s._31 * src2->u.s._13) + (src1->u.s._41 * src2->u.s._14);
    temp.u.s._21 = (src1->u.s._11 * src2->u.s._21) + (src1->u.s._21 * src2->u.s._22) + (src1->u.s._31 * src2->u.s._23) + (src1->u.s._41 * src2->u.s._24);
    temp.u.s._31 = (src1->u.s._11 * src2->u.s._31) + (src1->u.s._21 * src2->u.s._32) + (src1->u.s._31 * src2->u.s._33) + (src1->u.s._41 * src2->u.s._34);
    temp.u.s._41 = (src1->u.s._11 * src2->u.s._41) + (src1->u.s._21 * src2->u.s._42) + (src1->u.s._31 * src2->u.s._43) + (src1->u.s._41 * src2->u.s._44);

    temp.u.s._12 = (src1->u.s._12 * src2->u.s._11) + (src1->u.s._22 * src2->u.s._12) + (src1->u.s._32 * src2->u.s._13) + (src1->u.s._42 * src2->u.s._14);
    temp.u.s._22 = (src1->u.s._12 * src2->u.s._21) + (src1->u.s._22 * src2->u.s._22) + (src1->u.s._32 * src2->u.s._23) + (src1->u.s._42 * src2->u.s._24);
    temp.u.s._32 = (src1->u.s._12 * src2->u.s._31) + (src1->u.s._22 * src2->u.s._32) + (src1->u.s._32 * src2->u.s._33) + (src1->u.s._42 * src2->u.s._34);
    temp.u.s._42 = (src1->u.s._12 * src2->u.s._41) + (src1->u.s._22 * src2->u.s._42) + (src1->u.s._32 * src2->u.s._43) + (src1->u.s._42 * src2->u.s._44);

    temp.u.s._13 = (src1->u.s._13 * src2->u.s._11) + (src1->u.s._23 * src2->u.s._12) + (src1->u.s._33 * src2->u.s._13) + (src1->u.s._43 * src2->u.s._14);
    temp.u.s._23 = (src1->u.s._13 * src2->u.s._21) + (src1->u.s._23 * src2->u.s._22) + (src1->u.s._33 * src2->u.s._23) + (src1->u.s._43 * src2->u.s._24);
    temp.u.s._33 = (src1->u.s._13 * src2->u.s._31) + (src1->u.s._23 * src2->u.s._32) + (src1->u.s._33 * src2->u.s._33) + (src1->u.s._43 * src2->u.s._34);
    temp.u.s._43 = (src1->u.s._13 * src2->u.s._41) + (src1->u.s._23 * src2->u.s._42) + (src1->u.s._33 * src2->u.s._43) + (src1->u.s._43 * src2->u.s._44);

    temp.u.s._14 = (src1->u.s._14 * src2->u.s._11) + (src1->u.s._24 * src2->u.s._12) + (src1->u.s._34 * src2->u.s._13) + (src1->u.s._44 * src2->u.s._14);
    temp.u.s._24 = (src1->u.s._14 * src2->u.s._21) + (src1->u.s._24 * src2->u.s._22) + (src1->u.s._34 * src2->u.s._23) + (src1->u.s._44 * src2->u.s._24);
    temp.u.s._34 = (src1->u.s._14 * src2->u.s._31) + (src1->u.s._24 * src2->u.s._32) + (src1->u.s._34 * src2->u.s._33) + (src1->u.s._44 * src2->u.s._34);
    temp.u.s._44 = (src1->u.s._14 * src2->u.s._41) + (src1->u.s._24 * src2->u.s._42) + (src1->u.s._34 * src2->u.s._43) + (src1->u.s._44 * src2->u.s._44);

    /* And copy the new matrix in the good storage.. */
    memcpy(dest, &temp, 16 * sizeof(float));
}

DWORD get_flexible_vertex_size(DWORD d3dvtVertexType) {
    DWORD size = 0;
    int i;
    int numTextures = (d3dvtVertexType & WINED3DFVF_TEXCOUNT_MASK) >> WINED3DFVF_TEXCOUNT_SHIFT;

    if (d3dvtVertexType & WINED3DFVF_NORMAL) size += 3 * sizeof(float);
    if (d3dvtVertexType & WINED3DFVF_DIFFUSE) size += sizeof(DWORD);
    if (d3dvtVertexType & WINED3DFVF_SPECULAR) size += sizeof(DWORD);
    if (d3dvtVertexType & WINED3DFVF_PSIZE) size += sizeof(DWORD);
    switch (d3dvtVertexType & WINED3DFVF_POSITION_MASK) {
        case WINED3DFVF_XYZ:    size += 3 * sizeof(float); break;
        case WINED3DFVF_XYZRHW: size += 4 * sizeof(float); break;
        case WINED3DFVF_XYZB1:  size += 4 * sizeof(float); break;
        case WINED3DFVF_XYZB2:  size += 5 * sizeof(float); break;
        case WINED3DFVF_XYZB3:  size += 6 * sizeof(float); break;
        case WINED3DFVF_XYZB4:  size += 7 * sizeof(float); break;
        case WINED3DFVF_XYZB5:  size += 8 * sizeof(float); break;
        default: ERR("Unexpected position mask\n");
    }
    for (i = 0; i < numTextures; i++) {
        size += GET_TEXCOORD_SIZE_FROM_FVF(d3dvtVertexType, i) * sizeof(float);
    }

    return size;
}

/***********************************************************************
 * CalculateTexRect
 *
 * Calculates the dimensions of the opengl texture used for blits.
 * Handled oversized opengl textures and updates the source rectangle
 * accordingly
 *
 * Params:
 *  This: Surface to operate on
 *  Rect: Requested rectangle
 *
 * Returns:
 *  TRUE if the texture part can be loaded,
 *  FALSE otherwise
 *
 *********************************************************************/
#define GLINFO_LOCATION This->resource.wineD3DDevice->adapter->gl_info

BOOL CalculateTexRect(IWineD3DSurfaceImpl *This, RECT *Rect, float glTexCoord[4]) {
    int x1 = Rect->left, x2 = Rect->right;
    int y1 = Rect->top, y2 = Rect->bottom;
    GLint maxSize = GL_LIMITS(texture_size);

    TRACE("(%p)->(%d,%d)-(%d,%d)\n", This,
          Rect->left, Rect->top, Rect->right, Rect->bottom);

    /* The sizes might be reversed */
    if(Rect->left > Rect->right) {
        x1 = Rect->right;
        x2 = Rect->left;
    }
    if(Rect->top > Rect->bottom) {
        y1 = Rect->bottom;
        y2 = Rect->top;
    }

    /* No oversized texture? This is easy */
    if(!(This->Flags & SFLAG_OVERSIZE)) {
        /* Which rect from the texture do I need? */
        glTexCoord[0] = (float) Rect->left / (float) This->pow2Width;
        glTexCoord[2] = (float) Rect->top / (float) This->pow2Height;
        glTexCoord[1] = (float) Rect->right / (float) This->pow2Width;
        glTexCoord[3] = (float) Rect->bottom / (float) This->pow2Height;

        return TRUE;
    } else {
        /* Check if we can succeed at all */
        if( (x2 - x1) > maxSize ||
            (y2 - y1) > maxSize ) {
            TRACE("Requested rectangle is too large for gl\n");
            return FALSE;
        }

        /* A part of the texture has to be picked. First, check if
         * some texture part is loaded already, if yes try to re-use it.
         * If the texture is dirty, or the part can't be used,
         * re-position the part to load
         */
        if(This->Flags & SFLAG_INTEXTURE) {
            if(This->glRect.left <= x1 && This->glRect.right >= x2 &&
               This->glRect.top <= y1 && This->glRect.bottom >= x2 ) {
                /* Ok, the rectangle is ok, re-use it */
                TRACE("Using existing gl Texture\n");
            } else {
                /* Rectangle is not ok, dirtify the texture to reload it */
                TRACE("Dirtifying texture to force reload\n");
                This->Flags &= ~SFLAG_INTEXTURE;
            }
        }

        /* Now if we are dirty(no else if!) */
        if(!(This->Flags & SFLAG_INTEXTURE)) {
            /* Set the new rectangle. Use the following strategy:
             * 1) Use as big textures as possible.
             * 2) Place the texture part in the way that the requested
             *    part is in the middle of the texture(well, almost)
             * 3) If the texture is moved over the edges of the
             *    surface, replace it nicely
             * 4) If the coord is not limiting the texture size,
             *    use the whole size
             */
            if((This->pow2Width) > maxSize) {
                This->glRect.left = x1 - maxSize / 2;
                if(This->glRect.left < 0) {
                    This->glRect.left = 0;
                }
                This->glRect.right = This->glRect.left + maxSize;
                if(This->glRect.right > This->currentDesc.Width) {
                    This->glRect.right = This->currentDesc.Width;
                    This->glRect.left = This->glRect.right - maxSize;
                }
            } else {
                This->glRect.left = 0;
                This->glRect.right = This->pow2Width;
            }

            if(This->pow2Height > maxSize) {
                This->glRect.top = x1 - GL_LIMITS(texture_size) / 2;
                if(This->glRect.top < 0) This->glRect.top = 0;
                This->glRect.bottom = This->glRect.left + maxSize;
                if(This->glRect.bottom > This->currentDesc.Height) {
                    This->glRect.bottom = This->currentDesc.Height;
                    This->glRect.top = This->glRect.bottom - maxSize;
                }
            } else {
                This->glRect.top = 0;
                This->glRect.bottom = This->pow2Height;
            }
            TRACE("(%p): Using rect (%d,%d)-(%d,%d)\n", This,
                   This->glRect.left, This->glRect.top, This->glRect.right, This->glRect.bottom);
        }

        /* Re-calculate the rect to draw */
        Rect->left -= This->glRect.left;
        Rect->right -= This->glRect.left;
        Rect->top -= This->glRect.top;
        Rect->bottom -= This->glRect.top;

        /* Get the gl coordinates. The gl rectangle is a power of 2, eigher the max size,
         * or the pow2Width / pow2Height of the surface
         */
        glTexCoord[0] = (float) Rect->left / (float) (This->glRect.right - This->glRect.left);
        glTexCoord[2] = (float) Rect->top / (float) (This->glRect.bottom - This->glRect.top);
        glTexCoord[1] = (float) Rect->right / (float) (This->glRect.right - This->glRect.left);
        glTexCoord[3] = (float) Rect->bottom / (float) (This->glRect.bottom - This->glRect.top);
    }
    return TRUE;
}
#undef GLINFO_LOCATION

/* Hash table functions */

hash_table_t *hash_table_create(hash_function_t *hash_function, compare_function_t *compare_function)
{
    hash_table_t *table;
    unsigned int initial_size = 8;

    table = HeapAlloc(GetProcessHeap(), 0, sizeof(hash_table_t) + (initial_size * sizeof(struct list)));
    if (!table)
    {
        ERR("Failed to allocate table, returning NULL.\n");
        return NULL;
    }

    table->hash_function = hash_function;
    table->compare_function = compare_function;

    table->grow_size = initial_size - (initial_size >> 2);
    table->shrink_size = 0;

    table->buckets = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, initial_size * sizeof(struct list));
    if (!table->buckets)
    {
        ERR("Failed to allocate table buckets, returning NULL.\n");
        HeapFree(GetProcessHeap(), 0, table);
        return NULL;
    }
    table->bucket_count = initial_size;

    table->entries = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, table->grow_size * sizeof(hash_table_entry_t));
    if (!table->entries)
    {
        ERR("Failed to allocate table entries, returning NULL.\n");
        HeapFree(GetProcessHeap(), 0, table->buckets);
        HeapFree(GetProcessHeap(), 0, table);
        return NULL;
    }
    table->entry_count = 0;

    list_init(&table->free_entries);
    table->count = 0;

    return table;
}

void hash_table_destroy(hash_table_t *table)
{
    unsigned int i = 0;

    for (i = 0; i < table->entry_count; ++i)
    {
        HeapFree(GetProcessHeap(), 0, table->entries[i].key);
    }

    HeapFree(GetProcessHeap(), 0, table->entries);
    HeapFree(GetProcessHeap(), 0, table->buckets);
    HeapFree(GetProcessHeap(), 0, table);
}

static inline hash_table_entry_t *hash_table_get_by_idx(hash_table_t *table, void *key, unsigned int idx)
{
    hash_table_entry_t *entry;

    if (table->buckets[idx].next)
        LIST_FOR_EACH_ENTRY(entry, &(table->buckets[idx]), hash_table_entry_t, entry)
            if (table->compare_function(entry->key, key)) return entry;

    return NULL;
}

static BOOL hash_table_resize(hash_table_t *table, unsigned int new_bucket_count)
{
    unsigned int new_entry_count = 0;
    hash_table_entry_t *new_entries;
    struct list *new_buckets;
    unsigned int grow_size = new_bucket_count - (new_bucket_count >> 2);
    unsigned int i;

    new_buckets = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, new_bucket_count * sizeof(struct list));
    if (!new_buckets)
    {
        ERR("Failed to allocate new buckets, returning FALSE.\n");
        return FALSE;
    }

    new_entries = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, grow_size * sizeof(hash_table_entry_t));
    if (!new_entries)
    {
        ERR("Failed to allocate new entries, returning FALSE.\n");
        HeapFree(GetProcessHeap(), 0, new_buckets);
        return FALSE;
    }

    for (i = 0; i < table->bucket_count; ++i)
    {
        if (table->buckets[i].next)
        {
            hash_table_entry_t *entry, *entry2;

            LIST_FOR_EACH_ENTRY_SAFE(entry, entry2, &table->buckets[i], hash_table_entry_t, entry)
            {
                int j;
                hash_table_entry_t *new_entry = new_entries + (new_entry_count++);
                *new_entry = *entry;

                j = new_entry->hash & (new_bucket_count - 1);

                if (!new_buckets[j].next) list_init(&new_buckets[j]);
                list_add_head(&new_buckets[j], &new_entry->entry);
            }
        }
    }

    HeapFree(GetProcessHeap(), 0, table->buckets);
    table->buckets = new_buckets;

    HeapFree(GetProcessHeap(), 0, table->entries);
    table->entries = new_entries;

    table->entry_count = new_entry_count;
    list_init(&table->free_entries);

    table->bucket_count = new_bucket_count;
    table->grow_size = grow_size;
    table->shrink_size = new_bucket_count > 8 ? new_bucket_count >> 2 : 0;

    return TRUE;
}

void hash_table_put(hash_table_t *table, void *key, void *value)
{
    unsigned int idx;
    unsigned int hash;
    hash_table_entry_t *entry;

    hash = table->hash_function(key);
    idx = hash & (table->bucket_count - 1);
    entry = hash_table_get_by_idx(table, key, idx);

    if (entry)
    {
        HeapFree(GetProcessHeap(), 0, key);
        entry->value = value;

        if (!value)
        {
            HeapFree(GetProcessHeap(), 0, entry->key);
            entry->key = NULL;

            /* Remove the entry */
            list_remove(&entry->entry);
            list_add_head(&table->free_entries, &entry->entry);

            --table->count;

            /* Shrink if necessary */
            if (table->count < table->shrink_size) {
                if (!hash_table_resize(table, table->bucket_count >> 1))
                {
                    ERR("Failed to shrink the table...\n");
                }
            }
        }

        return;
    }

    if (!value) return;

    /* Grow if necessary */
    if (table->count >= table->grow_size)
    {
        if (!hash_table_resize(table, table->bucket_count << 1))
        {
            ERR("Failed to grow the table, returning.\n");
            return;
        }

        idx = hash & (table->bucket_count - 1);
    }

    /* Find an entry to insert */
    if (!list_empty(&table->free_entries))
    {
        struct list *elem = list_head(&table->free_entries);

        list_remove(elem);
        entry = LIST_ENTRY(elem, hash_table_entry_t, entry);
    } else {
        entry = table->entries + (table->entry_count++);
    }

    /* Insert the entry */
    entry->key = key;
    entry->value = value;
    entry->hash = hash;
    if (!table->buckets[idx].next) list_init(&table->buckets[idx]);
    list_add_head(&table->buckets[idx], &entry->entry);

    ++table->count;
}

void hash_table_remove(hash_table_t *table, void *key)
{
    hash_table_put(table, key, NULL);
}

void *hash_table_get(hash_table_t *table, void *key)
{
    unsigned int idx;
    hash_table_entry_t *entry;

    idx = table->hash_function(key) & (table->bucket_count - 1);
    entry = hash_table_get_by_idx(table, key, idx);

    return entry ? entry->value : NULL;
}

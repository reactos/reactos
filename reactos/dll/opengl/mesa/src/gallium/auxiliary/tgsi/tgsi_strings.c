/**************************************************************************
 * 
 * Copyright 2007-2008 Tungsten Graphics, Inc., Cedar Park, Texas.
 * Copyright 2012 VMware, Inc.
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/


#include "pipe/p_compiler.h"
#include "util/u_memory.h"
#include "tgsi_strings.h"


const char *tgsi_processor_type_names[3] =
{
   "FRAG",
   "VERT",
   "GEOM"
};

const char *tgsi_file_names[TGSI_FILE_COUNT] =
{
   "NULL",
   "CONST",
   "IN",
   "OUT",
   "TEMP",
   "SAMP",
   "ADDR",
   "IMM",
   "PRED",
   "SV",
   "IMMX",
   "TEMPX",
   "RES"
};

const char *tgsi_semantic_names[TGSI_SEMANTIC_COUNT] =
{
   "POSITION",
   "COLOR",
   "BCOLOR",
   "FOG",
   "PSIZE",
   "GENERIC",
   "NORMAL",
   "FACE",
   "EDGEFLAG",
   "PRIM_ID",
   "INSTANCEID",
   "VERTEXID",
   "STENCIL",
   "CLIPDIST",
   "CLIPVERTEX"
};

const char *tgsi_texture_names[TGSI_TEXTURE_COUNT] =
{
   "UNKNOWN",
   "1D",
   "2D",
   "3D",
   "CUBE",
   "RECT",
   "SHADOW1D",
   "SHADOW2D",
   "SHADOWRECT",
   "1DARRAY",
   "2DARRAY",
   "SHADOW1DARRAY",
   "SHADOW2DARRAY",
   "SHADOWCUBE"
};

const char *tgsi_property_names[TGSI_PROPERTY_COUNT] =
{
   "GS_INPUT_PRIMITIVE",
   "GS_OUTPUT_PRIMITIVE",
   "GS_MAX_OUTPUT_VERTICES",
   "FS_COORD_ORIGIN",
   "FS_COORD_PIXEL_CENTER",
   "FS_COLOR0_WRITES_ALL_CBUFS",
   "FS_DEPTH_LAYOUT",
   "VS_PROHIBIT_UCPS"
};

const char *tgsi_type_names[5] =
{
   "UNORM",
   "SNORM",
   "SINT",
   "UINT",
   "FLOAT"
};

const char *tgsi_interpolate_names[TGSI_INTERPOLATE_COUNT] =
{
   "CONSTANT",
   "LINEAR",
   "PERSPECTIVE",
   "COLOR"
};

const char *tgsi_primitive_names[PIPE_PRIM_MAX] =
{
   "POINTS",
   "LINES",
   "LINE_LOOP",
   "LINE_STRIP",
   "TRIANGLES",
   "TRIANGLE_STRIP",
   "TRIANGLE_FAN",
   "QUADS",
   "QUAD_STRIP",
   "POLYGON",
   "LINES_ADJACENCY",
   "LINE_STRIP_ADJACENCY",
   "TRIANGLES_ADJACENCY",
   "TRIANGLE_STRIP_ADJACENCY"
};

const char *tgsi_fs_coord_origin_names[2] =
{
   "UPPER_LEFT",
   "LOWER_LEFT"
};

const char *tgsi_fs_coord_pixel_center_names[2] =
{
   "HALF_INTEGER",
   "INTEGER"
};

const char *tgsi_immediate_type_names[3] =
{
   "FLT32",
   "UINT32",
   "INT32"
};


static INLINE void
tgsi_strings_check(void)
{
   STATIC_ASSERT(Elements(tgsi_file_names) == TGSI_FILE_COUNT);
   STATIC_ASSERT(Elements(tgsi_semantic_names) == TGSI_SEMANTIC_COUNT);
   STATIC_ASSERT(Elements(tgsi_texture_names) == TGSI_TEXTURE_COUNT);
   STATIC_ASSERT(Elements(tgsi_property_names) == TGSI_PROPERTY_COUNT);
   STATIC_ASSERT(Elements(tgsi_primitive_names) == PIPE_PRIM_MAX);
   STATIC_ASSERT(Elements(tgsi_interpolate_names) == TGSI_INTERPOLATE_COUNT);
   (void) tgsi_processor_type_names;
   (void) tgsi_type_names;
   (void) tgsi_immediate_type_names;
   (void) tgsi_fs_coord_origin_names;
   (void) tgsi_fs_coord_pixel_center_names;
}

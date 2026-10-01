/*
 * Copyright 2021 Nikolay Sivov for CodeWeavers
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

#include <stdarg.h>
#include "windef.h"
#include "winternl.h"
#include "dwrite.h"
#include "dwrite_private.h"
#include "wine/unixlib.h"

struct create_font_object_params
{
    const void *data;
    UINT64 size;
    unsigned int index;
    UINT64 *object;
};

struct release_font_object_params
{
    UINT64 object;
};

struct get_glyph_outline_params
{
    UINT64 object;
    unsigned int simulations;
    unsigned int glyph;
    float emsize;
    struct dwrite_outline *outline;
};

struct get_glyph_count_params
{
    UINT64 object;
    unsigned int *count;
};

struct get_glyph_advance_params
{
    UINT64 object;
    unsigned int glyph;
    unsigned int mode;
    float emsize;
    int *advance;
    unsigned int *has_contours;
};

struct get_glyph_bbox_params
{
    UINT64 object;
    unsigned int simulations;
    unsigned int glyph;
    float emsize;
    MATRIX_2X2 m;
    RECT *bbox;
};

struct get_glyph_bitmap_params
{
    UINT64 object;
    unsigned int simulations;
    unsigned int glyph;
    unsigned int mode;
    float emsize;
    MATRIX_2X2 m;
    RECT bbox;
    int pitch;
    BYTE *bitmap;
    unsigned int *is_1bpp;
};

struct get_design_glyph_metrics_params
{
    UINT64 object;
    unsigned int simulations;
    unsigned int glyph;
    unsigned int upem;
    unsigned int ascent;
    DWRITE_GLYPH_METRICS *metrics;
};

enum font_backend_funcs
{
    unix_process_attach,
    unix_process_detach,
    unix_create_font_object,
    unix_release_font_object,
    unix_get_glyph_outline,
    unix_get_glyph_count,
    unix_get_glyph_advance,
    unix_get_glyph_bbox,
    unix_get_glyph_bitmap,
    unix_get_design_glyph_metrics,
    unix_funcs_count,
};

#define UNIX_CALL( func, params ) WINE_UNIX_CALL( unix_ ## func, params )

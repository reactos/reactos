/*
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS
 * FILE:                 dll/opengl/opengl32/swimpl.c
 * PURPOSE:              OpenGL32 DLL, opengl software implementation
 */

#include "opengl32.h"

/* MESA includes */
#include <context.h>
#include <matrix.h>

WINE_DEFAULT_DEBUG_CHANNEL(opengl32);

#define WIDTH_BYTES_ALIGN32(cx, bpp) ((((cx) * (bpp) + 31) & ~31) >> 3)
#define WIDTH_BYTES_ALIGN16(cx, bpp) ((((cx) * (bpp) + 15) & ~15) >> 3)

/* Flags for our pixel formats */
#define SB_FLAGS            (PFD_DRAW_TO_BITMAP | PFD_SUPPORT_GDI | PFD_SUPPORT_OPENGL | PFD_GENERIC_FORMAT)
#define SB_FLAGS_WINDOW     (PFD_DRAW_TO_BITMAP | PFD_SUPPORT_GDI | PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_GENERIC_FORMAT)
#define SB_FLAGS_PALETTE    (PFD_DRAW_TO_BITMAP | PFD_SUPPORT_GDI | PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_GENERIC_FORMAT | PFD_NEED_PALETTE)
#define DB_FLAGS            (PFD_DOUBLEBUFFER   | PFD_SWAP_COPY   | PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_GENERIC_FORMAT)
#define DB_FLAGS_PALETTE    (PFD_DOUBLEBUFFER   | PFD_SWAP_COPY   | PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_GENERIC_FORMAT | PFD_NEED_PALETTE)


struct pixel_format
{
    DWORD dwFlags;
    BYTE iPixelType;
    BYTE cColorBits;
    BYTE cRedBits; BYTE cRedShift;
    BYTE cGreenBits; BYTE cGreenShift;
    BYTE cBlueBits; BYTE cBlueShift;
    BYTE cAlphaBits; BYTE cAlphaShift;
    BYTE cAccumBits;
    BYTE cAccumRedBits; BYTE cAccumGreenBits; BYTE cAccumBlueBits; BYTE cAccumAlphaBits;
    BYTE cDepthBits;
};

static const struct pixel_format pixel_formats_32[] =
{
    /* 32bpp */
    {SB_FLAGS_WINDOW,  PFD_TYPE_RGBA,       32, 8, 16, 8, 8, 8, 0, 0, 0, 64, 16, 16, 16, 0,  32},
    {SB_FLAGS_WINDOW,  PFD_TYPE_RGBA,       32, 8, 16, 8, 8, 8, 0, 0, 0, 64, 16, 16, 16, 0,  16},
    {DB_FLAGS,         PFD_TYPE_RGBA,       32, 8, 16, 8, 8, 8, 0, 0, 0, 64, 16, 16, 16, 0,  32},
    {DB_FLAGS,         PFD_TYPE_RGBA,       32, 8, 16, 8, 8, 8, 0, 0, 0, 64, 16, 16, 16, 0,  16},
    {SB_FLAGS_WINDOW,  PFD_TYPE_RGBA,       32, 8, 16, 8, 8, 8, 0, 8, 0, 64, 16, 16, 16, 16, 32},
    {SB_FLAGS_WINDOW,  PFD_TYPE_RGBA,       32, 8, 16, 8, 8, 8, 0, 8, 0, 64, 16, 16, 16, 16, 16},
    {DB_FLAGS,         PFD_TYPE_RGBA,       32, 8, 16, 8, 8, 8, 0, 8, 0, 64, 16, 16, 16, 16, 32},
    {DB_FLAGS,         PFD_TYPE_RGBA,       32, 8, 16, 8, 8, 8, 0, 8, 0, 64, 16, 16, 16, 16, 16},
    {SB_FLAGS_WINDOW,  PFD_TYPE_COLORINDEX, 32, 8, 16, 8, 8, 8, 0, 0, 0, 0,  0,  0,  0,  0,  32},
    {SB_FLAGS_WINDOW,  PFD_TYPE_COLORINDEX, 32, 8, 16, 8, 8, 8, 0, 0, 0, 0,  0,  0,  0,  0,  16},
    {DB_FLAGS,         PFD_TYPE_COLORINDEX, 32, 8, 16, 8, 8, 8, 0, 0, 0, 0,  0,  0,  0,  0,  32},
    {DB_FLAGS,         PFD_TYPE_COLORINDEX, 32, 8, 16, 8, 8, 8, 0, 0, 0, 0,  0,  0,  0,  0,  16},
    /* 24bpp */
    {SB_FLAGS,         PFD_TYPE_RGBA,       24, 8, 16, 8, 8, 8, 0, 0, 0, 64, 16, 16, 16, 0,  32},
    {SB_FLAGS,         PFD_TYPE_RGBA,       24, 8, 16, 8, 8, 8, 0, 0, 0, 64, 16, 16, 16, 0,  16},
    {SB_FLAGS,         PFD_TYPE_RGBA,       24, 8, 16, 8, 8, 8, 0, 8, 0, 64, 16, 16, 16, 16, 32},
    {SB_FLAGS,         PFD_TYPE_RGBA,       24, 8, 16, 8, 8, 8, 0, 8, 0, 64, 16, 16, 16, 16, 16},
    {SB_FLAGS,         PFD_TYPE_COLORINDEX, 24, 8, 16, 8, 8, 8, 0, 0, 0, 0,  0,  0,  0,  0,  32},
    {SB_FLAGS,         PFD_TYPE_COLORINDEX, 24, 8, 16, 8, 8, 8, 0, 0, 0, 0,  0,  0,  0,  0,  16},
    /* 16 bpp */
    {SB_FLAGS,         PFD_TYPE_RGBA,       16, 5, 10, 5, 5, 5, 0, 0, 0, 32, 11, 11, 10, 0,  32},
    {SB_FLAGS,         PFD_TYPE_RGBA,       16, 5, 10, 5, 5, 5, 0, 0, 0, 32, 11, 11, 10, 0,  16},
    {SB_FLAGS,         PFD_TYPE_RGBA,       16, 5, 10, 5, 5, 5, 0, 8, 0, 32, 8,  8,  8,  8,  32},
    {SB_FLAGS,         PFD_TYPE_RGBA,       16, 5, 10, 5, 5, 5, 0, 8, 0, 32, 8,  8,  8,  8,  16},
    {SB_FLAGS,         PFD_TYPE_COLORINDEX, 16, 5, 10, 5, 5, 5, 0, 0, 0, 0,  0,  0,  0,  0,  32},
    {SB_FLAGS,         PFD_TYPE_COLORINDEX, 16, 5, 10, 5, 5, 5, 0, 0, 0, 0,  0,  0,  0,  0,  16},
    /* 8bpp */
    {SB_FLAGS,         PFD_TYPE_RGBA,       8,  3, 0,  3, 3, 2, 6, 0, 0, 32, 11, 11, 10, 0,  32},
    {SB_FLAGS,         PFD_TYPE_RGBA,       8,  3, 0,  3, 3, 2, 6, 0, 0, 32, 11, 11, 10, 0,  16},
    {SB_FLAGS,         PFD_TYPE_RGBA,       8,  3, 0,  3, 3, 2, 6, 8, 0, 32, 8,  8,  8,  8,  32},
    {SB_FLAGS,         PFD_TYPE_RGBA,       8,  3, 0,  3, 3, 2, 6, 8, 0, 32, 8,  8,  8,  8,  16},
    {SB_FLAGS,         PFD_TYPE_COLORINDEX, 8,  3, 0,  3, 3, 2, 6, 0, 0, 0,  0,  0,  0,  0,  32},
    {SB_FLAGS,         PFD_TYPE_COLORINDEX, 8,  3, 0,  3, 3, 2, 6, 0, 0, 0,  0,  0,  0,  0,  16},
    /* 4bpp */
    {SB_FLAGS,         PFD_TYPE_RGBA,       4,  1, 0,  1, 1, 1, 2, 0, 0, 16, 5,  6,  5,  0,  32},
    {SB_FLAGS,         PFD_TYPE_RGBA,       4,  1, 0,  1, 1, 1, 2, 0, 0, 16, 5,  6,  5,  0,  16},
    {SB_FLAGS,         PFD_TYPE_RGBA,       4,  1, 0,  1, 1, 1, 2, 8, 0, 16, 4,  4,  4,  4,  32},
    {SB_FLAGS,         PFD_TYPE_RGBA,       4,  1, 0,  1, 1, 1, 2, 8, 0, 16, 4,  4,  4,  4,  16},
    {SB_FLAGS,         PFD_TYPE_COLORINDEX, 4,  1, 0,  1, 1, 1, 2, 0, 0, 0,  0,  0,  0,  0,  32},
    {SB_FLAGS,         PFD_TYPE_COLORINDEX, 4,  1, 0,  1, 1, 1, 2, 0, 0, 0,  0,  0,  0,  0,  16},
};

static const struct pixel_format pixel_formats_24[] =
{
    /* 24bpp */
    {SB_FLAGS_WINDOW,  PFD_TYPE_RGBA,       24, 8, 16, 8, 8, 8, 0, 0, 0, 64, 16, 16, 16, 0,  32},
    {SB_FLAGS_WINDOW,  PFD_TYPE_RGBA,       24, 8, 16, 8, 8, 8, 0, 0, 0, 64, 16, 16, 16, 0,  16},
    {DB_FLAGS,         PFD_TYPE_RGBA,       24, 8, 16, 8, 8, 8, 0, 0, 0, 64, 16, 16, 16, 0,  32},
    {DB_FLAGS,         PFD_TYPE_RGBA,       24, 8, 16, 8, 8, 8, 0, 0, 0, 64, 16, 16, 16, 0,  16},
    {SB_FLAGS_WINDOW,  PFD_TYPE_RGBA,       24, 8, 16, 8, 8, 8, 0, 8, 0, 64, 16, 16, 16, 16, 32},
    {SB_FLAGS_WINDOW,  PFD_TYPE_RGBA,       24, 8, 16, 8, 8, 8, 0, 8, 0, 64, 16, 16, 16, 16, 16},
    {DB_FLAGS,         PFD_TYPE_RGBA,       24, 8, 16, 8, 8, 8, 0, 8, 0, 64, 16, 16, 16, 16, 32},
    {DB_FLAGS,         PFD_TYPE_RGBA,       24, 8, 16, 8, 8, 8, 0, 8, 0, 64, 16, 16, 16, 16, 16},
    {SB_FLAGS_WINDOW,  PFD_TYPE_COLORINDEX, 24, 8, 16, 8, 8, 8, 0, 0, 0, 0,  0,  0,  0,  0,  32},
    {SB_FLAGS_WINDOW,  PFD_TYPE_COLORINDEX, 24, 8, 16, 8, 8, 8, 0, 0, 0, 0,  0,  0,  0,  0,  16},
    {DB_FLAGS,         PFD_TYPE_COLORINDEX, 24, 8, 16, 8, 8, 8, 0, 0, 0, 0,  0,  0,  0,  0,  32},
    {DB_FLAGS,         PFD_TYPE_COLORINDEX, 24, 8, 16, 8, 8, 8, 0, 0, 0, 0,  0,  0,  0,  0,  16},
    /* 32bpp */
    {SB_FLAGS,         PFD_TYPE_RGBA,       32, 8, 16, 8, 8, 8, 0, 0, 0, 64, 16, 16, 16, 0,  32},
    {SB_FLAGS,         PFD_TYPE_RGBA,       32, 8, 16, 8, 8, 8, 0, 0, 0, 64, 16, 16, 16, 0,  16},
    {SB_FLAGS,         PFD_TYPE_RGBA,       32, 8, 16, 8, 8, 8, 0, 8, 0, 64, 16, 16, 16, 16, 32},
    {SB_FLAGS,         PFD_TYPE_RGBA,       32, 8, 16, 8, 8, 8, 0, 8, 0, 64, 16, 16, 16, 16, 16},
    {SB_FLAGS,         PFD_TYPE_COLORINDEX, 32, 8, 16, 8, 8, 8, 0, 0, 0, 0,  0,  0,  0,  0,  32},
    {SB_FLAGS,         PFD_TYPE_COLORINDEX, 32, 8, 16, 8, 8, 8, 0, 0, 0, 0,  0,  0,  0,  0,  16},
    /* 16 bpp */
    {SB_FLAGS,         PFD_TYPE_RGBA,       16, 5, 10, 5, 5, 5, 0, 0, 0, 32, 11, 11, 10, 0,  32},
    {SB_FLAGS,         PFD_TYPE_RGBA,       16, 5, 10, 5, 5, 5, 0, 0, 0, 32, 11, 11, 10, 0,  16},
    {SB_FLAGS,         PFD_TYPE_RGBA,       16, 5, 10, 5, 5, 5, 0, 8, 0, 32, 8,  8,  8,  8,  32},
    {SB_FLAGS,         PFD_TYPE_RGBA,       16, 5, 10, 5, 5, 5, 0, 8, 0, 32, 8,  8,  8,  8,  16},
    {SB_FLAGS,         PFD_TYPE_COLORINDEX, 16, 5, 10, 5, 5, 5, 0, 0, 0, 0,  0,  0,  0,  0,  32},
    {SB_FLAGS,         PFD_TYPE_COLORINDEX, 16, 5, 10, 5, 5, 5, 0, 0, 0, 0,  0,  0,  0,  0,  16},
    /* 8bpp */
    {SB_FLAGS,         PFD_TYPE_RGBA,       8,  3, 0,  3, 3, 2, 6, 0, 0, 32, 11, 11, 10, 0,  32},
    {SB_FLAGS,         PFD_TYPE_RGBA,       8,  3, 0,  3, 3, 2, 6, 0, 0, 32, 11, 11, 10, 0,  16},
    {SB_FLAGS,         PFD_TYPE_RGBA,       8,  3, 0,  3, 3, 2, 6, 8, 0, 32, 8,  8,  8,  8,  32},
    {SB_FLAGS,         PFD_TYPE_RGBA,       8,  3, 0,  3, 3, 2, 6, 8, 0, 32, 8,  8,  8,  8,  16},
    {SB_FLAGS,         PFD_TYPE_COLORINDEX, 8,  3, 0,  3, 3, 2, 6, 0, 0, 0,  0,  0,  0,  0,  32},
    {SB_FLAGS,         PFD_TYPE_COLORINDEX, 8,  3, 0,  3, 3, 2, 6, 0, 0, 0,  0,  0,  0,  0,  16},
    /* 4bpp */
    {SB_FLAGS,         PFD_TYPE_RGBA,       4,  1, 0,  1, 1, 1, 2, 0, 0, 16, 5,  6,  5,  0,  32},
    {SB_FLAGS,         PFD_TYPE_RGBA,       4,  1, 0,  1, 1, 1, 2, 0, 0, 16, 5,  6,  5,  0,  16},
    {SB_FLAGS,         PFD_TYPE_RGBA,       4,  1, 0,  1, 1, 1, 2, 8, 0, 16, 4,  4,  4,  4,  32},
    {SB_FLAGS,         PFD_TYPE_RGBA,       4,  1, 0,  1, 1, 1, 2, 8, 0, 16, 4,  4,  4,  4,  16},
    {SB_FLAGS,         PFD_TYPE_COLORINDEX, 4,  1, 0,  1, 1, 1, 2, 0, 0, 0,  0,  0,  0,  0,  32},
    {SB_FLAGS,         PFD_TYPE_COLORINDEX, 4,  1, 0,  1, 1, 1, 2, 0, 0, 0,  0,  0,  0,  0,  16},
};

static const struct pixel_format pixel_formats_16[] =
{
    /* 16 bpp - 565 */
    {SB_FLAGS_WINDOW,  PFD_TYPE_RGBA,       16, 5, 11, 6, 5, 5, 0, 0, 0, 32, 11, 11, 10, 0,  32},
    {SB_FLAGS_WINDOW,  PFD_TYPE_RGBA,       16, 5, 11, 6, 5, 5, 0, 0, 0, 32, 11, 11, 10, 0,  16},
    {DB_FLAGS,         PFD_TYPE_RGBA,       16, 5, 11, 6, 5, 5, 0, 0, 0, 32, 11, 11, 10, 0,  32},
    {DB_FLAGS,         PFD_TYPE_RGBA,       16, 5, 11, 6, 5, 5, 0, 0, 0, 32, 11, 11, 10, 0,  16},
    {SB_FLAGS_WINDOW,  PFD_TYPE_RGBA,       16, 5, 11, 6, 5, 5, 0, 8, 0, 32, 8,  8,  8,  8,  32},
    {SB_FLAGS_WINDOW,  PFD_TYPE_RGBA,       16, 5, 11, 6, 5, 5, 0, 8, 0, 32, 8,  8,  8,  8,  16},
    {DB_FLAGS,         PFD_TYPE_RGBA,       16, 5, 11, 6, 5, 5, 0, 8, 0, 32, 8,  8,  8,  8,  32},
    {DB_FLAGS,         PFD_TYPE_RGBA,       16, 5, 11, 6, 5, 5, 0, 8, 0, 32, 8,  8,  8,  8,  16},
    {SB_FLAGS_WINDOW,  PFD_TYPE_COLORINDEX, 16, 5, 11, 6, 5, 5, 0, 0, 0, 0,  0,  0,  0,  0,  32},
    {SB_FLAGS_WINDOW,  PFD_TYPE_COLORINDEX, 16, 5, 11, 6, 5, 5, 0, 0, 0, 0,  0,  0,  0,  0,  16},
    {DB_FLAGS,         PFD_TYPE_COLORINDEX, 16, 5, 11, 6, 5, 5, 0, 0, 0, 0,  0,  0,  0,  0,  32},
    {DB_FLAGS,         PFD_TYPE_COLORINDEX, 16, 5, 11, 6, 5, 5, 0, 0, 0, 0,  0,  0,  0,  0,  16},
    /* 24bpp */
    {SB_FLAGS,         PFD_TYPE_RGBA,       24, 8, 16, 8, 8, 8, 0, 0, 0, 64, 16, 16, 16, 0,  32},
    {SB_FLAGS,         PFD_TYPE_RGBA,       24, 8, 16, 8, 8, 8, 0, 0, 0, 64, 16, 16, 16, 0,  16},
    {SB_FLAGS,         PFD_TYPE_RGBA,       24, 8, 16, 8, 8, 8, 0, 8, 0, 64, 16, 16, 16, 16, 32},
    {SB_FLAGS,         PFD_TYPE_RGBA,       24, 8, 16, 8, 8, 8, 0, 8, 0, 64, 16, 16, 16, 16, 16},
    {SB_FLAGS,         PFD_TYPE_COLORINDEX, 24, 8, 16, 8, 8, 8, 0, 0, 0, 0,  0,  0,  0,  0,  32},
    {SB_FLAGS,         PFD_TYPE_COLORINDEX, 24, 8, 16, 8, 8, 8, 0, 0, 0, 0,  0,  0,  0,  0,  16},
    /* 32bpp */
    {SB_FLAGS,         PFD_TYPE_RGBA,       32, 8, 16, 8, 8, 8, 0, 0, 0, 64, 16, 16, 16, 0,  32},
    {SB_FLAGS,         PFD_TYPE_RGBA,       32, 8, 16, 8, 8, 8, 0, 0, 0, 64, 16, 16, 16, 0,  16},
    {SB_FLAGS,         PFD_TYPE_RGBA,       32, 8, 16, 8, 8, 8, 0, 8, 0, 64, 16, 16, 16, 16, 32},
    {SB_FLAGS,         PFD_TYPE_RGBA,       32, 8, 16, 8, 8, 8, 0, 8, 0, 64, 16, 16, 16, 16, 16},
    {SB_FLAGS,         PFD_TYPE_COLORINDEX, 32, 8, 16, 8, 8, 8, 0, 0, 0, 0,  0,  0,  0,  0,  32},
    {SB_FLAGS,         PFD_TYPE_COLORINDEX, 32, 8, 16, 8, 8, 8, 0, 0, 0, 0,  0,  0,  0,  0,  16},
    /* 8bpp */
    {SB_FLAGS,         PFD_TYPE_RGBA,       8,  3, 0,  3, 3, 2, 6, 0, 0, 32, 11, 11, 10, 0,  32},
    {SB_FLAGS,         PFD_TYPE_RGBA,       8,  3, 0,  3, 3, 2, 6, 0, 0, 32, 11, 11, 10, 0,  16},
    {SB_FLAGS,         PFD_TYPE_RGBA,       8,  3, 0,  3, 3, 2, 6, 8, 0, 32, 8,  8,  8,  8,  32},
    {SB_FLAGS,         PFD_TYPE_RGBA,       8,  3, 0,  3, 3, 2, 6, 8, 0, 32, 8,  8,  8,  8,  16},
    {SB_FLAGS,         PFD_TYPE_COLORINDEX, 8,  3, 0,  3, 3, 2, 6, 0, 0, 0,  0,  0,  0,  0,  32},
    {SB_FLAGS,         PFD_TYPE_COLORINDEX, 8,  3, 0,  3, 3, 2, 6, 0, 0, 0,  0,  0,  0,  0,  16},
    /* 4bpp */
    {SB_FLAGS,         PFD_TYPE_RGBA,       4,  1, 0,  1, 1, 1, 2, 0, 0, 16, 5,  6,  5,  0,  32},
    {SB_FLAGS,         PFD_TYPE_RGBA,       4,  1, 0,  1, 1, 1, 2, 0, 0, 16, 5,  6,  5,  0,  16},
    {SB_FLAGS,         PFD_TYPE_RGBA,       4,  1, 0,  1, 1, 1, 2, 8, 0, 16, 4,  4,  4,  4,  32},
    {SB_FLAGS,         PFD_TYPE_RGBA,       4,  1, 0,  1, 1, 1, 2, 8, 0, 16, 4,  4,  4,  4,  16},
    {SB_FLAGS,         PFD_TYPE_COLORINDEX, 4,  1, 0,  1, 1, 1, 2, 0, 0, 0,  0,  0,  0,  0,  32},
    {SB_FLAGS,         PFD_TYPE_COLORINDEX, 4,  1, 0,  1, 1, 1, 2, 0, 0, 0,  0,  0,  0,  0,  16},
};

static const struct pixel_format pixel_formats_8[] =
{
    /* 8bpp */
    {SB_FLAGS_PALETTE, PFD_TYPE_RGBA,       8,  3, 0,  3, 3, 2, 6, 0, 0, 32, 11, 11, 10, 0,  32},
    {SB_FLAGS_PALETTE, PFD_TYPE_RGBA,       8,  3, 0,  3, 3, 2, 6, 0, 0, 32, 11, 11, 10, 0,  16},
    {DB_FLAGS_PALETTE, PFD_TYPE_RGBA,       8,  3, 0,  3, 3, 2, 6, 0, 0, 32, 11, 11, 10, 0,  32},
    {DB_FLAGS_PALETTE, PFD_TYPE_RGBA,       8,  3, 0,  3, 3, 2, 6, 0, 0, 32, 11, 11, 10, 0,  16},
    {SB_FLAGS_PALETTE, PFD_TYPE_RGBA,       8,  3, 0,  3, 3, 2, 6, 8, 0, 32, 8,  8,  8,  8,  32},
    {SB_FLAGS_PALETTE, PFD_TYPE_RGBA,       8,  3, 0,  3, 3, 2, 6, 8, 0, 32, 8,  8,  8,  8,  16},
    {DB_FLAGS_PALETTE, PFD_TYPE_RGBA,       8,  3, 0,  3, 3, 2, 6, 8, 0, 32, 8,  8,  8,  8,  32},
    {DB_FLAGS_PALETTE, PFD_TYPE_RGBA,       8,  3, 0,  3, 3, 2, 6, 8, 0, 32, 8,  8,  8,  8,  16},
    {SB_FLAGS_WINDOW,  PFD_TYPE_COLORINDEX, 8,  3, 0,  3, 3, 2, 6, 0, 0, 0,  0,  0,  0,  0,  32},
    {SB_FLAGS_WINDOW,  PFD_TYPE_COLORINDEX, 8,  3, 0,  3, 3, 2, 6, 0, 0, 0,  0,  0,  0,  0,  16},
    {DB_FLAGS,         PFD_TYPE_COLORINDEX, 8,  3, 0,  3, 3, 2, 6, 0, 0, 0,  0,  0,  0,  0,  32},
    {DB_FLAGS,         PFD_TYPE_COLORINDEX, 8,  3, 0,  3, 3, 2, 6, 0, 0, 0,  0,  0,  0,  0,  16},
    /* 24bpp */
    {SB_FLAGS,         PFD_TYPE_RGBA,       24, 8, 16, 8, 8, 8, 0, 0, 0, 64, 16, 16, 16, 0,  32},
    {SB_FLAGS,         PFD_TYPE_RGBA,       24, 8, 16, 8, 8, 8, 0, 0, 0, 64, 16, 16, 16, 0,  16},
    {SB_FLAGS,         PFD_TYPE_RGBA,       24, 8, 16, 8, 8, 8, 0, 8, 0, 64, 16, 16, 16, 16, 32},
    {SB_FLAGS,         PFD_TYPE_RGBA,       24, 8, 16, 8, 8, 8, 0, 8, 0, 64, 16, 16, 16, 16, 16},
    {SB_FLAGS,         PFD_TYPE_COLORINDEX, 24, 8, 16, 8, 8, 8, 0, 0, 0, 0,  0,  0,  0,  0,  32},
    {SB_FLAGS,         PFD_TYPE_COLORINDEX, 24, 8, 16, 8, 8, 8, 0, 0, 0, 0,  0,  0,  0,  0,  16},
    /* 32bpp */
    {SB_FLAGS,         PFD_TYPE_RGBA,       32, 8, 16, 8, 8, 8, 0, 0, 0, 64, 16, 16, 16, 0,  32},
    {SB_FLAGS,         PFD_TYPE_RGBA,       32, 8, 16, 8, 8, 8, 0, 0, 0, 64, 16, 16, 16, 0,  16},
    {SB_FLAGS,         PFD_TYPE_RGBA,       32, 8, 16, 8, 8, 8, 0, 8, 0, 64, 16, 16, 16, 16, 32},
    {SB_FLAGS,         PFD_TYPE_RGBA,       32, 8, 16, 8, 8, 8, 0, 8, 0, 64, 16, 16, 16, 16, 16},
    {SB_FLAGS,         PFD_TYPE_COLORINDEX, 32, 8, 16, 8, 8, 8, 0, 0, 0, 0,  0,  0,  0,  0,  32},
    {SB_FLAGS,         PFD_TYPE_COLORINDEX, 32, 8, 16, 8, 8, 8, 0, 0, 0, 0,  0,  0,  0,  0,  16},
    /* 16 bpp */
    {SB_FLAGS,         PFD_TYPE_RGBA,       16, 5, 10, 5, 5, 5, 0, 0, 0, 32, 11, 11, 10, 0,  32},
    {SB_FLAGS,         PFD_TYPE_RGBA,       16, 5, 10, 5, 5, 5, 0, 0, 0, 32, 11, 11, 10, 0,  16},
    {SB_FLAGS,         PFD_TYPE_RGBA,       16, 5, 10, 5, 5, 5, 0, 8, 0, 32, 8,  8,  8,  8,  32},
    {SB_FLAGS,         PFD_TYPE_RGBA,       16, 5, 10, 5, 5, 5, 0, 8, 0, 32, 8,  8,  8,  8,  16},
    {SB_FLAGS,         PFD_TYPE_COLORINDEX, 16, 5, 10, 5, 5, 5, 0, 0, 0, 0,  0,  0,  0,  0,  32},
    {SB_FLAGS,         PFD_TYPE_COLORINDEX, 16, 5, 10, 5, 5, 5, 0, 0, 0, 0,  0,  0,  0,  0,  16},
    /* 4bpp */
    {SB_FLAGS,         PFD_TYPE_RGBA,       4,  1, 0,  1, 1, 1, 2, 0, 0, 16, 5,  6,  5,  0,  32},
    {SB_FLAGS,         PFD_TYPE_RGBA,       4,  1, 0,  1, 1, 1, 2, 0, 0, 16, 5,  6,  5,  0,  16},
    {SB_FLAGS,         PFD_TYPE_RGBA,       4,  1, 0,  1, 1, 1, 2, 8, 0, 16, 4,  4,  4,  4,  32},
    {SB_FLAGS,         PFD_TYPE_RGBA,       4,  1, 0,  1, 1, 1, 2, 8, 0, 16, 4,  4,  4,  4,  16},
    {SB_FLAGS,         PFD_TYPE_COLORINDEX, 4,  1, 0,  1, 1, 1, 2, 0, 0, 0,  0,  0,  0,  0,  32},
    {SB_FLAGS,         PFD_TYPE_COLORINDEX, 4,  1, 0,  1, 1, 1, 2, 0, 0, 0,  0,  0,  0,  0,  16},
};

struct sw_framebuffer
{
    GLvisual *gl_visual;        /* Describes the buffers */
    GLframebuffer *gl_buffer;   /* Depth, stencil, accum, etc buffers */

    const struct pixel_format* pixel_format;
    HDC Hdc;

    /* Current width/height */
    GLuint width; GLuint height;

    /* BackBuffer, if any */
    BYTE* BackBuffer;
};

struct sw_context
{
    GLcontext *gl_ctx;          /* The core GL/Mesa context */

    /* This is to keep track of the size of the front buffer */
    HHOOK hook;

    /* Our frame buffer*/
    struct sw_framebuffer* fb;

    /* State variables */
    union
    {
        struct
        {
            BYTE ClearColor;
            BYTE CurrentColor;
        } u8;
        struct
        {
            USHORT ClearColor;
            USHORT CurrentColor;
        } u16;
        struct
        {
            ULONG ClearColor;
            ULONG CurrentColor;
        } u24;
        struct
        {
            ULONG ClearColor;
            ULONG CurrentColor;
        } u32;
    };
    GLenum Mode;
};

/* WGL <-> mesa glue */
static const struct pixel_format* get_format(INT pf_index, INT* pf_count)
{
    HDC hdc;
    INT bpp, nb_format;
    const struct pixel_format* ret;

    hdc = GetDC(NULL);
    bpp = GetDeviceCaps(hdc, BITSPIXEL);
    ReleaseDC(NULL, hdc);

    switch (bpp)
    {
#define HANDLE_BPP(__x__)                               \
    case __x__:                                         \
        nb_format = ARRAYSIZE(pixel_formats_##__x__);   \
        if ((pf_index > nb_format) || (pf_index <= 0))  \
            ret = NULL;                                 \
        else                                            \
            ret = &pixel_formats_##__x__[pf_index - 1]; \
    break

    HANDLE_BPP(32);
    HANDLE_BPP(24);
    HANDLE_BPP(16);
    HANDLE_BPP(8);
#undef HANDLE_BPP
    default:
        FIXME("Unhandled bit depth %u, defaulting to 32bpp\n", bpp);
        nb_format = ARRAYSIZE(pixel_formats_32);
        if ((pf_index > nb_format) || (pf_index == 0))
            ret = NULL;
        else
            ret = &pixel_formats_32[pf_index - 1];
    }

    if (pf_count)
        *pf_count = nb_format;

    return ret;
}

INT sw_DescribePixelFormat(HDC hdc, INT format, UINT size, PIXELFORMATDESCRIPTOR* descr)
{
    INT ret;
    const struct pixel_format *pixel_format;

    TRACE("Describing format %i.\n", format);

    pixel_format = get_format(format, &ret);
    if(!descr)
        return ret;
    if((format > ret) || (size != sizeof(*descr)))
        return 0;

    /* Fill the structure */
    descr->nSize            = sizeof(*descr);
    descr->nVersion         = 1;
    descr->dwFlags          = pixel_format->dwFlags;
    descr->iPixelType       = pixel_format->iPixelType;
    descr->cColorBits       = pixel_format->cColorBits;
    descr->cRedBits         = pixel_format->cRedBits;
    descr->cRedShift        = pixel_format->cRedShift;
    descr->cGreenBits       = pixel_format->cGreenBits;
    descr->cGreenShift      = pixel_format->cGreenShift;
    descr->cBlueBits        = pixel_format->cBlueBits;
    descr->cBlueShift       = pixel_format->cBlueShift;
    descr->cAlphaBits       = pixel_format->cAlphaBits;
    descr->cAlphaShift      = pixel_format->cAlphaShift;
    descr->cAccumBits       = pixel_format->cAccumBits;
    descr->cAccumRedBits    = pixel_format->cAccumRedBits;
    descr->cAccumGreenBits  = pixel_format->cAccumGreenBits;
    descr->cAccumBlueBits   = pixel_format->cAccumBlueBits;
    descr->cAccumAlphaBits  = pixel_format->cAccumAlphaBits;
    descr->cDepthBits       = pixel_format->cDepthBits;
    descr->cStencilBits     = STENCIL_BITS;
    descr->cAuxBuffers      = 0;
    descr->iLayerType       = PFD_MAIN_PLANE;
    descr->bReserved        = 0;
    descr->dwLayerMask      = 0;
    descr->dwVisibleMask    = 0;
    descr->dwDamageMask     = 0;

    return ret;
}

BOOL sw_SetPixelFormat(HDC hdc, struct wgl_dc_data* dc_data, INT format)
{
    struct sw_framebuffer* fb;
    const struct pixel_format *pixel_format;

    /* So, someone is crazy enough to ask for sw implementation. Announce it. */
    TRACE("OpenGL software implementation START for hdc %p, format %i!\n", hdc, format);

    pixel_format = get_format(format, NULL);
    if (!pixel_format)
        return FALSE;

    /* allocate our structure */
    fb = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*fb));
    if(!fb)
    {
        ERR("HeapAlloc FAILED!\n");
        return FALSE;
    }
    /* Set the format */
    fb->pixel_format = pixel_format;

    fb->gl_visual = gl_create_visual(
            pixel_format->iPixelType == PFD_TYPE_RGBA,
            pixel_format->cAlphaBits != 0,
            (pixel_format->dwFlags & PFD_DOUBLEBUFFER) != 0,
            pixel_format->cDepthBits,
            STENCIL_BITS,
            max(max(max(pixel_format->cAccumRedBits, pixel_format->cAccumGreenBits), pixel_format->cAccumBlueBits), pixel_format->cAccumAlphaBits),
            pixel_format->iPixelType == PFD_TYPE_COLORINDEX ? pixel_format->cColorBits : 0,
            ((1ul << pixel_format->cRedBits) - 1),
            ((1ul << pixel_format->cGreenBits) - 1),
            ((1ul << pixel_format->cBlueBits) - 1),
            pixel_format->cAlphaBits != 0 ? ((1ul << pixel_format->cAlphaBits) - 1) : 255.0f,
            pixel_format->cRedBits,
            pixel_format->cGreenBits,
            pixel_format->cBlueBits,
            pixel_format->cAlphaBits);

    if(!fb->gl_visual)
    {
        ERR("Failed to allocate a GL visual.\n");
        HeapFree(GetProcessHeap(), 0, fb);
        return FALSE;
    }

    /* Allocate the framebuffer structure */
    fb->gl_buffer = gl_create_framebuffer(fb->gl_visual);
    if (!fb->gl_buffer) {
        ERR("Failed to allocate the mesa framebuffer structure.\n");
        gl_destroy_visual( fb->gl_visual );
        HeapFree(GetProcessHeap(), 0, fb);
        return FALSE;
    }

    /* Save our DC */
    fb->Hdc = hdc;

    /* Everything went fine */
    dc_data->sw_data = fb;
    return TRUE;
}

DHGLRC sw_CreateContext(struct wgl_dc_data* dc_data)
{
    struct sw_context* sw_ctx;
    struct sw_framebuffer* fb = dc_data->sw_data;

    sw_ctx = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*sw_ctx));
    if(!sw_ctx)
        return NULL;

    /* Initialize the context */
    sw_ctx->gl_ctx = gl_create_context(fb->gl_visual, NULL, sw_ctx);
    if(!sw_ctx->gl_ctx)
    {
        ERR("Failed to initialize the mesa context.\n");
        HeapFree(GetProcessHeap(), 0, sw_ctx);
        return NULL;
    }

    sw_ctx->fb = fb;

    /* Choose relevant default */
    sw_ctx->Mode = fb->gl_visual->DBflag ? GL_BACK : GL_FRONT;

    return (DHGLRC)sw_ctx;
}

BOOL sw_DeleteContext(DHGLRC dhglrc)
{
    struct sw_context* sw_ctx = (struct sw_context*)dhglrc;
    /* Those get clobbered by _mesa_free_context_data via _glapi_set{context,dispath_table} */
    void* icd_save = IntGetCurrentICDPrivate();
    const GLDISPATCHTABLE* table_save = IntGetCurrentDispatchTable();

    /* Destroy everything */
    gl_destroy_context(sw_ctx->gl_ctx);

    HeapFree(GetProcessHeap(), 0, sw_ctx);

    /* Restore this */
    IntSetCurrentDispatchTable(table_save);
    IntSetCurrentICDPrivate(icd_save);

    return TRUE;
}

extern void APIENTRY _mesa_ColorTableEXT(GLenum, GLenum, GLsizei, GLenum, GLenum, const void*);
extern void APIENTRY _mesa_ColorSubTableEXT(GLenum, GLsizei, GLsizei, GLenum, GLenum, const void*);
extern void APIENTRY _mesa_GetColorTableEXT(GLenum, GLenum, GLenum, void*);
extern void APIENTRY _mesa_GetColorTableParameterivEXT(GLenum, GLenum, GLfloat*);
extern void APIENTRY _mesa_GetColorTableParameterfvEXT(GLenum, GLenum, GLint*);

static void APIENTRY _swimpl_AddSwapHintRectWIN(GLint x, GLint y, GLsizei width, GLsizei height)
{
    UNIMPLEMENTED;
}

PROC sw_GetProcAddress(LPCSTR name)
{
    /* GL_EXT_paletted_texture */
    if (strcmp(name, "glColorTableEXT") == 0)
        return (PROC)_mesa_ColorTableEXT;
    if (strcmp(name, "glColorSubTableEXT") == 0)
        return (PROC)_mesa_ColorSubTableEXT;
    if (strcmp(name, "glColorGetTableEXT") == 0)
        return (PROC)_mesa_GetColorTableEXT;
    if (strcmp(name, "glGetColorTableParameterivEXT") == 0)
        return (PROC)_mesa_GetColorTableParameterivEXT;
    if (strcmp(name, "glGetColorTableParameterfvEXT") == 0)
        return (PROC)_mesa_GetColorTableParameterfvEXT;
    if (strcmp(name, "glAddSwapHintRectWIN") == 0)
        return (PROC)_swimpl_AddSwapHintRectWIN;

    WARN("Asking for proc address %s, returning NULL.\n", name);
    return NULL;
}

BOOL sw_CopyContext(DHGLRC dhglrcSrc, DHGLRC dhglrcDst, UINT mask)
{
    FIXME("Software wglCopyContext is UNIMPLEMENTED, mask %lx.\n", mask);
    return FALSE;
}

BOOL sw_ShareLists(DHGLRC dhglrcSrc, DHGLRC dhglrcDst)
{
#if 0
    struct sw_context* sw_ctx_src = (struct sw_context*)dhglrcSrc;
    struct sw_context* sw_ctx_dst = (struct sw_context*)dhglrcDst;

    /* See if it was already shared */
    if(sw_ctx_dst->gl_ctx->Shared->RefCount > 1)
        return FALSE;

    /* Unreference the old, share the new */
    gl_reference_shared_state(sw_ctx_dst->gl_ctx,
        &sw_ctx_dst->gl_ctx->Shared,
        sw_ctx_src->gl_ctx->Shared);
#endif
    FIXME("Unimplemented!\n");
    return TRUE;
}

static
LRESULT CALLBACK
sw_call_window_proc(
   int nCode,
   WPARAM wParam,
   LPARAM lParam )
{
    struct wgl_dc_data* dc_data = IntGetCurrentDcData();
    struct sw_context* ctx = (struct sw_context*)IntGetCurrentDHGLRC();
    PCWPSTRUCT pParams = (PCWPSTRUCT)lParam;

    if((!dc_data) || (!ctx))
        return 0;

    if(!(dc_data->flags & WGL_DC_OBJ_DC))
        return 0;

    if((nCode < 0) || (dc_data->owner.hwnd != pParams->hwnd) || (dc_data->sw_data == NULL))
        return CallNextHookEx(ctx->hook, nCode, wParam, lParam);

    if (pParams->message == WM_WINDOWPOSCHANGED)
    {
        /* We handle WM_WINDOWPOSCHANGED instead of WM_SIZE because according to
         * http://blogs.msdn.com/oldnewthing/archive/2008/01/15/7113860.aspx
         * WM_SIZE is generated from WM_WINDOWPOSCHANGED by DefWindowProc so it
         * can be masked out by the application. */
        LPWINDOWPOS lpWindowPos = (LPWINDOWPOS)pParams->lParam;
        if((lpWindowPos->flags & SWP_SHOWWINDOW) ||
            !(lpWindowPos->flags & SWP_NOMOVE) ||
            !(lpWindowPos->flags & SWP_NOSIZE))
        {
            /* Size in WINDOWPOS includes the window frame, so get the size
             * of the client area via GetClientRect.  */
            RECT client_rect;
            UINT width, height;

            TRACE("Got WM_WINDOWPOSCHANGED\n");

            GetClientRect(pParams->hwnd, &client_rect);
            width = client_rect.right - client_rect.left;
            height = client_rect.bottom - client_rect.top;
            /* Do not reallocate for minimized windows */
            if(width <= 0 || height <= 0)
                goto end;
            /* Propagate to mesa */
            gl_ResizeBuffersMESA(ctx->gl_ctx);
        }
    }

end:
    return CallNextHookEx(ctx->hook, nCode, wParam, lParam);
}

static const char* renderer_string(void)
{
    return "ReactOS SW Implementation";
}

static inline void PUT_PIXEL_8(BYTE* Buffer, BYTE Value)
{
    *Buffer = Value;
}
static inline void PUT_PIXEL_16(USHORT* Buffer, USHORT Value)
{
    *Buffer = Value;
}
static inline void PUT_PIXEL_24(ULONG* Buffer, ULONG Value)
{
    *Buffer &= 0xFF000000ul;
    *Buffer |= Value & 0x00FFFFFF;
}
static inline void PUT_PIXEL_32(ULONG* Buffer, ULONG Value)
{
    *Buffer = Value;
}

static inline BYTE GET_PIXEL_8(BYTE* Buffer)
{
    return *Buffer;
}

static inline USHORT GET_PIXEL_16(USHORT* Buffer)
{
    return *Buffer;
}

static inline ULONG GET_PIXEL_24(ULONG* Buffer)
{
    return *Buffer & 0x00FFFFFF;
}

static inline ULONG GET_PIXEL_32(ULONG* Buffer)
{
    return *Buffer;
}

static inline BYTE PACK_COLOR_8(GLubyte r, GLubyte g, GLubyte b)
{
    return (r & 0x7) | ((g & 0x7) << 3) | ((b & 0x3) << 6);
}

static inline USHORT PACK_COLOR_16(GLubyte r, GLubyte g, GLubyte b)
{
    return ((r & 0x1F) << 11) | ((g & 0x3F) << 5) | (b & 0x1F);
}

static inline ULONG PACK_COLOR_24(GLubyte r, GLubyte g, GLubyte b)
{
    return (r << 16) | (g << 8) | (b);
}

static inline ULONG PACK_COLOR_32(GLubyte r, GLubyte g, GLubyte b)
{
    return (r << 16) | (g << 8) | (b);
}

static inline COLORREF PACK_COLORREF_8(GLubyte r, GLubyte g, GLubyte b)
{
    return RGB(r << 5, g << 5, b << 6);
}

static inline COLORREF PACK_COLORREF_16(GLubyte r, GLubyte g, GLubyte b)
{
    return RGB(r << 3, g << 2, b << 3);
}

static inline COLORREF PACK_COLORREF_24(GLubyte r, GLubyte g, GLubyte b)
{
    return RGB(r, g, b);
}

static inline COLORREF PACK_COLORREF_32(GLubyte r, GLubyte g, GLubyte b)
{
    return RGB(r, g, b);
}

static inline void UNPACK_COLOR_8(BYTE Color, GLubyte* r, GLubyte* g, GLubyte* b)
{
    *r = Color & 0x7;
    *g = (Color >> 3) & 0x7;
    *b = (Color >> 6) & 0x3;
}

static inline void UNPACK_COLOR_16(USHORT Color, GLubyte* r, GLubyte* g, GLubyte* b)
{
    *r = (Color >> 11) & 0x1F;
    *g = (Color >> 5) & 0x3F;
    *b = Color & 0x1F;
}

static inline void UNPACK_COLOR_24(ULONG Color, GLubyte* r, GLubyte* g, GLubyte* b)
{
    *r = (Color >> 16) & 0xFF;
    *g = (Color >> 8) & 0xFF;
    *b = Color & 0xFF;
}

static inline void UNPACK_COLOR_32(ULONG Color, GLubyte* r, GLubyte* g, GLubyte* b)
{
    *r = (Color >> 16) & 0xFF;
    *g = (Color >> 8) & 0xFF;
    *b = Color & 0xFF;
}

static inline void UNPACK_COLORREF_8(COLORREF Color, GLubyte* r, GLubyte* g, GLubyte* b)
{
    *r = GetRValue(Color) >> 5;
    *g = GetGValue(Color) >> 5;
    *b = GetBValue(Color) >> 6;
}

static inline void UNPACK_COLORREF_16(COLORREF Color, GLubyte* r, GLubyte* g, GLubyte* b)
{
    *r = GetRValue(Color) >> 3;
    *g = GetGValue(Color) >> 2;
    *b = GetBValue(Color) >> 3;
}

static inline void UNPACK_COLORREF_24(COLORREF Color, GLubyte* r, GLubyte* g, GLubyte* b)
{
    *r = GetRValue(Color);
    *g = GetGValue(Color);
    *b = GetBValue(Color);
}

static inline void UNPACK_COLORREF_32(COLORREF Color, GLubyte* r, GLubyte* g, GLubyte* b)
{
    *r = GetRValue(Color);
    *g = GetGValue(Color);
    *b = GetBValue(Color);
}

#define MAKE_COLORREF(__bpp, __type)                                                            \
static inline COLORREF MAKE_COLORREF_##__bpp(const struct pixel_format *format, __type Color)   \
{                                                                                               \
    GLubyte r,g,b;                                                                              \
                                                                                                \
    if (format->iPixelType == PFD_TYPE_COLORINDEX)                                              \
        return PALETTEINDEX(Color);                                                             \
                                                                                                \
    UNPACK_COLOR_##__bpp(Color, &r, &g, &b);                                                    \
                                                                                                \
    return PACK_COLORREF_##__bpp(r, g, b);                                                      \
}
MAKE_COLORREF(8, BYTE)
MAKE_COLORREF(16, USHORT)
MAKE_COLORREF(24, ULONG)
MAKE_COLORREF(32, ULONG)
#undef MAKE_COLORREF

/*
* Set the color index used to clear the color buffer.
*/
#define CLEAR_INDEX(__bpp, __type)                              \
static void clear_index_##__bpp(GLcontext* ctx, GLuint index)   \
{                                                               \
    struct sw_context* sw_ctx = ctx->DriverCtx;                 \
                                                                \
    sw_ctx->u##__bpp.ClearColor = (__type)index;                \
}
CLEAR_INDEX(8, BYTE)
CLEAR_INDEX(16, USHORT)
CLEAR_INDEX(24, ULONG)
CLEAR_INDEX(32, ULONG)
#undef CLEAR_INDEX

/*
* Set the color used to clear the color buffer.
*/
#define CLEAR_COLOR(__bpp)                                                                 \
static void clear_color_##__bpp( GLcontext* ctx, GLubyte r, GLubyte g, GLubyte b, GLubyte a )   \
{                                                                                               \
    struct sw_context* sw_ctx = ctx->DriverCtx;                                                 \
                                                                                                \
    sw_ctx->u##__bpp.ClearColor = PACK_COLOR_##__bpp(r, g, b);                                  \
                                                                                                \
    TRACE("Set Clear color %u, %u, %u.\n", r, g, b);                                            \
}
CLEAR_COLOR(8)
CLEAR_COLOR(16)
CLEAR_COLOR(24)
CLEAR_COLOR(32)
#undef CLEAR_COLOR

/*
 * Clear the specified region of the color buffer using the clear color
 * or index as specified by one of the two functions above.
 */
static void clear_frontbuffer(
    struct sw_context* sw_ctx,
    struct sw_framebuffer* fb,
    GLint x,
    GLint y,
    GLint width,
    GLint height,
    COLORREF ClearColor)
{
    HBRUSH Brush;
    BOOL ret;

    TRACE("Clearing front buffer (%u, %u, %u, %u), color 0x%08x.\n", x, y, width, height, ClearColor);

    Brush = CreateSolidBrush(ClearColor);
    Brush = SelectObject(fb->Hdc, Brush);

    ret = PatBlt(fb->Hdc, x, fb->height - (y + height), width, height, PATCOPY);
    if (!ret)
    {
        ERR("PatBlt failed. last Error %d.\n", GetLastError());
    }

    Brush = SelectObject(fb->Hdc, Brush);
    DeleteObject(Brush);
}

#define CLEAR(__bpp, __type, __pixel_size)                                                          \
static void clear_##__bpp(GLcontext* ctx, GLboolean all,GLint x, GLint y, GLint width, GLint height)\
{                                                                                                   \
    struct sw_context* sw_ctx = ctx->DriverCtx;                                                     \
    struct sw_framebuffer* fb = sw_ctx->fb;                                                         \
    BYTE* ScanLine;                                                                                 \
                                                                                                    \
    if (all)                                                                                        \
    {                                                                                               \
        x = y = 0;                                                                                  \
        width = fb->width;                                                                          \
        height = fb->height;                                                                        \
    }                                                                                               \
                                                                                                    \
    if (sw_ctx->Mode == GL_FRONT)                                                                   \
    {                                                                                               \
        clear_frontbuffer(sw_ctx, fb, x, y, width, height,                                          \
                MAKE_COLORREF_##__bpp(fb->pixel_format, sw_ctx->u##__bpp.ClearColor));              \
        return;                                                                                     \
    }                                                                                               \
                                                                                                    \
    ScanLine = fb->BackBuffer + y * WIDTH_BYTES_ALIGN32(fb->width, __bpp);                          \
    while (height--)                                                                                \
    {                                                                                               \
        BYTE* Buffer = ScanLine + x * __pixel_size;                                                 \
        UINT n = width;                                                                             \
                                                                                                    \
        while (n--)                                                                                 \
        {                                                                                           \
            PUT_PIXEL_##__bpp((__type*)Buffer, sw_ctx->u##__bpp.ClearColor);                        \
            Buffer += __pixel_size;                                                                 \
        }                                                                                           \
                                                                                                    \
        ScanLine += WIDTH_BYTES_ALIGN32(fb->width, __bpp);                                          \
    }                                                                                               \
}
CLEAR(8, BYTE, 1)
CLEAR(16, USHORT, 2)
CLEAR(24, ULONG, 3)
CLEAR(32, ULONG, 4)
#undef CLEAR

/* Set the current color index. */
#define SET_INDEX(__bpp)                                        \
static void set_index_##__bpp(GLcontext* ctx, GLuint index)     \
{                                                               \
    struct sw_context* sw_ctx = ctx->DriverCtx;                 \
                                                                \
    sw_ctx->u##__bpp.CurrentColor = index;                      \
}
SET_INDEX(8)
SET_INDEX(16)
SET_INDEX(24)
SET_INDEX(32)
#undef SET_INDEX

/* Set the current RGBA color. */
#define SET_COLOR(__bpp)                                                                    \
static void set_color_##__bpp( GLcontext* ctx, GLubyte r, GLubyte g, GLubyte b, GLubyte a ) \
{                                                                                           \
    struct sw_context* sw_ctx = ctx->DriverCtx;                                             \
                                                                                            \
    sw_ctx->u##__bpp.CurrentColor = PACK_COLOR_##__bpp(r, g, b);                            \
}
SET_COLOR(8)
SET_COLOR(16)
SET_COLOR(24)
SET_COLOR(32)
#undef SET_COLOR

/*
 * Selects either the front or back color buffer for reading and writing.
 * mode is either GL_FRONT or GL_BACK.
 */
static GLboolean set_buffer( GLcontext* ctx, GLenum mode )
{
    struct sw_context* sw_ctx = ctx->DriverCtx;
    struct sw_framebuffer* fb = sw_ctx->fb;

    if (!fb->gl_visual->DBflag)
        return GL_FALSE;

    if ((mode != GL_FRONT) && (mode != GL_BACK))
        return GL_FALSE;

    sw_ctx->Mode = mode;
    return GL_TRUE;
}

/* Return characteristics of the output buffer. */
static void buffer_size(GLcontext* ctx, GLuint *width, GLuint *height)
{
    struct sw_context* sw_ctx = ctx->DriverCtx;
    struct sw_framebuffer* fb = sw_ctx->fb;
    HWND Window = WindowFromDC(fb->Hdc);

    if (Window)
    {
        RECT client_rect;
        GetClientRect(Window, &client_rect);
        *width = client_rect.right - client_rect.left;
        *height = client_rect.bottom - client_rect.top;
    }
    else
    {
        /* We are drawing to a bitmap */
        BITMAP bm;
        HBITMAP Hbm;

        Hbm = GetCurrentObject(fb->Hdc, OBJ_BITMAP);

        if (!GetObjectW(Hbm, sizeof(bm), &bm))
            return;

        TRACE("Framebuffer size : %i, %i\n", bm.bmWidth, bm.bmHeight);

        *width = bm.bmWidth;
        *height = bm.bmHeight;
    }

    if ((*width != fb->width) || (*height != fb->height))
    {
        const struct pixel_format* pixel_format = fb->pixel_format;

        if (pixel_format->dwFlags & PFD_DOUBLEBUFFER)
        {
            /* Allocate a new backbuffer */
            size_t BufferSize = *height * WIDTH_BYTES_ALIGN32(*width, pixel_format->cColorBits);
            if (!fb->BackBuffer)
            {
                fb->BackBuffer = HeapAlloc(GetProcessHeap(), 0, BufferSize);
            }
            else
            {
                fb->BackBuffer = HeapReAlloc(GetProcessHeap(), 0, fb->BackBuffer, BufferSize);
            }
            if (!fb->BackBuffer)
            {
                ERR("Failed allocating back buffer !.\n");
                return;
            }
        }

        fb->width = *width;
        fb->height = *height;
    }
}

/* Write a horizontal span of color pixels with a boolean mask. */
#define WRITE_COLOR_SPAN_FRONTBUFFER(__bpp)                                     \
static void write_color_span_frontbuffer_##__bpp(struct sw_framebuffer* fb,     \
        GLuint n, GLint x, GLint y,                                             \
        const GLubyte red[], const GLubyte green[],                             \
        const GLubyte blue[], const GLubyte mask[] )                            \
{                                                                               \
    TRACE("Writing color span at %u, %u (%u)\n", x, y, n);                      \
                                                                                \
    if (mask)                                                                   \
    {                                                                           \
        while (n--)                                                             \
        {                                                                       \
            if (mask[n])                                                        \
            {                                                                   \
                SetPixel(fb->Hdc, x + n, fb->height - y,                        \
                        PACK_COLORREF_##__bpp(red[n], green[n], blue[n]));      \
            }                                                                   \
        }                                                                       \
    }                                                                           \
    else                                                                        \
    {                                                                           \
        while (n--)                                                             \
        {                                                                       \
            SetPixel(fb->Hdc, x + n, fb->height - y,                            \
                    PACK_COLORREF_##__bpp(red[n], green[n], blue[n]));          \
        }                                                                       \
    }                                                                           \
}
WRITE_COLOR_SPAN_FRONTBUFFER(8)
WRITE_COLOR_SPAN_FRONTBUFFER(16)
WRITE_COLOR_SPAN_FRONTBUFFER(24)
WRITE_COLOR_SPAN_FRONTBUFFER(32)
#undef WRITE_COLOR_SPAN_FRONTBUFFER

#define WRITE_COLOR_SPAN(__bpp, __type, __pixel_size)                               \
static void write_color_span_##__bpp(GLcontext* ctx,                                \
                                     GLuint n, GLint x, GLint y,                    \
                                     const GLubyte red[], const GLubyte green[],    \
                                     const GLubyte blue[], const GLubyte alpha[],   \
                                     const GLubyte mask[] )                         \
{                                                                                   \
    struct sw_context* sw_ctx = ctx->DriverCtx;                                     \
    struct sw_framebuffer* fb = sw_ctx->fb;                                         \
    BYTE* Buffer;                                                                   \
                                                                                    \
    if (sw_ctx->Mode == GL_FRONT)                                                   \
    {                                                                               \
        write_color_span_frontbuffer_##__bpp(fb, n, x, y, red, green, blue, mask);  \
        return;                                                                     \
    }                                                                               \
                                                                                    \
    Buffer = fb->BackBuffer + y * WIDTH_BYTES_ALIGN32(fb->width, __bpp)             \
            + (x + n) * __pixel_size;                                               \
    if (mask)                                                                       \
    {                                                                               \
        while (n--)                                                                 \
        {                                                                           \
            Buffer -= __pixel_size;                                                 \
            if (mask[n])                                                            \
            {                                                                       \
                PUT_PIXEL_##__bpp((__type*)Buffer,                                  \
                        PACK_COLOR_##__bpp(red[n], green[n], blue[n]));             \
            }                                                                       \
        }                                                                           \
    }                                                                               \
    else                                                                            \
    {                                                                               \
        while (n--)                                                                 \
        {                                                                           \
            Buffer -= __pixel_size;                                                 \
            PUT_PIXEL_##__bpp((__type*)Buffer,                                      \
                    PACK_COLOR_##__bpp(red[n], green[n], blue[n]));                 \
        }                                                                           \
    }                                                                               \
}
WRITE_COLOR_SPAN(8, BYTE, 1)
WRITE_COLOR_SPAN(16, USHORT, 2)
WRITE_COLOR_SPAN(24, ULONG, 3)
WRITE_COLOR_SPAN(32, ULONG, 4)
#undef WRITE_COLOR_SPAN

static void write_monocolor_span_frontbuffer(struct sw_framebuffer* fb, GLuint n, GLint x, GLint y,
        const GLubyte mask[], COLORREF Color)
{
    TRACE("Writing monocolor span at %u %u (%u), Color 0x%08x", x, y, n, Color);

    if (mask)
    {
        while (n--)
        {
            if (mask[n])
                SetPixel(fb->Hdc, x + n, y, Color);
        }
    }
    else
    {
        HBRUSH Brush = CreateSolidBrush(Color);
        Brush = SelectObject(fb->Hdc, Brush);

        PatBlt(fb->Hdc, x, fb->height - y, n, 1, PATCOPY);

        Brush = SelectObject(fb->Hdc, Brush);
        DeleteObject(Brush);
    }
}

#define WRITE_MONOCOLOR_SPAN(__bpp, __type, __pixel_size)                                           \
static void write_monocolor_span_##__bpp(GLcontext* ctx,                                            \
                                 GLuint n, GLint x, GLint y,                                        \
                                 const GLubyte mask[])                                              \
{                                                                                                   \
    struct sw_context* sw_ctx = ctx->DriverCtx;                                                     \
    struct sw_framebuffer* fb = sw_ctx->fb;                                                         \
    BYTE* Buffer;                                                                                   \
                                                                                                    \
    if (sw_ctx->Mode == GL_FRONT)                                                                   \
    {                                                                                               \
        write_monocolor_span_frontbuffer(fb, n, x, y, mask,                                         \
                MAKE_COLORREF_##__bpp(fb->pixel_format, sw_ctx->u##__bpp.CurrentColor));            \
        return;                                                                                     \
    }                                                                                               \
                                                                                                    \
    Buffer = fb->BackBuffer + y * WIDTH_BYTES_ALIGN32(fb->width, __bpp) + (x + n) * __pixel_size;   \
    if (mask)                                                                                       \
    {                                                                                               \
        while (n--)                                                                                 \
        {                                                                                           \
            Buffer -= __pixel_size;                                                                 \
            if (mask[n])                                                                            \
                PUT_PIXEL_##__bpp((__type*)Buffer, sw_ctx->u##__bpp.CurrentColor);                  \
        }                                                                                           \
    }                                                                                               \
    else                                                                                            \
    {                                                                                               \
        while(n--)                                                                                  \
        {                                                                                           \
            Buffer -= __pixel_size;                                                                 \
            PUT_PIXEL_##__bpp((__type*)Buffer, sw_ctx->u##__bpp.CurrentColor);                      \
        }                                                                                           \
    }                                                                                               \
}
WRITE_MONOCOLOR_SPAN(8, BYTE, 1)
WRITE_MONOCOLOR_SPAN(16, USHORT, 2)
WRITE_MONOCOLOR_SPAN(24, ULONG, 3)
WRITE_MONOCOLOR_SPAN(32, ULONG, 4)
#undef WRITE_MONOCOLOR_SPAN

/* Write an array of pixels with a boolean mask. */
#define WRITE_COLOR_PIXELS(__bpp, __type, __pixel_size)                                             \
static void write_color_pixels_##__bpp(GLcontext* ctx,                                              \
                               GLuint n, const GLint x[], const GLint y[],                          \
                               const GLubyte r[], const GLubyte g[],                                \
                               const GLubyte b[], const GLubyte a[],                                \
                               const GLubyte mask[])                                                \
{                                                                                                   \
    struct sw_context* sw_ctx = ctx->DriverCtx;                                                     \
    struct sw_framebuffer* fb = sw_ctx->fb;                                                         \
                                                                                                    \
    TRACE("Writing color pixels\n");                                                                \
                                                                                                    \
    if (sw_ctx->Mode == GL_FRONT)                                                                   \
    {                                                                                               \
        while (n--)                                                                                 \
        {                                                                                           \
            if (mask[n])                                                                            \
            {                                                                                       \
                TRACE("Setting pixel %u, %u to 0x%08x.\n", x[n], fb->height - y[n],                 \
                        PACK_COLORREF_##__bpp(r[n], g[n], b[n]));                                   \
                SetPixel(fb->Hdc, x[n], fb->height - y[n],                                          \
                        PACK_COLORREF_##__bpp(r[n], g[n], b[n]));                                   \
            }                                                                                       \
        }                                                                                           \
                                                                                                    \
        return;                                                                                     \
    }                                                                                               \
                                                                                                    \
    while (n--)                                                                                     \
    {                                                                                               \
        if (mask[n])                                                                                \
        {                                                                                           \
            BYTE* Buffer = fb->BackBuffer + y[n] * WIDTH_BYTES_ALIGN32(fb->width, __bpp)            \
                            + x[n] * __pixel_size;                                                  \
            PUT_PIXEL_##__bpp((__type*)Buffer, PACK_COLOR_##__bpp(r[n], g[n], b[n]));               \
        }                                                                                           \
    }                                                                                               \
}
WRITE_COLOR_PIXELS(8, BYTE, 1)
WRITE_COLOR_PIXELS(16, USHORT, 2)
WRITE_COLOR_PIXELS(24, ULONG, 3)
WRITE_COLOR_PIXELS(32, ULONG, 4)
#undef WRITE_COLOR_PIXELS

static void write_monocolor_pixels_frontbuffer(
        struct sw_framebuffer* fb, GLuint n,
        const GLint x[], const GLint y[],
        const GLubyte mask[], COLORREF Color)
{
    TRACE("Writing monocolor pixels to front buffer.\n");

    while (n--)
    {
        if (mask[n])
        {
            SetPixel(fb->Hdc, x[n], fb->height - y[n], Color);
        }
    }
}

/*
* Write an array of pixels with a boolean mask.  The current color
* is used for all pixels.
*/
#define WRITE_MONOCOLOR_PIXELS(__bpp, __type, __pixel_size)                                 \
static void write_monocolor_pixels_##__bpp(GLcontext* ctx, GLuint n,                        \
                                   const GLint x[], const GLint y[],                        \
                                   const GLubyte mask[] )                                   \
{                                                                                           \
    struct sw_context* sw_ctx = ctx->DriverCtx;                                             \
    struct sw_framebuffer* fb = sw_ctx->fb;                                                 \
                                                                                            \
    if (sw_ctx->Mode == GL_FRONT)                                                           \
    {                                                                                       \
        write_monocolor_pixels_frontbuffer(fb, n, x, y, mask,                               \
                MAKE_COLORREF_##__bpp(fb->pixel_format, sw_ctx->u##__bpp.CurrentColor));    \
                                                                                            \
        return;                                                                             \
    }                                                                                       \
                                                                                            \
    while (n--)                                                                             \
    {                                                                                       \
        if (mask[n])                                                                        \
        {                                                                                   \
            BYTE* Buffer = fb->BackBuffer + y[n] * WIDTH_BYTES_ALIGN32(fb->width, 32)       \
                    + x[n] * __pixel_size;                                                  \
            PUT_PIXEL_##__bpp((__type*)Buffer, sw_ctx->u##__bpp.CurrentColor);              \
        }                                                                                   \
    }                                                                                       \
}
WRITE_MONOCOLOR_PIXELS(8, BYTE, 1)
WRITE_MONOCOLOR_PIXELS(16, USHORT, 2)
WRITE_MONOCOLOR_PIXELS(24, ULONG, 3)
WRITE_MONOCOLOR_PIXELS(32, ULONG, 4)
#undef WRITE_MONOCOLOR_PIXELS

/* Write a horizontal span of color pixels with a boolean mask. */
static void write_index_span( GLcontext* ctx,
                             GLuint n, GLint x, GLint y,
                             const GLuint index[],
                             const GLubyte mask[] )
{
    ERR("Not implemented yet !\n");
}

/* Write an array of pixels with a boolean mask. */
static void write_index_pixels( GLcontext* ctx,
                               GLuint n, const GLint x[], const GLint y[],
                               const GLuint index[], const GLubyte mask[] )
{
    ERR("Not implemented yet !\n");
}

/* Read a horizontal span of color-index pixels. */
static void read_index_span( GLcontext* ctx, GLuint n, GLint x, GLint y, GLuint index[])
{
    ERR("Not implemented yet !\n");
}

/* Read a horizontal span of color pixels. */
#define READ_COLOR_SPAN(__bpp, __type, __pixel_size)                            \
static void read_color_span_##__bpp(GLcontext* ctx,                             \
                            GLuint n, GLint x, GLint y,                         \
                            GLubyte red[], GLubyte green[],                     \
                            GLubyte blue[], GLubyte alpha[] )                   \
{                                                                               \
    struct sw_context* sw_ctx = ctx->DriverCtx;                                 \
    struct sw_framebuffer* fb = sw_ctx->fb;                                     \
    BYTE* Buffer;                                                               \
                                                                                \
    if (sw_ctx->Mode == GL_FRONT)                                               \
    {                                                                           \
        COLORREF Color;                                                         \
        while (n--)                                                             \
        {                                                                       \
            Color = GetPixel(fb->Hdc, x + n, fb->height - y);                   \
            UNPACK_COLORREF_##__bpp(Color, &red[n], &green[n], &blue[n]);       \
            alpha[n] = 0;                                                       \
        }                                                                       \
                                                                                \
        return;                                                                 \
    }                                                                           \
                                                                                \
    Buffer = fb->BackBuffer + y * WIDTH_BYTES_ALIGN32(fb->width, __bpp)         \
            + (x + n) * __pixel_size;                                           \
    while (n--)                                                                 \
    {                                                                           \
        Buffer -= __pixel_size;                                                 \
        UNPACK_COLOR_##__bpp(GET_PIXEL_##__bpp((__type*)Buffer),                \
                &red[n], &green[n], &blue[n]);                                  \
        alpha[n] = 0;                                                           \
    }                                                                           \
}
READ_COLOR_SPAN(8, BYTE, 1)
READ_COLOR_SPAN(16, USHORT, 2)
READ_COLOR_SPAN(24, ULONG, 3)
READ_COLOR_SPAN(32, ULONG, 4)
#undef READ_COLOR_SPAN

/* Read an array of color index pixels. */
static void read_index_pixels(GLcontext* ctx,
                              GLuint n, const GLint x[], const GLint y[],
                              GLuint index[], const GLubyte mask[])
{

    ERR("Not implemented yet !\n");
}

/* Read an array of color pixels. */
#define READ_COLOR_PIXELS(__bpp, __type, __pixel_size)                                      \
static void read_color_pixels_##__bpp(GLcontext* ctx,                                       \
                              GLuint n, const GLint x[], const GLint y[],                   \
                              GLubyte red[], GLubyte green[],                               \
                              GLubyte blue[], GLubyte alpha[],                              \
                              const GLubyte mask[] )                                        \
{                                                                                           \
    struct sw_context* sw_ctx = ctx->DriverCtx;                                             \
    struct sw_framebuffer* fb = sw_ctx->fb;                                                 \
                                                                                            \
    if (sw_ctx->Mode == GL_FRONT)                                                           \
    {                                                                                       \
        COLORREF Color;                                                                     \
        while (n--)                                                                         \
        {                                                                                   \
            if (mask[n])                                                                    \
            {                                                                               \
                Color = GetPixel(fb->Hdc, x[n], fb->height - y[n]);                         \
                UNPACK_COLORREF_##__bpp(Color, &red[n], &green[n], &blue[n]);               \
                alpha[n] = 0;                                                               \
            }                                                                               \
        }                                                                                   \
                                                                                            \
        return;                                                                             \
    }                                                                                       \
                                                                                            \
    while (n--)                                                                             \
    {                                                                                       \
        if (mask[n])                                                                        \
        {                                                                                   \
            BYTE *Buffer = fb->BackBuffer + y[n] * WIDTH_BYTES_ALIGN32(fb->width, __bpp)    \
                    + x[n] * __pixel_size;                                                  \
            UNPACK_COLOR_##__bpp(GET_PIXEL_##__bpp((__type*)Buffer),                        \
                    &red[n], &green[n], &blue[n]);                                          \
            alpha[n] = 0;                                                                   \
        }                                                                                   \
    }                                                                                       \
}
READ_COLOR_PIXELS(8, BYTE, 1)
READ_COLOR_PIXELS(16, USHORT, 2)
READ_COLOR_PIXELS(24, ULONG, 3)
READ_COLOR_PIXELS(32, ULONG, 4)
#undef READ_COLOR_PIXELS

static void setup_DD_pointers( GLcontext* ctx )
{
    struct sw_context* sw_ctx = ctx->DriverCtx;

    ctx->Driver.RendererString = renderer_string;
    ctx->Driver.UpdateState = setup_DD_pointers;

    switch (sw_ctx->fb->pixel_format->cColorBits)
    {
#define HANDLE_BPP(__bpp)                                                   \
    case __bpp:                                                             \
        ctx->Driver.ClearIndex = clear_index_##__bpp;                       \
        ctx->Driver.ClearColor = clear_color_##__bpp;                       \
        ctx->Driver.Clear = clear_##__bpp;                                  \
        ctx->Driver.Index = set_index_##__bpp;                              \
        ctx->Driver.Color = set_color_##__bpp;                              \
        ctx->Driver.WriteColorSpan       = write_color_span_##__bpp;        \
        ctx->Driver.WriteMonocolorSpan   = write_monocolor_span_##__bpp;    \
        ctx->Driver.WriteMonoindexSpan   = write_monocolor_span_##__bpp;    \
        ctx->Driver.WriteColorPixels     = write_color_pixels_##__bpp;      \
        ctx->Driver.WriteMonocolorPixels = write_monocolor_pixels_##__bpp;  \
        ctx->Driver.WriteMonoindexPixels = write_monocolor_pixels_##__bpp;  \
        ctx->Driver.ReadColorSpan = read_color_span_##__bpp;                \
        ctx->Driver.ReadColorPixels = read_color_pixels_##__bpp;            \
        break
HANDLE_BPP(8);
HANDLE_BPP(16);
HANDLE_BPP(24);
HANDLE_BPP(32);
#undef HANDLE_BPP
    default:
        ERR("Unhandled bit depth %u, defaulting to 32bpp.\n", sw_ctx->fb->pixel_format->cColorBits);
        ctx->Driver.ClearIndex = clear_index_32;
        ctx->Driver.ClearColor = clear_color_32;
        ctx->Driver.Clear = clear_32;
        ctx->Driver.Index = set_index_32;
        ctx->Driver.Color = set_color_32;
        ctx->Driver.WriteColorSpan       = write_color_span_32;
        ctx->Driver.WriteMonocolorSpan   = write_monocolor_span_32;
        ctx->Driver.WriteMonoindexSpan   = write_monocolor_span_32;
        ctx->Driver.WriteColorPixels     = write_color_pixels_32;
        ctx->Driver.WriteMonocolorPixels = write_monocolor_pixels_32;
        ctx->Driver.WriteMonoindexPixels = write_monocolor_pixels_32;
        ctx->Driver.ReadColorSpan = read_color_span_32;
        ctx->Driver.ReadColorPixels = read_color_pixels_32;
        break;
    }

    ctx->Driver.SetBuffer = set_buffer;
    ctx->Driver.GetBufferSize = buffer_size;

    /* Pixel/span writing functions: */
    ctx->Driver.WriteIndexSpan       = write_index_span;
    ctx->Driver.WriteIndexPixels     = write_index_pixels;

    /* Pixel/span reading functions: */
    ctx->Driver.ReadIndexSpan = read_index_span;
    ctx->Driver.ReadIndexPixels = read_index_pixels;
}

/* Declare API table */
#define USE_GL_FUNC(name, proto_args, call_args, offset, stack) extern void WINAPI _mesa_##name proto_args ;
#define USE_GL_FUNC_RET(name, ret_type, proto_args, call_args, offset, stack) extern ret_type WINAPI _mesa_##name proto_args ;
#include "glfuncs.h"

static GLCLTPROCTABLE sw_api_table =
{
    OPENGL_VERSION_110_ENTRIES,
    {
#define USE_GL_FUNC(name, proto_args, call_args, offset, stack) _mesa_##name,
#include "glfuncs.h"
    }
};

/* Glue code */
GLcontext* gl_get_thread_context(void)
{
    struct sw_context* sw_ctx = (struct sw_context*)IntGetCurrentDHGLRC();
    return sw_ctx->gl_ctx;
}


BOOL sw_SetContext(struct wgl_dc_data* dc_data, DHGLRC dhglrc)
{
    struct sw_context* sw_ctx = (struct sw_context*)dhglrc;
    struct sw_framebuffer* fb = dc_data->sw_data;
    UINT width, height;

    /* Get framebuffer size */
    if(dc_data->flags & WGL_DC_OBJ_DC)
    {
        HWND hwnd = dc_data->owner.hwnd;
        RECT client_rect;
        if(!hwnd)
        {
            ERR("Physical DC without a window!\n");
            return FALSE;
        }
        if(!GetClientRect(hwnd, &client_rect))
        {
            ERR("GetClientRect failed!\n");
            return FALSE;
        }

        /* This is a physical DC. Setup the hook */
        sw_ctx->hook = SetWindowsHookEx(WH_CALLWNDPROC,
                            sw_call_window_proc,
                            NULL,
                            GetCurrentThreadId());

        /* Calculate width & height */
        width  = client_rect.right  - client_rect.left;
        height = client_rect.bottom - client_rect.top;
    }
    else /* OBJ_MEMDC */
    {
        BITMAP bm;
        HBITMAP hbmp;
        HDC hdc = dc_data->owner.hdc;

        if(fb->gl_visual->DBflag)
        {
            ERR("Memory DC called with a double buffered format.\n");
            return FALSE;
        }

        hbmp = GetCurrentObject( hdc, OBJ_BITMAP );
        if(!hbmp)
        {
            ERR("No Bitmap!\n");
            return FALSE;
        }
        if(GetObject(hbmp, sizeof(bm), &bm) == 0)
        {
            ERR("GetObject failed!\n");
            return FALSE;
        }
        width = bm.bmWidth;
        height = bm.bmHeight;
    }

    if(!width) width = 1;
    if(!height) height = 1;

    /* Also make the mesa context current to mesa */
    gl_make_current(sw_ctx->gl_ctx, fb->gl_buffer);

    /* Setup our functions */
    setup_DD_pointers(sw_ctx->gl_ctx);

    /* Set the viewport if this is the first time we initialize this context */
    if(sw_ctx->gl_ctx->Viewport.X == 0 &&
       sw_ctx->gl_ctx->Viewport.Y == 0 &&
       sw_ctx->gl_ctx->Viewport.Width == 0 &&
       sw_ctx->gl_ctx->Viewport.Height == 0)
    {
        gl_Viewport(sw_ctx->gl_ctx, 0, 0, width, height);
    }

    /* update the framebuffer size */
    gl_ResizeBuffersMESA(sw_ctx->gl_ctx);

    /* Use our API table */
    IntSetCurrentDispatchTable(&sw_api_table.glDispatchTable);

   /* We're good */
   return TRUE;
}

void sw_ReleaseContext(DHGLRC dhglrc)
{
    struct sw_context* sw_ctx = (struct sw_context*)dhglrc;

    /* Forward to mesa */
    gl_make_current(NULL, NULL);

    /* Unhook */
    if(sw_ctx->hook)
    {
        UnhookWindowsHookEx(sw_ctx->hook);
        sw_ctx->hook = NULL;
    }
}

BOOL sw_SwapBuffers(HDC hdc, struct wgl_dc_data* dc_data)
{
    struct sw_framebuffer* fb = dc_data->sw_data;
    char Buffer[sizeof(BITMAPINFOHEADER) + 3 * sizeof(DWORD)];
    BITMAPINFO *bmi = (BITMAPINFO*)Buffer;
    BYTE Bpp = fb->pixel_format->cColorBits;

    if (!fb->gl_visual->DBflag)
        return TRUE;

    if (!fb->BackBuffer)
        return FALSE;

    bmi->bmiHeader.biSize = sizeof(bmi->bmiHeader);
    bmi->bmiHeader.biBitCount = Bpp;
    bmi->bmiHeader.biClrImportant = 0;
    bmi->bmiHeader.biClrUsed = 0;
    bmi->bmiHeader.biPlanes = 1;
    bmi->bmiHeader.biSizeImage = WIDTH_BYTES_ALIGN32(fb->width, Bpp) * fb->height;
    bmi->bmiHeader.biXPelsPerMeter = 0;
    bmi->bmiHeader.biYPelsPerMeter = 0;
    bmi->bmiHeader.biHeight = fb->height;
    bmi->bmiHeader.biWidth = fb->width;
    bmi->bmiHeader.biCompression = Bpp == 16 ? BI_BITFIELDS : BI_RGB;

    if (Bpp == 16)
    {
        DWORD* BitMasks = (DWORD*)(&bmi->bmiColors[0]);
        BitMasks[0] = 0x0000F800;
        BitMasks[1] = 0x000007E0;
        BitMasks[2] = 0x0000001F;
    }

    return SetDIBitsToDevice(fb->Hdc, 0, 0, fb->width, fb->height, 0, 0, 0, fb->height, fb->BackBuffer, bmi,
            fb->pixel_format->iPixelType == PFD_TYPE_COLORINDEX ? DIB_PAL_COLORS : DIB_RGB_COLORS) != 0;
}

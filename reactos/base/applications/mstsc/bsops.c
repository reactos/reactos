/* -*- c-basic-offset: 8 -*-
   rdesktop: A Remote Desktop Protocol client.
   Generics backingstore operations
   Copyright (C) Jay Sorg 2005-2006

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

//#include <stdlib.h>
//#include <string.h>
//#include "bsops.h"
#include "todo.h"

/* globals */
static char * g_bs = 0;
static int g_bs_size = 0;

static int g_width1 = 800;
static int g_height1 = 600;
static int g_bpp = 8;
static int g_Bpp = 1;

static int g_clip_left1 = 0;
static int g_clip_top1 = 0;
static int g_clip_right1 = 800;
static int g_clip_bottom1 = 600;

/* for bs_patblt */
static unsigned char g_hatch_patterns[] =
{
  0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00,  /* 0 - bsHorizontal */
  0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,  /* 1 - bsVertical */
  0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01,  /* 2 - bsFDiagonal */
  0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80,  /* 3 - bsBDiagonal */
  0x08, 0x08, 0x08, 0xff, 0x08, 0x08, 0x08, 0x08,  /* 4 - bsCross */
  0x81, 0x42, 0x24, 0x18, 0x18, 0x24, 0x42, 0x81   /* 5 - bsDiagCross */
};


/*****************************************************************************/
/* do a raster op */
int
bs_do_rop(int rop, int src, int dst)
{
  switch (rop)
  {
    case 0x0: return 0;
    case 0x1: return ~(src | dst);
    case 0x2: return (~src) & dst;
    case 0x3: return ~src;
    case 0x4: return src & (~dst);
    case 0x5: return ~(dst);
    case 0x6: return src ^ dst;
    case 0x7: return ~(src & dst);
    case 0x8: return src & dst;
    case 0x9: return ~(src) ^ dst;
    case 0xa: return dst;
    case 0xb: return (~src) | dst;
    case 0xc: return src;
    case 0xd: return src | (~dst);
    case 0xe: return src | dst;
    case 0xf: return ~0;
  }
  return dst;
}

/*****************************************************************************/
/* get a pixel from the in memory copy of whats on the screen */
int
bs_get_pixel(int x, int y)
{
  char * p;

  if (x >= 0 && x < g_width1 && y >= 0 && y < g_height1)
  {
    p = g_bs + (y * g_width1 * g_Bpp) + (x * g_Bpp);
    if (g_Bpp == 1)
    {
      return *((unsigned char *) p);
    }
    else if (g_Bpp == 2)
    {
      return *((unsigned short *) p);
    }
    else
    {
      return *((unsigned int *) p);
    }
  }
  else
  {
    return 0;
  }
}

/*****************************************************************************/
/* set a pixel on the screen using the clip */
void
bs_set_pixel(int x, int y, int pixel, int rop, int use_clip)
{
  char * p;

  if (!use_clip ||
        (x >= g_clip_left1 && x < g_clip_right1 &&
         y >= g_clip_top1 && y < g_clip_bottom1))
  {
    if (x >= 0 && x < g_width1 && y >= 0 && y < g_height1)
    {
      p = g_bs + (y * g_width1 * g_Bpp) + (x * g_Bpp);
      if (rop != 12)
      {
        pixel = bs_do_rop(rop, pixel, bs_get_pixel(x, y));
      }
      if (g_Bpp == 1)
      {
        *((unsigned char *) p) = pixel;
      }
      else if (g_Bpp == 2)
      {
        *((unsigned short *) p) = pixel;
      }
      else
      {
        *((unsigned int *) p) = pixel;
      }
    }
  }
}

/*****************************************************************************/
static char *
get_bs_ptr(int x, int y)
{
  char * p;

  if (x >= 0 && x < g_width1 && y >= 0 && y < g_height1)
  {
    p = g_bs + (y * g_width1 * g_Bpp) + (x * g_Bpp);
    return p;
  }
  else
  {
    return 0;
  }
}

/*****************************************************************************/
void
bs_init(int width, int height, int bpp)
{
  if (g_bs != 0)
  {
    free(g_bs);
  }
  g_width1 = width;
  g_height1 = height;
  g_bpp = bpp;
  g_Bpp = (bpp + 7) / 8;
  g_bs_size = width * height * g_Bpp;
  g_bs = malloc(g_bs_size);
  memset(g_bs, 0, g_bs_size);
  g_clip_left1 = 0;
  g_clip_top1 = 0;
  g_clip_right1 = width;
  g_clip_bottom1 = height;
}

/*****************************************************************************/
void
bs_exit(void)
{
  if (g_bs != 0)
  {
    free(g_bs);
  }
}

/*****************************************************************************/
void
bs_set_clip(int x, int y, int cx, int cy)
{
  g_clip_left1 = x;
  g_clip_top1 = y;
  g_clip_right1 = x + cx;
  g_clip_bottom1 = y + cy;
}

/*****************************************************************************/
void
bs_reset_clip(void)
{
  g_clip_left1 = 0;
  g_clip_top1 = 0;
  g_clip_right1 = g_width1;
  g_clip_bottom1 = g_height1;
}

/*****************************************************************************/
/* check if a certain pixel is set in a bitmap */
int
bs_is_pixel_on(char * data, int x, int y, int width, int bpp)
{
  int start;
  int shift;

  if (bpp == 1)
  {
    width = (width + 7) / 8;
    start = (y * width) + x / 8;
    shift = x % 8;
    return (data[start] & (0x80 >> shift)) != 0;
  }
  else if (bpp == 8)
  {
    return data[y * width + x] != 0;
  }
  else if (bpp == 24)
  {
    return data[(y * 3) * width + (x * 3)] != 0 &&
           data[(y * 3) * width + (x * 3) + 1] != 0 &&
           data[(y * 3) * width + (x * 3) + 2] != 0;
  }
  else
  {
    return 0;
  }
}

/*****************************************************************************/
void
bs_set_pixel_on(char * data, int x, int y, int width, int bpp,
                int pixel)
{
  int start;
  int shift;

  if (bpp == 1)
  {
    width = (width + 7) / 8;
    start = (y * width) + x / 8;
    shift = x % 8;
    if (pixel != 0)
    {
      data[start] = data[start] | (0x80 >> shift);
    }
    else
    {
      data[start] = data[start] & ~(0x80 >> shift);
    }
  }
  else if (bpp == 8)
  {
    data[y * width + x] = pixel;
  }
  else if (bpp == 15 || bpp == 16)
  {
    ((unsigned short *) data)[y * width + x] = pixel;
  }
}

/*****************************************************************************/
void
bs_copy_mem(char * d, char * s, int n)
{
  while (n & (~7))
  {
    *(d++) = *(s++);
    *(d++) = *(s++);
    *(d++) = *(s++);
    *(d++) = *(s++);
    *(d++) = *(s++);
    *(d++) = *(s++);
    *(d++) = *(s++);
    *(d++) = *(s++);
    n = n - 8;
  }
  while (n > 0)
  {
    *(d++) = *(s++);
    n--;
  }
}

/*****************************************************************************/
void
bs_copy_memb(char * d, char * s, int n)
{
  d = (d + n) - 1;
  s = (s + n) - 1;
  while (n & (~7))
  {
    *(d--) = *(s--);
    *(d--) = *(s--);
    *(d--) = *(s--);
    *(d--) = *(s--);
    *(d--) = *(s--);
    *(d--) = *(s--);
    *(d--) = *(s--);
    *(d--) = *(s--);
    n = n - 8;
  }
  while (n > 0)
  {
    *(d--) = *(s--);
    n--;
  }
}

/*****************************************************************************/
/* return true is the is something to draw */
int
bs_warp_coords(int * x, int * y, int * cx, int * cy,
               int * srcx, int * srcy)
{
  int dx;
  int dy;

  if (g_clip_left1 > *x)
  {
    dx = g_clip_left1 - *x;
  }
  else
  {
    dx = 0;
  }
  if (g_clip_top1 > *y)
  {
    dy = g_clip_top1 - *y;
  }
  else
  {
    dy = 0;
  }
  if (*x + *cx > g_clip_right1)
  {
    *cx = (*cx - ((*x + *cx) - g_clip_right1));
  }
  if (*y + *cy > g_clip_bottom1)
  {
    *cy = (*cy - ((*y + *cy) - g_clip_bottom1));
  }
  *cx = *cx - dx;
  *cy = *cy - dy;
  if (*cx <= 0)
  {
    return 0;
  }
  if (*cy <= 0)
  {
    return 0;
  }
  *x = *x + dx;
  *y = *y + dy;
  if (srcx != 0)
  {
    *srcx = *srcx + dx;
  }
  if (srcy != 0)
  {
    *srcy = *srcy + dy;
  }
  return 1;
}

/*****************************************************************************/
void
bs_rect(int x, int y, int cx, int cy, int colour, int rop)
{
  int i;
  int j;
  unsigned char * p8;
  unsigned short * p16;
  unsigned int * p32;

  if (bs_warp_coords(&x, &y, &cx, &cy, 0, 0))
  {
    if (rop == 0) /* black */
    {
      rop = 12;
      colour = 0;
    }
    else if (rop == 15) /* white */
    {
      rop = 12;
      colour = 0xffffff;
    }
    if (rop == 12) /* copy */
    {
      if (g_Bpp == 1)
      {
        for (i = 0; i < cy; i++)
        {
          p8 = (unsigned char *) get_bs_ptr(x, y + i);
          if (p8 != 0)
          {
            for (j = 0; j < cx; j++)
            {
              *p8 = colour;
              p8++;
            }
          }
        }
      }
      else if (g_Bpp == 2)
      {
        for (i = 0; i < cy; i++)
        {
          p16 = (unsigned short *) get_bs_ptr(x, y + i);
          if (p16 != 0)
          {
            for (j = 0; j < cx; j++)
            {
              *p16 = colour;
              p16++;
            }
          }
        }
      }
      else
      {
        for (i = 0; i < cy; i++)
        {
          p32 = (unsigned int *) get_bs_ptr(x, y + i);
          if (p32 != 0)
          {
            for (j = 0; j < cx; j++)
            {
              *p32 = colour;
              p32++;
            }
          }
        }
      }
    }
    else /* slow */
    {
      for (i = 0; i < cy; i++)
      {
        for (j = 0; j < cx; j++)
        {
          bs_set_pixel(j + x, i + y, colour, rop, 0);
        }
      }
    }
  }
}

/*****************************************************************************/
void
bs_screenblt(int rop, int x, int y, int cx, int cy,
             int srcx, int srcy)
{
  int p;
  int i;
  int j;
  char * src;
  char * dst;

  if (bs_warp_coords(&x, &y, &cx, &cy, &srcx, &srcy))
  {
    if (rop == 12) /* copy */
    {
      if (srcy < y) /* copy down - bottom to top */
      {
        for (i = cy - 1; i >= 0; i--)
        {
          src = get_bs_ptr(srcx, srcy + i);
          dst = get_bs_ptr(x, y + i);
          if (src != 0 && dst != 0)
          {
            bs_copy_mem(dst, src, cx * g_Bpp);
          }
        }
      }
      else if (srcy > y || srcx > x) /* copy up or left - top to bottom */
      {
        for (i = 0; i < cy; i++)
        {
          src = get_bs_ptr(srcx, srcy + i);
          dst = get_bs_ptr(x, y + i);
          if (src != 0 && dst != 0)
          {
            bs_copy_mem(dst, src, cx * g_Bpp);
          }
        }
      }
      else /* copy straight right */
      {
        for (i = 0; i < cy; i++)
        {
          src = get_bs_ptr(srcx, srcy + i);
          dst = get_bs_ptr(x, y + i);
          if (src != 0 && dst != 0)
          {
            bs_copy_memb(dst, src, cx * g_Bpp);
          }
        }
      }
    }
    else /* slow */
    {
      if (srcy < y) /* copy down - bottom to top */
      {
        for (i = cy - 1; i >= 0; i--)
        {
          for (j = 0; j < cx; j++)
          {
            p = bs_get_pixel(srcx + j, srcy + i);
            bs_set_pixel(x + j, y + i, p, rop, 0);
          }
        }
      }
      else if (srcy > y || srcx > x) /* copy up or left - top to bottom */
      {
        for (i = 0; i < cy; i++)
        {
          for (j = 0; j < cx; j++)
          {
            p = bs_get_pixel(srcx + j, srcy + i);
            bs_set_pixel(x + j, y + i, p, rop, 0);
          }
        }
      }
      else /* copy straight right */
      {
        for (i = 0; i < cy; i++)
        {
          for (j = cx - 1; j >= 0; j--)
          {
            p = bs_get_pixel(srcx + j, srcy + i);
            bs_set_pixel(x + j, y + i, p, rop, 0);
          }
        }
      }
    }
  }
}

/*****************************************************************************/
void
bs_memblt(int opcode, int x, int y, int cx, int cy,
          void * srcdata, int srcwidth, int srcheight,
          int srcx, int srcy)
{
  int i;
  int j;
  int p;
  char * dst;
  char * src;

  if (bs_warp_coords(&x, &y, &cx, &cy, &srcx, &srcy))
  {
    if (opcode == 12) /* copy */
    {
      if (g_Bpp == 1)
      {
        src = (char *) (((unsigned char *) srcdata) + srcy * srcwidth + srcx);
      }
      else if (g_Bpp == 2)
      {
        src = (char *) (((unsigned short *) srcdata) + srcy * srcwidth + srcx);
      }
      else
      {
        src = (char *) (((unsigned int *) srcdata) + srcy * srcwidth + srcx);
      }
      for (i = 0; i < cy; i++)
      {
        dst = get_bs_ptr(x, y + i);
        if (dst != 0)
        {
          bs_copy_mem(dst, src, cx * g_Bpp);
          src += srcwidth * g_Bpp;
        }
      }
    }
    else /* slow */
    {
      if (g_Bpp == 1)
      {
        for (i = 0; i < cy; i++)
        {
          for (j = 0; j < cx; j++)
          {
            p = *(((unsigned char *) srcdata) +
                         ((i + srcy) * srcwidth + (j + srcx)));
            bs_set_pixel(x + j, y + i, p, opcode, 0);
          }
        }
      }
      else if (g_Bpp == 2)
      {
        for (i = 0; i < cy; i++)
        {
          for (j = 0; j < cx; j++)
          {
            p = *(((unsigned short *) srcdata) +
                         ((i + srcy) * srcwidth + (j + srcx)));
            bs_set_pixel(x + j, y + i, p, opcode, 0);
          }
        }
      }
      else
      {
        for (i = 0; i < cy; i++)
        {
          for (j = 0; j < cx; j++)
          {
            p = *(((unsigned int *) srcdata) +
                         ((i + srcy) * srcwidth + (j + srcx)));
            bs_set_pixel(x + j, y + i, p, opcode, 0);
          }
        }
      }
    }
  }
}

/*****************************************************************************/
void
bs_draw_glyph(int x, int y, char * glyph_data, int glyph_width,
              int glyph_height, int fgcolour)
{
  int i;
  int j;

  for (i = 0; i < glyph_height; i++)
  {
    for (j = 0; j < glyph_width; j++)
    {
      if (bs_is_pixel_on(glyph_data, j, i, glyph_width, 8))
      {
        bs_set_pixel(x + j, y + i, fgcolour, 12, 1);
      }
    }
  }
}

/*****************************************************************************/
/* Bresenham's line drawing algorithm */
void
bs_line(int opcode, int startx, int starty, int endx, int endy,
        int pen_width, int pen_style, int pen_colour)
{
  int dx;
  int dy;
  int incx;
  int incy;
  int dpr;
  int dpru;
  int p;

  if (startx > endx)
  {
    dx = startx - endx;
    incx = -1;
  }
  else
  {
    dx = endx - startx;
    incx = 1;
  }
  if (starty > endy)
  {
    dy = starty - endy;
    incy = -1;
  }
  else
  {
    dy = endy - starty;
    incy = 1;
  }
  if (dx >= dy)
  {
    dpr = dy << 1;
    dpru = dpr - (dx << 1);
    p = dpr - dx;
    for (; dx >= 0; dx--)
    {
      if (startx != endx || starty != endy)
      {
        bs_set_pixel(startx, starty, pen_colour, opcode, 1);
      }
      if (p > 0)
      {
        startx += incx;
        starty += incy;
        p += dpru;
      }
      else
      {
        startx += incx;
        p += dpr;
      }
    }
  }
  else
  {
    dpr = dx << 1;
    dpru = dpr - (dy << 1);
    p = dpr - dy;
    for (; dy >= 0; dy--)
    {
      if (startx != endx || starty != endy)
      {
        bs_set_pixel(startx, starty, pen_colour, opcode, 1);
      }
      if (p > 0)
      {
        startx += incx;
        starty += incy;
        p += dpru;
      }
      else
      {
        starty += incy;
        p += dpr;
      }
    }
  }
}

/*****************************************************************************/
void
bs_patblt(int opcode, int x, int y, int cx, int cy,
          int brush_style, char * brush_pattern,
          int brush_x_org, int brush_y_org,
          int bgcolour, int fgcolour)
{
  int i;
  int j;
  char ipattern[8];
  char * b;

  b = 0;
  switch (brush_style)
  {
    case 0:
      bs_rect(x, y, cx, cy, fgcolour, opcode);
      break;
    case 2: /* Hatch */
      b = g_hatch_patterns + brush_pattern[0] * 8;
      break;
    case 3:
      for (i = 0; i < 8; i++)
      {
        ipattern[i] = ~brush_pattern[7 - i];
      }
      b = ipattern;
      break;
  }
  if (b != 0)
  {
    for (i = 0; i < cy; i++)
    {
      for (j = 0; j < cx; j++)
      {
        if (bs_is_pixel_on(b, (x + j + brush_x_org) % 8,
                           (y + i + brush_y_org) % 8, 8, 1))
        {
          bs_set_pixel(x + j, y + i, fgcolour, opcode, 1);
        }
        else
        {
          bs_set_pixel(x + j, y + i, bgcolour, opcode, 1);
        }
      }
    }
  }
}

/*****************************************************************************/
void
bs_copy_box(char * dst, int x, int y, int cx, int cy, int line_size)
{
  char * src;
  int i;

  /* shouldn't happen */
  if (cx < 1 || cy < 1)
  {
    return;
  }
  /* nothing to draw, memset and leave */
  if (x + cx < 0 || y + cy < 0 || x >= g_width1 || y >= g_height1)
  {
    memset(dst, 0, cx * cy * g_Bpp);
    return;
  }
  /* check if it goes over an edge */
  if (x < 0 || y < 0 || x + cx > g_width1 || y + cy > g_height1)
  {
    memset(dst, 0, cx * cy * g_Bpp);
    if (x < 0)
    {
      cx += x;
      dst += -x * g_Bpp;
      x = 0;
    }
    if (x + cx > g_width1)
    {
      cx = g_width1 - x;
    }
    for (i = 0; i < cy; i++)
    {
      src = get_bs_ptr(x, y + i);
      if (src != 0)
      {
        bs_copy_mem(dst, src, cx * g_Bpp);
      }
      dst += line_size;
    }
  }
  else /* whole box is within */
  {
    for (i = 0; i < cy; i++)
    {
      src = get_bs_ptr(x, y + i);
      if (src != 0)
      {
        bs_copy_mem(dst, src, cx * g_Bpp);
      }
      dst += line_size;
    }
  }
}


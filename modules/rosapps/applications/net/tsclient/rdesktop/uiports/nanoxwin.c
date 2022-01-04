/* -*- c-basic-offset: 8 -*-
   rdesktop: A Remote Desktop Protocol client.
   User interface services - NanoX(microwindows)
   Copyright (C) Jay Sorg 2004-2005

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/
/*
  problems with nanox lib
    opcodes don't work, can only rely on copy
    stipple orgins don't work
    clip seems to affect source too, it should only affect dest
      in copyarea functions
*/

#include "../rdesktop.h"

#include <stdarg.h>  /* va_list va_start va_end */
#include <unistd.h> /* gethostname */
#include <pwd.h> /* getpwuid */

#include <nano-X.h>

extern int g_tcp_port_rdp;
int g_use_rdp5 = 1;
char g_hostname[16];
char g_username[64];
int g_width = 800;
int g_height = 600;
int g_server_bpp = 16;
int g_encryption = 1;
int g_desktop_save = 0; /* todo */
int g_polygon_ellipse_orders = 0;
int g_bitmap_cache = 1;
int g_bitmap_cache_persist_enable = 0;
int g_bitmap_cache_precache = 1;
int g_bitmap_compression = 1;
uint32 g_rdp5_performanceflags =
  RDP5_NO_WALLPAPER | RDP5_NO_FULLWINDOWDRAG | RDP5_NO_MENUANIMATIONS;
int g_console_session = 0;
int g_keylayout = 0x409; /* Defaults to US keyboard layout */
int g_keyboard_type = 0x4; /* Defaults to US keyboard layout */
int g_keyboard_subtype = 0x0; /* Defaults to US keyboard layout */
int g_keyboard_functionkeys = 0xc; /* Defaults to US keyboard layout */

static int g_sck = 0;
static char g_servername[256] = "";
static char g_password[64] = "";
static char g_domain[64] = "";
static char g_shell[64] = "";
static char g_directory[64] = "";
static GR_WINDOW_ID g_wnd = 0;
static GR_GC_ID g_gc = 0;
static GR_GC_ID g_gc_clean = 0;
static int g_deactivated = 0;
static int g_ext_disc_reason = 0;
static GR_SCREEN_INFO g_screen_info;
static int g_bpp = 0;
static int g_Bpp = 0;
static GR_RECT g_clip; /* set in main */
static GR_CURSOR_ID g_null_cursor; /* set in main */
static int g_flags = RDP_LOGON_NORMAL;

struct key
{
  int ch1;
  int ch2;
  int ch3;
  int chs; /* shift char */
};

static struct key g_keys[256];

/* Session Directory redirection */
BOOL g_redirect = False;
char g_redirect_server[64];
char g_redirect_domain[16];
char g_redirect_password[64];
char g_redirect_username[64];
char g_redirect_cookie[128];
uint32 g_redirect_flags = 0;

#define COLOR16TO32(color) \
( \
  ((((color >> 8) & 0xf8) | ((color >> 13) & 0x7)) <<  0) | \
  ((((color >> 3) & 0xfc) | ((color >>  9) & 0x3)) <<  8) | \
  ((((color << 3) & 0xf8) | ((color >>  2) & 0x7)) << 16) \
)

static uint32 g_ops[16] =
{
  GR_MODE_CLEAR,         /* 0 */
  GR_MODE_NOR,           /* ~(src | dst) */
  GR_MODE_ANDINVERTED,   /* (~src) & dst */
  GR_MODE_COPYINVERTED,  /* ~src */
  GR_MODE_ANDREVERSE,    /* src & (~dst) */
  GR_MODE_INVERT,        /* ~(dst) */
  GR_MODE_XOR,           /* src ^ dst */
  GR_MODE_NAND,          /* ~(src & dst) */
  GR_MODE_AND,           /* src & dst */
  GR_MODE_EQUIV,         /* ~(src) ^ dst or is it ~(src ^ dst) */
  GR_MODE_NOOP,          /* dst */
  GR_MODE_ORINVERTED,    /* (~src) | dst */
  GR_MODE_COPY,          /* src */
  GR_MODE_ORREVERSE,     /* src | (~dst) */
  GR_MODE_OR,            /* src | dst */
  GR_MODE_SETTO1         /* ~0 */
};

/*****************************************************************************/
/* do a raster op */
static int rop(int rop, int src, int dst)
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
static int get_pixel32(uint8 * data, int x, int y,
                       int width, int height)
{
  if (x >= 0 && y >= 0 && x < width && y < height)
  {
    return *(((int*)data) + (y * width + x));
  }
  else
  {
    return 0;
  }
}

/*****************************************************************************/
static void set_pixel32(uint8 * data, int x, int y,
                        int width, int height, int pixel)
{
  if (x >= 0 && y >= 0 && x < width && y < height)
  {
    *(((int*)data) + (y * width + x)) = pixel;
  }
}

/*****************************************************************************/
static int warp_coords(int * x, int * y, int * cx, int * cy,
                       int * srcx, int * srcy)
{
  int dx;
  int dy;

  if (g_clip.x > *x)
  {
    dx = g_clip.x - *x;
  }
  else
  {
    dx = 0;
  }
  if (g_clip.y > *y)
  {
    dy = g_clip.y - *y;
  }
  else
  {
    dy = 0;
  }
  if (*x + *cx > g_clip.x + g_clip.width)
  {
    *cx = (*cx - ((*x + *cx) - (g_clip.x + g_clip.width)));
  }
  if (*y + *cy > g_clip.y + g_clip.height)
  {
    *cy = (*cy - ((*y + *cy) - (g_clip.y + g_clip.height)));
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

/******************************************************************************/
/* check if a certain pixel is set in a bitmap */
static int is_pixel_on(uint8 * data, int x, int y, int width, int bpp)
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
  else
    return 0;
}

/*****************************************************************************/
int ui_select(int in)
{
  if (g_sck == 0)
  {
    g_sck = in;
  }
  return 1;
}

/*****************************************************************************/
void ui_set_clip(int x, int y, int cx, int cy)
{
  GR_REGION_ID region;

  g_clip.x = x;
  g_clip.y = y;
  g_clip.width = cx;
  g_clip.height = cy;
  region = GrNewRegion();
  GrUnionRectWithRegion(region, &g_clip);
  GrSetGCRegion(g_gc, region); /* can't destroy region here, i guess gc */
                               /* takes owership, if you destroy it */
                               /* clip is reset, hum */
}

/*****************************************************************************/
void ui_reset_clip(void)
{
  GrSetGCRegion(g_gc, 0);
  g_clip.x = 0;
  g_clip.y = 0;
  g_clip.width = g_width;
  g_clip.height = g_height;
}

/*****************************************************************************/
void ui_bell(void)
{
  GrBell();
}

/*****************************************************************************/
/* gota convert the rdp glyph to nanox glyph */
void * ui_create_glyph(int width, int height, uint8 * data)
{
  char * p, * q, * r;
  int datasize, i, j;

  datasize = GR_BITMAP_SIZE(width, height) * sizeof(GR_BITMAP);
  p = xmalloc(datasize);
  q = p;
  r = data;
  memset(p, 0, datasize);
  for (i = 0; i < height; i++)
  {
    j = 0;
    while (j + 8 < width)
    {
      *q = *(r + 1);
      q++;
      r++;
      *q = *(r - 1);
      q++;
      r++;
      j += 16;
    }
    if ((width % 16) <= 8 && (width % 16) > 0)
    {
      q++;
      *q = *r;
      q++;
      r++;
      j += 8;
    }
  }
  return p;
}

/*****************************************************************************/
void ui_destroy_glyph(void * glyph)
{
  xfree(glyph);
}

/*****************************************************************************/
void * ui_create_colourmap(COLOURMAP * colors)
{
  return 0;
}

/*****************************************************************************/
void ui_set_colourmap(void * map)
{
}

/*****************************************************************************/
void * ui_create_bitmap(int width, int height, uint8 * data)
{
  GR_WINDOW_ID pixmap;
  uint8 * p;
  uint32 i, j, pixel;

  p = data;
  pixmap = GrNewPixmap(width, height, 0);
  if (g_server_bpp == 16 && g_bpp == 32)
  {
    p = xmalloc(width * height * g_Bpp);
    for (i = 0; i < height; i++)
    {
      for (j = 0; j < width; j++)
      {
        pixel = *(((uint16 *) data) + (i * width + j));
        pixel = COLOR16TO32(pixel);
        *(((uint32 *) p) + (i * width + j)) = pixel;
      }
    }
  }
  GrArea(pixmap, g_gc_clean, 0, 0, width, height, p, MWPF_RGB);
  if (p != data)
  {
    xfree(p);
  }
  return (void *) pixmap;
}

/*****************************************************************************/
void ui_destroy_bitmap(void * bmp)
{
  GrDestroyWindow((GR_WINDOW_ID)bmp);
}

/*****************************************************************************/
#define DO_GLYPH(ttext,idx) \
{ \
  glyph = cache_get_font (font, ttext[idx]); \
  if (!(flags & TEXT2_IMPLICIT_X)) \
  { \
    xyoffset = ttext[++idx]; \
    if ((xyoffset & 0x80)) \
    { \
      if (flags & TEXT2_VERTICAL) \
      { \
        y += ttext[idx+1] | (ttext[idx+2] << 8); \
      } \
      else \
      { \
        x += ttext[idx+1] | (ttext[idx+2] << 8); \
      } \
      idx += 2; \
    } \
    else \
    { \
      if (flags & TEXT2_VERTICAL) \
      { \
        y += xyoffset; \
      } \
      else \
      { \
        x += xyoffset; \
      } \
    } \
  } \
  if (glyph != NULL) \
  { \
    x1 = x + glyph->offset; \
    y1 = y + glyph->baseline; \
    GrBitmap(g_wnd, g_gc, x1, y1, glyph->width, glyph->height, glyph->pixmap); \
    if (flags & TEXT2_IMPLICIT_X) \
    { \
      x += glyph->width; \
    } \
  } \
}

/*****************************************************************************/
void ui_draw_text(uint8 font, uint8 flags, uint8 opcode, int mixmode,
                  int x, int y,
                  int clipx, int clipy, int clipcx, int clipcy,
                  int boxx, int boxy, int boxcx, int boxcy, BRUSH * brush,
                  int bgcolor, int fgcolor, uint8 * text, uint8 length)
{
  FONTGLYPH * glyph;
  int i, j, xyoffset, x1, y1;
  DATABLOB * entry;

  GrSetGCMode(g_gc, GR_MODE_COPY);
  GrSetGCUseBackground(g_gc, 0); /* this can be set when gc is created */
  if (g_server_bpp == 16 && g_bpp == 32)
  {
    fgcolor = COLOR16TO32(fgcolor);
    bgcolor = COLOR16TO32(bgcolor);
  }
  GrSetGCForeground(g_gc, bgcolor);
  if (boxx + boxcx > g_width)
  {
    boxcx = g_width - boxx;
  }
  if (boxcx > 1)
  {
    GrFillRect(g_wnd, g_gc, boxx, boxy, boxcx, boxcy);
  }
  else if (mixmode == MIX_OPAQUE)
  {
    GrFillRect(g_wnd, g_gc, clipx, clipy, clipcx, clipcy);
  }
  GrSetGCForeground(g_gc, fgcolor);
  /* Paint text, character by character */
  for (i = 0; i < length;)
  {
    switch (text[i])
    {
      case 0xff:
        if (i + 2 < length)
        {
          cache_put_text(text[i + 1], text, text[i + 2]);
        }
        else
        {
          error("this shouldn't be happening\n");
          exit(1);
        }
        /* this will move pointer from start to first character after */
        /* FF command */
        length -= i + 3;
        text = &(text[i + 3]);
        i = 0;
        break;
      case 0xfe:
        entry = cache_get_text(text[i + 1]);
        if (entry != NULL)
        {
          if ((((uint8 *) (entry->data))[1] == 0) &&
                                (!(flags & TEXT2_IMPLICIT_X)))
          {
            if (flags & TEXT2_VERTICAL)
            {
              y += text[i + 2];
            }
            else
            {
              x += text[i + 2];
            }
          }
          for (j = 0; j < entry->size; j++)
          {
            DO_GLYPH(((uint8 *) (entry->data)), j);
          }
        }
        if (i + 2 < length)
        {
          i += 3;
        }
        else
        {
          i += 2;
        }
        length -= i;
        /* this will move pointer from start to first character after */
        /* FE command */
        text = &(text[i]);
        i = 0;
        break;
      default:
        DO_GLYPH(text, i);
        i++;
        break;
    }
  }
}

/*****************************************************************************/
void ui_line(uint8 opcode, int startx, int starty, int endx, int endy,
             PEN * pen)
{
  uint32 op;
  uint32 color;

  color = pen->colour;
  if (opcode == 5) /* GR_MODE_INVERT, not supported so convert it */
  {                /* i think x ^ -1 = ~x */
    color = 0xffffffff;
    opcode = 6; /* GR_MODE_XOR */
  }
  if (opcode == 12 || opcode == 6) /* nanox only supports these 2 opcode */
  {
    op = g_ops[opcode];
    GrSetGCMode(g_gc, op);
    if (g_server_bpp == 16 && g_bpp == 32)
    {
      color = COLOR16TO32(color);
    }
    GrSetGCForeground(g_gc, color);
    GrLine(g_wnd, g_gc, startx, starty, endx, endy);
    GrSetGCMode(g_gc, GR_MODE_COPY);
  }
  else
  {
    unimpl("opcode %d in ui_line\n", opcode);
  }
}

/*****************************************************************************/
void ui_triblt(uint8 opcode, int x, int y, int cx, int cy,
               void * src, int srcx, int srcy,
               BRUSH * brush, int bgcolor, int fgcolor)
{
/* not used, turned off */
}

/*****************************************************************************/
void ui_memblt(uint8 opcode, int x, int y, int cx, int cy,
               void * src, int srcx, int srcy)
{
  uint8 * dest;
  uint8 * source;
  uint8 * final;
  GR_WINDOW_INFO wi;
  int i, j, s, d;
  GR_WINDOW_ID pixmap;

  if (opcode == 12)
  {
    GrCopyArea(g_wnd, g_gc, x, y, cx, cy, (GR_DRAW_ID)src, srcx, srcy,
               GR_MODE_COPY);
  }
  else /* do opcodes ourself */
  {    /* slow but its correct, ok to be slow here, these are rare */
    GrGetWindowInfo((GR_DRAW_ID)src, &wi);
    dest = xmalloc(cx * cy * g_Bpp);
    source = xmalloc(wi.width * wi.height * g_Bpp);
    final = xmalloc(cx * cy * g_Bpp);
    memset(final, 0, cx * cy * g_Bpp);
    /* dest */
    GrReadArea(g_wnd, x, y, cx, cy, (GR_PIXELVAL*)dest);
    /* source */
    GrReadArea((GR_DRAW_ID)src, 0, 0,
               wi.width, wi.height, (GR_PIXELVAL*)source);
    for (i = 0; i < cy; i++)
    {
      for (j = 0; j < cx; j++)
      {
        s = get_pixel32(source, j + srcx, i + srcy, wi.width, wi.height);
        d = get_pixel32(dest, j, i, cx ,cy);
        set_pixel32(final, j, i, cx, cy, rop(opcode, s, d));
      }
    }
    pixmap = GrNewPixmap(cx, cy, 0);
    GrArea(pixmap, g_gc_clean, 0, 0, cx, cy, final, MWPF_TRUECOLOR0888);
    GrCopyArea(g_wnd, g_gc, x, y, cx, cy, pixmap, 0, 0, GR_MODE_COPY);
    GrDestroyWindow(pixmap);
    xfree(dest);
    xfree(source);
    xfree(final);
  }
}

/*****************************************************************************/
void ui_desktop_restore(uint32 offset, int x, int y, int cx, int cy)
{
/* not used, turned off */
}

/*****************************************************************************/
void ui_desktop_save(uint32 offset, int x, int y, int cx, int cy)
{
/* not used, turned off */
}

/*****************************************************************************/
void ui_rect(int x, int y, int cx, int cy, int color)
{
  if (g_server_bpp == 16 && g_bpp == 32)
  {
    color = COLOR16TO32(color);
  }
  GrSetGCForeground(g_gc, color);
  GrFillRect(g_wnd, g_gc, x, y, cx, cy);
}

/*****************************************************************************/
/* using warp_coords cause clip seems to affect source in GrCopyArea */
void ui_screenblt(uint8 opcode, int x, int y, int cx, int cy,
                  int srcx, int srcy)
{
  if (opcode == 12)
  {
    if (warp_coords(&x, &y, &cx, &cy, &srcx, &srcy))
    {
      GrCopyArea(g_wnd, g_gc_clean, x, y, cx, cy, g_wnd, srcx, srcy,
                 GR_MODE_COPY);
    }
  }
  else
  {
    unimpl("opcode %d in ui_screenblt\n", opcode);
  }
}

/******************************************************************************/
/* can't use stipple cause tsorigin don't work, GrPoint too slow,
   GrPoints too slow but better, using a copy from the screen,
   do the pattern and copy it back */
void ui_patblt(uint8 opcode, int x, int y, int cx, int cy,
               BRUSH * brush, int bgcolor, int fgcolor)
{
  uint8 ipattern[8], * dest, * final;
  uint32 op;
  int i, j, s, d;
  GR_WINDOW_ID pixmap;

  if (g_server_bpp == 16 && g_bpp == 32)
  {
    fgcolor = COLOR16TO32(fgcolor);
    bgcolor = COLOR16TO32(bgcolor);
  }
  switch (brush->style)
  {
    case 0: /* Solid */
      if (opcode == 12 || opcode == 6)
      {
        op = g_ops[opcode];
        GrSetGCMode(g_gc, op);
        GrSetGCForeground(g_gc, fgcolor);
        GrFillRect(g_wnd, g_gc, x, y, cx, cy);
        GrSetGCMode(g_gc, GR_MODE_COPY);
      }
      else
      {
        unimpl("opcode %d in ui_patblt solid brush\n", opcode);
      }
      break;
    case 3: /* Pattern - all opcodes ok */
      for (i = 0; i != 8; i++)
      {
        ipattern[7 - i] = brush->pattern[i];
      }
      dest = xmalloc(cx * cy * g_Bpp);
      final = xmalloc(cx * cy * g_Bpp);
      memset(final, 0, cx * cy * g_Bpp);
      /* dest */
      if (opcode != 12)
      {
        GrReadArea(g_wnd, x, y, cx, cy, (GR_PIXELVAL*)dest);
      }
      for (i = 0; i < cy; i++)
      {
        for (j = 0; j < cx; j++)
        {
          if (is_pixel_on(ipattern, (x + j + brush->xorigin) % 8,
                          (y + i + brush->yorigin) % 8, 8, 1))
          {
            s = fgcolor;
          }
          else
          {
            s = bgcolor;
          }
          d = get_pixel32(dest, j, i, cx ,cy);
          set_pixel32(final, j, i, cx, cy, rop(opcode, s, d));
        }
      }
      pixmap = GrNewPixmap(cx, cy, 0);
      GrArea(pixmap, g_gc_clean, 0, 0, cx, cy, final, MWPF_TRUECOLOR0888);
      GrCopyArea(g_wnd, g_gc, x, y, cx, cy, pixmap, 0, 0, GR_MODE_COPY);
      GrDestroyWindow(pixmap);
      xfree(dest);
      xfree(final);
      break;
  }
}

/*****************************************************************************/
void ui_destblt(uint8 opcode, int x, int y, int cx, int cy)
{
  uint32 op;

  if (opcode == 0) /* black */
  {
    GrSetGCForeground(g_gc, 0);
    opcode = 12;
  }
  else if (opcode == 5) /* invert */
  {
    GrSetGCForeground(g_gc, 0xffffffff);
    opcode = 6;
  }
  else if (opcode == 15) /* white */
  {
    GrSetGCForeground(g_gc, 0xffffffff);
    opcode = 12;
  }
  if (opcode == 12 || opcode == 6)
  {
    op = g_ops[opcode];
    GrSetGCMode(g_gc, op);
    GrFillRect(g_wnd, g_gc, x, y, cx, cy);
    GrSetGCMode(g_gc, GR_MODE_COPY);
  }
  else
  {
    unimpl("opcode %d in ui_destblt\n", opcode);
  }
}

/*****************************************************************************/
void ui_paint_bitmap(int x, int y, int cx, int cy,
                     int width, int height, uint8 * data)
{
  void * b;

  b = ui_create_bitmap(width, height, data);
  ui_memblt(12, x, y, cx, cy, b, 0, 0);
  ui_destroy_bitmap(b);
}

/*****************************************************************************/
void ui_move_pointer(int x, int y)
{
  GrMoveCursor(x, y);
}

/*****************************************************************************/
void ui_set_null_cursor(void)
{
  GrSetWindowCursor(g_wnd, g_null_cursor);
}

/*****************************************************************************/
void ui_set_cursor(void * cursor)
{
  GrSetWindowCursor(g_wnd, (GR_CURSOR_ID)cursor);
}

//******************************************************************************
static int is24on(uint8 * data, int x, int y)
{
  uint8 r, g, b;
  int start;

  if (data == 0)
  {
    return 0;
  }
  start = y * 32 * 3 + x * 3;
  r = data[start];
  g = data[start + 1];
  b = data[start + 2];
  return !((r == 0) && (g == 0) && (b == 0));
}

//******************************************************************************
static int is1on(uint8 * data, int x, int y)
{
  int start;
  int shift;

  if (data == 0)
  {
    return 0;
  }
  start = (y * 32) / 8 + x / 8;
  shift = x % 8;
  return (data[start] & (0x80 >> shift)) == 0;
}

//******************************************************************************
static void set1(uint8 * data, int x, int y)
{
  int start;
  int shift;

  if (data == 0)
  {
    return;
  }
  start = (y * 32) / 8 + x / 8;
  shift = x % 8;
  data[start] = data[start] | (0x80 >> shift);
}

//******************************************************************************
static void flipover(uint8 * data)
{
  uint8 adata[128];
  int index;

  if (data == 0)
  {
    return;
  }
  memcpy(adata, data, 128);
  for (index = 0; index <= 31; index++)
  {
    data[127 - (index * 4 + 3)] = adata[index * 4];
    data[127 - (index * 4 + 2)] = adata[index * 4 + 1];
    data[127 - (index * 4 + 1)] = adata[index * 4 + 2];
    data[127 - index * 4] = adata[index * 4 + 3];
  }
}

/*****************************************************************************/
void * ui_create_cursor(uint32 x, uint32 y,
                        int width, int height,
                        uint8 * andmask, uint8 * xormask)
{
  uint8 adata[128];
  uint8 amask[128];
  GR_BITMAP * databitmap;
  GR_BITMAP * maskbitmap;
  GR_CURSOR_ID cursor;
  int i1, i2, bon, mon;

  if (width != 32 || height != 32)
  {
    return 0;
  }
  memset(adata, 0, 128);
  memset(amask, 0, 128);
  for (i1 = 0; i1 <= 31; i1++)
  {
    for (i2 = 0; i2 <= 31; i2++)
    {
      mon = is24on(xormask, i1, i2);
      bon = is1on(andmask, i1, i2);
      if (bon ^ mon) // xor
      {
        set1(adata, i1, i2);
        if (!mon)
        {
          set1(amask, i1, i2);
        }
      }
      if (mon)
      {
        set1(amask, i1, i2);
      }
    }
  }
  flipover(adata);
  flipover(amask);
  databitmap = ui_create_glyph(32, 32, adata);
  maskbitmap = ui_create_glyph(32, 32, amask);
  cursor = GrNewCursor(32, 32, x, y, 0xffffff, 0, databitmap, maskbitmap);
  ui_destroy_glyph(databitmap);
  ui_destroy_glyph(maskbitmap);
  return (void*)cursor;
}

/*****************************************************************************/
void ui_destroy_cursor(void * cursor)
{
  GrDestroyCursor((GR_CURSOR_ID)cursor);
}

/*****************************************************************************/
uint16 ui_get_numlock_state(uint32 state)
{
  return 0;
}

/*****************************************************************************/
uint32 read_keyboard_state(void)
{
  return 0;
}

/*****************************************************************************/
void ui_resize_window(void)
{
}

/*****************************************************************************/
void ui_begin_update(void)
{
}

/*****************************************************************************/
void ui_end_update(void)
{
}

/*****************************************************************************/
void ui_polygon(uint8 opcode, uint8 fillmode, POINT * point, int npoints,
                BRUSH * brush, int bgcolor, int fgcolor)
{
/* not used, turned off */
}

/*****************************************************************************/
void ui_polyline(uint8 opcode, POINT * points, int npoints, PEN * pen)
{
  int i, x, y, dx, dy;

  if (npoints > 0)
  {
    x = points[0].x;
    y = points[0].y;
    for (i = 1; i < npoints; i++)
    {
      dx = points[i].x;
      dy = points[i].y;
      ui_line(opcode, x, y, x + dx, y + dy, pen);
      x = x + dx;
      y = y + dy;
    }
  }
}

/*****************************************************************************/
void ui_ellipse(uint8 opcode, uint8 fillmode,
                int x, int y, int cx, int cy,
                BRUSH * brush, int bgcolor, int fgcolor)
{
/* not used, turned off */
}

/*****************************************************************************/
void generate_random(uint8 * random)
{
  memcpy(random, "12345678901234567890123456789012", 32);
}

/*****************************************************************************/
void save_licence(uint8 * data, int length)
{
}

/*****************************************************************************/
int load_licence(uint8 ** data)
{
  return 0;
}

/*****************************************************************************/
void * xrealloc(void * in, int size)
{
  if (size < 1)
  {
    size = 1;
  }
  return realloc(in, size);
}

/*****************************************************************************/
void * xmalloc(int size)
{
  return malloc(size);
}

/*****************************************************************************/
void xfree(void * in)
{
  if (in != 0)
  {
    free(in);
  }
}

/*****************************************************************************/
char * xstrdup(const char * s)
{
  char * mem = strdup(s);
  if (mem == NULL)
  {
    perror("strdup");
    exit(1);
  }
  return mem;
}

/*****************************************************************************/
void warning(char * format, ...)
{
  va_list ap;

  fprintf(stderr, "WARNING: ");
  va_start(ap, format);
  vfprintf(stderr, format, ap);
  va_end(ap);
}

/*****************************************************************************/
void unimpl(char * format, ...)
{
  va_list ap;

  fprintf(stderr, "NOT IMPLEMENTED: ");
  va_start(ap, format);
  vfprintf(stderr, format, ap);
  va_end(ap);
}

/*****************************************************************************/
void error(char * format, ...)
{
  va_list ap;

  fprintf(stderr, "ERROR: ");
  va_start(ap, format);
  vfprintf(stderr, format, ap);
  va_end(ap);
}

/*****************************************************************************/
int rd_pstcache_mkdir(void)
{
  return 0;
}

/*****************************************************************************/
int rd_open_file(char * filename)
{
  return 0;
}

/*****************************************************************************/
void rd_close_file(int fd)
{
  return;
}

/*****************************************************************************/
int rd_read_file(int fd, void * ptr, int len)
{
  return 0;
}

/*****************************************************************************/
int rd_write_file(int fd, void * ptr, int len)
{
  return 0;
}

/*****************************************************************************/
int rd_lseek_file(int fd, int offset)
{
  return 0;
}

/*****************************************************************************/
int rd_lock_file(int fd, int start, int len)
{
  return False;
}

/*****************************************************************************/
/*static int key(int ch, int flags)
{
  return (ch & 0xffff) | ((flags & 0xffff) << 16);
}*/

/*****************************************************************************/
static void init_keys(void)
{
  memset(&g_keys, 0, sizeof(g_keys));
  g_keys[0x01].ch1 = 27; /* esc */
  g_keys[0x02].ch1 = '1';
  g_keys[0x02].chs = '!';
  g_keys[0x03].ch1 = '2';
  g_keys[0x03].chs = '@';
  g_keys[0x04].ch1 = '3';
  g_keys[0x04].chs = '#';
  g_keys[0x05].ch1 = '4';
  g_keys[0x05].chs = '$';
  g_keys[0x06].ch1 = '5';
  g_keys[0x06].chs = '%';
  g_keys[0x07].ch1 = '6';
  g_keys[0x07].chs = '^';
  g_keys[0x08].ch1 = '7';
  g_keys[0x08].chs = '&';
  g_keys[0x09].ch1 = '8';
  g_keys[0x09].chs = '*';
  g_keys[0x0a].ch1 = '9';
  g_keys[0x0a].chs = '(';
  g_keys[0x0b].ch1 = '0';
  g_keys[0x0b].chs = ')';
  g_keys[0x0c].ch1 = '-';
  g_keys[0x0c].chs = '_';
  g_keys[0x0d].ch1 = '=';
  g_keys[0x0d].chs = '+';
  g_keys[0x0e].ch1 = 8; /* backspace */
  g_keys[0x0f].ch1 = 9; /* tab */
  g_keys[0x10].ch1 = 'q';
  g_keys[0x10].chs = 'Q';
  g_keys[0x11].ch1 = 'w';
  g_keys[0x11].chs = 'W';
  g_keys[0x12].ch1 = 'e';
  g_keys[0x12].chs = 'E';
  g_keys[0x13].ch1 = 'r';
  g_keys[0x13].chs = 'R';
  g_keys[0x14].ch1 = 't';
  g_keys[0x14].chs = 'T';
  g_keys[0x15].ch1 = 'y';
  g_keys[0x15].chs = 'Y';
  g_keys[0x16].ch1 = 'u';
  g_keys[0x16].chs = 'U';
  g_keys[0x17].ch1 = 'i';
  g_keys[0x17].chs = 'I';
  g_keys[0x18].ch1 = 'o';
  g_keys[0x18].chs = 'O';
  g_keys[0x19].ch1 = 'p';
  g_keys[0x19].chs = 'P';
  g_keys[0x1a].ch1 = '[';
  g_keys[0x1a].chs = '{';
  g_keys[0x1b].ch1 = ']';
  g_keys[0x1b].chs = '}';
  g_keys[0x1c].ch2 = 13; /* enter */
  g_keys[0x1d].ch1 = 63533; /* left control */
  g_keys[0x1d].ch2 = 63534; /* right control */
  g_keys[0x1e].ch1 = 'a';
  g_keys[0x1e].chs = 'A';
  g_keys[0x1f].ch1 = 's';
  g_keys[0x1f].chs = 'S';
  g_keys[0x20].ch1 = 'd';
  g_keys[0x20].chs = 'D';
  g_keys[0x21].ch1 = 'f';
  g_keys[0x21].chs = 'F';
  g_keys[0x22].ch1 = 'g';
  g_keys[0x22].chs = 'G';
  g_keys[0x23].ch1 = 'h';
  g_keys[0x23].chs = 'H';
  g_keys[0x24].ch1 = 'j';
  g_keys[0x24].chs = 'J';
  g_keys[0x25].ch1 = 'k';
  g_keys[0x25].chs = 'K';
  g_keys[0x26].ch1 = 'l';
  g_keys[0x26].chs = 'L';
  g_keys[0x27].ch1 = ';';
  g_keys[0x27].chs = ':';
  g_keys[0x28].ch1 = '\'';
  g_keys[0x28].chs = '"';
  g_keys[0x29].ch1 = '`';
  g_keys[0x29].chs = '~';
  g_keys[0x2a].ch1 = 63531; /* left shift */
  g_keys[0x2b].ch1 = '\\';
  g_keys[0x2c].ch1 = 'z';
  g_keys[0x2c].chs = 'Z';
  g_keys[0x2d].ch1 = 'x';
  g_keys[0x2d].chs = 'X';
  g_keys[0x2e].ch1 = 'c';
  g_keys[0x2e].chs = 'C';
  g_keys[0x2f].ch1 = 'v';
  g_keys[0x2f].chs = 'V';
  g_keys[0x30].ch1 = 'b';
  g_keys[0x30].chs = 'B';
  g_keys[0x31].ch1 = 'n';
  g_keys[0x31].chs = 'N';
  g_keys[0x32].ch1 = 'm';
  g_keys[0x32].chs = 'M';
  g_keys[0x33].ch1 = ',';
  g_keys[0x33].chs = '<';
  g_keys[0x34].ch1 = '.';
  g_keys[0x34].chs = '>';
  g_keys[0x35].ch1 = '/';
  g_keys[0x35].ch2 = 63509;
  g_keys[0x35].chs = '?';
  g_keys[0x36].ch1 = 63532; /* right shift */
  g_keys[0x37].ch1 = '*'; /* star on keypad */
  g_keys[0x37].ch2 = 63510; /* star on keypad */
  g_keys[0x38].ch1 = 63535; /* alt */
  g_keys[0x38].ch2 = 63536; /* alt */
  g_keys[0x39].ch1 = ' ';
  g_keys[0x3a].ch1 = 0; /* caps lock */
  g_keys[0x3b].ch1 = 63515; /* f1 */
  g_keys[0x3c].ch1 = 63516; /* f2 */
  g_keys[0x3d].ch1 = 63517; /* f3 */
  g_keys[0x3e].ch1 = 63518; /* f4 */
  g_keys[0x3f].ch1 = 63519; /* f5 */
  g_keys[0x40].ch1 = 63520; /* f6 */
  g_keys[0x41].ch1 = 63521; /* f7 */
  g_keys[0x42].ch1 = 63522; /* f8 */
  g_keys[0x43].ch1 = 63523; /* f9 */
  g_keys[0x44].ch1 = 63524; /* f10 */
  g_keys[0x45].ch1 = 0; /* num lock */
  g_keys[0x46].ch1 = 0; /* scroll lock */
  g_keys[0x47].ch1 = 63505; /* home */
  g_keys[0x47].ch2 = 63494; /* home */
  g_keys[0x48].ch1 = 63490; /* arrow up */
  g_keys[0x48].ch2 = 63506; /* arrow up */
  g_keys[0x49].ch1 = 63507; /* page up */
  g_keys[0x49].ch2 = 63496; /* page up */
  g_keys[0x4a].ch1 = '-'; /* -(minus) on keypad */
  g_keys[0x4a].ch2 = 63511; /* -(minus) on keypad */
  g_keys[0x4b].ch1 = 63502; /* arrow left */
  g_keys[0x4b].ch2 = 63488; /* arrow left */
  g_keys[0x4c].ch1 = 63503; /* middle(5 key) on keypad */
  g_keys[0x4d].ch1 = 63504; /* arrow right */
  g_keys[0x4d].ch2 = 63489; /* arrow right */
  g_keys[0x4e].ch1 = '+'; /* +(plus) on keypad */
  g_keys[0x4e].ch2 = 63512; /* +(plus) on keypad */
  g_keys[0x4f].ch1 = 63499; /* end */
  g_keys[0x4f].ch2 = 63495; /* end */
  g_keys[0x50].ch1 = 63500; /* arrow down */
  g_keys[0x50].ch2 = 63491; /* arrow down */
  g_keys[0x51].ch1 = 63501; /* page down */
  g_keys[0x51].ch2 = 63497; /* page down */
  g_keys[0x52].ch1 = 63498; /* insert */
  g_keys[0x52].ch2 = 63492; /* insert */
  g_keys[0x53].ch1 = 63508; /* delete */
  g_keys[0x53].ch2 = 63493; /* delete */
  g_keys[0x54].ch1 = 63525; /* f11 */
  g_keys[0x55].ch1 = 63527; /* f12 */
}

/*****************************************************************************/
/* returns 0 if found key */
static int get_sc(GR_EVENT_KEYSTROKE * event_keystroke, int * sc, int * ec)
{
  int i;

  //printf("%d %d\n", event_keystroke->ch, event_keystroke->modifiers);
  *sc = 0;
  *ec = 0;
  for (i = 0; i < 256; i++)
  {
    if (event_keystroke->modifiers & 1) /* shift is down */
    {
      if (event_keystroke->ch == g_keys[i].chs)
      {
        *sc = i;
        break;
      }
    }
    if (event_keystroke->ch == g_keys[i].ch1 ||
        event_keystroke->ch == g_keys[i].ch2 ||
        event_keystroke->ch == g_keys[i].ch3)
    {
      *sc = i;
      break;
    }
  }
  if (*sc == 0)
  {
    return 1;
  }
  else
  {
    return 0;
  }
}

/*****************************************************************************/
void static process_keystroke(GR_EVENT_KEYSTROKE * event_keystroke, int down)
{
  int sc, ec;

  if (get_sc(event_keystroke, &sc, &ec) == 0)
  {
    if (down)
    {
      rdp_send_input(0, RDP_INPUT_SCANCODE, RDP_KEYPRESS, sc, ec);
    }
    else
    {
      rdp_send_input(0, RDP_INPUT_SCANCODE, RDP_KEYRELEASE, sc, ec);
    }
  }
}

/*****************************************************************************/
void nanox_event(GR_EVENT * ev)
{
  GR_EVENT_MOUSE * event_mouse;
  GR_EVENT_BUTTON * event_button;
  GR_EVENT_FDINPUT * event_fdinput;
  GR_EVENT_KEYSTROKE * event_keystroke;

  do
  {
    if (ev->type == GR_EVENT_TYPE_FDINPUT) /* 12 */
    {
      event_fdinput = (GR_EVENT_FDINPUT *) ev;
      if (event_fdinput->fd == g_sck)
      {
        if (!rdp_loop(&g_deactivated, &g_ext_disc_reason))
        {
          fprintf(stderr, "rdp_loop in nanox_event exit codes %d %d\n",
                  g_deactivated, g_ext_disc_reason);
          exit(1);
        }
      }
    }
    else if (ev->type == GR_EVENT_TYPE_BUTTON_DOWN) /* 2 */
    {
      event_button = (GR_EVENT_BUTTON *) ev;
      if (event_button->changebuttons & 4) /* left */
      {
        rdp_send_input(0, RDP_INPUT_MOUSE, MOUSE_FLAG_DOWN | MOUSE_FLAG_BUTTON1,
                       event_button->x, event_button->y);
      }
      else if (event_button->changebuttons & 1) /* right */
      {
        rdp_send_input(0, RDP_INPUT_MOUSE, MOUSE_FLAG_DOWN | MOUSE_FLAG_BUTTON2,
                       event_button->x, event_button->y);
      }
    }
    else if (ev->type == GR_EVENT_TYPE_BUTTON_UP) /* 3 */
    {
      event_button = (GR_EVENT_BUTTON *) ev;
      if (event_button->changebuttons & 4) /* left */
      {
        rdp_send_input(0, RDP_INPUT_MOUSE, MOUSE_FLAG_BUTTON1,
                       event_button->x, event_button->y);
      }
      else if (event_button->changebuttons & 1) /* right */
      {
        rdp_send_input(0, RDP_INPUT_MOUSE, MOUSE_FLAG_BUTTON2,
                       event_button->x, event_button->y);
      }
    }
    else if (ev->type == GR_EVENT_TYPE_MOUSE_MOTION) /* 6 */
    {
      event_mouse = (GR_EVENT_MOUSE *) ev;
      rdp_send_input(0, RDP_INPUT_MOUSE, MOUSE_FLAG_MOVE,
                     event_mouse->x, event_mouse->y);
    }
    else if (ev->type == GR_EVENT_TYPE_MOUSE_POSITION) /* 7 */
    {
      /* use GR_EVENT_TYPE_MOUSE_MOTION */
    }
    else if (ev->type == GR_EVENT_TYPE_KEY_DOWN) /* 8 */
    {
      event_keystroke = (GR_EVENT_KEYSTROKE *) ev;
      process_keystroke(event_keystroke, 1);
    }
    else if (ev->type == GR_EVENT_TYPE_KEY_UP) /* 9 */
    {
      event_keystroke = (GR_EVENT_KEYSTROKE *) ev;
      process_keystroke(event_keystroke, 0);
    }
    else if (ev->type == GR_EVENT_TYPE_FOCUS_IN) /* 10 */
    {
    }
    else if (ev->type == GR_EVENT_TYPE_FOCUS_OUT) /* 11 */
    {
    }
    else if (ev->type == GR_EVENT_TYPE_UPDATE) /* 13 */
    {
    }
    GrCheckNextEvent(ev);
  } while (ev->type != GR_EVENT_TYPE_NONE);
}

/*****************************************************************************/
static void get_username_and_hostname(void)
{
  char fullhostname[64];
  char * p;
  struct passwd * pw;

  STRNCPY(g_username, "unknown", sizeof(g_username));
  STRNCPY(g_hostname, "unknown", sizeof(g_hostname));
  pw = getpwuid(getuid());
  if (pw != NULL && pw->pw_name != NULL)
  {
    STRNCPY(g_username, pw->pw_name, sizeof(g_username));
  }
  if (gethostname(fullhostname, sizeof(fullhostname)) != -1)
  {
    p = strchr(fullhostname, '.');
    if (p != NULL)
    {
      *p = 0;
    }
    STRNCPY(g_hostname, fullhostname, sizeof(g_hostname));
  }
}
/*****************************************************************************/
static void out_params(void)
{
  fprintf(stderr, "rdesktop: A Remote Desktop Protocol client.\n");
  fprintf(stderr, "Version " VERSION ". Copyright (C) 1999-2005 Matt Chapman.\n");
  fprintf(stderr, "nanox uiport by Jay Sorg\n");
  fprintf(stderr, "See http://www.rdesktop.org/ for more information.\n\n");
  fprintf(stderr, "Usage: nanoxrdesktop [options] server\n");
  fprintf(stderr, "   -u: user name\n");
  fprintf(stderr, "   -n: client hostname\n");
  fprintf(stderr, "   -p: password\n");
  fprintf(stderr, "   -d: domain\n");
  fprintf(stderr, "   -s: shell\n");
  fprintf(stderr, "   -c: working directory\n");
  fprintf(stderr, "\n");
}

/*****************************************************************************/
static int parse_parameters(int in_argc, char ** in_argv)
{
  int i;

  if (in_argc <= 1)
  {
    out_params();
    return 0;
  }
  for (i = 1; i < in_argc; i++)
  {
    strcpy(g_servername, in_argv[i]);
    if (strcmp(in_argv[i], "-h") == 0)
    {
      out_params();
      return 0;
    }
    else if (strcmp(in_argv[i], "-n") == 0)
    {
      STRNCPY(g_hostname, in_argv[i + 1], sizeof(g_hostname));
    }
    else if (strcmp(in_argv[i], "-u") == 0)
    {
      STRNCPY(g_username, in_argv[i + 1], sizeof(g_username));
    }
    else if (strcmp(in_argv[i], "-p") == 0)
    {
      STRNCPY(g_password, in_argv[i + 1], sizeof(g_password));
      g_flags |= RDP_LOGON_AUTO;
      i++;
    }
    else if (strcmp(in_argv[i], "-d") == 0)
    {
      STRNCPY(g_domain, in_argv[i + 1], sizeof(g_domain));
      i++;
    }
    else if (strcmp(in_argv[i], "-s") == 0)
    {
      STRNCPY(g_shell, in_argv[i + 1], sizeof(g_shell));
      i++;
    }
    else if (strcmp(in_argv[i], "-c") == 0)
    {
      STRNCPY(g_directory, in_argv[i + 1], sizeof(g_directory));
      i++;
    }
  }
  return 1;
}

/*****************************************************************************/
int main(int in_argc, char ** in_argv)
{
  get_username_and_hostname();
  /* read command line options */
  if (!parse_parameters(in_argc, in_argv))
  {
    exit(0);
  }
  /* connect to server */
  if (GrOpen() < 0)
  {
    fprintf(stderr, "Couldn't connect to Nano-X server\n");
    exit(1);
  }
  GrGetScreenInfo(&g_screen_info);
  g_bpp = g_screen_info.bpp;
  g_Bpp = (g_screen_info.bpp + 7) / 8;
  g_width = g_screen_info.vs_width;
  g_height = g_screen_info.vs_height;
  g_clip.x = 0;
  g_clip.y = 0;
  g_clip.width = g_width;
  g_clip.height = g_height;
  if (!((g_bpp == 32 && g_server_bpp == 16) ||
        (g_bpp == 16 && g_server_bpp == 16)))
  {
    fprintf(stderr, "unsupported bpp, server = %d, client = %d\n",
            g_server_bpp, g_bpp);
    GrClose();
    exit(0);
  }
  init_keys();
  /* connect to server */
  if (!rdp_connect(g_servername, g_flags, g_domain, g_password, g_shell,
                   g_directory))
  {
    fprintf(stderr, "Error connecting\n");
    GrClose();
    exit(1);
  }
  /* create window */
  g_wnd = GrNewWindow(GR_ROOT_WINDOW_ID, 0, 0, g_width, g_height, 0, 0, 0);
  /* show window */
  GrMapWindow(g_wnd);
  /* create graphic context */
  g_gc = GrNewGC();
  g_gc_clean = GrNewGC();
  /* clear screen */
  GrSetGCForeground(g_gc, 0);
  GrFillRect(g_wnd, g_gc, 0, 0, g_width, g_height);
  /* create null cursor */
  g_null_cursor = (GR_CURSOR_ID)ui_create_cursor(0, 0, 32, 32, 0, 0);
  /* register callbacks, set mask, and run main loop */
  GrSelectEvents(g_wnd, -1); /* all events */
  GrRegisterInput(g_sck);
  GrMainLoop(nanox_event);
  /* free null cursor */
  ui_destroy_cursor((void*)g_null_cursor);
  /* free graphic context */
  GrDestroyGC(g_gc);
  GrDestroyGC(g_gc_clean);
  /* free window */
  GrDestroyWindow(g_wnd);
  /* close connection */
  GrClose();
  return 0;
}

/* -*- c-basic-offset: 8 -*-
   rdesktop: A Remote Desktop Protocol client.
   Main ui file
   Copyright (C) Jay Sorg 2006

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

#include "precomp.h"

#include "bsops.h"

char g_username[256] = "";
char g_hostname[256] = "";
char g_servername[256] = "";
char g_password[256] = "";
char g_shell[256] = "";
char g_directory[256] = "";
char g_domain[256] = "";
RD_BOOL g_desktop_save = False; /* desktop save order */
RD_BOOL g_polygon_ellipse_orders = False; /* polygon / ellipse orders */
RD_BOOL g_bitmap_compression = True;
uint32 g_rdp5_performanceflags =
  RDP5_NO_WALLPAPER | RDP5_NO_FULLWINDOWDRAG | RDP5_NO_MENUANIMATIONS | RDP5_NO_CURSOR_SHADOW;
RD_BOOL g_bitmap_cache_persist_enable = False;
RD_BOOL g_bitmap_cache_precache = True;
RD_BOOL g_bitmap_cache = True;
RD_BOOL g_encryption = True;
int g_server_depth = 8;
RD_BOOL g_use_rdp5 = False;
int g_width = 800;
int g_height = 600;
uint32 g_keylayout = 0x409; /* Defaults to US keyboard layout */
int g_keyboard_type = 0x4; /* Defaults to US keyboard layout */
int g_keyboard_subtype = 0x0; /* Defaults to US keyboard layout */
int g_keyboard_functionkeys = 0xc; /* Defaults to US keyboard layout */
RD_BOOL g_console_session = False;

/* can't be static, hardware file or bsops need these */
int g_tcp_sck = 0;
int pal_entries[256];

/* Session Directory redirection */
RD_BOOL g_redirect = False;
char g_redirect_server[256];
uint32 g_redirect_server_len;
char g_redirect_domain[256];
uint32 g_redirect_domain_len;
char g_redirect_username[256];
uint32 g_redirect_username_len;
uint8 g_redirect_lb_info[256];
uint32 g_redirect_lb_info_len;
uint8 g_redirect_cookie[256];
uint32 g_redirect_cookie_len;
uint32 g_redirect_flags = 0;
uint32 g_redirect_session_id = 0;

extern int g_tcp_port_rdp;

static int g_deactivated = 0;
static uint32 g_ext_disc_reason = 0;

RDP_VERSION g_rdp_version = RDP_V5;	/* Default to version 5 */
RD_BOOL g_encryption_initial = True;
RD_BOOL g_user_quit = False;
RD_BOOL g_network_error = False;
uint8 g_client_random[SEC_RANDOM_SIZE];
RD_BOOL g_pending_resize = False;
RD_BOOL g_numlock_sync = False;

uint32 g_reconnect_logonid = 0;
char g_reconnect_random[16];
time_t g_reconnect_random_ts;
RD_BOOL g_has_reconnect_random = False;
RD_BOOL g_reconnect_loop = False;

struct bitmap
{
  uint8 * data;
  uint32 width;
  uint32 height;
};

/* in ui specific file eg win32.c, qt.c, df.c, ... */
int
mi_create_window(void);
int
mi_main_loop(void);
void
mi_error(char * msg);
void
mi_warning(char * msg);
void
mi_paint_rect(char * data, int width, int height, int x, int y, int cx, int cy);
void
mi_begin_update(void);
void
mi_end_update(void);
void
mi_fill_rect(int x, int y, int cx, int cy, int colour);
void
mi_screen_copy(int x, int y, int cx, int cy, int srcx, int srcy);
void
mi_set_clip(int x, int y, int cx, int cy);
void
mi_reset_clip(void);
void
mi_line(int x1, int y1, int x2, int y2, int colour);
void*
mi_create_cursor(unsigned int x, unsigned int y,
                 int width, int height,
                 unsigned char * andmask, unsigned char * xormask);


void
mi_destroy_cursor(void * cursor);
void
mi_set_cursor(void * cursor);
void
mi_set_null_cursor(void);
int
mi_read_keyboard_state(void);

/*****************************************************************************/
/* put part of the screen from the backing store to the display */
void
ui_invalidate(int x, int y, int cx, int cy)
{
  char * data;

  if (cx < 1 || cy < 1)
  {
    return;
  }
  if (bs_warp_coords(&x, &y, &cx, &cy, 0, 0))
  {
    cx = (cx + 3) & ~3;
    data = (char *) xmalloc(cx * cy * 4);
    bs_copy_box(data, x, y, cx, cy, cx * ((g_server_depth + 7) / 8));
    mi_paint_rect(data, cx, cy, x, y, cx, cy);
    xfree(data);
  }
}

/*****************************************************************************/
void
ui_bell(void)
{
}

/*****************************************************************************/
int
ui_select(int in)
{
  if (g_tcp_sck == 0)
  {
    g_tcp_sck = in;
  }
  return 1;
}

/*****************************************************************************/
void *
ui_create_cursor(unsigned int x, unsigned int y,
                 int width, int height,
                 uint8 * andmask, uint8 * xormask, int xor_bpp)
{
  int i;
  int j;
  char am[32 * 4];
  char xm[32 * 4];

  if (width != 32 || height != 32)
  {
    return NULL;
  }
  if (xor_bpp==1)
  {
    return (void *) mi_create_cursor(x, y, width, height, (unsigned char *)andmask, (unsigned char *)xormask);
  }
  memset(am, 0, 32 * 4);
  memset(xm, 0, 32 * 4);
  for (i = 0; i < 32; i++)
  {
    for (j = 0; j < 32; j++)
    {
      if (bs_is_pixel_on((char *)andmask, j, i, 32, 1))
      {
        bs_set_pixel_on(am, j, 31 - i, 32, 1, 1);
      }
      if (bs_is_pixel_on((char *)xormask, j, i, 32, 24))
      {
        bs_set_pixel_on(xm, j, 31 - i, 32, 1, 1);
      }
    }
  }
  return (void *) mi_create_cursor(x, y, width, height, (unsigned char *)am, (unsigned char *)xm);
}

/*****************************************************************************/
void
ui_destroy_cursor(void * cursor)
{
  mi_destroy_cursor(cursor);
}

/*****************************************************************************/
void
ui_set_cursor(void * cursor)
{
  mi_set_cursor(cursor);
}

/*****************************************************************************/
void
ui_set_null_cursor(void)
{
  mi_set_null_cursor();
}

/*****************************************************************************/
void *
ui_create_glyph(int width, int height, uint8 * data)
{
  int i;
  int j;
  char * glyph_data;
  struct bitmap * the_glyph;

  glyph_data = (char *) xmalloc(width * height);
  memset(glyph_data, 0, width * height);
  the_glyph = (struct bitmap *) xmalloc(sizeof(struct bitmap));
  memset(the_glyph, 0, sizeof(struct bitmap));
  the_glyph->width = width;
  the_glyph->height = height;
  the_glyph->data = (uint8 *)glyph_data;
  for (i = 0; i < height; i++)
  {
    for (j = 0; j < width; j++)
    {
      if (bs_is_pixel_on((char *)data, j, i, width, 1))
      {
        bs_set_pixel_on(glyph_data, j, i, width, 8, 255);
      }
    }
  }
  return the_glyph;
}

/*****************************************************************************/
void
ui_destroy_glyph(void * glyph)
{
  struct bitmap * the_glyph;

  the_glyph = glyph;
  if (the_glyph != 0)
  {
    xfree(the_glyph->data);
  }
  xfree(the_glyph);
}

/*****************************************************************************/
void *
ui_create_bitmap(int width, int height, uint8 * data)
{
  struct bitmap * b;
  int size;

  size = width * height * ((g_server_depth + 7) / 8);
  b = (struct bitmap *) xmalloc(sizeof(struct bitmap));
  b->data = (uint8 *) xmalloc(size);
  memcpy(b->data, data, size);
  b->width = width;
  b->height = height;
  return b;
}

/*****************************************************************************/
void
ui_destroy_bitmap(void * bmp)
{
  struct bitmap * b;

  b = (struct bitmap *) bmp;
  if (b != 0)
  {
    xfree(b->data);
  }
  xfree(b);
}

/*****************************************************************************/
void
ui_paint_bitmap(int x, int y, int cx, int cy,
                int width, int height, uint8 * data)
{
  struct bitmap b;

  b.width = width;
  b.height = height;
  b.data = data;
  ui_memblt(12, x, y, cx, cy, &b, 0, 0);
}

/*****************************************************************************/
void
ui_set_clip(int x, int y, int cx, int cy)
{
  bs_set_clip(x, y, cx, cy);
  mi_set_clip(x, y, cx, cy);
}

/*****************************************************************************/
void
ui_reset_clip(void)
{
  bs_reset_clip();
  mi_reset_clip();
}

/*****************************************************************************/
void *
ui_create_colourmap(COLOURMAP * colours)
{
  int i;
  int n;

  n = MIN(256, colours->ncolours);
  memset(pal_entries, 0, sizeof(pal_entries));
  for (i = 0; i < n; i++)
  {
    pal_entries[i] = (colours->colours[i].red << 16) |
                     (colours->colours[i].green << 8) |
                     colours->colours[i].blue;
  }
  return 0;
}

/*****************************************************************************/
void
ui_set_colourmap(void * map)
{
}

/*****************************************************************************/
static void
draw_glyph(int x, int y, void * glyph, int fgcolor)
{
  struct bitmap * b;

  b = glyph;
  bs_draw_glyph(x, y, (char *)b->data, b->width, b->height, fgcolor);
}

/*****************************************************************************/
#define DO_GLYPH(ttext,idx) \
{ \
  glyph = cache_get_font(font, ttext[idx]); \
  if (!(flags & TEXT2_IMPLICIT_X)) \
  { \
    xyoffset = ttext[++idx]; \
    if (xyoffset & 0x80) \
    { \
      if (flags & TEXT2_VERTICAL) \
      { \
        y += ttext[idx + 1] | (ttext[idx + 2] << 8); \
      } \
      else \
      { \
        x += ttext[idx + 1] | (ttext[idx + 2] << 8); \
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
    draw_glyph(x + glyph->offset, y + glyph->baseline, glyph->pixmap, \
               fgcolour); \
    if (flags & TEXT2_IMPLICIT_X) \
    { \
      x += glyph->width; \
    } \
  } \
}

/*****************************************************************************/
void
ui_draw_text(uint8 font, uint8 flags, uint8 opcode, int mixmode,
             int x, int y,
             int clipx, int clipy, int clipcx, int clipcy,
             int boxx, int boxy, int boxcx, int boxcy, BRUSH * brush,
             int bgcolour, int fgcolour, uint8 * text, uint8 length)
{
  int i;
  int j;
  int xyoffset;
  DATABLOB * entry;
  FONTGLYPH * glyph;

  if (boxx + boxcx > g_width)
  {
    boxcx = g_width - boxx;
  }
  if (boxcx > 1)
  {
    bs_rect(boxx, boxy, boxcx, boxcy, bgcolour, 0xc);
  }
  else
  {
    if (mixmode == MIX_OPAQUE)
    {
      bs_rect(clipx, clipy, clipcx, clipcy, bgcolour, 0xc);
    }
  }
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
  if (boxcx > 1)
  {
    ui_invalidate(boxx, boxy, boxcx, boxcy);
  }
  else
  {
    ui_invalidate(clipx, clipy, clipcx, clipcy);
  }
}

/*****************************************************************************/
void
ui_line(uint8 opcode, int startx, int starty, int endx, int endy,
        PEN * pen)
{
  int x;
  int y;
  int cx;
  int cy;

  bs_line(opcode, startx, starty, endx, endy, pen->width, pen->style,
          pen->colour);
  if (pen->style == 0 && pen->width < 2 && opcode == 12)
  {
    mi_line(startx, starty, endx, endy, pen->colour);
  }
  else
  {
    x = MIN(startx, endx);
    y = MIN(starty, endy);
    cx = (MAX(startx, endx) + 1) - x;
    cy = (MAX(starty, endy) + 1) - y;
    ui_invalidate(x, y, cx, cy);
  }
}

/*****************************************************************************/
void
ui_triblt(uint8 opcode, int x, int y, int cx, int cy,
          void * src, int srcx, int srcy,
          BRUSH* brush, int bgcolour, int fgcolour)
{
  /* not used */
}

/*****************************************************************************/
void
ui_memblt(uint8 opcode, int x, int y, int cx, int cy,
          void * src, int srcx, int srcy)
{
  struct bitmap* b;

  b = (struct bitmap*)src;
  bs_memblt(opcode, x, y, cx, cy, b->data, b->width, b->height,
            srcx, srcy);
  ui_invalidate(x, y, cx, cy);
}

/*****************************************************************************/
void
ui_desktop_restore(uint32 offset, int x, int y, int cx, int cy)
{
}

/*****************************************************************************/
void
ui_desktop_save(uint32 offset, int x, int y, int cx, int cy)
{
}

/*****************************************************************************/
void
ui_rect(int x, int y, int cx, int cy, int colour)
{
  bs_rect(x, y, cx, cy, colour, 12);
  mi_fill_rect(x, y, cx, cy, colour);
}

/*****************************************************************************/
void
ui_screenblt(uint8 opcode, int x, int y, int cx, int cy,
             int srcx, int srcy)
{
  bs_screenblt(opcode, x, y, cx, cy, srcx, srcy);
  if (opcode == 12)
  {
    mi_screen_copy(x, y, cx, cy, srcx, srcy);
  }
  else
  {
    ui_invalidate(x, y, cx, cy);
  }
}

/*****************************************************************************/
void
ui_patblt(uint8 opcode, int x, int y, int cx, int cy,
          BRUSH * brush, int bgcolour, int fgcolour)
{
  bs_patblt(opcode, x, y, cx, cy, brush->style, (char *)brush->pattern,
            brush->xorigin, brush->yorigin, bgcolour, fgcolour);
  ui_invalidate(x, y, cx, cy);
}

/*****************************************************************************/
void
ui_destblt(uint8 opcode, int x, int y, int cx, int cy)
{
  bs_rect(x, y, cx, cy, 0, opcode);
  ui_invalidate(x, y, cx, cy);
  /* todo */
}

/*****************************************************************************/
void
ui_move_pointer(int x, int y)
{
}

/*****************************************************************************/
uint16
ui_get_numlock_state(uint32 state)
{
  return (uint16) state;
}

/*****************************************************************************/
/* get the num, caps, and scroll lock state */
/* scroll lock is 1, num lock is 2 and caps lock is 4 */
/* just returning 0, the hardware specific file is responsable for this */
uint32
read_keyboard_state(void)
{
  return (uint32) mi_read_keyboard_state();
}

/*****************************************************************************/
void
ui_set_modifier_state(int code)

{

  //error("%8.8x", code);

  rdp_send_input(0, RDP_INPUT_SYNCHRONIZE, 0, (uint16) code, 0);

}

/*****************************************************************************/
void
ui_resize_window(void)
{
}

/*****************************************************************************/
void
ui_begin_update(void)
{
  mi_begin_update();
}

/*****************************************************************************/
void
ui_end_update(void)
{
  mi_end_update();
}

/*****************************************************************************/
void
ui_polygon(uint8 opcode, uint8 fillmode, RD_POINT * point, int npoints,
           BRUSH * brush, int bgcolour, int fgcolour)
{
  /* not used */
}

/*****************************************************************************/
void
ui_polyline(uint8 opcode, RD_POINT * points, int npoints, PEN * pen)
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
void
ui_ellipse(uint8 opcode, uint8 fillmode,
           int x, int y, int cx, int cy,
           BRUSH * brush, int bgcolour, int fgcolour)
{
  /* not used */
}

/*****************************************************************************/
/* get a 32 byte random */
void
generate_random(uint8 * random)
{
  int i;

  rand();
  rand();
  for (i = 0; i < 32; i++)
  {
    random[i] = rand(); /* higher bits are more random */
  }
}

/*****************************************************************************/
void
save_licence(uint8 * data, int length)
{
}

/*****************************************************************************/
int
load_licence(uint8 ** data)
{
  return 0;
}

/*****************************************************************************/
void *
xrealloc(void * in, size_t size)
{
  if (size < 1)
  {
    size = 1;
  }
  return realloc(in, size);
}

/*****************************************************************************/
void *
xmalloc(int size)
{
  if (size < 1)
  {
    size = 1;
  }
  return malloc(size);
}

/*****************************************************************************/
void
xfree(void * in)
{
  if (in != 0)
  {
    free(in);
  }
}

/*****************************************************************************/
char *
xstrdup(const char * s)
{
  int len;
  char * p;

  if (s == 0)
  {
    return 0;
  }
  len = strlen(s);
  p = (char *) xmalloc(len + 1);
  strcpy(p, s);
  return p;
}

/*****************************************************************************/
void
warning(char * format, ...)
{
  va_list ap;
  char text[512];
  char text1[512];

  sprintf(text1, "WARNING: ");
  va_start(ap, format);
  vsprintf(text, format, ap);
  va_end(ap);
  strcat(text1, text);
  mi_warning(text1);
}

/*****************************************************************************/
void
unimpl(char * format, ...)
{
  va_list ap;
  char text[512];
  char text1[512];

  sprintf(text1, "UNIMPL: ");
  va_start(ap, format);
  vsprintf(text, format, ap);
  va_end(ap);
  strcat(text1, text);
  mi_warning(text1);
}

/*****************************************************************************/
void
error(char * format, ...)
{
  va_list ap;
  char text[512];
  char text1[512];

  sprintf(text1, "ERROR: ");
  va_start(ap, format);
  vsprintf(text, format, ap);
  va_end(ap);
  strcat(text1, text);
  mi_error(text1);
}

/*****************************************************************************/
BOOL
rd_pstcache_mkdir(void)
{
  return 0;
}

/*****************************************************************************/
int
rd_open_file(char * filename)
{
  return 0;
}

/*****************************************************************************/
void
rd_close_file(int fd)
{
  return;
}

/*****************************************************************************/
int
rd_read_file(int fd, void * ptr, int len)
{
  return 0;
}

/*****************************************************************************/
int
rd_write_file(int fd, void * ptr, int len)
{
  return 0;
}

/*****************************************************************************/
int
rd_lseek_file(int fd, int offset)
{
  return 0;
}

/*****************************************************************************/
BOOL
rd_lock_file(int fd, int start, int len)
{
  return False;
}


/*****************************************************************************/
void
ui_mouse_move(int x, int y)
{
  rdp_send_input(0, RDP_INPUT_MOUSE, MOUSE_FLAG_MOVE, (uint16) x, (uint16) y);
}


/*****************************************************************************/
void
ui_mouse_button(int button, int x, int y, int down)
{
  uint16 flags;

  flags = 0;
  if (down)
  {
    flags |= MOUSE_FLAG_DOWN;
  }
  switch (button)
  {
    case 1:
      flags |= MOUSE_FLAG_BUTTON1;
      break;
    case 2:
      flags |= MOUSE_FLAG_BUTTON2;
      break;
    case 3:
      flags |= MOUSE_FLAG_BUTTON3;
      break;
    case 4:
      flags |= MOUSE_FLAG_BUTTON4;
      break;
    case 5:
      flags |= MOUSE_FLAG_BUTTON5;
      break;
  }
  rdp_send_input(0, RDP_INPUT_MOUSE, flags, (uint16) x, (uint16) y);
}


/*****************************************************************************/
void
ui_key_down(int key, int ext)

{
  rdp_send_input(0, RDP_INPUT_SCANCODE, (uint16) (RDP_KEYPRESS | ext),
                 (uint16) key, 0);
}

/*****************************************************************************/
void
ui_key_up(int key, int ext)
{
  rdp_send_input(0, RDP_INPUT_SCANCODE, (uint16) (RDP_KEYRELEASE | ext),
                 (uint16) key, 0);
}

/*****************************************************************************/
/* returns boolean, non zero is good */
int
ui_read_wire(void)
{
  return rdp_loop(&g_deactivated, &g_ext_disc_reason);
}

/*****************************************************************************/
/* called after the command line parameters are processed */
/* returns boolean, non zero is ok */
int
ui_main(void)
{
  uint32 flags;

  /* try to connect */
  flags = RDP_LOGON_NORMAL;
  if (g_password[0] != 0)
  {
    flags |= RDP_INFO_AUTOLOGON;
  }
  if (!rdp_connect(g_servername, flags, g_domain, g_password,
                   g_shell, g_directory, FALSE))
  {
    return 0;
  }
  /* init backingstore */
  bs_init(g_width, g_height, g_server_depth);
  /* create the window */
  if (!mi_create_window())
  {
    return 0;
  }
  /* if all ok, enter main loop */
  return mi_main_loop();
}

/*****************************************************************************/
/* produce a hex dump */
void
hexdump(uint8 * p, uint32 len)
{
  uint8 * line = p;
  int i, thisline, offset = 0;

  while (offset < (int)len)
  {
    printf("%04x ", offset);
    thisline = len - offset;
    if (thisline > 16)
      thisline = 16;

    for (i = 0; i < thisline; i++)
      printf("%02x ", line[i]);

    for (; i < 16; i++)
      printf("   ");

    for (i = 0; i < thisline; i++)
      printf("%c", (line[i] >= 0x20 && line[i] < 0x7f) ? line[i] : '.');

    printf("\n");
    offset += thisline;
    line += thisline;
  }
}


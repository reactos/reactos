/* -*- c-basic-offset: 8 -*-
   rdesktop: A Remote Desktop Protocol client.
   User interface services - SVGA lib
   Copyright (C) Jay Sorg 2004-2005

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

#include "../rdesktop.h"

#include <vga.h>
#include <vgakeyboard.h>
#include <vgamouse.h>
#include <vgagl.h>

#include <unistd.h> // gethostname
#include <pwd.h> // getpwuid
#include <stdarg.h> // va_list va_start va_end

#include <sys/ioctl.h>
#include <linux/keyboard.h>
#include <linux/kd.h>
#include <fcntl.h>

extern int g_tcp_port_rdp;
int g_use_rdp5 = 0;
char g_hostname[16] = "";
char g_username[64] = "";
int g_height = 600;
int g_width = 800;
int g_server_bpp = 8;
int g_encryption = 1;
int g_desktop_save = 1;
int g_polygon_ellipse_orders = 0;
int g_bitmap_cache = 1;
int g_bitmap_cache_persist_enable = False;
int g_bitmap_cache_precache = True;
int g_bitmap_compression = 1;
int g_rdp5_performanceflags = 0;
int g_console_session = 0;
int g_keylayout = 0x409; /* Defaults to US keyboard layout */
int g_keyboard_type = 0x4; /* Defaults to US keyboard layout */
int g_keyboard_subtype = 0x0; /* Defaults to US keyboard layout */
int g_keyboard_functionkeys = 0xc; /* Defaults to US keyboard layout */

/* hack globals */
int g_argc = 0;
char** g_argv = 0;
int UpAndRunning = 0;
int g_sock = 0;
int deactivated = 0;
uint32 ext_disc_reason = 0;
char g_servername[128] = "";
static uint32* colmap = 0;
static uint8* desk_save = 0;
static int g_server_Bpp = 1;

/* Keyboard LEDS */
static int numlock;
static int capslock;
static int scrolllock;

// this is non null if vgalib has non accel functions available
// reading from video memory is sooo slow
static uint8* sdata = 0;
static int g_save_mem = 0; // for video memory use eg sdata == 0

// video acceleration
static int use_accel = 1;
static int has_fill_box = 0;
static int has_screen_copy = 0;
static int has_put_image = 0;

// clip
int clip_startx;
int clip_starty;
int clip_endx;
int clip_endy;

// mouse
uint8 mouse_under[32 * 32 * 4]; // save area under mouse
int mousex = 0;
int mousey = 0;
int mouseb = 0;

// mouse info
typedef struct
{
  uint8 andmask[32 * 32];
  uint8 xormask[32 * 32];
  int x;
  int y;
  int w;
  int h;
} tcursor;

// mouse global
static tcursor mcursor;

static int g_draw_mouse = 1;

/* Session Directory redirection */
BOOL g_redirect = False;
char g_redirect_server[64];
char g_redirect_domain[16];
char g_redirect_password[64];
char g_redirect_username[64];
char g_redirect_cookie[128];
uint32 g_redirect_flags = 0;

// bitmap
typedef struct
{
  int width;
  int height;
  uint8* data;
  uint8 Bpp;
} bitmap;

typedef struct
{
  int x;
  int y;
  int cx;
  int cy;
  void* prev;
  void* next;
} myrect;

myrect* head_rect = 0;

//*****************************************************************************
// Keyboard stuff - PeterS
static void setled(int mask, int state)
{
  int fd;
  long int leds;

  if (( fd=open("/dev/console", O_NOCTTY)) != -1 )
  {
    if (ioctl (fd, KDGETLED, &leds) != -1)
    {
      leds &= 7;
      if (state)
        leds |= mask;
      else
        leds &= ~mask;
      ioctl (fd, KDSETLED, leds);
    }
    close(fd);
  }
}


//*****************************************************************************
// do a raster op
int rop(int rop, int src, int dst)
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

//*****************************************************************************
// get a screen pixel
int get_pixel(int x, int y)
{
  if (x >= 0 && x < g_width && y >= 0 && y < g_height)
  {
    if (sdata != 0)
    {
      if (g_server_Bpp == 1)
        return sdata[y * g_width + x];
      else if (g_server_Bpp == 2)
        return ((uint16*)sdata)[y * g_width + x];
      else
        return 0;
    }
    else
      return vga_getpixel(x, y);
  }
  else
    return 0;
}

//*****************************************************************************
// set a screen pixel
void set_pixel(int x, int y, int pixel, int op)
{
  if (x >= clip_startx && x < clip_endx && y >= clip_starty && y < clip_endy)
  {
    if (x >= 0 && x < g_width && y >= 0 && y < g_height)
    {
      if (op == 0x0)
        pixel = 0;
      else if (op == 0xf)
        pixel = -1;
      else if (op != 0xc)
        pixel = rop(op, pixel, get_pixel(x, y));
      if (sdata != 0)
      {
        if (g_server_Bpp == 1)
          sdata[y * g_width + x] = pixel;
        else if (g_server_Bpp == 2)
          ((uint16*)sdata)[y * g_width + x] = pixel;
      }
      else
      {
        vga_setcolor(pixel);
        vga_drawpixel(x, y);
      }
    }
  }
}

//*****************************************************************************
// get a pixel from a bitmap
int get_pixel2(int x, int y, uint8* data, int width, int bpp)
{
  if (bpp == 8)
    return data[y * width + x];
  else if (bpp == 16)
    return ((uint16*)data)[y * width + x];
  else
    return 0;
}

//*****************************************************************************
// set a pixel in a bitmap
void set_pixel2(int x, int y, int pixel, uint8* data, int width, int bpp)
{
  if (bpp == 8)
    data[y * width + x] = pixel;
  else if (bpp == 16)
    ((uint16*)data)[y * width + x] = pixel;
}

//*****************************************************************************
// get a pointer into a bitmap
uint8* get_ptr(int x, int y, uint8* data, int width, int bpp)
{
  if (bpp == 8)
    return data + (y * width + x);
  else if (bpp == 16)
    return data + (y * width + x) * 2;
  else
    return 0;
}

//*****************************************************************************
// check if a certain pixel is set in a bitmap
BOOL is_pixel_on(uint8* data, int x, int y, int width, int bpp)
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
    return False;
}

//*****************************************************************************
void set_pixel_on(uint8* data, int x, int y, int width, int bpp, int pixel)
{
  if (bpp == 8)
  {
    data[y * width + x] = pixel;
  }
}

/*****************************************************************************/
int warp_coords(int* x, int* y, int* cx, int* cy, int* srcx, int* srcy)
{
  int dx;
  int dy;
//  int lx = *x, ly = *y, lcx = *cx, lcy = *cy;

  if (clip_startx > *x)
    dx = clip_startx - *x;
  else
    dx = 0;
  if (clip_starty > *y)
    dy = clip_starty - *y;
  else
    dy = 0;
  if (*x + *cx > clip_endx)
    *cx = (*cx - ((*x + *cx) - clip_endx)) /*+ 1*/;
  if (*y + *cy > clip_endy)
    *cy = (*cy - ((*y + *cy) - clip_endy)) /*+ 1*/;
  *cx = *cx - dx;
  *cy = *cy - dy;
  if (*cx <= 0)
    return False;
  if (*cy <= 0)
    return False;
  *x = *x + dx;
  *y = *y + dy;
  if (srcx != NULL)
    *srcx = *srcx + dx;
  if (srcy != NULL)
    *srcy = *srcy + dy;

//  if (*x != lx || *y != ly || *cx != lcx || *cy != lcy)
//    printf("%d %d %d %d to %d %d %d %d\n", lx, ly, lcx, lcy, *x, *y, *cx, *cy);

  return True;
}

//*****************************************************************************
void copy_mem(uint8* d, uint8* s, int n)
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

//*****************************************************************************
void copy_memb(uint8* d, uint8* s, int n)
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

//*****************************************************************************
// all in pixel except line_size is in bytes
void accel_draw_box(int x, int y, int cx, int cy, uint8* data, int line_size)
{
  int i;
  uint8* s;
  uint8* d;

  if (sdata != 0)
  {
    s = data;
    d = get_ptr(x, y, sdata, g_width, g_server_bpp);
    for (i = 0; i < cy; i++)
    {
      copy_mem(d, s, cx * g_server_Bpp);
      s = s + line_size;
      d = d + g_width * g_server_Bpp;
    }
  }
  else if (has_put_image && line_size == cx * g_server_Bpp)
  {
    vga_accel(ACCEL_PUTIMAGE, x, y, cx, cy, data);
  }
  else
  {
    s = data;
    for (i = 0; i < cy; i++)
    {
      vga_drawscansegment(s, x, y + i, cx * g_server_Bpp);
      s = s + line_size;
    }
  }
}

//*****************************************************************************
void accel_fill_rect(int x, int y, int cx, int cy, int color)
{
  int i;
  uint8* temp;
  uint8* d;

  if (sdata != 0)
  {
    temp = xmalloc(cx * g_server_Bpp);
    if (g_server_Bpp == 1)
      for (i = 0; i < cx; i++)
        temp[i] = color;
    else if (g_server_Bpp == 2)
      for (i = 0; i < cx; i++)
        ((uint16*)temp)[i] = color;
    d = get_ptr(x, y, sdata, g_width, g_server_bpp);
    for (i = 0; i < cy; i++)
    {
      copy_mem(d, temp, cx * g_server_Bpp);
      d = d + g_width * g_server_Bpp;
    }
    xfree(temp);
  }
  else if (has_fill_box)
  {
    vga_accel(ACCEL_SETFGCOLOR, color);
    vga_accel(ACCEL_FILLBOX, x, y, cx, cy);
  }
  else
  {
    temp = xmalloc(cx * g_server_Bpp);
    if (g_server_Bpp == 1)
      for (i = 0; i < cx; i++)
        temp[i] = color;
    else if (g_server_Bpp == 2)
      for (i = 0; i < cx; i++)
        ((uint16*)temp)[i] = color;
    for (i = 0; i < cy; i++)
      vga_drawscansegment(temp, x, y + i, cx * g_server_Bpp);
    xfree(temp);
  }
}

//*****************************************************************************
void accel_screen_copy(int x, int y, int cx, int cy, int srcx, int srcy)
{
  uint8* temp;
  uint8* s;
  uint8* d;
  int i;

  if (sdata != 0)
  {
    if (srcy < y)
    { // bottom to top
      s = get_ptr(srcx, (srcy + cy) - 1, sdata, g_width, g_server_bpp);
      d = get_ptr(x, (y + cy) - 1, sdata, g_width, g_server_bpp);
      for (i = 0; i < cy; i++)  // copy down
      {
        copy_mem(d, s, cx * g_server_Bpp);
        s = s - g_width * g_server_Bpp;
        d = d - g_width * g_server_Bpp;
      }
    }
    else if (srcy > y || srcx > x) // copy up or left
    { // top to bottom
      s = get_ptr(srcx, srcy, sdata, g_width, g_server_bpp);
      d = get_ptr(x, y, sdata, g_width, g_server_bpp);
      for (i = 0; i < cy; i++)
      {
        copy_mem(d, s, cx * g_server_Bpp);
        s = s + g_width * g_server_Bpp;
        d = d + g_width * g_server_Bpp;
      }
    }
    else // copy straight right
    {
      s = get_ptr(srcx, srcy, sdata, g_width, g_server_bpp);
      d = get_ptr(x, y, sdata, g_width, g_server_bpp);
      for (i = 0; i < cy; i++)
      {
        copy_memb(d, s, cx * g_server_Bpp);
        s = s + g_width * g_server_Bpp;
        d = d + g_width * g_server_Bpp;
      }
    }
  }
  else if (has_screen_copy)
  {
    vga_accel(ACCEL_SCREENCOPY, srcx, srcy, x, y, cx, cy);
  }
  else
  {
    // slow
    temp = (uint8*)xmalloc(cx * cy * g_server_Bpp);
    for (i = 0; i < cy; i++)
      vga_getscansegment(get_ptr(0, i, temp, cx, g_server_bpp), srcx, srcy + i, cx * g_server_Bpp);
    for (i = 0; i < cy; i++)
      vga_drawscansegment(get_ptr(0, i, temp, cx, g_server_bpp), x, y + i, cx * g_server_Bpp);
    xfree(temp);
  }
}

//*****************************************************************************
// return bool
int contains_mouse(int x, int y, int cx, int cy)
{
  if (mousex + 32 >= x &&
      mousey + 32 >= y &&
      mousex <= x + cx &&
      mousey <= y + cy)
    return 1;
  else
    return 0;
}

//*****************************************************************************
void fill_rect(int x, int y, int cx, int cy, int colour, int opcode)
{
  int i;
  int j;

  if (warp_coords(&x, &y, &cx, &cy, NULL, NULL))
  {
    if (opcode == 0xc)
      accel_fill_rect(x, y, cx, cy, colour);
    else if (opcode == 0xf)
      accel_fill_rect(x, y, cx, cy, -1);
    else if (opcode == 0x0)
      accel_fill_rect(x, y, cx, cy, 0);
    else
    {
      for (i = 0; i < cy; i++)
        for (j = 0; j < cx; j++)
          set_pixel(x + j, y + i, colour, opcode);
    }
  }
}

//*****************************************************************************
void get_rect(int x, int y, int cx, int cy, uint8* p)
{
  int i;

  if (x < 0)
  {
    cx = cx + x;
    x = 0;
  }
  if (y < 0)
  {
    cy = cy + y;
    y = 0;
  }
  if (sdata != 0)
  {
    for (i = 0; i < cy; i++)
    {
      copy_mem(p, get_ptr(x, y + i, sdata, g_width, g_server_bpp), cx * g_server_Bpp);
      p = p + cx * g_server_Bpp;
    }
  }
  else
  {
    for (i = 0; i < cy; i++)
    {
      vga_getscansegment(p, x, y + i, cx * g_server_Bpp);
      p = p + cx * g_server_Bpp;
    }
  }
}

/*****************************************************************************/
// return true if r1 is contained by r2
int is_contained_by(myrect* r1, myrect* r2)
{
  if (r1->x >= r2->x &&
      r1->y >= r2->y &&
      r1->x + r1->cx <= r2->x + r2->cx &&
      r1->y + r1->cy <= r2->y + r2->cy)
    return 1;
  else
    return 0;
}

/*****************************************************************************/
void draw_cursor_under(int ox, int oy)
{
  int i;
  int j;
  int k;
  uint8* ptr;
  int len;

  if (ox < 0)
    k = -ox;
  else
    k = 0;
  j = g_width - ox;
  if (j > 32)
    j = 32;
  if (j > 0)
  {
    for (i = 0; i < 32; i++)
    {
      ptr = get_ptr(k, i, mouse_under, 32, g_server_bpp);
      len = (j - k) * g_server_Bpp;
      if (ox + k >= 0 && oy + i >= 0 && ox + k < g_width && oy + i < g_height)
        vga_drawscansegment(ptr, ox + k, oy + i, len);
    }
  }
  g_draw_mouse = 1;
}

/*****************************************************************************/
void draw_cursor(void)
{
  int i;
  int j;
  int k;
  int pixel;
  uint8 mouse_a[32 * 32 * 4];
  uint8* ptr;
  int len;

  if (!g_draw_mouse)
    return;
  memset(mouse_under, 0, sizeof(mouse_under));
  for (i = 0; i < 32; i++)
  {
    for (j = 0; j < 32; j++)
    {
      pixel = get_pixel(mousex + j, mousey + i);
      set_pixel2(j, i, pixel, mouse_under, 32, g_server_bpp);
      if (mcursor.andmask[i * 32 + j] == 0)
        k = 0;
      else
        k = ~0;
      pixel = rop(0x8, k, pixel);
      if (mcursor.xormask[i * 32 + j] == 0)
        k = 0;
      else
        k = ~0;
      pixel = rop(0x6, k, pixel);
      set_pixel2(j, i, pixel, mouse_a, 32, g_server_bpp);
    }
  }
  if (mousex < 0)
    k = -mousex;
  else
    k = 0;
  j = g_width - mousex;
  if (j > 32)
    j = 32;
  if (j > 0)
  {
    for (i = mousey; i < mousey + 32; i++)
      if (i < g_height && i >= 0)
      {
        ptr = get_ptr(k, i - mousey, mouse_a, 32, g_server_bpp);
        len = (j - k) * g_server_Bpp;
        vga_drawscansegment(ptr, mousex + k, i, len);
      }
  }
  g_draw_mouse = 0;
}

/*****************************************************************************/
// add a rect to cache
void cache_rect(int x, int y, int cx, int cy, int do_warp)
{
  myrect* rect;
  myrect* walk_rect;

  if (sdata == 0)
  {
    draw_cursor();
    return;
  }
  if (do_warp)
    if (!warp_coords(&x, &y, &cx, &cy, NULL, NULL))
      return;
  rect = (myrect*)xmalloc(sizeof(myrect));
  rect->x = x;
  rect->y = y;
  rect->cx = cx;
  rect->cy = cy;
  rect->next = 0;
  rect->prev = 0;
  if (head_rect == 0)
    head_rect = rect;
  else
  {
    walk_rect = 0;
    do
    {
      if (walk_rect == 0)
        walk_rect = head_rect;
      else
        walk_rect = walk_rect->next;
      if (is_contained_by(rect, walk_rect))
      {
        xfree(rect);
        return;
      }
    }
    while (walk_rect->next != 0);
    walk_rect->next = rect;
    rect->prev = walk_rect;
  }
}

//*****************************************************************************
void draw_cache_rects(void)
{
  int i;
  myrect* rect;
  myrect* rect1;
  uint8* p;

  // draw all the rects
  rect = head_rect;
  while (rect != 0)
  {
    p = get_ptr(rect->x, rect->y, sdata, g_width, g_server_bpp);
    for (i = 0; i < rect->cy; i++)
    {
      vga_drawscansegment(p, rect->x, rect->y + i, rect->cx * g_server_Bpp);
      p = p + g_width * g_server_Bpp;
    }
    rect1 = rect;
    rect = rect->next;
    xfree(rect1);
  }
  head_rect = 0;
}

/*****************************************************************************/
void key_event(int scancode, int pressed)
{
  int rdpkey;
  int ext;

  if (!UpAndRunning)
    return;
  rdpkey = scancode;
  ext = 0;
  
  // Keyboard LEDS
  if ((scancode == SCANCODE_CAPSLOCK) && pressed)
  {
     capslock = !capslock;
     setled(LED_CAP, capslock);
  }
  if ((scancode == SCANCODE_SCROLLLOCK) && pressed)
  {
     scrolllock = !scrolllock;
     setled(LED_SCR, scrolllock);
  }

  if ((scancode == SCANCODE_NUMLOCK) && pressed)
  {
     numlock = !numlock;
     setled(LED_NUM, numlock);
  }
     
  switch (scancode)
  {
    case SCANCODE_CURSORBLOCKUP:    rdpkey = 0xc8; ext = KBD_FLAG_EXT; break; // up arrow
    case SCANCODE_CURSORBLOCKDOWN:  rdpkey = 0xd0; ext = KBD_FLAG_EXT; break; // down arrow
    case SCANCODE_CURSORBLOCKRIGHT: rdpkey = 0xcd; ext = KBD_FLAG_EXT; break; // right arrow
    case SCANCODE_CURSORBLOCKLEFT:  rdpkey = 0xcb; ext = KBD_FLAG_EXT; break; // left arrow
    case SCANCODE_PAGEDOWN:         rdpkey = 0xd1; ext = KBD_FLAG_EXT; break; // page down
    case SCANCODE_PAGEUP:           rdpkey = 0xc9; ext = KBD_FLAG_EXT; break; // page up
    case SCANCODE_HOME:             rdpkey = 0xc7; ext = KBD_FLAG_EXT; break; // home
    case SCANCODE_END:              rdpkey = 0xcf; ext = KBD_FLAG_EXT; break; // end
    case SCANCODE_INSERT:           rdpkey = 0xd2; ext = KBD_FLAG_EXT; break; // insert
    case SCANCODE_REMOVE:           rdpkey = 0xd3; ext = KBD_FLAG_EXT; break; // delete
    case SCANCODE_KEYPADDIVIDE:     rdpkey = 0x35; break; // /
    case SCANCODE_KEYPADENTER:      rdpkey = 0x1c; break; // enter
    case SCANCODE_RIGHTCONTROL:     rdpkey = 0x1d; break; // right ctrl
    case SCANCODE_RIGHTALT:         rdpkey = 0x38; break; // right alt
    case SCANCODE_LEFTWIN:          rdpkey = 0x5b; ext = KBD_FLAG_EXT; break; // left win
    case SCANCODE_RIGHTWIN:         rdpkey = 0x5c; ext = KBD_FLAG_EXT; break; // right win
    case 127:                       rdpkey = 0x5d; ext = KBD_FLAG_EXT; break; // menu key
    case SCANCODE_PRINTSCREEN:      rdpkey = 0x37; ext = KBD_FLAG_EXT; break; // print screen
    case SCANCODE_BREAK:            //rdpkey = 0; break; // break
    {
      if (pressed)
      {
        ext = KBD_FLAG_EXT;
        rdp_send_input(0, RDP_INPUT_SCANCODE, RDP_KEYPRESS | ext, 0x46, 0);
        rdp_send_input(0, RDP_INPUT_SCANCODE, RDP_KEYPRESS | ext, 0xc6, 0);
      }    
      rdpkey = 0;
    }
    case SCANCODE_SCROLLLOCK:       rdpkey = 0x46; break; // scroll lock
    case 112: // mouse down
    {
      rdp_send_input(0, RDP_INPUT_MOUSE, MOUSE_FLAG_DOWN | MOUSE_FLAG_BUTTON4,
                     mouse_getx(), mouse_gety());
      return;
    }
    case 113: // mouse up
    {
      rdp_send_input(0, RDP_INPUT_MOUSE, MOUSE_FLAG_DOWN | MOUSE_FLAG_BUTTON5,
                     mouse_getx(), mouse_gety());
      return;
    }
  }
//  printf("%d %d\n", scancode, pressed);
  if (pressed)
    rdp_send_input(0, RDP_INPUT_SCANCODE, RDP_KEYPRESS | ext, rdpkey, 0);
  else
    rdp_send_input(0, RDP_INPUT_SCANCODE, RDP_KEYRELEASE | ext, rdpkey, 0);
  

}

/*****************************************************************************/
int ui_init(void)
{
  vga_init();
  memset(&mcursor, 0, sizeof(tcursor));
  desk_save = (uint8*)xmalloc(0x38400 * g_server_Bpp);
  return 1;
}

/*****************************************************************************/
void ui_deinit(void)
{
  xfree(desk_save);
}

/*****************************************************************************/
int ui_create_window(void)
{
  int vgamode;
  int i;

  vgamode = G800x600x256;
  if (g_width == 640 && g_height == 480)
  {
    if (g_server_Bpp == 1)
      vgamode = G640x480x256;
    else if (g_server_Bpp == 2)
      vgamode = G640x480x64K;
  }
  else if (g_width == 800 && g_height == 600)
  {
    if (g_server_Bpp == 1)
      vgamode = G800x600x256;
    else if (g_server_Bpp == 2)
      vgamode = G800x600x64K;
  }
  else if (g_width == 1024 && g_height == 768)
  {
    if (g_server_Bpp == 1)
      vgamode = G1024x768x256;
    else if (g_server_Bpp == 2)
      vgamode = G1024x768x64K;
  }
  else
  {
    error("Invalid width / height");
    return 0;
  }
  ui_reset_clip();
  if (!vga_hasmode(vgamode))
  {
    error("Graphics unavailable");
    return 0;
  }
  vga_setmousesupport(1);
  mouse_setposition(g_width / 2, g_height / 2);
  vga_setmode(vgamode);
  if (keyboard_init())
  {
    error("Keyboard unavailable");
    return 0;
  }
  keyboard_seteventhandler(key_event);
  if (use_accel)
  {
    i = vga_ext_set(VGA_EXT_AVAILABLE, VGA_AVAIL_ACCEL);
    if (i & ACCELFLAG_PUTIMAGE)
      has_put_image = 1;
    if (i & ACCELFLAG_SCREENCOPY)
      has_screen_copy = 1;
    if (i & ACCELFLAG_FILLBOX)
      has_fill_box = 1;
    printf("accel %d\n", i);
  }
  if (!has_screen_copy && !g_save_mem)
    sdata = xmalloc(g_width * g_height * g_server_Bpp);
  return 1;
}

/*****************************************************************************/
void ui_destroy_window(void)
{
  keyboard_close(); /* Don't forget this! */
  vga_setmode(TEXT);
  if (sdata != 0)
    xfree(sdata);
}

/*****************************************************************************/
void process_mouse(void)
{
  int ox = mousex;
  int oy = mousey;
  int ob = mouseb;

  if (!UpAndRunning)
    return;
  mousex = mouse_getx() - mcursor.x;
  mousey = mouse_gety() - mcursor.y;
  mouseb = mouse_getbutton();

  if (mouseb != ob) // button
  {
    // right button
    if (mouseb & 1)
      if (!(ob & 1))
        rdp_send_input(0, RDP_INPUT_MOUSE, MOUSE_FLAG_DOWN | MOUSE_FLAG_BUTTON2,
                       mousex + mcursor.x, mousey + mcursor.y);
    if (ob & 1)
      if (!(mouseb & 1))
        rdp_send_input(0, RDP_INPUT_MOUSE, MOUSE_FLAG_BUTTON2,
                       mousex + mcursor.x, mousey + mcursor.y);
    // middle button
    if (mouseb & 2)
      if (!(ob & 2))
        rdp_send_input(0, RDP_INPUT_MOUSE, MOUSE_FLAG_DOWN | MOUSE_FLAG_BUTTON3,
                       mousex + mcursor.x, mousey + mcursor.y);
    if (ob & 2)
      if (!(mouseb & 2))
        rdp_send_input(0, RDP_INPUT_MOUSE, MOUSE_FLAG_BUTTON3,
                       mousex + mcursor.x, mousey + mcursor.y);
    // left button
    if (mouseb & 4)
      if (!(ob & 4))
        rdp_send_input(0, RDP_INPUT_MOUSE, MOUSE_FLAG_DOWN | MOUSE_FLAG_BUTTON1,
                       mousex + mcursor.x, mousey + mcursor.y);
    if (ob & 4)
      if (!(mouseb & 4))
        rdp_send_input(0, RDP_INPUT_MOUSE, MOUSE_FLAG_BUTTON1,
                       mousex + mcursor.x, mousey + mcursor.y);
  }
  if (mousex != ox || mousey != oy) // movement
  {
    rdp_send_input(0, RDP_INPUT_MOUSE, MOUSE_FLAG_MOVE,
                   mousex + mcursor.x, mousey + mcursor.y);
    draw_cursor_under(ox, oy);
    draw_cursor();
  }
}

/*****************************************************************************/
void process_keyboard(void)
{
  if (!UpAndRunning)
    return;
}

/*****************************************************************************/
BOOL ui_main_loop(void)
{
  int sel;
  fd_set rfds;

  if (!rdp_connect(g_servername, RDP_LOGON_NORMAL, "", "", "", ""))
    return False;
  UpAndRunning = 1;
  FD_ZERO(&rfds);
  FD_SET(g_sock, &rfds);
  sel = vga_waitevent(3, &rfds, NULL, NULL, NULL);
  while (sel >= 0)
  {
    if (sel & 1) /* mouse */
    {
      process_mouse();
    }
    else if (sel & 2) /* keyboard */
    {
      process_keyboard();
    }
    else
    {
      if (!rdp_loop(&deactivated, &ext_disc_reason))
        return True; /* ok */
    }
    FD_ZERO(&rfds);
    FD_SET(g_sock, &rfds);
    sel = vga_waitevent(3, &rfds, NULL, NULL, NULL);
  }
  return True;
}

/*****************************************************************************/
void ui_bell(void)
{
}

/*****************************************************************************/
int ui_select(int in)
{
  g_sock = in;
  return 1;
}

/*****************************************************************************/
void* ui_create_glyph(int width, int height, uint8* data)
{
  int i, j;
  uint8* glyph_data;
  bitmap* the_glyph;

  glyph_data = (uint8*)xmalloc(width * height);
  the_glyph = (bitmap*)xmalloc(sizeof(bitmap));
  the_glyph->width = width;
  the_glyph->height = height;
  the_glyph->data = glyph_data;
  memset(glyph_data, 0, width * height);
  for (i = 0; i < height; i++)
    for (j = 0; j < width; j++)
      if (is_pixel_on(data, j, i, width, 1))
        set_pixel_on(glyph_data, j, i, width, 8, 255);
  return the_glyph;
}

/*****************************************************************************/
void ui_destroy_glyph(void* glyph)
{
  bitmap* the_glyph;

  the_glyph = (bitmap*)glyph;
  if (the_glyph != NULL)
  {
    if (the_glyph->data != NULL)
      xfree(the_glyph->data);
    xfree(the_glyph);
  }
}

/*****************************************************************************/
void ui_destroy_bitmap(void* bmp)
{
  bitmap* b;

  b = (bitmap*)bmp;
  xfree(b->data);
  xfree(b);
}

/*****************************************************************************/
void ui_reset_clip(void)
{
  clip_startx = 0;
  clip_starty = 0;
  clip_endx = g_width;
  clip_endy = g_height;
}

/*****************************************************************************/
void ui_set_clip(int x, int y, int cx, int cy)
{
  clip_startx = x;
  clip_starty = y;
  clip_endx = x + cx;
  clip_endy = y + cy;
}

/*****************************************************************************/
void* ui_create_colourmap(COLOURMAP * colours)
{
  int i = 0;
  int n = colours->ncolours;
  COLOURENTRY* c = colours->colours;
  int* cmap = (int*)xmalloc(3 * 256 * sizeof (int));
  if (n > 256)
    n = 256;
  bzero(cmap, 256 * 3 * sizeof (int));
  for (i = 0; i < (3 * n); c++)
  {
    cmap[i++] = (c->red) >> 2;
    cmap[i++] = (c->green) >> 2;
    cmap[i++] = (c->blue) >> 2;
  }
  return cmap;
}

/*****************************************************************************/
void ui_destroy_colourmap(HCOLOURMAP map)
{
  if (colmap == map)
    colmap = 0;
  xfree(map);
}

/*****************************************************************************/
void ui_set_colourmap(void* map)
{
  if (colmap != 0)
    xfree(colmap);
  vga_setpalvec(0, 256, (int*)map);
  colmap = map;
}

/*****************************************************************************/
HBITMAP ui_create_bitmap(int width, int height, uint8* data)
{
  bitmap* b;

  b = (bitmap*)xmalloc(sizeof(bitmap));
  b->data = (uint8*)xmalloc(width * height * g_server_Bpp);
  b->width = width;
  b->height = height;
  b->Bpp = g_server_Bpp;
  copy_mem(b->data, data, width * height * g_server_Bpp);
  return (void*)b;
}

//*****************************************************************************
void draw_glyph (int x, int y, HGLYPH glyph, int fgcolour)
{
  bitmap* the_glyph;
  int i, j;

  the_glyph = (bitmap*)glyph;
  if (the_glyph == NULL)
    return;
  for (i = 0; i < the_glyph->height; i++)
    for (j = 0; j < the_glyph->width; j++)
      if (is_pixel_on(the_glyph->data, j, i, the_glyph->width, 8))
        set_pixel(x + j, y + i, fgcolour, 0xc);
}

#define DO_GLYPH(ttext,idx) \
{\
  glyph = cache_get_font (font, ttext[idx]);\
  if (!(flags & TEXT2_IMPLICIT_X))\
    {\
      xyoffset = ttext[++idx];\
      if ((xyoffset & 0x80))\
	{\
	  if (flags & TEXT2_VERTICAL) \
	    y += ttext[idx+1] | (ttext[idx+2] << 8);\
	  else\
	    x += ttext[idx+1] | (ttext[idx+2] << 8);\
	  idx += 2;\
	}\
      else\
	{\
	  if (flags & TEXT2_VERTICAL) \
	    y += xyoffset;\
	  else\
	    x += xyoffset;\
	}\
    }\
  if (glyph != NULL)\
    {\
      draw_glyph (x + glyph->offset, y + glyph->baseline, glyph->pixmap, fgcolour);\
      if (flags & TEXT2_IMPLICIT_X)\
	x += glyph->width;\
    }\
}

/*****************************************************************************/
void ui_draw_text(uint8 font, uint8 flags, uint8 opcode, int mixmode,
                  int x, int y,
                  int clipx, int clipy, int clipcx, int clipcy,
                  int boxx, int boxy, int boxcx, int boxcy, BRUSH * brush,
                  int bgcolour, int fgcolour, uint8* text, uint8 length)
{
  int i;
  int j;
  int xyoffset;
  DATABLOB* entry;
  FONTGLYPH* glyph;

  if (boxcx > 1)
  {
    if (contains_mouse(boxx, boxy, boxcx, boxcy))
      draw_cursor_under(mousex, mousey);
    fill_rect(boxx, boxy, boxcx, boxcy, bgcolour, 0xc);
  }
  else
  {
    if (contains_mouse(clipx, clipy, clipcx, clipcy))
      draw_cursor_under(mousex, mousey);
    if (mixmode == MIX_OPAQUE)
      fill_rect(clipx, clipy, clipcx, clipcy, bgcolour, 0xc);
  }

  /* Paint text, character by character */
  for (i = 0; i < length;)
  {
    switch (text[i])
    {
      case 0xff:
        if (i + 2 < length)
          cache_put_text(text[i + 1], text, text[i + 2]);
        else
        {
          error("this shouldn't be happening\n");
          exit(1);
        }
        /* this will move pointer from start to first character after FF command */
        length -= i + 3;
        text = &(text[i + 3]);
        i = 0;
        break;

      case 0xfe:
        entry = cache_get_text(text[i + 1]);
        if (entry != NULL)
        {
          if ((((uint8 *) (entry->data))[1] == 0) && (!(flags & TEXT2_IMPLICIT_X)))
          {
            if (flags & TEXT2_VERTICAL)
              y += text[i + 2];
            else
              x += text[i + 2];
          }
          for (j = 0; j < entry->size; j++)
            DO_GLYPH(((uint8 *) (entry->data)), j);
        }
        if (i + 2 < length)
          i += 3;
        else
          i += 2;
        length -= i;
        /* this will move pointer from start to first character after FE command */
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
    cache_rect(boxx, boxy, boxcx, boxcy, True);
  else
    cache_rect(clipx, clipy, clipcx, clipcy, True);
}

//*****************************************************************************
// Bresenham's line drawing algorithm
void ui_line(uint8 opcode, int startx, int starty, int endx,
             int endy, PEN* pen)
{
  int dx;
  int dy;
  int incx;
  int incy;
  int dpr;
  int dpru;
  int p;
  int left;
  int top;
  int right;
  int bottom;

  if (startx > endx)
  {
    dx = startx - endx;
    incx = -1;
    left = endx;
    right = startx;
  }
  else
  {
    dx = endx - startx;
    incx = 1;
    left = startx;
    right = endx;
  }
  if (starty > endy)
  {
    dy = starty - endy;
    incy = -1;
    top = endy;
    bottom = starty;
  }
  else
  {
    dy = endy - starty;
    incy = 1;
    top = starty;
    bottom = endy;
  }
  if (contains_mouse(left, top, (right - left) + 1, (bottom - top) + 1))
    draw_cursor_under(mousex, mousey);
  if (dx >= dy)
  {
    dpr = dy << 1;
    dpru = dpr - (dx << 1);
    p = dpr - dx;
    for (; dx >= 0; dx--)
    {
      set_pixel(startx, starty, pen->colour, opcode);
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
      set_pixel(startx, starty, pen->colour, opcode);
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
  cache_rect(left, top, (right - left) + 1, (bottom - top) + 1, True);
}

/*****************************************************************************/
void ui_triblt(uint8 opcode, int x, int y, int cx, int cy,
               HBITMAP src, int srcx, int srcy,
               BRUSH* brush, int bgcolour, int fgcolour)
{
  // non used
}

/*****************************************************************************/
void ui_memblt(uint8 opcode, int x, int y, int cx, int cy,
               HBITMAP src, int srcx, int srcy)
{
  bitmap* b;
  int i;
  int j;
  int pixel;

  if (warp_coords(&x, &y, &cx, &cy, &srcx, &srcy))
  {
    if (contains_mouse(x, y, cx, cy))
      draw_cursor_under(mousex, mousey);
    b = (bitmap*)src;
    if (opcode == 0xc)
      accel_draw_box(x, y, cx, cy, get_ptr(srcx, srcy, b->data, b->width, g_server_bpp),
                     b->width * g_server_Bpp);
    else
    {
      for (i = 0; i < cy; i++)
      {
        for (j = 0; j < cx; j++)
        {
          pixel = get_pixel2(srcx + j, srcy + i, b->data, b->width, g_server_bpp);
          set_pixel(x + j, y + i, pixel, opcode);
        }
      }
    }
    cache_rect(x, y, cx, cy, False);
  }
}

/*****************************************************************************/
void ui_desktop_restore(uint32 offset, int x, int y, int cx, int cy)
{
  uint8* p;

  if (offset > 0x38400)
    offset = 0;
  if (offset + cx * cy > 0x38400)
    return;
  p = desk_save + offset * g_server_Bpp;
  ui_paint_bitmap(x, y, cx, cy, cx, cy, p);
}

/*****************************************************************************/
void ui_desktop_save(uint32 offset, int x, int y, int cx, int cy)
{
  uint8* p;

  if (offset > 0x38400)
    offset = 0;
  if (offset + cx * cy > 0x38400)
    return;
  if (contains_mouse(x, y, cx, cy))
    draw_cursor_under(mousex, mousey);
  p = desk_save + offset * g_server_Bpp;
  get_rect(x, y, cx, cy, p);
}

/*****************************************************************************/
void ui_rect(int x, int y, int cx, int cy, int colour)
{
  if (warp_coords(&x, &y, &cx, &cy, NULL, NULL))
  {
    if (contains_mouse(x, y, cx, cy))
      draw_cursor_under(mousex, mousey);
    accel_fill_rect(x, y, cx, cy, colour);
    cache_rect(x, y, cx, cy, False);
  }
}

/*****************************************************************************/
void ui_screenblt(uint8 opcode, int x, int y, int cx, int cy,
                  int srcx, int srcy)
{
  int i;
  int j;
  uint8* temp;

  if (x == srcx && y == srcy)
    return;
  if (warp_coords(&x, &y, &cx, &cy, &srcx, &srcy))
  {
    if (contains_mouse(x, y, cx, cy) || contains_mouse(srcx, srcy, cx, cy))
      draw_cursor_under(mousex, mousey);
    if (opcode == 0xc) /* copy */
      accel_screen_copy(x, y, cx, cy, srcx, srcy);
    else
    {
      temp = (uint8*)xmalloc(cx * cy * g_server_Bpp);
      for (i = 0; i < cy; i++)
        for (j = 0; j < cx; j++)
          set_pixel2(j, i, get_pixel(srcx + j, srcy + i), temp, cx, g_server_bpp);
      for (i = 0; i < cy; i++)
        for (j = 0; j < cx; j++)
          set_pixel(x + j, y + i, get_pixel2(j, i, temp, cx, g_server_bpp), opcode);
      xfree(temp);
    }
    cache_rect(x, y, cx, cy, False);
    draw_cache_rects(); // draw them all so screen is not jumpy
  }
}

/*****************************************************************************/
void ui_patblt(uint8 opcode, int x, int y, int cx, int cy,
               BRUSH * brush, int bgcolour, int fgcolour)
{
  int i;
  int j;
  uint8 ipattern[8];

  if (warp_coords(&x, &y, &cx, &cy, NULL, NULL))
  {
    if (contains_mouse(x, y, cx, cy))
      draw_cursor_under(mousex, mousey);
    switch (brush->style)
    {
      case 0:
        fill_rect(x, y, cx, cy, fgcolour, opcode);
        break;
      case 3:
        for (i = 0; i < 8; i++)
          ipattern[i] = ~brush->pattern[7 - i];
        for (i = 0; i < cy; i++)
          for (j = 0; j < cx; j++)
            if (is_pixel_on(ipattern, (x + j + brush->xorigin) % 8,
                              (y + i + brush->yorigin) % 8, 8, 1))
              set_pixel(x + j, y + i, fgcolour, opcode);
            else
              set_pixel(x + j, y + i, bgcolour, opcode);
        break;
    }
    cache_rect(x, y, cx, cy, False);
  }
}

/*****************************************************************************/
void ui_destblt(uint8 opcode, int x, int y, int cx, int cy)
{
  if (warp_coords(&x, &y, &cx, &cy, NULL, NULL))
  {
    if (contains_mouse(x, y, cx, cy))
      draw_cursor_under(mousex, mousey);
    fill_rect(x, y, cx, cy, -1, opcode);
    cache_rect(x, y, cx, cy, False);
  }
}

/*****************************************************************************/
void ui_move_pointer(int x, int y)
{
}

/*****************************************************************************/
void ui_set_null_cursor(void)
{
  draw_cursor_under(mousex, mousey);
  mousex = mousex - mcursor.x;
  mousey = mousey - mcursor.y;
  memset(&mcursor, 0, sizeof(mcursor));
  memset(mcursor.andmask, 255, sizeof(mcursor.andmask));
  memset(mcursor.xormask, 0, sizeof(mcursor.xormask));
  draw_cursor();
}

/*****************************************************************************/
void ui_paint_bitmap(int x, int y, int cx, int cy,
                     int width, int height, uint8* data)
{
  if (warp_coords(&x, &y, &cx, &cy, NULL, NULL))
  {
    if (contains_mouse(x, y, cx, cy))
      draw_cursor_under(mousex, mousey);
    accel_draw_box(x, y, cx, cy, data, width * g_server_Bpp);
    cache_rect(x, y, cx, cy, False);
  }
}

/*****************************************************************************/
void* ui_create_cursor(unsigned int x, unsigned int y,
                       int width, int height,
                       uint8* andmask, uint8* xormask)
{
  tcursor* c;
  int i;
  int j;

  c = (tcursor*)xmalloc(sizeof(tcursor));
  memset(c, 0, sizeof(tcursor));
  c->w = width;
  c->h = height;
  c->x = x;
  c->y = y;
  for (i = 0; i < 32; i++)
  {
    for (j = 0; j < 32; j++)
    {
      if (is_pixel_on(andmask, j, i, 32, 1))
        set_pixel_on(c->andmask, j, 31 - i, 32, 8, 255);
      if (is_pixel_on(xormask, j, i, 32, 24))
        set_pixel_on(c->xormask, j, 31 - i, 32, 8, 255);
    }
  }
  return (void*)c;
}

/*****************************************************************************/
void ui_destroy_cursor(void* cursor)
{
  if (cursor != NULL)
    xfree(cursor);
}

/*****************************************************************************/
void ui_set_cursor(void* cursor)
{
  int x;
  int y;
  int ox;
  int oy;

  ox = mousex;
  oy = mousey;
  x = mousex + mcursor.x;
  y = mousey + mcursor.y;
  memcpy(&mcursor, cursor, sizeof(tcursor));
  mousex = x - mcursor.x;
  mousey = y - mcursor.y;
  draw_cursor_under(ox, oy);
  draw_cursor();
}

/*****************************************************************************/
uint16 ui_get_numlock_state(unsigned int state)
{
  return 0;
}

/*****************************************************************************/
unsigned int read_keyboard_state(void)
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
  draw_cache_rects();
  draw_cursor();
}

/*****************************************************************************/
void ui_polygon(uint8 opcode, uint8 fillmode, POINT * point, int npoints,
                BRUSH * brush, int bgcolour, int fgcolour)
{
}

/*****************************************************************************/
void ui_polyline(uint8 opcode, POINT * points, int npoints, PEN * pen)
{
}

/*****************************************************************************/
void ui_ellipse(uint8 opcode, uint8 fillmode,
                int x, int y, int cx, int cy,
                BRUSH * brush, int bgcolour, int fgcolour)
{
}

/*****************************************************************************/
void generate_random(uint8* random)
{
  memcpy(random, "12345678901234567890123456789012", 32);
}

/*****************************************************************************/
void save_licence(uint8* data, int length)
{
}

/*****************************************************************************/
int load_licence(uint8** data)
{
  return 0;
}

/*****************************************************************************/
void* xrealloc(void* in_val, int size)
{
  return realloc(in_val, size);
}

/*****************************************************************************/
void* xmalloc(int size)
{
  return malloc(size);
}

/*****************************************************************************/
void xfree(void* in_val)
{
  free(in_val);
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
void warning(char* format, ...)
{
  va_list ap;

  fprintf(stderr, "WARNING: ");
  va_start(ap, format);
  vfprintf(stderr, format, ap);
  va_end(ap);
}

/*****************************************************************************/
void unimpl(char* format, ...)
{
  va_list ap;

  fprintf(stderr, "NOT IMPLEMENTED: ");
  va_start(ap, format);
  vfprintf(stderr, format, ap);
  va_end(ap);
}

/*****************************************************************************/
void error(char* format, ...)
{
  va_list ap;

  fprintf(stderr, "ERROR: ");
  va_start(ap, format);
  vfprintf(stderr, format, ap);
  va_end(ap);
}

BOOL rd_pstcache_mkdir(void)
{
  return 0;
}

/*****************************************************************************/
int rd_open_file(char *filename)
{
  return 0;
}

/*****************************************************************************/
void rd_close_file(int fd)
{
  return;
}

/*****************************************************************************/
int rd_read_file(int fd, void *ptr, int len)
{
  return 0;
}

/*****************************************************************************/
int rd_write_file(int fd, void* ptr, int len)
{
  return 0;
}

/*****************************************************************************/
int rd_lseek_file(int fd, int offset)
{
  return 0;
}

/*****************************************************************************/
BOOL rd_lock_file(int fd, int start, int len)
{
  return False;
}

/*****************************************************************************/
void get_username_and_hostname(void)
{
  char fullhostname[64];
  char* p;
  struct passwd* pw;

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
      *p = 0;
    STRNCPY(g_hostname, fullhostname, sizeof(g_hostname));
  }
}

/*****************************************************************************/
void out_params(void)
{
  fprintf(stderr, "rdesktop: A Remote Desktop Protocol client.\n");
  fprintf(stderr, "Version " VERSION ". Copyright (C) 1999-2003 Matt Chapman.\n");
  fprintf(stderr, "See http://www.rdesktop.org/ for more information.\n\n");
  fprintf(stderr, "Usage: svgardesktop [options] server\n");
  fprintf(stderr, "   -g: desktop geometry (WxH)\n");
  fprintf(stderr, "   -4: use RDP version 4\n");
  fprintf(stderr, "   -5: use RDP version 5 (default)\n");
  fprintf(stderr, "   -t: tcp port\n");
  fprintf(stderr, "   -u: user name\n");
  fprintf(stderr, "   -n: client hostname\n");
  fprintf(stderr, "   -d: disable accel funcs\n");
  fprintf(stderr, "   -a: connection colour depth\n");
  fprintf(stderr, "   -l: low memory\n");
  fprintf(stderr, "\n");
}

/* produce a hex dump */
void hexdump(uint8* p, uint32 len)
{
  uint8* line;
  int i;
  int thisline;
  int offset;

  line = p;
  offset = 0;
  while (offset < len)
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

/*****************************************************************************/
int parse_parameters(int in_argc, char** in_argv)
{
  int i;
  char* p;

  if (in_argc <= 1)
  {
    out_params();
    return 0;
  }
  g_argc = in_argc;
  g_argv = in_argv;
  for (i = 1; i < in_argc; i++)
  {
    strcpy(g_servername, in_argv[i]);
    if (strcmp(in_argv[i], "-g") == 0)
    {
      g_width = strtol(in_argv[i + 1], &p, 10);
      if (g_width <= 0)
      {
        error("invalid geometry\n");
        return 0;
      }
      if (*p == 'x')
        g_height = strtol(p + 1, NULL, 10);
      if (g_height <= 0)
      {
        error("invalid geometry\n");
        return 0;
      }
      g_width = (g_width + 3) & ~3;
    }
    else if (strcmp(in_argv[i], "-4") == 0)
      g_use_rdp5 = 0;
    else if (strcmp(in_argv[i], "-5") == 0)
      g_use_rdp5 = 1;
    else if (strcmp(in_argv[i], "-t") == 0)
      g_tcp_port_rdp = strtol(in_argv[i + 1], &p, 10);
    else if (strcmp(in_argv[i], "-h") == 0)
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
    else if (strcmp(in_argv[i], "-d") == 0)
    {
      use_accel = 0;
    }
    else if (strcmp(in_argv[i], "-a") == 0)
    {
      g_server_bpp = strtol(in_argv[i + 1], NULL, 10);
      if (g_server_bpp != 8 && g_server_bpp != 16)
      {
        error("invalid server bpp\n");
        return 0;
      }
      g_server_Bpp = (g_server_bpp + 7) / 8;
    }
    else if (strcmp(in_argv[i], "-l") == 0)
      g_save_mem = 1;
  }
  return 1;
}

/*****************************************************************************/
int main(int in_argc, char** in_argv)
{
  get_username_and_hostname();
  if (!parse_parameters(in_argc, in_argv))
    return 0;
  if (!ui_init())
    return 1;
  if (!ui_create_window())
    return 1;
  ui_main_loop();
  ui_destroy_window();
  ui_deinit();
  return 0;
}

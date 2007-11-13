/* -*- c-basic-offset: 8 -*-
   rdesktop: A Remote Desktop Protocol client.
   win32 calls
   Copyright (C) Jay Sorg 2006

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

#include <winsock2.h> /* winsock2.h first */
#include <precomp.h>

//FIXME: remove eventually
#ifndef _UNICODE
#define _UNICODE
#endif
#include <tchar.h>


extern char g_username[];
extern char g_hostname[];
extern char g_servername[];
extern char g_password[];
extern char g_shell[];
extern char g_directory[];
extern char g_domain[];
extern int g_width;
extern int g_height;
extern int g_tcp_sck;
extern int g_server_depth;
extern int g_tcp_port_rdp; /* in tcp.c */
extern int pal_entries[];

static HWND g_Wnd = 0;
static HINSTANCE g_Instance = 0;
static HCURSOR g_cursor = 0;
static int g_block = 0;
static int g_xoff = 0; /* offset from window to client coords */
static int g_yoff = 0;
static int g_xscroll = 0; /* current scroll position */
static int g_yscroll = 0;
static int g_screen_width = 0;
static int g_screen_height = 0;
static int g_wnd_cwidth = 0; /* set from WM_SIZE */
static int g_wnd_cheight = 0;
static int g_fullscreen = 0;
static int g_workarea = 0;
static int g_mousex = 0; /* in client coords */
static int g_mousey = 0;
//static int g_width_height_set = 0;

static int g_clip_left = 0;
static int g_clip_top = 0;
static int g_clip_right = 800;
static int g_clip_bottom = 600;
static RECT g_wnd_clip; /* this client area of whats actually visable */
                        /* set from WM_SIZE */

/*****************************************************************************/
static void
str_to_uni(TCHAR * sizex, char * size1)
{
  int len;
  int i;

  len = strlen(size1);
  for (i = 0; i < len; i++)
  {
    sizex[i] = size1[i];
  }
  sizex[len] = 0;
}

/*****************************************************************************/
static void
uni_to_str(char * sizex, TCHAR * size1)
{
  int len;
  int i;

  len = _tcslen(size1);
  for (i = 0; i < len; i++)
  {
    sizex[i] = (char *)size1[i];
  }
  sizex[len] = 0;
}

/*****************************************************************************/
/* returns non zero if it processed something */
static int
check_sck(void)
{
  fd_set rfds;
  struct timeval tm;
  int count;
  int rv;

  rv = 0;
  if (g_block == 0)
  {
    g_block = 1;
    /* see if there really is data */
    FD_ZERO(&rfds);
    FD_SET((unsigned int)g_tcp_sck, &rfds);
    ZeroMemory(&tm, sizeof(tm));
    count = select(g_tcp_sck + 1, &rfds, 0, 0, &tm);
    if (count > 0)
    {
      if (ui_read_wire())
      {
        rv = 1;
      }
      else
      {
        PostQuitMessage(0);
      }
    }
    g_block = 0;
  }
  return rv;
}

/*****************************************************************************/
void
mi_error(char * msg)
{
#ifdef WITH_DEBUG
  printf(msg);
#else /* WITH_DEBUG */
  TCHAR lmsg[512];
  TCHAR ltitle[512];

  str_to_uni(lmsg, msg);
  str_to_uni(ltitle, "Error");
  MessageBox(g_Wnd, lmsg, ltitle, MB_OK);
#endif /* WITH_DEBUG */
}

/*****************************************************************************/
static int
get_scan_code_from_ascii(int code)
{
  int rv;

  rv = 0;
  switch (code & 0xff)
  {
    case 0x30: rv = 0x0b; break; // 0
    case 0x31: rv = 0x02; break; // 1
    case 0x32: rv = 0x03; break; // 2
    case 0x33: rv = 0x04; break; // 3
    case 0x34: rv = 0x05; break; // 4
    case 0x35: rv = 0x06; break; // 5
    case 0x36: rv = 0x07; break; // 6
    case 0x37: rv = 0x08; break; // 7
    case 0x38: rv = 0x09; break; // 8
    case 0x39: rv = 0x0a; break; // 9

    case 0xbd: rv = 0x0c; break; // -
    case 0xbb: rv = 0x0d; break; // =
    case 0x08: rv = 0x0e; break; // backspace
    case 0x09: rv = 0x0f; break; // tab
    case 0xdb: rv = 0x1b; break; // ]
    case 0xdd: rv = 0x1a; break; // [
    case 0x14: rv = 0x3a; break; // capslock
    case 0xba: rv = 0x27; break; // ;
    case 0xde: rv = 0x28; break; // '
    case 0x10: rv = 0x2a; break; // shift
    case 0xbc: rv = 0x33; break; // ,
    case 0xbe: rv = 0x34; break; // .
    case 0xbf: rv = 0x35; break; // /
    case 0x0d: rv = 0x1c; break; // enter
    case 0x27: rv = 0x4d; break; // arrow right
    case 0x25: rv = 0x4b; break; // arrow left
    case 0x26: rv = 0x48; break; // arrow up
    case 0x28: rv = 0x50; break; // arrow down
    case 0x20: rv = 0x39; break; // space
    case 0xdc: rv = 0x2b; break; // backslash
    case 0xc0: rv = 0x29; break; // `
    case 0x11: rv = 0x1d; break; // ctl

    case 0x41: rv = 0x1e; break; // a
    case 0x42: rv = 0x30; break; // b
    case 0x43: rv = 0x2e; break; // c
    case 0x44: rv = 0x20; break; // d
    case 0x45: rv = 0x12; break; // e
    case 0x46: rv = 0x21; break; // f
    case 0x47: rv = 0x22; break; // g
    case 0x48: rv = 0x23; break; // h
    case 0x49: rv = 0x17; break; // i
    case 0x4a: rv = 0x24; break; // j
    case 0x4b: rv = 0x25; break; // k
    case 0x4c: rv = 0x26; break; // l
    case 0x4d: rv = 0x32; break; // m
    case 0x4e: rv = 0x31; break; // n
    case 0x4f: rv = 0x18; break; // o
    case 0x50: rv = 0x19; break; // p
    case 0x51: rv = 0x10; break; // q
    case 0x52: rv = 0x13; break; // r
    case 0x53: rv = 0x1f; break; // s
    case 0x54: rv = 0x14; break; // t
    case 0x55: rv = 0x16; break; // u
    case 0x56: rv = 0x2f; break; // v
    case 0x57: rv = 0x11; break; // w
    case 0x58: rv = 0x2d; break; // x
    case 0x59: rv = 0x15; break; // y
    case 0x5a: rv = 0x2c; break; // z
  }
  return rv;
}

/*****************************************************************************/
static void
mi_scroll(int dx, int dy)
{
  HRGN rgn;

  rgn = CreateRectRgn(0, 0, 0, 0);
  ScrollWindowEx(g_Wnd, dx, dy, 0, 0, rgn, 0, SW_ERASE);
  InvalidateRgn(g_Wnd, rgn, 0);
  DeleteObject(rgn);
}

/*****************************************************************************/
int
mi_read_keyboard_state(void)
{
  short keydata;
  int code;

  code = 0;
  keydata = GetKeyState(VK_SCROLL);
  if (keydata & 0x0001)
  {
    code |= 1;
  }
  keydata = GetKeyState(VK_NUMLOCK);
  if (keydata & 0x0001)
  {
    code |= 2;
  }
  keydata = GetKeyState(VK_CAPITAL);
  if (keydata & 0x0001)
  {
    code |= 4;
  }
  return code;
}

/*****************************************************************************/
static void
mi_check_modifier(void)
{
  int code;

  code = mi_read_keyboard_state();
  ui_set_modifier_state(code);
}

/*****************************************************************************/
static LRESULT
handle_WM_SETCURSOR(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  if (g_mousex >= g_wnd_clip.left &&
      g_mousey >= g_wnd_clip.top &&
      g_mousex < g_wnd_clip.right &&
      g_mousey < g_wnd_clip.bottom)
  {
    SetCursor(g_cursor);
  }
  /* need default behavoir here */
  return DefWindowProc(hWnd, message, wParam, lParam);
}

/*****************************************************************************/
static LRESULT
handle_WM_NCHITTEST(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  POINT pt;

  pt.x = LOWORD(lParam);
  pt.y = HIWORD(lParam);
  if (ScreenToClient(g_Wnd, &pt))
  {
    g_mousex = pt.x;
    g_mousey = pt.y;
  }
  return DefWindowProc(hWnd, message, wParam, lParam);
}

/*****************************************************************************/
static LRESULT
handle_WM_MOUSEMOVE(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  g_mousex = LOWORD(lParam);
  g_mousey = HIWORD(lParam);
  ui_mouse_move(g_mousex + g_xscroll, g_mousey + g_yscroll);
  return 0;
}

/*****************************************************************************/
static LRESULT
handle_WM_LBUTTONDOWN(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  g_mousex = LOWORD(lParam);
  g_mousey = HIWORD(lParam);
  ui_mouse_button(1, g_mousex + g_xscroll, g_mousey + g_yscroll, 1);
  return 0;
}

/*****************************************************************************/
static LRESULT
handle_WM_LBUTTONUP(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  g_mousex = LOWORD(lParam);
  g_mousey = HIWORD(lParam);
  ui_mouse_button(1, g_mousex + g_xscroll, g_mousey + g_yscroll, 0);
  return 0;
}

/*****************************************************************************/
static LRESULT
handle_WM_RBUTTONDOWN(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  g_mousex = LOWORD(lParam);
  g_mousey = HIWORD(lParam);
  ui_mouse_button(2, g_mousex + g_xscroll, g_mousey + g_yscroll, 1);
  return 0;
}

/*****************************************************************************/
static LRESULT
handle_WM_RBUTTONUP(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  g_mousex = LOWORD(lParam);
  g_mousey = HIWORD(lParam);
  ui_mouse_button(2, g_mousex + g_xscroll, g_mousey + g_yscroll, 0);
  return 0;
}

/*****************************************************************************/
static LRESULT
handle_WM_MBUTTONDOWN(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  g_mousex = LOWORD(lParam);
  g_mousey = HIWORD(lParam);
  ui_mouse_button(3, g_mousex + g_xscroll, g_mousey + g_yscroll, 1);
  return 0;
}

/*****************************************************************************/
static LRESULT
handle_WM_MBUTTONUP(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  g_mousex = LOWORD(lParam);
  g_mousey = HIWORD(lParam);
  ui_mouse_button(3, g_mousex + g_xscroll, g_mousey + g_yscroll, 0);
  return 0;
}

/*****************************************************************************/
static LRESULT
handle_WM_MOUSEWHEEL(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  int delta;

  delta = ((signed short)HIWORD(wParam)); /* GET_WHEEL_DELTA_WPARAM(wParam); */
  if (delta > 0)
  {
    ui_mouse_button(4, 0, 0, 1);
    ui_mouse_button(4, 0, 0, 0);
  }
  else
  {
    ui_mouse_button(5, 0, 0, 1);
    ui_mouse_button(5, 0, 0, 0);
  }
  return 0;
}

/*****************************************************************************/
static LRESULT
handle_WM_KEY(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  int scancode;
  int ext;
  int down;

  ext = HIWORD(lParam);
  scancode = ext;
  down = !(ext & 0x8000);
  scancode &= 0xff;
  if (scancode == 0)
  {
    scancode = get_scan_code_from_ascii(wParam);
  }
  ext &= 0x0100;
  if (scancode == 0x0045) /* num lock */
  {
    ext &= ~0x0100;
  }
  if (down)
  {
    ui_key_down(scancode, ext);
  }
  else
  {
    ui_key_up(scancode, ext);
  }
  return 0;
}

/*****************************************************************************/
static LRESULT
handle_WM_PAINT(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  PAINTSTRUCT ps;
  RECT rect;
  HBRUSH brush;

  BeginPaint(hWnd, &ps);
  /* paint the area outside the rdp screen with one colour */
  rect = ps.rcPaint;
  rect.left = UI_MAX(rect.left, g_width);
  if (!IsRectEmpty(&rect))
  {
    brush = CreateSolidBrush(RGB(0, 0, 255));
    FillRect(ps.hdc, &rect, brush);
    DeleteObject(brush);
  }
  rect = ps.rcPaint;
  rect.top = UI_MAX(rect.top, g_height);
  if (!IsRectEmpty(&rect))
  {
    brush = CreateSolidBrush(RGB(0, 0, 255));
    FillRect(ps.hdc, &rect, brush);
    DeleteObject(brush);
  }
  rect = ps.rcPaint;
  EndPaint(hWnd, &ps);
  ui_invalidate(rect.left + g_xscroll,
                rect.top + g_yscroll,
                (rect.right - rect.left) + 1,
                (rect.bottom - rect.top) + 1);
  return 0;
}

/*****************************************************************************/
static LRESULT
handle_WM_SIZE(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  int oldxscroll;
  int oldyscroll;

  if (wParam == SIZE_MINIMIZED)
  {
    return DefWindowProc(hWnd, message, wParam, lParam);
  }
  g_wnd_cwidth = LOWORD(lParam); /* client width / height */
  g_wnd_cheight = HIWORD(lParam);
  g_wnd_clip.left = 0;
  g_wnd_clip.top = 0;
  g_wnd_clip.right = g_wnd_clip.left + g_wnd_cwidth;
  g_wnd_clip.bottom = g_wnd_clip.top + g_wnd_cheight;
  if (g_wnd_cwidth < g_width || g_wnd_cheight < g_height)
  {
    SetScrollRange(g_Wnd, SB_HORZ, 0, g_width - g_wnd_cwidth, 1);
    SetScrollRange(g_Wnd, SB_VERT, 0, g_height - g_wnd_cheight, 1);
  }
  oldxscroll = g_xscroll;
  oldyscroll = g_yscroll;
  if (g_wnd_cwidth >= g_width)
  {
    g_xscroll = 0;
  }
  else
  {
    g_xscroll = UI_MIN(g_xscroll, g_width - g_wnd_cwidth);
  }
  if (g_wnd_cheight >= g_height)
  {
    g_yscroll = 0;
  }
  else
  {
    g_yscroll = UI_MIN(g_yscroll, g_height - g_wnd_cheight);
  }
  mi_scroll(oldxscroll - g_xscroll, oldyscroll - g_yscroll);
  if (wParam == SIZE_RESTORED || wParam == SIZE_MAXIMIZED)
  {
    /* check the caps, num, and scroll lock here */
    mi_check_modifier();
  }
  return 0;
}

/*****************************************************************************/
static LRESULT
handle_WM_SIZING(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  LPRECT prect;
  int width;
  int height;
  int style;

  prect = (LPRECT) lParam; /* total window rect */
  width = (prect->right - prect->left) - (g_xoff * 2);
  height = (prect->bottom - prect->top) - (g_yoff + g_xoff);
  if (height < g_height || width < g_width)
  {
    style = GetWindowLong(g_Wnd, GWL_STYLE);
    if (!(style & WS_HSCROLL))
    {
      style |= WS_HSCROLL | WS_VSCROLL;
      SetWindowLong(g_Wnd, GWL_STYLE, style);
      g_xscroll = 0;
      g_yscroll = 0;
      SetScrollPos(g_Wnd, SB_HORZ, g_xscroll, 1);
      SetScrollPos(g_Wnd, SB_VERT, g_yscroll, 1);
    }
  }
  else if (height >= g_height && width >= g_width)
  {
    style = GetWindowLong(g_Wnd, GWL_STYLE);
    if (style & WS_HSCROLL)
    {
      style &= ~WS_HSCROLL;
      style &= ~WS_VSCROLL;
      SetWindowLong(g_Wnd, GWL_STYLE, style);
      g_xscroll = 0;
      g_yscroll = 0;
    }
  }
  return 0;
}

/*****************************************************************************/
static LRESULT
handle_WM_HSCROLL(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  int code;
  int oldxscroll;

  code = (int) LOWORD(wParam); /* scroll bar value */
  if (code == SB_LINELEFT)
  {
    oldxscroll = g_xscroll;
    g_xscroll--;
    g_xscroll = UI_MAX(g_xscroll, 0);
    SetScrollPos(g_Wnd, SB_HORZ, g_xscroll, 1);
    mi_scroll(oldxscroll - g_xscroll, 0);
  }
  else if (code == SB_LINERIGHT)
  {
    oldxscroll = g_xscroll;
    g_xscroll++;
    g_xscroll = UI_MIN(g_xscroll, g_width - g_wnd_cwidth);
    SetScrollPos(g_Wnd, SB_HORZ, g_xscroll, 1);
    mi_scroll(oldxscroll - g_xscroll, 0);
  }
  else if (code == SB_PAGELEFT)
  {
    oldxscroll = g_xscroll;
    g_xscroll -= g_wnd_cwidth / 2;
    g_xscroll = UI_MAX(g_xscroll, 0);
    SetScrollPos(g_Wnd, SB_HORZ, g_xscroll, 1);
    mi_scroll(oldxscroll - g_xscroll, 0);
  }
  else if (code == SB_PAGERIGHT)
  {
    oldxscroll = g_xscroll;
    g_xscroll += g_wnd_cwidth / 2;
    g_xscroll = UI_MIN(g_xscroll, g_width - g_wnd_cwidth);
    SetScrollPos(g_Wnd, SB_HORZ, g_xscroll, 1);
    mi_scroll(oldxscroll - g_xscroll, 0);
  }
  else if (code == SB_BOTTOM)
  {
    oldxscroll = g_xscroll;
    g_xscroll = g_width - g_wnd_cwidth;
    SetScrollPos(g_Wnd, SB_HORZ, g_xscroll, 1);
    mi_scroll(oldxscroll - g_xscroll, 0);
  }
  else if (code == SB_TOP)
  {
    oldxscroll = g_xscroll;
    g_xscroll = 0;
    SetScrollPos(g_Wnd, SB_HORZ, g_xscroll, 1);
    mi_scroll(oldxscroll - g_xscroll, 0);
  }
  else if (code == SB_THUMBPOSITION)
  {
    oldxscroll = g_xscroll;
    g_xscroll = (signed short) HIWORD(wParam);
    SetScrollPos(g_Wnd, SB_HORZ, g_xscroll, 1);
    mi_scroll(oldxscroll - g_xscroll, 0);
  }
  return 0;
}

/*****************************************************************************/
static LRESULT
handle_WM_VSCROLL(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  int code;
  int oldyscroll;

  code = (int) LOWORD(wParam); /* scroll bar value */
  if (code == SB_LINELEFT)
  {
    oldyscroll = g_yscroll;
    g_yscroll--;
    g_yscroll = UI_MAX(g_yscroll, 0);
    SetScrollPos(g_Wnd, SB_VERT, g_yscroll, 1);
    mi_scroll(0, oldyscroll - g_yscroll);
  }
  else if (code == SB_LINERIGHT)
  {
    oldyscroll = g_yscroll;
    g_yscroll++;
    g_yscroll = UI_MIN(g_yscroll, g_height - g_wnd_cheight);
    SetScrollPos(g_Wnd, SB_VERT, g_yscroll, 1);
    mi_scroll(0, oldyscroll - g_yscroll);
  }
  else if (code == SB_PAGELEFT)
  {
    oldyscroll = g_yscroll;
    g_yscroll -= g_wnd_cheight / 2;
    g_yscroll = UI_MAX(g_yscroll, 0);
    SetScrollPos(g_Wnd, SB_VERT, g_yscroll, 1);
    mi_scroll(0, oldyscroll - g_yscroll);
  }
  else if (code == SB_PAGERIGHT)
  {
    oldyscroll = g_yscroll;
    g_yscroll += g_wnd_cheight / 2;
    g_yscroll = UI_MIN(g_yscroll, g_height - g_wnd_cheight);
    SetScrollPos(g_Wnd, SB_VERT, g_yscroll, 1);
    mi_scroll(0, oldyscroll - g_yscroll);
  }
  else if (code == SB_BOTTOM)
  {
    oldyscroll = g_yscroll;
    g_yscroll = g_height - g_wnd_cheight;
    SetScrollPos(g_Wnd, SB_VERT, g_yscroll, 1);
    mi_scroll(0, oldyscroll - g_yscroll);
  }
  else if (code == SB_TOP)
  {
    oldyscroll = g_yscroll;
    g_yscroll = 0;
    SetScrollPos(g_Wnd, SB_VERT, g_yscroll, 1);
    mi_scroll(0, oldyscroll - g_yscroll);
  }
  else if (code == SB_THUMBPOSITION)
  {
    oldyscroll = g_yscroll;
    g_yscroll = (signed short) HIWORD(wParam);
    SetScrollPos(g_Wnd, SB_VERT, g_yscroll, 1);
    mi_scroll(0, oldyscroll - g_yscroll);
  }
  return 0;
}


/*****************************************************************************/
LRESULT CALLBACK
WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  switch (message)
  {
    case WM_SETCURSOR:
      return handle_WM_SETCURSOR(hWnd, message, wParam, lParam);
    case 0x0084: /* WinCE don't have this WM_NCHITTEST: */
      return handle_WM_NCHITTEST(hWnd, message, wParam, lParam);
    case WM_MOUSEMOVE:
      return handle_WM_MOUSEMOVE(hWnd, message, wParam, lParam);
    case WM_LBUTTONDOWN:
      return handle_WM_LBUTTONDOWN(hWnd, message, wParam, lParam);
    case WM_LBUTTONUP:
      return handle_WM_LBUTTONUP(hWnd, message, wParam, lParam);
    case WM_RBUTTONDOWN:
      return handle_WM_RBUTTONDOWN(hWnd, message, wParam, lParam);
    case WM_RBUTTONUP:
      return handle_WM_RBUTTONUP(hWnd, message, wParam, lParam);
    case WM_MBUTTONDOWN:
      return handle_WM_MBUTTONDOWN(hWnd, message, wParam, lParam);
    case WM_MBUTTONUP:
      return handle_WM_MBUTTONUP(hWnd, message, wParam, lParam);
    /* some windows compilers don't have these defined like vc6 */
    case 0x020a: /* WM_MOUSEWHEEL: */
      return handle_WM_MOUSEWHEEL(hWnd, message, wParam, lParam);
    case WM_KEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
      return handle_WM_KEY(hWnd, message, wParam, lParam);
    case WM_CHAR:
    case WM_DEADCHAR:
    case WM_SYSCHAR:
    case WM_SYSDEADCHAR:
      break;
    case WM_PAINT:
      return handle_WM_PAINT(hWnd, message, wParam, lParam);
    case WM_DESTROY:
      PostQuitMessage(0);
      break;
    case WM_APP + 1:
    case WM_TIMER:
      check_sck();
      break;
    case WM_SIZE:
      return handle_WM_SIZE(hWnd, message, wParam, lParam);
    case 532: /* not defined in wince WM_SIZING: */
      return handle_WM_SIZING(hWnd, message, wParam, lParam);
    case WM_HSCROLL:
      return handle_WM_HSCROLL(hWnd, message, wParam, lParam);
    case WM_VSCROLL:
      return handle_WM_VSCROLL(hWnd, message, wParam, lParam);
    case WM_SETFOCUS:
      mi_check_modifier();
      return DefWindowProc(hWnd, message, wParam, lParam);
    default:
      return DefWindowProc(hWnd, message, wParam, lParam);
  }
  return 0;
}

/*****************************************************************************/
static HRGN
mi_clip(HDC dc)
{
  HRGN rgn;

  rgn = CreateRectRgn(g_clip_left + g_xoff - g_xscroll,
                      g_clip_top + g_yoff - g_yscroll,
                      g_clip_right + g_xoff - g_xscroll,
                      g_clip_bottom + g_yoff - g_yscroll);
  SelectClipRgn(dc, rgn);
  IntersectClipRect(dc, g_wnd_clip.left + g_xoff, g_wnd_clip.top + g_yoff,
                    g_wnd_clip.right + g_xoff, g_wnd_clip.bottom + g_yoff);
  return rgn;
}

/*****************************************************************************/
/* returns non zero if ok */
int
mi_create_window(void)
{
  RECT rc;
  WNDCLASS wc;
  TCHAR classname[512];
  TCHAR caption[512];
  DWORD style;
  int x;
  int y;
  int w;
  int h;

  if (g_Wnd != 0 || g_Instance != 0)
  {
    return 0;
  }
  g_Instance = GetModuleHandle(NULL);
  ZeroMemory(&wc, sizeof(wc));
  wc.lpfnWndProc = WndProc; /* points to window procedure */
  /* name of window class */
  str_to_uni(classname, "rdesktop");
  wc.lpszClassName = classname;
  str_to_uni(caption, "ReactOS Remote Desktop");
  wc.hIcon = LoadIcon(g_Instance,
                      MAKEINTRESOURCE(IDI_MSTSC));
  /* Register the window class. */
  if (!RegisterClass(&wc))
  {
    return 0; /* Failed to register window class */
  }
  rc.left = 0;
  rc.right = rc.left + UI_MIN(g_width, g_screen_width);
  rc.top = 0;
  rc.bottom = rc.top + UI_MIN(g_height, g_screen_height);

  if (g_fullscreen)
  {
    style = WS_POPUP;
  }
  else
  {
    style = WS_OVERLAPPED | WS_CAPTION | WS_POPUP | WS_MINIMIZEBOX |
            WS_SYSMENU | WS_SIZEBOX | WS_MAXIMIZEBOX;
  }
  if (g_screen_width < g_width || g_screen_height < g_height)
  {
    style |= WS_HSCROLL | WS_VSCROLL;
  }
  AdjustWindowRectEx(&rc, style, 0, 0);
  x = CW_USEDEFAULT;
  y = CW_USEDEFAULT;
  w = rc.right - rc.left;
  h = rc.bottom - rc.top;

  g_Wnd = CreateWindow(wc.lpszClassName, caption,
                       style, x, y, w, h,
                       (HWND) NULL, (HMENU) NULL, g_Instance,
                       (LPVOID) NULL);
  g_clip_left = 0;
  g_clip_top = 0;
  g_clip_right = g_clip_left + g_width;
  g_clip_bottom = g_clip_top + g_height;
  if (g_workarea)
  {
    ShowWindow(g_Wnd, SW_SHOWMAXIMIZED);
  }
  else
  {
    ShowWindow(g_Wnd, SW_SHOWNORMAL);
  }
  UpdateWindow(g_Wnd);

  WSAAsyncSelect(g_tcp_sck, g_Wnd, WM_APP + 1, FD_READ);
  SetTimer(g_Wnd, 1, 333, 0);

  return 1;
}

/*****************************************************************************/
int
mi_main_loop(void)
{
  MSG msg;

  while (GetMessage(&msg, NULL, 0, 0))
  {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
  return msg.wParam;
}

/*****************************************************************************/
void
mi_warning(char * msg)
{
}

/*****************************************************************************/
static void
mi_show_error(char * caption)
{
  LPVOID lpMsgBuf;
  TCHAR lcaption[512];

  FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPTSTR) &lpMsgBuf, 0, NULL);
#ifdef WITH_DEBUG
  printf(lpMsgBuf);
#else /* WITH_DEBUG */
  str_to_uni(lcaption, caption);
  MessageBox(g_Wnd, (LPTSTR) lpMsgBuf, lcaption,
             MB_OK | MB_ICONINFORMATION);
#endif /* WITH_DEBUG */
  LocalFree(lpMsgBuf);
}

/*****************************************************************************/
void
mi_paint_rect(char * data, int width, int height, int x, int y, int cx, int cy)
{
  HBITMAP bitmap;
  BITMAPINFO bi;
  HDC dc;
  HDC maindc;
  HGDIOBJ save;
  HRGN rgn;
  void * bits;
  int i;
  int j;
  int colour;
  int red;
  int green;
  int blue;

  ZeroMemory(&bi, sizeof(bi));
  bi.bmiHeader.biSize = sizeof(bi.bmiHeader);
  bi.bmiHeader.biWidth = width;
  bi.bmiHeader.biHeight = -height;
  bi.bmiHeader.biPlanes = 1;
  bi.bmiHeader.biBitCount = 32;
  bi.bmiHeader.biCompression = BI_RGB;
  maindc = GetWindowDC(g_Wnd);
  bitmap = CreateDIBSection(maindc, &bi, DIB_RGB_COLORS, (void **) &bits, 0, 0);
  if (bitmap == 0)
  {
    mi_show_error("CreateDIBSection failed");
  }

  if (g_server_depth == 8)
  {
    for (i = cy - 1; i >= 0; i--)
    {
      for (j = cx - 1; j >= 0; j--)
      {
        colour = ((unsigned char*)data)[i * cx + j];
        red = (pal_entries[colour & 0xff] & 0xff0000) >> 16;
        green = (pal_entries[colour & 0xff] & 0xff00) >> 8;
        blue = pal_entries[colour & 0xff] & 0xff;
        MAKE_COLOUR32(colour, red, green, blue);
        ((unsigned int*)bits)[i * cx + j] = colour;
      }
    }
  }
  else if (g_server_depth == 15)
  {
    for (i = cy - 1; i >= 0; i--)
    {
      for (j = cx - 1; j >= 0; j--)
      {
        colour = ((unsigned short*)data)[i * cx + j];
        SPLIT_COLOUR15(colour, red, green, blue);
        MAKE_COLOUR32(colour, red, green, blue);
        ((unsigned int*)bits)[i * cx + j] = colour;
      }
    }
  }
  else if (g_server_depth == 16)
  {
    for (i = cy - 1; i >= 0; i--)
    {
      for (j = cx - 1; j >= 0; j--)
      {
        colour = ((unsigned short*)data)[i * cx + j];
        SPLIT_COLOUR16(colour, red, green, blue);
        MAKE_COLOUR32(colour, red, green, blue);
        ((unsigned int*)bits)[i * cx + j] = colour;
      }
    }
  }
  dc = CreateCompatibleDC(maindc);
  if (dc == 0)
  {
    mi_show_error("CreateCompatibleDC failed");
  }
  save = SelectObject(dc, bitmap);
  rgn = mi_clip(maindc);
  BitBlt(maindc, x + g_xoff - g_xscroll, y + g_yoff - g_yscroll, cx, cy, dc,
         0, 0, SRCCOPY);
  SelectObject(dc, save);
  DeleteObject(bitmap);
  DeleteDC(dc);
  ReleaseDC(g_Wnd, maindc);
  DeleteObject(rgn);

}

#if 0
/*****************************************************************************/
static int
mi_process_a_param(char * param1, int state)
{
  char * p;

  if (state == 0)
  {
    if (strcmp(param1, "-g") == 0 || strcmp(param1, "geometry") == 0)
    {
      state = 1;
    }
    if (strcmp(param1, "-t") == 0 || strcmp(param1, "port") == 0)
    {
      state = 2;
    }
    if (strcmp(param1, "-a") == 0 || strcmp(param1, "bpp") == 0)
    {
      state = 3;
    }
    if (strcmp(param1, "-f") == 0 || strcmp(param1, "fullscreen") == 0)
    {
      g_fullscreen = 1;
    }
    if (strcmp(param1, "-u") == 0 || strcmp(param1, "username") == 0)
    {
      state = 5;
    }
    if (strcmp(param1, "-p") == 0 || strcmp(param1, "password") == 0)
    {
      state = 6;
    }
    if (strcmp(param1, "-d") == 0 || strcmp(param1, "domain") == 0)
    {
      state = 7;
    }
    if (strcmp(param1, "-s") == 0 || strcmp(param1, "shell") == 0)
    {
      state = 8;
    }
    if (strcmp(param1, "-c") == 0 || strcmp(param1, "directory") == 0)
    {
      state = 9;
    }
    if (strcmp(param1, "-n") == 0 || strcmp(param1, "hostname") == 0)
    {
      state = 10;
    }
  }
  else
  {
    if (state == 1) /* -g widthxheight*/
    {
      state = 0;
      if (strcmp(param1, "workarea") == 0)
      {
        g_workarea = 1;
        return state;
      }
      g_width = strtol(param1, &p, 10);
      if (g_width <= 0)
      {
        mi_error("invalid geometry\r\n");
      }
      if (*p == 'x')
      {
        g_height = strtol(p + 1, &p, 10);
      }
      if (g_height <= 0)
      {
        mi_error("invalid geometry\r\n");
      }
      g_width_height_set = 1;
    }
    if (state == 2) /* -t port */
    {
      state = 0;
      g_tcp_port_rdp = atol(param1);
    }
    if (state == 3) /* -a bpp */
    {
      state = 0;
      g_server_depth = atol(param1);
      if (g_server_depth != 8 && g_server_depth != 15 && g_server_depth != 16 && g_server_depth != 24)
      {
        mi_error("invalid server bpp\r\n");
      }
    }
    if (state == 5) /* -u username */
    {
      state = 0;
      strcpy(g_username, param1);
    }
    if (state == 6) /* -p password */
    {
      state = 0;
      strcpy(g_password, param1);
    }
    if (state == 7) /* -d domain */
    {
      state = 0;
      strcpy(g_domain, param1);
    }
    if (state == 8) /* -s shell */
    {
      state = 0;
      strcpy(g_shell, param1);
    }
    if (state == 9) /* -c workin directory*/
    {
      state = 0;
      strcpy(g_directory, param1);
    }
    if (state == 10) /* -n host name*/
    {
      state = 0;
      strcpy(g_hostname, param1);
    }
  }
  return state;
}

/*****************************************************************************/
static int
mi_post_param(void)
{
  /* after parameters */
  if (g_fullscreen)
  {
    g_xoff = 0;
    g_yoff = 0;
    if (!g_width_height_set)
    {
      g_width = g_screen_width;
      g_height = g_screen_height;
    }
  }
  else if (g_workarea)
  {
    g_xoff = GetSystemMetrics(SM_CXEDGE) * 2;
    g_yoff = GetSystemMetrics(SM_CYCAPTION) +
             GetSystemMetrics(SM_CYEDGE) * 2;
    g_width = g_screen_width;
    g_height = g_screen_height;
    g_height = (g_height - g_yoff) - g_xoff - 20; /* todo */
    g_width_height_set = 1;
  }
  else
  {
    g_xoff = GetSystemMetrics(SM_CXEDGE) * 2;
    g_yoff = GetSystemMetrics(SM_CYCAPTION) +
             GetSystemMetrics(SM_CYEDGE) * 2;
  }
  g_width = g_width & (~3);
  return 1;
}


/*****************************************************************************/
static int
mi_check_config_file(void)
{
  HANDLE fd;
  DWORD count;
  TCHAR filename[256];
  char buffer[256];
  char vname[256];
  char value[256];
  int index;
  int mode;
  int vnameindex;
  int valueindex;
  int state;
  int rv;

  rv = 0;
  mode = 0;
  vnameindex = 0;
  valueindex = 0;
  vname[vnameindex] = 0;
  value[valueindex] = 0;
  str_to_uni(filename, ".\\winrdesktop.ini");

  fd = CreateFile(filename, GENERIC_READ,
                  FILE_SHARE_READ | FILE_SHARE_WRITE,
                  NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  count = 255;
  while (ReadFile(fd, buffer, count, &count, NULL))
  {
    if (count == 0)
    {
      break;
    }
    buffer[count] = 0;
    index = 0;
    while (index < (int) count)
    {
      if (buffer[index] == '=')
      {
        mode = 1;
      }
      else if (buffer[index] == 10 || buffer[index] == 13)
      {
        mode = 0;
        vname[vnameindex] = 0;
        value[valueindex] = 0;
        if (strlen(vname) > 0 || strlen(value) > 0)
        {
          if (strcmp(vname, "server") == 0)
          {
            strcpy(g_servername, value);
            rv = 1;
          }
          else
          {
            state = mi_process_a_param(vname, 0);
            mi_process_a_param(value, state);
          }
        }
        vnameindex = 0;
        valueindex = 0;
      }
      else if (mode == 0)
      {
        vname[vnameindex] = buffer[index];
        vnameindex++;
      }
      else
      {
        value[valueindex] = buffer[index];
        valueindex++;
      }
      index++;
    }
    count = 255;
  }
  CloseHandle(fd);
  if (rv)
  {
    mi_post_param();
  }
  return rv;
}


/*****************************************************************************/
/* process the command line parameters */
/* returns boolean, non zero is ok */
static int
mi_process_cl(LPTSTR lpCmdLine)
{
  char param[256];
  char param1[256];
  TCHAR l_username[256];
  DWORD size;
  int len;
  int i;
  int i1;
  int state;

  strcpy(g_hostname, "test");
  strcpy(g_username, "pda");
  /* get username and convert it from unicode */
  size = 255;

  if (GetUserName(l_username, &size))
  {
    for (i = size; i >= 0; i--)
    {
      g_username[i] = (char) l_username[i];
    }
    g_username[size] = 0;
  }
  else
  {
    mi_show_error("GetUserName");
  }

  /* get computer name */
  if (gethostname(g_hostname, 255) != 0)
  {
    mi_show_error("gethostname");
  }
  /* defaults */
  strcpy(g_servername, "127.0.0.1");
  g_server_depth = 8;
  g_screen_width = GetSystemMetrics(SM_CXSCREEN);
  g_screen_height = GetSystemMetrics(SM_CYSCREEN);
  /* process parameters */
  i1 = 0;
  state = 0;
  len = lstrlen(lpCmdLine);
  if (len == 0)
  {
    return mi_check_config_file();
  }
  for (i = 0; i < len; i++)
  {
    if (lpCmdLine[i] != 32 && lpCmdLine[i] != 9) /* space or tab */
    {
      param[i1] = (char) lpCmdLine[i];
      i1++;
    }
    else
    {
      param[i1] = 0;
      i1 = 0;
      strcpy(param1, param);
      state = mi_process_a_param(param1, state);
      strcpy(g_servername, param1);
    }
  }
  if (i1 > 0)
  {
    param[i1] = 0;
    strcpy(param1, param);
    state = mi_process_a_param(param1, state);
    strcpy(g_servername, param1);
  }
  if (state == 0)
  {
    mi_post_param();
  }
  return (state == 0);
}
#endif

/*****************************************************************************/
int WINAPI
wWinMain(HINSTANCE hInstance,
        HINSTANCE hPrevInstance,
        LPWSTR lpCmdLine,
        int nCmdShow)
{
    PRDPSETTINGS pRdpSettings;
    WSADATA d;
    int ret = 1;

    if (WSAStartup(MAKEWORD(2, 0), &d) == 0)
    {
        pRdpSettings = HeapAlloc(GetProcessHeap(),
                                 0,
                                 sizeof(RDPSETTINGS));
        if (pRdpSettings)
        {
            pRdpSettings->pSettings = NULL;
            pRdpSettings->NumSettings = 0;

            if (InitRdpSettings(pRdpSettings))
            {
                LoadRdpSettingsFromFile(pRdpSettings, NULL);

                //mi_process_cl(lpCmdLine)
                if (OpenRDPConnectDialog(hInstance,
                                         pRdpSettings))
                {
                    char szValue[MAXVALUE];

                    uni_to_str(szValue, GetStringFromSettings(pRdpSettings, L"full address"));

                    strcpy(g_servername, szValue);
                    //g_port = 3389;
                    strcpy(g_username, "");
                    strcpy(g_password, "");
                    g_server_depth = GetIntegerFromSettings(pRdpSettings, L"session bpp");
                    if (g_server_depth > 16) g_server_depth = 16;  /* hack, we don't support 24bpp yet */
                    g_width = GetIntegerFromSettings(pRdpSettings, L"desktopwidth");
                    g_height = GetIntegerFromSettings(pRdpSettings, L"desktopheight");
                    g_screen_width = GetSystemMetrics(SM_CXSCREEN);
                    g_screen_height = GetSystemMetrics(SM_CYSCREEN);
                    g_xoff = GetSystemMetrics(SM_CXEDGE) * 2;
                    g_yoff = GetSystemMetrics(SM_CYCAPTION) + GetSystemMetrics(SM_CYEDGE) * 2;

                    ui_main();
                    ret = 0;
                }

                HeapFree(GetProcessHeap(),
                         0,
                         pRdpSettings->pSettings);
            }

            HeapFree(GetProcessHeap(),
                     0,
                     pRdpSettings);
      }

        WSACleanup();
    }

  return ret;
}


/*****************************************************************************/
void
mi_begin_update(void)
{
}

/*****************************************************************************/
void
mi_end_update(void)
{
}

/*****************************************************************************/
void
mi_fill_rect(int x, int y, int cx, int cy, int colour)
{
  HBRUSH brush;
  RECT rect;
  HDC maindc;
  HRGN rgn;
  int red;
  int green;
  int blue;

  if (g_server_depth == 8)
  {
    red = (pal_entries[colour & 0xff] & 0xff0000) >> 16;
    green = (pal_entries[colour & 0xff] & 0xff00) >> 8;
    blue = pal_entries[colour & 0xff] & 0xff;
  }
  else if (g_server_depth == 15)
  {
    SPLIT_COLOUR15(colour, red, green, blue);
  }
  else if (g_server_depth == 16)
  {
    SPLIT_COLOUR16(colour, red, green, blue);
  }
  else
  {
    red = 0;
    green = 0;
    blue = 0;
  }
  maindc = GetWindowDC(g_Wnd);
  rgn = mi_clip(maindc);
  brush = CreateSolidBrush(RGB(red, green, blue));
  rect.left = x + g_xoff - g_xscroll;
  rect.top = y + g_yoff - g_yscroll;
  rect.right = rect.left + cx;
  rect.bottom = rect.top + cy;
  FillRect(maindc, &rect, brush);
  DeleteObject(brush);
  ReleaseDC(g_Wnd, maindc);
  DeleteObject(rgn);
}

/*****************************************************************************/
void
mi_line(int x1, int y1, int x2, int y2, int colour)
{
  HPEN pen;
  HDC maindc;
  HGDIOBJ save;
  HRGN rgn;
  int red;
  int green;
  int blue;

  if (g_server_depth == 8)
  {
    red = (pal_entries[colour & 0xff] & 0xff0000) >> 16;
    green = (pal_entries[colour & 0xff] & 0xff00) >> 8;
    blue = pal_entries[colour & 0xff] & 0xff;
  }
  else if (g_server_depth == 15)
  {
    SPLIT_COLOUR15(colour, red, green, blue);
  }
  else if (g_server_depth == 16)
  {
    SPLIT_COLOUR16(colour, red, green, blue);
  }
  else
  {
    red = 0;
    green = 0;
    blue = 0;
  }
  maindc = GetWindowDC(g_Wnd);
  rgn = mi_clip(maindc);
  pen = CreatePen(PS_SOLID, 0, RGB(red, green, blue));
  save = SelectObject(maindc, pen);
  MoveToEx(maindc, x1 + g_xoff - g_xscroll, y1 + g_yoff - g_yscroll, 0);
  LineTo(maindc, x2 + g_xoff - g_xscroll, y2 + g_yoff - g_yscroll);
  SelectObject(maindc, save);
  DeleteObject(pen);
  ReleaseDC(g_Wnd, maindc);
  DeleteObject(rgn);
}

/*****************************************************************************/
void
mi_screen_copy(int x, int y, int cx, int cy, int srcx, int srcy)
{
  RECT rect;
  RECT clip_rect;
  RECT draw_rect;
  HRGN rgn;
  int ok_to_ScrollWindowEx;

  ok_to_ScrollWindowEx = 1;

  if (!ok_to_ScrollWindowEx)
  {
    rgn = CreateRectRgn(x - g_xscroll, y - g_yscroll,
                        (x - g_xscroll) + cx,
                        (y - g_yscroll) + cy);
    InvalidateRgn(g_Wnd, rgn, 0);
    DeleteObject(rgn);
  }
  else
  {
    /* this is all in client coords */
    rect.left = srcx - g_xscroll;
    rect.top = srcy - g_yscroll;
    rect.right = rect.left + cx;
    rect.bottom = rect.top + cy;
    clip_rect.left = g_clip_left - g_xscroll;
    clip_rect.top = g_clip_top - g_yscroll;
    clip_rect.right = g_clip_right - g_xscroll;
    clip_rect.bottom = g_clip_bottom - g_yscroll;
    if (IntersectRect(&draw_rect, &clip_rect, &g_wnd_clip))
    {
      rgn = CreateRectRgn(0, 0, 0, 0);
      ScrollWindowEx(g_Wnd, x - srcx, y - srcy, &rect, &draw_rect,
                     rgn, 0, SW_ERASE);
      InvalidateRgn(g_Wnd, rgn, 0);
      DeleteObject(rgn);
    }
  }
}

/*****************************************************************************/
void
mi_set_clip(int x, int y, int cx, int cy)
{
  g_clip_left = x;
  g_clip_top = y;
  g_clip_right = g_clip_left + cx;
  g_clip_bottom = g_clip_top + cy;
}

/*****************************************************************************/
void
mi_reset_clip(void)
{
  g_clip_left = 0;
  g_clip_top = 0;
  g_clip_right = g_clip_left + g_width;
  g_clip_bottom = g_clip_top + g_height;
}

/*****************************************************************************/
void *
mi_create_cursor(unsigned int x, unsigned int y,
                 int width, int height,
                 unsigned char * andmask, unsigned char * xormask)
{
  HCURSOR hCur;

  hCur = CreateCursor(g_Instance, x, y, width, height, andmask, xormask);
  if (hCur == 0)
  {
    hCur = LoadCursor(NULL, IDC_ARROW);
  }
  return hCur;
}

/*****************************************************************************/
void
mi_destroy_cursor(void * cursor)
{
  if (g_cursor == cursor)
  {
    g_cursor = 0;
  }
  DestroyCursor(cursor);
}

/*****************************************************************************/
void
mi_set_cursor(void * cursor)
{
  g_cursor = cursor;
  SetCursor(g_cursor);
}

/*****************************************************************************/
void
mi_set_null_cursor(void)
{
}


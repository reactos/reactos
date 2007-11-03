/* -*- c-basic-offset: 8 -*-
   rdesktop: A Remote Desktop Protocol client.
   main ui header
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

/* in uimain.c */
int
ui_main(void);
void
ui_invalidate(int x, int y, int cx, int cy);
int
ui_read_wire(void);
void
ui_mouse_move(int x, int y);
void
ui_mouse_button(int button, int x, int y, int down);
void
ui_key_down(int key, int ext);
void
ui_key_up(int key, int ext);

void

ui_set_modifier_state(int code);

#define SPLIT_COLOUR15(c, r, g, b) \
{ \
  r = ((c >> 7) & 0xf8) | ((c >> 12) & 0x7); \
  g = ((c >> 2) & 0xf8) | ((c >>  8) & 0x7); \
  b = ((c << 3) & 0xf8) | ((c >>  2) & 0x7); \
}

#define SPLIT_COLOUR16(c, r, g, b) \
{ \
  r = ((c >> 8) & 0xf8) | ((c >> 13) & 0x7); \
  g = ((c >> 3) & 0xfc) | ((c >>  9) & 0x3); \
  b = ((c << 3) & 0xf8) | ((c >>  2) & 0x7); \
}

#define MAKE_COLOUR15(c, r, g, b) \
{ \
  c = ( \
        (((r & 0xff) >> 3) << 10) | \
        (((g & 0xff) >> 3) <<  5) | \
        (((b & 0xff) >> 3) <<  0) \
      ); \
}

#define MAKE_COLOUR32(c, r, g, b) \
{ \
  c = ( \
        ((r & 0xff) << 16) | \
        ((g & 0xff) <<  8) | \
        ((b & 0xff) <<  0) \
      ); \
}

#undef UI_MAX
#define UI_MAX(a, b)  (((a) > (b)) ? (a) : (b))
#undef UI_MIN
#define UI_MIN(a, b)  (((a) < (b)) ? (a) : (b))

/* in connectdialog.c */
BOOL OpenRDPConnectDialog();


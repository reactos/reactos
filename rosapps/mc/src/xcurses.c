/* Glue code to run with bsd curses.
   Copyright (C) 1994 Miguel de Icaza
   
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

#include <config.h>
#include "tty.h"

int has_colors (void)
{
    return 0;
}

int init_pair (int pair, int fore, int back)
{
    return 0;
}

int start_color ()
{
    return 0;
}

#ifndef SUNOS_CURSES
void hline (int character, int len)
{
    int i, x, y;

    getyx (stdscr, y, x);
    while (len--)
      addch (character);
    move(y, x);
}

void vline (int character, int len)
{
  int i, x, y;

  getyx (stdscr, y, x);
  while (len--)
      mvaddch (y++, x, character);
}
#endif


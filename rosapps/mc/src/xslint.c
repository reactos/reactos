/* Routines expected by the Midnight Commander
   
   Copyright (C) 1999 The Free Software Foundation.

   Author Miguel de Icaza

   FIXME: This expects the user to always use slang instead of ncurses.
   
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

   This file just has dummy procedures that don't do nothing under X11
   editions. I will make macros once I feel right with the Tk edition.

   */

#include <stdio.h>

int
interrupts_enabled (void) { return 0; }

void
enable_interrupt_key(void) {}
   
void
disable_interrupt_key(void) {}

/* FIXME: We could provide a better way of doing this */
int
got_interrupt () { return 0; }

void
slang_init (void) {}

void
slang_set_raw_mode (void) {}

void
slang_prog_mode (void) {}

void
slang_shell_mode (void) {}

void
slang_shutdown () {}

void
slang_keypad (int set) {}

void
set_slang_delay (int v) {}

void
hline (int ch, int len) {}

void
vline (int character, int len) {}

#ifndef HAVE_GNOME
void
init_pair (int index, char *foreground, char *background) {}
#endif

int has_colors ()
{
    return 1;
}

void
attrset (int color) { }

void
do_define_key (int code, char *strcap) {}

void
load_terminfo_keys () {}

int
getch ()
{
    return getchar ();
}

void
mc_refresh (void)
{
}

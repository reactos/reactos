/* Term name pollution avoiding.
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
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#include <config.h>
#include <stdio.h>
#include "global.h"
#include "main.h"

/* This flag is set by xterm detection routine in function main() */
/* It is used by function view_other_cmd() */
int xterm_flag = 0;

/* The following routines only work on xterm terminals */

void do_enter_ca_mode (void)
{
    if (!xterm_flag)
	return;
    fprintf (stdout, /* ESC_STR ")0" */ ESC_STR "7" ESC_STR "[?47h");
    fflush (stdout);
}

void do_exit_ca_mode (void)
{
    if (!xterm_flag)
	return;
    fprintf (stdout, ESC_STR "[?47l" ESC_STR "8" ESC_STR "[m");
    fflush (stdout);
}

/* Dialog managing.
   Copyright (C) 1994 Miguel de Icaza.
   
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
#include "tty.h"
#include <stdio.h>
#include <stdlib.h>	/* For free() */
#include <stdarg.h>
#include <sys/types.h>
#include <string.h>
#include "x.h"
#include "mad.h"
#include "global.h"
#include "util.h"
#include "dialog.h"
#include "color.h"
#include "win.h"
#include "mouse.h"
#include "main.h"
#include "key.h"		/* For mi_getch() */
#include "dlg.h"		/* draw_box, yes I know, it's silly */
#include "background.h"		/* we_are_background definition */

/* "$Id: dialog.c,v 1.1 2001/12/30 09:55:26 sedwards Exp $" */

Refresh *refresh_list = 0;

void push_refresh (void (*new_refresh)(void *), void *parameter, int flags)
{
    Refresh *new;

    new = xmalloc (sizeof (Refresh), "push_refresh");
    new->next = (struct Refresh *) refresh_list;
    new->refresh_fn = new_refresh;
    new->parameter = parameter;
    new->flags     = flags;
    refresh_list = new;
}

void pop_refresh (void)
{
    Refresh *old;
    
    if (!refresh_list)
	fprintf (stderr, _("\n\n\nrefresh stack underflow!\n\n\n"));
    else {
	old = refresh_list;
	refresh_list = refresh_list->next;
	free (old);
    }
}

static void do_complete_refresh (Refresh *refresh_list)
{
    if (!refresh_list)
	return;

    if (refresh_list->flags != REFRESH_COVERS_ALL)
	do_complete_refresh (refresh_list->next);
    
    (*(refresh_list->refresh_fn))(refresh_list->parameter);
}

void do_refresh (void)
{
    if (we_are_background)
	return;
    if (!refresh_list)
	return;
    else {
	if (fast_refresh)
	    (*(refresh_list->refresh_fn))(refresh_list->parameter);
	else {
	    do_complete_refresh (refresh_list);
	}
    }
}

/* Poor man's window puts, it doesn't handle auto-wrap */
void my_wputs (int y, int x, char *text)
{
    char p;

    move (y, x);
    while ((p = *text++) != 0){
	if (p == '\n')
	    move (++y, x);
	else 
	    addch ((unsigned char)p);
    }
}


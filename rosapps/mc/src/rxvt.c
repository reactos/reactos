/* rxvt.c - gives output lines on rxvt with a special rxvt patch
   Copyright (C) 1997 Paul Sheer

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
#include <stdio.h>	/* read, printf */
#include <stdlib.h>	/* getenv */
#include <sys/types.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#   include <unistd.h>
#endif
#include <malloc.h>	/* malloc */

#ifndef SCO_FLAVOR
#	include <sys/time.h>		/* struct timeval */
#endif /* SCO_FLAVOR */

#if HAVE_SYS_SELECT_H
#   include <sys/select.h>
#endif

#include "tty.h"	/* move, addch */
#include "util.h"	/* is_printable */
#include "cons.saver.h"

int rxvt_extensions = 0;

int look_for_rxvt_extensions (void)
{
    static int been_called = 0;
    char *e;
    if (!been_called) {
	rxvt_extensions = 0;
	e = getenv ("RXVT_EXT");
	if (e)
	    if (!strcmp (e, "1.0"))
		rxvt_extensions = 1;
	been_called = 1;
    }
    if (rxvt_extensions)
	console_flag = 4;
    return rxvt_extensions;
}

/* my own wierd protocol base 16 - paul */
static int rxvt_getc (void)
{
    int r;
    unsigned char c;
    while (read (0, &c, 1) != 1);
    if (c == '\n')
	return -1;
    r = (c - 'A') * 16;
    while (read (0, &c, 1) != 1);
    r += (c - 'A');
    return r;
}

extern int keybar_visible;

static int anything_ready ()
{
    fd_set fds;
    struct timeval tv;

    FD_ZERO (&fds);
    FD_SET (0, &fds);
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    return select (1, &fds, 0, 0, &tv);
}

void show_rxvt_contents (int starty, unsigned char y1, unsigned char y2)
{
    unsigned char *k;
    int bytes, i, j, cols = 0;
    y1 += (keybar_visible != 0);	/* i don't knwo why we need this - paul */
    y2 += (keybar_visible != 0);
    while (anything_ready ())
	getch ();

/* my own wierd protocol base 26 - paul */
    printf ("\033CL%c%c%c%c\n",
	    (y1 / 26) + 'A', (y1 % 26) + 'A',
	    (y2 / 26) + 'A', (y2 % 26) + 'A');

    bytes = (y2 - y1) * (COLS + 1) + 1;		/* *should* be the number of bytes read */
    j = 0;
    k = malloc (bytes);
    for (;;) {
	int c;
	c = rxvt_getc ();
	if (c < 0)
	    break;
	if (j < bytes)
	    k[j++] = c;
	for (cols = 1;;cols++) {
	    c = rxvt_getc ();
	    if (c < 0)
		break;
	    if (j < bytes)
		k[j++] = c;
	}
    }
    for (i = 0; i < j; i++) {
	if ((i % cols) == 0)
	    move (starty + (i / cols), 0);
	addch (is_printable (k[i]) ? k[i] : ' ');
    }
    free (k);
}


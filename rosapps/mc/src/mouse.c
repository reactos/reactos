/* Mouse managing
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

/* Events received by clients of this library have their coordinates 0 */
/* based */

/* "$Id: mouse.c,v 1.1 2001/12/30 09:55:23 sedwards Exp $" */

#include <config.h>
#include <sys/types.h>
#ifdef HAVE_UNISTD_H
#   include <unistd.h>
#endif
#include <signal.h>	/* For kill() and SIGQUIT */
#include <fcntl.h>
#if (!defined(__IBMC__) && !defined(__IBMCPP__)) && !defined(OS2_NT)
#    include <termios.h>
#endif
#include <malloc.h>
#include <stdio.h>

#include "mad.h"
#include "mouse.h"
#include "global.h"		/* ESC_STR */
#include "util.h"		/* xmalloc */
#include "key.h"		/* define sequence */
#include "tty.h"		/* get ncurses header */

int xmouse_flag = 0;

/*
 * This chunk of NCURSES internals is used to support the keyok() function for
 * NCURSES versions between 1.9.6 (when the mouse code was introduced in 1995),
 * and 4.2 (when the keyok function will be release).
 */
#ifndef HAVE_KEYOK
#ifdef NCURSES_MOUSE_VERSION	/* first defined for ncurses 1.9.6 */
#ifdef NCURSES_970530		/* defined by configure script */
/*
 * ncurses 1.9.8a ported to QNX doesn't provide the SP pointer as a global
 * symbol in the library...
 */
#ifndef __QNX__
struct tries {
	struct tries    *child;
	struct tries    *sibling;
	unsigned char    ch;
	unsigned short   value;
};

struct screen {
	int             _ifd;
	FILE            *_ofp;
#if NCURSES_970530 >= 2
	char            *_setbuf;	/*4.0*/
#endif
	int             _checkfd;
	struct term     *_term;
	short           _lines;
	short           _columns;
#if NCURSES_970530 >= 1
	short           _lines_avail;	/*1.9.9g*/
	short           _topstolen;	/*1.9.9g*/
#endif
	struct _win_st  *_curscr;
	struct _win_st  *_newscr;
	struct _win_st  *_stdscr;
	struct tries    *_keytry;       /* "Try" for use with keypad mode   */
	/* there's more, but this is just for alignment */
};
extern struct screen *SP;

/*
 * Remove a code from the specified tree, freeing the unused nodes.  Returns
 * true if the code was found/removed.
 */
static
int _nc_remove_key(struct tries **tree, unsigned short code)
{
	struct tries *ptr = (*tree);

	if (code > 0) {
		while (ptr != 0) {
			if (_nc_remove_key(&(ptr->child), code)) {
				return TRUE;
			}
			if (ptr->value == code) {
				*tree = 0;
				free(ptr);
				return TRUE;
			}
			ptr = ptr->sibling;
		}
	}
	return FALSE;
}

int
keyok(int code, bool flag)
{
	_nc_remove_key(&(SP->_keytry), code);
	return OK;
}
#endif /* __QNX__ */
#endif /* NCURSES_970530 */
#endif /* NCURSES_MOUSE_VERSION */
#endif /* HAVE_KEYOK */

#ifdef HAVE_LIBGPM
static int mouse_d;		/* Handle to the mouse server */
#endif

#ifdef DEBUGMOUSE
/* Only used for debugging */
static int top_event = 0;
FILE *log;
#endif

#ifdef HAVE_LIBGPM

void show_mouse_pointer (int x, int y)
{
#ifdef HAVE_LIBGPM
    if (use_mouse_p == GPM_MOUSE){
	Gpm_DrawPointer (x, y, gpm_consolefd);
    }
#endif
}

#endif /* HAVE_LIBGPM */
#if 0
int mouse_handler (Gpm_Event *gpm_event)
{
    MouseEvent *event = mouse_events;
    int x = last_x = gpm_event->x;
    int y = last_y = gpm_event->y;
    int redo = 0;
    
/*    DEBUGM ((log, "Mouse [%d, %d]\n", x, y)); */

    /* Call any registered event handlers */
    for (; event; event = (MouseEvent *) event->next){
	if ((event->x1 <= x) && (x <= event->x2)
	    && (event->y1 <= y) && (y <= event->y2)){
	    gpm_event->x -= event->x1;
	    gpm_event->y -= event->y1;
	    last_mouse_event = event;
	    redo = (*(event->mouse_callback))(gpm_event, event->data);
	    gpm_event->x += event->x1;
	    gpm_event->y += event->y1;
	    break;
	}
    }
    return redo;
}

int redo_mouse (Gpm_Event *event)
{
    if (last_mouse_event){
    	int result;
    	event->x -= last_mouse_event->x1;
    	event->y -= last_mouse_event->y1;
	result = (*(last_mouse_event->mouse_callback))
	         (event,last_mouse_event->data);
    	event->x += last_mouse_event->x1;
    	event->y += last_mouse_event->y1;
    	return result;
    }
    return MOU_NORMAL;
}
#endif

void init_mouse (void)
{
    /*
     * MC's use of xterm mouse is incompatible with NCURSES's support.  The
     * simplest solution is to disable NCURSE's mouse.
     */
#ifdef NCURSES_MOUSE_VERSION
/* See the comment above about QNX/ncurses 1.9.8a ... */
#ifndef __QNX__
    keyok(KEY_MOUSE, FALSE);
#endif /* __QNX__ */
#endif /* NCURSES_MOUSE_VERSION */

#if defined(NCURSES_970530)
#endif
    switch (use_mouse_p)
    {
#ifdef HAVE_LIBGPM
      case GPM_MOUSE:
	{
	    Gpm_Connect conn;

	    conn.eventMask   = ~GPM_MOVE;
	    conn.defaultMask = GPM_MOVE;
	    conn.minMod      = 0;
	    conn.maxMod      = 0;

	    if ((mouse_d = Gpm_Open (&conn, 0)) == -1)
	        return;

#ifdef DEBUGMOUSE
	    log = fopen ("mouse.log", "w");
#endif
	}
	break;
#endif /* HAVE_LIBGPM */
	case XTERM_MOUSE:
	    if (!xmouse_flag) {

		/* save old highlight mouse tracking */
		printf("%c[?1001s",27);

		/* enable mouse tracking */
		printf("%c[?1000h",27);

		fflush (stdout);
		/* turn on */
		xmouse_flag = 1; 
		define_sequence (MCKEY_MOUSE, ESC_STR "[M", MCKEY_NOACTION);
	    }
	    break;
	default:
	    /* nothing */
	break;
    } /* switch (use_mouse_p) */ 
}

void shut_mouse (void)
{
    switch (use_mouse_p){
#ifdef HAVE_LIBGPM
      case GPM_MOUSE:
	Gpm_Close ();
	break;
#endif
      case XTERM_MOUSE:
	if (xmouse_flag) {

	    /* disable mouse tracking */
	    /* Changed the 1 for an 'l' below: */
	    printf("%c[?1000l",27);

	    /* restore old highlight mouse tracking */
	    printf("%c[?1001r",27);

	    fflush (stdout);
	    /* off */
	    xmouse_flag = 0;
	}
	break;
      default:
	/* nothing */
	break;
    }
}

#ifdef DEBUGMOUSE
void mouse_log (char *function, char *file, int line)
{
    fprintf (log, "%s called from %s:%d\n", function, file, line);
}
#endif


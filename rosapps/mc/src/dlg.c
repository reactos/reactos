/* Dlg box features module for the Midnight Commander
   Copyright (C) 1994, 1995 Radek Doulik, Miguel de Icaza

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
/* "$Id: dlg.c,v 1.1 2001/12/30 09:55:26 sedwards Exp $" */
#include <string.h>
#include <stdio.h>
#include <malloc.h>
#include <ctype.h>
#include "tty.h"
#include <stdarg.h>
#include "mad.h"
#include "x.h"
#include "util.h"
#include "menu.h"
#include "global.h"
#include "win.h"
#include "color.h"
#include "mouse.h"
#include "help.h"
#include "key.h"	/* For mi_getch() */
#include "dlg.h"
#include "dialog.h"	/* For push_refresh() and pop_refresh() */
#include "layout.h"
#include "main.h"	

/* This is the current frame, used to group Tk packings */
char *the_frame = "";

#define waddc(w,y1,x1,c) move (w->y+y1, w->x+x1); addch (c)

/* Primitive way to check if the the current dialog is our dialog */
/* This is needed by async routines like load_prompt */
Dlg_head *current_dlg = 0;

/* A hook list for idle events */
Hook *idle_hook = 0;

#ifndef PORT_HAS_SET_IDLE
#    define x_set_idle(d,x)
#endif

#ifndef PORT_HAS_DIALOG_STOP
#   define x_dialog_stop(d)
#endif

static void slow_box (Dlg_head *h, int y, int x, int ys, int xs)
{
    move (h->y+y, h->x+x);
    hline (' ', xs);
    vline (' ', ys);
    move (h->y+y, h->x+x+xs-1);
    vline (' ', ys);
    move (h->y+y+ys-1, h->x+x);
    hline (' ', xs);
}

/* draw box in window */
void draw_box (Dlg_head *h, int y, int x, int ys, int xs)
{
    extern int slow_terminal;
	
    if (slow_terminal){
	slow_box (h, y, x, ys, xs);
	return;
    }
    
#ifndef HAVE_SLANG
    waddc (h, y, x, ACS_ULCORNER);
    hline (ACS_HLINE, xs - 2);
    waddc (h, y + ys - 1, x, ACS_LLCORNER);
    hline (ACS_HLINE, xs - 2);

    waddc (h, y, x + xs - 1, ACS_URCORNER);
    waddc (h, y + ys - 1, x + xs - 1, ACS_LRCORNER);

    move (h->y+y+1, h->x+x);
    vline (ACS_VLINE, ys - 2);
    move (h->y+y+1, h->x+x+xs-1);
    vline (ACS_VLINE, ys - 2);
#else
    SLsmg_draw_box (h->y+y, h->x+x, ys, xs);
#endif
}

/* draw box in window */
void draw_double_box (Dlg_head *h, int y, int x, int ys, int xs)
{
#ifndef HAVE_SLANG
    draw_box (h, y, x, ys, xs);
#else
    SLsmg_draw_double_box (h->y+y, h->x+x, ys, xs);
#endif
}

void widget_erase (Widget *w)
{
    int x, y;

    for (y = 0; y < w->lines; y++){
	widget_move (w, y, 0);
	for (x = 0; x < w->cols; x++)
	    addch (' ');
    }
}

void dlg_erase (Dlg_head *h)
{
    int x, y;

    for (y = 0; y < h->lines; y++){
	move (y+h->y, h->x);	/* FIXME: should test if ERR */
	for (x = 0; x < h->cols; x++){
	    addch (' ');
	}
    }
}

void init_widget (Widget *w, int y, int x, int lines, int cols,
		  int (*callback)(Dlg_head *, void *, int, int),
		  destroy_fn destroy, mouse_h mouse_handler, char *tkname)
{
    w->x = x;
    w->y = y;
    w->cols = cols;
    w->lines = lines;
    w->color = -1;
    w->callback = callback;
    w->destroy  = destroy;
    w->mouse = mouse_handler;
    w->wdata = 0;
    w->wcontainer = 0;
    w->frame = "";
    w->parent = 0;
    w->tkname = tkname;

    if (tkname && *tkname == 0){
	fprintf (stderr, "Got a null string for the tkname\n");
	abort ();
    }
    /* Almost all widgets want to put the cursor in a suitable place */
    w->options = W_WANT_CURSOR; 
}

int default_proc (Dlg_head *h, int Msg, int Par)
{
    switch (Msg){

    case WIDGET_HOTKEY:         /* Didn't use the key */
        return 0;

    case WIDGET_INIT:		/* We could tell if something went wrong */
	return 1;
	
    case WIDGET_KEY:
	return 0;		/* Didn't use the key */
	
    case WIDGET_FOCUS:		/* We accept FOCUSes */
	if (h->current)
	    x_focus_widget (h->current);
	return 1;
	
    case WIDGET_UNFOCUS:	/* We accept loose FOCUSes */
	if (h->current)
	    x_unfocus_widget (h->current);
	return 1;
	
    case WIDGET_DRAW:
	return 1;

    case WIDGET_DESTROY:
	return 1;

    case WIDGET_CURSOR:
	/* Move the cursor to the default widget position */
	return 1;

    case WIDGET_IDLE:
	return 1;
    }
    printf ("Internal error: unhandled message: %d\n", Msg);
    return 1;
}

int default_dlg_callback (Dlg_head *h, int id, int msg)
{
    if (msg == DLG_IDLE){
	dlg_broadcast_msg_to (h, WIDGET_IDLE, 0, W_WANT_IDLE);
    }
    return 0;
}

#ifdef HAVE_X
int midnight_callback (struct Dlg_head *h, int id, int msg);
#endif
Dlg_head *create_dlg (int y1, int x1, int lines, int cols,
		      int *color_set,
		      int (*callback) (struct Dlg_head *, int, int),
		      char *help_ctx, char *name,
		      int flags)
{
    Dlg_head *new_d;

    if (flags & DLG_CENTER){
	y1 = (LINES-lines)/2;
	x1 = (COLS-cols)/2;
    }
    if ((flags & DLG_TRYUP) && (y1 > 3))
	y1 -= 2;

    new_d = (Dlg_head *) malloc (sizeof (Dlg_head));
    new_d->current = NULL;
    new_d->count = 0;
    new_d->direction = DIR_FORWARD;
    new_d->color = color_set;
    new_d->help_ctx = help_ctx;
    new_d->callback = callback ? callback : default_dlg_callback;
    new_d->send_idle_msg = 0;
    new_d->x = x1;
    new_d->y = y1;
    new_d->title = 0;
    new_d->cols = cols;
    new_d->lines = lines;
    new_d->refresh_pushed = 0;
    new_d->has_menubar = 0;
    new_d->name = name;
    new_d->raw = 0;
    new_d->grided = 0;
    new_d->initfocus = NULL;
    new_d->running = 0;
#ifdef HAVE_X
    if (callback != midnight_callback)
        new_d->wdata = xtoolkit_create_dialog (new_d, flags);
    else
    	new_d->wdata = xtoolkit_get_main_dialog (new_d);
#endif    
    return (new_d);
}
		      
void set_idle_proc (Dlg_head *d, int state)
{
    d->send_idle_msg = state;
    x_set_idle (d, state);
}

/* add component to dialog buffer */
int add_widgetl (Dlg_head *where, void *what, WLay layout)
{
    Widget_Item *back;
    Widget      *widget = (Widget *) what;

    /* Only used by Tk */
    widget->frame = the_frame;
    
    widget->layout = layout;
    /* Don't accept 0 widgets, this could be from widgets that could not */
    /* initialize properly */
    if (!what)
	return 0;

    widget->x += where->x;
    widget->y += where->y;

    if (where->running){
	    Widget_Item *point = where->current;

	    where->current = (Widget_Item *) malloc (sizeof (Widget_Item));

	    if (point){
		    where->current->next = point->next;
		    where->current->prev = point;
		    point->next->prev = where->current;
		    point->next = where->current;
	    } else {
		    where->current->next = where->current;
		    where->first = where->current;
		    where->current->prev = where->first;
		    where->last = where->current;
		    where->first->next = where->last;
	    }
    } else {
	    back = where->current;
	    where->current = (Widget_Item *) malloc (sizeof (Widget_Item));
	    if (back){
		    back->prev = where->current;
		    where->current->next = back;
	    } else {
		    where->current->next = where->current;
		    where->first = where->current;
	    }
	    
	    where->current->prev = where->first;
	    where->last = where->current;
	    where->first->next = where->last;
	    
    }
    where->current->dlg_id = where->count;
    where->current->widget = what;
    where->current->widget->parent = where;
    
    where->count++;

    /* If the widget is inserted in a running dialog */
    if (where->running){
	send_message (where, widget, WIDGET_INIT, 0);
	send_message (where, widget, WIDGET_DRAW, 0);
#ifdef HAVE_GNOME
	x_add_widget (where, where->current);
#endif
    }
    return (where->count - 1);
}

int remove_widget (Dlg_head *h, void *what)
{
    Widget_Item *first, *p;
    
    first = p = h->current;
    
    do {
	if (p->widget == what){
	    /* Remove links to this Widget_Item */
	    p->prev->next = p->next;
	    p->next->prev = p->prev;
	    
	    /* Make sure h->current is always valid */
	    if (p == h->current){
		h->current = h->current->next;
		if (h->current == p)
		    h->current = 0;
	    }
	    h->count--;
	    free (p);
	    return 1;
	}
	p = p->next;
    } while (p != first);
    return 0;
}

int destroy_widget (Widget *w)
{
    send_message (w->parent, w, WIDGET_DESTROY, 0);
    if (w->destroy)
	w->destroy (w);
    free (w);
    return 1;
}

int add_widget (Dlg_head *where, void *what)
{
    return add_widgetl (where, what, XV_WLAY_DONTCARE);
}

int send_message (Dlg_head *h, Widget *w, int msg, int par)
{
    return (*(w->callback))(h, w, msg, par);
}

/* broadcast a message to all the widgets in a dialog that have
 * the options set to flags.
 */
void dlg_broadcast_msg_to (Dlg_head *h, int message, int reverse, int flags)
{
    Widget_Item *p, *first;

    if (!h->current)
	    return;
		    
    if (reverse)
	first = p = h->current->prev;
    else
	/* FIXME: On XView the layout for the widget->next widget is
	   invoked, and we should change the buttons order on query_dialog
	   in order to use the HAVE_X part of the statement */
#ifdef HAVE_X
	first = p = h->current;
#else
	first = p = h->current->next;
#endif
    do {
/*	if (p->widget->options & flags) */
	    send_message (h, p->widget, message, 0);

	if (reverse)
	    p = p->prev;
	else
	    p = p->next;
    } while (first != p);
}

/* broadcast a message to all the widgets in a dialog */
void dlg_broadcast_msg (Dlg_head *h, int message, int reverse)
{
    dlg_broadcast_msg_to (h, message, reverse, ~0);
}

int dlg_focus (Dlg_head *h)
{
    if (send_message (h, h->current->widget, WIDGET_FOCUS, 0)){
	(*h->callback) (h, h->current->dlg_id, DLG_FOCUS);
	return 1;
    }
    return 0;
}

int dlg_unfocus (Dlg_head *h)
{
    if (send_message (h, h->current->widget, WIDGET_UNFOCUS, 0)){
	(*h->callback) (h, h->current->dlg_id, DLG_UNFOCUS);
	return 1;
    }
    return 0;
}

static void select_a_widget (Dlg_head *h, int down)
{
    int direction = h->direction;

    if (!down)
	direction = !direction;
    
    do {
	if (direction)
	    h->current = h->current->next;
	else
	    h->current = h->current->prev;
	
	(*h->callback) (h, h->current->dlg_id, DLG_ONE_DOWN);
    } while (!dlg_focus (h));
}

/* Return true if the windows overlap */
int dlg_overlap (Widget *a, Widget *b)
{
    if ((b->x >= a->x + a->cols)
	|| (a->x >= b->x + b->cols)
	|| (b->y >= a->y + a->lines)
	|| (a->y >= b->y + b->lines))
	return 0;
    return 1;
}


/* Searches a widget, uses the callback as a signature in the dialog h */
Widget *find_widget_type (Dlg_head *h, callback_fn signature)
{
    Widget *w;
    Widget_Item *item;
    int i;

    if (!h)
	return 0;
    
    w = 0;
    for (i = 0, item = h->current; i < h->count; i++, item = item->next){
	if (item->widget->callback == signature){
	    w = item->widget;
	    break;
	}
    }
    return w;
}

void dlg_one_up (Dlg_head *h)
{
    Widget_Item *old;

    old = h->current;
    /* If it accepts unFOCUSion */
    if (!dlg_unfocus(h))
	return;
    
    select_a_widget (h, 0);
    if (dlg_overlap (old->widget, h->current->widget)){
	send_message (h, h->current->widget, WIDGET_DRAW, 0);
	send_message (h, h->current->widget, WIDGET_FOCUS, 0);
    }
}

void dlg_one_down (Dlg_head *h)
{
    Widget_Item *old;

    old = h->current;
    if (!dlg_unfocus (h))
	return;

    select_a_widget (h, 1); 
    if (dlg_overlap (old->widget, h->current->widget)){
	send_message (h, h->current->widget, WIDGET_DRAW, 0);
	send_message (h, h->current->widget, WIDGET_FOCUS, 0);
    }
}

int dlg_select_widget (Dlg_head *h, void *w)
{
    if (dlg_unfocus (h)){
	while (h->current->widget != w)
	    h->current = h->current->next;
	while (!dlg_focus (h))
	    h->current = h->current->next;

	return 1;
    }
    return 0;
}

int send_message_to (Dlg_head *h, Widget *w, int msg, int par)
{
    Widget_Item *p = h->current;
    int v, i;

    v = 0;
    for (i = 0; i < h->count; i++){
	if (w == (void *) p->widget){
	    v = send_message (h, p->widget, msg, par);
	    break;
	}
	p = p->next;
    }
    return v;
}

#define callback(h) (h->current->widget->callback)

void update_cursor (Dlg_head *h)
{
    if (!h->current)
         return;
    if (h->current->widget->options & W_WANT_CURSOR)
	send_message (h, h->current->widget, WIDGET_CURSOR, 0);
    else {
	Widget_Item *p = h->current;
    
	do {
	    if (p->widget->options & W_WANT_CURSOR)
		if ((*p->widget->callback)(h, p->widget, WIDGET_CURSOR, 0)){
		    x_focus_widget (p);
		    break;
		}
	    p = p->next;
	} while (h->current != p);
    }
}

/* Redraw the widgets in reverse order, leaving the current widget
 * as the last one
 */
void dlg_redraw (Dlg_head *h)
{
    (h->callback)(h, 0, DLG_DRAW);

    dlg_broadcast_msg (h, WIDGET_DRAW, 1);
    
    update_cursor (h);
}

void dlg_refresh (void *parameter)
{
    dlg_redraw ((Dlg_head *) parameter);
}

void dlg_stop (Dlg_head *h)
{
    h->running = 0;
    x_dialog_stop (h);	
}

static INLINE void dialog_handle_key (Dlg_head *h, int d_key)
{
   char *hlpfile;

    switch (d_key){
    case KEY_LEFT:
    case KEY_UP:
	dlg_one_up (h);
	break;
	
    case KEY_RIGHT:
    case KEY_DOWN:
	dlg_one_down (h);
	break;

    case KEY_F(1):
        hlpfile = concat_dir_and_file (mc_home, "mc.hlp");
	interactive_display (hlpfile, h->help_ctx);
        free (hlpfile);
	do_refresh ();
	break;

    case XCTRL('z'):
	suspend_cmd ();
	/* Fall through */
	    
    case XCTRL('l'):
#ifndef HAVE_SLANG
	/* Use this if the refreshes fail */
	clr_scr ();
	do_refresh ();
#else
	touchwin (stdscr);
#endif
	mc_refresh ();
	doupdate ();
	break;
	
    case '\n':
    case KEY_ENTER:
	h->ret_value = B_ENTER;
	h->running = 0;
	x_dialog_stop (h);
	break;

    case ESC_CHAR:
    case KEY_F (10):
    case XCTRL ('c'):
    case XCTRL ('g'):
	h->ret_value = B_CANCEL;
	dlg_stop (h);
	break;
    }
}

static int dlg_try_hotkey (Dlg_head *h, int d_key)
{
    Widget_Item *hot_cur;
    Widget_Item *previous;
    int    handled, c;
    extern input_event ();
    
    /*
     * Explanation: we don't send letter hotkeys to other widgets if
     * the currently selected widget is an input line
     */

    if (h->current->widget->options & W_IS_INPUT){
		if(d_key < 255 && isalpha(d_key))
			return 0;
    }
    
    /* If it's an alt key, send the message */
    c = d_key & ~ALT(0);
    if (d_key & ALT(0) && c < 255 && isalpha(c))
	d_key = tolower(c);

#ifdef _OS_NT
	/* .ado: fix problem with file_permission under Win95 */
    if (d_key == 0) return 0;
#endif
	
    handled = 0;
    if (h->current->widget->options & W_WANT_HOTKEY)
	handled = callback (h) (h, h->current->widget, WIDGET_HOTKEY, d_key);
    
    /* If not used, send hotkey to other widgets */
    if (handled)
	return handled;
    
    hot_cur = h->current;
    
    /* send it to all widgets */
    do {
	if (hot_cur->widget->options & W_WANT_HOTKEY)
	    handled |= (*hot_cur->widget->callback)
		(h, hot_cur->widget, WIDGET_HOTKEY, d_key);
	
	if (!handled)
	    hot_cur = hot_cur->next;
    } while (h->current != hot_cur && !handled);
	
    if (!handled)
	return 0;

    (*h->callback) (h, 0, DLG_HOTKEY_HANDLED);
    previous = h->current;
    if (!dlg_unfocus (h))
	return handled;
    
    h->current = hot_cur;
    if (!dlg_focus (h)){
	h->current = previous;
	dlg_focus (h);
    }
    return handled;
}

void dlg_key_event (Dlg_head *h, int d_key)
{
    int handled;
    
    /* TAB used to cycle */
    if (!h->raw && (d_key == '\t' || d_key == KEY_BTAB))
	if (d_key == '\t')
	    dlg_one_down (h);
        else
	    dlg_one_up (h);
    else {
	
	/* first can dlg_callback handle the key */
	handled = (*h->callback) (h, d_key, DLG_KEY);

	/* next try the hotkey */
	if (!handled)
	    handled = dlg_try_hotkey (h, d_key);
	
	/* not used - then try widget_callback */
	if (!handled)
	    handled |= callback (h)(h, h->current->widget, WIDGET_KEY, d_key);
	
	/* not used- try to use the unhandled case */
	if (!handled)
	    handled |= (*h->callback) (h, d_key, DLG_UNHANDLED_KEY);
	
	if (!handled)
	    dialog_handle_key (h, d_key);
	(*h->callback) (h, d_key, DLG_POST_KEY);
    }
}

static INLINE int dlg_mouse_event (Dlg_head *h, Gpm_Event *event)
{
    Widget_Item *item;
    Widget_Item *starting_widget = h->current;
    Gpm_Event   new_event;
    int x = event->x;
    int y = event->y;
    int ret_value;

    /* kludge for the menubar: start at h->first, not current  */
    /* Must be carefull in the insertion order to the dlg list */
    if (y == 1 && h->has_menubar)
	starting_widget = h->first;

    item = starting_widget;
    do {
	Widget *widget = item->widget;

	item = item->next;
	
	if (!((x > widget->x) && (x <= widget->x+widget->cols) 
	    && (y > widget->y) && (y <= widget->y+widget->lines)))
	    continue;

	new_event = *event;
	new_event.x -= widget->x;
	new_event.y -= widget->y;

	ret_value = widget->mouse ? (*widget->mouse) (&new_event, widget) :
	    MOU_NORMAL;

	return ret_value;
    } while (item != starting_widget);
    return 0;
}

/* Run dialog routines */

/* Init the process */
void init_dlg (Dlg_head *h)
{
    int refresh_mode;

    tk_end_frame ();
    
    /* Initialize dialog manager and widgets */
    (*h->callback) (h, 0, DLG_INIT);
    dlg_broadcast_msg (h, WIDGET_INIT, 0);

    if (h->x == 0 && h->y == 0 && h->cols == COLS && h->lines == LINES)
	refresh_mode = REFRESH_COVERS_ALL;
    else
	refresh_mode = REFRESH_COVERS_PART;
    push_refresh (dlg_refresh, h, refresh_mode);
    h->refresh_pushed = 1;
    
    /* Initialize direction */
    if (!h->direction)
	h->current =  h->first;
	
    if (h->initfocus != NULL)
        h->current = h->initfocus;

    h->previous_dialog = current_dlg;
    current_dlg = h;
    
    /* Initialize the mouse status */
    h->mouse_status = 0;

    /* Redraw the screen */
    dlg_redraw (h);
    
    while (!dlg_focus (h))
	h->current = h->current->next;

    h->ret_value = 0;
    h->running = 1;
    x_init_dlg (h);
}

/* Shutdown the run_dlg */
void dlg_run_done (Dlg_head *h)
{
    (*h->callback) (h, h->current->dlg_id, DLG_END);
    current_dlg = (Dlg_head *) h->previous_dialog;
    if (current_dlg)
	    x_focus_widget (current_dlg->current);
}

void dlg_process_event (Dlg_head *h, int key, Gpm_Event *event)
{
    if (key == EV_NONE){
	if (got_interrupt ())
	    key = XCTRL('g');
	else
	    return;
    }
    
    if (key == EV_MOUSE)
	h->mouse_status = dlg_mouse_event (h, event);
    else
	dlg_key_event (h, key);
}

#ifndef PORT_HAS_FRONTEND_RUN_DLG
static inline void
frontend_run_dlg (Dlg_head *h)
{
    int d_key;
    Gpm_Event event;

    event.x = -1;
    while (h->running) {
#if defined(HAVE_SLANG) || NCURSES_VERSION_MAJOR >= 4
	/* It does not work with ncurses before 1.9.9g, it will break */
	if (winch_flag)
	    change_screen_size ();
#endif
	if (is_idle ()){
	    if (idle_hook)
		execute_hooks (idle_hook);
	    
	    while (h->send_idle_msg && is_idle ()){
		(*h->callback) (h, 0, DLG_IDLE);
	    }
	}

	update_cursor (h);
	(*h->callback)(h, 0, DLG_PRE_EVENT);

	/* Clear interrupt flag */
	got_interrupt ();
	d_key = get_event (&event, h->mouse_status == MOU_REPEAT, 1);

	dlg_process_event (h, d_key, &event);
    }
}
#endif /* PORT_HAS_FRONTEND_RUN_DLG */

/* Standard run dialog routine
 * We have to keep this routine small so that we can duplicate it's
 * behavior on complex routines like the file routines, this way,
 * they can call the dlg_process_event without rewriting all the code
 */
void run_dlg (Dlg_head *h)
{
    init_dlg (h);
    frontend_run_dlg (h);
    dlg_run_done (h);
}

void
destroy_dlg (Dlg_head *h)
{
    int i;
    Widget_Item *c;

    if (h->refresh_pushed)
	pop_refresh (); 

    x_destroy_dlg_start (h);
    dlg_broadcast_msg (h, WIDGET_DESTROY, 0);
    c = h->current;
    for (i = 0; i < h->count; i++){
	if (c->widget->destroy)
	    c->widget->destroy (c->widget);
	c = c->next;
	free (h->current->widget);
	free (h->current);
	h->current = c;
    }
    if (h->title)
	free (h->title);
    x_destroy_dlg (h);
    free (h);
    if (refresh_list)
	do_refresh ();
}

int std_callback (Dlg_head *h, int Msg, int Par)
{
    return 0;
}

void widget_set_size (Widget *widget, int y, int x, int lines, int cols)
{
    widget->x = x;
    widget->y = y;
    widget->cols = cols;
    widget->lines = lines;
}

/* Replace widget old for widget new in the h dialog */
void dlg_replace_widget (Dlg_head *h, Widget *old, Widget *new)
{
    Widget_Item *p = h->current;
    int should_focus = 0;
    
    do {
	if (p->widget == old){

	    if (old == h->current->widget)
		should_focus = 1;
	    
	    /* We found the widget */
	    /* First kill the widget */
	    new->focused = old->focused;
	    new->parent  = h;
	    send_message_to (h, old, WIDGET_DESTROY, 0);
	    (*old->destroy) (old);

	    /* We insert the new widget */
	    p->widget = new;
	    send_message_to (h, new, WIDGET_INIT, 0);
	    if (should_focus){
		if (dlg_focus (h) == 0)
		    select_a_widget (h, 1);
	    }
	    send_message_to (h, new, WIDGET_DRAW, 0);
	    break;
	}
	p = p->next;
    } while (p != h->current);
}

void widget_redraw (Dlg_head *h, Widget_Item *w)
{
    Widget_Item *save = h->current;

    h->current = w;
    (*w->widget->callback)(h, h->current->widget, WIDGET_DRAW, 0);
    h->current = save;
}

/* Returns the index of h->current from h->first */
int dlg_item_number (Dlg_head *h)
{
    Widget_Item *p;
    int i = 0;

    p = h->first;

    do {
	if (p == h->current)
	    return i;
	i++;
	p = p->next;
    } while (p != h->first);
    fprintf (stderr, "Internal error: current not in dialog list\n\r");
    exit (1);
}

int dlg_select_nth_widget (Dlg_head *h, int n)
{
    Widget_Item *w;
    int i;

    w = h->first;
    for (i = 0; i < n; i++)
	w = w->next;

    return dlg_select_widget (h, w->widget);
}

#ifdef HAVE_TK
/* Frames must include a trailing dot */
static void tk_frame_proc (Dlg_head *h, char *frame, int new_frame)
{
    char *s = strdup (frame);
    
    if (frame [strlen (frame)-1] != '.'){
	fprintf (stderr, "Invalid frame name\n");
	exit (1);
    }
    s [strlen (frame)-1] = 0;
    the_frame = frame;
    
    if (new_frame)
	tk_evalf ("frame %s.%s", (char *)h->wdata, s);
}

/* If passed a null string, it returns */
void tk_new_frame (Dlg_head *h, char *frame)
{
    if (!*frame)
	return;
    tk_frame_proc (h, frame, 1);
}

void tk_frame (Dlg_head *h, char *frame)
{
    tk_frame_proc (h, frame, 0);
}

void tk_end_frame ()
{
    the_frame = "";
}
#else
void tk_new_frame (Dlg_head *h, char *x)
{
}

void tk_frame (Dlg_head *h, char *x)
{
}

void tk_end_frame (void)
{
}
#endif

#ifndef PORT_HAS_DIALOG_TITLE
void
x_set_dialog_title (Dlg_head *h, char *title)
{
  h->title = strdup(title);
}
#endif


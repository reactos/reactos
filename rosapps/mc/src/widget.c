/* Widgets for the Midnight Commander

   Copyright (C) 1994, 1995, 1996 the Free Software Foundation
   
   Authors: 1994, 1995 Radek Doulik
            1994, 1995 Miguel de Icaza
            1995 Jakub Jelinek
	    1996 Andrej Borsenkow
	    1997 Norbert Warmuth

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
/* "$Id: widget.c,v 1.1 2001/12/30 09:55:20 sedwards Exp $" */

#include <config.h>
#include <string.h>
#include <stdio.h>
#include <malloc.h>
#include "tty.h"
#include <ctype.h>
#include "mad.h"
#include "global.h"
#include "util.h"
#include "color.h"
#include "mouse.h"
#include "dlg.h"
#include "widget.h"
#include "win.h"
#include "complete.h"
#include "key.h"		/* XCTRL and ALT macros  */
#include "x.h"
#include "profile.h"	/* for history loading and saving */

#ifndef HAVE_X
#   define x_create_button(a,b,c)  1
#   define x_create_radio(a,b,c)   1
#   define x_create_check(a,b,c)   1
#   define x_create_label(a,b,c)   1
#   define x_create_input(a,b,c)   1
#   define x_create_listbox(a,b,c) 1
#   define x_create_buttonbar(a,b,c) 1
#   define x_create_gauge(a,b,c) 1
#   define x_listbox_select_nth(a,b)
#   define x_list_insert(a,b,c)
#   define x_redefine_label(a,b)
#endif

#ifndef PORT_HAS_DESTROY_CMD
#   define x_destroy_cmd(w)
#endif

#ifndef PORT_HAS_RADIO_FOCUS_ITEM
#   define x_radio_focus_item(r)
#endif

#ifndef PORT_HAS_RADIO_TOGGLE
#   define x_radio_toggle
#endif

static int button_event (Gpm_Event *event, WButton *b);

int quote = 0;

static int
button_callback (Dlg_head *h, WButton *b, int Msg, int Par)
{
    char *txt, buf[256];
    int stop = 0;
    int off = 0;
    
    switch (Msg){
    case WIDGET_INIT:
	return x_create_button (h, h->wdata, b);
#ifndef	HAVE_XVIEW
    case WIDGET_HOTKEY:
        if (b->hotkey == Par ||  toupper(b->hotkey) == Par){
	    button_callback (h, b, WIDGET_KEY, ' '); /* to make action */
	    return 1;
	} else
	    return 0;
	
    case WIDGET_KEY:
	if (Par != ' ' && Par != '\n')
	    break;

	if (b->callback)
	    stop = (*b->callback)(b->action, b->callback_data);
	if (!b->callback || stop){
	    h->ret_value = b->action;
	    dlg_stop (h);
	}
	return 1;

#ifdef HAVE_TK
    case WIDGET_FOCUS:
    case WIDGET_CURSOR:
    {
	char *s = b->action == B_ENTER ? ".button" : "";
	
	tk_evalf ("focus %s%s", (char *)(b->widget.wdata)+1, s);
	/* Do not call default_proc: we did the tk focus command */
	return 1;
    }
#else
	
    case WIDGET_CURSOR:
	switch (b->flags) {
	    case DEFPUSH_BUTTON:
		off = 3;
		break;
	    case NORMAL_BUTTON:
		off = 2;
		break;
	    case NARROW_BUTTON:
		off = 1;
		break;
	    case HIDDEN_BUTTON:
	    default:
		off = 0;
		break;
        }
	widget_move (&b->widget, 0, b->hotpos + off);
	return 1;

    case WIDGET_UNFOCUS:
    case WIDGET_FOCUS:
    case WIDGET_DRAW:
	if (Msg==WIDGET_UNFOCUS) 
	    b->selected = 0;
	else if (Msg==WIDGET_FOCUS) 
	    b->selected = 1;

	switch (b->flags){
	    case DEFPUSH_BUTTON:
		sprintf (buf, "[< %s >]", b->text);
		off = 3;
		break;
	    case NORMAL_BUTTON:
		sprintf (buf, "[ %s ]", b->text);
		off = 2;
		break;
	    case NARROW_BUTTON:
		sprintf (buf, "[%s]", b->text);
		off = 1;
		break;
	    case HIDDEN_BUTTON:
	    default:
		buf[0] = '\0';
		off = 0;
		break;
	}
	txt = buf;

	attrset ((b->selected) ? FOCUSC : NORMALC);
	widget_move (&b->widget, 0, 0);

	addstr (txt);

	if (b->hotpos >= 0){
	    attrset ((b->selected) ? HOT_FOCUSC : HOT_NORMALC);
	    widget_move (&b->widget, 0, b->hotpos+off);
	    addch ((unsigned char)b->text [b->hotpos]);
        }
	if (Msg == WIDGET_FOCUS)
	    break;
	else
	    return 1;
	break;
#endif	
#endif /* !HAVE_XVIEW */
    }	
    return default_proc (h, Msg, Par);
}

static int
button_event (Gpm_Event *event, WButton *b)
{
#ifndef HAVE_X
    if (event->type & (GPM_DOWN|GPM_UP)){
    	Dlg_head *h=b->widget.parent;
	dlg_select_widget (h, b);
	if (event->type & GPM_UP){
	    button_callback (h, b, WIDGET_KEY, ' ');
	    (*h->callback) (h, ' ', DLG_POST_KEY);
	    return MOU_NORMAL;
	}
    }
#endif
    return MOU_NORMAL;
}

static void
button_destroy (WButton *b)
{
    x_destroy_cmd (b);
    free (b->text);
}

static int
button_len (const char *text, unsigned int flags)
{
#ifndef	HAVE_X
    int ret = strlen (text);
    switch (flags){
	case DEFPUSH_BUTTON:
	    ret += 6;
	    break;
	case NORMAL_BUTTON:
	    ret += 4;
	    break;
	case NARROW_BUTTON:
	    ret += 2;
	    break;
	case HIDDEN_BUTTON:
	default:
	    return 0;
    }
    return ret;
#else
    return strlen (text);
#endif
}

/*
 * Assuming that button text is malloc'ed, we may safely change it
 * (as opposed to statically allocated); from other hand, excluding &
 * and shifting data past it to the left results to one unused byte.
 * This does not harm though :)
 */
void
button_scan_hotkey(WButton* b)
{
    char* cp = strchr (b->text, '&');

    if (cp != NULL && cp[1] != '\0'){
	strcpy (cp, cp+1);
	b->hotkey = tolower (*cp);
	b->hotpos = cp - b->text;
    }
}

WButton *
button_new (int y, int x, int action, int flags, char *text, 
	    int (*callback)(int, void *), void *callback_data, char *tkname)
{
    WButton *b = xmalloc (sizeof (WButton), "new_button");

    init_widget (&b->widget, y, x, 1, button_len (text, flags),
		 (callback_fn) button_callback,
		 (destroy_fn) button_destroy, (mouse_h)button_event, tkname);
    
    b->action = action;
    b->flags  = flags;
    b->selected = 0;
    b->text   = strdup (text);
    b->callback = callback;
    b->callback_data = callback_data;
    widget_want_hotkey (b->widget, 1);
    b->hotkey = 0;
    b->hotpos = -1;

    button_scan_hotkey(b);
    return b;
}

void
button_set_text (WButton *b, char *text)
{
    free (b->text);
    b->text = strdup (text);
    b->widget.cols = button_len (text, b->flags);
    button_scan_hotkey(b);
#ifdef HAVE_X
    x_button_set (b, b->text);
#else
    dlg_redraw (b->widget.parent);
#endif
}


/* Radio button widget */
static int radio_event (Gpm_Event *event, WRadio *r);

static int
radio_callback (Dlg_head *h, WRadio *r, int Msg, int Par)
{
    int i;
    
    switch (Msg) {
    case WIDGET_INIT:
	return x_create_radio (h, h->wdata, r);
	
#ifndef HAVE_XVIEW
    case WIDGET_HOTKEY:
	{
	    int i, lp = tolower(Par);
	    char *cp;
	    
	    for (i = 0; i < r->count; i++){
		cp = strchr (r->texts [i], '&');
		if (cp != NULL && cp[1] != '\0'){
		    int c = tolower (cp [1]);
		    
		    if (c != lp)
			continue;
		    r->pos = i;
		    radio_callback (h, r, WIDGET_KEY, ' '); /* Take action */
		    return 1;
		}
	    }
	}
	return 0;
	
    case WIDGET_KEY:
	switch (Par){
	case ' ':
	    r->sel = r->pos;
    	    (*h->callback) (h, h->current->dlg_id, DLG_ACTION);
	    radio_callback (h, r, WIDGET_FOCUS, ' ');
	    x_radio_toggle (r);
	    return 1;

	case KEY_UP:
	case KEY_LEFT:
	    if (r->pos > 0){
		r->pos--;
		x_radio_focus_item (r);
		return 1;
	    }
	    return 0;
	    
	case KEY_DOWN:
	case KEY_RIGHT:
	    if (r->count - 1 > r->pos) {
		r->pos++;
		x_radio_focus_item (r);
		return 1;
	    }
	}
	return 0;

#ifdef HAVE_X
    case WIDGET_FOCUS:
    case WIDGET_CURSOR:
	    x_radio_focus_item (r);
	    return 1;
#endif
#endif
	
#ifndef HAVE_X
    case WIDGET_CURSOR:
	(*h->callback) (h, h->current->dlg_id, DLG_ACTION);
	radio_callback (h, r, WIDGET_FOCUS, ' ');
	widget_move (&r->widget, r->pos, 1);
	break;
	    
    case WIDGET_UNFOCUS:
    case WIDGET_FOCUS:
    case WIDGET_DRAW:
	for (i = 0; i < r->count; i++){
		register unsigned char* cp;
	    attrset ((i==r->pos && Msg==WIDGET_FOCUS) ? FOCUSC :NORMALC);
	    widget_move (&r->widget, i, 0);

		printw("(%c) ", (r->sel == i) ? '*' : ' ');
		for (cp =  r->texts[i]; *cp; cp++)
		{
			if (*cp == '&')
			{
				attrset ((i==r->pos && Msg==WIDGET_FOCUS) 
					? HOT_FOCUSC : HOT_NORMALC);
				addch(*++cp);
				attrset ((i==r->pos && Msg==WIDGET_FOCUS) ? FOCUSC : NORMALC);
			}
			else
				addch(*cp);
		}
	}
	return 1;
	break;
#endif	
    }
    return default_proc (h, Msg, Par);
}

#ifdef HAVE_TK
    static void Radio_destroy (WRadio *r)
    {
	x_destroy_cmd (r);
    }
#   define radio_destroy (destroy_fn) Radio_destroy
#else
#   define radio_destroy 0
#endif

static int
radio_event (Gpm_Event *event, WRadio *r)
{
#ifndef HAVE_X
    if (event->type & (GPM_DOWN|GPM_UP)){
    	Dlg_head *h = r->widget.parent;
	
	r->pos = event->y - 1;
	dlg_select_widget (h, r);
	if (event->type & GPM_UP){
	    radio_callback (h, r, WIDGET_KEY, ' ');
	    radio_callback (h, r, WIDGET_FOCUS, 0);
	    (*h->callback) (h, ' ', DLG_POST_KEY);
	    return MOU_NORMAL;
	}
    }
#endif
    return MOU_NORMAL;
}

WRadio *
radio_new (int y, int x, int count, char **texts, int use_hotkey, char *tkname)
{
    WRadio *r = xmalloc (sizeof (WRadio), "radio_new");
    int i, max, m;

    /* Compute the longest string */
    max = 0;
    for (i = 0; i < count; i++){
	m = strlen (texts [i]);
	if (m > max)
	    max = m;
    }

    init_widget (&r->widget, y, x, count, max, (callback_fn) radio_callback,
		 radio_destroy, (mouse_h) radio_event, tkname);
    r->state = 1;
    r->pos = 0;
    r->sel = 0;
    r->count = count;
    r->texts = texts;
    r->upper_letter_is_hotkey = use_hotkey;
    widget_want_hotkey (r->widget, 1);
    
    return r;
}


/* Checkbutton widget */

static int check_event (Gpm_Event *event, WCheck *b);

static int
check_callback (Dlg_head *h, WCheck *c, int Msg, int Par)
{
    switch (Msg) {
    case WIDGET_INIT:
	return x_create_check (h, h->wdata, c);

#ifndef	HAVE_XVIEW
    case WIDGET_HOTKEY:
        if (c->hotkey==Par ||
	    (c->hotkey>='a' && c->hotkey<='z' && c->hotkey-32==Par)){
	    check_callback (h, c, WIDGET_KEY, ' ');        /* make action */
	    return 1;
	} else
	    return 0;

    case WIDGET_KEY:
	if (Par != ' ')
	    break;
	c->state ^= C_BOOL;
	c->state ^= C_CHANGE;
        (*h->callback) (h, h->current->dlg_id, DLG_ACTION);
	check_callback (h, c, WIDGET_FOCUS, ' ');
	return 1;

#ifndef HAVE_X	
    case WIDGET_CURSOR:
	widget_move (&c->widget, 0, 1);
	break;
	
    case WIDGET_FOCUS:
    case WIDGET_UNFOCUS:	
    case WIDGET_DRAW:
	attrset ((Msg == WIDGET_FOCUS) ? FOCUSC : NORMALC);
	widget_move (&c->widget, 0, 0);
	printw ("[%c] %s", (c->state & C_BOOL) ? 'x' : ' ', c->text);

	if (c->hotpos >= 0){
	    attrset ((Msg == WIDGET_FOCUS) ? HOT_FOCUSC : HOT_NORMALC);
	    widget_move (&c->widget, 0, + c->hotpos+4);
	    addch ((unsigned char)c->text [c->hotpos]);
	}
	return 1;
#endif /* !HAVE_X */
#endif /* !HAVE_XVIEW */
    }
    return default_proc (h, Msg, Par);
}

static int
check_event (Gpm_Event *event, WCheck *c)
{
#ifndef HAVE_X
    if (event->type & (GPM_DOWN|GPM_UP)){
    	Dlg_head *h = c->widget.parent;
	
	dlg_select_widget (h, c);
	if (event->type & GPM_UP){
	    check_callback (h, c, WIDGET_KEY, ' ');
	    check_callback (h, c, WIDGET_FOCUS, 0);
	    (*h->callback) (h, ' ', DLG_POST_KEY);
	    return MOU_NORMAL;
	}
    }
#endif
    return MOU_NORMAL;
}

static void
check_destroy (WCheck *c)
{
	x_destroy_cmd (c);
	free (c->text);
}

WCheck *
check_new (int y, int x, int state, char *text, char *tkname)
{
    WCheck *c =  xmalloc (sizeof (WCheck), "check_new");
    char *s, *t;
    
    init_widget (&c->widget, y, x, 1, strlen (text),
		 (callback_fn)check_callback,
		 (destroy_fn)check_destroy, (mouse_h) check_event, tkname);
    c->state = state ? C_BOOL : 0;
    c->text = strdup (text);
    c->hotkey = 0;
    c->hotpos = -1;
    widget_want_hotkey (c->widget, 1);

    /* Scan for the hotkey */
    for (s = text, t = c->text; *s; s++, t++){
	if (*s != '&'){
	    *t = *s;
	    continue;
	}
	s++;
	if (*s){
	    c->hotkey = tolower (*s);
	    c->hotpos = t - c->text;
	}
	*t = *s;
    }
    *t = 0;
    return c;
}


/* Label widget */

static int
label_callback (Dlg_head *h, WLabel *l, int Msg, int Par)
{
    if (Msg == WIDGET_INIT)
	return x_create_label (h, h->wdata, l);
    
    /* We don't want to get the focus */
    if (Msg == WIDGET_FOCUS)
	return 0;
#ifndef HAVE_X
    if (Msg == WIDGET_DRAW && l->text){
	char *p = l->text, *q, c = 0;
	int y = 0;
	if (l->transparent)
	    attrset (DEFAULT_COLOR);
	else
	    attrset (NORMALC);
	for (;;){
	    int xlen;
		
	    q = strchr (p, '\n');
	    if (q){
		c = *q;
		*q = 0;
	    }
	    widget_move (&l->widget, y, 0);
	    printw ("%s", p);
	    xlen = l->widget.cols - strlen (p);
	    if (xlen > 0)
		printw ("%*s", xlen, " ");
	    if (!q)
		break;
	    *q = c;
	    p = q + 1;
	    y++;
	}
	return 1;
    }
#endif    
    return default_proc (h, Msg, Par);
}

void
label_set_text (WLabel *label, char *text)
{
    int newcols = label->widget.cols;
    
    if (label->text && text && !strcmp (label->text, text))
        return; /* Flickering is not nice */

    if (label->text){
	free (label->text);
    }
    if (text){
	label->text = strdup (text);
	if (label->auto_adjust_cols) {
	    newcols = strlen (text);
	    if (newcols > label->widget.cols)
	    label->widget.cols = newcols;
	}
    } else
	label->text = 0;
    
    if (label->widget.parent)
#ifdef HAVE_X
	x_label_set_text (label, text);
#else
	label_callback (label->widget.parent, label, WIDGET_DRAW, 0);
#endif
    if (newcols < label->widget.cols)
        label->widget.cols = newcols;
}

static void
label_destroy (WLabel *l)
{
    x_destroy_cmd (l);
    if (l->text)
	free (l->text);
}

WLabel *
label_new (int y, int x, char *text, char *tkname)
{
    WLabel *l = xmalloc (sizeof (WLabel), "label_new");

    init_widget (&l->widget, y, x, 1, 1,
		 (callback_fn) label_callback,
		 (destroy_fn) label_destroy, NULL, tkname);
    l->text = text ? strdup (text) : 0;
    l->auto_adjust_cols = 1;
    l->transparent = 0;
    widget_want_cursor (l->widget, 0);
    return l;
}


/* Gauge widget (progress indicator) */
/* Currently width is hardcoded here for text mode */
#define gauge_len 47

static int
gauge_callback (Dlg_head *h, WGauge *g, int Msg, int Par)
{

    if (Msg == WIDGET_INIT)
	return x_create_gauge (h, h->wdata, g);
    
    /* We don't want to get the focus */
    if (Msg == WIDGET_FOCUS)
	return 0;

#ifndef HAVE_X
    if (Msg == WIDGET_DRAW){
	widget_move (&g->widget, 0, 0);
	attrset (NORMALC);
	if (!g->shown)
	    printw ("%*s", gauge_len, "");
	else {
	    long percentage, columns;
	    long total = g->max, done = g->current;
	    
	    if (total <= 0 || done < 0) {
	        done = 0;
	        total = 100;
	    }
	    if (done > total)
	        done = total;
	    while (total > 65535) {
	        total /= 256;
	        done /= 256;
	    }
	    percentage = (200 * done / total + 1) / 2;
	    columns = (2 * (gauge_len - 7) * done / total + 1) / 2;
	    addch ('[');
	    attrset (GAUGE_COLOR);
	    printw ("%*s", columns, "");
	    attrset (NORMALC);
	    printw ("%*s] %3d%%", gauge_len - 7 - columns, "", percentage);
	}
	return 1;
    }
#endif    
    return default_proc (h, Msg, Par);
}

void
gauge_set_value (WGauge *g, int max, int current)
{
    if (g->current == current && g->max == max)
    	return; /* Do not flicker */
    if (max == 0)
        max = 1; /* I do not like division by zero :) */
#ifdef HAVE_X
/* NOTE: x_gauge_set_value has to be called before we change actual 
 *       max and current values in g, since it assumes g->max and
 *       g->current as the previous values and max and current
 *       as the new ones :) */
    x_gauge_set_value (g, max, current);
#endif    
    g->current = current;
    g->max = max;
#ifndef HAVE_X
    gauge_callback (g->widget.parent, g, WIDGET_DRAW, 0);
#endif
}

void
gauge_show (WGauge *g, int shown)
{
    if (g->shown == shown)
        return;
    g->shown = shown;
#ifdef HAVE_X
    x_gauge_show (g);
#else
    gauge_callback (g->widget.parent, g, WIDGET_DRAW, 0);
#endif    
}

static void
gauge_destroy (WGauge *g)
{
    /* nothing */
}

WGauge *
gauge_new (int y, int x, int shown, int max, int current, char *tkname)
{
    WGauge *g = xmalloc (sizeof (WGauge), "gauge_new");

    init_widget (&g->widget, y, x, 1, gauge_len,
		 (callback_fn) gauge_callback,
		 (destroy_fn) gauge_destroy, NULL, tkname);
    g->shown = shown;
    if (max == 0)
        max = 1; /* I do not like division by zero :) */
    g->max = max;
    g->current = current;
    g->pixels = 0;
    widget_want_cursor (g->widget, 0);
    return g;
}


/* Input widget */

/* {{{ history button */
 
#define LARGE_HISTORY_BUTTON 1
 
#ifdef LARGE_HISTORY_BUTTON
#  define HISTORY_BUTTON_WIDTH 3
#else
#  define HISTORY_BUTTON_WIDTH 1
#endif
 
#define should_show_history_button(in) \
	    (in->history && in->field_len > HISTORY_BUTTON_WIDTH * 2 + 1 && in->widget.parent)

static void draw_history_button (WInput * in)
{
    char c;
    c = in->history->next ? (in->history->prev ? '|' : 'v') : '^';
    widget_move (&in->widget, 0, in->field_len - HISTORY_BUTTON_WIDTH);
#ifdef LARGE_HISTORY_BUTTON
    {
	Dlg_head *h;
	h = in->widget.parent;
#if 0
	attrset (NORMALC);	/* button has the same colour as other buttons */
	addstr ("[ ]");
	attrset (HOT_NORMALC);
#else
	attrset (NORMAL_COLOR);
	addstr ("[ ]");
	/* Too distracting: attrset (MARKED_COLOR); */
#endif
	widget_move (&in->widget, 0, in->field_len - HISTORY_BUTTON_WIDTH + 1);
	addch (c);
    }
#else
    attrset (MARKED_COLOR);
    addch (c);
#endif
}
 
/* }}} history button */


/* Input widgets now have a global kill ring */
/* Pointer to killed data */
static char *kill_buffer = 0;

void
update_input (WInput *in, int clear_first)
{
#ifndef HAVE_XVIEW
    int has_history = 0;
    int    i, j;
    char   c;
    int    buf_len = strlen (in->buffer);

    if (should_show_history_button (in))
	has_history = HISTORY_BUTTON_WIDTH;

    if (in->disable_update)
	return;

    /* Make the point visible */
    if ((in->point < in->first_shown) ||
	(in->point >= in->first_shown+in->field_len - has_history)){
	in->first_shown = in->point - (in->field_len / 3);
	if (in->first_shown < 0)
	    in->first_shown = 0;
    }

    /* Adjust the mark */
    if (in->mark > buf_len)
	in->mark = buf_len;
    
#ifdef HAVE_X
    if (clear_first && in->first)
	    in->first = -1;
    x_update_input (in);
#else

    if (has_history)
	draw_history_button (in);

    attrset (in->color);
    
    widget_move (&in->widget, 0, 0);
    for (i = 0; i < in->field_len - has_history; i++)
	addch (' ');
    widget_move (&in->widget, 0, 0);
    
    for (i = 0, j = in->first_shown; i < in->field_len - has_history && in->buffer [j]; i++){
	c = in->buffer [j++];
	c = is_printable (c) ? c : '.';
	if (in->is_password)
	    c = '*';
	addch (c);
    }
    widget_move (&in->widget, 0, in->point - in->first_shown);

    if (clear_first)
	    in->first = 0;
#endif

#endif
}

void
winput_set_origin (WInput *in, int x, int field_len)
{
    in->widget.x    = x;
    in->field_len = in->widget.cols = field_len;
    update_input (in, 0);
}

/* {{{ history saving and loading */

/*
   This loads and saves the history of an input line to and from the
   widget. It is called with the widgets tk name on creation of the
   widget, and returns the Hist list. It stores histories in the file
   ~/.mc/history in using the profile code.

   If def_text is passed as INPUT_LAST_TEXT (to the input_new()
   function) then input_new assigns the default text to be the last text
   entered, or "" if not found.
 */

int num_history_items_recorded = 60;

Hist *history_get (char *input_name)
{
    int i;
    Hist *old = 0, *new = 0;
    char *profile;
    
    if (!num_history_items_recorded)	/* this is how to disable */
	return 0;
    if (!input_name)
	return 0;
    if (!*input_name)
	return 0;
    profile = concat_dir_and_file (home_dir, HISTORY_FILE_NAME);
    for (i = 0;; i++) {
	char key_name[32];
	char this_entry[1024];
	sprintf (key_name, "%d", i);
	GetPrivateProfileString (input_name, key_name, "", this_entry, sizeof (this_entry), profile);
	if (!*this_entry)
	    break;
	new = xmalloc (sizeof (Hist), "history_get");
	memset (new, 0, sizeof (Hist));
	new->text = strdup (this_entry);
	new->prev = old;	/* set up list pointers */
	if (old)
	    old->next = new;
	old = new;
    }
    free (profile);
    return new;			/* return pointer to last entry in list */
}

#ifdef PORT_WIDGET_WANTS_HISTORY
void history_put (char *input_name, Hist *h)
{
    int i;
    char *profile;

    if (!input_name)
	return;

    if (!*input_name)
	return;
    
    if (!h)
	return;

    if (!num_history_items_recorded)	/* this is how to disable */
	return;

    profile = concat_dir_and_file (home_dir, HISTORY_FILE_NAME);
    while (h->next)		/* go to end of list */
	h = h->next;

    /* go back 60 places */
    for (i = 0; i < num_history_items_recorded - 1 && h->prev; i++)	
	h = h->prev;
    i = 0;

    if (input_name)
	profile_clean_section (input_name, profile);

    /* dump histories into profile */
    while (h){
	if (h->text){

	    /* probably aren't any null entries, but lets be sure */
	    if (*(h->text)){
		char key_name[32];
		sprintf (key_name, "%d", i++);
		WritePrivateProfileString (input_name, key_name, h->text, profile);
	    }
	}
	h = h->next;
    }
    free (profile);
}
#else
void history_put (char *input_name, Hist *h)
{
}
#endif

/* }}} history saving and loading */


/* {{{ history display */

static const char history_title[] = " History ";

int history_callback (Dlg_head * h, int Par, int Msg)
{
#ifndef HAVE_X
    switch (Msg) {
    case DLG_DRAW:
	attrset (COLOR_NORMAL);
	dlg_erase (h);
	draw_box (h, 0, 0, h->lines, h->cols);
	attrset (COLOR_HOT_NORMAL);
	dlg_move (h, 0, (h->cols - strlen (history_title)) / 2);
	printw ((char *) history_title);
	break;
    }
#endif
    return 0;
}

static inline int listbox_fwd (WListbox *l);

char *show_hist (Hist *history, int widget_x, int widget_y)
{
    Hist *hi, *z;
    int maxlen = strlen(history_title), i, count = 0;
    int x, y, w, h;
    char *q, *r = 0;
    Dlg_head *query_dlg;
    WListbox *query_list;

    z = history;
    if (!z)
		return 0;

    while (z->prev)		/* goto first */
		z = z->prev;
    hi = z;
    while (hi) {
		if ((i = strlen (hi->text)) > maxlen)
		    maxlen = i;
		count++;
		hi = hi->next;
    }

       y = widget_y;
	h = count + 2;
	if (h <= y || y > LINES - 6)
	{
		h = min(h, y - 1);
		y -= h;
	}
	else
	{
		y++;
		h = min(h, LINES - y);
	}

	x = widget_x - 2;
	if ((w = maxlen + 4) + x > COLS)
	{
		w = min(w,COLS);
		x = COLS - w;
	}

    query_dlg = create_dlg (y, x, h, w, dialog_colors, history_callback,
			    "[History-query]", "history", DLG_NONE);
    query_list = listbox_new (1, 1, w - 2, h - 2, listbox_finish, 0, NULL);
    add_widget (query_dlg, query_list);
    hi = z;
    if (y < widget_y) {
		while (hi) {		/* traverse */
		    listbox_add_item (query_list, 0, 0, hi->text, NULL);
	    	hi = hi->next;
		}
		while (listbox_fwd (query_list));
    } else {
		while (hi->next)
		    hi = hi->next;
		while (hi) {		/* traverse backwards */
		    listbox_add_item (query_list, 0, 0, hi->text, NULL);
	    	hi = hi->prev;
		}
    }
    run_dlg (query_dlg);
    q = NULL;
    if (query_dlg->ret_value != B_CANCEL) {
		listbox_get_current (query_list, &q, NULL);
		if (q)
		    r = strdup (q);
    }
    destroy_dlg (query_dlg);
    return r;
}

static void do_show_hist (WInput * in)
{
    char *r;
    r = show_hist (in->history, in->widget.x, in->widget.y);
    if (r) {
	assign_text (in, r);
	free (r);
    }
}

/* }}} history display */

static void
input_destroy (WInput *in)
{
    if (!in){
	fprintf (stderr, "Internal error: null Input *\n");
	exit (1);
    }

    new_input (in);
    if (in->history){
	Hist *current, *old;

	if (!in->is_password && PORT_WIDGET_WANTS_HISTORY)	/* don't save passwords ;-) */
	    history_put (in->history_name, in->history);

	current = in->history;
	while (current->next)
	    current = current->next;
	while (current){
	    old = current;
	    current = current->prev;
	    free (old->text);
	    free (old);
	}
    }
    x_destroy_cmd (in);
    free (in->buffer);
    free_completions (in);
    if (in->history_name)
	free (in->history_name);
}

static char disable_update = 0;

void
input_disable_update (WInput *in)
{
    in->disable_update++;
}

void
input_enable_update (WInput *in)
{
    in->disable_update--;
    update_input (in, 0);
}

int
push_history (WInput *in, char *text)
{
    Hist *new;
    char *p;
    
    for (p = text; *p == ' ' || *p == '\t'; p++);
    if (!*p)
        return 0;
    if (in->history){
	while (in->history->next)
	    in->history = in->history->next;
	if (!strcmp (in->history->text, text))
	    return 1;
    	new = xmalloc (sizeof (Hist), "push_history");
	in->history->next = new;
    } else
    	new = xmalloc (sizeof (Hist), "push_history");
    in->need_push = 0;
    new->next = 0;
    new->prev = in->history;
    new->text = strdup (text);
    in->history = new;
    return 2;
}

/* Cleans the input line and adds the current text to the history */
void
new_input (WInput *in)
{
    if (in->buffer)
	push_history (in, in->buffer);
    in->need_push = 1;
    in->buffer [0] = 0;
    in->point = 0;
    in->mark = 0;
    free_completions (in);
    update_input (in, 0);
}

static int
insert_char (WInput *in, int c_code)
{
    int i;

    if (c_code == -1)
	return 0;
    
    in->need_push = 1;
    if (strlen (in->buffer)+1 == in->current_max_len){
	/* Expand the buffer */
	char *narea = xmalloc(in->current_max_len + in->field_len, "string expansion");
	if (narea){
	    char *p = in->buffer;

	    strcpy (narea, in->buffer);
	    in->buffer = narea;
	    in->current_max_len += in->field_len;
	    free (p);
	}
    }
    if (strlen (in->buffer)+1 < in->current_max_len){
	int l = strlen (&in->buffer [in->point]);
	for (i = l+1; i > 0; i--)
	    in->buffer [in->point+i] = in->buffer [in->point+i-1];
	in->buffer [in->point] = c_code;
	in->point++;
    }
    return 1;
}

static void
beginning_of_line (WInput *in)
{
    in->point = 0;
}

static void
end_of_line (WInput *in)
{
    in->point = strlen (in->buffer);
}

static void
backward_char (WInput *in)
{
    if (in->point)
	in->point--;
}

static void
forward_char (WInput *in)
{
    if (in->buffer [in->point])
	in->point++;
}

static void
forward_word (WInput *in)
{
    char *p = in->buffer+in->point;

    while ((*p && isspace (*p)) || ispunct (*p))
	p++;
    while (*p && isalnum (*p))
	p++;
    in->point = p - in->buffer;
}

static void
backward_word (WInput *in)
{
    char *p = in->buffer+in->point;

    while (p-1 > in->buffer-1 && (isspace (*(p-1)) || ispunct (*(p-1))))
	p--;
    while (p-1 > in->buffer-1 && isalnum (*(p-1)))
	p--;
    in->point = p - in->buffer;
}

#ifdef __linux__
static void
key_left (WInput *in)
{
    if (ctrl_pressed ())
	backward_word (in);
    else
	backward_char (in);
}

static void
key_right (WInput *in)
{
    if (ctrl_pressed ())
	forward_word (in);
    else
	forward_char (in);
}
#else
#define key_left  backward_char
#define key_right forward_char
#endif
    
static void
backward_delete (WInput *in)
{
    int i;
    
    if (!in->point)
	return;
    for (i = in->point; in->buffer [i-1]; i++)
	in->buffer [i-1] = in->buffer [i];
    in->need_push = 1;
    in->point--;
}

static void
delete_char (WInput *in)
{
    int i;

    for (i = in->point; in->buffer [i]; i++)
	in->buffer [i] = in->buffer [i+1];
    in->need_push = 1;
}

static void
copy_region (WInput *in, int x_first, int x_last)
{
    int first = min (x_first, x_last);
    int last  = max (x_first, x_last);
    
    if (last == first)
	return;
    
    if (kill_buffer)
	free (kill_buffer);
    
    kill_buffer = xmalloc (last-first + 1, "copy_region");
    strncpy (kill_buffer, in->buffer+first, last-first);
    kill_buffer [last-first] = 0;
}

static void
delete_region (WInput *in, int x_first, int x_last)
{
   int first = min (x_first, x_last);
   int last  = max (x_first, x_last);

   in->point = first;
   in->mark  = first;
   strcpy (&in->buffer [first], &in->buffer [last]);
   in->need_push = 1;
}

static void
kill_word (WInput *in)
{
    int old_point = in->point;
    int new_point;

    forward_word (in);
    new_point = in->point;
    in->point = old_point;

    copy_region (in, old_point, new_point);
    delete_region (in, old_point, new_point);
    in->need_push = 1;
}

static void
back_kill_word (WInput *in)
{
    int old_point = in->point;
    int new_point;

    backward_word (in);
    new_point = in->point;
    in->point = old_point;

    copy_region (in, old_point, new_point);
    delete_region (in, old_point, new_point);
    in->need_push = 1;
}

static void
set_mark (WInput *in)
{
    in->mark = in->point;
}

static void
kill_save (WInput *in)
{
    copy_region (in, in->mark, in->point);
}

static void
kill_region (WInput *in)
{
    kill_save (in);
    delete_region (in, in->point, in->mark);
}

static void
yank (WInput *in)
{
    char *p;
    
    if (!kill_buffer)
        return;
    for (p = kill_buffer; *p; p++)
	insert_char (in, *p);
}

static void
kill_line (WInput *in)
{
    if (kill_buffer)
	free (kill_buffer);
    kill_buffer = strdup (&in->buffer [in->point]);
    in->buffer [in->point] = 0;
}

void
assign_text (WInput *in, char *text)
{
    free_completions (in);
    free (in->buffer);
    in->buffer = strdup (text);	/* was in->buffer->text */
    in->current_max_len = strlen (in->buffer) + 1;
    in->point = strlen (in->buffer);
    in->mark = 0;
    in->need_push = 1;
}

static void
hist_prev (WInput *in)
{
    if (!in->history)
	return;

    if (in->need_push) {
	switch (push_history (in, in->buffer)) {
	 case 2: in->history = in->history->prev; break;
	 case 1: if (in->history->prev) in->history = in->history->prev; break;
	 case 0: break;
	}
    } else if (in->history->prev)
        in->history = in->history->prev;
    else
        return;
    assign_text (in, in->history->text);
    in->need_push = 0;
}

static void
hist_next (WInput *in)
{
    if (in->need_push) {
        switch (push_history (in, in->buffer)) {
         case 2:
            assign_text (in, "");
            return;
         case 0:
            return;
        }
    }
    
    if (!in->history)
	return;

    if (!in->history->next) {
        assign_text (in, "");
	return;
    }
    
    in->history = in->history->next;
    assign_text (in, in->history->text);
    in->need_push = 0;
}

static struct {
    int key_code;
    void (*fn)(WInput *in);
} input_map [] = {
    /* Motion */
    { XCTRL('a'),         beginning_of_line },
    { KEY_HOME,	          beginning_of_line },
    { KEY_A1,	          beginning_of_line },
    { XCTRL('e'),         end_of_line },
    { KEY_END,            end_of_line },
    { KEY_C1,             end_of_line },
    { KEY_LEFT,           key_left },
    { XCTRL('b'),         backward_char },
    { ALT('b'),           backward_word },
    { KEY_RIGHT,          key_right },
    { XCTRL('f'),         forward_char },
    { ALT('f'),           forward_word },
		          
    /* Editing */         
    { 0177,               backward_delete },
    { KEY_BACKSPACE,      backward_delete },
    { XCTRL('h'),         backward_delete },
    { KEY_DC,             delete_char },
    { XCTRL('d'),         delete_char },
    { ALT('d'),           kill_word },
    { ALT(KEY_BACKSPACE), back_kill_word },
    { ALT(XCTRL('h')),    back_kill_word },
    { ALT(127),           back_kill_word },
    
    /* Region manipulation */
    { 0,              	  set_mark },
    { XCTRL('w'),     	  kill_region },
    { ALT('w'),       	  kill_save },
    { XCTRL('y'),     	  yank },
    { XCTRL('k'),     	  kill_line },
    		      	  
    /* History */     	  
    { ALT('p'),       	  hist_prev },
    { ALT('n'),       	  hist_next },
    { ALT('h'),       	  do_show_hist },
    
    /* Completion */
    { ALT('\t'),	  complete },
    
    { 0,            0 }
};

/* This function is a test for a special input key used in complete.c */
/* Returns 0 if it is not a special key, 1 if it is a non-complete key
   and 2 if it is a complete key */
int
is_in_input_map (WInput *in, int c_code)
{
    int i;
    
    for (i = 0; input_map [i].fn; i++)
	if (c_code == input_map [i].key_code)
	    if (input_map [i].fn == complete)
	    	return 2;
	    else
	    	return 1;
    return 0;
}

#ifdef PORT_WINPUT_DELETES_MARKED
static void
port_region_marked_for_delete (WInput *in)
{
	kill_region (in);
}
#else
static void
port_region_marked_for_delete (WInput *in)
{
    *in->buffer = 0;
    in->point = 0;
    in->first = 0;
}
#endif

int
handle_char (WInput *in, int c_code)
{
    int    i;
    int    v;

    v = 0;

#ifdef HAVE_TK    
    in->inserted_one = 0;
#endif
    if (quote){
    	free_completions (in);
	v = insert_char (in, c_code);
	update_input (in, 1);
	quote = 0;
	return v;
    }

    for (i = 0; input_map [i].fn; i++){
	if (c_code == input_map [i].key_code){
	    if (input_map [i].fn != complete)
	    	free_completions (in);
	    (*input_map [i].fn)(in);
	    v = 1;
	    break;
	}
    }
    if (!input_map [i].fn){
	if (c_code > 255 || !is_printable (c_code))
	    return 0;
	if (in->first){
	    port_region_marked_for_delete (in);
	}
    	free_completions (in);
	v = insert_char (in, c_code);
	in->inserted_one = c_code;
    }
    if (!disable_update)
	update_input (in, 1);
    return v;
}

/* Inserts text in input line */
void
stuff (WInput *in, char *text, int insert_extra_space)
{
    input_disable_update (in);
    while (*text)
	handle_char (in, *text++);
    if (insert_extra_space)
	handle_char (in, ' ');
    input_enable_update (in);
    update_input (in, 1);
}

void
input_set_point (WInput *in, int pos)
{
    if (pos > in->current_max_len)
	pos = in->current_max_len;
    if (pos != in->point)
    	free_completions (in);
    in->point = pos;
    update_input (in, 1);
}

int input_event (Gpm_Event *event, WInput *b);

static int
input_callback (Dlg_head *h, WInput *in, int Msg, int Par)
{
    switch (Msg){
    case WIDGET_INIT:
	return x_create_input (h, h->wdata, in);

#ifndef HAVE_XVIEW
    case WIDGET_KEY:
	if (Par == XCTRL('q')){
	    int v;
	    
	    quote = 1;
	    v = handle_char (in, mi_getch ());
	    quote = 0;
	    return v;
	}
	if (Par == KEY_UP || Par == KEY_DOWN ||
	    Par == ESC_CHAR || Par == KEY_F(10) ||
	    Par == XCTRL('g'))
	    return 0;		/* We don't handle up/down */

	if (Par == '\n'){
	    dlg_one_down (h);
	    return 1;
	}
	return handle_char (in, Par);

    case WIDGET_FOCUS:
    case WIDGET_UNFOCUS:	
    case WIDGET_DRAW:
	update_input (in, 0);
	break;
#endif /* !HAVE_XVIEW */
#ifndef HAVE_X
    case WIDGET_CURSOR:
	widget_move (&in->widget, 0, in->point - in->first_shown);
	return 1;
#endif
	
    }
    return default_proc (h, Msg, Par);
}

/* Not declared static, since we check against this value in dlg.c */
/* FIXME: Declare static again and provide an identification mechanism */
int 
input_event (Gpm_Event *event, WInput *in) 
{ 
#ifndef HAVE_X
    if (event->type & (GPM_DOWN|GPM_DRAG)){ 
	dlg_select_widget (in->widget.parent, in); 

	if (event->x >= in->field_len - HISTORY_BUTTON_WIDTH + 1 && should_show_history_button (in)) {
	    do_show_hist (in);
	    update_input (in, 1);
	} else {
	    in->point = strlen (in->buffer);
	    if (event->x - in->first_shown - 1 < in->point)
		in->point = event->x - in->first_shown - 1;
	    if (in->point < 0)
		in->point = 0;

	    update_input (in, 1);
	} 
    } 
#endif
    return MOU_NORMAL; 
} 

WInput *
input_new (int y, int x, int color, int len, char *def_text, char *tkname)
{
    WInput *in = xmalloc (sizeof (WInput), "input_new");
    int initial_buffer_len;

    init_widget (&in->widget, y, x, 1, len,
		 (callback_fn) input_callback,
	      (destroy_fn) input_destroy, (mouse_h) input_event, tkname);

    /* history setup */
    in->history = NULL;
    in->history_name = 0;
    if (tkname && PORT_WIDGET_WANTS_HISTORY){
	if (*tkname) {
	    in->history_name = strdup (tkname);
	    in->history = history_get (tkname);
	}
    }
    if (def_text == INPUT_LAST_TEXT) {
	def_text = "";
	if (in->history)
	    if (in->history->text)
		def_text = in->history->text;
    }
    initial_buffer_len = 1 + max (len, strlen (def_text));
    in->widget.options |= W_IS_INPUT;
    in->completions = NULL;
    in->completion_flags =
	INPUT_COMPLETE_FILENAMES | INPUT_COMPLETE_HOSTNAMES |
	INPUT_COMPLETE_VARIABLES | INPUT_COMPLETE_USERNAMES;
    in->current_max_len = initial_buffer_len;
    in->buffer = xmalloc (initial_buffer_len, "create_input: in->buffer");
    in->color = color;
    in->field_len = len;
    in->first = 1;
    in->first_shown = 0;
    in->disable_update = 0;
    in->mark = 0;
    in->need_push = 1;
    in->is_password = 0;

    strcpy (in->buffer, def_text);
    in->point = strlen (in->buffer);
    in->first = 1;
    return in;
}


/* Listbox widget */

/* Should draw the scrollbar, but currently draws only
 * indications that there is more information
 */
static int listbox_cdiff (WLEntry *s, WLEntry *e);

static void
listbox_drawscroll (WListbox *l)
{
    extern int slow_terminal;
    int line;
    int i, top;
    int max_line = l->height-1;
    
    /* Are we at the top? */
    widget_move (&l->widget, 0, l->width);
    if (l->list == l->top)
	one_vline ();
    else
	addch ('^');

    /* Are we at the bottom? */
    widget_move (&l->widget, max_line, l->width);
    top = listbox_cdiff (l->list, l->top);
    if ((top + l->height == l->count) || l->height >= l->count)
	one_vline ();
    else
	addch ('v');

    /* Now draw the nice relative pointer */
    if (l->count)
	line = 1+ ((l->pos * (l->height-2)) / l->count);
    else
	line = 0;
    
    for (i = 1; i < max_line; i++){
	widget_move (&l->widget, i, l->width);
	if (i != line)
	    one_vline ();
	else
	    addch ('*');
    }
}
    
static void
listbox_draw (WListbox *l, Dlg_head *h, int focused)
{
    WLEntry *e;
    int i;
    int sel_line;
    int normalc, selc;
    char *text; 

    if (focused){
	normalc = NORMALC;
	selc    = FOCUSC;
    } else {
	normalc = NORMALC;
	selc    = HOT_FOCUSC;
    }
    sel_line = -1;

    for (e = l->top, i = 0; (i < l->height); i++){
	
	/* Display the entry */
	if (e == l->current && sel_line == -1){
	    sel_line = i;
	    attrset (selc);
	} else
	    attrset (normalc);

	widget_move (&l->widget, i, 0);

	if ((i > 0 && e == l->list) || !l->list)
	    text = "";
	else {
	    text = e->text;
	    e = e->next;
	}
	printw (" %-*s ", l->width-2, name_trunc (text, l->width-2));
    }
    l->cursor_y = sel_line;
    if (!l->scrollbar)
	return;
    attrset (normalc);
    listbox_drawscroll (l);
}

/* Returns the number of items between s and e,
   must be on the same linked list */
static int
listbox_cdiff (WLEntry *s, WLEntry *e)
{
    int count;

    for (count = 0; s != e; count++)
	s = s->next;
    return count;
}

static WLEntry *
listbox_check_hotkey (WListbox *l, int key)
{
    int i;
    WLEntry *e;
    
    i = 0;
    e = l->list;
    if (!e)
	return 0;
    
    while (1){

	/* If we didn't find anything, return */
	if (i && e == l->list)
	    return 0;

	if (e->hotkey == key)
	    return e;
	
	i++;
	e = e->next;
    }
}

/* Used only for display updating, for avoiding line at a time scroll */
void
listbox_select_last (WListbox *l, int set_top)
{
    if (l->list){
	l->current = l->list->prev;
	l->pos = l->count - 1;
	if (set_top)
	    l->top = l->list->prev;
	x_listbox_select_nth (l, l->pos);
    }
}

void
listbox_remove_list (WListbox *l)
{
    WLEntry *p, *q;

    if (!l->count)
	return;

#ifdef HAVE_X
    if (l->widget.wdata != (widget_data) NULL) {
	int i;
	for (i = 0; i < l->count; i++)
	    x_listbox_delete_nth (l, i);
    }
#endif
    p = l->list;
    
    while (l->count--) {
	q = p->next;
	free (p->text);
	free (p);
	p = q;
    }
    l->pos = l->count = 0;
    l->list = l->top = l->current = 0;
}

/*
 * bor 30.10.96: added force flag to remove *last* entry as well
 * bor 30.10.96: corrected selection bug if last entry was removed
 */

void
listbox_remove_current (WListbox *l, int force)
{
    WLEntry *p;
    
    /* Ok, note: this won't allow for emtpy lists */
    if (!force && (!l->count || l->count == 1))
	return;
    
#ifdef HAVE_X
    if (l->widget.wdata != (widget_data) NULL) {
        x_listbox_delete_nth (l, l->pos);
	if (l->count > 1)
	    if (l->current->next != l->list)
		x_listbox_select_nth (l, l->pos);
	    else if (l->current != l->list)
		x_listbox_select_nth (l, l->pos - 1);
	else
	    x_listbox_select_nth (l, 0);
    }
#endif
    l->count--;
    p = l->current;

    if (l->count) {
	l->current->next->prev = l->current->prev;
	l->current->prev->next = l->current->next;
	if (p->next == l->list) {
	    l->current = p->prev;
	    l->pos--;
	}	
	else 
	    l->current = p->next;
	
	if (p == l->list)
	    l->list = l->top = p->next;
    } else {
	l->pos = 0;
	l->list = l->top = l->current = 0;
    }

    free (p->text);
    free (p);
}

/* Makes *e the selected entry (sets current and pos) */
void
listbox_select_entry (WListbox *l, WLEntry *dest)
{
    WLEntry *e;
    int pos;
    int top_seen;
    
    top_seen = 0;
    
    /* Special case */
    for (pos = 0, e = l->list; pos < l->count; e = e->next, pos++){

	if (e == l->top)
	    top_seen = 1;
	
	if (e == dest){
	    l->current = e;
	    if (top_seen){
		while (listbox_cdiff (l->top, l->current) >= l->height)
		    l->top = l->top->next;
	    } else {
		l->top = l->current;
	    }
	    l->pos = pos;
	    x_listbox_select_nth (l, l->pos);
	    return;
	}
    }
    /* If we are unable to find it, set decent values */
    l->current = l->top = l->list;
    l->pos = 0;
    x_listbox_select_nth (l, l->pos);
}

/* Selects from base the pos element */
static WLEntry *
listbox_select_pos (WListbox *l, WLEntry *base, int pos)
{
    WLEntry *last = l->list->prev;

    if (base == last)
    	return last;
    while (pos--){
	base = base->next;
	if (base == last)
	    break;
    }
    return base;
}

static inline int
listbox_back (WListbox *l)
{
    if (l->pos){
	listbox_select_entry (l, listbox_select_pos (l, l->list, l->pos-1));
	return 1;
    }
    return 0;
}

static inline int
listbox_fwd (WListbox *l)
{
    if (l->current != l->list->prev){
	listbox_select_entry (l, listbox_select_pos (l, l->list, l->pos+1));
	return 1;
    }
    return 0;
}

/* Returns 1 if we want a redraw */
static int
listbox_key (WListbox *l, int key)
{
    int i;
    int j = 0;

    if (!l->list)
	return 0;
    
    switch (key){
    case KEY_HOME:
    case KEY_A1:
	l->current = l->top = l->list;
	l->pos = 0;
	return 1;
	
    case KEY_END:
    case KEY_C1:
	l->current = l->top = l->list->prev;
	for (i = min (l->height - 1, l->count - 1); i; i--)
	    l->top = l->top->prev;
	l->pos = l->count - 1;
	return 1;
	
    case XCTRL('p'):
    case KEY_UP:
	listbox_back (l);
	return 1;
	
    case XCTRL('n'):
    case KEY_DOWN:
	listbox_fwd (l);
	return 1;

    case KEY_NPAGE:
    case XCTRL('v'):
	for (i = 0; i < l->height-1; i++)
	    j |= listbox_fwd (l);
	return j > 0;
	
    case KEY_PPAGE:
    case ALT('v'):
	for (i = 0; i < l->height-1; i++)
	    j |= listbox_back (l);
	return j > 0;
    }
    return 0;
}

static int listbox_event (Gpm_Event *event, WListbox *l);
static int
listbox_callback (Dlg_head *h, WListbox *l, int msg, int par)
{
    WLEntry  *e;
    /* int selected_color; Never used */
    int ret_code;
    
    switch (msg){
    case WIDGET_INIT:
	return x_create_listbox (h, h->wdata, l);
	
#ifndef	HAVE_XVIEW
    case WIDGET_HOTKEY:
	if ((e = listbox_check_hotkey (l, par)) != NULL){
	    listbox_select_entry (l, e);

	    /* Take the appropriate action */
	    if (l->action == listbox_finish){
		l->widget.parent->running   = 0;
		l->widget.parent->ret_value = B_ENTER;
	    } else if (l->action == listbox_cback){
		if ((*l->cback)(l) == listbox_finish){
		    l->widget.parent->running = 0;
		    l->widget.parent->ret_value = B_ENTER;
		}
	    }
	    return 1;
	} else
	    return 0;
	
    case WIDGET_KEY:
	if ((ret_code = listbox_key (l, par)))
	    listbox_draw (l, h, 1);
	return ret_code;

#ifndef HAVE_X
    case WIDGET_CURSOR:
	widget_move (&l->widget, l->cursor_y, 0);
	return 1;
	
    case WIDGET_FOCUS:
    case WIDGET_UNFOCUS:
    case WIDGET_DRAW:
	listbox_draw (l, h, msg != WIDGET_UNFOCUS);
	return 1;
#endif	
#endif /* !HAVE_XVIEW */
    }
    return default_proc (h, msg, par);
}

static int
listbox_event (Gpm_Event *event, WListbox *l)
{
#ifndef HAVE_X
    int i;
    
    Dlg_head *h = l->widget.parent;
    
    /* Single click */
    if (event->type & GPM_DOWN)
	dlg_select_widget (l->widget.parent, l);
    if (!l->list)
	return MOU_NORMAL;
    if (event->type & (GPM_DOWN|GPM_DRAG)){
	if (event->x < 0 || event->x >= l->width)
	    return MOU_REPEAT;
	if (event->y < 1)
	    for (i = -event->y; i >= 0; i--)
		listbox_back (l);
	else if (event->y > l->height)
	    for (i = event->y - l->height; i > 0; i--)
		listbox_fwd (l);
	else
            listbox_select_entry (l, listbox_select_pos (l, l->top,
	      					         event->y - 1));
	
	/* We need to refresh ourselves since the dialog manager doesn't */
	/* know about this event */
	listbox_callback (h, l, WIDGET_DRAW, 0);
	mc_refresh ();
	return MOU_REPEAT;
    }

    /* Double click */
    if ((event->type & (GPM_DOUBLE|GPM_UP)) == (GPM_UP|GPM_DOUBLE)){
     	if (event->x < 0 || event->x >= l->width)
     	    return MOU_NORMAL;
     	if (event->y < 1 || event->y > l->height)
     	    return MOU_NORMAL;
	
	dlg_select_widget (l->widget.parent, l);
	listbox_select_entry (l, listbox_select_pos (l, l->top, event->y - 1)); 

	switch (l->action){
	case listbox_nothing:
	    break;

	case listbox_finish:
	    h->ret_value = B_ENTER;
	    dlg_stop (h);
	    return MOU_ENDLOOP;

	case listbox_cback:
	    if ((*l->cback)(l) == listbox_finish)
		return MOU_ENDLOOP;
	}
    }
#endif    
    return MOU_NORMAL;
}

static void
listbox_destroy (WListbox *l)
{
    WLEntry *n, *p = l->list;
    int i;

    x_destroy_cmd (l);
    for (i = 0; i < l->count; i++){
	n = p->next;
	free (p->text);
	free (p);
	p = n;
    }
}

WListbox *
listbox_new (int y, int x, int width, int height,
	     int action, lcback callback, char *tkname)
{
    WListbox *l = xmalloc (sizeof (WListbox), "listbox_new");
    extern int slow_terminal;
    
    init_widget (&l->widget, y, x, height, width,
		 (callback_fn)listbox_callback,
		 (destroy_fn) listbox_destroy, (mouse_h)listbox_event, tkname);

    l->list   = l->top = l->current = 0;
    l->pos    = 0;
    l->width  = width;
    l->height = height;
    l->count  = 0;
    l->top    = 0;
    l->current= 0;
    l->cback  = callback;
    l->action = action;
    l->allow_duplicates = 1;
    l->scrollbar = slow_terminal ? 0 : 1;
    widget_want_hotkey (l->widget, 1);
    
    return l;
}

/* Listbox item adding function.  They still lack a lot of functionality */
/* any takers? */
/* 1.11.96 bor: added pos argument to control placement of new entry */
static void
listbox_append_item (WListbox *l, WLEntry *e, enum append_pos pos)
{
    if (!l->list){
	l->list = e;
	l->top = e;
	l->current = e;
	e->next = l->list;
	e->prev = l->list;
    } else if (pos == LISTBOX_APPEND_AT_END) {
	e->next = l->list;
	e->prev = l->list->prev;
	l->list->prev->next = e;
	l->list->prev = e;
    } else if (pos == LISTBOX_APPEND_BEFORE){
	e->next = l->current;
	e->prev = l->current->prev;
	l->current->prev->next = e;
	l->current->prev = e;
	if (l->list == l->current) {	/* move list one position down */
	    l->list = e;
	    l->top =  e;
	}
    } else if (pos == LISTBOX_APPEND_AFTER) {
	e->prev = l->current;
	e->next = l->current->next;
	l->current->next->prev = e;
	l->current->next = e;
    }
    x_list_insert (l, l->list, e);
    l->count++;
}

char *
listbox_add_item (WListbox *l, enum append_pos pos, int hotkey, char *text,
			void *data)
{
    WLEntry *entry;

    if (!l)
	return 0;

    if (!l->allow_duplicates)
	if (listbox_search_text (l, text))
	    return 0;
	    
    entry = xmalloc (sizeof (WLEntry), "listbox_add_item");
    entry->text = strdup (text);
    entry->data = data;
    entry->hotkey = hotkey;

    listbox_append_item (l, entry, pos);
    
    return entry->text;
}

/* Selects the nth entry in the listbox */
void
listbox_select_by_number (WListbox *l, int n)
{
    listbox_select_entry (l, listbox_select_pos (l, l->list, n));
}

WLEntry *
listbox_search_text (WListbox *l, char *text)
{
    WLEntry *e;

    e = l->list;
    if (!e)
	return NULL;
    
    do {
	if(!strcmp (e->text, text))
	    return e;
	e = e->next;
    } while (e!=l->list);

    return NULL;
}

/* Returns the current string text as well as the associated extra data */
void
listbox_get_current (WListbox *l, char **string, char **extra)
{
    if (!l->current){
	*string = 0;
	*extra  = 0;
    }
    if (string && l->current)
	*string = l->current->text;
    if (extra && l->current)
	*extra = l->current->data;
}

int
buttonbar_callback (Dlg_head *h, WButtonBar *bb, int msg, int par)
{
    int i;
    
    switch (msg){
    case WIDGET_INIT:
	return x_create_buttonbar (h, h->wdata, bb);

    case WIDGET_FOCUS:
	return 0;

#ifndef	HAVE_XVIEW
    case WIDGET_HOTKEY:
	for (i = 0; i < 10; i++){
	    if (par == KEY_F(i+1) && bb->labels [i].function){
		(*bb->labels [i].function)(bb->labels [i].data);
		return 1;
	    }
	}
	return 0;
	
#ifndef HAVE_X
    case WIDGET_DRAW:
	if (!bb->visible)
	    return 1;
	widget_move (&bb->widget, 0, 0);
	attrset (DEFAULT_COLOR);
	printw ("%-*s", bb->widget.cols - 1, "");
	for (i = 0; i < COLS/8 && i < 10; i++){
	    widget_move (&bb->widget, 0, i*8);
	    attrset (DEFAULT_COLOR);
	    printw ("%d", i+1);
	    attrset (SELECTED_COLOR);
	    printw ("%-*s", ((i+1) * 8 == COLS ? 5 : 6),
		    bb->labels [i].text ? bb->labels [i].text : "");
	    attrset (DEFAULT_COLOR);
	}
	attrset (SELECTED_COLOR);
	return 1;
#endif
#endif /* !HAVE_XVIEW */
    }
    return default_proc (h, msg, par);
}

static void
buttonbar_destroy (WButtonBar *bb)
{
    int i;

    for (i = 0; i < 10; i++){
	if (bb->labels [i].text)
	    free (bb->labels [i].text);
    }
}

static int
buttonbar_event (Gpm_Event *event, WButtonBar *bb)
{
#ifndef HAVE_X
    int button;

    if (!(event->type & GPM_UP))
	return MOU_NORMAL;
    if (event->y == 2)
	return MOU_NORMAL;
    button = event->x / 8;
    if (button < 10 && bb->labels [button].function)
	(*bb->labels [button].function)(bb->labels [button].data);
#endif
    return MOU_NORMAL;
}

WButtonBar *
buttonbar_new (int visible)
{
    int i;
    WButtonBar *bb = xmalloc (sizeof (WButtonBar), "buttonbar_new");

    init_widget (&bb->widget, LINES-1, 0, 1, COLS,
		 (callback_fn) buttonbar_callback,
		 (destroy_fn) buttonbar_destroy, (mouse_h) buttonbar_event, NULL);
    
    bb->visible = visible;
    for (i = 0; i < 10; i++){
	bb->labels [i].text     = 0;
	bb->labels [i].function = 0;
    }
    widget_want_hotkey (bb->widget, 1);
    widget_want_cursor (bb->widget, 0);

    return bb;
}

void
set_label_text (WButtonBar *bb, int index, char *text)
{
    if (bb->labels [index-1].text)
	free (bb->labels [index-1].text);

    bb->labels [index-1].text = strdup (text);
}

/* paneletc is either the panel widget, or info or view or tree widget */
WButtonBar *
find_buttonbar (Dlg_head *h, Widget *paneletc)
{
    WButtonBar *bb;
    Widget_Item *item;
    int i;

    bb = 0;
    for (i = 0, item = h->current; i < h->count; i++, item = item->next){
	if (item->widget->callback == (callback_fn) buttonbar_callback){
	    bb = (WButtonBar *) item->widget;
#ifdef HAVE_XVIEW
	    /* Jakub: do we really need this routine here?
	     * Does XView hold more that a buttonbar per Dlg_head?
	     */
	    if (x_find_buttonbar_check (bb, paneletc)) {
	        bb = 0;
	        continue;
	    }
#endif	    
	    break;
	}
    }
    return bb;
}

void
define_label_data (Dlg_head *h, Widget *paneletc, int idx, char *text,
		   buttonbarfn cback, void *data)
{
    WButtonBar *bb = find_buttonbar (h, paneletc);
    if (!bb)
	return;
    
    set_label_text (bb, idx, text);
    bb->labels [idx-1].function = (void (*)(void *)) cback;
    bb->labels [idx-1].data = data;
    x_redefine_label (bb, idx);
}

void
define_label (Dlg_head *h, Widget *paneletc, int idx, char *text, void (*cback)(void))
{
    define_label_data (h, paneletc, idx, text, (void (*)(void *)) cback, 0);
}

#ifdef HAVE_X
void redraw_labels (Dlg_head *h, Widget *paneletc)
{
}

#else
void
redraw_labels (Dlg_head *h, Widget *paneletc)
{
    Widget_Item *item;
    int i;

    for (i = 0, item = h->current; i < h->count; i++, item = item->next){
	if (item->widget->callback == (callback_fn) buttonbar_callback){
	    widget_redraw (h, item);
	    return;
	}
    }
}
#endif

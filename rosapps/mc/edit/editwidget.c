/* editor initialisation and callback handler.

   Copyright (C) 1996, 1997 the Free Software Foundation

   Authors: 1996, 1997 Paul Sheer

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
#include "edit.h"

#ifndef MIDNIGHT
#include <X11/Xmd.h>		/* CARD32 */
#include <X11/Xatom.h>
#include "app_glob.c"
#include "coollocal.h"
#include "editcmddef.h"
#include "mousemark.h"
#endif


#ifndef MIDNIGHT

extern int EditExposeRedraw;
CWidget *wedit = 0;

void edit_destroy_callback (CWidget * w)
{
    if (w) {
	edit_clean (w->editor);
	if (w->editor)
	    free (w->editor);
	w->editor = NULL;
    } else
/* NLS ? */
	CError ("Trying to destroy non-existing editor widget.\n");
}

void link_hscrollbar_to_editor (CWidget * scrollbar, CWidget * editor, XEvent * xevent, CEvent * cwevent, int whichscrbutton);

extern int option_editor_bg_normal;
void edit_tri_cursor (Window win);
/* starting_directory is for the filebrowser */
CWidget *CDrawEditor (const char *identifier, Window parent, int x, int y,
	   int width, int height, const char *text, const char *filename,
		const char *starting_directory, unsigned int options, unsigned long text_size)
{
    static made_directory = 0;
    int extra_space_for_hscroll = 0;
    CWidget *w;
    WEdit *e;

    if (options & EDITOR_HORIZ_SCROLL)
	extra_space_for_hscroll = 8;

    wedit = w = CSetupWidget (identifier, parent, x, y,
			      width + 7, height + 6, C_EDITOR_WIDGET,
		   ExposureMask | ButtonPressMask | ButtonReleaseMask | \
		     KeyPressMask | KeyReleaseMask | ButtonMotionMask | \
			      PropertyChangeMask | StructureNotifyMask | \
			      EnterWindowMask | LeaveWindowMask, color_palette (option_editor_bg_normal), 1);
    edit_tri_cursor (w->winid);
    w->options = options | WIDGET_TAKES_SELECTION;

    w->destroy = edit_destroy_callback;
    if (filename)
	w->label = strdup (filename);
    else
	w->label = strdup ("");

    if (!made_directory) {
	mkdir (catstrs (home_dir, EDIT_DIR, 0), 0700);
	made_directory = 1;
    }
    e = w->editor = CMalloc (sizeof (WEdit));
    if (!w->editor) {
/* Not essential to translate */
	CError (_ ("Error initialising editor.\n"));
	return 0;
    }
    w->editor->widget = w;
    e = w->editor = edit_init (e, height / FONT_PIX_PER_LINE, width / FONT_MEAN_WIDTH, filename, text, starting_directory, text_size);
    if (!e) {
	CDestroyWidget (w->ident);
	return 0;
    }
    e->macro_i = -1;
    e->widget = w;

    set_hint_pos (x + width + 7 + WIDGET_SPACING, y + height + 6 + WIDGET_SPACING + extra_space_for_hscroll);
    if (extra_space_for_hscroll) {
	w->hori_scrollbar = CDrawHorizontalScrollbar (catstrs (identifier, ".hsc", 0), parent,
		x, y + height + 6, width + 6, 12, 0, 0);
	CSetScrollbarCallback (w->hori_scrollbar->ident, w->ident, link_hscrollbar_to_editor);
    }
    if (!(options & EDITOR_NO_TEXT))
	CDrawText (catstrs (identifier, ".text", 0), parent, x, y + height + 6 + WIDGET_SPACING + extra_space_for_hscroll, "%s", e->filename);
    if (!(options & EDITOR_NO_SCROLL)) {
	w->vert_scrollbar = CDrawVerticalScrollbar (catstrs (identifier, ".vsc", 0), parent,
		x + width + 7 + WIDGET_SPACING, y, height + 6, 20, 0, 0);
	CSetScrollbarCallback (w->vert_scrollbar->ident, w->ident, link_scrollbar_to_editor);
    }
    return w;
}

void update_scroll_bars (WEdit * e)
{
    int i, x1, x2;
    CWidget *scroll;
    scroll = e->widget->vert_scrollbar;
    if (scroll) {
	i = e->total_lines - e->start_line + 1;
	if (i > e->num_widget_lines)
	    i = e->num_widget_lines;
	if (e->total_lines) {
	    x1 = (double) 65535.0 *e->start_line / (e->total_lines + 1);
	    x2 = (double) 65535.0 *i / (e->total_lines + 1);
	} else {
	    x1 = 0;
	    x2 = 65535;
	}
	if (x1 != scroll->firstline || x2 != scroll->numlines) {
	    scroll->firstline = x1;
	    scroll->numlines = x2;
	    EditExposeRedraw = 1;
	    render_scrollbar (scroll);
	    EditExposeRedraw = 0;
	}
    }
    scroll = e->widget->hori_scrollbar;
    if (scroll) {
	i = e->max_column - (-e->start_col) + 1;
	if (i > e->num_widget_columns * FONT_MEAN_WIDTH)
	    i = e->num_widget_columns * FONT_MEAN_WIDTH;
	x1 = (double) 65535.0 *(-e->start_col) / (e->max_column + 1);
	x2 = (double) 65535.0 *i / (e->max_column + 1);
	if (x1 != scroll->firstline || x2 != scroll->numlines) {
	    scroll->firstline = x1;
	    scroll->numlines = x2;
	    EditExposeRedraw = 1;
	    render_scrollbar (scroll);
	    EditExposeRedraw = 0;
	}
    }
}

/* returns the position in the edit buffer of a window click */
long edit_get_click_pos (WEdit * edit, int x, int y)
{
    long click;
/* (1) goto to left margin */
    click = edit_bol (edit, edit->curs1);

/* (1) move up or down */
    if (y > (edit->curs_row + 1))
	click = edit_move_forward (edit, click, y - (edit->curs_row + 1), 0);
    if (y < (edit->curs_row + 1))
	click = edit_move_backward (edit, click, (edit->curs_row + 1) - y);

/* (3) move right to x pos */
    click = edit_move_forward3 (edit, click, x - edit->start_col - 1, 0);
    return click;
}

void edit_translate_xy (int xs, int ys, int *x, int *y)
{
    *x = xs - EDIT_TEXT_HORIZONTAL_OFFSET;
    *y = (ys - EDIT_TEXT_VERTICAL_OFFSET - option_text_line_spacing / 2 - 1) / FONT_PIX_PER_LINE + 1;
}

extern int just_dropped_something;

static void mouse_redraw (WEdit * edit, long click)
{
    edit->force |= REDRAW_PAGE | REDRAW_LINE;
    edit_update_curs_row (edit);
    edit_update_curs_col (edit);
    edit->prev_col = edit_get_col (edit);
    edit_update_screen (edit);
    edit->search_start = click;
}

static void xy (int x, int y, int *x_return, int *y_return)
{
    edit_translate_xy (x, y, x_return, y_return);
}

static long cp (WEdit *edit, int x, int y)
{
    return edit_get_click_pos (edit, x, y);
}

/* return 1 if not marked */
static int marks (WEdit * edit, long *start, long *end)
{
    return eval_marks (edit, start, end);
}

int column_highlighting = 0;

static int erange (WEdit * edit, long start, long end, int click)
{
    if (column_highlighting) {
	int x;
	x = edit_move_forward3 (edit, edit_bol (edit, click), 0, click);
	if ((x >= edit->column1 && x < edit->column2)
	    || (x > edit->column2 && x <= edit->column1))
	    return (start <= click && click < end);
	else
	    return 0;
    }
    return (start <= click && click < end);
}

static void fin_mark (WEdit *edit)
{
    if (edit->mark2 < 0)
	edit_mark_cmd (edit, 0);
}

static void move_mark (WEdit *edit)
{
    edit_mark_cmd (edit, 1);
    edit_mark_cmd (edit, 0);
}

static void release_mark (WEdit *edit, XEvent *event)
{
    if (edit->mark2 < 0)
	edit_mark_cmd (edit, 0);
    else
	edit_mark_cmd (edit, 1);
    if (edit->mark1 != edit->mark2) {
	edit_get_selection (edit);
	XSetSelectionOwner (CDisplay, XA_PRIMARY, edit->widget->winid, event->xbutton.time);
    }
}

static char *get_block (WEdit * edit, long start_mark, long end_mark, int *type, int *l)
{
    char *t;
    t = (char *) edit_get_block (edit, start_mark, end_mark, l);
    if (strlen (t) < *l)
	*type = DndRawData;	/* if there are nulls in the data, send as raw */
    else
	*type = DndText;	/* else send as text */
    return t;
}

static void move (WEdit *edit, long click, int y)
{
    edit_cursor_move (edit, click - edit->curs1);
}

static void dclick (WEdit *edit, XEvent *event)
{
    edit_mark_cmd (edit, 1);
    edit_right_word_move (edit);
    edit_mark_cmd (edit, 0);
    edit_left_word_move (edit);
    release_mark (edit, event);
}

static void redraw (WEdit *edit, long click)
{
    mouse_redraw (edit, click);
}

static void edit_mouse_mark (WEdit * edit, XEvent * event, CEvent * ce)
{
    edit_update_curs_row (edit);
    edit_update_curs_col (edit);
    if (event->type != MotionNotify) {
	edit_push_action (edit, KEY_PRESS + edit->start_display);
	if (edit->mark2 == -1)
	    edit_push_action (edit, MARK_1 + edit->mark1);	/* mark1 must be following the cursor */
    }
    if (event->type == ButtonPress) {
	edit->highlight = 0;
	edit->found_len = 0;
    }
    mouse_mark (
	(void  *) edit,
	event,
	ce,
	(void (*) (int, int, int *, int *)) xy,
	(long (*) (void *, int, int)) cp,
	(int (*) (void *, long *, long *)) marks,
	(int (*) (void *, long, long, long)) erange,
	(void (*) (void *)) fin_mark,
	(void (*) (void *)) move_mark,
	(void (*) (void *, XEvent *)) release_mark,
	(char * (*) (void *, long, long, int *, int *)) get_block,
	(void (*) (void *, long, int)) move,
	0,
	(void (*) (void *, XEvent *)) dclick,
	(void (*) (void *, long)) redraw
    );
}

void link_scrollbar_to_editor (CWidget * scrollbar, CWidget * editor, XEvent * xevent, CEvent * cwevent, int whichscrbutton)
{
    int i, start_line;
    WEdit *e;
    e = editor->editor;
    if (!e)
	return;
    if (!e->widget->vert_scrollbar)
	return;
    start_line = e->start_line;
    if ((xevent->type == ButtonRelease || xevent->type == MotionNotify) && whichscrbutton == 3) {
	edit_move_display (e, (double) scrollbar->firstline * e->total_lines / 65535.0 + 1);
    } else if (xevent->type == ButtonPress && (cwevent->button == Button1 || cwevent->button == Button2)) {
	switch (whichscrbutton) {
	case 1:
	    edit_move_display (e, e->start_line - e->num_widget_lines + 1);
	    break;
	case 2:
	    edit_move_display (e, e->start_line - 1);
	    break;
	case 5:
	    edit_move_display (e, e->start_line + 1);
	    break;
	case 4:
	    edit_move_display (e, e->start_line + e->num_widget_lines - 1);
	    break;
	}
    }
    if (e->total_lines)
	scrollbar->firstline = (double) 65535.0 *e->start_line / (e->total_lines + 1);
    else
	scrollbar->firstline = 0;
    i = e->total_lines - e->start_line + 1;
    if (i > e->num_widget_lines)
	i = e->num_widget_lines;
    if (e->total_lines)
	scrollbar->numlines = (double) 65535.0 *i / (e->total_lines + 1);
    else
	scrollbar->numlines = 65535;
    if (start_line != e->start_line) {
	e->force |= REDRAW_PAGE | REDRAW_LINE;
	set_cursor_position (0, 0, 0, 0, 0, 0, 0, 0, 0);
	if (CCheckWindowEvent (xevent->xany.window, ButtonReleaseMask | ButtonMotionMask, 0))
	    return;
    }
    if (e->force) {
	edit_render_keypress (e);
	edit_status (e);
    }
}

void link_hscrollbar_to_editor (CWidget * scrollbar, CWidget * editor, XEvent * xevent, CEvent * cwevent, int whichscrbutton)
{
    int i, start_col;
    WEdit *e;
    e = editor->editor;
    if (!e)
	return;
    if (!e->widget->hori_scrollbar)
	return;
    start_col = (-e->start_col);
    if ((xevent->type == ButtonRelease || xevent->type == MotionNotify) && whichscrbutton == 3) {
	e->start_col = (double) scrollbar->firstline * e->max_column / 65535.0 + 1;
	e->start_col -= e->start_col % FONT_MEAN_WIDTH;
	if (e->start_col < 0)
	    e->start_col = 0;
	e->start_col = (-e->start_col);
    } else if (xevent->type == ButtonPress && (cwevent->button == Button1 || cwevent->button == Button2)) {
	switch (whichscrbutton) {
	case 1:
	    edit_scroll_left (e, (e->num_widget_columns - 1) * FONT_MEAN_WIDTH);
	    break;
	case 2:
	    edit_scroll_left (e, FONT_MEAN_WIDTH);
	    break;
	case 5:
	    edit_scroll_right (e, FONT_MEAN_WIDTH);
	    break;
	case 4:
	    edit_scroll_right (e, (e->num_widget_columns - 1) * FONT_MEAN_WIDTH);
	    break;
	}
    }
    scrollbar->firstline = (double) 65535.0 *(-e->start_col) / (e->max_column + 1);
    i = e->max_column - (-e->start_col) + 1;
    if (i > e->num_widget_columns * FONT_MEAN_WIDTH)
	i = e->num_widget_columns * FONT_MEAN_WIDTH;
    scrollbar->numlines = (double) 65535.0 *i / (e->max_column + 1);
    if (start_col != (-e->start_col)) {
	e->force |= REDRAW_PAGE | REDRAW_LINE;
	set_cursor_position (0, 0, 0, 0, 0, 0, 0, 0, 0);
	if (CCheckWindowEvent (xevent->xany.window, ButtonReleaseMask | ButtonMotionMask, 0))
	    return;
    }
    if (e->force) {
	edit_render_keypress (e);
	edit_status (e);
    }
}

/* 
   This section comes from rxvt-2.21b1/src/screen.c by
   Robert Nation <nation@rocket.sanders.lockheed.com> &
   mods by mj olesen <olesen@me.QueensU.CA>

   Changes made for cooledit
 */
void selection_send (XSelectionRequestEvent * rq)
{
    XEvent ev;
    static Atom xa_targets = None;
    if (xa_targets == None)
	xa_targets = XInternAtom (CDisplay, "TARGETS", False);

    ev.xselection.type = SelectionNotify;
    ev.xselection.property = None;
    ev.xselection.display = rq->display;
    ev.xselection.requestor = rq->requestor;
    ev.xselection.selection = rq->selection;
    ev.xselection.target = rq->target;
    ev.xselection.time = rq->time;

    if (rq->target == xa_targets) {
	/*
	 * On some systems, the Atom typedef is 64 bits wide.
	 * We need to have a typedef that is exactly 32 bits wide,
	 * because a format of 64 is not allowed by the X11 protocol.
	 */
	typedef CARD32 Atom32;

	Atom32 target_list[2];

	target_list[0] = (Atom32) xa_targets;
	target_list[1] = (Atom32) XA_STRING;

	XChangeProperty (CDisplay, rq->requestor, rq->property,
		xa_targets, 8 * sizeof (target_list[0]), PropModeReplace,
			 (unsigned char *) target_list,
			 sizeof (target_list) / sizeof (target_list[0]));
	ev.xselection.property = rq->property;
    } else if (rq->target == XA_STRING) {
	XChangeProperty (CDisplay, rq->requestor, rq->property,
			 XA_STRING, 8, PropModeReplace,
			 selection.text, selection.len);
	ev.xselection.property = rq->property;
    }
    XSendEvent (CDisplay, rq->requestor, False, 0, &ev);
}

/*{{{ paste selection */

/*
 * Respond to a notification that a primary selection has been sent
 */
void paste_prop (void *data, void (*insert) (void *, int), Window win, unsigned prop, int delete)
{
    long nread;
    unsigned long bytes_after;

    if (prop == None)
	return;

    nread = 0;
    do {
	unsigned char *s;
	Atom actual_type;
	int actual_fmt, i;
	unsigned long nitems;

	if (XGetWindowProperty (CDisplay, win, prop,
				nread / 4, 65536, delete,
			      AnyPropertyType, &actual_type, &actual_fmt,
				&nitems, &bytes_after,
				&s) != Success) {
	    XFree (s);
	    return;
	}
	nread += nitems;
	for (i = 0; i < nitems; i++)
	    (*insert) (data, s[i]);
	XFree (s);
    } while (bytes_after);
}

void selection_paste (WEdit * edit, Window win, unsigned prop, int delete)
{
    long c;
    c = edit->curs1;
    paste_prop ((void *) edit,
		(void (*)(void *, int)) edit_insert,
		win, prop, delete);
    edit_cursor_move (edit, c - edit->curs1);
    edit->force |= REDRAW_COMPLETELY | REDRAW_LINE;
}

/*}}} */

void selection_clear (void)
{
    selection.text = 0;
    selection.len = 0;
}

void edit_update_screen (WEdit * e)
{
    if (!e)
	return;
    if (!e->force)
	return;

    edit_scroll_screen_over_cursor (e);
    edit_update_curs_row (e);
    edit_update_curs_col (e);
    update_scroll_bars (e);
    edit_status (e);

    if (e->force & REDRAW_COMPLETELY)
	e->force |= REDRAW_PAGE;

/* pop all events for this window for internal handling */
    if (e->force & (REDRAW_CHAR_ONLY | REDRAW_COMPLETELY)) {
	edit_render_keypress (e);
    } else if (CCheckWindowEvent (e->widget->winid, ButtonPressMask | ButtonReleaseMask | ButtonMotionMask, 0)
	       || CKeyPending ()) {
	e->force |= REDRAW_PAGE;
	return;
    } else {
	edit_render_keypress (e);
    }
}

extern int space_width;

static void edit_insert_column_of_text (WEdit * edit, unsigned char *data, int size, int width)
{
    long cursor;
    int i, col;
    cursor = edit->curs1;
    col = edit_get_col (edit);
    for (i = 0; i < size; i++) {
	if (data[i] == '\n') {	/* fill in and move to next line */
	    int l;
	    long p;
	    if (edit_get_byte (edit, edit->curs1) != '\n') {
		l = width - (edit_get_col (edit) - col);
		while (l > 0) {
		    edit_insert (edit, ' ');
		    l -= space_width;
		}
	    }
	    for (p = edit->curs1;; p++) {
		if (p == edit->last_byte)
		    edit_insert_ahead (edit, '\n');
		if (edit_get_byte (edit, p) == '\n') {
		    p++;
		    break;
		}
	    }
	    edit_cursor_move (edit, edit_move_forward3 (edit, p, col, 0) - edit->curs1);
	    l = col - edit_get_col (edit);
	    while (l >= space_width) {
		edit_insert (edit, ' ');
		l -= space_width;
	    }
	    continue;
	}
	edit_insert (edit, data[i]);
    }
    edit_cursor_move (edit, cursor - edit->curs1);
}

#define free_data if (data) {free(data);data=0;}

/* handles drag and drop */
void handle_client_message (CWidget * w, XEvent * xevent, CEvent * cwevent)
{
    int data_type;
    unsigned char *data;
    unsigned long size;
    int xs, ys;
    long start_line;
    int x, y, r, deleted = 0;
    long click;
    unsigned int state;
    long start_mark = 0, end_mark = 0;
    WEdit *e = w->editor;

/* see just below for a comment on what this is for: */
    if (CIsDropAcknowledge (xevent, &state) != DndNotDnd) {
	if (!(state & Button1Mask) && just_dropped_something) {
	    edit_push_action (e, KEY_PRESS + e->start_display);
	    edit_block_delete_cmd (e);
	}
	return;
    }
    data_type = CGetDrop (xevent, &data, &size, &xs, &ys);

    if (data_type == DndNotDnd || xs < 0 || ys < 0 || xs >= w->width || ys >= w->height) {
	free_data;
	return;
    }
    edit_translate_xy (xs, ys, &x, &y);
    click = edit_get_click_pos (e, x, y);

    r = eval_marks (e, &start_mark, &end_mark);
/* musn't be able to drop into a block, otherwise a single click will copy a block: */
    if (r)
	goto fine;
    if (start_mark > click || click >= end_mark)
	goto fine;
    if (column_highlighting) {
	if (!((x >= e->column1 && x < e->column2)
	      || (x > e->column2 && x <= e->column1)))
	    goto fine;
    }
    free_data;
    return;
  fine:
    edit_push_action (e, KEY_PRESS + e->start_display);

/* drops to the same window moving to the left: */
    start_line = e->start_line;
    if (xevent->xclient.data.l[2] == xevent->xclient.window && !(xevent->xclient.data.l[1] & Button1Mask))
	if ((column_highlighting && x < max (e->column1, e->column2)) || !column_highlighting) {
	    edit_block_delete_cmd (e);
	    deleted = 1;
	}
    edit_update_curs_row (e);
    edit_move_display (e, start_line);
    click = edit_get_click_pos (e, x, y);	/* click pos changes with edit_block_delete_cmd() */
    edit_cursor_move (e, click - e->curs1);
    if (data_type == DndFile) {
	edit_insert_file (e, (char *) data);
    } else if (data_type != DndFiles) {
	if (dnd_null_term_type (data_type)) {
	    int len;
	    len = strlen ((char *) data);
	    size = min (len, size);
	}
	if (column_highlighting) {
	    edit_insert_column_of_text (e, data, size, abs (e->column2 - e->column1));
	} else {
	    while (size--)
		edit_insert_ahead (e, data[size]);
	}
    } else {
	while (size--)
	    edit_insert_ahead (e, data[size] ? data[size] : '\n');
    }

/* drops to the same window moving to the right: */
    if (xevent->xclient.data.l[2] == xevent->xclient.window && !(xevent->xclient.data.l[1] & Button1Mask))
	if (column_highlighting && !deleted)
	    edit_block_delete_cmd (e);

/* The drop has now been successfully recieved. We can now send an acknowledge
   event back to the window that send the data. When this window recieves
   the acknowledge event, the app can decide whether or not to delete the data.
   This allows text to be safely moved betweem text windows without the
   risk of data being lost. In our case, drag with button1 is a copy
   drag, while drag with any other button is a move drag (i.e. the sending
   application must delete its selection after recieving an acknowledge
   event). We must not, however, send an acknowledge signal if a filelist
   (for example) was passed to us, since the sender might take this to
   mean that all those files can be deleted! The two types we can acknowledge
   are: */
    if (xevent->xclient.data.l[2] != xevent->xclient.window)	/* drops to the same window */
	if (data_type == DndText || data_type == DndRawData)
	    CDropAcknowledge (xevent);
    e->force |= REDRAW_COMPLETELY | REDRAW_LINE;
    free_data;
}

int eh_editor (CWidget * w, XEvent * xevent, CEvent * cwevent)
{
    WEdit *e = w->editor;
    int r = 0;
    static int old_tab_spacing = -1;

    if (!e)
	return 0;

    if (old_tab_spacing != option_tab_spacing)
	e->force |= REDRAW_COMPLETELY + REDRAW_LINE;
    old_tab_spacing = option_tab_spacing;

    if (xevent->type == KeyPress) {
	if (xevent->xkey.keycode == 0x31 && xevent->xkey.state == 0xD) {
	    CSetColor (color_palette (18));
	    CRectangle (w->winid, 0, 0, w->width, w->height);
	}
    }
    switch (xevent->type) {
    case SelectionNotify:
	selection_paste (e, xevent->xselection.requestor, xevent->xselection.property, True);
	r = 1;
	break;
    case SelectionRequest:
	selection_send (&(xevent->xselectionrequest));
	return 1;
/*  case SelectionClear:   ---> This is handled by coolnext.c: CNextEvent() */
    case ClientMessage:
	handle_client_message (w, xevent, cwevent);
	r = 1;
	break;
    case ButtonPress:
	CFocus (w);
	edit_render_tidbits (w);
    case ButtonRelease:
	if (xevent->xbutton.state & ControlMask)
	    column_highlighting = 1;
	else
	    column_highlighting = 0;
    case MotionNotify:
	if (!xevent->xmotion.state && xevent->type == MotionNotify)
	    return 0;
	resolve_button (xevent, cwevent);
	edit_mouse_mark (e, xevent, cwevent);
	break;
    case Expose:
	edit_render_expose (e, &(xevent->xexpose));
	return 1;
    case FocusIn:
	CSetCursorColor (e->overwrite ? color_palette (24) : color_palette (19));
    case FocusOut:
	edit_render_tidbits (w);
	e->force |= REDRAW_CHAR_ONLY | REDRAW_LINE;
	edit_render_keypress (e);
	return 1;
	break;
    case KeyRelease:
	if (column_highlighting) {
	    column_highlighting = 0;
	    e->force = REDRAW_COMPLETELY | REDRAW_LINE;
	    edit_mark_cmd (e, 1);
	}
	break;
    case KeyPress:
	cwevent->ident = w->ident;
	if (!cwevent->command && cwevent->insert < 0) {		/* no translation */
	    if ((cwevent->key == XK_r || cwevent->key == XK_R) && (cwevent->state & ControlMask)) {
		cwevent->command = e->macro_i < 0 ? CK_Begin_Record_Macro : CK_End_Record_Macro;
	    } else {
		cwevent->command = CKeySymMod (xevent);
		if (cwevent->command > 0)
		    cwevent->command = CK_Macro (cwevent->command);
		else
		    break;
	    }
	}
	r = edit_execute_key_command (e, cwevent->command, cwevent->insert);
	if (r)
	    edit_update_screen (e);
	return r;
	break;
    case EditorCommand:
	cwevent->ident = w->ident;
	cwevent->command = xevent->xkey.keycode;
	r = cwevent->handled = edit_execute_key_command (e, xevent->xkey.keycode, -1);
	break;
    default:
	return 0;
    }
    edit_update_screen (e);
    return r;
}

#else

WEdit *wedit;
WButtonBar *edit_bar;
Dlg_head *edit_dlg;
WMenu *edit_menubar;

int column_highlighting = 0;

static int edit_callback (Dlg_head * h, WEdit * edit, int msg, int par);

static int edit_mode_callback (struct Dlg_head *h, int id, int msg)
{
    return 0;
}

int edit_event (WEdit * edit, Gpm_Event * event, int *result)
{
    *result = MOU_NORMAL;
    edit_update_curs_row (edit);
    edit_update_curs_col (edit);
    if (event->type & (GPM_DOWN | GPM_DRAG | GPM_UP)) {
	if (event->y > 1 && event->x > 0
	    && event->x <= edit->num_widget_columns
	    && event->y <= edit->num_widget_lines + 1) {
	    if (edit->mark2 != -1 && event->type & (GPM_UP | GPM_DRAG))
		return 1;	/* a lone up mustn't do anything */
	    if (event->type & (GPM_DOWN | GPM_UP))
		edit_push_key_press (edit);
	    edit_cursor_move (edit, edit_bol (edit, edit->curs1) - edit->curs1);
	    if (--event->y > (edit->curs_row + 1))
		edit_cursor_move (edit,
				  edit_move_forward (edit, edit->curs1, event->y - (edit->curs_row + 1), 0)
				  - edit->curs1);
	    if (event->y < (edit->curs_row + 1))
		edit_cursor_move (edit,
				  +edit_move_backward (edit, edit->curs1, (edit->curs_row + 1) - event->y)
				  - edit->curs1);
	    edit_cursor_move (edit, (int) edit_move_forward3 (edit, edit->curs1,
		       event->x - edit->start_col - 1, 0) - edit->curs1);
	    edit->prev_col = edit_get_col (edit);
	    if (event->type & GPM_DOWN) {
		edit_mark_cmd (edit, 1);	/* reset */
		edit->highlight = 0;
	    }
	    if (!(event->type & GPM_DRAG))
		edit_mark_cmd (edit, 0);
	    edit->force |= REDRAW_COMPLETELY;
	    edit_update_curs_row (edit);
	    edit_update_curs_col (edit);
	    edit_update_screen (edit);
	    return 1;
	}
    }
    return 0;
}



int menubar_event (Gpm_Event * event, WMenu * menubar);		/* menu.c */

int edit_mouse_event (Gpm_Event * event, void *x)
{
    int result;
    if (edit_event ((WEdit *) x, event, &result))
	return result;
    else
	return menubar_event (event, edit_menubar);
}

extern Menu EditMenuBar[5];

int edit (const char *_file, int line)
{
    static int made_directory = 0;
    int framed = 0;
    int midnight_colors[4];
    char *text = 0;

    if (option_backup_ext_int != -1) {
	option_backup_ext = malloc (sizeof(int) + 1);
	option_backup_ext[sizeof(int)] = '\0';
	memcpy (option_backup_ext, (char *) &option_backup_ext_int, sizeof (int));
    }

    if (!made_directory) {
	mkdir (catstrs (home_dir, EDIT_DIR, 0), 0700);
	made_directory = 1;
    }
    if (_file) {
	if (!(*_file)) {
	    _file = 0;
	    text = "";
	}
    } else
	text = "";

    if (!(wedit = edit_init (NULL, LINES - 2, COLS, _file, text, "", 0))) {
	message (1, _(" Error "), get_error_msg (""));
	return 0;
    }
    wedit->macro_i = -1;

    /* Create a new dialog and add it widgets to it */
    edit_dlg = create_dlg (0, 0, LINES, COLS, midnight_colors,
			   edit_mode_callback, "[Internal File Editor]",
			   "edit",
			   DLG_NONE);

    edit_dlg->raw = 1;		/*so that tab = '\t' key works */

    init_widget (&(wedit->widget), 0, 0, LINES - 1, COLS,
		 (callback_fn) edit_callback,
		 (destroy_fn) edit_clean,
		 (mouse_h) edit_mouse_event, 0);

    widget_want_cursor (wedit->widget, 1);

    edit_bar = buttonbar_new (1);

    if (!framed) {
	switch (edit_key_emulation) {
	case EDIT_KEY_EMULATION_NORMAL:
	    edit_init_menu_normal ();	/* editmenu.c */
	    break;
	case EDIT_KEY_EMULATION_EMACS:
	    edit_init_menu_emacs ();	/* editmenu.c */
	    break;
	}
	edit_menubar = menubar_new (0, 0, COLS, EditMenuBar, N_menus);
    }
    add_widget (edit_dlg, wedit);

    if (!framed)
	add_widget (edit_dlg, edit_menubar);

    add_widget (edit_dlg, edit_bar);
    edit_move_display (wedit, line - 1);
    edit_move_to_line (wedit, line - 1);

    run_dlg (edit_dlg);

    if (!framed)
	edit_done_menu ();	/* editmenu.c */

    destroy_dlg (edit_dlg);

    return 1;
}

static void edit_my_define (Dlg_head * h, int idx, char *text,
			    void (*fn) (WEdit *), WEdit * edit)
{
    define_label_data (h, (Widget *) edit, idx, text, (buttonbarfn) fn, edit);
}


void cmd_F1 (WEdit * edit)
{
    send_message (edit->widget.parent, (Widget *) edit, WIDGET_KEY, KEY_F (1));
}

void cmd_F2 (WEdit * edit)
{
    send_message (edit->widget.parent, (Widget *) edit, WIDGET_KEY, KEY_F (2));
}

void cmd_F3 (WEdit * edit)
{
    send_message (edit->widget.parent, (Widget *) edit, WIDGET_KEY, KEY_F (3));
}

void cmd_F4 (WEdit * edit)
{
    send_message (edit->widget.parent, (Widget *) edit, WIDGET_KEY, KEY_F (4));
}

void cmd_F5 (WEdit * edit)
{
    send_message (edit->widget.parent, (Widget *) edit, WIDGET_KEY, KEY_F (5));
}

void cmd_F6 (WEdit * edit)
{
    send_message (edit->widget.parent, (Widget *) edit, WIDGET_KEY, KEY_F (6));
}

void cmd_F7 (WEdit * edit)
{
    send_message (edit->widget.parent, (Widget *) edit, WIDGET_KEY, KEY_F (7));
}

void cmd_F8 (WEdit * edit)
{
    send_message (edit->widget.parent, (Widget *) edit, WIDGET_KEY, KEY_F (8));
}

void cmd_F9 (WEdit * edit)
{
    send_message (edit->widget.parent, (Widget *) edit, WIDGET_KEY, KEY_F (9));
}

void cmd_F10 (WEdit * edit)
{
    send_message (edit->widget.parent, (Widget *) edit, WIDGET_KEY, KEY_F (10));
}

void edit_labels (WEdit * edit)
{
    Dlg_head *h = edit->widget.parent;

    edit_my_define (h, 1, _("Help"), cmd_F1, edit);
    edit_my_define (h, 2, _("Save"), cmd_F2, edit);
    edit_my_define (h, 3, _("Mark"), cmd_F3, edit);
    edit_my_define (h, 4, _("Replac"), cmd_F4, edit);
    edit_my_define (h, 5, _("Copy"), cmd_F5, edit);
    edit_my_define (h, 6, _("Move"), cmd_F6, edit);
    edit_my_define (h, 7, _("Search"), cmd_F7, edit);
    edit_my_define (h, 8, _("Delete"), cmd_F8, edit);
    if (!edit->have_frame)
	edit_my_define (h, 9, _("PullDn"), edit_menu_cmd, edit);
    edit_my_define (h, 10, _("Quit"), cmd_F10, edit);

    redraw_labels (h, (Widget *) edit);
}


long get_key_state ()
{
    return (long) get_modifier ();
}

void edit_adjust_size (Dlg_head * h)
{
    WEdit *edit;
    WButtonBar *edit_bar;

    edit = (WEdit *) find_widget_type (h, (callback_fn) edit_callback);
    edit_bar = (WButtonBar *) edit->widget.parent->current->next->widget;
    widget_set_size (&edit->widget, 0, 0, LINES - 1, COLS);
    widget_set_size (&edit_bar->widget, LINES - 1, 0, 1, COLS);
    widget_set_size (&edit_menubar->widget, 0, 0, 1, COLS);

#ifdef RESIZABLE_MENUBAR
	menubar_arrange(edit_menubar);
#endif
}

void edit_update_screen (WEdit * e)
{
    edit_scroll_screen_over_cursor (e);

    edit_update_curs_col (e);
    edit_status (e);

/* pop all events for this window for internal handling */

    if (!is_idle ()) {
	e->force |= REDRAW_PAGE;
	return;
    }
    if (e->force & REDRAW_COMPLETELY)
	e->force |= REDRAW_PAGE;
    edit_render_keypress (e);
}

static int edit_callback (Dlg_head * h, WEdit * e, int msg, int par)
{
    switch (msg) {
    case WIDGET_INIT:
	e->force |= REDRAW_COMPLETELY;
	edit_labels (e);
	break;
    case WIDGET_DRAW:
	e->force |= REDRAW_COMPLETELY;
	e->num_widget_lines = LINES - 2;
	e->num_widget_columns = COLS;
    case WIDGET_FOCUS:
	edit_update_screen (e);
	return 1;
    case WIDGET_KEY:{
	    int cmd, ch;
	    if (edit_drop_hotkey_menu (e, par))		/* first check alt-f, alt-e, alt-s, etc for drop menus */
		return 1;
	    if (!edit_translate_key (e, 0, par, get_key_state (), &cmd, &ch))
		return 0;
	    edit_execute_key_command (e, cmd, ch);
	    edit_update_screen (e);
	}
	return 1;
    case WIDGET_COMMAND:
	edit_execute_key_command (e, par, -1);
	edit_update_screen (e);
	return 1;
    case WIDGET_CURSOR:
	widget_move (&e->widget, e->curs_row + EDIT_TEXT_VERTICAL_OFFSET, e->curs_col + e->start_col);
	return 1;
    }
    return default_proc (h, msg, par);
}

#endif

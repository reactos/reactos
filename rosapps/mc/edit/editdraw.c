/* editor text drawing.

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

#define MAX_LINE_LEN 1024

#ifndef MIDNIGHT
#include "app_glob.c"
#include "coollocal.h"
#else
#include "../src/mad.h"
#endif


static void status_string (WEdit * edit, char *s, int w, int fill, int font_width)
{
    int i;
    char t[160];		/* 160 just to be sure */
/* The field lengths just prevents the status line from shortening to much */
    sprintf (t, "[%c%c%c%c] %2ld:%3ld+%2ld=%3ld/%3ld - *%-4ld/%4ldb=%3d",
	     edit->mark1 != edit->mark2 ? 'B' : '-',
	     edit->modified ? 'M' : '-', edit->macro_i < 0 ? '-' : 'R',
	     edit->overwrite == 0 ? '-' : 'O',
	     edit->curs_col / font_width, edit->start_line + 1, edit->curs_row,
	     edit->curs_line + 1, edit->total_lines + 1, edit->curs1,
	     edit->last_byte, edit->curs1 < edit->last_byte
	     ? edit_get_byte (edit, edit->curs1) : -1);
    sprintf (s, "%.*s", w + 1, t);
    i = strlen (s);
    s[i] = ' ';
    i = w;
    do {
	if (strchr (" +-*=/:b", s[i]))	/* chop off the last word/number */
	    break;
	s[i] = fill;
    } while (i--);
    s[i] = fill;
    s[w] = 0;
}


#ifdef MIDNIGHT

/* how to get as much onto the status line as is numerically possible :) */
void edit_status (WEdit * edit)
{
    int w, i, t;
    char *s;
    w = edit->widget.cols - (edit->have_frame * 2);
    s = malloc (w + 15);
    if (w < 4)
	w = 4;
    memset (s, ' ', w);
    attrset (SELECTED_COLOR);
    if (w > 4) {
	widget_move (edit, edit->have_frame, edit->have_frame);
	i = w > 24 ? 18 : w - 6;
	i = i < 13 ? 13 : i;
	sprintf (s, "%s", name_trunc (edit->filename ? edit->filename : "", i));
	i += strlen (s);
	s[strlen (s)] = ' ';
	t = w - 20;
	if (t < 0)
	    t = 0;
	status_string (edit, s + 20, t, ' ', 1);
    } else {
	s[w] = 0;
    }
    printw ("%.*s", w, s);

    attrset (NORMAL_COLOR);
    free (s);
}

#else

extern int fixed_font;

void rerender_text (CWidget * wdt);
    
void edit_status (WEdit * edit)
{
    if ((edit->widget->options & EDITOR_NO_TEXT)) {
	return;
    } else {
	int w, i, t;
	CWidget *wdt;
	char id[33];
	char s[160];
	w = edit->num_widget_columns - 1;
	if (w > 150)
	    w = 150;
	if (w < 0)
	    w = 0;
	memset (s, 0, w);
	if (w > 1) {
	    i = w > 24 ? 18 : w - 6;
	    i = i < 13 ? 13 : i;
	    sprintf (s, "%s", name_trunc (edit->filename ? edit->filename : "", i));
	    i = strlen (s);
	    s[i] = ' ';
	    s[i+1] = ' ';
	    t = w - i - 2;
	    if (t < 0)
		t = 0;
	    status_string (edit, s + i + 2, t, 0, FONT_MEAN_WIDTH);
	}
	s[w] = 0;
	strcpy (id, edit->widget->ident);
	strcat (id, ".text");
	wdt = CIdent (id);
	free (wdt->text);
	wdt->text = strdup (s);
	CSetWidgetSize (id, edit->widget->width, wdt->height);
	rerender_text (wdt);
    }
}


#endif


/* boolean */
int cursor_in_screen (WEdit * edit, long row)
{
    if (row < 0 || row >= edit->num_widget_lines)
	return 0;
    else
	return 1;
}

/* returns rows from the first displayed line to the cursor */
int cursor_from_display_top (WEdit * edit)
{
    if (edit->curs1 < edit->start_display)
	return -edit_move_forward (edit, edit->curs1, 0, edit->start_display);
    else
	return edit_move_forward (edit, edit->start_display, 0, edit->curs1);
}

/* returns how far the cursor is out of the screen */
int cursor_out_of_screen (WEdit * edit)
{
    int row = cursor_from_display_top (edit);
    if (row >= edit->num_widget_lines)
	return row - edit->num_widget_lines + 1;
    if (row < 0)
	return row;
    return 0;
}

#ifndef MIDNIGHT
extern unsigned char per_char[256];
int edit_width_of_long_printable (int c);
#endif

/* this scrolls the text so that cursor is on the screen */
void edit_scroll_screen_over_cursor (WEdit * edit)
{
    int p, l;
    int outby;
    p = edit_get_col (edit);
    edit_update_curs_row (edit);
#ifdef MIDNIGHT
    outby = p + edit->start_col - edit->num_widget_columns + 1 + (EDIT_RIGHT_EXTREME + edit->found_len);
#else
    outby = p + edit->start_col - edit->widget->width + 7 + (EDIT_RIGHT_EXTREME + edit->found_len) * FONT_MEAN_WIDTH + edit_width_of_long_printable (edit_get_byte (edit, edit->curs1));
#endif
    if (outby > 0)
	edit_scroll_right (edit, outby);
#ifdef MIDNIGHT
    outby = EDIT_LEFT_EXTREME - p - edit->start_col;
#else
    outby = EDIT_LEFT_EXTREME * FONT_MEAN_WIDTH - p - edit->start_col;
#endif
    if (outby > 0)
	edit_scroll_left (edit, outby);
    p = edit->curs_row;
    l = 0;
    if (edit->found_len != 0)
	l = edit->num_widget_lines / 5;
    outby = p - edit->num_widget_lines + 1 + EDIT_BOTTOM_EXTREME + l;
    if (outby > 0)
	edit_scroll_downward (edit, outby);
    outby = EDIT_TOP_EXTREME - p + l;
    if (outby > 0)
	edit_scroll_upward (edit, outby);
    edit_update_curs_row (edit);
}


#ifndef MIDNIGHT

#define CACHE_WIDTH 256
#define CACHE_HEIGHT 128

int EditExposeRedraw = 0;
int EditClear = 0;

/* background colors: marked is refers to mouse highlighting, highlighted refers to a found string. */
unsigned long edit_abnormal_color, edit_marked_abnormal_color;
unsigned long edit_highlighted_color, edit_marked_color;
unsigned long edit_normal_background_color;

/* foreground colors */
unsigned long edit_normal_foreground_color, edit_bold_color;
unsigned long edit_italic_color;

/* cursor color */
unsigned long edit_cursor_color;

void edit_set_foreground_colors (unsigned long normal, unsigned long bold, unsigned long italic)
{
    edit_normal_foreground_color = normal;
    edit_bold_color = bold;
    edit_italic_color = italic;
}

void edit_set_background_colors (unsigned long normal, unsigned long abnormal, unsigned long marked, unsigned long marked_abnormal, unsigned long highlighted)
{
    edit_abnormal_color = abnormal;
    edit_marked_abnormal_color = marked_abnormal;
    edit_marked_color = marked;
    edit_highlighted_color = highlighted;
    edit_normal_background_color = normal;
}

void edit_set_cursor_color (unsigned long c)
{
    edit_cursor_color = c;
}

#else

#define BOLD_COLOR        MARKED_COLOR
#define UNDERLINE_COLOR   VIEW_UNDERLINED_COLOR
#define MARK_COLOR        SELECTED_COLOR
#define DEF_COLOR         NORMAL_COLOR

static void set_color (int font)
{
    attrset (font);
}

#define edit_move(x,y) widget_move(edit, y, x);

static void print_to_widget (WEdit * edit, long row, int start_col, float start_col_real, long end_col, unsigned int line[])
{
    int x = (float) start_col_real + EDIT_TEXT_HORIZONTAL_OFFSET;
    int x1 = start_col + EDIT_TEXT_HORIZONTAL_OFFSET;
    int y = row + EDIT_TEXT_VERTICAL_OFFSET;

    set_color (DEF_COLOR);
    edit_move (x1, y);
    hline (' ', end_col + 1 - EDIT_TEXT_HORIZONTAL_OFFSET - x1);

    edit_move (x + FONT_OFFSET_X, y + FONT_OFFSET_Y);
    {
	unsigned int *p = line;
	int textchar = ' ';
	long style;

	while (*p) {
	    style = *p >> 8;
	    textchar = *p & 0xFF;
#ifdef HAVE_SYNTAXH
	    if (!(style & (0xFF - MOD_ABNORMAL - MOD_CURSOR)))
		SLsmg_set_color ((*p & 0x007F0000) >> 16);
#endif
	    if (style & MOD_ABNORMAL)
		textchar = '.';
	    if (style & MOD_HIGHLIGHTED) {
		set_color (BOLD_COLOR);
	    } else if (style & MOD_MARKED) {
		set_color (MARK_COLOR);
	    }
	    if (style & MOD_UNDERLINED) {
		set_color (UNDERLINE_COLOR);
	    }
	    if (style & MOD_BOLD) {
		set_color (BOLD_COLOR);
	    }
	    addch (textchar);
	    p++;
	}
    }
}

/* b pointer to begining of line */
static void edit_draw_this_line (WEdit * edit, long b, long row, long start_col, long end_col)
{
    static unsigned int line[MAX_LINE_LEN];
    unsigned int *p = line;
    long m1 = 0, m2 = 0, q;
    int col, start_col_real;
    unsigned int c;
    int fg, bg;
    int i;

    edit_get_syntax_color (edit, b - 1, &fg, &bg);
    q = edit_move_forward3 (edit, b, start_col - edit->start_col, 0);
    start_col_real = (col = (int) edit_move_forward3 (edit, b, 0, q)) + edit->start_col;

    if (col + 16 > -edit->start_col) {
	eval_marks (edit, &m1, &m2);

	if (row <= edit->total_lines - edit->start_line) {
	    while (col <= end_col - edit->start_col) {
		*p = 0;
		if (q == edit->curs1)
		    *p |= MOD_CURSOR * 256;
		if (q >= m1 && q < m2)
		    *p |= MOD_MARKED * 256;
		if (q == edit->bracket)
		    *p |= MOD_BOLD * 256;
		if (q >= edit->found_start && q < edit->found_start + edit->found_len)
		    *p |= MOD_HIGHLIGHTED * 256;
		c = edit_get_byte (edit, q);
		edit_get_syntax_color (edit, q, &fg, &bg);
/* we don't use bg for mc - fg contains both */
		*p |= fg << 16;
		q++;
		switch (c) {
		case '\n':
		    col = end_col - edit->start_col + 1;	/* quit */
		    *(p++) |= ' ';
		    break;
		case '\t':
		    i = TAB_SIZE - ((int) col % TAB_SIZE);
		    *p |= ' ';
		    c = *(p++) & (0xFFFFFFFF - MOD_CURSOR * 256);
		    col += i;
		    while (--i)
			*(p++) = c;
		    break;
		case '\r':
		    break;
		default:
		    if (is_printable (c)) {
			*(p++) |= c;
		    } else {
			*(p++) = '.';
			*p |= (256 * MOD_ABNORMAL);
		    }
		    col++;
		    break;
		}
	    }
	}
    } else {
	start_col_real = start_col = 0;
    }
    *p = 0;

    print_to_widget (edit, row, start_col, start_col_real, end_col, line);
}

#endif

#ifdef MIDNIGHT

#define key_pending(x) (!is_idle())

#else

int edit_mouse_pending (Window win);
#define edit_draw_this_line edit_draw_this_line_proportional

static int key_pending (WEdit * edit)
{
    if (!(edit->force & REDRAW_COMPLETELY) && !EditExposeRedraw)
	return CKeyPending ();
    return 0;
}

#endif


/* b for pointer to begining of line */
static void edit_draw_this_char (WEdit * edit, long curs, long row)
{
    int b = edit_bol (edit, curs);
#ifdef MIDNIGHT
    edit_draw_this_line (edit, b, row, 0, edit->num_widget_columns - 1);
#else
    edit_draw_this_line (edit, b, row, 0, edit->widget->width);
#endif
}

/* cursor must be in screen for other than REDRAW_PAGE passed in force */
void render_edit_text (WEdit * edit, long start_row, long start_column, long end_row, long end_column)
{
    long row = 0, curs_row;
    static int prev_curs_row = 0;
    static long prev_start_display = 0;
    static int prev_start_col = 0;
    static long prev_curs = 0;

#ifndef MIDNIGHT
    static Window prev_win = 0;
#endif
    int fg, bg;

    int force = edit->force;
    long b;

/*
   if the position of the page has not moved then we can draw the cursor character only.
   This will prevent line flicker when using arrow keys.
 */
    if ((!(force & REDRAW_CHAR_ONLY)) || (force & REDRAW_PAGE)
#ifndef MIDNIGHT
	|| prev_win != edit->widget->winid
#endif
	) {
	if (!(force & REDRAW_IN_BOUNDS)) {	/* !REDRAW_IN_BOUNDS means to ignore bounds and redraw whole rows */
	    start_row = 0;
	    end_row = edit->num_widget_lines - 1;
	    start_column = 0;
#ifdef MIDNIGHT
	    end_column = edit->num_widget_columns - 1;
#else
	    end_column = edit->widget->width;
#endif
	}
	if (force & REDRAW_PAGE) {
	    row = start_row;
	    b = edit_move_forward (edit, edit->start_display, start_row, 0);
	    while (row <= end_row) {
		if (key_pending (edit))
		    goto exit_render;
		edit_draw_this_line (edit, b, row, start_column, end_column);
		b = edit_move_forward (edit, b, 1, 0);
		row++;
	    }
	} else {
	    curs_row = edit->curs_row;

	    if (force & REDRAW_BEFORE_CURSOR) {
		if (start_row < curs_row) {
		    long upto = curs_row - 1 <= end_row ? curs_row - 1 : end_row;
		    row = start_row;
		    b = edit->start_display;
		    while (row <= upto) {
			if (key_pending (edit))
			    goto exit_render;
			edit_draw_this_line (edit, b, row, start_column, end_column);
			b = edit_move_forward (edit, b, 1, 0);
		    }
		}
	    }
/*          if (force & REDRAW_LINE) {          ---> default */
	    b = edit_bol (edit, edit->curs1);
	    if (curs_row >= start_row && curs_row <= end_row) {
		if (key_pending (edit))
		    goto exit_render;
		edit_draw_this_line (edit, b, curs_row, start_column, end_column);
	    }
	    if (force & REDRAW_AFTER_CURSOR) {
		if (end_row > curs_row) {
		    row = curs_row + 1 < start_row ? start_row : curs_row + 1;
		    b = edit_move_forward (edit, b, 1, 0);
		    while (row <= end_row) {
			if (key_pending (edit))
			    goto exit_render;
			edit_draw_this_line (edit, b, row, start_column, end_column);
			b = edit_move_forward (edit, b, 1, 0);
			row++;
		    }
		}
	    }
	    if (force & REDRAW_LINE_ABOVE && curs_row >= 1) {
		row = curs_row - 1;
		b = edit_move_backward (edit, edit_bol (edit, edit->curs1), 1);
		if (row >= start_row && row <= end_row) {
		    if (key_pending (edit))
			goto exit_render;
		    edit_draw_this_line (edit, b, row, start_column, end_column);
		}
	    }
	    if (force & REDRAW_LINE_BELOW && row < edit->num_widget_lines - 1) {
		row = curs_row + 1;
		b = edit_bol (edit, edit->curs1);
		b = edit_move_forward (edit, b, 1, 0);
		if (row >= start_row && row <= end_row) {
		    if (key_pending (edit))
			goto exit_render;
		    edit_draw_this_line (edit, b, row, start_column, end_column);
		}
	    }
	}
    } else {
	if (prev_curs_row < edit->curs_row) {	/* with the new text highlighting, we must draw from the top down */
	    edit_draw_this_char (edit, prev_curs, prev_curs_row);
	    edit_draw_this_char (edit, edit->curs1, edit->curs_row);
	} else {
	    edit_draw_this_char (edit, edit->curs1, edit->curs_row);
	    edit_draw_this_char (edit, prev_curs, prev_curs_row);
	}
    }

    edit->force = 0;

    prev_curs_row = edit->curs_row;
    prev_curs = edit->curs1;
    prev_start_display = edit->start_display;
    prev_start_col = edit->start_col;
#ifndef MIDNIGHT
    prev_win = edit->widget->winid;
#endif
  exit_render:
    edit_get_syntax_color (edit, edit->start_display - 1, &fg, &bg);
}



#ifndef MIDNIGHT

void edit_convert_expose_to_area (XExposeEvent * xexpose, int *row1, int *col1, int *row2, int *col2)
{
    *col1 = xexpose->x - EDIT_TEXT_HORIZONTAL_OFFSET;
    *row1 = (xexpose->y - EDIT_TEXT_VERTICAL_OFFSET) / FONT_PIX_PER_LINE;
    *col2 = xexpose->x + xexpose->width + EDIT_TEXT_HORIZONTAL_OFFSET + 3;
    *row2 = (xexpose->y + xexpose->height - EDIT_TEXT_VERTICAL_OFFSET) / FONT_PIX_PER_LINE;
}

void edit_render_tidbits (CWidget * wdt)
{
    int isfocussed;
    int w = wdt->width, h = wdt->height;
    Window win;

    win = wdt->winid;
    isfocussed = (win == CGetFocus ());

    CSetColor (COLOR_FLAT);

    if (isfocussed) {
	render_bevel (win, 0, 0, w - 1, h - 1, 3, 1);	/*most outer border bevel */
    } else {
	render_bevel (win, 2, 2, w - 3, h - 3, 1, 1);	/*border bevel */
	render_bevel (win, 0, 0, w - 1, h - 1, 2, 0);	/*most outer border bevel */
    }
}

void edit_set_space_width (int s);
extern int option_long_whitespace;

#endif

void edit_render (WEdit * edit, int page, int row_start, int col_start, int row_end, int col_end)
{
    int f = 0;
    if (page)			/* if it was an expose event, 'page' would be set */
	edit->force |= REDRAW_PAGE | REDRAW_IN_BOUNDS;
    f = edit->force & (REDRAW_PAGE | REDRAW_COMPLETELY);

#ifdef MIDNIGHT
    if (edit->force & REDRAW_COMPLETELY)
	redraw_labels (edit->widget.parent, (Widget *) edit);
#else
    if (option_long_whitespace)
	edit_set_space_width (per_char[' '] * 2);
    else
	edit_set_space_width (per_char[' ']);
    edit_set_foreground_colors (
				 color_palette (option_editor_fg_normal),
				   color_palette (option_editor_fg_bold),
				   color_palette (option_editor_fg_italic)
	);
    edit_set_background_colors (
				 color_palette (option_editor_bg_normal),
			       color_palette (option_editor_bg_abnormal),
				 color_palette (option_editor_bg_marked),
			color_palette (option_editor_bg_marked_abnormal),
			     color_palette (option_editor_bg_highlighted)
	);
    edit_set_cursor_color (
			      color_palette (option_editor_fg_cursor)
	);

    if (!EditExposeRedraw)
	set_cursor_position (0, 0, 0, 0, 0, 0, 0, 0, 0);
#endif

    render_edit_text (edit, row_start, col_start, row_end, col_end);
    if (edit->force)		/* edit->force != 0 means a key was pending and the redraw 
				   was halted, so next time we must redraw everything in case stuff
				   was left undrawn from a previous key press */
	edit->force |= REDRAW_PAGE;
#ifndef MIDNIGHT
    if (f) {
	edit_render_tidbits (edit->widget);
	CSetColor (edit_normal_background_color);
	CLine (edit->widget->winid, 3, 3, 3, edit->widget->height - 4);
    }
#endif
}

#ifndef MIDNIGHT
void edit_render_expose (WEdit * edit, XExposeEvent * xexpose)
{
    int row_start, col_start, row_end, col_end;
    EditExposeRedraw = 1;
    edit->num_widget_lines = (edit->widget->height - 6) / FONT_PIX_PER_LINE;
    edit->num_widget_columns = (edit->widget->width - 7) / FONT_MEAN_WIDTH;
    if (edit->force & (REDRAW_PAGE | REDRAW_COMPLETELY)) {
	edit->force |= REDRAW_PAGE | REDRAW_COMPLETELY;
	edit_render_keypress (edit);
    } else {
	edit_convert_expose_to_area (xexpose, &row_start, &col_start, &row_end, &col_end);
	edit_render (edit, 1, row_start, col_start, row_end, col_end);
    }
    EditExposeRedraw = 0;
}

void edit_render_keypress (WEdit * edit)
{
    edit_render (edit, 0, 0, 0, 0, 0);
}

#else

void edit_render_keypress (WEdit * edit)
{
    edit_render (edit, 0, 0, 0, 0, 0);
}

#endif

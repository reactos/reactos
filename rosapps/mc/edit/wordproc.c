/* wordproc.c - word-processor mode for the editor: does dynamic
		paragraph formatting.
   Copyright (C) 1996 Paul Sheer

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
#include "edit.h"

#ifdef MIDNIGHT
#define tab_width option_tab_spacing
#endif

int line_is_blank (WEdit * edit, long line);

#define NO_FORMAT_CHARS_START "-+*\\,.;:&>"

static long line_start (WEdit * edit, long line)
{
    static long p = -1, l = 0;
    int c;
    if (p == -1 || abs (l - line) > abs (edit->curs_line - line)) {
	l = edit->curs_line;
	p = edit->curs1;
    }
    if (line < l)
	p = edit_move_backward (edit, p, l - line);
    else if (line > l)
	p = edit_move_forward (edit, p, line - l, 0);
    l = line;
    p = edit_bol (edit, p);
    while (strchr ("\t ", c = edit_get_byte (edit, p)))
	p++;
    return p;
}

static int bad_line_start (WEdit * edit, long p)
{
    int c;
    c = edit_get_byte (edit, p);
    if (c == '.') {		/* `...' is acceptable */
	if (edit_get_byte (edit, p + 1) == '.')
	    if (edit_get_byte (edit, p + 2) == '.')
		return 0;
	return 1;
    }
    if (c == '-') {
	if (edit_get_byte (edit, p + 1) == '-')
	    if (edit_get_byte (edit, p + 2) == '-')
		return 0;	/* `---' is acceptable */
	return 1;
    }
    if (strchr (NO_FORMAT_CHARS_START, c))
	return 1;
    return 0;
}

static long begin_paragraph (WEdit * edit, long p, int force)
{
    int i;
    for (i = edit->curs_line - 1; i > 0; i--) {
	if (line_is_blank (edit, i)) {
	    i++;
	    break;
	}
	if (force) {
	    if (bad_line_start (edit, line_start (edit, i))) {
		i++;
		break;
	    }
	}
    }
    return edit_move_backward (edit, edit_bol (edit, edit->curs1), edit->curs_line - i);
}

static long end_paragraph (WEdit * edit, long p, int force)
{
    int i;
    for (i = edit->curs_line + 1; i < edit->total_lines; i++) {
	if (line_is_blank (edit, i)) {
	    i--;
	    break;
	}
	if (force)
	    if (bad_line_start (edit, line_start (edit, i))) {
		i--;
		break;
	    }
    }
    return edit_eol (edit, edit_move_forward (edit, edit_bol (edit, edit->curs1), i - edit->curs_line, 0));
}

static char *get_paragraph (WEdit * edit, long p, long q, int indent, int *size)
{
    char *s, *t;
    t = malloc ((q - p) + 2 * (q - p) / option_word_wrap_line_length + 10);
    if (!t)
	return 0;
    for (s = t; p < q; p++, s++) {
	if (indent)
	    if (edit_get_byte (edit, p - 1) == '\n')
		while (strchr ("\t ", edit_get_byte (edit, p)))
		    p++;
	*s = edit_get_byte (edit, p);
    }
    *size = (unsigned long) s - (unsigned long) t;
    t[*size] = '\n';
    return t;
}

static void strip_newlines (char *t, int size)
{
    char *p = t;
    while (size--) {
	*p = *p == '\n' ? ' ' : *p;
	p++;
    }
}

#ifndef MIDNIGHT
int edit_width_of_long_printable (int c);
#endif
/* 
   This is a copy of the function 
   int calc_text_pos (WEdit * edit, long b, long *q, int l)
   in propfont.c  :(
   It calculates the number of chars in a line specified to length l in pixels
 */
extern int tab_width;
static inline int next_tab_pos (int x)
{
    return x += tab_width - x % tab_width;
}
static int line_pixel_length (char *t, long b, int l)
{
    int x = 0, c, xn = 0;
    for (;;) {
	c = t[b];
	switch (c) {
	case '\n':
	    return b;
	case '\t':
	    xn = next_tab_pos (x);
	    break;
	default:
#ifdef MIDNIGHT
	    xn = x + 1;
#else
	    xn = x + edit_width_of_long_printable (c);
#endif
	    break;
	}
	if (xn > l)
	    break;
	x = xn;
	b++;
    }
    return b;
}

/* find the start of a word */
static int next_word_start (char *t, int q, int size)
{
    int i;
    for (i = q;; i++) {
	switch (t[i]) {
	case '\n':
	    return -1;
	case '\t':
	case ' ':
	    for (;; i++) {
		if (t[i] == '\n')
		    return -1;
		if (t[i] != ' ' && t[i] != '\t')
		    return i;
	    }
	    break;
	}
    }
}

/* find the start of a word */
static int word_start (char *t, int q, int size)
{
    int i = q;
    if (t[q] == ' ' || t[q] == '\t')
	return next_word_start (t, q, size);
    for (;;) {
	int c;
	if (!i)
	    return -1;
	c = t[i - 1];
	if (c == '\n')
	    return -1;
	if (c == ' ' || c == '\t')
	    return i;
	i--;
    }
}

/* replaces ' ' with '\n' to properly format a paragraph */
static void format_this (char *t, int size, int indent)
{
    int q = 0, ww;
    strip_newlines (t, size);
    ww = option_word_wrap_line_length * FONT_MEAN_WIDTH - indent;
    if (ww < FONT_MEAN_WIDTH * 2)
	ww = FONT_MEAN_WIDTH * 2;
    for (;;) {
	int p;
	q = line_pixel_length (t, q, ww);
	if (q > size)
	    break;
	if (t[q] == '\n')
	    break;
	p = word_start (t, q, size);
	if (p == -1)
	    q = next_word_start (t, q, size);	/* Return the end of the word if the beginning 
						   of the word is at the beginning of a line 
						   (i.e. a very long word) */
	else
	    q = p;
	if (q == -1)	/* end of paragraph */
	    break;
	if (q)
	    t[q - 1] = '\n';
    }
}

static void replace_at (WEdit * edit, long q, int c)
{
    edit_cursor_move (edit, q - edit->curs1);
    edit_delete (edit);
    edit_insert_ahead (edit, c);
}

void edit_insert_indent (WEdit * edit, int indent);

/* replaces a block of text */
static void put_paragraph (WEdit * edit, char *t, long p, long q, int indent, int size)
{
    long cursor;
    int i, c = 0;
    cursor = edit->curs1;
    if (indent)
	while (strchr ("\t ", edit_get_byte (edit, p)))
	    p++;
    for (i = 0; i < size; i++, p++) {
	if (i && indent) {
	    if (t[i - 1] == '\n' && c == '\n') {
		while (strchr ("\t ", edit_get_byte (edit, p)))
		    p++;
	    } else if (t[i - 1] == '\n') {
		long curs;
		edit_cursor_move (edit, p - edit->curs1);
		curs = edit->curs1;
		edit_insert_indent (edit, indent);
		if (cursor >= curs)
		    cursor += edit->curs1 - p;
		p = edit->curs1;
	    } else if (c == '\n') {
		edit_cursor_move (edit, p - edit->curs1);
		while (strchr ("\t ", edit_get_byte (edit, p))) {
		    edit_delete (edit);
		    if (cursor > edit->curs1)
			cursor--;
		}
		p = edit->curs1;
	    }
	}
	c = edit_get_byte (edit, p);
	if (c != t[i])
	    replace_at (edit, p, t[i]);
    }
    edit_cursor_move (edit, cursor - edit->curs1);	/* restore cursor position */
}

int edit_indent_width (WEdit * edit, long p);

static int test_indent (WEdit * edit, long p, long q)
{
    int indent;
    indent = edit_indent_width (edit, p++);
    if (!indent)
	return 0;
    for (; p < q; p++)
	if (edit_get_byte (edit, p - 1) == '\n')
	    if (indent != edit_indent_width (edit, p))
		return 0;
    return indent;
}

void format_paragraph (WEdit * edit, int force)
{
    long p, q;
    int size;
    char *t;
    int indent = 0;
    if (option_word_wrap_line_length < 2)
	return;
    if (line_is_blank (edit, edit->curs_line))
	return;
    p = begin_paragraph (edit, edit->curs1, force);
    q = end_paragraph (edit, edit->curs1, force);
    indent = test_indent (edit, p, q);
    t = get_paragraph (edit, p, q, indent, &size);
    if (!t)
	return;
    if (!force) {
	int i;
	if (strchr (NO_FORMAT_CHARS_START, *t)) {
	    free (t);
	    return;
	}
	for (i = 0; i < size - 1; i++) {
	    if (t[i] == '\n') {
		if (strchr (NO_FORMAT_CHARS_START "\t ", t[i + 1])) {
		    free (t);
		    return;
		}
	    }
	}
    }
    format_this (t, q - p, indent);
    put_paragraph (edit, t, p, q, indent, size);
    free (t);
}











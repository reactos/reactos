/* editor syntax highlighting.

   Copyright (C) 1996, 1997, 1998 the Free Software Foundation

   Authors: 1998 Paul Sheer

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
#ifdef MIDNIGHT
#include "edit.h"
#else
#include "coolwidget.h"
#endif

#if !defined(MIDNIGHT) || defined(HAVE_SYNTAXH)

int option_syntax_highlighting = 1;

/* these three functions are called from the outside */
void edit_load_syntax (WEdit * edit, char **names, char *type);
void edit_free_syntax_rules (WEdit * edit);
void edit_get_syntax_color (WEdit * edit, long byte_index, int *fg, int *bg);

static void *syntax_malloc (size_t x)
{
    void *p;
    p = malloc (x);
    memset (p, 0, x);
    return p;
}

#define syntax_free(x) {if(x){free(x);(x)=0;}}

static int compare_word_to_right (WEdit * edit, long i, char *text, char *whole_left, char *whole_right, int line_start)
{
    char *p;
    int c, j;
    if (!*text)
	return 0;
    c = edit_get_byte (edit, i - 1);
    if (line_start)
	if (c != '\n')
	    return 0;
    if (whole_left)
	if (strchr (whole_left, c))
	    return 0;
    for (p = text; *p; p++, i++) {
	switch (*p) {
	case '\001':
	    p++;
	    for (;;) {
		c = edit_get_byte (edit, i);
		if (c == *p)
		    break;
		if (c == '\n')
		    return 0;
		i++;
	    }
	    break;
	case '\002':
	    p++;
	    for (;;) {
		c = edit_get_byte (edit, i);
		if (c == *p)
		    break;
		if (c == '\n' || c == '\t' || c == ' ')
		    return 0;
		i++;
	    }
	    break;
	case '\003':
	    p++;
#if 0
	    c = edit_get_byte (edit, i++);
	    for (j = 0; p[j] != '\003'; j++)
		if (c == p[j])
		    goto found_char1;
	    return 0;
	  found_char1:
#endif
	    for (;;i++) {
		c = edit_get_byte (edit, i);
		for (j = 0; p[j] != '\003'; j++)
		    if (c == p[j])
			goto found_char2;
		break;
	      found_char2:;
	    }
	    i--;
	    while (*p != '\003')
		p++;
	    break;
#if 0
	case '\004':
	    p++;
	    c = edit_get_byte (edit, i++);
	    for (j = 0; p[j] != '\004'; j++)
		if (c == p[j])
		    return 0;
	    for (;;i++) {
		c = edit_get_byte (edit, i);
		for (j = 0; p[j] != '\004'; j++)
		    if (c == p[j])
			goto found_char4;
		continue;
	      found_char4:
		break;
	    }
	    i--;
	    while (*p != '\004')
		p++;
	    break;
#endif
	default:
	    if (*p != edit_get_byte (edit, i))
		return 0;
	}
    }
    if (whole_right)
	if (strchr (whole_right, edit_get_byte (edit, i)))
	    return 0;
    return 1;
}

static int compare_word_to_left (WEdit * edit, long i, char *text, char *whole_left, char *whole_right, int line_start)
{
    char *p;
    int c, j;
#if 0
    int d;
#endif
    if (!*text)
	return 0;
    if (whole_right)
	if (strchr (whole_right, edit_get_byte (edit, i + 1)))
	    return 0;
    for (p = text + strlen (text) - 1; (unsigned long) p >= (unsigned long) text; p--, i--) {
	switch (*p) {
	case '\001':
	    p--;
	    for (;;) {
		c = edit_get_byte (edit, i);
		if (c == *p)
		    break;
		if (c == '\n')
		    return 0;
		i--;
	    }
	    break;
	case '\002':
	    p--;
	    for (;;) {
		c = edit_get_byte (edit, i);
		if (c == *p)
		    break;
		if (c == '\n' || c == '\t' || c == ' ')
		    return 0;
		i--;
	    }
	    break;
	case '\003':
	    while (*(--p) != '\003');
	    p++;
#if 0
	    c = edit_get_byte (edit, i--);
	    for (j = 0; p[j] != '\003'; j++)
		if (c == p[j])
		    goto found_char1;
	    return 0;
	  found_char1:
#endif
	    for (;; i--) {
		c = edit_get_byte (edit, i);
		for (j = 0; p[j] != '\003'; j++)
		    if (c == p[j])
			goto found_char2;
		break;
	      found_char2:;
	    }
	    i++;
	    p--;
	    break;
#if 0
	case '\004':
	    while (*(--p) != '\004');
	    d = *p;
	    p++;
	    c = edit_get_byte (edit, i--);
	    for (j = 0; p[j] != '\004'; j++)
		if (c == p[j])
		    return 0;
	    for (;; i--) {
		c = edit_get_byte (edit, i);
		for (j = 0; p[j] != '\004'; j++)
		    if (c == p[j] || c == '\n' || c == d)
			goto found_char4;
		continue;
	      found_char4:
		break;
	    }
	    i++;
	    p--;
	    break;
#endif
	default:
	    if (*p != edit_get_byte (edit, i))
		return 0;
	}
    }
    c = edit_get_byte (edit, i);
    if (line_start)
	if (c != '\n')
	    return 0;
    if (whole_left)
	if (strchr (whole_left, c))
	    return 0;
    return 1;
}


#if 0
#define debug_printf(x,y) fprintf(stderr,x,y)
#else
#define debug_printf(x,y)
#endif

static inline unsigned long apply_rules_going_right (WEdit * edit, long i, unsigned long rule)
{
    struct context_rule *r;
    int context, contextchanged = 0, keyword, c1, c2;
    int found_right = 0, found_left = 0, keyword_foundleft = 0;
    int done = 0;
    unsigned long border;
    context = (rule & RULE_CONTEXT) >> RULE_CONTEXT_SHIFT;
    keyword = (rule & RULE_WORD) >> RULE_WORD_SHIFT;
    border = rule & (RULE_ON_LEFT_BORDER | RULE_ON_RIGHT_BORDER);
    c1 = edit_get_byte (edit, i - 1);
    c2 = edit_get_byte (edit, i);
    if (!c2 || !c1)
	return rule;

    debug_printf ("%c->", c1);
    debug_printf ("%c ", c2);

/* check to turn off a keyword */
    if (keyword) {
	struct key_word *k;
	k = edit->rules[context]->keyword[keyword];
	if (c1 == '\n')
	    keyword = 0;
	if (k->last == c1 && compare_word_to_left (edit, i - 1, k->keyword, k->whole_word_chars_left, k->whole_word_chars_right, k->line_start)) {
	    keyword = 0;
	    keyword_foundleft = 1;
	    debug_printf ("keyword=%d ", keyword);
	}
    }
    debug_printf ("border=%s ", border ? ((border & RULE_ON_LEFT_BORDER) ? "left" : "right") : "off");

/* check to turn off a context */
    if (context && !keyword) {
	r = edit->rules[context];
	if (r->first_right == c2 && compare_word_to_right (edit, i, r->right, r->whole_word_chars_left, r->whole_word_chars_right, r->line_start_right) \
	    &&!(rule & RULE_ON_RIGHT_BORDER)) {
	    debug_printf ("A:3 ", 0);
	    found_right = 1;
	    border = RULE_ON_RIGHT_BORDER;
	    if (r->between_delimiters)
		context = 0;
	} else if (!found_left) {
	    if (r->last_right == c1 && compare_word_to_left (edit, i - 1, r->right, r->whole_word_chars_left, r->whole_word_chars_right, r->line_start_right) \
		&&(rule & RULE_ON_RIGHT_BORDER)) {
/* always turn off a context at 4 */
		debug_printf ("A:4 ", 0);
		found_left = 1;
		border = 0;
		if (!keyword_foundleft)
		    context = 0;
	    } else if (r->last_left == c1 && compare_word_to_left (edit, i - 1, r->left, r->whole_word_chars_left, r->whole_word_chars_right, r->line_start_left) \
		       &&(rule & RULE_ON_LEFT_BORDER)) {
/* never turn off a context at 2 */
		debug_printf ("A:2 ", 0);
		found_left = 1;
		border = 0;
	    }
	}
    }
    debug_printf ("\n", 0);

/* check to turn on a keyword */
    if (!keyword) {
	char *p;
	p = (r = edit->rules[context])->keyword_first_chars;
	while ((p = strchr (p + 1, c2))) {
	    struct key_word *k;
	    int count;
	    count = (unsigned long) p - (unsigned long) r->keyword_first_chars;
	    k = r->keyword[count];
	    if (compare_word_to_right (edit, i, k->keyword, k->whole_word_chars_left, k->whole_word_chars_right, k->line_start)) {
		keyword = count;
		debug_printf ("keyword=%d ", keyword);
		break;
	    }
	}
    }
/* check to turn on a context */
    if (!context) {
	int count;
	for (count = 1; edit->rules[count] && !done; count++) {
	    r = edit->rules[count];
	    if (!found_left) {
		if (r->last_right == c1 && compare_word_to_left (edit, i - 1, r->right, r->whole_word_chars_left, r->whole_word_chars_right, r->line_start_right) \
		    &&(rule & RULE_ON_RIGHT_BORDER)) {
		    debug_printf ("B:4 count=%d", count);
		    found_left = 1;
		    border = 0;
		    context = 0;
		    contextchanged = 1;
		    keyword = 0;
		} else if (r->last_left == c1 && compare_word_to_left (edit, i - 1, r->left, r->whole_word_chars_left, r->whole_word_chars_right, r->line_start_left) \
			   &&(rule & RULE_ON_LEFT_BORDER)) {
		    debug_printf ("B:2 ", 0);
		    found_left = 1;
		    border = 0;
		    if (r->between_delimiters) {
			context = count;
			contextchanged = 1;
			keyword = 0;
			debug_printf ("context=%d ", context);
			if (r->first_right == c2 && compare_word_to_right (edit, i, r->right, r->whole_word_chars_left, r->whole_word_chars_right, r->line_start_right)) {
			    debug_printf ("B:3 ", 0);
			    found_right = 1;
			    border = RULE_ON_RIGHT_BORDER;
			    context = 0;
			}
		    }
		    break;
		}
	    }
	    if (!found_right) {
		if (r->first_left == c2 && compare_word_to_right (edit, i, r->left, r->whole_word_chars_left, r->whole_word_chars_right, r->line_start_left)) {
		    debug_printf ("B:1 ", 0);
		    found_right = 1;
		    border = RULE_ON_LEFT_BORDER;
		    if (!r->between_delimiters) {
			debug_printf ("context=%d ", context);
			if (!keyword)
			    context = count;
		    }
		    break;
		}
	    }
	}
    }
    if (!keyword && contextchanged) {
	char *p;
	p = (r = edit->rules[context])->keyword_first_chars;
	while ((p = strchr (p + 1, c2))) {
	    struct key_word *k;
	    int coutner;
	    coutner = (unsigned long) p - (unsigned long) r->keyword_first_chars;
	    k = r->keyword[coutner];
	    if (compare_word_to_right (edit, i, k->keyword, k->whole_word_chars_left, k->whole_word_chars_right, k->line_start)) {
		keyword = coutner;
		debug_printf ("keyword=%d ", keyword);
		break;
	    }
	}
    }
    debug_printf ("border=%s ", border ? ((border & RULE_ON_LEFT_BORDER) ? "left" : "right") : "off");
    debug_printf ("keyword=%d ", keyword);

    debug_printf (" %d#\n\n", context);

    return (context << RULE_CONTEXT_SHIFT) | (keyword << RULE_WORD_SHIFT) | border;
}

static inline int resolve_left_delim (WEdit * edit, long i, struct context_rule *r, int s)
{
    int c, count;
    if (!r->conflicts)
	return s;
    for (;;) {
	c = edit_get_byte (edit, i);
	if (c == '\n')
	    break;
	for (count = 1; r->conflicts[count]; count++) {
	    struct context_rule *p;
	    p = edit->rules[r->conflicts[count]];
	    if (!p)
		break;
	    if (p->first_left == c && r->between_delimiters == p->between_delimiters && compare_word_to_right (edit, i, p->left, p->whole_word_chars_left, r->whole_word_chars_right, p->line_start_left))
		return r->conflicts[count];
	}
	i--;
    }
    return 0;
}

static inline unsigned long apply_rules_going_left (WEdit * edit, long i, unsigned long rule)
{
    struct context_rule *r;
    int context, contextchanged = 0, keyword, c2, c1;
    int found_left = 0, found_right = 0, keyword_foundright = 0;
    int done = 0;
    unsigned long border;
    context = (rule & RULE_CONTEXT) >> RULE_CONTEXT_SHIFT;
    keyword = (rule & RULE_WORD) >> RULE_WORD_SHIFT;
    border = rule & (RULE_ON_RIGHT_BORDER | RULE_ON_LEFT_BORDER);
    c1 = edit_get_byte (edit, i);
    c2 = edit_get_byte (edit, i + 1);
    if (!c2 || !c1)
	return rule;

    debug_printf ("%c->", c2);
    debug_printf ("%c ", c1);

/* check to turn off a keyword */
    if (keyword) {
	struct key_word *k;
	k = edit->rules[context]->keyword[keyword];
	if (c2 == '\n')
	    keyword = 0;
	if ((k->first == c2 && compare_word_to_right (edit, i + 1, k->keyword, k->whole_word_chars_right, k->whole_word_chars_left, k->line_start)) || (c2 == '\n')) {
	    keyword = 0;
	    keyword_foundright = 1;
	    debug_printf ("keyword=%d ", keyword);
	}
    }
    debug_printf ("border=%s ", border ? ((border & RULE_ON_RIGHT_BORDER) ? "right" : "left") : "off");

/* check to turn off a context */
    if (context && !keyword) {
	r = edit->rules[context];
	if (r->last_left == c1 && compare_word_to_left (edit, i, r->left, r->whole_word_chars_right, r->whole_word_chars_left, r->line_start_left) \
	    &&!(rule & RULE_ON_LEFT_BORDER)) {
	    debug_printf ("A:2 ", 0);
	    found_left = 1;
	    border = RULE_ON_LEFT_BORDER;
	    if (r->between_delimiters)
		context = 0;
	} else if (!found_right) {
	    if (r->first_left == c2 && compare_word_to_right (edit, i + 1, r->left, r->whole_word_chars_right, r->whole_word_chars_left, r->line_start_left) \
		&&(rule & RULE_ON_LEFT_BORDER)) {
/* always turn off a context at 4 */
		debug_printf ("A:1 ", 0);
		found_right = 1;
		border = 0;
		if (!keyword_foundright)
		    context = 0;
	    } else if (r->first_right == c2 && compare_word_to_right (edit, i + 1, r->right, r->whole_word_chars_right, r->whole_word_chars_left, r->line_start_right) \
		       &&(rule & RULE_ON_RIGHT_BORDER)) {
/* never turn off a context at 2 */
		debug_printf ("A:3 ", 0);
		found_right = 1;
		border = 0;
	    }
	}
    }
    debug_printf ("\n", 0);

/* check to turn on a keyword */
    if (!keyword) {
	char *p;
	p = (r = edit->rules[context])->keyword_last_chars;
	while ((p = strchr (p + 1, c1))) {
	    struct key_word *k;
	    int count;
	    count = (unsigned long) p - (unsigned long) r->keyword_last_chars;
	    k = r->keyword[count];
	    if (compare_word_to_left (edit, i, k->keyword, k->whole_word_chars_right, k->whole_word_chars_left, k->line_start)) {
		keyword = count;
		debug_printf ("keyword=%d ", keyword);
		break;
	    }
	}
    }
/* check to turn on a context */
    if (!context) {
	int count;
	for (count = 1; edit->rules[count] && !done; count++) {
	    r = edit->rules[count];
	    if (!found_right) {
		if (r->first_left == c2 && compare_word_to_right (edit, i + 1, r->left, r->whole_word_chars_right, r->whole_word_chars_left, r->line_start_left) \
		    &&(rule & RULE_ON_LEFT_BORDER)) {
		    debug_printf ("B:1 count=%d", count);
		    found_right = 1;
		    border = 0;
		    context = 0;
		    contextchanged = 1;
		    keyword = 0;
		} else if (r->first_right == c2 && compare_word_to_right (edit, i + 1, r->right, r->whole_word_chars_right, r->whole_word_chars_left, r->line_start_right) \
			   &&(rule & RULE_ON_RIGHT_BORDER)) {
		    if (!(c2 == '\n' && r->single_char)) {
			debug_printf ("B:3 ", 0);
			found_right = 1;
			border = 0;
			if (r->between_delimiters) {
			    debug_printf ("context=%d ", context);
			    context = resolve_left_delim (edit, i, r, count);
			    contextchanged = 1;
			    keyword = 0;
			    if (r->last_left == c1 && compare_word_to_left (edit, i, r->left, r->whole_word_chars_right, r->whole_word_chars_left, r->line_start_left)) {
				debug_printf ("B:2 ", 0);
				found_left = 1;
				border = RULE_ON_LEFT_BORDER;
				context = 0;
			    }
			}
			break;
		    }
		}
	    }
	    if (!found_left) {
		if (r->last_right == c1 && compare_word_to_left (edit, i, r->right, r->whole_word_chars_right, r->whole_word_chars_left, r->line_start_right)) {
		    if (!(c1 == '\n' && r->single_char)) {
			debug_printf ("B:4 ", 0);
			found_left = 1;
			border = RULE_ON_RIGHT_BORDER;
			if (!keyword)
			    if (!r->between_delimiters)
				context = resolve_left_delim (edit, i - 1, r, count);
			break;
		    }
		}
	    }
	}
    }
    if (!keyword && contextchanged) {
	char *p;
	p = (r = edit->rules[context])->keyword_last_chars;
	while ((p = strchr (p + 1, c1))) {
	    struct key_word *k;
	    int coutner;
	    coutner = (unsigned long) p - (unsigned long) r->keyword_last_chars;
	    k = r->keyword[coutner];
	    if (compare_word_to_left (edit, i, k->keyword, k->whole_word_chars_right, k->whole_word_chars_left, k->line_start)) {
		keyword = coutner;
		debug_printf ("keyword=%d ", keyword);
		break;
	    }
	}
    }
    debug_printf ("border=%s ", border ? ((border & RULE_ON_RIGHT_BORDER) ? "right" : "left") : "off");
    debug_printf ("keyword=%d ", keyword);

    debug_printf (" %d#\n\n", context);

    return (context << RULE_CONTEXT_SHIFT) | (keyword << RULE_WORD_SHIFT) | border;
}


static unsigned long edit_get_rule (WEdit * edit, long byte_index)
{
    long i;
    if (byte_index < 0) {
	edit->last_get_rule = -1;
	edit->rule = 0;
	return 0;
    }
#if 0
    if (byte_index < edit->last_get_rule_start_display) {
/* this is for optimisation */
	for (i = edit->last_get_rule_start_display - 1; i >= byte_index; i--)
	    edit->rule_start_display = apply_rules_going_left (edit, i, edit->rule_start_display);
	edit->last_get_rule_start_display = byte_index;
	edit->rule = edit->rule_start_display;
    } else
#endif
    if (byte_index > edit->last_get_rule) {
	for (i = edit->last_get_rule + 1; i <= byte_index; i++)
	    edit->rule = apply_rules_going_right (edit, i, edit->rule);
    } else if (byte_index < edit->last_get_rule) {
	for (i = edit->last_get_rule - 1; i >= byte_index; i--)
	    edit->rule = apply_rules_going_left (edit, i, edit->rule);
    }
    edit->last_get_rule = byte_index;
    return edit->rule;
}

static void translate_rule_to_color (WEdit * edit, unsigned long rule, int *fg, int *bg)
{
    struct key_word *k;
    k = edit->rules[(rule & RULE_CONTEXT) >> RULE_CONTEXT_SHIFT]->keyword[(rule & RULE_WORD) >> RULE_WORD_SHIFT];
    *bg = k->bg;
    *fg = k->fg;
}

void edit_get_syntax_color (WEdit * edit, long byte_index, int *fg, int *bg)
{
    unsigned long rule;
    if (!edit->rules || byte_index >= edit->last_byte || !option_syntax_highlighting) {
#ifdef MIDNIGHT
	*fg = NORMAL_COLOR;
#else
	*fg = NO_COLOR;
	*bg = NO_COLOR;
#endif
    } else {
	rule = edit_get_rule (edit, byte_index);
	translate_rule_to_color (edit, rule, fg, bg);
    }
}


/*
   Returns 0 on error/eof or a count of the number of bytes read
   including the newline. Result must be free'd.
 */
static int read_one_line (char **line, FILE * f)
{
    char *p;
    int len = 256, c, r = 0, i = 0;
    p = syntax_malloc (len);
    for (;;) {
	c = fgetc (f);
	if (c == -1) {
	    r = 0;
	    break;
	} else if (c == '\n') {
	    r = i + 1;		/* extra 1 for the newline just read */
	    break;
	} else {
	    if (i >= len - 1) {
		char *q;
		q = syntax_malloc (len * 2);
		memcpy (q, p, len);
		syntax_free (p);
		p = q;
		len *= 2;
	    }
	    p[i++] = c;
	}
    }
    p[i] = 0;
    *line = p;
    return r;
}

static char *strdup_convert (char *s)
{
#if 0
    int e = 0;
#endif
    char *r, *p;
    p = r = strdup (s);
    while (*s) {
	switch (*s) {
	case '\\':
	    s++;
	    switch (*s) {
	    case 'n':
		*p = '\n';
		break;
	    case 'r':
		*p = '\r';
		break;
	    case 't':
		*p = '\t';
		break;
	    case 's':
		*p = ' ';
		break;
	    case '*':
		*p = '*';
		break;
	    case '\\':
		*p = '\\';
		break;
	    case '[':
	    case ']':
		if ((unsigned long) p == (unsigned long) r || strlen (s) == 1)
		    *p = *s;
		else {
#if 0
		    if (!strncmp (s, "[^", 2)) {
			*p = '\004';
			e = 1;
			s++;
		    } else {
			if (e)
			    *p = '\004';
			else
#endif
			    *p = '\003';
#if 0
			e = 0;
		    }
#endif
		}
		break;
	    default:
		*p = *s;
		break;
	    }
	    break;
	case '*':
/* a * or + at the beginning or end of the line must be interpreted literally */
	    if ((unsigned long) p == (unsigned long) r || strlen (s) == 1)
		*p = '*';
	    else
		*p = '\001';
	    break;
	case '+':
	    if ((unsigned long) p == (unsigned long) r || strlen (s) == 1)
		*p = '+';
	    else
		*p = '\002';
	    break;
	default:
	    *p = *s;
	    break;
	}
	s++;
	p++;
    }
    *p = 0;
    return r;
}

#define whiteness(x) ((x) == '\t' || (x) == '\n' || (x) == ' ')

static void get_args (char *l, char **args, int *argc)
{
    *argc = 0;
    l--;
    for (;;) {
	char *p;
	for (p = l + 1; *p && whiteness (*p); p++);
	if (!*p)
	    break;
	for (l = p + 1; *l && !whiteness (*l); l++);
	*l = '\0';
	*args = strdup_convert (p);
	(*argc)++;
	args++;
    }
    *args = 0;
}

static void free_args (char **args)
{
    while (*args) {
	syntax_free (*args);
	*args = 0;
	args++;
    }
}

#define check_a {if(!*a){result=line;break;}}
#define check_not_a {if(*a){result=line;break;}}

#ifdef MIDNIGHT

int try_alloc_color_pair (char *fg, char *bg);

int this_try_alloc_color_pair (char *fg, char *bg)
{
    char f[80], b[80], *p;
    if (fg) {
	strcpy (f, fg);
	p = strchr (f, '/');
	if (p)
	    *p = '\0';
	fg = f;
    }
    if (bg) {
	strcpy (b, bg);
	p = strchr (b, '/');
	if (p)
	    *p = '\0';
	bg = b;
    }
    return try_alloc_color_pair (fg, bg);
}
#else
int this_allocate_color (char *fg)
{
    char *p;
    if (!fg)
	return allocate_color (0);
    p = strchr (fg, '/');
    if (!p)
	return allocate_color (fg);
    return allocate_color (p + 1);
}
#endif

/* returns line number on error */
static int edit_read_syntax_rules (WEdit * edit, FILE * f)
{
    char *fg, *bg;
    char whole_right[256];
    char whole_left[256];
    char *args[1024], *l = 0;
    int line = 0;
    struct context_rule **r, *c;
    int num_words = -1, num_contexts = -1;
    int argc, result = 0;
    int i, j;

    args[0] = 0;

    strcpy (whole_left, "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_01234567890");
    strcpy (whole_right, "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_01234567890");

    r = edit->rules = syntax_malloc (256 * sizeof (struct context_rule *));

    for (;;) {
	char **a;
	line++;
	if (!read_one_line (&l, f))
	    break;
	get_args (l, args, &argc);
	a = args + 1;
	if (!args[0]) {
	    /* do nothing */
	} else if (!strcmp (args[0], "wholechars")) {
	    check_a;
	    if (!strcmp (*a, "left")) {
		a++;
		strcpy (whole_left, *a);
	    } else if (!strcmp (*a, "right")) {
		a++;
		strcpy (whole_right, *a);
	    } else {
		strcpy (whole_left, *a);
		strcpy (whole_right, *a);
	    }
	    a++;
	    check_not_a;
	} else if (!strcmp (args[0], "context")) {
	    check_a;
	    if (num_contexts == -1) {
		if (strcmp (*a, "default")) {	/* first context is the default */
		    *a = 0;
		    check_a;
		}
		a++;
		c = r[0] = syntax_malloc (sizeof (struct context_rule));
		c->left = strdup (" ");
		c->right = strdup (" ");
		num_contexts = 0;
	    } else {
		c = r[num_contexts] = syntax_malloc (sizeof (struct context_rule));
		if (!strcmp (*a, "exclusive")) {
		    a++;
		    c->between_delimiters = 1;
		}
		check_a;
		if (!strcmp (*a, "whole")) {
		    a++;
		    c->whole_word_chars_left = strdup (whole_left);
		    c->whole_word_chars_right = strdup (whole_right);
		} else if (!strcmp (*a, "wholeleft")) {
		    a++;
		    c->whole_word_chars_left = strdup (whole_left);
		} else if (!strcmp (*a, "wholeright")) {
		    a++;
		    c->whole_word_chars_right = strdup (whole_right);
		}
		check_a;
		if (!strcmp (*a, "linestart")) {
		    a++;
		    c->line_start_left = 1;
		}
		check_a;
		c->left = strdup (*a++);
		check_a;
		if (!strcmp (*a, "linestart")) {
		    a++;
		    c->line_start_right = 1;
		}
		check_a;
		c->right = strdup (*a++);
		c->last_left = c->left[strlen (c->left) - 1];
		c->last_right = c->right[strlen (c->right) - 1];
		c->first_left = *c->left;
		c->first_right = *c->right;
		c->single_char = (strlen (c->right) == 1);
	    }
	    c->keyword = syntax_malloc (1024 * sizeof (struct key_word *));
	    num_words = 1;
	    c->keyword[0] = syntax_malloc (sizeof (struct key_word));
	    fg = *a;
	    if (*a)
		a++;
	    bg = *a;
	    if (*a)
		a++;
#ifdef MIDNIGHT
	    c->keyword[0]->fg = this_try_alloc_color_pair (fg, bg);
#else
	    c->keyword[0]->fg = this_allocate_color (fg);
	    c->keyword[0]->bg = this_allocate_color (bg);
#endif
	    c->keyword[0]->keyword = strdup (" ");
	    check_not_a;
	    num_contexts++;
	} else if (!strcmp (args[0], "keyword")) {
	    struct key_word *k;
	    if (num_words == -1)
		*a = 0;
	    check_a;
	    k = r[num_contexts - 1]->keyword[num_words] = syntax_malloc (sizeof (struct key_word));
	    if (!strcmp (*a, "whole")) {
		a++;
		k->whole_word_chars_left = strdup (whole_left);
		k->whole_word_chars_right = strdup (whole_right);
	    } else if (!strcmp (*a, "wholeleft")) {
		a++;
		k->whole_word_chars_left = strdup (whole_left);
	    } else if (!strcmp (*a, "wholeright")) {
		a++;
		k->whole_word_chars_right = strdup (whole_right);
	    }
	    check_a;
	    if (!strcmp (*a, "linestart")) {
		a++;
		k->line_start = 1;
	    }
	    check_a;
	    if (!strcmp (*a, "whole")) {
		*a = 0;
		check_a;
	    }
	    k->keyword = strdup (*a++);
	    k->last = k->keyword[strlen (k->keyword) - 1];
	    k->first = *k->keyword;
	    fg = *a;
	    if (*a)
		a++;
	    bg = *a;
	    if (*a)
		a++;
#ifdef MIDNIGHT
	    k->fg = this_try_alloc_color_pair (fg, bg);
#else
	    k->fg = this_allocate_color (fg);
	    k->bg = this_allocate_color (bg);
#endif
	    check_not_a;
	    num_words++;
	} else if (!strncmp (args[0], "#", 1)) {
	    /* do nothing for comment */
	} else if (!strcmp (args[0], "file")) {
	    break;
	} else {		/* anything else is an error */
	    *a = 0;
	    check_a;
	}
	free_args (args);
	syntax_free (l);
    }
    free_args (args);
    syntax_free (l);

    if (result)
	return result;

    if (num_contexts == -1) {
	result = line;
	return result;
    }
    for (i = 1; edit->rules[i]; i++) {
	for (j = i + 1; edit->rules[j]; j++) {
	    if (strstr (edit->rules[j]->right, edit->rules[i]->right) && strcmp (edit->rules[i]->right, "\n")) {
		unsigned char *s;
		if (!edit->rules[i]->conflicts)
		    edit->rules[i]->conflicts = syntax_malloc (sizeof (unsigned char) * 260);
		s = edit->rules[i]->conflicts;
		s[strlen ((char *) s)] = (unsigned char) j;
	    }
	}
    }

    {
	char first_chars[1024], *p;
	char last_chars[1024], *q;
	for (i = 0; edit->rules[i]; i++) {
	    c = edit->rules[i];
	    p = first_chars;
	    q = last_chars;
	    *p++ = (char) 1;
	    *q++ = (char) 1;
	    for (j = 1; c->keyword[j]; j++) {
		*p++ = c->keyword[j]->first;
		*q++ = c->keyword[j]->last;
	    }
	    *p = '\0';
	    *q = '\0';
	    c->keyword_first_chars = strdup (first_chars);
	    c->keyword_last_chars = strdup (last_chars);
	}
    }

    return result;
}

void (*syntax_change_callback) (CWidget *) = 0;

void edit_set_syntax_change_callback (void (*callback) (CWidget *))
{
    syntax_change_callback = callback;
}

void edit_free_syntax_rules (WEdit * edit)
{
    int i, j;
    if (!edit)
	return;
    if (!edit->rules)
	return;
    syntax_free (edit->syntax_type);
    if (syntax_change_callback)
#ifdef MIDNIGHT
	(*syntax_change_callback) (&edit->widget);
#else
	(*syntax_change_callback) (edit->widget);
#endif
    for (i = 0; edit->rules[i]; i++) {
	if (edit->rules[i]->keyword) {
	    for (j = 0; edit->rules[i]->keyword[j]; j++) {
		syntax_free (edit->rules[i]->keyword[j]->keyword);
		syntax_free (edit->rules[i]->keyword[j]->whole_word_chars_left);
		syntax_free (edit->rules[i]->keyword[j]->whole_word_chars_right);
		syntax_free (edit->rules[i]->keyword[j]);
	    }
	}
	syntax_free (edit->rules[i]->conflicts);
	syntax_free (edit->rules[i]->left);
	syntax_free (edit->rules[i]->right);
	syntax_free (edit->rules[i]->whole_word_chars_left);
	syntax_free (edit->rules[i]->whole_word_chars_right);
	syntax_free (edit->rules[i]->keyword);
	syntax_free (edit->rules[i]->keyword_first_chars);
	syntax_free (edit->rules[i]->keyword_last_chars);
	syntax_free (edit->rules[i]);
    }
    syntax_free (edit->rules);
}

#define CURRENT_SYNTAX_RULES_VERSION "22"

char *syntax_text = 
"# syntax rules version " CURRENT_SYNTAX_RULES_VERSION "\n"
"# Allowable colors for mc are\n"
"# (after the slash is a Cooledit color, 0-26 or any of the X colors in rgb.txt)\n"
"# black\n"
"# red\n"
"# green\n"
"# brown\n"
"# blue\n"
"# magenta\n"
"# cyan\n"
"# lightgray\n"
"# gray\n"
"# brightred\n"
"# brightgreen\n"
"# yellow\n"
"# brightblue\n"
"# brightmagenta\n"
"# brightcyan\n"
"# white\n"
"\n"
"###############################################################################\n"
"file ..\\*\\\\.tex$ LaTeX\\s2.09\\sDocument\n"
"context default\n"
"wholechars left \\\\\n"
"wholechars right abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ\n"
"\n"
"# type style\n"
"    keyword whole \\\\tiny yellow/24\n"
"    keyword whole \\\\scriptsize yellow/24\n"
"    keyword whole \\\\footnotesize yellow/24\n"
"    keyword whole \\\\small yellow/24\n"
"    keyword whole \\\\normalsize yellow/24\n"
"    keyword whole \\\\large yellow/24\n"
"    keyword whole \\\\Large yellow/24\n"
"    keyword whole \\\\LARGE yellow/24\n"
"    keyword whole \\\\huge yellow/24\n"
"    keyword whole \\\\Huge yellow/24\n"
"\n"
"# accents and symbols\n"
"    keyword whole \\\\`{\\[aeiouAEIOU\\]} yellow/24\n"
"    keyword whole \\\\'{\\[aeiouAEIOU\\]} yellow/24\n"
"    keyword whole \\\\^{\\[aeiouAEIOU\\]} yellow/24\n"
"    keyword whole \\\\\"{\\[aeiouAEIOU\\]} yellow/24\n"
"    keyword whole \\\\~{\\[aeiouAEIOU\\]} yellow/24\n"
"    keyword whole \\\\={\\[aeiouAEIOU\\]} yellow/24\n"
"    keyword whole \\\\.{\\[aeiouAEIOU\\]} yellow/24\n"
"    keyword whole \\\\u{\\[aeiouAEIOU\\]} yellow/24\n"
"    keyword whole \\\\v{\\[aeiouAEIOU\\]} yellow/24\n"
"    keyword whole \\\\H{\\[aeiouAEIOU\\]} yellow/24\n"
"    keyword whole \\\\t{\\[aeiouAEIOU\\]} yellow/24\n"
"    keyword whole \\\\c{\\[aeiouAEIOU\\]} yellow/24\n"
"    keyword whole \\\\d{\\[aeiouAEIOU\\]} yellow/24\n"
"    keyword whole \\\\b{\\[aeiouAEIOU\\]} yellow/24\n"
"\n"
"    keyword whole \\\\dag yellow/24\n"
"    keyword whole \\\\ddag yellow/24\n"
"    keyword whole \\\\S yellow/24\n"
"    keyword whole \\\\P yellow/24\n"
"    keyword whole \\\\copyright yellow/24\n"
"    keyword whole \\\\pounds yellow/24\n"
"\n"
"# sectioning and table of contents\n"
"    keyword whole \\\\part[*]{*} brightred/19\n"
"    keyword whole \\\\part{*} brightred/19\n"
"    keyword whole \\\\part\\*{*} brightred/19\n"
"    keyword whole \\\\chapter[*]{*} brightred/19\n"
"    keyword whole \\\\chapter{*} brightred/19\n"
"    keyword whole \\\\chapter\\*{*} brightred/19\n"
"    keyword whole \\\\section[*]{*} brightred/19\n"
"    keyword whole \\\\section{*} brightred/19\n"
"    keyword whole \\\\section\\*{*} brightred/19\n"
"    keyword whole \\\\subsection[*]{*} brightred/19\n"
"    keyword whole \\\\subsection{*} brightred/19\n"
"    keyword whole \\\\subsection\\*{*} brightred/19\n"
"    keyword whole \\\\subsubsection[*]{*} brightred/19\n"
"    keyword whole \\\\subsubsection{*} brightred/19\n"
"    keyword whole \\\\subsubsection\\*{*} brightred/19\n"
"    keyword whole \\\\paragraph[*]{*} brightred/19\n"
"    keyword whole \\\\paragraph{*} brightred/19\n"
"    keyword whole \\\\paragraph\\*{*} brightred/19\n"
"    keyword whole \\\\subparagraph[*]{*} brightred/19\n"
"    keyword whole \\\\subparagraph{*} brightred/19\n"
"    keyword whole \\\\subparagraph\\*{*} brightred/19\n"
"\n"
"    keyword whole \\\\appendix brightred/19\n"
"    keyword whole \\\\tableofcontents brightred/19\n"
"\n"
"# misc\n"
"    keyword whole \\\\item[*] yellow/24\n"
"    keyword whole \\\\item yellow/24\n"
"    keyword whole \\\\\\\\ yellow/24\n"
"    keyword \\\\\\s yellow/24 black/0\n"
"    keyword %% yellow/24\n"
"\n"
"# docuement and page styles    \n"
"    keyword whole \\\\documentstyle[*]{*} yellow/20\n"
"    keyword whole \\\\documentstyle{*} yellow/20\n"
"    keyword whole \\\\pagestyle{*} yellow/20\n"
"\n"
"# cross references\n"
"    keyword whole \\\\label{*} yellow/24\n"
"    keyword whole \\\\ref{*} yellow/24\n"
"\n"
"# bibliography and citations\n"
"    keyword whole \\\\bibliography{*} yellow/24\n"
"    keyword whole \\\\bibitem[*]{*} yellow/24\n"
"    keyword whole \\\\bibitem{*} yellow/24\n"
"    keyword whole \\\\cite[*]{*} yellow/24\n"
"    keyword whole \\\\cite{*} yellow/24\n"
"\n"
"# splitting the input\n"
"    keyword whole \\\\input{*} yellow/20\n"
"    keyword whole \\\\include{*} yellow/20\n"
"    keyword whole \\\\includeonly{*} yellow/20\n"
"\n"
"# line breaking\n"
"    keyword whole \\\\linebreak[\\[01234\\]] yellow/24\n"
"    keyword whole \\\\nolinebreak[\\[01234\\]] yellow/24\n"
"    keyword whole \\\\linebreak yellow/24\n"
"    keyword whole \\\\nolinebreak yellow/24\n"
"    keyword whole \\\\[+] yellow/24\n"
"    keyword whole \\\\- yellow/24\n"
"    keyword whole \\\\sloppy yellow/24\n"
"\n"
"# page breaking\n"
"    keyword whole \\\\pagebreak[\\[01234\\]] yellow/24\n"
"    keyword whole \\\\nopagebreak[\\[01234\\]] yellow/24\n"
"    keyword whole \\\\pagebreak yellow/24\n"
"    keyword whole \\\\nopagebreak yellow/24\n"
"    keyword whole \\\\samepage yellow/24\n"
"    keyword whole \\\\newpage yellow/24\n"
"    keyword whole \\\\clearpage yellow/24\n"
"\n"
"# defintiions\n"
"    keyword \\\\newcommand{*}[*] cyan/5\n"
"    keyword \\\\newcommand{*} cyan/5\n"
"    keyword \\\\newenvironment{*}[*]{*} cyan/5\n"
"    keyword \\\\newenvironment{*}{*} cyan/5\n"
"\n"
"# boxes\n"
"\n"
"# begins and ends\n"
"    keyword \\\\begin{document} brightred/14\n"
"    keyword \\\\begin{equation} brightred/14\n"
"    keyword \\\\begin{eqnarray} brightred/14\n"
"    keyword \\\\begin{quote} brightred/14\n"
"    keyword \\\\begin{quotation} brightred/14\n"
"    keyword \\\\begin{center} brightred/14\n"
"    keyword \\\\begin{verse} brightred/14\n"
"    keyword \\\\begin{verbatim} brightred/14\n"
"    keyword \\\\begin{itemize} brightred/14\n"
"    keyword \\\\begin{enumerate} brightred/14\n"
"    keyword \\\\begin{description} brightred/14\n"
"    keyword \\\\begin{array} brightred/14\n"
"    keyword \\\\begin{tabular} brightred/14\n"
"    keyword \\\\begin{thebibliography}{*} brightred/14\n"
"    keyword \\\\begin{sloppypar} brightred/14\n"
"\n"
"    keyword \\\\end{document} brightred/14\n"
"    keyword \\\\end{equation} brightred/14\n"
"    keyword \\\\end{eqnarray} brightred/14\n"
"    keyword \\\\end{quote} brightred/14\n"
"    keyword \\\\end{quotation} brightred/14\n"
"    keyword \\\\end{center} brightred/14\n"
"    keyword \\\\end{verse} brightred/14\n"
"    keyword \\\\end{verbatim} brightred/14\n"
"    keyword \\\\end{itemize} brightred/14\n"
"    keyword \\\\end{enumerate} brightred/14\n"
"    keyword \\\\end{description} brightred/14\n"
"    keyword \\\\end{array} brightred/14\n"
"    keyword \\\\end{tabular} brightred/14\n"
"    keyword \\\\end{thebibliography}{*} brightred/14\n"
"    keyword \\\\end{sloppypar} brightred/14\n"
"\n"
"    keyword \\\\begin{*} brightcyan/16\n"
"    keyword \\\\end{*} brightcyan/16\n"
"\n"
"    keyword \\\\theorem{*}{*} yellow/24\n"
"\n"
"# if all else fails\n"
"    keyword whole \\\\+[*]{*}{*}{*} brightcyan/17\n"
"    keyword whole \\\\+[*]{*}{*} brightcyan/17\n"
"    keyword whole \\\\+{*}{*}{*}{*} brightcyan/17\n"
"    keyword whole \\\\+{*}{*}{*} brightcyan/17\n"
"    keyword whole \\\\+{*}{*} brightcyan/17\n"
"    keyword whole \\\\+{*} brightcyan/17\n"
"    keyword \\\\\\[abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ\\]\\n brightcyan/17\n"
"    keyword \\\\\\[abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ\\]\\s brightcyan/17\n"
"    keyword \\\\\\[abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ\\]\\t brightcyan/17\n"
"\n"
"context \\\\pagenumbering{ } yellow/20\n"
"    keyword arabic brightcyan/17\n"
"    keyword roman brightcyan/17\n"
"    keyword alph brightcyan/17\n"
"    keyword Roman brightcyan/17\n"
"    keyword Alph brightcyan/17\n"
"\n"
"context % \\n brown/22\n"
"\n"
"# mathematical formulas\n"
"context $ $ brightgreen/6\n"
"context exclusive \\\\begin{equation} \\\\end{equation} brightgreen/6\n"
"context exclusive \\\\begin{eqnarray} \\\\end{eqnarray} brightgreen/6\n"
"\n"
"\n"
"###############################################################################\n"
"file ..\\*\\\\.([chC]|CC|cxx|cc|cpp|CPP|CXX)$ C/C\\+\\+\\sProgram\n"
"context default\n"
"    keyword whole auto yellow/24\n"
"    keyword whole break yellow/24\n"
"    keyword whole case yellow/24\n"
"    keyword whole char yellow/24\n"
"    keyword whole const yellow/24\n"
"    keyword whole continue yellow/24\n"
"    keyword whole default yellow/24\n"
"    keyword whole do yellow/24\n"
"    keyword whole double yellow/24\n"
"    keyword whole else yellow/24\n"
"    keyword whole enum yellow/24\n"
"    keyword whole extern yellow/24\n"
"    keyword whole float yellow/24\n"
"    keyword whole for yellow/24\n"
"    keyword whole goto yellow/24\n"
"    keyword whole if yellow/24\n"
"    keyword whole int yellow/24\n"
"    keyword whole long yellow/24\n"
"    keyword whole register yellow/24\n"
"    keyword whole return yellow/24\n"
"    keyword whole short yellow/24\n"
"    keyword whole signed yellow/24\n"
"    keyword whole sizeof yellow/24\n"
"    keyword whole static yellow/24\n"
"    keyword whole struct yellow/24\n"
"    keyword whole switch yellow/24\n"
"    keyword whole typedef yellow/24\n"
"    keyword whole union yellow/24\n"
"    keyword whole unsigned yellow/24\n"
"    keyword whole void yellow/24\n"
"    keyword whole volatile yellow/24\n"
"    keyword whole while yellow/24\n"
"    keyword whole asm yellow/24\n"
"    keyword whole catch yellow/24\n"
"    keyword whole class yellow/24\n"
"    keyword whole friend yellow/24\n"
"    keyword whole delete yellow/24\n"
"    keyword whole inline yellow/24\n"
"    keyword whole new yellow/24\n"
"    keyword whole operator yellow/24\n"
"    keyword whole private yellow/24\n"
"    keyword whole protected yellow/24\n"
"    keyword whole public yellow/24\n"
"    keyword whole this yellow/24\n"
"    keyword whole throw yellow/24\n"
"    keyword whole template yellow/24\n"
"    keyword whole try yellow/24\n"
"    keyword whole virtual yellow/24\n"
"    keyword whole bool yellow/24\n"
"    keyword whole const_cast yellow/24\n"
"    keyword whole dynamic_cast yellow/24\n"
"    keyword whole explicit yellow/24\n"
"    keyword whole false yellow/24\n"
"    keyword whole mutable yellow/24\n"
"    keyword whole namespace yellow/24\n"
"    keyword whole reinterpret_cast yellow/24\n"
"    keyword whole static_cast yellow/24\n"
"    keyword whole true yellow/24\n"
"    keyword whole typeid yellow/24\n"
"    keyword whole typename yellow/24\n"
"    keyword whole using yellow/24\n"
"    keyword whole wchar_t yellow/24\n"
"    keyword whole ... yellow/24\n"
"\n"
"    keyword /\\* brown/22\n"
"    keyword \\*/ brown/22\n"
"\n"
"    keyword '\\s' brightgreen/16\n"
"    keyword '+' brightgreen/16\n"
"    keyword > yellow/24\n"
"    keyword < yellow/24\n"
"    keyword \\+ yellow/24\n"
"    keyword - yellow/24\n"
"    keyword \\* yellow/24\n"
"    keyword / yellow/24\n"
"    keyword % yellow/24\n"
"    keyword = yellow/24\n"
"    keyword != yellow/24\n"
"    keyword == yellow/24\n"
"    keyword { brightcyan/14\n"
"    keyword } brightcyan/14\n"
"    keyword ( brightcyan/15\n"
"    keyword ) brightcyan/15\n"
"    keyword [ brightcyan/14\n"
"    keyword ] brightcyan/14\n"
"    keyword , brightcyan/14\n"
"    keyword : brightcyan/14\n"
"    keyword ; brightmagenta/19\n"
"context exclusive /\\* \\*/ brown/22\n"
"context linestart # \\n brightred/18\n"
"    keyword \\\\\\n yellow/24\n"
"    keyword /\\**\\*/ brown/22\n"
"    keyword \"+\" red/19\n"
"    keyword <+> red/19\n"
"context \" \" green/6\n"
"    keyword \\\\\" brightgreen/16\n"
"    keyword %% brightgreen/16\n"
"    keyword %\\[#0\\s-\\+,\\]\\[0123456789\\]\\[.\\]\\[0123456789\\]\\[L\\]e brightgreen/16\n"
"    keyword %\\[#0\\s-\\+,\\]\\[0123456789\\]\\[.\\]\\[0123456789\\]\\[L\\]E brightgreen/16\n"
"    keyword %\\[#0\\s-\\+,\\]\\[0123456789\\]\\[.\\]\\[0123456789\\]\\[L\\]f brightgreen/16\n"
"    keyword %\\[#0\\s-\\+,\\]\\[0123456789\\]\\[.\\]\\[0123456789\\]\\[L\\]g brightgreen/16\n"
"    keyword %\\[#0\\s-\\+,\\]\\[0123456789\\]\\[.\\]\\[0123456789\\]\\[L\\]G brightgreen/16\n"
"    keyword %\\[0\\s-\\+,\\]\\[0123456789\\]\\[.\\]\\[0123456789\\]\\[hl\\]d brightgreen/16\n"
"    keyword %\\[0\\s-\\+,\\]\\[0123456789\\]\\[.\\]\\[0123456789\\]\\[hl\\]i brightgreen/16\n"
"    keyword %\\[#0\\s-\\+,\\]\\[0123456789\\]\\[.\\]\\[0123456789\\]\\[hl\\]o brightgreen/16\n"
"    keyword %\\[0\\s-\\+,\\]\\[0123456789\\]\\[.\\]\\[0123456789\\]\\[hl\\]u brightgreen/16\n"
"    keyword %\\[#0\\s-\\+,\\]\\[0123456789\\]\\[.\\]\\[0123456789\\]\\[hl\\]x brightgreen/16\n"
"    keyword %\\[#0\\s-\\+,\\]\\[0123456789\\]\\[.\\]\\[0123456789\\]\\[hl\\]X brightgreen/16\n"
"    keyword %\\[hl\\]n brightgreen/16\n"
"    keyword %\\[.\\]\\[0123456789\\]s brightgreen/16\n"
"    keyword %[*] brightgreen/16\n"
"    keyword %c brightgreen/16\n"
"    keyword \\\\\\\\ brightgreen/16\n"
"\n"
"###############################################################################\n"
"file .\\*ChangeLog$ GNU\\sDistribution\\sChangeLog\\sFile\n"
"\n"
"context default\n"
"    keyword \\s+() brightmagenta/23\n"
"    keyword \\t+() brightmagenta/23\n"
"\n"
"context linestart \\t\\* : brightcyan/17\n"
"context linestart \\s\\s\\s\\s\\s\\s\\s\\s\\* : brightcyan/17\n"
"\n"
"context linestart 19+-+\\s \\n            yellow/24\n"
"    keyword <+@+> 			brightred/19\n"
"context linestart 20+-+\\s \\n            yellow/24\n"
"    keyword <+@+> 			brightred/19\n"
"context linestart Mon\\s+\\s+\\s+\\s \\n     yellow/24\n"
"    keyword <+@+> 			brightred/19\n"
"context linestart Tue\\s+\\s+\\s+\\s \\n     yellow/24\n"
"    keyword <+@+> 			brightred/19\n"
"context linestart Wed\\s+\\s+\\s+\\s \\n     yellow/24\n"
"    keyword <+@+> 			brightred/19\n"
"context linestart Thu\\s+\\s+\\s+\\s \\n     yellow/24\n"
"    keyword <+@+> 			brightred/19\n"
"context linestart Fri\\s+\\s+\\s+\\s \\n     yellow/24\n"
"    keyword <+@+> 			brightred/19\n"
"context linestart Sat\\s+\\s+\\s+\\s \\n     yellow/24\n"
"    keyword <+@+> 			brightred/19\n"
"context linestart Sun\\s+\\s+\\s+\\s \\n     yellow/24\n"
"    keyword <+@+> 			brightred/19\n"
"\n"
"\n"
"###############################################################################\n"
"file .\\*Makefile[\\\\\\.a-z]\\*$ Makefile\n"
"\n"
"context default\n"
"    keyword $(*) yellow/24\n"
"    keyword ${*} brightgreen/16\n"
"    keyword whole linestart include magenta\n"
"    keyword whole linestart endif magenta\n"
"    keyword whole linestart ifeq magenta\n"
"    keyword whole linestart ifneq magenta\n"
"    keyword whole linestart else magenta\n"
"    keyword linestart \\t lightgray/13 red\n"
"    keyword whole .PHONY white/25\n"
"    keyword whole .NOEXPORT white/25\n"
"    keyword = white/25\n"
"    keyword : yellow/24\n"
"    keyword \\\\\\n yellow/24\n"
"# this handles strange cases like @something@@somethingelse@ properly\n"
"    keyword whole @+@ brightmagenta/23 black/0\n"
"    keyword @+@ brightmagenta/23 black/0\n"
"\n"
"context linestart # \\n brown/22\n"
"    keyword whole @+@ brightmagenta/23 black/0\n"
"    keyword @+@ brightmagenta/23 black/0\n"
"\n"
"context exclusive = \\n brightcyan/17\n"
"    keyword \\\\\\n yellow/24\n"
"    keyword $(*) yellow/24\n"
"    keyword ${*} brightgreen/16\n"
"    keyword linestart \\t lightgray/13 red\n"
"    keyword whole @+@ brightmagenta/23 black/0\n"
"    keyword @+@ brightmagenta/23 black/0\n"
"\n"
"context exclusive linestart \\t \\n\n"
"    keyword \\\\\\n yellow/24\n"
"    keyword $(*) yellow/24\n"
"    keyword ${*} brightgreen/16\n"
"    keyword linestart \\t lightgray/13 red\n"
"    keyword whole @+@ brightmagenta/23 black/0\n"
"    keyword @+@ brightmagenta/23 black/0\n"
"\n"
"###############################################################################\n"
"\n"
"file .\\*syntax$ Syntax\\sHighlighting\\sdefinitions\n"
"\n"
"context default\n"
"    keyword whole keyw\\ord yellow/24\n"
"    keyword whole whole\\[\\t\\s\\]l\\inestart brightcyan/17\n"
"    keyword whole whole\\[\\t\\s\\]l\\inestart brightcyan/17\n"
"    keyword whole wh\\oleleft\\[\\t\\s\\]l\\inestart brightcyan/17\n"
"    keyword whole wh\\oleright\\[\\t\\s\\]l\\inestart brightcyan/17\n"
"    keyword whole l\\inestart\\[\\t\\s\\]wh\\ole\n"
"    keyword whole l\\inestart\\[\\t\\s\\]wh\\ole\n"
"    keyword whole l\\inestart\\[\\t\\s\\]wh\\oleleft\n"
"    keyword whole l\\inestart\\[\\t\\s\\]wh\\oleright\n"
"    keyword wholeleft whole\\s brightcyan/17\n"
"    keyword wholeleft whole\\t brightcyan/17\n"
"    keyword whole wh\\oleleft brightcyan/17\n"
"    keyword whole wh\\oleright brightcyan/17\n"
"    keyword whole lin\\[e\\]start brightcyan/17\n"
"    keyword whole c\\ontext\\[\\t\\s\\]exclusive brightred/18\n"
"    keyword whole c\\ontext\\[\\t\\s\\]default brightred/18\n"
"    keyword whole c\\ontext brightred/18\n"
"    keyword whole wh\\olechars\\[\\t\\s\\]left brightcyan/17\n"
"    keyword whole wh\\olechars\\[\\t\\s\\]right brightcyan/17\n"
"    keyword whole wh\\olechars brightcyan/17\n"
"    keyword whole f\\ile brightgreen/6\n"
"\n"
"    keyword whole 0 lightgray/0	blue/26\n"
"    keyword whole 1 lightgray/1	blue/26\n"
"    keyword whole 2 lightgray/2	blue/26\n"
"    keyword whole 3 lightgray/3	blue/26\n"
"    keyword whole 4 lightgray/4	blue/26\n"
"    keyword whole 5 lightgray/5	blue/26\n"
"    keyword whole 6 lightgray/6\n"
"    keyword whole 7 lightgray/7\n"
"    keyword whole 8 lightgray/8\n"
"    keyword whole 9 lightgray/9\n"
"    keyword whole 10 lightgray/10\n"
"    keyword whole 11 lightgray/11\n"
"    keyword whole 12 lightgray/12\n"
"    keyword whole 13 lightgray/13\n"
"    keyword whole 14 lightgray/14\n"
"    keyword whole 15 lightgray/15\n"
"    keyword whole 16 lightgray/16\n"
"    keyword whole 17 lightgray/17\n"
"    keyword whole 18 lightgray/18\n"
"    keyword whole 19 lightgray/19\n"
"    keyword whole 20 lightgray/20\n"
"    keyword whole 21 lightgray/21\n"
"    keyword whole 22 lightgray/22\n"
"    keyword whole 23 lightgray/23\n"
"    keyword whole 24 lightgray/24\n"
"    keyword whole 25 lightgray/25\n"
"    keyword whole 26 lightgray/26\n"
"\n"
"    keyword wholeleft black\\/ black/0\n"
"    keyword wholeleft red\\/ red/DarkRed\n"
"    keyword wholeleft green\\/ green/green3\n"
"    keyword wholeleft brown\\/ brown/saddlebrown\n"
"    keyword wholeleft blue\\/ blue/blue3\n"
"    keyword wholeleft magenta\\/ magenta/magenta3\n"
"    keyword wholeleft cyan\\/ cyan/cyan3\n"
"    keyword wholeleft lightgray\\/ lightgray/lightgray\n"
"    keyword wholeleft gray\\/ gray/gray\n"
"    keyword wholeleft brightred\\/ brightred/red\n"
"    keyword wholeleft brightgreen\\/ brightgreen/green1\n"
"    keyword wholeleft yellow\\/ yellow/yellow\n"
"    keyword wholeleft brightblue\\/ brightblue/blue1\n"
"    keyword wholeleft brightmagenta\\/ brightmagenta/magenta\n"
"    keyword wholeleft brightcyan\\/ brightcyan/cyan1\n"
"    keyword wholeleft white\\/ white/26\n"
"\n"
"context linestart # \\n brown/22\n"
"\n"
"file \\.\\* Help\\ssupport\\sother\\sfile\\stypes\n"
"context default\n"
"file \\.\\* by\\scoding\\srules\\sin\\s~/.cedit/syntax.\n"
"context default\n"
"file \\.\\* See\\sman/syntax\\sin\\sthe\\ssource\\sdistribution\n"
"context default\n"
"file \\.\\* and\\sconsult\\sthe\\sman\\spage.\n"
"context default\n";



FILE *upgrade_syntax_file (char *syntax_file)
{
    FILE *f;
    char line[80];
    f = fopen (syntax_file, "r");
    if (!f) {
	f = fopen (syntax_file, "w");
	if (!f)
	    return 0;
	fprintf (f, "%s", syntax_text);
	fclose (f);
	return fopen (syntax_file, "r");
    }
    memset (line, 0, 79);
    fread (line, 80, 1, f);
    if (!strstr (line, "syntax rules version")) {
	goto rename_rule_file;
    } else {
	char *p;
	p = strstr (line, "version") + strlen ("version") + 1;
	if (atoi (p) < atoi (CURRENT_SYNTAX_RULES_VERSION)) {
	    char s[1024];
	  rename_rule_file:
	    strcpy (s, syntax_file);
	    strcat (s, ".OLD");
	    unlink (s);
	    rename (syntax_file, s);
	    unlink (syntax_file);	/* might rename() fail ? */
#ifdef MIDNIGHT
	    edit_message_dialog (" Load Syntax Rules ", " Your syntax rule file is outdated \n A new rule file is being installed. \n Your old rule file has been saved with a .OLD extension. ");
#else
	    CMessageDialog (0, 20, 20, 0, " Load Syntax Rules ", " Your syntax rule file is outdated \n A new rule file is being installed. \n Your old rule file has been saved with a .OLD extension. ");
#endif
	    return upgrade_syntax_file (syntax_file);
	} else {
	    rewind (f);
	    return (f);
	}
    }
    return 0;			/* not reached */
}

/* returns -1 on file error, line number on error in file syntax */
static int edit_read_syntax_file (WEdit * edit, char **names, char *syntax_file, char *editor_file, char *type)
{
    FILE *f;
    regex_t r;
    regmatch_t pmatch[1];
    char *args[1024], *l;
    int line = 0;
    int argc;
    int result = 0;
    int count = 0;

    f = upgrade_syntax_file (syntax_file);
    if (!f)
	return -1;
    args[0] = 0;

    for (;;) {
	line++;
	if (!read_one_line (&l, f))
	    break;
	get_args (l, args, &argc);
	if (!args[0]) {
	} else if (!strcmp (args[0], "file")) {
	    if (!args[1] || !args[2]) {
		result = line;
		break;
	    }
	    if (regcomp (&r, args[1], REG_EXTENDED)) {
		result = line;
		break;
	    }
	    if (names) {
		names[count++] = strdup (args[2]);
		names[count] = 0;
	    } else if (type) {
		if (!strcmp (type, args[2]))
		    goto found_type;
	    } else if (editor_file && edit) {
		if (!regexec (&r, editor_file, 1, pmatch, 0)) {
		    int line_error;
		  found_type:
		    line_error = edit_read_syntax_rules (edit, f);
		    if (line_error)
			result = line + line_error;
		    else {
			syntax_free (edit->syntax_type);
			edit->syntax_type = strdup (args[2]);
			if (syntax_change_callback)
#ifdef MIDNIGHT
			    (*syntax_change_callback) (&edit->widget);
#else
			    (*syntax_change_callback) (edit->widget);
#endif
/* if there are no rules then turn off syntax highlighting for speed */
			if (!edit->rules[1])
			    if (!edit->rules[0]->keyword[1])
				edit_free_syntax_rules (edit);
		    }
		    break;
		}
	    }
	}
	free_args (args);
	syntax_free (l);
    }
    free_args (args);
    syntax_free (l);

    fclose (f);

    return result;
}

/* loads rules into edit struct. one of edit or names must be zero. if
   edit is zero, a list of types will be stored into name. type may be zero
   in which case the type will be selected according to the filename. */
void edit_load_syntax (WEdit * edit, char **names, char *type)
{
    int r;
    char *f;

    edit_free_syntax_rules (edit);

#ifdef MIDNIGHT
    if (!SLtt_Use_Ansi_Colors)
	return;
#endif

    if (edit) {
	if (!edit->filename)
	    return;
	if (!*edit->filename && !type)
	    return;
    }
    f = catstrs (home_dir, SYNTAX_FILE, 0);
    r = edit_read_syntax_file (edit, names, f, edit ? edit->filename : 0, type);
    if (r == -1) {
	edit_free_syntax_rules (edit);
	edit_error_dialog (_ (" Load syntax file "), _ (" File access error "));
	return;
    }
    if (r) {
	char s[80];
	edit_free_syntax_rules (edit);
	sprintf (s, _ (" Syntax error in file %s on line %d "), f, r);
	edit_error_dialog (_ (" Load syntax file "), s);
	return;
    }
}

#else

int option_syntax_highlighting = 0;

void edit_load_syntax (WEdit * edit, char **names, char *type)
{
    return;
}

void edit_free_syntax_rules (WEdit * edit)
{
    return;
}

void edit_get_syntax_color (WEdit * edit, long byte_index, int *fg, int *bg)
{
    *fg = NORMAL_COLOR;
}

#endif		/* !defined(MIDNIGHT) || defined(HAVE_SYNTAXH) */





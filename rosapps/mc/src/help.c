/* Hypertext file browser.
   Copyright (C) 1994, 1995 Miguel de Icaza.
   Copyright (C) 1994, 1995 Janne Kukonlehto
   
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

   Implements the hypertext file viewer.
   The hypertext file is a file that may have one or more nodes.  Each
   node ends with a ^D character and starts with a bracket, then the
   name of the node and then a closing bracket.

   Links in the hypertext file are specified like this: the text that
   will be highlighted should have a leading ^A, then it comes the
   text, then a ^B indicating that highlighting is done, then the name
   of the node you want to link to and then a ^C.

   The file must contain a ^D at the beginning and at the end of the
   file or the program will not be able to detect the end of file.

   Lazyness/widgeting attack: This file does use the dialog manager
   and uses mainly the dialog to achieve the help work.  there is only
   one specialized widget and it's only used to forward the mouse messages
   to the appropiate routine.
   
*/

#include <config.h>
#include "tty.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <malloc.h>
#include <errno.h>
#include "mad.h"
#include "color.h"
#include "util.h"
#include "dialog.h"
#include "win.h"
#include "global.h"
#include "mouse.h"
#include "key.h"	/* For mi_getch() */
#include "help.h"
#include "layout.h"		/* keybar_visible */
#include "x.h"
#include "dlg.h"		/* For Dlg_head */
#include "widget.h"		/* For Widget */

#ifdef HAVE_TK
#    include "tkmain.h"
#endif

#define MAXLINKNAME 80
#define HISTORY_SIZE 20
#define HELP_WINDOW_WIDTH 62

/* "$Id: help.c,v 1.1 2001/12/30 09:55:25 sedwards Exp $" */

static char *data;		/* Pointer to the loaded data file */
static int help_lines = 18;	/* Lines in help viewer */
static int  history_ptr;	/* For the history queue */
static char *main;		/* The main node */
static char *last_shown = 0;	/* Last byte shown in a screen */
static int end_of_node = 0;	/* Flag: the last character of the node shown? */
char *currentpoint, *startpoint;
static char *selected_item;

/* The widget variables */
static Dlg_head *whelp;

static struct {
    char *page;			/* Pointer to the selected page */
    char *link;			/* Pointer to the selected link */
} history [HISTORY_SIZE];

/* Link areas for the mouse */
typedef struct Link_Area {
    int x1, y1, x2, y2;
    char *link_name;
    struct Link_Area *next;
} Link_Area;

static Link_Area *link_area = NULL;
static int inside_link_area = 0;

static int help_callback (struct Dlg_head *h, int id, int msg);

#ifdef OS2_NT
struct {
    int acscode;
    int pccode;
} acs2pc_table [] = {
    { 'q',  0xC4 },
    { 'x',  0xB3 },
    { 'l',  0xDA },
    { 'k',  0xBF },
    { 'm',  0xC0 },
    { 'j',  0xD9 },
    { 'a',  0xB0 },
    { 'u',  0xB4 },
    { 't',  0xC3 },
    { 'w',  0xC2 },
    { 'v',  0xC1 },
    { 'n',  0xC5 },
    { 0, 0 } };

static int acs2pc (int acscode)
{
    int i;

    for (i = 0; acs2pc_table[i].acscode != 0; i++)
	if (acscode == acs2pc_table[i].acscode) {
	    return acs2pc_table[i].pccode;
	}
    return 0;
}
#endif

/* returns the position where text was found in the start buffer */
/* or 0 if not found */
char *search_string (char *start, char *text)
{
    char *d = text;
    char *e = start;

    /* fmt sometimes replaces a space with a newline in the help file */
    /* Replace the newlines in the link name with spaces to correct the situation */
    while (*d){
	if (*d == '\n')
	    *d = ' ';
	d++;
    }
    /* Do search */
    for (d = text; *e; e++){
	if (*d == *e)
	    d++;
	else
	    d = text;
	if (!*d)
	    return e+1;
    }
    return 0;
}

/* Searches text in the buffer pointed by start.  Search ends */
/* if the CHAR_NODE_END is found in the text.  Returns 0 on failure */
static char *search_string_node (char *start, char *text)
{
    char *d = text;
    char *e = start;

    if (!start)
	return 0;
    
    for (; *e && *e != CHAR_NODE_END; e++){
	if (*d == *e)
	    d++;
	else
	    d = text;
	if (!*d)
	    return e+1;
    }
    return 0;
}

/* Searches the_char in the buffer pointer by start and searches */
/* it can search forward (direction = 1) or backward (direction = -1) */
static char *search_char_node (char *start, char the_char, int direction)
{
    char *e;

    e = start;
    
    for (; *e && (*e != CHAR_NODE_END); e += direction){
	if (*e == the_char)
	    return e;
    }
    return 0;
}

/* Returns the new current pointer when moved lines lines */
static char *move_forward2 (char *c, int lines)
{
    char *p;
    int  line;

    currentpoint = c;
    for (line = 0, p = currentpoint; *p && *p != CHAR_NODE_END; p++){
	if (line == lines)
	    return currentpoint = p;
	if (*p == '\n')
	    line++;
    }
    return currentpoint = c;
}

static char *move_backward2 (char *c, int lines)
{
    char *p;
    int line;

    currentpoint = c;
    for (line = 0, p = currentpoint; *p && p >= data; p--){
	if (*p == CHAR_NODE_END)
	{
	    /* We reached the beginning of the node */
	    /* Skip the node headers */
	    while (*p != ']') p++;
	    return currentpoint = p + 2;
	}
	if (*(p - 1) == '\n')
	    line++;
	if (line == lines)
	    return currentpoint = p;
    }
    return currentpoint = c;
}

static void move_forward (int i)
{
    if (end_of_node)
	return;
    currentpoint = move_forward2 (currentpoint, i);
}

static void move_backward (int i)
{
    currentpoint = move_backward2 (currentpoint, ++i);
}

static void move_to_top (int dummy)
{
    while (currentpoint > data && *currentpoint != CHAR_NODE_END)
	currentpoint--;
    while (*currentpoint != ']')
	currentpoint++;
    currentpoint = currentpoint + 1;
    selected_item = NULL;
}

static void move_to_bottom (int dummy)
{
    while (*currentpoint && *currentpoint != CHAR_NODE_END)
	currentpoint++;
    currentpoint--;
    move_backward (help_lines - 1);
}

char *help_follow_link (char *start, char *selected_item)
{
    char link_name [MAXLINKNAME];
    char *p;
    int  i = 0;

    if (!selected_item)
	return start;
    
    for (p = selected_item; *p && *p != CHAR_NODE_END && *p != CHAR_LINK_POINTER; p++)
	;
    if (*p == CHAR_LINK_POINTER){
	link_name [0] = '[';
	for (i = 1; *p != CHAR_LINK_END && *p && *p != CHAR_NODE_END && i < MAXLINKNAME-3; )
	    link_name [i++] = *++p;
	link_name [i-1] = ']';
	link_name [i] = 0;
	p = search_string (data, link_name);
	if (p)
	    return p;
    }
    return _(" Help file format error\n\x4");	/*  */
}

static char *select_next_link (char *start, char *current_link)
{
    char *p;

    if (!current_link)
	return 0;

    p = search_string_node (current_link, STRING_LINK_END);
    if (!p)
	return NULL;
    p = search_string_node (p, STRING_LINK_START);
    if (!p)
	return NULL;
    return p - 1;
}

static char *select_prev_link (char *start, char *current_link)
{
    char *p;

    if (!current_link)
	return 0;
    
    p = current_link - 1;
    if (p <= start)
	return 0;
    
    p = search_char_node (p, CHAR_LINK_START, -1);
    return p;
}

static void start_link_area (int x, int y, char *link_name)
{
    Link_Area *new;

    if (inside_link_area)
	message (0, _(" Warning "), _(" Internal bug: Double start of link area "));

    /* Allocate memory for a new link area */
    new = (Link_Area*) xmalloc (sizeof (Link_Area), "Help, link_area");
    new->next = link_area;
    link_area = new;

    /* Save the beginning coordinates of the link area */
    link_area->x1 = x;
    link_area->y1 = y;

    /* Save the name of the destination anchor */
    link_area->link_name = link_name;

    inside_link_area = 1;
}

static void end_link_area (int x, int y)
{
    if (inside_link_area){
	/* Save the end coordinates of the link area */
	link_area->x2 = x;
	link_area->y2 = y;

	inside_link_area = 0;
    }
}

static void clear_link_areas (void)
{
    Link_Area *current;

    while (link_area){
	current = link_area;
	link_area = current -> next;
	free (current);
    }
    inside_link_area = 0;
}

static void show (Dlg_head *h, char *paint_start)
{
    char *p;
    int  col, line, c;
    int  painting = 1;
    int acs;			/* Flag: Alternate character set active? */
    int repeat_paint;
    int active_col, active_line;/* Active link position */

    do {
	
	line = col = acs = active_col = active_line = repeat_paint = 0;
    
	clear_link_areas ();
	if (selected_item < paint_start)
	    selected_item = NULL;
	
	for (p = paint_start; *p != CHAR_NODE_END && line < help_lines; p++){
	    c = *p;
	    switch (c){
	    case CHAR_LINK_START:
		if (selected_item == NULL)
		    selected_item = p;
		if (p == selected_item){
		    attrset (HELP_SLINK_COLOR);

		    /* Store the coordinates of the link */
		    active_col = col + 2;
		    active_line = line + 2;
		}
		else
		    attrset (HELP_LINK_COLOR);
		start_link_area (col, line, p);
		break;
	    case CHAR_LINK_POINTER:
		painting = 0;
		end_link_area (col - 1, line);
		break;
	    case CHAR_LINK_END:
		painting = 1;
		attrset (HELP_NORMAL_COLOR);
		break;
	    case CHAR_ALTERNATE:
		acs = 1;
		break;
	    case CHAR_NORMAL:
		acs = 0;
		break;
	    case CHAR_VERSION:
		dlg_move (h, line+2, col+2);
		addstr (VERSION);
		col += strlen (VERSION);
		break;
	    case CHAR_BOLD_ON:
		attrset (HELP_BOLD_COLOR);
		break;
	    case CHAR_ITALIC_ON:
		attrset (HELP_ITALIC_COLOR);
		break;
	    case CHAR_BOLD_OFF:
		attrset (HELP_NORMAL_COLOR);
		break;
	    case '\n':
		line++;
		col = 0;
		break;
	    case '\t':
		col = (col/8 + 1) * 8;
		break;
	    case CHAR_MCLOGO:
	    case CHAR_TEXTONLY_START:
	    case CHAR_TEXTONLY_END:
		break;
	    case CHAR_XONLY_START:
		while (*p && *p != CHAR_NODE_END && *p != CHAR_XONLY_END)
		    p++;
		if (*p == CHAR_NODE_END || !*p)
		    p--;
		break;
	    default:
		if (!painting)
		    continue;
		if (col > HELP_WINDOW_WIDTH-1)
		    continue;
		
		dlg_move (h, line+2, col+2);
		if (acs){
		    if (c == ' ' || c == '.')
			addch (c);
		    else
#ifndef OS2_NT
#ifndef HAVE_SLANG
			addch (acs_map [c]);
#else
			SLsmg_draw_object (h->y + line + 2, h->x + col + 2, c);
#endif
#else
			addch (acs2pc (c));
#endif /* OS2_NT */
		} else
		    addch (c);
		col++;
		break;
	    }
	}
	last_shown = p;
	end_of_node = line < help_lines;
	attrset (HELP_NORMAL_COLOR);
	if (selected_item >= last_shown){
	    if (link_area != NULL){
		selected_item = link_area->link_name;
		repeat_paint = 1;
	    }
	    else
		selected_item = NULL;
	}
    } while (repeat_paint);

    /* Position the cursor over a nice link */
    if (active_col)
	dlg_move (h, active_line, active_col);
}

static int help_event (Gpm_Event *event, Widget *w)
{
    Link_Area *current_area;

    if (! (event->type & GPM_UP))
	return 0;

    /* The event is relative to the dialog window, adjust it: */
    event->y -= 2;
    
    if (event->buttons & GPM_B_RIGHT){
	currentpoint = startpoint = history [history_ptr].page;
	selected_item = history [history_ptr].link;
	history_ptr--;
	if (history_ptr < 0)
	    history_ptr = HISTORY_SIZE-1;
	
	help_callback (w->parent, 0, DLG_DRAW);
	return 0;
    }

    /* Test whether the mouse click is inside one of the link areas */
    current_area = link_area;
    while (current_area)
    {
	/* Test one line link area */
	if (event->y == current_area->y1 && event->x >= current_area->x1 &&
	    event->y == current_area->y2 && event->x <= current_area->x2)
	    break;
	/* Test two line link area */
	if (current_area->y1 + 1 == current_area->y2){
	    /* The first line */
	    if (event->y == current_area->y1 && event->x >= current_area->x1)
		break;
	    /* The second line */
	    if (event->y == current_area->y2 && event->x <= current_area->x2)
		break;
	}
	/* Mouse will not work with link areas of more than two lines */

	current_area = current_area -> next;
    }

    /* Test whether a link area was found */
    if (current_area){
	/* The click was inside a link area -> follow the link */
	history_ptr = (history_ptr+1) % HISTORY_SIZE;
	history [history_ptr].page = currentpoint;
	history [history_ptr].link = current_area->link_name;
	currentpoint = startpoint = help_follow_link (currentpoint, current_area->link_name);
	selected_item = NULL;
    } else{
	if (event->y < 0)
	    move_backward (help_lines - 1);
	else if (event->y >= help_lines)
	    move_forward (help_lines - 1);
	else if (event->y < help_lines/2)
	    move_backward (1);
	else
	    move_forward (1);
    }

    /* Show the new node */
    help_callback (w->parent, 0, DLG_DRAW);

    return 0;
}

/* show help */
void help_help_cmd (Dlg_head *h)
{
    history_ptr = (history_ptr+1) % HISTORY_SIZE;
    history [history_ptr].page = currentpoint;
    history [history_ptr].link = selected_item;
    currentpoint = startpoint = search_string (data, "[How to use help]") + 1;
    selected_item = NULL;
#ifndef HAVE_XVIEW    
    help_callback (h, 0, DLG_DRAW);
#endif    
}

void help_index_cmd (Dlg_head *h)
{
    char *new_item;

    history_ptr = (history_ptr+1) % HISTORY_SIZE;
    history [history_ptr].page = currentpoint;
    history [history_ptr].link = selected_item;
    currentpoint = startpoint = search_string (data, "[Help]") + 1;

    if (!(new_item = search_string (data, "[Contents]")))
	message (1, MSG_ERROR, _(" Can't find node [Contents] in help file "));
    else
	currentpoint = startpoint = new_item + 1;
    selected_item = NULL;
#ifndef HAVE_XVIEW    
    help_callback (h, 0, DLG_DRAW);
#endif    
}

static void quit_cmd (void *x)
{
    Dlg_head *h = (Dlg_head *) x;
    
    dlg_stop (x);
}

static void prev_node_cmd (Dlg_head *h)
{
    currentpoint = startpoint = history [history_ptr].page;
    selected_item = history [history_ptr].link;
    history_ptr--;
    if (history_ptr < 0)
	history_ptr = HISTORY_SIZE-1;
    
#ifndef HAVE_XVIEW    
    help_callback (h, 0, DLG_DRAW);
#endif    
}

static int md_callback (Dlg_head *h, Widget *w, int msg, int par)
{
    return default_proc (h, msg, par);
}

static Widget *mousedispatch_new (int y, int x, int yl, int xl)
{
    Widget *w = xmalloc (sizeof (Widget), "disp_new");

    init_widget (w, y, x, yl, xl,
		 (callback_fn) md_callback, 0, (mouse_h)  help_event, NULL);

    return w;
}

static int help_handle_key (struct Dlg_head *h, int c)
{
    char *new_item;

    if (c != KEY_UP && c != KEY_DOWN &&
	check_movement_keys (c, 1, help_lines, currentpoint,
			     (movefn) move_backward2,
			     (movefn) move_forward2,
			     (movefn) move_to_top,
			     (movefn) move_to_bottom))
	/* Nothing */;
    else switch (c){
    case 'l':
    case KEY_LEFT:
	prev_node_cmd (h);
	break;
	
    case '\n':
    case KEY_RIGHT:
	/* follow link */
	if (!selected_item){
#ifdef WE_WANT_TO_GO_BACKWARD_ON_KEY_RIGHT
	    /* Is there any reason why the right key would take us
	     * backward if there are no links selected?, I agree
	     * with Torben than doing nothing in this case is better
	     */
	    /* If there are no links, go backward in history */
	    history_ptr--;
	    if (history_ptr < 0)
		history_ptr = HISTORY_SIZE-1;
	    
	    currentpoint = startpoint = history [history_ptr].page;
	    selected_item   = history [history_ptr].link;
#endif
	} else {
	    history_ptr = (history_ptr+1) % HISTORY_SIZE;
	    history [history_ptr].page = currentpoint;
	    history [history_ptr].link = selected_item;
	    currentpoint = startpoint = help_follow_link (currentpoint, selected_item) + 1;
	}
	selected_item = NULL;
	break;
	
    case KEY_DOWN:
    case '\t':
	/* select next link */
	new_item = select_next_link (startpoint, selected_item);
	if (new_item){
	    selected_item = new_item;
	    if (selected_item >= last_shown){
		if (c == KEY_DOWN)
		    move_forward (1);
		else
		    selected_item = NULL;
	    }
	} else if (c == KEY_DOWN)
	    move_forward (1);
	else
	    selected_item = NULL;
	break;
	
    case KEY_UP:
    case ALT ('\t'):
	/* select previous link */
	new_item = select_prev_link (startpoint, selected_item);
	selected_item = new_item;
	if (selected_item < currentpoint || selected_item >= last_shown){
	    if (c == KEY_UP)
		move_backward (1);
	    else{
		if (link_area != NULL)
		    selected_item = link_area->link_name;
		else
		    selected_item = NULL;
	    }
	}
	break;
	
    case 'n':
	/* Next node */
	new_item = currentpoint;
	while (*new_item && *new_item != CHAR_NODE_END)
	    new_item++;
	if (*++new_item == '['){
	    while (*new_item != ']')
		new_item++;
	    currentpoint = new_item + 2;
	    selected_item = NULL;
	}
	break;
	
    case 'p':
	/* Previous node */
	new_item = currentpoint;
	while (new_item > data + 1 && *new_item != CHAR_NODE_END)
	    new_item--;
	new_item--;
	while (new_item > data && *new_item != CHAR_NODE_END)
	    new_item--;
	while (*new_item != ']')
	    new_item++;
	currentpoint = new_item + 2;
	selected_item = NULL;
	break;
	
    case 'c':
	help_index_cmd (h);
	break;
	
    case ESC_CHAR:
    case XCTRL('g'):
	dlg_stop (h);
	break;

    default:
	return 0;
	    
    }
    help_callback (h, 0, DLG_DRAW);
    return 1;
}

static int help_callback (struct Dlg_head *h, int id, int msg)
{
    switch (msg){
    case DLG_DRAW:
	attrset (HELP_NORMAL_COLOR);
	dlg_erase (h);
	draw_box (h, 1, 1, help_lines+2, HELP_WINDOW_WIDTH+2);
	attrset (COLOR_HOT_NORMAL);
	dlg_move (h, 1, (HELP_WINDOW_WIDTH - 1) / 2);
	addstr (_(" Help "));
	attrset (HELP_NORMAL_COLOR);
	show (h, currentpoint);
	break;

    case DLG_KEY:
	return help_handle_key (h, id);
    }
    return 0;
}

void interactive_display_finish (void)
{
    clear_link_areas ();
    free (data);
}

void interactive_display (char *filename, char *node)
{
    WButtonBar *help_bar;
    Widget     *md;
    
    if ((data = load_file (filename)) == 0){
	message (1, MSG_ERROR, _(" Can't open file %s \n %s "),
		 filename, unix_error_string (errno));
	return;
    }
    if (!(main = search_string (data, node))){
	message (1, MSG_ERROR, _(" Can't find node %s in help file "), node);
	interactive_display_finish ();
	return;
    }

#ifndef HAVE_X
    if (help_lines > LINES - 4)
	help_lines = LINES - 4;

    whelp = create_dlg (0, 0, help_lines+4, HELP_WINDOW_WIDTH+4, dialog_colors,
			help_callback, "[Help]", "help", DLG_TRYUP|DLG_CENTER);

    /* allow us to process the tab key */
    whelp->raw = 1;

#endif    
    selected_item = search_string_node (main, STRING_LINK_START) - 1;
    currentpoint = startpoint = main + 1;

    for (history_ptr = HISTORY_SIZE; history_ptr;){
	history_ptr--;
	history [history_ptr].page = currentpoint;
	history [history_ptr].link = selected_item;
    }

#ifndef HAVE_X
    help_bar = buttonbar_new (keybar_visible);
    help_bar->widget.y -= whelp->y;
    help_bar->widget.x -= whelp->x;
    
    md       = mousedispatch_new (1, 1, help_lines, HELP_WINDOW_WIDTH-2);
    
    add_widget (whelp, help_bar);
    add_widget (whelp, md);

    define_label_data (whelp, (Widget *)NULL, 1, _("Help"),
		       (buttonbarfn) help_help_cmd, whelp);
    define_label_data (whelp, (Widget *)NULL, 2, _("Index"),
		       (buttonbarfn) help_index_cmd,whelp);
    define_label_data (whelp, (Widget *)NULL, 3, _("Prev"),
		       (buttonbarfn) prev_node_cmd, whelp);
    define_label (whelp, (Widget *) NULL, 4, "", 0);
    define_label (whelp, (Widget *) NULL, 5, "", 0);
    define_label (whelp, (Widget *) NULL, 6, "", 0);
    define_label (whelp, (Widget *) NULL, 7, "", 0);
    define_label (whelp, (Widget *) NULL, 8, "", 0);
    define_label (whelp, (Widget *) NULL, 9, "", 0);
    define_label_data (whelp, (Widget *) NULL, 10, _("Quit"), quit_cmd, whelp);

    run_dlg (whelp);
    interactive_display_finish ();
    destroy_dlg (whelp);
#else
    x_interactive_display ();
#endif
}


/* Panel layout module for the Midnight Commander
   Copyright (C) 1995 the Free Software Foundation
   
   Written: 1995 Janne Kukonlehto
            1995 Miguel de Icaza

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
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/param.h>		/* Required by tree.h */
#include <sys/types.h>
#include <sys/stat.h>
#if (!defined(__IBMC__) && !defined(__IBMCPP__)) && !defined(OS2_NT)
#    include <termios.h>
     /*
      * If TIOCGWINSZ supported, make it available here, because window-
      * resizing code depends on it...
      */
#    ifdef __QNX__              /* Maybe not only QNX-specific... */
#	include <sys/ioctl.h>
#    endif
#endif
#ifdef HAVE_UNISTD_H
#   include <unistd.h>
#endif
#include <signal.h>
#include "tty.h"
#include "mad.h"
#include "util.h"		/* Needed for the externs */
#include "win.h"
#include "color.h"
#include "key.h"

#include "dlg.h"
#include "widget.h"
#include "command.h"

#include "dialog.h"		/* For do_refresh() */
#include "profile.h"		/* For sync_profiles() */
#include "mouse.h"
#define WANT_WIDGETS
#include "main.h"
#include "subshell.h"	/* For use_subshell and resize_subshell() */
#include "tree.h"
#include "menu.h"

/* Needed for the extern declarations of integer parameters */
#include "dir.h"
#include "panel.h"		/* The Panel widget */
#include "file.h"
#include "cons.saver.h"
#include "layout.h"
#include "info.h"		/* The Info widget */
#include "view.h"		/* The view widget */

#define WANT_DEFAULTS
#include "setup.h"		/* For save_setup() */

#include "x.h"

/* "$Id: layout.c,v 1.1 2001/12/30 09:55:24 sedwards Exp $" */

/* Controls the display of the rotating dash on the verbose mode */
int nice_rotating_dash = 1;

/* If set, then we have to call the layout_change routine from main */
int layout_do_change = 0;

/* Set if the panels are split horizontally */
int horizontal_split = 0;

/* Set if the window has changed it's size */
int winch_flag = 0;

/* Set if the split is the same */
int equal_split = 1;

/* First panel size if the panel are not split equally */
int first_panel_size = 0;

/* The number of output lines shown (if available) */
int output_lines = 0;

/* Set if the command prompt is to be displayed */
int command_prompt = 1;

/* Set if the nice and usefull keybar is visible */
int keybar_visible = 1;

/* Set if the nice message (hint) bar is visible */
int message_visible = 1;

/* Set if you want the message bar shown in xterm title bar to save space */
int xterm_hintbar = 0;

/* The starting line for the output of the subprogram */
int output_start_y = 0;

/* The maximum number of views managed by the set_display_type routine */
/* Must be at least two (for current and other).  Please note that until */
/* Janne gets around this, we will only manage two of them :-) */
#define MAX_VIEWS 2

struct {
    int    type;
    Widget *widget;
} panels [MAX_VIEWS];

/* These variables are used to avoid updating the information unless */
/* we need it */
static int old_first_panel_size;
static int old_horizontal_split;
static int old_output_lines;

/* Internal variables */
static int _horizontal_split;
static int _equal_split;
static int _first_panel_size;
static int _menubar_visible;
static int _output_lines;
static int _command_prompt;
static int _keybar_visible;
static int _message_visible;
static int _xterm_hintbar;
static int _permission_mode;
static int _filetype_mode;

static int height;

#define MINWIDTH 10
#define MINHEIGHT 5

#define BY      12

#define B_2LEFT B_USER
#define B_2RIGHT B_USER + 1
#define B_PLUS B_USER + 2
#define B_MINUS B_USER + 3

static Dlg_head *layout_dlg;

static char *s_split_direction [2] = {
    N_("&Vertical"), 
    N_("&Horizontal")
};
WRadio *radio_widget;

static struct {
    char   *text;
    int    *variable;
    WCheck *widget;
    char   *tkname;
} check_options [] = {
    { N_("&Xterm hintbar"),    &xterm_hintbar,   0, "h" },
    { N_("h&Intbar visible"),  &message_visible, 0, "v" },
    { N_("&Keybar visible"),   &keybar_visible,  0, "k" },
    { N_("command &Prompt"),   &command_prompt,  0, "p" },
    { N_("show &Mini status"), &show_mini_info,  0, "m" },
    { N_("menu&Bar visible"),  &menubar_visible, 0, "me" },
    { N_("&Equal split"),      &equal_split,     0, "eq" },
    { N_("pe&Rmissions"),      &permission_mode, 0, "pr" },
    { N_("&File types"),       &filetype_mode,   0, "ft" },
    { 0, 0, 0, 0 }
};

static int first_width, second_width;
static char *layout_title, *title1, *title2, *title3, *output_lines_label;

static WButton *bleft_widget, *bright_widget;

static void _check_split (void)
{
    if (_horizontal_split){
	if (_equal_split)
	    _first_panel_size = height / 2;
	else if (_first_panel_size < MINHEIGHT)
	    _first_panel_size = MINHEIGHT;
	else if (_first_panel_size > height - MINHEIGHT)
	    _first_panel_size = height - MINHEIGHT;
    } else {
	if (_equal_split)
	    _first_panel_size = COLS / 2;
	else if (_first_panel_size < MINWIDTH)
	    _first_panel_size = MINWIDTH;
	else if (_first_panel_size > COLS - MINWIDTH)
	    _first_panel_size = COLS - MINWIDTH;
    }
}

static void update_split (void)
{
    /* Check split has to be done before testing if it changed, since
       it can change due to calling _check_split() as well*/
    _check_split ();
    
    /* To avoid setting the cursor to the wrong place */
    if ((old_first_panel_size == _first_panel_size) &&
	(old_horizontal_split == _horizontal_split)){
	return;
    }

    old_first_panel_size = _first_panel_size;
    old_horizontal_split = _horizontal_split; 
   
    attrset (COLOR_NORMAL);
    dlg_move (layout_dlg, 6, 6);
    printw ("%03d", _first_panel_size);
    dlg_move (layout_dlg, 6, 18);
    if (_horizontal_split)
	printw ("%03d", height - _first_panel_size);
    else
	printw ("%03d", COLS - _first_panel_size);
}

static int b2left_cback (int action, void *data)
{
    if (_equal_split){
	/* Turn equal split off */
	_equal_split = 0;
	check_options [6].widget->state = check_options [6].widget->state & ~C_BOOL;
	dlg_select_widget (layout_dlg, check_options [6].widget);
	dlg_select_widget (layout_dlg, bleft_widget);
    }
    _first_panel_size++;
    return 0;
}

static int b2right_cback (int action, void *data)
{
    if (_equal_split){
	/* Turn equal split off */
	_equal_split = 0;
	check_options [6].widget->state = check_options [6].widget->state & ~C_BOOL;
	dlg_select_widget (layout_dlg, check_options [6].widget);
	dlg_select_widget (layout_dlg, bright_widget);
    }
    _first_panel_size--;
    return 0;
}

static int bplus_cback (int action, void *data)
{
    if (_output_lines < 99)
	_output_lines++;
    return 0;
}

static int bminus_cback (int action, void *data)
{
    if (_output_lines > 0)
	_output_lines--;
    return 0;
}

static int layout_callback (struct Dlg_head *h, int Id, int Msg)
{
    switch (Msg){
    case DLG_DRAW:
#ifndef HAVE_X
    	/*When repainting the whole dialog (e.g. with C-l) we have to
    	  update everything*/
   	old_first_panel_size = -1;
    	old_horizontal_split = -1;
    	old_output_lines     = -1;
	attrset (COLOR_NORMAL);
	dlg_erase (h);
	draw_box (h, 1, 2, h->lines - 2, h->cols - 4);
	draw_box (h, 2, 4, 6, first_width);
	draw_box (h, 8, 4, 4, first_width);
	draw_box (h, 2, 5 + first_width, 10, second_width);

	attrset (COLOR_HOT_NORMAL);
	dlg_move (h, 1, (h->cols - strlen(layout_title))/2);
	addstr (layout_title);
	dlg_move (h, 2, 5);
	addstr (title1);
	dlg_move (h, 8, 5);
	addstr (title2);
	dlg_move (h, 2, 6 + first_width);
	addstr (title3);
	update_split ();
	dlg_move (h, 6, 13);
	addch ('=');
	if (console_flag){
	    if (old_output_lines != _output_lines){
		old_output_lines = _output_lines;
		attrset (COLOR_NORMAL);
		dlg_move (h, 9, 16 + first_width);
		addstr (output_lines_label);
		dlg_move (h, 9, 10 + first_width);
		printw ("%02d", _output_lines);
	    }
	}
#endif
	break;

    case DLG_POST_KEY:
	_filetype_mode = check_options [8].widget->state & C_BOOL;
	_permission_mode = check_options [7].widget->state & C_BOOL;
#ifndef HAVE_X
	_equal_split = check_options [6].widget->state & C_BOOL;
#endif
	_menubar_visible = check_options [5].widget->state & C_BOOL;
	_command_prompt = check_options [4].widget->state & C_BOOL;
	_keybar_visible = check_options [2].widget->state & C_BOOL;
	_message_visible = check_options [1].widget->state & C_BOOL;
	_xterm_hintbar = check_options [0].widget->state & C_BOOL;
	if (console_flag){
	    int minimum;
	    if (_output_lines < 0)
		_output_lines = 0;
	    height = LINES - _keybar_visible - _command_prompt -
		     _menubar_visible - _output_lines - _message_visible;
	    if (_message_visible && _xterm_hintbar && xterm_flag) height++;
	    minimum = MINHEIGHT * (1 + _horizontal_split);
	    if (height < minimum){
		_output_lines -= minimum - height;
		height = minimum;
	    }
	} else {
	    height = LINES - _keybar_visible - _command_prompt -
		_menubar_visible - _output_lines - _message_visible;
	    if (_message_visible && _xterm_hintbar && xterm_flag) height++;
	}
	if (_horizontal_split != radio_widget->sel){
	    _horizontal_split = radio_widget->sel;
	    if (_horizontal_split)
		_first_panel_size = height / 2;
	    else
		_first_panel_size = COLS / 2;
	}
	update_split ();
	if (console_flag){
	    if (old_output_lines != _output_lines){
		old_output_lines = _output_lines;
		attrset (COLOR_NORMAL);
		dlg_move (h, 9, 10 + first_width);
		printw ("%02d", _output_lines);
	    }
	}
	break;

    case DLG_END:
	break;
    }
    return 0;
}

static void init_layout (void)
{
    static int i18n_layt_flag = 0;
	static int b1, b2, b3;
    int i = sizeof (s_split_direction) / sizeof(char*) ;
	char* ok_button = _("&Ok");
	char* cancel_button = _("&Cancel");
	char* save_button = _("&Save");
    
    if (!i18n_layt_flag)
    {
		register int l1;
		
		first_width = 19; /* length of line with '<' '>' buttons */
		
		layout_title = _(" Layout ");
		title1 = _(" Panel split ");
		title2 = _(" Highlight... ");
		title3 = _(" Other options ");
		output_lines_label = _("output lines");
		
		while (i--)
		{
		    s_split_direction [i] = _(s_split_direction [i]);
			l1 = strlen (s_split_direction [i]) + 7;
			if (l1 > first_width)
				first_width = l1;
		}

		for (i = 0; i <= 8; i++)
		{
			check_options[i].text = _(check_options[i].text);
			l1 = strlen (check_options[i].text) + 7;
			if (l1 > first_width)
				first_width = l1;
		}

		l1 = strlen (title1) + 1;
		if (l1 > first_width)
			first_width = l1;
		
		l1 = strlen (title2) + 1;
		if (l1 > first_width)
			first_width = l1;
		

		second_width = strlen (title3) + 1;
		for (i = 0; i < 6; i++)
		{
			check_options[i].text = _(check_options[i].text);
			l1 = strlen (check_options[i].text) + 7;
			if (l1 > second_width)
				second_width = l1;
		}
		if (console_flag)
		{
			l1 = strlen (output_lines_label) + 13;
			if (l1 > second_width)
				second_width = l1;
		}

		/* 
		 * alex@bcs.zp.ua:
		 * To be completely correct, one need to check if layout_title
		 * does not exceed dialog length and total length of 3 buttons
		 * allows their placement in one row. But assuming this dialog
		 * is wide enough, I don't include such a tests.
		 *
		 * Now the last thing to do - properly space buttons...
		 */
		l1 = 11 + strlen (ok_button)   /* 14 - all brackets and inner space */
		 	+ strlen (save_button)     /* notice: it is 3 char less because */
			+ strlen (cancel_button);  /* of '&' char in button text */
		
		i = (first_width + second_width - l1) / 4;
		b1 = 5 + i;
		b2 = b1 + strlen(ok_button) + i + 6;
		b3 = b2 + strlen(save_button) + i + 4;

		i18n_layt_flag = 1;
    }

    layout_dlg = create_dlg (0, 0, 15, first_width + second_width + 9, 
		dialog_colors, layout_callback,
		"[Layout]", "layout", DLG_CENTER | DLG_GRID);
			     
    x_set_dialog_title (layout_dlg, _("Layout"));

    add_widgetl (layout_dlg,
		button_new (BY, b3, B_CANCEL, NORMAL_BUTTON, cancel_button, 0, 0, "c"), 
		XV_WLAY_RIGHTOF);
    add_widgetl (layout_dlg,
		button_new (BY, b2, B_EXIT, NORMAL_BUTTON, save_button, 0, 0, "s"),
		XV_WLAY_RIGHTOF);
    add_widgetl (layout_dlg,
		button_new (BY, b1, B_ENTER, DEFPUSH_BUTTON, ok_button, 0, 0, "o"),
		XV_WLAY_CENTERROW);
#ifndef HAVE_X
    if (console_flag){
	add_widget (layout_dlg,
		    button_new (9, 12 + first_width, B_MINUS, NARROW_BUTTON, "&-",
			bminus_cback, 0, NULL));
	add_widget (layout_dlg,
		    button_new (9, 7 + first_width, B_PLUS, NARROW_BUTTON, "&+", 
			bplus_cback, 0, NULL));
    }
#endif    

#define XTRACT(i) *check_options[i].variable, check_options[i].text, check_options[i].tkname

    for (i = 0; i < 6; i++){
	check_options [i].widget = check_new (8 - i, 7 + first_width, XTRACT(i));
	add_widgetl (layout_dlg, check_options [i].widget, XV_WLAY_BELOWCLOSE);
    }
#ifdef HAVE_XVIEW
    add_widgetl (layout_dlg, label_new (2, 7 + first_width, _("Other options"), "oo"),
        XV_WLAY_NEXTCOLUMN);
    add_widgetl (layout_dlg, label_new (8, 5, _("Highlight..."), "hl"),
        XV_WLAY_NEXTCOLUMN);
    add_widgetl (layout_dlg, label_new (2, 5, _("Panel split"), "ps"),
        XV_WLAY_NEXTCOLUMN);
#endif        
    check_options [8].widget = check_new (10, 6, XTRACT(8));
    add_widgetl (layout_dlg, check_options [8].widget, XV_WLAY_BELOWCLOSE);
    check_options [7].widget = check_new (9, 6, XTRACT(7));
    add_widgetl (layout_dlg, check_options [7].widget, XV_WLAY_BELOWCLOSE);

    _filetype_mode = filetype_mode;
    _permission_mode = permission_mode;
    _equal_split = equal_split;
    _menubar_visible = menubar_visible;
    _command_prompt = command_prompt;
    _keybar_visible = keybar_visible;
    _message_visible = message_visible;
    _xterm_hintbar = xterm_hintbar;
#ifndef HAVE_X
    bright_widget = button_new(6, 15, B_2RIGHT, NARROW_BUTTON, "&>", b2right_cback, 0, ">");
    add_widgetl (layout_dlg, bright_widget, XV_WLAY_RIGHTOF);
    bleft_widget = button_new (6, 9, B_2LEFT, NARROW_BUTTON, "&<", b2left_cback, 0, "<");
    add_widgetl (layout_dlg, bleft_widget, XV_WLAY_RIGHTOF);
    check_options [6].widget = check_new (5, 6, XTRACT(6));
#endif
    old_first_panel_size = -1;
    old_horizontal_split = -1;
    old_output_lines     = -1;
    
    _first_panel_size = first_panel_size;
    _output_lines = output_lines;
#ifndef HAVE_X
    add_widget (layout_dlg, check_options [6].widget);
    radio_widget = radio_new (3, 6, 2, s_split_direction, 1, "r");
    add_widget (layout_dlg, radio_widget);
    radio_widget->sel = horizontal_split;
#endif
}

void layout_change (void)
{
    setup_panels ();
    layout_do_change = 0;
#ifndef HAVE_X
    /* re-init the menu, because perhaps there was a change in the way 
	how the panel are split (horizontal/vertical). */
    done_menu();
    init_menu();
    menubar_arrange(the_menubar);
#endif
}

void layout_cmd (void)
{
    int result;
    int i;

    init_layout ();
    run_dlg (layout_dlg);
    result = layout_dlg->ret_value;

    if (result == B_ENTER || result == B_EXIT){
	for (i = 0; check_options [i].text; i++)
	    if (check_options [i].widget)
		*check_options [i].variable = check_options [i].widget->state & C_BOOL;
#ifndef HAVE_X		
	horizontal_split = radio_widget->sel;
	first_panel_size = _first_panel_size;
	output_lines = _output_lines;
	layout_do_change = 1;
#endif	
    }
    if (result == B_EXIT){
	save_layout ();
	sync_profiles ();
    }

    destroy_dlg (layout_dlg);
    if (layout_do_change)
	layout_change ();
}

static void check_split (void)
{
    if (horizontal_split){
	if (equal_split)
	    first_panel_size = height / 2;
	else if (first_panel_size < MINHEIGHT)
	    first_panel_size = MINHEIGHT;
	else if (first_panel_size > height - MINHEIGHT)
	    first_panel_size = height - MINHEIGHT;
    } else {
	if (equal_split)
	    first_panel_size = COLS / 2;
	else if (first_panel_size < MINWIDTH)
	    first_panel_size = MINWIDTH;
	else if (first_panel_size > COLS - MINWIDTH)
	    first_panel_size = COLS - MINWIDTH;
    }
}

int panel_event    (Gpm_Event *event, WPanel *panel);
int menu_bar_event (Gpm_Event *event, void *);
extern char *prompt;

#ifndef HAVE_X
#ifdef HAVE_SLANG
void init_curses ()
{
    extern int force_ugly_line_drawing;
    extern int SLtt_Has_Alt_Charset;
    SLtt_get_terminfo ();
#ifndef OS2_NT
    if (force_ugly_line_drawing)
	SLtt_Has_Alt_Charset = 0;
#endif
    SLsmg_init_smg ();
    do_enter_ca_mode ();
    init_colors ();
    keypad (stdscr, TRUE);
    nodelay (stdscr, FALSE);
}
#else
void init_curses (void)
{
    initscr();
    if (!status_using_ncurses)
	do_enter_ca_mode ();
    mc_raw_mode ();
    noecho ();
    keypad (stdscr, TRUE);
    nodelay (stdscr, FALSE);
    init_colors ();
}
#endif /* ! HAVE_SLANG */
void done_screen ()                                                      
{                                                                               
    if (!(quit & SUBSHELL_EXIT))                                                
	clr_scr ();                                                             
    reset_shell_mode ();                                                        
    mc_noraw_mode ();                                                           
    if (use_mouse_p)                                                            
	shut_mouse ();                                                          
    keypad (stdscr, FALSE);                                                     
}                                                                               
#else
void init_curses ()
{
}
void done_screen ()
{
}
#endif /* HAVE_X */

void panel_do_cols (int index)
{
    if (get_display_type (index) == view_listing)
	set_panel_formats ((WPanel *) panels [index].widget);
    else {
	panel_update_cols (panels [index].widget, frame_half);
    }
}

#ifdef HAVE_X
void
setup_panels (void)
{
    Widget *w = panels [0].widget;

    winput_set_origin (&cmdline->input, 0, 60);

    /* Only needed by the startup code */
    if (panels [0].type == view_listing){
	x_panel_set_size (0);
    }
    
    if (panels [1].type == view_listing){
	x_panel_set_size (1);
    }

    load_hint ();
#ifdef HAVE_XVIEW
    panel_do_cols (0);
    panel_do_cols (1);
#endif    
}

#else

void setup_panels (void)
{
    int start_y;
    int promptl;		/* the prompt len */

    if (console_flag){
	int minimum;
	if (output_lines < 0)
	    output_lines = 0;
	height = LINES - keybar_visible - command_prompt - menubar_visible
	         - output_lines - message_visible;
	if (message_visible && xterm_hintbar && xterm_flag) height++;
	minimum = MINHEIGHT * (1 + horizontal_split);
	if (height < minimum){
	    output_lines -= minimum - height;
	    height = minimum;
	}
    } else {
	height = LINES - menubar_visible - command_prompt -
	    keybar_visible - message_visible;
	if (message_visible && xterm_hintbar && xterm_flag) height++;
    }
    check_split ();
    start_y = menubar_visible;

    /* The column computing is defered until panel_do_cols */
    if (horizontal_split){
	widget_set_size (panels [0].widget, start_y, 0, 
			 first_panel_size, 0);
			
	widget_set_size (panels [1].widget, start_y+first_panel_size, 0,
			 height-first_panel_size, 0);
    } else {
	int first_x = first_panel_size;

	widget_set_size (panels [0].widget, start_y, 0,
			 height, 0);

	widget_set_size (panels [1].widget, start_y, first_x,
			 height, 0);
			
    }
    panel_do_cols (0);
    panel_do_cols (1);
    
    promptl = strlen (prompt);

    widget_set_size (&the_menubar->widget, 0, 0, 1, COLS);

    if (command_prompt) {
	    widget_set_size (&cmdline->input.widget,
			     LINES-1-keybar_visible, promptl,
			     1, COLS-promptl-(keybar_visible ? 0 : 1));
	    winput_set_origin (&cmdline->input, promptl, COLS-promptl-(keybar_visible ? 0 : 1));
	    widget_set_size (&the_prompt->widget,
			     LINES-1-keybar_visible, 0,
			     1, promptl);
    } else {
	    widget_set_size (&cmdline->input.widget, 0, 0, 0, 0);
	    winput_set_origin (&cmdline->input, 0, 0);
	    widget_set_size (&the_prompt->widget, LINES, COLS, 0, 0);
    }			     

    widget_set_size (&the_bar->widget, LINES-1, 0, 1, COLS);
    the_bar->visible = keybar_visible;
    
    /* Output window */
    if (console_flag && output_lines){
	output_start_y = LINES -command_prompt-keybar_visible-
	    output_lines;
	show_console_contents (output_start_y,
			       LINES-output_lines-keybar_visible-1,
			       LINES-keybar_visible-1);
    } 
    if (message_visible && (!xterm_hintbar || !xterm_flag))
	widget_set_size (&the_hint->widget, height+start_y, 0, 1,COLS);
    else
	widget_set_size (&the_hint->widget, 0, 0, 0, 0);
    
    load_hint ();
}
#endif

void flag_winch (int dummy)
{
    winch_flag = 1;
}

void edit_adjust_size (Dlg_head * h);

#ifdef PORT_NEEDS_CHANGE_SCREEN_SIZE
void low_level_change_screen_size (void)
{
#if defined(HAVE_SLANG) || NCURSES_VERSION_MAJOR >= 4
#if defined TIOCGWINSZ && !defined SCO_FLAVOR
    struct winsize winsz;

    winsz.ws_col = winsz.ws_row = 0;
    /* Ioctl on the STDIN_FILENO */
    ioctl (0, TIOCGWINSZ, &winsz);
    if (winsz.ws_col && winsz.ws_row){
#if defined(NCURSES_VERSION) && defined(HAVE_RESIZETERM)
	resizeterm(winsz.ws_row, winsz.ws_col);
	clearok(stdscr,TRUE);	/* FIXME: sigwinch's should use a semaphore! */
#else
	COLS = winsz.ws_col;
	LINES = winsz.ws_row;
#endif
#ifdef HAVE_SUBSHELL_SUPPORT
	    resize_subshell ();
#endif
    }
#endif /* TIOCGWINSZ && !SCO_FLAVOR */
#endif /* defined(HAVE_SLANG) || NCURSES_VERSION_MAJOR >= 4 */
}

void change_screen_size (void)
{
#if defined(HAVE_SLANG) || NCURSES_VERSION_MAJOR >= 4
#if defined TIOCGWINSZ && !defined SCO_FLAVOR
    extern Dlg_head *view_dlg;
    extern Dlg_head *edit_dlg;
    
#ifndef NCURSES_VERSION
    mc_noraw_mode ();
    endwin ();
#endif
    low_level_change_screen_size ();
    check_split ();
#ifndef NCURSES_VERSION
    /* XSI Curses spec states that portable applications shall not invoke
     * initscr() more than once.  This kludge could be done within the scope
     * of the specification by using endwin followed by a refresh (in fact,
     * more than one curses implementation does this); it is guaranteed to work
     * only with slang.
     */
    init_curses ();
#endif
    setup_panels ();
    if (current_dlg == view_dlg)
	view_adjust_size (view_dlg);
#ifdef USE_INTERNAL_EDIT
    if (current_dlg == edit_dlg)
	edit_adjust_size (edit_dlg);
#endif
    
#ifdef RESIZABLE_MENUBAR
	menubar_arrange(the_menubar);
#endif
		
    /* Now, force the redraw */
    do_refresh ();
    touchwin (stdscr);
#endif /* TIOCGWINSZ && !SCO_FLAVOR */
#endif /* defined(HAVE_SLANG) || NCURSES_VERSION_MAJOR >= 4 */
    winch_flag = 0;
}
#endif /* HAVE_X */

extern int verbose;
static int ok_to_refresh = 1;

void use_dash (int flag)
{
    if (flag)
	ok_to_refresh++;
    else
	ok_to_refresh--;
}

void set_hintbar(char *str) 
{
#ifndef HAVE_X
    if (xterm_flag && xterm_hintbar) {
        fprintf (stderr, "\33]0;mc - %s\7", str);
    } else
#endif
    {
        label_set_text (the_hint, str);
        if (ok_to_refresh > 0)
	    refresh();
    }
}

void print_vfs_message(char *msg, ...)
{
    va_list ap;
    char str[128];

    va_start(ap, msg);
    vsprintf(str, msg, ap);
    va_end(ap);
    if (midnight_shutdown || !the_hint || !the_hint->widget.parent)
	return;
    
    if (message_visible || (xterm_flag && xterm_hintbar)) {
        set_hintbar(str);
    }
}

void rotate_dash (void)
{
#ifndef HAVE_X
    static char rotating_dash [] = "|/-\\";
    static int pos = 0;

    if (!nice_rotating_dash || (ok_to_refresh <= 0))
	return;

    if (pos >= sizeof (rotating_dash)-1)
	pos = 0;
    move (0, COLS-1);
    addch (rotating_dash [pos]);
    mc_refresh ();
    pos++;
#endif
}

void remove_dash (void)
{
#ifndef HAVE_X
    if (!nice_rotating_dash)
	return;

    /* Currently, it's much nicer with the CPU to do this instead of
       calling do_refresh.

       I should implement a routine called invalidate_region that would
       send a draw message only to the affected views.  But for now
       this is fine.
    */
    
    move (0, COLS-1);
    addch (' ');
#endif    
}

char *get_nth_panel_name (int num)
{
    static char buffer [20];
    
    if (!num)
        return "New Left Panel";
    else if (num == 1)
        return "New Right Panel";
    else {
        sprintf (buffer, "%ith Panel", num);
        return buffer;
    }
}

/* I wonder if I should start to use the folding mode than Dugan uses */
/*                                                                     */
/* This is the centralized managing of the panel display types         */
/* This routine takes care of destroying and creating new widgets      */
/* Please note that it could manage MAX_VIEWS, not just left and right */
/* Currently nothing in the code takes advantage of this and has hard- */
/* coded values for two panels only                                    */

/* Set the num-th panel to the view type: type */
/* This routine also keeps at least one WPanel object in the screen */
/* since a lot of routines depend on the current_panel variable */
void set_display_type (int num, int type)
{
    int x, y, cols, lines;
    int    the_other;		/* Index to the other panel */
    char   *file_name = 0;	/* For Quick view */
    Widget *new_widget, *old_widget;
    WPanel  *the_other_panel;

    x =y = cols = lines = 0;
    old_widget = 0;
    if (num >= MAX_VIEWS){
	fprintf (stderr, "Could not allocate more that %d views\n", MAX_VIEWS);
	abort ();
    }

    /* Check that we will have a WPanel * at least */
    the_other = 0;
    if (type != view_listing){
	the_other = num == 0 ? 1 : 0;

	if (panels [the_other].type != view_listing)
	    return;

    }
    
    /* Get rid of it */
    if (panels [num].widget){
	Widget *w = panels [num].widget;
	WPanel *panel = (WPanel *) panels [num].widget;
	
	x = w->x;
	y = w->y;
	cols  = w->cols;
	lines = w->lines;
	old_widget = panels [num].widget;

	if (panels [num].type == view_listing){
	    if (panel->frame_size == frame_full && type != view_listing){
		cols = COLS - first_panel_size;
		if (num == 1)
		    x = first_panel_size;
	    }
	}
#ifdef HAVE_TK
	tk_evalf ("container_clean %s", panel->widget.wcontainer);
#endif
    }

    new_widget = 0;
    
    switch (type){
    case view_listing:
	new_widget = (Widget *) panel_new (get_nth_panel_name (num));
	break;
	
    case view_info:
	new_widget = (Widget *) info_new ();
	
	break;

    case view_tree:
	new_widget = (Widget *) tree_new (1, 0, 0, 0, 0);
	break;

    case view_quick:
	new_widget = (Widget *) view_new (0, 0, 0, 0, 1);
	the_other_panel = (WPanel *) panels [the_other].widget;
	if (the_other_panel)
	    file_name =
		the_other_panel->dir.list[the_other_panel->selected].fname;
	else
	    file_name = "";
	
	view_init ((WView *) new_widget, 0, file_name, 0);
	break;
    }
    panels [num].type = type;
    panels [num].widget = (Widget *) new_widget;
    
    /* We set the same size the old widget had */
    widget_set_size ((Widget *) new_widget, y, x, lines, cols);
    
    /* We wanna the new widget at the same position */
    /* XView sets wcontainer to !0 <- Not XView, but we, when we create it */
    /* Ok, the XView support code does it */
    if (old_widget && old_widget->wcontainer){
	new_widget->wcontainer = old_widget->wcontainer;
	new_widget->area = old_widget->area;
    }

    /* We use replace to keep the circular list of the dialog in the */
    /* same state.  Maybe we could just kill it and then replace it  */
    if (midnight_dlg && old_widget){
	dlg_replace_widget (midnight_dlg, old_widget, panels [num].widget);
    }
    if (type == view_listing){
	if (num == 0)
	    left_panel = (WPanel *) new_widget;
	else
	    right_panel = (WPanel *) new_widget;
    }

    if (type == view_tree)
	the_tree = (WTree *) new_widget;

    /* Prevent current_panel's value from becoming invalid.
     * It's just a quick hack to prevent segfaults. Comment out and
     * try following:
     * - select left panel
     * - invoke menue left/tree
     * - as long as you stay in the left panel almost everything that uses
     *   cpanel causes segfault, e.g. C-Enter, C-x c, ...
     */

    if (type != view_listing)
	if (cpanel == (WPanel *) old_widget)
	    current_panel = num == 0 ? right_panel : left_panel;
}

#ifndef HAVE_XVIEW
/* This routine is deeply sticked to the two panels idea.
   What should it do in more panels. ANSWER - don't use it
   in any multiple panels environment. */
void swap_panels ()
{
    Widget tmp;
    Widget *tmp_widget;
    WPanel panel;
    WPanel *panel1, *panel2;
    int tmp_type;
    
#if 0
#ifdef HAVE_PORTABLE_TOKEN_PASTING
#define panelswap(e) panel.##e = panel1->##e; panel1->##e = panel2->##e; panel2->##e = panel.##e;
#define panelswapstr(e) strcpy (panel.##e, panel1->##e); strcpy (panel1->##e, panel2->##e); strcpy (panel2->##e, panel.##e);
#else
#define panelswap(e) panel./**/e = panel1->/**/e; panel1->/**/e = panel2->/**/e; panel2->/**/e = panel./**/e;
#define panelswapstr(e) strcpy (panel./**/e, panel1->/**/e); strcpy (panel1->/**/e, panel2->/**/e); strcpy (panel2->/**/e, panel./**/e);
#endif
#endif

#define panelswap(x) panel. x = panel1-> x; panel1-> x = panel2-> x; panel2-> x = panel. x;

#define panelswapstr(e) strcpy (panel. e, panel1-> e); \
                        strcpy (panel1-> e, panel2-> e); \
                        strcpy (panel2-> e, panel. e);
    panel1 = (WPanel *) panels [0].widget;
    panel2 = (WPanel *) panels [1].widget;
    if (panels [0].type == view_listing && panels [1].type == view_listing) {
        /* Change everything except format/sort/panel_name etc. */
        panelswap (dir);
        panelswap (active);
        panelswapstr (cwd);
        panelswapstr (lwd);
        panelswap (count);
        panelswap (marked);
        panelswap (dirs_marked);
        panelswap (total);
        panelswap (top_file);
        panelswap (selected);
        panelswap (is_panelized);
        panelswap (dir_stat);
	
        panel1->searching = 0;
        panel2->searching = 0;
        if (cpanel == panel1)
            current_panel = panel2;
        else
            current_panel = panel1;
        if (midnight_dlg->current->widget == panels [0].widget)
            dlg_select_widget (midnight_dlg, (void *) panels [1].widget);
        else if (midnight_dlg->current->widget == panels [1].widget)
            dlg_select_widget (midnight_dlg, (void *) panels [0].widget);
    } else {
	WPanel *tmp_panel;
	
	tmp_panel=right_panel;
	right_panel=left_panel;
	left_panel=tmp_panel;
	
	if (panels [0].type == view_listing) {
            if (!strcmp (panel1->panel_name, get_nth_panel_name (0))) {
                free (panel1->panel_name);
                panel1->panel_name = strdup (get_nth_panel_name (1));
            }
        }
        if (panels [1].type == view_listing) {
            if (!strcmp (panel2->panel_name, get_nth_panel_name (1))) {
                free (panel2->panel_name);
                panel2->panel_name = strdup (get_nth_panel_name (0));
            }
        }
        
        tmp.x = panels [0].widget->x;
        tmp.y = panels [0].widget->y;
        tmp.cols = panels [0].widget->cols;
        tmp.lines = panels [0].widget->lines;

        panels [0].widget->x = panels [1].widget->x;
        panels [0].widget->y = panels [1].widget->y;
        panels [0].widget->cols = panels [1].widget->cols;
        panels [0].widget->lines = panels [1].widget->lines;

        panels [1].widget->x = tmp.x;
        panels [1].widget->y = tmp.y;
        panels [1].widget->cols = tmp.cols;
        panels [1].widget->lines = tmp.lines;
        
        tmp_widget = panels [0].widget;
        panels [0].widget = panels [1].widget;
        panels [1].widget = tmp_widget;
        tmp_type = panels [0].type;
        panels [0].type = panels [1].type;
        panels [1].type = tmp_type;
    }
}
#endif

int get_display_type (int index)
{
    return panels [index].type;
}

Widget *get_panel_widget (int index)
{
    return panels [index].widget;
}

int get_current_index (void)
{
    if (panels [0].widget == ((Widget *) cpanel))
	return 0;
    else
	return 1;
}

int get_other_index (void)
{
    return !get_current_index ();
}

/* Returns the view type for the current panel/view */
int get_current_type (void)
{
    if (panels [0].widget == (Widget *) cpanel)
	return panels [0].type;
    else
	return panels [1].type;
}

/* Returns the view type of the unselected panel */
int get_other_type (void)
{
    if (panels [0].widget == (Widget *) cpanel)
	return panels [1].type;
    else
	return panels [0].type;
}


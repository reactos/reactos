/* Some misc dialog boxes for the program.
   
   Copyright (C) 1994, 1995 the Free Software Foundation
   
   Authors: 1994, 1995 Miguel de Icaza
            1995 Jakub Jelinek
   
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
#include "tty.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <malloc.h>
#include <signal.h>
#include <ctype.h>
#include "global.h"
#include "mad.h"		/* The great mad */
#include "util.h"		/* Required by panel.h */
#include "win.h"		/* Our window tools */
#include "color.h"		/* Color definitions */
#include "dlg.h"		/* The nice dialog manager */
#include "widget.h"		/* The widgets for the nice dialog manager */
#include "dialog.h"		/* For do_refresh() */
#include "wtools.h"
#include "setup.h"		/* For profile_name */
#include "profile.h"		/* Load/save user formats */
#include "key.h"		/* XCTRL and ALT macros  */
#include "command.h"		/* For cmdline */
#include "dir.h"
#include "panel.h"
#include "boxes.h"
#include "main.h"		/* For the confirm_* variables */
#include "tree.h"
#include "layout.h"		/* for get_nth_panel_name proto */
#include "background.h"		/* for background definitions */
#include "x.h"

static int DISPLAY_X = 45, DISPLAY_Y = 14;

static Dlg_head *dd;
static WRadio *my_radio;
static WInput *user;
static WInput *status;
static WCheck *check_status;
static int current_mode;
extern int ftpfs_always_use_proxy;

static char **displays_status;
static char* display_title = N_(" Listing mode ");

/* Controls whether the array strings have been translated */
static int i18n_displays_flag;
static char *displays [LIST_TYPES] = {
    N_("&Full file list"),
    N_("&Brief file list"),
    N_("&Long file list"),
    N_("&User defined:"),
    N_("&Icon view"),
};

static int user_hotkey = 'u';

static int display_callback (struct Dlg_head *h, int id, int Msg)
{
#ifndef HAVE_X
    char *text;
    WInput *input;
    
    switch (Msg){
    case DLG_DRAW:
	attrset (COLOR_NORMAL);
	dlg_erase (h);
	draw_box (h, 1, 2, h->lines - 2, h->cols - 4);

	attrset (COLOR_HOT_NORMAL);
	dlg_move (h, 1, (h->cols - strlen(display_title))/2);
	addstr (display_title);
	attrset (COLOR_NORMAL);
	break;

    case DLG_UNFOCUS:
	if((WRadio *) h->current->widget == my_radio){
	    assign_text (status, displays_status [my_radio->sel]);
	    input_set_point (status, 0);
	}
	break;
	
    case DLG_KEY:
	if (id == '\n'){
	    if((WRadio *) h->current->widget == my_radio){
		assign_text (status, displays_status [my_radio->sel]);
		dlg_stop (h);
		break;
	    }
	    
	    if ((WInput *) h->current->widget == user){
		h->ret_value = B_USER + 6;
		dlg_stop (h);
		break;
	    }
	
	    if ((WInput *) h->current->widget == status){
		h->ret_value = B_USER + 7;
		dlg_stop (h);
		break;
	    }
	}

	if (tolower(id) == user_hotkey && h->current->widget != (Widget *) user
	    && h->current->widget != (Widget *) status){
	    my_radio->sel = 3;
	    dlg_select_widget (h, my_radio); /* force redraw */
	    dlg_select_widget (h, user);
	    return MSG_HANDLED;
	}
    }
#endif    
    return MSG_NOT_HANDLED;
}

static void display_init (int radio_sel, char *init_text,
			  int _check_status, char ** _status)
{
	char* user_mini_status = _("user &Mini status");
	char* ok_button = _("&Ok");
	char* cancel_button = _("&Cancel");
	
	static int button_start = 30;
	
    displays_status = _status;

    if (!i18n_displays_flag){
		int i, l, maxlen = 0;
		char* cp;

 		display_title = _(display_title);
		for (i = 0; i < LIST_TYPES; i++)
		{
		    displays [i] = _(displays [i]);
			if ((l = strlen(displays [i])) > maxlen)
				maxlen = l;
		}

		i = strlen (ok_button) + 5;
		l = strlen (cancel_button) + 3;
		l = max(i, l);

		i = maxlen + l + 16;
		if (i > DISPLAY_X)
			DISPLAY_X = i;

		i = strlen (user_mini_status) + 13;
		if (i > DISPLAY_X)
			DISPLAY_X = i;
			
		i = strlen (display_title) + 8;
		if (i > DISPLAY_X)
			DISPLAY_X = i;

		button_start = DISPLAY_X - l - 5;
		
		/* get hotkey of user-defined format string */
		cp = strchr(displays[LIST_TYPES-1],'&');
		if (cp != NULL && *++cp != '\0')
			user_hotkey = tolower(*cp);

        i18n_displays_flag = 1;
    }
    dd = create_dlg (0, 0, DISPLAY_Y, DISPLAY_X, dialog_colors,
		     display_callback, "[Left and Right Menus]", "display",
		     DLG_CENTER | DLG_GRID);

    x_set_dialog_title (dd, _("Listing mode"));
    add_widgetl (dd,
        button_new (4, button_start, B_CANCEL, 
			NORMAL_BUTTON, cancel_button, 0, 0, "cancel-button"),
	XV_WLAY_RIGHTOF);

    add_widgetl (dd,
		button_new (3, button_start, B_ENTER, 
			DEFPUSH_BUTTON, ok_button, 0, 0, "ok-button"),
	 XV_WLAY_CENTERROW);

    status = input_new (10, 9, INPUT_COLOR, DISPLAY_X-14, _status [radio_sel], "mini-input");
    add_widgetl (dd, status, XV_WLAY_RIGHTDOWN);
    input_set_point (status, 0);

    check_status = check_new (9, 5, _check_status, user_mini_status, "mini-status");
    add_widgetl (dd, check_status, XV_WLAY_NEXTROW);
    
    user = input_new  (7, 9, INPUT_COLOR, DISPLAY_X-14, init_text, "user-fmt-input");
    add_widgetl (dd, user, XV_WLAY_RIGHTDOWN);
    input_set_point (user, 0);

#ifdef PORT_HAS_ICON_VIEW
    my_radio = radio_new (3, 5, LIST_TYPES, displays, 1, "radio");
#else
    my_radio = radio_new (3, 5, LIST_TYPES-1, displays, 1, "radio");
#endif
    my_radio->sel = my_radio->pos = current_mode;
    add_widgetl (dd, my_radio, XV_WLAY_BELOWCLOSE);
}

int display_box (WPanel *panel, char **userp, char **minip, int *use_msformat,
    int num)
{
    int result, i;
    char *section = NULL;
    char *p;

    if (!panel) {
        p = get_nth_panel_name (num);
        panel = (WPanel *) xmalloc (sizeof (WPanel), "temporary panel");
        panel->list_type = list_full;
        panel->user_format = strdup (DEFAULT_USER_FORMAT);
        panel->user_mini_status = 0;
	for (i = 0; i < LIST_TYPES; i++)
    	    panel->user_status_format[i] = strdup (DEFAULT_USER_FORMAT);
        section = copy_strings ("Temporal:", p, 0);
        if (!profile_has_section (section, profile_name)) {
            free (section);
            section = strdup (p);
        }
        panel_load_setup (panel, section);
        free (section);
    }

    current_mode = panel->list_type;
    display_init (current_mode, panel->user_format, 
	panel->user_mini_status, panel->user_status_format);
		  
    run_dlg (dd);

    result = -1;
    
    if (section) {
        free (panel->user_format);
	for (i = 0; i < LIST_TYPES; i++)
	    free (panel->user_status_format [i]);
        free (panel);
    }
    
    if (dd->ret_value != B_CANCEL){
	result = my_radio->sel;
	*userp = strdup (user->buffer);
	*minip = strdup (status->buffer);
	*use_msformat = check_status->state & C_BOOL;
    }
    destroy_dlg (dd);

    return result;
}

int SORT_X = 40, SORT_Y = 14;

char *sort_orders_names [SORT_TYPES];

sortfn *sort_box (sortfn *sort_fn, int *reverse, int *case_sensitive)
{
    int i, r, l;
    sortfn *result;
    WCheck *c, *case_sense;

	char* ok_button = _("&Ok");
	char* cancel_button = _("&Cancel");
	char* reverse_label = _("&Reverse");
	char* case_label = _("case sensi&tive");
	char* sort_title = _("Sort order");

	static int i18n_sort_flag = 0, check_pos = 0, button_pos = 0;

	if (!i18n_sort_flag)
	{
		int maxlen = 0;
		for (i = SORT_TYPES-1; i >= 0; i--)
		{
			sort_orders_names [i] = _(sort_orders [i].sort_name);
			r = strlen (sort_orders_names [i]);
			if (r > maxlen)
				maxlen = r;
		}

		check_pos = maxlen + 9;

		r = strlen (reverse_label) + 4;
		i = strlen (case_label) + 4;
		if (i > r)
			r = i;
		
		l = strlen (ok_button) + 6;
		i = strlen (cancel_button) + 4;
		if (i > l)
			l = i;
			
		i = check_pos + max(r,l) + 2;

		if (i > SORT_X)
			SORT_X = i;

		i = strlen (sort_title) + 6;
		if (i > SORT_X)
			SORT_X = i;

		button_pos = SORT_X - l - 2;

		i18n_sort_flag = 1;
	}

    result = 0;
    
    for (i = 0; i < SORT_TYPES; i++)
	if ((sortfn *) (sort_orders [i].sort_fn) == sort_fn){
	    current_mode = i;
	    break;
	}
    
    dd = create_dlg (0, 0, SORT_Y, SORT_X, dialog_colors, common_dialog_callback,
		     "[Left and Right Menus]", "sort", DLG_CENTER | DLG_GRID);
		     
    x_set_dialog_title (dd, sort_title);

    add_widgetl (dd, 
		button_new (10, button_pos, B_CANCEL, NORMAL_BUTTON, cancel_button, 
		0, 0, "cancel-button"), XV_WLAY_CENTERROW);

    add_widgetl (dd, 
		button_new (9, button_pos, B_ENTER, DEFPUSH_BUTTON, ok_button,
		0, 0, "ok-button"),	XV_WLAY_RIGHTDOWN);

    case_sense = check_new (4, check_pos, *case_sensitive, case_label, "case-check");
    add_widgetl (dd, case_sense, XV_WLAY_RIGHTDOWN);
    c = check_new (3, check_pos, *reverse, reverse_label, "reverse-check");
    add_widgetl (dd, c, XV_WLAY_RIGHTDOWN);

    my_radio = radio_new (3, 3, SORT_TYPES, sort_orders_names, 1, "radio-1");
    my_radio->sel = my_radio->pos = current_mode;
    
    add_widget (dd, my_radio);
    run_dlg (dd);

    r = dd->ret_value;
    if (r != B_CANCEL){
	result = (sortfn *) sort_orders [my_radio->sel].sort_fn;
	*reverse = c->state & C_BOOL;
	*case_sensitive = case_sense->state & C_BOOL;
    } else
	result = sort_fn;
    destroy_dlg (dd);

    return result;
}

#define CONFY 10
#define CONFX 46

static int my_delete;
static int my_overwrite;
static int my_execute;
static int my_exit;

static QuickWidget conf_widgets [] = {
{ quick_button,   4, 6, 4, CONFY, N_("&Cancel"),
      0, B_CANCEL, 0, 0, XV_WLAY_RIGHTOF, "c" },
{ quick_button,   4, 6, 3, CONFY, N_("&Ok"),
      0, B_ENTER, 0, 0, XV_WLAY_CENTERROW, "o" },

{ quick_checkbox, 1, 13, 6, CONFY, N_(" confirm &Exit "),
      9, 0, &my_exit, 0, XV_WLAY_BELOWCLOSE, "e" },
{ quick_checkbox, 1, 13, 5, CONFY, N_(" confirm e&Xecute "),
      10, 0, &my_execute, 0, XV_WLAY_BELOWCLOSE, "x" },
{ quick_checkbox, 1, 13, 4, CONFY, N_(" confirm o&Verwrite "),
      10, 0, &my_overwrite, 0, XV_WLAY_BELOWCLOSE, "ov" },
{ quick_checkbox, 1, 13, 3, CONFY, N_(" confirm &Delete "),
      9, 0, &my_delete, 0, XV_WLAY_BELOWCLOSE, "de" },
{ 0,              0, 0, 0, 0, 0, 0, 0, 0, 0, 0, XV_WLAY_DONTCARE }
};

static QuickDialog confirmation =
{ CONFX, CONFY, -1, -1, N_(" Confirmation "), "[Confirmation]", "quick_confirm",
      conf_widgets, 0 };

void confirm_box ()
{

#ifdef ENABLE_NLS
	static int i18n_flag = 0;
	
	if (!i18n_flag)
	{
		register int i = sizeof(conf_widgets)/sizeof(QuickWidget) - 1;
		int l1, maxlen = 0;
		while (i--)
		{
			conf_widgets [i].text = _(conf_widgets [i].text);
			l1 = strlen (conf_widgets [i].text) + 3;
			if (l1 > maxlen)
				maxlen = l1;
		}

		/*
		 * If buttons start on 4/6, checkboxes (with some add'l space)
		 * must take not more than it.
		 */
		confirmation.xlen = (maxlen + 5) * 6 / 4;

		/*
		 * And this for the case when buttons with some space to the right
		 * do not fit within 2/6
		 */
		l1 = strlen (conf_widgets [0].text) + 3;
		i = strlen (conf_widgets [1].text) + 5;
		if (i > l1)
			l1 = i;

		i = (l1 + 3) * 6 / 2;
		if (i > confirmation.xlen)
			confirmation.xlen = i;

		confirmation.title = _(confirmation.title);
		
		i18n_flag = confirmation.i18n = 1;
	}

#endif /* ENABLE_NLS */

    my_delete    = confirm_delete;
    my_overwrite = confirm_overwrite;
    my_execute   = confirm_execute;
    my_exit      = confirm_exit;

    if (quick_dialog (&confirmation) != B_CANCEL){
	confirm_delete    = my_delete;
	confirm_overwrite = my_overwrite;
	confirm_execute   = my_execute;
	confirm_exit      = my_exit;
    }
}

#define DISPY 11
#define DISPX 46

static int new_mode;
static int new_meta;

char *display_bits_str [] =
{ N_("Full 8 bits output"), N_("ISO 8859-1"), N_("7 bits") };

static QuickWidget display_widgets [] = {
{ quick_button,   4,  6,    4, DISPY, N_("&Cancel"),
      0, B_CANCEL, 0, 0, XV_WLAY_CENTERROW, "c" },
{ quick_button,   4,  6,    3, DISPY, N_("&Ok"),
      0, B_ENTER, 0, 0, XV_WLAY_CENTERROW, "o" },
{ quick_checkbox, 4, DISPX, 7, DISPY, N_("F&ull 8 bits input"),
      0, 0, &new_meta, 0, XV_WLAY_BELOWCLOSE, "u" },
{ quick_radio,    4, DISPX, 3, DISPY, "", 3, 0,
      &new_mode, display_bits_str, XV_WLAY_BELOWCLOSE, "r" },
{ 0,              0, 0, 0, 0, 0,  0, 0, 0, 0, XV_WLAY_DONTCARE }
};

static QuickDialog display_bits =
{ DISPX, DISPY, -1, -1, N_(" Display bits "), "[Display bits]",
  "dbits", display_widgets, 0 };

void display_bits_box ()
{
    int current_mode;

#ifdef ENABLE_NLS
	static int i18n_flag = 0;
	if (!i18n_flag)
	{
		register int i;
		int l1, maxlen = 0;
		for (i = 0; i < 3; i++)
		{
			display_widgets [i].text = _(display_widgets[i].text);
			display_bits_str [i] = _(display_bits_str [i]);
			l1 = strlen (display_bits_str [i]);
			if (l1 > maxlen)
				maxlen = l1;
		}
		l1 = strlen (display_widgets [2].text);
		if (l1 > maxlen)
			maxlen = l1;
		

		display_bits.xlen = (maxlen + 5) * 6 / 4;

		/* See above confirm_box */
		l1 = strlen (display_widgets [0].text) + 3;
		i = strlen (display_widgets [1].text) + 5;
		if (i > l1)
			l1 = i;

		i = (l1 + 3) * 6 / 2;
		if (i > display_bits.xlen)
			display_bits.xlen = i;

		display_bits.title = _(display_bits.title);
		i18n_flag = display_bits.i18n = 1;
	}

#endif /* ENABLE_NLS */

    if (full_eight_bits)
	current_mode = 0;
    else if (eight_bit_clean)
	current_mode = 1;
    else
	current_mode = 2;

    display_widgets [3].value = current_mode;
    new_meta = !use_8th_bit_as_meta;
    if (quick_dialog (&display_bits) != B_ENTER)
	    return;

    eight_bit_clean = new_mode < 2;
    full_eight_bits = new_mode == 0;
#ifndef HAVE_SLANG
    meta (stdscr, eight_bit_clean);
#else
    SLsmg_Display_Eight_Bit = full_eight_bits ? 128 : 160;
#endif
    use_8th_bit_as_meta = !new_meta;
}

#define TREE_Y 20
#define TREE_X 60

static int tree_colors [4];

static int tree_callback (struct Dlg_head *h, int id, int msg)
{
    switch (msg){

    case DLG_POST_KEY:
	/* The enter key will be processed by the tree widget */
	if (id == '\n' || ((WTree *)(h->current->widget))->done){
	    h->ret_value = B_ENTER;
	    dlg_stop (h);
	}
	return MSG_HANDLED;
	
    case DLG_DRAW:
	common_dialog_repaint (h);
	break;
    }
    return MSG_NOT_HANDLED;
}

char *tree (char *current_dir)
{
    WTree    *mytree;
    Dlg_head *dlg;
    char     *val;
    WButtonBar *bar;

    tree_colors [3] = dialog_colors [0];
    tree_colors [1] = dialog_colors [1];
    
    /* Create the components */
    dlg = create_dlg (0, 0, TREE_Y, TREE_X, tree_colors,
		      tree_callback, "[Directory Tree]", "tree", DLG_CENTER);
    mytree = tree_new (0, 2, 2, TREE_Y - 6, TREE_X - 5);
    add_widget (dlg, mytree);
    bar = buttonbar_new(1);
    add_widget (dlg, bar);
    bar->widget.x = 0;
    bar->widget.y = LINES - 1;
    
    run_dlg (dlg);
    if (dlg->ret_value == B_ENTER)
	val = strdup (mytree->selected_ptr->name);
    else
	val = 0;
    
    destroy_dlg (dlg);
    return val;
}
#ifndef USE_VFS
#ifdef USE_NETCODE
#undef USE_NETCODE
#endif
#endif

#ifdef USE_VFS

#if defined(USE_NETCODE)
#define VFSY 15
#else
#define VFSY 11
#endif

#define VFSX 56

extern int vfs_timeout;
extern int tar_gzipped_memlimit;
extern int ftpfs_always_use_proxy;

#if defined(USE_NETCODE)
extern char *ftpfs_anonymous_passwd;
extern char *ftpfs_proxy_host;
extern ftpfs_directory_timeout;
extern int use_netrc;
#endif

int vfs_use_limit = 1;
static char *ret_timeout;
static char *ret_limit;

#if defined(USE_NETCODE)
static char *ret_passwd;
static char *ret_directory_timeout;
static char *ret_ftp_proxy;
static int ret_use_netrc;
#endif

#if 0
/* Not used currently */
{ quick_checkbox,  4, VFSX, 10, VFSY, "Use ~/.netrc",
      'U', 0, 0, &ret_use_netrc, 0, XV_WLAY_BELOWCLOSE, "" },
#endif

char *confvfs_str [] =
{ N_("Always to memory"), N_("If size less than:") };

static QuickWidget confvfs_widgets [] = {
{ quick_button,   30,  VFSX,    VFSY - 3, VFSY, N_("&Cancel"),
      0, B_CANCEL, 0, 0, XV_WLAY_RIGHTOF, "button-cancel" },
{ quick_button,   12, VFSX,    VFSY - 3, VFSY, N_("&Ok"),
      0, B_ENTER, 0, 0, XV_WLAY_CENTERROW, "button-ok" },
#if defined(USE_NETCODE)
{ quick_input,    30, VFSX, 10, VFSY, "", 22, 0, 0, &ret_ftp_proxy,
      XV_WLAY_RIGHTDOWN, "input-ftp-proxy" },
{ quick_checkbox,    4, VFSX, 10, VFSY, N_("&Always use ftp proxy"), 0, 0,
      &ftpfs_always_use_proxy, 0, XV_WLAY_RIGHTDOWN, "check-ftp-proxy" },
{ quick_label,    46, VFSX, 9, VFSY, N_("sec"),
      0, 0, 0, 0, XV_WLAY_RIGHTOF, "label-sec" },
{ quick_input,    35, VFSX, 9, VFSY, "", 10, 0, 0, &ret_directory_timeout,
      XV_WLAY_RIGHTDOWN, "input-timeout" },
{ quick_label,     4, VFSX, 9, VFSY, N_("ftpfs directory cache timeout:"),
      0, 0, 0, 0, XV_WLAY_NEXTROW, "label-cache"},
{ quick_input,    28, VFSX, 8, VFSY, "", 24, 0, 0, &ret_passwd,
      XV_WLAY_RIGHTDOWN, "input-passwd" },
{ quick_label,     4, VFSX, 8, VFSY, N_("ftp anonymous password:"),
      0, 0, 0, 0, XV_WLAY_NEXTROW, "label-pass"},
#endif
{ quick_input,    26, VFSX, 6, VFSY, "", 10, 0, 0, &ret_limit, 
      XV_WLAY_RIGHTDOWN, "input-limit" },
{ quick_radio,    4, VFSX, 5, VFSY, "", 2, 0,
      &vfs_use_limit, confvfs_str, XV_WLAY_BELOWCLOSE, "radio" },
{ quick_label,    4,  VFSX, 4, VFSY, N_("Gzipped tar archive extract:"), 
      0, 0, 0, 0, XV_WLAY_NEXTROW, "label-tar" },
{ quick_label,    46, VFSX, 3, VFSY, "sec",
      0, 0, 0, 0, XV_WLAY_RIGHTOF, "label-sec2" },
{ quick_input,    35, VFSX, 3, VFSY, "", 10, 0, 0, &ret_timeout, 
      XV_WLAY_RIGHTOF, "input-timo-vfs" },
{ quick_label,    4,  VFSX, 3, VFSY, N_("Timeout for freeing VFSs:"), 
      0, 0, 0, 0, XV_WLAY_BELOWCLOSE, "label-vfs" },
{ 0,              0, 0, 0, 0, 0, 0, 0, 0, 0, 0, XV_WLAY_DONTCARE, 0 }
};

static QuickDialog confvfs_dlg =
{ VFSX, VFSY, -1, -1, N_(" Virtual File System Setting "), "[Virtual FS]", "quick_vfs", confvfs_widgets, 0 };

#if defined(USE_NETCODE)
#define VFS_WIDGETBASE 7
#else
#define VFS_WIDGETBASE 0
#endif

void configure_vfs ()
{
    char buffer1 [15], buffer2 [15];
#if defined(USE_NETCODE)
    char buffer3[15];
#endif

    if (tar_gzipped_memlimit > -1) {
        if (tar_gzipped_memlimit == 0)
            strcpy (buffer1, "0 B");
	else if ((tar_gzipped_memlimit % (1024*1024)) == 0) /* I.e. in M */
	    sprintf (buffer1, "%i MB", (int)(((unsigned)tar_gzipped_memlimit) >> 20));
	else if ((tar_gzipped_memlimit % 1024) == 0) /* I.e. in K */
	    sprintf (buffer1, "%i KB", (int)(((unsigned)tar_gzipped_memlimit) >> 10));
	else if ((tar_gzipped_memlimit % 1000) == 0)
	    sprintf (buffer1, "%i kB", (int)(tar_gzipped_memlimit / 1000));
	else
	    sprintf (buffer1, "%i B", (int)tar_gzipped_memlimit);
        confvfs_widgets [2 + VFS_WIDGETBASE].text = buffer1;
    } else
    	confvfs_widgets [2 + VFS_WIDGETBASE].text = "5 MB";
    sprintf (buffer2, "%i", vfs_timeout);
    confvfs_widgets [6 + VFS_WIDGETBASE].text = buffer2;
    confvfs_widgets [3 + VFS_WIDGETBASE].value = vfs_use_limit;
#if defined(USE_NETCODE)
    ret_use_netrc = use_netrc;
    sprintf(buffer3, "%i", ftpfs_directory_timeout);
    confvfs_widgets[5].text = buffer3;
    confvfs_widgets[7].text = ftpfs_anonymous_passwd;
    confvfs_widgets[2].text = ftpfs_proxy_host ? ftpfs_proxy_host : "";
#endif

    if (quick_dialog (&confvfs_dlg) != B_CANCEL) {
        char *p;
        
        vfs_timeout = atoi (ret_timeout);
        free (ret_timeout);
        if (vfs_timeout < 0 || vfs_timeout > 10000)
            vfs_timeout = 10;
        if (!vfs_use_limit)
            tar_gzipped_memlimit = -1;
        else {
            tar_gzipped_memlimit = atoi (ret_limit);
            if (tar_gzipped_memlimit < 0)
                tar_gzipped_memlimit = -1;
            else {
                for (p = ret_limit; *p == ' ' || (*p >= '0' && *p <= '9'); p++);
                switch (*p) {
		case 'm':
		case 'M': tar_gzipped_memlimit <<= 20; break;
		case 'K': tar_gzipped_memlimit <<= 10; break;
		case 'k': tar_gzipped_memlimit *= 1000; break;
                }
            }
        }
        free (ret_limit);
#if defined(USE_NETCODE)
	free(ftpfs_anonymous_passwd);
	ftpfs_anonymous_passwd = ret_passwd;
	if (ftpfs_proxy_host)
	    free(ftpfs_proxy_host);
	ftpfs_proxy_host = ret_ftp_proxy;
	ftpfs_directory_timeout = atoi(ret_directory_timeout);
	use_netrc = ret_use_netrc;
	free(ret_directory_timeout);
#endif
    }
}

#endif

char *cd_dialog (void)
{
    QuickDialog Quick_input;
    QuickWidget quick_widgets [] = {
#ifdef HAVE_TK
#define INPUT_INDEX 2
    { quick_button, 0, 1, 0, 1, N_("&Cancel"), 0, B_CANCEL, 0, 0, XV_WLAY_DONTCARE, "cancel" },
    { quick_button, 0, 1, 0, 1, N_("&Ok"),     0, B_ENTER,  0, 0, XV_WLAY_DONTCARE, "ok" },
#else
#define INPUT_INDEX 0
#endif
    { quick_input,  6, 57, 5, 0, "", 50, 0, 0, 0, XV_WLAY_RIGHTOF, "input" },
    { quick_label,  3, 57, 2, 0, "",  0, 0, 0, 0, XV_WLAY_DONTCARE, "label" },
    { 0 } };
    
    char *my_str;
	int len;
    
    Quick_input.xlen  = 57;
    Quick_input.title = _("Quick cd");
    Quick_input.help  = "[Quick cd]";
    Quick_input.class = "quick_input";
    quick_widgets [INPUT_INDEX].text = "";
    quick_widgets [INPUT_INDEX].value = 2; /* want cd like completion */
    quick_widgets [INPUT_INDEX+1].text = _("cd");
    quick_widgets [INPUT_INDEX+1].y_divisions =
	quick_widgets [INPUT_INDEX].y_divisions = Quick_input.ylen = 5;

	len = strlen (quick_widgets [INPUT_INDEX+1].text);

	quick_widgets [INPUT_INDEX+1].relative_x = 3;
	quick_widgets [INPUT_INDEX].relative_x = 
		quick_widgets [INPUT_INDEX+1].relative_x + len + 1;

    Quick_input.xlen = len + quick_widgets [INPUT_INDEX].hotkey_pos + 7;
	quick_widgets [INPUT_INDEX].x_divisions =
		quick_widgets [INPUT_INDEX+1].x_divisions = Quick_input.xlen;

    Quick_input.i18n = 1;
    Quick_input.xpos = 2;
    Quick_input.ypos = LINES - 2 - Quick_input.ylen;
    quick_widgets [INPUT_INDEX].relative_y = 2;
    quick_widgets [INPUT_INDEX].str_result = &my_str;
    
    Quick_input.widgets = quick_widgets;
    if (quick_dialog (&Quick_input) != B_CANCEL){
	return *(quick_widgets [INPUT_INDEX].str_result);
    } else
	return 0;
}

void symlink_dialog (char *existing, char *new, char **ret_existing, 
    char **ret_new)
{
    QuickDialog Quick_input;
    QuickWidget quick_widgets [] = {
#undef INPUT_INDEX
#if defined(HAVE_TK) || defined(HAVE_GNOME)
#define INPUT_INDEX 2
    { quick_button, 0, 1, 0, 1, _("&Cancel"), 0, B_CANCEL, 0, 0,
	  XV_WLAY_DONTCARE, "cancel" },
    { quick_button, 0, 1, 0, 1, _("&Ok"), 0, B_ENTER, 0, 0,
	  XV_WLAY_DONTCARE, "ok" },
#else
#define INPUT_INDEX 0
#endif
    { quick_input,  6, 80, 5, 8, "", 58, 0, 0, 0, XV_WLAY_BELOWCLOSE, "input-1" },
    { quick_label,  6, 80, 4, 8, "", 0, 0, 0, 0, XV_WLAY_BELOWOF, "label-1" },
    { quick_input,  6, 80, 3, 8, "", 58, 0, 0, 0, XV_WLAY_BELOWCLOSE, "input-2" },
    { quick_label,  6, 80, 2, 8, "", 0, 0, 0, 0, XV_WLAY_DONTCARE, "label-2" },
    { 0 } };
    
    Quick_input.xlen  = 64;
    Quick_input.ylen  = 8;
    Quick_input.title = "Symbolic link";
    Quick_input.help  = "[File Menu]";
    Quick_input.class = "quick_symlink";
    Quick_input.i18n  = 0;
    quick_widgets [INPUT_INDEX].text = new;
    quick_widgets [INPUT_INDEX+1].text = _("Symbolic link filename:");
    quick_widgets [INPUT_INDEX+2].text = existing;
    quick_widgets [INPUT_INDEX+3].text = _("Existing filename (filename symlink will point to):");
    Quick_input.xpos = -1;
    quick_widgets [INPUT_INDEX].str_result = ret_new;
    quick_widgets [INPUT_INDEX+2].str_result = ret_existing;
    
    Quick_input.widgets = quick_widgets;
    if (quick_dialog (&Quick_input) == B_CANCEL){
        *ret_new = NULL;
        *ret_existing = NULL;
    }
}

#ifdef WITH_BACKGROUND
#define B_STOP   B_USER+1
#define B_RESUME B_USER+2
#define B_KILL   B_USER+3

static int JOBS_X = 60;
#define JOBS_Y 15
static WListbox *bg_list;
static Dlg_head *jobs_dlg;

static void
jobs_fill_listbox (void)
{
    static char *state_str [2];
    TaskList *tl = task_list;

    if (!state_str [0]){
       state_str [0] = _("Running ");
       state_str [1] = _("Stopped");
    }
    
    while (tl){
	char *s;

	s = copy_strings (state_str [tl->state], " ", tl->info, NULL);
	listbox_add_item (bg_list, LISTBOX_APPEND_AT_END, 0, s, (void *) tl);
	free (s);
	tl = tl->next;
    }
}
	
static int
task_cb (int action, void *ignored)
{
    TaskList *tl;
    int sig;
    
    if (!bg_list->list)
	return 0;

    /* Get this instance information */
    tl = (TaskList *) bg_list->current->data;
    
    if (action == B_STOP){
	sig   = SIGSTOP;
	tl->state = Task_Stopped;
    } else if (action == B_RESUME){
	sig   = SIGCONT;
	tl->state = Task_Running;
    } else if (action == B_KILL){
	sig = SIGKILL;
    }
    
    if (sig == SIGINT)
	unregister_task_running (tl->pid, tl->fd);

    kill (tl->pid, sig);
    listbox_remove_list (bg_list);
    jobs_fill_listbox ();

    /* This can be optimized to just redraw this widget :-) */
    dlg_redraw (jobs_dlg);
    
    return 0;
}

static struct 
{
	char* name;
	int xpos;
	int value;
	int (*callback)();
	char* tkname;
} 
job_buttons [] =
{
	{N_("&Stop"),   3,  B_STOP,   task_cb, "button-stop"},
	{N_("&Resume"), 12, B_RESUME, task_cb, "button-cont"},
	{N_("&Kill"),   23, B_KILL,   task_cb, "button-kill"},
	{N_("&Ok"),     35, B_CANCEL, NULL,    "button-ok"},
};

void
jobs_cmd (void)
{
	register int i;
	int n_buttons = sizeof (job_buttons) / sizeof (job_buttons[0]);

#ifdef ENABLE_NLS
	static int i18n_flag = 0;
	if (!i18n_flag)
	{
		int startx = job_buttons [0].xpos;
		int len;

		for (i = 0; i < n_buttons; i++)
		{
			job_buttons [i].name = _(job_buttons [i].name);

			len = strlen (job_buttons [i].name) + 4;
			JOBS_X = max (JOBS_X, startx + len + 3);

			job_buttons [i].xpos = startx;
			startx += len;
		}

		/* Last button - Ok a.k.a. Cancel :) */
		job_buttons [n_buttons - 1].xpos =
			JOBS_X - strlen (job_buttons [n_buttons - 1].name) - 7;

		i18n_flag = 1;
	}
#endif /* ENABLE_NLS */

    jobs_dlg = create_dlg (0, 0, JOBS_Y, JOBS_X, dialog_colors,
			   common_dialog_callback, "[Background jobs]", "jobs",
			   DLG_CENTER | DLG_GRID);
    x_set_dialog_title (jobs_dlg, _("Background Jobs"));
    
    bg_list = listbox_new (2, 3, JOBS_X-7, JOBS_Y-9, listbox_nothing, 0, "listbox");
    add_widget (jobs_dlg, bg_list);

	i = n_buttons;
	while (i--)
	{
		add_widget (jobs_dlg, button_new (JOBS_Y-4, 
			job_buttons [i].xpos, job_buttons [i].value,
			NORMAL_BUTTON, job_buttons [i].name, 
			job_buttons [i].callback, 0,
			job_buttons [i].tkname));
	}
	
    /* Insert all of task information in the list */
    jobs_fill_listbox ();
    run_dlg (jobs_dlg);
    
    destroy_dlg (jobs_dlg);
}
#endif

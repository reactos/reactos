/* Configure box module for the Midnight Commander
   Copyright (C) 1994 Radek Doulik

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
#include <string.h>
#include <stdio.h>
/* Needed for the extern declarations of integer parameters */
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#ifdef HAVE_UNISTD_H
#   include <unistd.h>
#endif
#include "tty.h"
#include "mad.h"
#include "util.h"
#include "win.h"
#include "color.h"
#include "dlg.h"
#include "widget.h"
#include "setup.h"		/* For save_setup() */
#include "dialog.h"		/* For do_refresh() */
#include "main.h"
#include "profile.h"		/* For sync_profiles */

#include "dir.h"
#include "panel.h"		/* Needed for the externs */
#include "file.h"
#include "layout.h"		/* For nice_rotating_dash */
#define PX	4
#define PY	2

#define RX	4
#define RY	11

#define BY	16
#define OY	2

static Dlg_head *conf_dlg;

static int r_but;

#define TOGGLE_VARIABLE 0

extern int use_internal_edit;

int dummy;

static int OX = 33, first_width = 27, second_width = 27;
static char *configure_title, *title1, *title2, *title3;

static struct {
    char   *text;
    int    *variable;
    void   (*toggle_function)(void);
    WCheck *widget;
    char   *tk;
} check_options [] = {
   {N_("safe de&Lete"),       &know_not_what_am_i_doing, TOGGLE_VARIABLE,0, "safe-del" },
   {N_("cd follows lin&Ks"),  &cd_symlinks,       TOGGLE_VARIABLE,       0, "cd-follow" },
   {N_("advanced cho&Wn"),    &advanced_chfns,    TOGGLE_VARIABLE,       0, "achown" },
   {N_("l&Ynx-like motion"),  &navigate_with_arrows,TOGGLE_VARIABLE,     0, "lynx" },
#ifdef HAVE_GNOME
   {N_("Animation"),          &dummy,             TOGGLE_VARIABLE,       0, "dummy" },
#else
   {N_("ro&Tating dash"),     &nice_rotating_dash,TOGGLE_VARIABLE,       0, "rotating" },
#endif
   {N_("co&Mplete: show all"),&show_all_if_ambiguous,TOGGLE_VARIABLE,    0, "completion" },
   {N_("&Use internal view"), &use_internal_view, TOGGLE_VARIABLE,       0, "view-int" },
   {N_("use internal ed&It"), &use_internal_edit, TOGGLE_VARIABLE,       0, "edit-int" },
   {N_("auto m&Enus"),        &auto_menu,         TOGGLE_VARIABLE,       0, "auto-menus" },
   {N_("&Auto save setup"),   &auto_save_setup,   TOGGLE_VARIABLE,       0, "auto-save" },
   {N_("shell &Patterns"),    &easy_patterns,     TOGGLE_VARIABLE,       0, "shell-patt" },
   {N_("&Verbose operation"), &verbose,           TOGGLE_VARIABLE,       0, "verbose" },
   {N_("&Fast dir reload"),   &fast_reload,       toggle_fast_reload,    0, "fast-reload" },
   {N_("mi&X all files"),     &mix_all_files,     toggle_mix_all_files,  0, "mix-files" },
   {N_("&Drop down menus"),   &drop_menus,        TOGGLE_VARIABLE,       0, "drop-menus" },
   {N_("ma&Rk moves down"),   &mark_moves_down,   TOGGLE_VARIABLE,       0, "mark-moves" },
   {N_("show &Hidden files"), &show_dot_files,    toggle_show_hidden,    0, "show-hidden" },
   {N_("show &Backup files"), &show_backups,      toggle_show_backup,    0, "show-backup" },
   { 0, 0, 0, 0 }
};

static WRadio *pause_radio;

static char *pause_options [3] = {
    N_("&Never"),
    N_("on dumb &Terminals"),
    N_("alwa&Ys") };

static int configure_callback (struct Dlg_head *h, int Id, int Msg)
{
    switch (Msg) {
    case DLG_DRAW:
#ifndef HAVE_X    
	attrset (COLOR_NORMAL);
	dlg_erase (h);
	draw_box (h, 1, 2, h->lines - 2, h->cols - 4);
	draw_box (h, PY, PX, 8, first_width);
	draw_box (h, RY, RX, 5, first_width);
	draw_box (h, OY, OX, 14, second_width);

	attrset (COLOR_HOT_NORMAL);
	dlg_move (h, 1, (h->cols - strlen(configure_title))/2);
	addstr (configure_title);
	dlg_move (h, OY, OX+1);
	addstr (title3);
	dlg_move (h, RY, RX+1);
	addstr (title2);
	dlg_move (h, PY, PX+1);
	addstr (title1);
#endif
	break;

    case DLG_END:
	r_but = Id;
	break;
    }
    return 0;
}

static void init_configure (void)
{
    int i;
	static int i18n_config_flag = 0;
	static int b1, b2, b3;
	char* ok_button = _("&Ok");
	char* cancel_button = _("&Cancel");
	char* save_button = _("&Save");

	if (!i18n_config_flag)
	{
		register int l1;

		/* Similar code is in layout.c (init_layout())  */

		configure_title = _(" Configure options ");
		title1 = _(" Panel options ");
		title2 = _(" Pause after run... ");
		title3 = _(" Other options ");

		first_width = strlen (title1) + 1;
		for (i = 12; i < 18; i++)
		{
			check_options[i].text = _(check_options[i].text);
			l1 = strlen (check_options[i].text) + 7;
			if (l1 > first_width)
				first_width = l1;
		}

		i = sizeof(pause_options)/sizeof(char*);
		while (i--)
		{
			pause_options [i] = _(pause_options [i]);
			l1 = strlen (pause_options [i]) + 7;
			if (l1 > first_width)
				first_width = l1;
		}

		l1 = strlen (title2) + 1;
		if (l1 > first_width)
			first_width = l1;

		OX = first_width + 5;

		second_width = strlen (title3) + 1;
		for (i = 0; i < 12; i++)
		{
			check_options[i].text = _(check_options[i].text);
			l1 = strlen (check_options[i].text) + 7;
			if (l1 > second_width)
				second_width = l1;
		}

		l1 = 11 + strlen (ok_button)
		 	+ strlen (save_button)
			+ strlen (cancel_button);
		
		i = (first_width + second_width - l1) / 4;
		b1 = 5 + i;
		b2 = b1 + strlen(ok_button) + i + 6;
		b3 = b2 + strlen(save_button) + i + 4;

		i18n_config_flag = 1;
	}

    conf_dlg = create_dlg (0, 0, 19, first_width + second_width + 9,
		dialog_colors, configure_callback, "[Options Menu]",
		"option", DLG_CENTER | DLG_GRID);

    x_set_dialog_title (conf_dlg, _("Configure options"));

    add_widgetl (conf_dlg,
	button_new (BY, b3, B_CANCEL, NORMAL_BUTTON, cancel_button, 0, 0, "button-cancel"),
	XV_WLAY_RIGHTOF);

    add_widgetl (conf_dlg,
	button_new (BY, b2, B_EXIT, NORMAL_BUTTON, save_button, 0, 0, "button-save"),
	XV_WLAY_RIGHTOF);
    
    add_widgetl (conf_dlg,
        button_new (BY, b1, B_ENTER, DEFPUSH_BUTTON, ok_button, 0, 0, "button-ok"),
        XV_WLAY_CENTERROW);

#define XTRACT(i) *check_options[i].variable, check_options[i].text, check_options [i].tk

    /* Add all the checkboxes */
    for (i = 0; i < 12; i++){
	check_options [i].widget = check_new (OY + (12-i), OX+2, XTRACT(i));
	add_widgetl (conf_dlg, check_options [i].widget,
	    XV_WLAY_BELOWCLOSE);
    }

    pause_radio = radio_new (RY+1, RX+2, 3, pause_options, 1, "pause-radio");
    pause_radio->sel = pause_after_run;
#ifndef HAVE_GNOME
    add_widgetl (conf_dlg, pause_radio, XV_WLAY_BELOWCLOSE);
#endif
    for (i = 0; i < 6; i++){
	check_options [i+12].widget = check_new (PY + (6-i), PX+2,
						  XTRACT(i+12));
	add_widgetl (conf_dlg, check_options [i+12].widget,
	    XV_WLAY_BELOWCLOSE);
    }
#ifdef HAVE_XVIEW
     add_widgetl (conf_dlg, label_new (OY, OX + 1, _("Other options"), "label-other"),
         XV_WLAY_NEXTCOLUMN);
     add_widgetl (conf_dlg, label_new (RY, RX + 1, _("Pause after run..."), "label-pause"),
         XV_WLAY_BELOWOF);
     add_widgetl (conf_dlg, label_new (PY, PX + 1, _("Panel options"), "label-panel"),
         XV_WLAY_NEXTCOLUMN);
#endif
}


void configure_box (void)
{
    int result, i;
    
    init_configure ();
    run_dlg (conf_dlg);

    result = conf_dlg->ret_value;
    if (result == B_ENTER || result == B_EXIT){
	for (i = 0; check_options [i].text; i++)
	    if (check_options [i].widget->state & C_CHANGE){
		if (check_options [i].toggle_function)
		    (*check_options [i].toggle_function)();
		else
		    *check_options [i].variable =
			!(*check_options [i].variable);
	    }
	pause_after_run = pause_radio->sel;
    }

    /* If they pressed the save button */
    if (result == B_EXIT){
	save_configure ();
	sync_profiles ();
    }

    destroy_dlg (conf_dlg);
}

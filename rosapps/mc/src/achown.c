/* Chown-advanced command -- for the Midnight Commander
   Copyright (C) 1994, 1995 Radek Doulik

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
/* Needed for the extern declarations of integer parameters */
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <grp.h>
#include <pwd.h>
#ifdef HAVE_UNISTD_H
#   include <unistd.h>
#endif
#include <string.h>
#include <stdio.h>
#include <stdlib.h>		/* For malloc() */
#include <errno.h>	/* For errno on SunOS systems	      */
#include "mad.h"
#include "tty.h"
#include "util.h"		/* Needed for the externs */
#include "win.h"
#include "color.h"
#include "dlg.h"
#include "widget.h"
#include "dialog.h"		/* For do_refresh() */
#include "wtools.h"		/* For init_box_colors() */
#include "key.h"		/* XCTRL and ALT macros */

#include "dir.h"
#include "panel.h"		/* Needed for the externs */
#include "file.h"
#include "chmod.h"
#include "main.h"
#include "../vfs/vfs.h"

#define BX		5
#define BY		6

#define TX              50
#define TY              2

#define BUTTONS		9

#define B_SETALL        B_USER
#define B_SKIP          B_USER + 1

#define B_OWN           B_USER + 3
#define B_GRP           B_USER + 4
#define B_OTH           B_USER + 5
#define B_OUSER         B_USER + 6
#define B_OGROUP        B_USER + 7

struct {
    int ret_cmd, flags, y, x;
    char *text;
} chown_advanced_but [BUTTONS] = {
    { B_CANCEL, NORMAL_BUTTON, 4, 55, N_("&Cancel") },
    { B_ENTER,  DEFPUSH_BUTTON,4, 45, N_("&Set") },
    { B_SKIP,   NORMAL_BUTTON, 4, 36, N_("S&kip") },
    { B_SETALL, NORMAL_BUTTON, 4, 24, N_("Set &all")},
    { B_ENTER,  NARROW_BUTTON, 0, 47, "               "},
    { B_ENTER,  NARROW_BUTTON, 0, 29, "               "},
    { B_ENTER,  NARROW_BUTTON, 0, 19, "   "},
    { B_ENTER,  NARROW_BUTTON, 0, 11, "   "},
    { B_ENTER,  NARROW_BUTTON, 0, 3, "   "},
};

WButton *b_att[3];	/* permission */
WButton *b_user, *b_group;	/* owner */

static int files_on_begin;	/* Number of files at startup */
static int flag_pos;
static int x_toggle;
static char ch_flags[11];
static char *ch_perm = "rwx";
static umode_t ch_cmode;
struct stat *sf_stat;
static int need_update;
static int end_chown;
static int current_file;
static int single_set;
static char *fname;

static void get_ownership ()
{				/* set buttons  - ownership */
    char *name_t;

    name_t = name_trunc (get_owner (sf_stat->st_uid), 15);
    memset (b_user->text, ' ', 15);
    strncpy (b_user->text, name_t, strlen (name_t));
    name_t = name_trunc (get_group (sf_stat->st_gid), 15);
    memset (b_group->text, ' ', 15);
    strncpy (b_group->text, name_t, strlen (name_t));
}


static int inc_flag_pos (int f_pos)
{
    if (flag_pos == 10) {
	flag_pos = 0;
	return 0;
    }
    flag_pos++;
    if (!(flag_pos % 3) || f_pos > 2)
	return 0;
    return 1;
}

static int dec_flag_pos (int f_pos)
{
    if (!flag_pos) {
	flag_pos = 10;
	return 0;
    }
    flag_pos--;
    if (!((flag_pos + 1) % 3) || f_pos > 2)
	return 0;
    return 1;
}

static void set_perm_by_flags (char *s, int f_p)
{
    int i;

    for (i = 0; i < 3; i++)
	if (ch_flags[f_p + i] == '+')
	    s[i] = ch_perm[i];
	else if (ch_flags[f_p + i] == '-')
	    s[i] = '-';
	else
	    s[i] = (ch_cmode & (1 << (8 - f_p - i))) ? ch_perm[i] : '-';
}

static void set_perm (char *s, int p)
{
    s[0] = (p & 4) ? 'r' : '-';
    s[1] = (p & 2) ? 'w' : '-';
    s[2] = (p & 1) ? 'x' : '-';
}

static umode_t get_perm (char *s, int base)
{
    umode_t m;

    m = 0;
    m |= (s [0] == '-') ? 0 :
	((s[0] == '+') ? (1 << (base + 2)) : (1 << (base + 2)) & ch_cmode);

    m |= (s [1] == '-') ? 0 :
	((s[1] == '+') ? (1 << (base + 1)) : (1 << (base + 1)) & ch_cmode);
    
    m |= (s [2] == '-') ? 0 :
	((s[2] == '+') ? (1 << base) : (1 << base) & ch_cmode);

    return m;
}

static umode_t get_mode ()
{
    umode_t m;

    m = ch_cmode ^ (ch_cmode & 0777);
    m |= get_perm (ch_flags, 6);
    m |= get_perm (ch_flags + 3, 3);
    m |= get_perm (ch_flags + 6, 0);

    return m;
}

static void print_flags (void)
{
    int i;

    attrset (COLOR_NORMAL);

    for (i = 0; i < 3; i++){
	dlg_move (ch_dlg, BY+1, 9+i);
	addch (ch_flags [i]);
    }
    
    for (i = 0; i < 3; i++){
	dlg_move (ch_dlg, BY + 1, 17 + i);
	addch (ch_flags [i+3]);
    }
    
    for (i = 0; i < 3; i++){
	dlg_move (ch_dlg, BY + 1, 25 + i);
	addch (ch_flags [i+6]);
    }

    set_perm_by_flags (b_att[0]->text, 0);
    set_perm_by_flags (b_att[1]->text, 3);
    set_perm_by_flags (b_att[2]->text, 6);

    for (i = 0; i < 15; i++){
	dlg_move (ch_dlg, BY+1, 35+i);
	addch (ch_flags[9]);
    }
    for (i = 0; i < 15; i++){
	dlg_move (ch_dlg, BY + 1, 53 + i);
	addch (ch_flags[10]);
    }
}

static void update_mode (Dlg_head * h)
{
    print_flags ();
    attrset (COLOR_NORMAL);
    dlg_move (h, BY + 2, 9);
    printw ("%12o", get_mode ());
    send_message (h, h->current->widget, WIDGET_FOCUS, 0);
}

static int l_call (void *data)
{
    return 1;
}

static int chl_callback (Dlg_head * h, int Par, int Msg)
{
    switch (Msg) {
    case DLG_DRAW:
	attrset (COLOR_NORMAL);
	dlg_erase (h);
	draw_box (h, 0, 0, 13, 17);
	break;
	
    case DLG_KEY:
	switch (Par) {
	case KEY_LEFT:
	case KEY_RIGHT:
	    h->ret_value = Par;
	    dlg_stop (h);
	}
    }
    return 0;
}

static void do_enter_key (Dlg_head *h, int f_pos)
{
    Dlg_head *chl_dlg;
    WListbox *chl_list;
    struct   passwd *chl_pass;
    struct   group *chl_grp;
    WLEntry  *fe;
    int      lxx, lyy, chl_end, b_pos;
    
    do {
	lxx = (COLS - 74) / 2 + ((f_pos == 3) ? 35 : 53);
	lyy = (LINES - 13) / 2;
	chl_end = 0;
	
	chl_dlg = create_dlg (lyy, lxx, 13, 17, dialog_colors, chl_callback,
			      "[Chown-advanced]", "achown_enter", DLG_NONE);
	
	/* get new listboxes */
	chl_list = listbox_new (1, 1, 15, 11, 0, l_call, NULL);
	
	listbox_add_item (chl_list, 0, 0, "<Unknown>", NULL);
	
	if (f_pos == 3) {
	    /* get and put user names in the listbox */
	    setpwent ();
	    while ((chl_pass = getpwent ()))
		listbox_add_item (chl_list, 0, 0, chl_pass->pw_name, NULL);
	    endpwent ();
	    fe = listbox_search_text (chl_list, get_owner (sf_stat->st_uid));
	}
	else
	{
	    /* get and put group names in the listbox */
	    setgrent ();	
	    while ((chl_grp = getgrent ())) {
		listbox_add_item (chl_list, 0, 0, chl_grp->gr_name, NULL);
	    }
	    endgrent ();
	    fe = listbox_search_text (chl_list, get_group (sf_stat->st_gid));
	}
	
	if (fe)
	    listbox_select_entry (chl_list, fe);
	
	b_pos = chl_list->pos;
	add_widget (chl_dlg, chl_list);
	
	run_dlg (chl_dlg);
	
	if (b_pos != chl_list->pos){
	    int ok = 0;
	    if (f_pos == 3){
		chl_pass = getpwnam (chl_list->current->text);
		if (chl_pass){
		    ok = 1;
		    sf_stat->st_uid = chl_pass->pw_uid;
		}
	    } else {
		chl_grp = getgrnam (chl_list->current->text);
		if (chl_grp){
		    sf_stat->st_gid = chl_grp->gr_gid;
		    ok = 1;
		}
	    }
	    if (ok){
		ch_flags [f_pos + 6] = '+';
		get_ownership ();
	    }
	    dlg_focus (h);
	    if (ok)
		print_flags ();
	}
	if (chl_dlg->ret_value == KEY_LEFT){
	    if (f_pos == 4)
		chl_end = 1;
	    dlg_one_up (ch_dlg);
	    f_pos--;
	} else if (chl_dlg->ret_value == KEY_RIGHT) {
	    if (f_pos == 3)
		chl_end = 1;
	    dlg_one_down (ch_dlg);
	    f_pos++;
	}
	/* Here we used to redraw the window */
	destroy_dlg (chl_dlg);
    } while (chl_end);
}

static void chown_refresh (void)
{
    attrset (COLOR_NORMAL);
    dlg_erase (ch_dlg);

    draw_box (ch_dlg, 1, 2, 11, 70);

    dlg_move (ch_dlg, BY - 1, 8);
    addstr (_("owner"));
    dlg_move (ch_dlg, BY - 1, 16);
    addstr (_("group"));
    dlg_move (ch_dlg, BY - 1, 24);
    addstr (_("other"));
    
    dlg_move (ch_dlg, BY - 1, 35);
    addstr (_("owner"));
    dlg_move (ch_dlg, BY - 1, 53);
    addstr (_("group"));
    
    dlg_move (ch_dlg, 3, 4);
    addstr (_("On"));
    dlg_move (ch_dlg, BY + 1, 4);
    addstr (_("Flag"));
    dlg_move (ch_dlg, BY + 2, 4);
    addstr (_("Mode"));
    

    if (!single_set){
	dlg_move (ch_dlg, 3, 54);
	printw (_("%6d of %d"), files_on_begin - (cpanel->marked) + 1,
		   files_on_begin);
    }

    print_flags ();

    attrset (COLOR_HOT_NORMAL);
    dlg_move (ch_dlg, 1, 24);
    addstr (_(" Chown advanced command "));
}

static void chown_info_update ()
{
    /* display file info */
    attrset (COLOR_NORMAL);
    
    /* name && mode */
    dlg_move (ch_dlg, 3, 8);
    printw ("%s", name_trunc (fname, 45));
    dlg_move (ch_dlg, BY + 2, 9);
    printw ("%12o", get_mode ());
    
    /* permissions */
    set_perm (b_att[0]->text, sf_stat->st_mode >> 6);
    set_perm (b_att[1]->text, sf_stat->st_mode >> 3);
    set_perm (b_att[2]->text, sf_stat->st_mode);
}

static void b_setpos (int f_pos) {
	b_att[0]->hotpos=-1;
	b_att[1]->hotpos=-1;
	b_att[2]->hotpos=-1;
	b_att[f_pos]->hotpos = (flag_pos % 3);
}

static int advanced_chown_callback (Dlg_head * h, int Par, int Msg)
{
    int i = 0, f_pos = BUTTONS - h->current->dlg_id - single_set - 1;

    switch (Msg) {
    case DLG_DRAW:
	chown_refresh ();
	chown_info_update ();
	return 1;
	
    case DLG_POST_KEY:
	if (f_pos < 3)
		b_setpos (f_pos);
	break;

    case DLG_FOCUS:
	if (f_pos < 3) {
	    if ((flag_pos / 3) != f_pos)
		flag_pos = f_pos * 3;
	    b_setpos (f_pos);
	} else if (f_pos < 5)
	    flag_pos = f_pos + 6;
	break;

    case DLG_KEY:
	switch (Par) {
	    
	case XCTRL('b'):
	case KEY_LEFT:
	    if (f_pos < 5)
		return (dec_flag_pos (f_pos));
	    break;
	    
	case XCTRL('f'):
	case KEY_RIGHT:
	    if (f_pos < 5)
		return (inc_flag_pos (f_pos));
	    break;
	    
	case ' ':
	    if (f_pos < 3)
		return 1;
	    break;
	    
	case '\n':
	case KEY_ENTER:
	    if (f_pos <= 2 || f_pos >= 5)
		break;
	    do_enter_key (h, f_pos);
	    return 1;
	    
	case ALT ('x'):
	    i++;
	    
	case ALT ('w'):
	    i++;
	    
	case ALT ('r'):
	    Par = i + 3;
	    for (i = 0; i < 3; i++)
		ch_flags[i * 3 + Par - 3] = (x_toggle & (1 << Par)) ? '-' : '+';
	    x_toggle ^= (1 << Par);
	    update_mode (h);
	    dlg_broadcast_msg (h, WIDGET_DRAW, 0);
	    send_message (h, h->current->widget, WIDGET_FOCUS, 0);
	    break;
	    
	case XCTRL ('x'):
	    i++;
	    
	case XCTRL ('w'):
	    i++;
	    
	case XCTRL ('r'):
	    Par = i;
	    for (i = 0; i < 3; i++)
		ch_flags[i * 3 + Par] = (x_toggle & (1 << Par)) ? '-' : '+';
	    x_toggle ^= (1 << Par);
	    update_mode (h);
	    dlg_broadcast_msg (h, WIDGET_DRAW, 0);
	    send_message (h, h->current->widget, WIDGET_FOCUS, 0);
	    break;
	    
	case 'x':
	    i++;
	    
	case 'w':
	    i++;
	    
	case 'r':
	    if (f_pos > 2)
		break;
	    flag_pos = f_pos * 3 + i;	/* (strchr(ch_perm,Par)-ch_perm); */
	    if (((WButton *) h->current->widget)->text[(flag_pos % 3)] == '-')
		ch_flags[flag_pos] = '+';
	    else
		ch_flags[flag_pos] = '-';
	    update_mode (h);
	    break;

	case '4':
	    i++;
	    
	case '2':
	    i++;
	    
	case '1':
	    if (f_pos > 2)
		break;
	    flag_pos = i + f_pos * 3;
	    ch_flags[flag_pos] = '=';
	    update_mode (h);
	    break;
	    
	case '-':
	    if (f_pos > 2)
		break;
	    
	case '*':
	    if (Par == '*')
		Par = '=';
	    
	case '=':
	case '+':
	    if (f_pos > 4)
		break;
	    ch_flags[flag_pos] = Par;
	    update_mode (h);
	    advanced_chown_callback (h, KEY_RIGHT, DLG_KEY);
	    if (flag_pos>8 || !(flag_pos%3)) dlg_one_down (h);

	    break;
	}
	return 0;
    }
    return 0;
}

static void init_chown_advanced (void)
{
    int i;

    sf_stat = (struct stat *) malloc (sizeof (struct stat));
    do_refresh ();
    end_chown = need_update = current_file = 0;
    single_set = (cpanel->marked < 2) ? 2 : 0;
    memset (ch_flags, '=', 11);
    flag_pos = 0;
    x_toggle = 070;

    ch_dlg = create_dlg (0, 0, 13, 74, dialog_colors, advanced_chown_callback,
			 "[Chown-advanced]", "achown", DLG_CENTER);

#define XTRACT(i) BY+chown_advanced_but[i].y, BX+chown_advanced_but[i].x, \
	chown_advanced_but[i].ret_cmd, chown_advanced_but[i].flags, chown_advanced_but[i].text, \
        0, 0, NULL

    for (i = 0; i < BUTTONS - 5; i++)
	if (!single_set || i < 2)
	    add_widget (ch_dlg, button_new (XTRACT (i)));

    b_att[0] = button_new (XTRACT (8));
    b_att[1] = button_new (XTRACT (7));
    b_att[2] = button_new (XTRACT (6));
    b_user = button_new (XTRACT (5));
    b_group = button_new (XTRACT (4));

    add_widget (ch_dlg, b_group);
    add_widget (ch_dlg, b_user);
    add_widget (ch_dlg, b_att[2]);
    add_widget (ch_dlg, b_att[1]);
    add_widget (ch_dlg, b_att[0]);
}

void chown_advanced_done (void)
{
    free (sf_stat);
    if (need_update)
	update_panels (UP_OPTIMIZE, UP_KEEPSEL);
    repaint_screen ();
}

#if 0
static inline void do_chown (uid_t u, gid_t g)
{
    chown (cpanel->dir.list[current_file].fname, u, g);
    file_mark (cpanel, current_file, 0);
}
#endif

static char *next_file (void)
{
    while (!cpanel->dir.list[current_file].f.marked)
	current_file++;

    return cpanel->dir.list[current_file].fname;
}

static void apply_advanced_chowns (struct stat *sf)
{
    char *fname;
    gid_t a_gid = sf->st_gid;
    uid_t a_uid = sf->st_uid;

    fname = cpanel->dir.list[current_file].fname;
    need_update = end_chown = 1;
    if (mc_chmod (fname, get_mode ()) == -1)
	message (1, MSG_ERROR, _(" Couldn't chmod \"%s\" \n %s "),
		 fname, unix_error_string (errno));
    /* call mc_chown only, if mc_chmod didn't fail */
    else if (mc_chown (fname, (ch_flags[9] == '+') ? sf->st_uid : -1,
		       (ch_flags[10] == '+') ? sf->st_gid : -1) == -1)
	message (1, MSG_ERROR, _(" Couldn't chown \"%s\" \n %s "),
		 fname, unix_error_string (errno));
    do_file_mark (cpanel, current_file, 0);

    do {
	fname = next_file ();

	if (!stat_file (fname, sf))
	    break;
	ch_cmode = sf->st_mode;
	if (mc_chmod (fname, get_mode ()) == -1)
	    message (1, MSG_ERROR, _(" Couldn't chmod \"%s\" \n %s "),
		     fname, unix_error_string (errno));
	/* call mc_chown only, if mc_chmod didn't fail */
	else if (mc_chown (fname, (ch_flags[9] == '+') ? a_uid : -1, (ch_flags[10] == '+') ? a_gid : -1) == -1)
	    message (1, MSG_ERROR, _(" Couldn't chown \"%s\" \n %s "),
		     fname, unix_error_string (errno));

	do_file_mark (cpanel, current_file, 0);
    } while (cpanel->marked);
}

void chown_advanced_cmd (void)
{

    files_on_begin = cpanel->marked;

    if (!vfs_current_is_local ()) {
	if (vfs_current_is_extfs ()) {
	    message (1, _(" Oops... "),
		     _(" I can't run the Advanced Chown command on an extfs "));
	    return;
	} else if (vfs_current_is_tarfs ()) {
	    message (1, _(" Oops... "),
		     _(" I can't run the Advanced Chown command on a tarfs "));
	    return;
	}
    }

    do {			/* do while any files remaining */
	init_chown_advanced ();

	if (cpanel->marked)
	    fname = next_file ();	/* next marked file */
	else
	    fname = selection (cpanel)->fname;	/* single file */

	if (!stat_file (fname, sf_stat)){	/* get status of file */
	    destroy_dlg (ch_dlg);
	    break;
	}
	ch_cmode = sf_stat->st_mode;

	chown_refresh ();

	get_ownership ();

	/* game can begin */
	run_dlg (ch_dlg);

	switch (ch_dlg->ret_value) {
	case B_CANCEL:
	    end_chown = 1;
	    break;

	case B_ENTER:
	    need_update = 1;
	    if (mc_chmod (fname, get_mode ()) == -1)
		message (1, MSG_ERROR, _(" Couldn't chmod \"%s\" \n %s "),
			 fname, unix_error_string (errno));
	    /* call mc_chown only, if mc_chmod didn't fail */
	    else if (mc_chown (fname, (ch_flags[9] == '+') ? sf_stat->st_uid : -1, (ch_flags[10] == '+') ? sf_stat->st_gid : -1) == -1)
		message (1, MSG_ERROR, _(" Couldn't chown \"%s\" \n %s "),
			 fname, unix_error_string (errno));
	    break;
	case B_SETALL:
	    apply_advanced_chowns (sf_stat);
	    break;

	case B_SKIP:
	    break;

	}

	if (cpanel->marked && ch_dlg->ret_value != B_CANCEL) {
	    do_file_mark (cpanel, current_file, 0);
	    need_update = 1;
	}
	destroy_dlg (ch_dlg);
    } while (cpanel->marked && !end_chown);

    chown_advanced_done ();
}

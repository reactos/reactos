/* Chmod command for Windows NT and OS/2

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

#ifdef __os2__
#define INCL_DOSFILEMGR
#include <os2.h>
#endif

#ifdef _OS_NT
#include <windows.h>
#endif

#include <string.h>
#include <stdio.h>
/* for chmod and stat */
#include <io.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "../src/tty.h"
#include "../src/mad.h"
#include "../src/util.h"
#include "../src/win.h"
#include "../src/color.h"
#include "../src/dlg.h"
#include "../src/widget.h"
#include "../src/dialog.h"	/* For do_refresh() */

#include "../src/dir.h"
#include "../src/panel.h"		/* Needed for the externs */
#include "../src/file.h"
#include "../src/main.h"
#include "../src/chmod.h"
#include "../src/achown.h"
#include "../src/chown.h"

#ifdef _OS_NT
#define FILE_ARCHIVED  FILE_ATTRIBUTE_ARCHIVE
#define FILE_DIRECTORY FILE_ATTRIBUTE_DIRECTORY
#define FILE_HIDDEN    FILE_ATTRIBUTE_HIDDEN
#define FILE_READONLY  FILE_ATTRIBUTE_READONLY
#define FILE_SYSTEM    FILE_ATTRIBUTE_SYSTEM
#define mk_chmod(fname,st) SetFileAttributes(fname,st)
#endif

static int single_set;
struct Dlg_head *ch_dlg;

#define PX		5
#define PY		2

#define FX		40
#define FY		2

#define BX		6
#define BY		17

#define TX		40
#define TY		12

#define PERMISSIONS	4
#define BUTTONS		6

#define B_MARKED	B_USER
#define B_ALL		B_USER+1
#define B_SETMRK        B_USER+2
#define B_CLRMRK        B_USER+3


int     mode_change, need_update;
int     c_file,      end_chmod;

umode_t and_mask,    or_mask,   c_stat;
char    *c_fname,    *c_fown,   *c_fgrp, *c_fperm;
int     c_fsize;

static WLabel        *statl;
static int           normal_color;
static int           title_color;
static int           selection_color;

/* bsedos.h */
struct {
    mode_t      mode;
    char        *text;
    int         selected;
    WCheck      *check;
} check_perm[PERMISSIONS] = {

    {	
	FILE_ARCHIVED, N_("Archive"), 0, 0,
    },
    {
	FILE_READONLY, N_("Read Only"), 0, 0,
    },
    {
	FILE_HIDDEN, N_("Hidden"), 0, 0,
    },
    {
	FILE_SYSTEM, N_("System"), 0, 0,
    },
};

struct {
    int ret_cmd, flags, y, x;
    char *text;
} chmod_but[BUTTONS] = {

    {
	B_CANCEL, NORMAL_BUTTON, 2, 33, N_("&Cancel"),
    },
    {
	B_ENTER, DEFPUSH_BUTTON, 2, 17, N_("&Set"),
    },
    {
	B_CLRMRK, NORMAL_BUTTON, 0, 42, N_("C&lear marked"),
    },
    {
	B_SETMRK, NORMAL_BUTTON, 0, 27, N_("S&et marked"),
    },
    {
	B_MARKED, NORMAL_BUTTON, 0, 12, N_("&Marked all"),
    },
    {
	B_ALL, NORMAL_BUTTON, 0, 0, N_("Set &all"),
    },
};

static void chmod_toggle_select (void)
{
    int Id = ch_dlg->current->dlg_id - BUTTONS + single_set * 2;

    attrset (normal_color);
    check_perm[Id].selected ^= 1;

    dlg_move (ch_dlg, PY + PERMISSIONS - Id, PX + 1);
    addch ((check_perm[Id].selected) ? '*' : ' ');
    dlg_move (ch_dlg, PY + PERMISSIONS - Id, PX + 3);
}

static void chmod_refresh (void)
{
    attrset (normal_color);
    dlg_erase (ch_dlg);

    draw_box (ch_dlg, 1, 2, 20 - single_set, 66);
    draw_box (ch_dlg, PY, PX, PERMISSIONS + 2, 33);
    draw_box (ch_dlg, FY, FX, 10, 25);

    dlg_move (ch_dlg, FY + 1, FX + 2);
    addstr (_("Name"));
    dlg_move (ch_dlg, FY + 3, FX + 2);
    addstr (_("Permissions (Octal)"));
    dlg_move (ch_dlg, FY + 5, FX + 2);
    addstr (_("Owner name"));
    dlg_move (ch_dlg, FY + 7, FX + 2);
    addstr (_("Group name"));

    attrset (title_color);
    dlg_move (ch_dlg, 1, 28);
    addstr (_(" Chmod command "));
    dlg_move (ch_dlg, PY, PX + 1);
    addstr (_(" Permission "));
    dlg_move (ch_dlg, FY, FX + 1);
    addstr (_(" File "));

    attrset (selection_color);

    dlg_move (ch_dlg, TY, TX);
    addstr (_("Use SPACE to change"));
    dlg_move (ch_dlg, TY + 1, TX);
    addstr (_("an option, ARROW KEYS"));
    dlg_move (ch_dlg, TY + 2, TX);
    addstr (_("to move between options"));
    dlg_move (ch_dlg, TY + 3, TX);
    addstr (_("and T or INS to mark"));
}

static int chmod_callback (Dlg_head *h, int Par, int Msg)
{
    char buffer [10];

    switch (Msg) {
    case DLG_ACTION:
	if (Par >= BUTTONS - single_set * 2){
	    c_stat ^= check_perm[Par - BUTTONS + single_set * 2].mode;
	    sprintf (buffer, "%o", c_stat);
	    label_set_text (statl, buffer);
	    chmod_toggle_select ();
	    mode_change = 1;
	}
	break;

    case DLG_KEY:
	if ((Par == 'T' || Par == 't' || Par == KEY_IC) &&
	    ch_dlg->current->dlg_id >= BUTTONS - single_set * 2) {
	    chmod_toggle_select ();
	    if (Par == KEY_IC)
		dlg_one_down (ch_dlg);
	    return 1;
	}
	break;
#ifndef HAVE_X
    case DLG_DRAW:
	chmod_refresh ();
	break;
#endif
    }
    return 0;
}

static void init_chmod (void)
{
    int i;

    do_refresh ();
    end_chmod = c_file = need_update = 0;
    single_set = (cpanel->marked < 2) ? 2 : 0;

    if (use_colors){
	normal_color = COLOR_NORMAL;
	title_color  = COLOR_HOT_NORMAL;
	selection_color = COLOR_NORMAL;
    } else {
	normal_color = NORMAL_COLOR;
	title_color  = SELECTED_COLOR;
	selection_color = SELECTED_COLOR;
    }

    ch_dlg = create_dlg (0, 0, 22 - single_set, 70, dialog_colors,
			 chmod_callback, _("[Chmod]"), _("chmod"), DLG_CENTER);
			
    x_set_dialog_title (ch_dlg, _("Chmod command"));

#define XTRACT(i) BY+chmod_but[i].y-single_set, BX+chmod_but[i].x, \
     chmod_but[i].ret_cmd, chmod_but[i].flags, chmod_but[i].text, 0, 0, NULL

    tk_new_frame (ch_dlg, "b.");
    for (i = 0; i < BUTTONS; i++) {
	if (i == 2 && single_set)
	    break;
	else
	    add_widgetl (ch_dlg, button_new (XTRACT (i)), XV_WLAY_RIGHTOF);
    }


#define XTRACT2(i) 0, check_perm [i].text, NULL
    tk_new_frame (ch_dlg, "c.");
    for (i = 0; i < PERMISSIONS; i++) {
	check_perm[i].check = check_new (PY + (PERMISSIONS - i), PX + 2,
					 XTRACT2 (i));
	add_widget (ch_dlg, check_perm[i].check);
    }
}

int pc_stat_file (char *filename)
{
    mode_t st;

#ifdef _OS_NT
    st = GetFileAttributes (filename);
#endif /* _OS_NT */

#ifdef __os2__
    HFILE       fHandle    = 0L;
    ULONG       fInfoLevel = 1;  /* 1st Level Info: Standard attributs */
    FILESTATUS3 fInfoBuf;
    ULONG       fInfoBufSize;
    ULONG      fAction    = 0;
    APIRET      rc;

    fInfoBufSize = sizeof(FILESTATUS3);
    rc = DosOpen((PSZ) filename,
                 &fHandle,
                 &fAction,
                 (ULONG) 0,
                 FILE_NORMAL,
                 OPEN_ACTION_OPEN_IF_EXISTS,
                 (OPEN_ACCESS_READONLY | OPEN_SHARE_DENYNONE),
                 (PEAOP2) NULL);
    if (rc != 0) {
       return -1;
    }

    rc = DosQueryFileInfo(fHandle, fInfoLevel, &fInfoBuf, fInfoBufSize);
    DosClose(fHandle);
    if (rc != 0) {
       return -1;  /* error ! */
    } else {
       st = fInfoBuf.attrFile;
    }
#endif /* __os2__ */

    if (st & FILE_DIRECTORY)
    	st = -1;
    return st;
}

static void chmod_done (void)
{
    if (need_update)
	update_panels (UP_OPTIMIZE, UP_KEEPSEL);
    repaint_screen ();
}

char *next_file (void)
{
    while (!cpanel->dir.list[c_file].f.marked)
	c_file++;

    return cpanel->dir.list[c_file].fname;
}

#ifdef __os2__
static int mk_chmod (char *filename, ULONG st)
{
    HFILE       fHandle    = 0L;
    ULONG       fInfoLevel = 1;  /* 1st Level Info: Standard attributs */
    FILESTATUS3 fInfoBuf;
    ULONG       fInfoBufSize;
    ULONG       fAction    = 0L;
    APIRET      rc;

    if (!(st & FILE_READONLY))
       chmod(filename, (S_IWRITE | S_IREAD));
    fInfoBufSize = sizeof(FILESTATUS3);
    rc = DosOpen((PSZ) filename,
                 &fHandle,
                 &fAction,
                 (ULONG) 0,
                 FILE_NORMAL,
                 OPEN_ACTION_OPEN_IF_EXISTS,
                 (OPEN_ACCESS_READWRITE | OPEN_SHARE_DENYNONE),
                 0L);
    if (rc != 0) {
       return rc;
    }

    rc = DosQueryFileInfo(fHandle, fInfoLevel, &fInfoBuf, fInfoBufSize);
    if (rc!=0) {
       DosClose(fHandle);
       return rc;
    }
    fInfoBuf.attrFile = st;
    rc = DosSetFileInfo(fHandle, fInfoLevel, &fInfoBuf, fInfoBufSize);
    rc = DosClose(fHandle);
    return rc;
}
#endif /* __os2__ */

static void do_chmod (mode_t sf)
{
    sf &= and_mask;
    sf |= or_mask;

    mk_chmod(cpanel->dir.list[c_file].fname, sf);

    do_file_mark (cpanel, c_file, 0);
}

static void apply_mask (mode_t sf)
{
    char *fname;
    mode_t sf_stat;

    need_update = end_chmod = 1;
    do_chmod (sf);

    do {
	fname = next_file ();
	if ((sf_stat = pc_stat_file (fname)) < 0)
	    break;

	c_stat = sf_stat;
	do_chmod (c_stat);
    } while (cpanel->marked);
}

void chmod_cmd (void)
{
    char buffer [10];
    char *fname;
    int i;
    mode_t sf_stat;

    do {			/* do while any files remaining */
	init_chmod ();
	if (cpanel->marked)
	    fname = next_file ();	/* next marked file */
	else
	    fname = selection (cpanel)->fname;	/* single file */

	if ((sf_stat = pc_stat_file (fname)) < 0) /* get status of file */
	    break;

	c_stat = sf_stat;
	mode_change = 0;	/* clear changes flag */

	/* set check buttons */
	for (i = 0; i < PERMISSIONS; i++){
	    check_perm[i].check->state = (c_stat & check_perm[i].mode) ? 1 : 0;
	    check_perm[i].selected = 0;
	}

	tk_new_frame (ch_dlg, "l.");
	/* Set the labels */
	c_fname = name_trunc (fname, 21);
	add_widget (ch_dlg, label_new (FY+2, FX+2, c_fname, NULL));
	c_fown = _("unknown");
	add_widget (ch_dlg, label_new (FY+6, FX+2, c_fown, NULL));
	c_fgrp = _("unknown");
	add_widget (ch_dlg, label_new (FY+8, FX+2, c_fgrp, NULL));
	sprintf (buffer, "%o", c_stat);
	statl = label_new (FY+4, FX+2, buffer, NULL);
	add_widget (ch_dlg, statl);
	tk_end_frame ();
	
	run_dlg (ch_dlg);	/* retrieve an action */
	
	/* do action */
	switch (ch_dlg->ret_value){
	case B_ENTER:
	    if (mode_change)
		mk_chmod (fname, c_stat);  /*.ado */
	    need_update = 1;
	    break;
	
	case B_CANCEL:
	    end_chmod = 1;
	    break;
	
	case B_ALL:
	case B_MARKED:
	    and_mask = or_mask = 0;
	    and_mask = ~and_mask;

	    for (i = 0; i < PERMISSIONS; i++) {
		if (check_perm[i].selected || ch_dlg->ret_value == B_ALL)
		    if (check_perm[i].check->state & C_BOOL)
			or_mask |= check_perm[i].mode;
		    else
			and_mask &= ~check_perm[i].mode;
	    }

	    apply_mask (sf_stat);
	    break;
	
	case B_SETMRK:
	    and_mask = or_mask = 0;
	    and_mask = ~and_mask;

	    for (i = 0; i < PERMISSIONS; i++) {
		if (check_perm[i].selected)
		    or_mask |= check_perm[i].mode;
	    }

	    apply_mask (sf_stat);
	    break;
	case B_CLRMRK:
	    and_mask = or_mask = 0;
	    and_mask = ~and_mask;

	    for (i = 0; i < PERMISSIONS; i++) {
		if (check_perm[i].selected)
		    and_mask &= ~check_perm[i].mode;
	    }

	    apply_mask (sf_stat);
	    break;
	}

	if (cpanel->marked && ch_dlg->ret_value!=B_CANCEL) {
	    do_file_mark (cpanel, c_file, 0);
	    need_update = 1;
	}
	destroy_dlg (ch_dlg);
    } while (cpanel->marked && !end_chmod);
    chmod_done ();
}

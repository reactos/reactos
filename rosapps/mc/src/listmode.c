/* Directory panel listing format editor -- for the Midnight Commander
   Copyright (C) 1994, 1995 The Free Software Foundation

   Written by: 1994 Radek Doulik
   	       1995 Janne Kukonlehto

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
#ifdef HAVE_UNISTD_H
#   include <unistd.h>
#endif
#include <string.h>
#include <stdio.h>
#include <stdlib.h>		/* For malloc() */
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#ifndef OS2_NT
#    include <grp.h>
#    include <pwd.h>
#endif
#include "tty.h"
#include "mad.h"
#include "util.h"		/* Needed for the externs */
#include "win.h"
#include "color.h"
#include "dlg.h"
#include "widget.h"
#include "dialog.h"		/* For do_refresh() */
#include "wtools.h"

/* Needed for the extern declarations of integer parameters */
#include "dir.h"
#include "panel.h"		/* Needed for the externs */
#include "file.h"
#include "main.h"
#include "global.h"
#include "listmode.h"

#define UX		5
#define UY		2

#define BX		5
#define BY		18

#define BUTTONS		4
#define LABELS          4
#define B_ADD		B_USER
#define B_REMOVE        B_USER + 1

static WListbox *l_listmode;

static Dlg_head *listmode_dlg;

static WLabel *pname;

static char *listmode_section = "[Listing format edit]";

static char *s_genwidth [2] = {"Half width", "Full width"};
WRadio *radio_genwidth;
static char *s_columns [2] = {"One column", "Two columns"};
WRadio *radio_columns;
static char *s_justify [3] =
{"Left justified", "Default justification", "Right justified"};
WRadio *radio_justify;
static char *s_itemwidth [3] =
{"Free width", "Fixed width", "Growable width"};
WRadio *radio_itemwidth;

struct {
    int ret_cmd, flags, y, x;
    char *text;
} listmode_but[BUTTONS] = {
    { B_CANCEL, NORMAL_BUTTON, 0, 53, "&Cancel" },
    { B_ADD, NORMAL_BUTTON,    0, 22, "&Add item"},
    { B_REMOVE, NORMAL_BUTTON, 0, 10, "&Remove" },
    { B_ENTER, DEFPUSH_BUTTON, 0,  0, "&Ok" },
};

#define B_PLUS B_USER
#define B_MINUS B_USER+1

struct {
    int y, x;
    char *text;
} listmode_text [LABELS] = {
    { UY, UX + 1, " General options " },
    { UY+4, UX+1, " Items "},
    { UY+4, UX+21, " Item options" },
    { UY+13, UX+22, "Item width:" }
};

#ifndef HAVE_X
static void listmode_refresh (void)
{
    attrset (COLOR_NORMAL);
    dlg_erase (listmode_dlg);
    
    draw_box (listmode_dlg, 1, 2, 20, 70);
    draw_box (listmode_dlg, UY, UX, 4, 63);
    draw_box (listmode_dlg, UY + 4, UX, 11, 18);
    draw_box (listmode_dlg, UY + 4, UX+20, 11, 43);
}
#endif

static int bplus_cback (int action, void *data)
{
    return 0;
}

static int bminus_cback (int action, void *data)
{
    return 0;
}

static int listmode_callback (Dlg_head * h, int Par, int Msg)
{
    switch (Msg) {
#ifndef HAVE_X    
    case DLG_DRAW:
	listmode_refresh ();
	break;
#endif	

    case DLG_POST_KEY:
	/* fall */
    case DLG_INIT:
	attrset (COLOR_NORMAL);
	dlg_move (h, UY+13, UX+35);
	printw ("%02d", 99);
	attrset (MENU_ENTRY_COLOR);
	break;
    }
    return 0;
}

static int l_call (void *data)
{
    return 1;
}

static void init_listmode (char *oldlistformat)
{
    int i;
    char *s;
    int format_width = 0;
    int format_columns = 0;

    do_refresh ();

    listmode_dlg = create_dlg (0, 0, 22, 74, dialog_colors,
			      listmode_callback, listmode_section, "listmode",
			      DLG_CENTER);
    x_set_dialog_title (listmode_dlg, "Listing format edit");	    

#define XTRACT(i) BY+listmode_but[i].y, BX+listmode_but[i].x, listmode_but[i].ret_cmd, listmode_but[i].flags, listmode_but[i].text, 0, 0, NULL

    for (i = 0; i < BUTTONS; i++)
	add_widgetl (listmode_dlg, button_new (XTRACT (i)), (i == BUTTONS - 1) ?
	    XV_WLAY_CENTERROW : XV_WLAY_RIGHTOF);

    /* We add the labels. */
    for (i = 0; i < LABELS; i++){
	pname = label_new (listmode_text [i].y,
			   listmode_text [i].x, listmode_text [i].text, NULL);
	add_widget (listmode_dlg, pname);
    }

    add_widget (listmode_dlg, button_new (UY+13, UX+37, B_MINUS, NORMAL_BUTTON,
					  "&-", bminus_cback, 0, NULL));
    add_widget (listmode_dlg, button_new (UY+13, UX+34, B_PLUS, NORMAL_BUTTON,
					  "&+", bplus_cback, 0, NULL));
    radio_itemwidth = radio_new (UY+9, UX+22, 3, s_itemwidth, 1, NULL);
    add_widget (listmode_dlg, radio_itemwidth);
    radio_itemwidth = 0;
    radio_justify = radio_new (UY+5, UX+22, 3, s_justify, 1, NULL);
    add_widget (listmode_dlg, radio_justify);
    radio_justify->sel = 1;

    /* get new listbox */
    l_listmode = listbox_new (UY + 5, UX + 1, 16, 9, 0, l_call, NULL);

    if (strncmp (oldlistformat, "full ", 5) == 0){
	format_width = 1;
	oldlistformat += 5;
    }
    if (strncmp (oldlistformat, "half ", 5) == 0){
	oldlistformat += 5;
    }
    if (strncmp (oldlistformat, "2 ", 2) == 0){
	format_columns = 1;
	oldlistformat += 2;
    }
    if (strncmp (oldlistformat, "1 ", 2) == 0){
	oldlistformat += 2;
    }
    s = strtok (oldlistformat, ",");

    while (s){
	listbox_add_item (l_listmode, 0, 0, s, NULL);
	s = strtok (NULL, ",");
    }

    /* add listbox to the dialogs */
    add_widgetl (listmode_dlg, l_listmode, XV_WLAY_EXTENDWIDTH);

    radio_columns = radio_new (UY+1, UX+32, 2, s_columns, 1, NULL);
    add_widget (listmode_dlg, radio_columns);
    radio_columns->sel = format_columns;
    radio_genwidth = radio_new (UY+1, UX+2, 2, s_genwidth, 1, NULL);
    add_widget (listmode_dlg, radio_genwidth);
    radio_genwidth->sel = format_width;
}

static void listmode_done (void)
{
    destroy_dlg (listmode_dlg);
    if (0)
	update_panels (UP_OPTIMIZE, UP_KEEPSEL);
    repaint_screen ();
}

char *select_new_item (void)
{
    /* NOTE: The following array of possible items must match the
       formats array in screen.c. Better approach might be to make the
       formats array global */
    char *possible_items [] =
    { "name", "size", "type", "mtime", "perm", "mode", "|", "nlink",
	  "owner", "group", "atime", "ctime", "space", "mark",
	  "inode", NULL };

    int i;
    Listbox *mylistbox;

    mylistbox = create_listbox_window (12, 20, " Add listing format item ", listmode_section);
    for (i = 0; possible_items [i]; i++){
	listbox_add_item (mylistbox->list, 0, 0, possible_items [i], NULL);
    }

    i = run_listbox (mylistbox);
    if (i >= 0)
	return possible_items [i];
    else
	return NULL;
}

char *collect_new_format (void)
{
    char *newformat;
    int i;
    char *last;
    char *text, *extra;

    newformat = xmalloc (1024, "collect_new_format");
    if (radio_genwidth->sel)
	strcpy (newformat, "full ");
    else
	strcpy (newformat, "half ");
    if (radio_columns->sel)
	strcat (newformat, "2 ");
    last = NULL;
    for (i = 0;;i++){
	listbox_select_by_number (l_listmode, i);
	listbox_get_current (l_listmode, &text, &extra);
	if (text == last)
	    break;
	if (last != NULL)
	    strcat (newformat, ",");
	strcat (newformat, text);
	last = text;
    }
    return newformat;
}

char *listmode_edit (char *oldlistformat)
{
    char *newformat = NULL;
    char *s;

    s = strdup (oldlistformat);
    init_listmode (s);
    free (s);

    while (newformat == NULL)
    {
	/* display file info */
	attrset (SELECTED_COLOR);

	run_dlg (listmode_dlg);

	switch (listmode_dlg->ret_value) {
	case B_CANCEL:
	    newformat = strdup (oldlistformat);
	    break;

	case B_ADD:
	    s = select_new_item ();
	    if (s)
		listbox_add_item (l_listmode, 0, 0, s, NULL);
	    break;

	case B_REMOVE:
	    listbox_remove_current (l_listmode, 0);
	    break;

	case B_ENTER:
	    newformat = collect_new_format ();
	    break;
	}
    }

    listmode_done ();
    return newformat;
}

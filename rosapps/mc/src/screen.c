/* Panel managing.
   Copyright (C) 1994, 1995 Miguel de Icaza.
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   Written by: 1995 Miguel de Icaza
               1997 Timur Bakeyev

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#include <config.h>
#include "tty.h"
#include "fs.h"
#include <sys/param.h>
#include <string.h>
#include <stdlib.h>	/* For malloc() and free() */
#include <stdio.h>
#include <errno.h>
#ifdef HAVE_UNISTD_H
#   include <unistd.h>	/* For chdir(), readlink() and getwd()/getcwd() */
#endif
#include "mem.h"
#include "mad.h"
#include "global.h"
#include "dir.h"
#include "util.h"
#include "panel.h"
#include "color.h"
#include "tree.h"
#include "win.h"
#include "main.h"
#include "ext.h"		/* regexp_command */
#include "mouse.h"		/* For Gpm_Event */
#include "cons.saver.h"		/* For console_flag */
#include "layout.h"		/* Most layout variables are here */
#include "dialog.h"		/* for message (...) */
#include "cmd.h"
#include "key.h"		/* XCTRL and ALT macros  */
#include "setup.h"		/* For loading/saving panel options */
#include "user.h"
#include "profile.h"
#include "widget.h"
#include "../vfs/vfs.h"
#include "../vfs/extfs.h"

#if defined(OS2_NT)
# include "drive.h"
#endif

#include "x.h"

/* "$Id: screen.c,v 1.1 2001/12/30 09:55:21 sedwards Exp $" */
#define ELEMENTS(arr) ( sizeof(arr) / sizeof((arr)[0]) )

/* If true, show the mini-info on the panel */
int show_mini_info = 1;

/* If true, then use stat() on the cwd to determine directory changes */
int fast_reload = 0;

/* If true, use some usability hacks by Torben */
int torben_fj_mode = 0;

/* If true, up/down keys scroll the pane listing by pages */
int panel_scroll_pages = 1;

/* If 1, we use permission hilighting */
int permission_mode = 0;

/* If 1 - then add per file type hilighting */
int filetype_mode = 1;


/* This gives abilitiy to determine colored user priveleges */
extern user_in_groups *current_user_gid;
extern uid_t current_user_uid;

/* If we have an info panel, this points to it */
WPanel *the_info_panel = 0;

/* The hook list for the select file function */
Hook *select_file_hook = 0;

static int panel_callback (Dlg_head *h, WPanel *p, int Msg, int Par);
int panel_event (Gpm_Event *event, WPanel *panel);

#ifndef PORT_HAS_PANEL_ADJUST_TOP_FILE
#   define x_adjust_top_file(p)
#endif

#ifndef PORT_HAS_PANEL_RESET_SORT_LABELS
#   define x_reset_sort_labels(x) 
#endif

/* This macro extracts the number of available lines in a panel */
#ifndef PORT_HAS_LLINES
#define llines(p) (p->widget.lines-3 - (show_mini_info ? 2 : 0))
#else
#define llines(p) (p->widget.lines)
#endif

#ifdef PORT_NOT_FOCUS_SELECT_ITEM
#   define focus_select_item(x)
#else
#   define focus_select_item(x) select_item(x)
#endif

#ifdef PORT_NOT_UNFOCUS_UNSELECT_ITEM
#    define unfocus_unselect_item(x)
#else
#    define unfocus_unselect_item(x) unselect_item(x)
#endif

#ifdef HAVE_X
#   define set_colors(x)
#else
#   define x_create_panel(x,y,z) 1;
#   define x_panel_load_index(p,x)
#   define x_panel_select_item(a,b,c)
#   define x_panel_destroy(p)

void
set_colors (WPanel *panel)
{
    standend ();
    if (hascolors)
	attrset (NORMAL_COLOR);
}
#endif

#ifndef ICONS_PER_ROW
#    define ICONS_PER_ROW(x) 1
#endif

/* Delete format string, it is a linked list */
void
delete_format (format_e *format)
{
    format_e *next;
    
    while (format){
        next = format->next;
        free (format);
        format = next;
     }
}

/* This code relies on the default justification!!! */
void
add_permission_string (char *dest, int width, file_entry *fe, int attr, int color, int is_octal)
{
    int i, r, l;
    
    l = get_user_rights (&fe->buf);
    
    if (is_octal){
	/* Place of the access bit in octal mode */
        l = width + l - 3;
	r = l + 1;
    } else {
	/* The same to the triplet in string mode */
        l = l * 3 + 1;
	r = l + 3;
    }
    
    for(i = 0; i < width; i++){
	if (i >= l && i < r){
            if (attr == SELECTED || attr == MARKED_SELECTED)
                attrset (MARKED_SELECTED_COLOR);
            else
                attrset (MARKED_COLOR);
        } else
            attrset (color);
	
	addch (dest[i]);
    }
}

int
file_entry_color (file_entry *fe)
{
    if (filetype_mode){
        if (S_ISDIR (fe->buf.st_mode))
            return (DIRECTORY_COLOR);
        else if (S_ISLNK (fe->buf.st_mode)) {
            if (fe->f.link_to_dir)
                return (DIRECTORY_COLOR);
            else if (fe->f.stalled_link)
                return (STALLED_COLOR);
            else
                return (LINK_COLOR);
        } else if (S_ISSOCK (fe->buf.st_mode))
            return (SPECIAL_COLOR);
        else if (S_ISCHR (fe->buf.st_mode))
            return (DEVICE_COLOR);
        else if (S_ISBLK (fe->buf.st_mode))
            return (DEVICE_COLOR);
        else if (S_ISFIFO (fe->buf.st_mode))
            return (SPECIAL_COLOR);
        else if (is_exe (fe->buf.st_mode))
            return (EXECUTABLE_COLOR);
	else if (fe->fname && (strcmp (fe->fname, "core") == 0))
	    return (CORE_COLOR);
    }
    return (NORMAL_COLOR);
}

/* This functions return a string representation of a file entry */
char *
string_file_type (file_entry *fe, int len)
{
    static char buffer [2];

    if (S_ISDIR (fe->buf.st_mode))
	buffer [0] = PATH_SEP;
    else if (S_ISLNK (fe->buf.st_mode)) {
        if (fe->f.link_to_dir)
            buffer [0] = '~';
        else if (fe->f.stalled_link)
            buffer [0] = '!';
        else
	    buffer [0] = '@';
    } else if (S_ISSOCK (fe->buf.st_mode))
	buffer [0] = '=';
    else if (S_ISCHR (fe->buf.st_mode))
	buffer [0] = '-';
    else if (S_ISBLK (fe->buf.st_mode))
	buffer [0] = '+';
    else if (S_ISFIFO (fe->buf.st_mode))
	buffer [0] = '|';
    else if (is_exe (fe->buf.st_mode))
	buffer [0] = '*';
    else
	buffer [0] = ' ';
    buffer [1] = 0;
    return buffer;
}

char *
string_file_size_brief (file_entry *fe, int len)
{
    static char buffer [8];

    if (S_ISDIR (fe->buf.st_mode)){
       strcpy (buffer, (strcmp (fe->fname, "..") ? "SUB-DIR" : "UP--DIR"));
       return buffer;
    }

    return string_file_size (fe, len);
}

char *
string_file_permission (file_entry *fe, int len)
{
    return string_perm (fe->buf.st_mode);
}

char *
string_file_nlinks (file_entry *fe, int len)
{
    static char buffer [20];

    sprintf (buffer, "%16d", fe->buf.st_nlink);
    return buffer;
}

char *
string_file_owner (file_entry *fe, int len)
{
    return get_owner (fe->buf.st_uid);
}

char *
string_file_group (file_entry *fe, int len)
{
    return get_group (fe->buf.st_gid);
}

char *
string_file_size (file_entry *fe, int len)
{
    static char buffer [16];
    int i;

#ifdef HAVE_ST_RDEV
    if (S_ISBLK (fe->buf.st_mode) || S_ISCHR (fe->buf.st_mode))
        sprintf (buffer, "%3d,%3d", (int) (fe->buf.st_rdev >> 8), 
            (int) (fe->buf.st_rdev & 0xff));
    else
#endif 
    {   
        sprintf (buffer, "%lu", (unsigned long) fe->buf.st_size);
        if (len && (i = strlen (buffer)) > len) {
            if (i - 2 > len) {
                if (i - 5 > len)
                    sprintf (buffer, "%luG", (unsigned long) ((fe->buf.st_size) >> 30));
                else
                    sprintf (buffer, "%luM", (unsigned long) ((fe->buf.st_size) >> 20));
            } else
                sprintf (buffer, "%luK", (unsigned long) ((fe->buf.st_size) >> 10));
        }
    }
    return buffer;
}

char *
string_file_mtime (file_entry *fe, int len)
{
    return file_date (fe->buf.st_mtime);
}

char *
string_file_atime (file_entry *fe, int len)
{
    return file_date (fe->buf.st_atime);
}

char *
string_file_ctime (file_entry *fe, int len)
{
    return file_date (fe->buf.st_ctime);
}

#ifdef HAVE_GNOME
/* In GNOME, the CList truncates the names */
char *
string_file_name (file_entry *fe, int len)
{
	return fe->fname;
}
#else
char *
string_file_name (file_entry *fe, int len)
{
    if (len)
        return name_trunc (fe->fname, len);
    else
	return fe->fname;
}
#endif

char *
string_space (file_entry *fe, int len)
{
    return " ";
}

char *
string_dot (file_entry *fe, int len)
{
    return ".";
}

char *
string_marked (file_entry *fe, int len)
{
    return fe->f.marked ? "*" : " ";
}

char *
string_file_perm_octal (file_entry *fe, int len)
{
    static char buffer [9];

    sprintf (buffer, "0%06o", fe->buf.st_mode);
    return buffer;
}

char *
string_inode (file_entry *fe, int len)
{
    static char buffer [9];

    sprintf (buffer, "%ld", (long) fe->buf.st_ino);
    return buffer;
}

char *
string_file_ngid (file_entry *fe, int len)
{
    static char buffer [9];

    sprintf (buffer, "%d", fe->buf.st_gid);
    return buffer;
}

char *
string_file_nuid (file_entry *fe, int len)
{
    static char buffer [9];

    sprintf (buffer, "%d", fe->buf.st_uid);
    return buffer;
}

#ifdef HAVE_GNOME
#    define GT 2
#else
#    define GT 1
#endif

static struct {
    char *id;
    int  min_size;
    int  expands;
    int  default_just;
    char *title;
    int  use_in_gui;
    char *(*string_fn)(file_entry *, int);
    sortfn *sort_routine;
} formats [] = {
{ "name",  12, 1, J_LEFT,  N_("Name"),       1, string_file_name,       (sortfn *) sort_name },
{ "size",  7,  0, J_RIGHT, N_("Size"),       1, string_file_size,       (sortfn *) sort_size },
{ "type",  GT, 0, J_LEFT,     "",            1, string_file_type,       (sortfn *) sort_type },
{ "mtime", 12, 0, J_RIGHT, N_("MTime"),      1, string_file_mtime,      (sortfn *) sort_time },
{ "bsize", 7,  0, J_RIGHT, N_("Size"),       1, string_file_size_brief, (sortfn *) sort_size },
{ "perm",  10, 0, J_LEFT,  N_("Permission"), 1, string_file_permission, NULL },
{ "mode",  6,  0, J_RIGHT, N_("Perm"),       1, string_file_perm_octal, NULL },
{ "|",     1,  0, J_RIGHT, N_("|"),          0, 0,                      NULL },
{ "nlink", 2,  0, J_RIGHT, N_("Nl"),         1, string_file_nlinks,     (sortfn *) sort_links },
{ "ngid",  5,  0, J_RIGHT, N_("GID"),        1, string_file_ngid,       (sortfn *) sort_ngid },
{ "nuid",  5,  0, J_RIGHT, N_("UID"),        1, string_file_nuid,       (sortfn *) sort_nuid },
{ "owner", 8,  0, J_LEFT,  N_("Owner"),      1, string_file_owner,      (sortfn *) sort_owner },
{ "group", 8,  0, J_LEFT,  N_("Group"),      1, string_file_group,      (sortfn *) sort_group },
{ "atime", 12, 0, J_RIGHT, N_("ATime"),      1, string_file_atime,      (sortfn *) sort_atime },
{ "ctime", 12, 0, J_RIGHT, N_("CTime"),      1, string_file_ctime,      (sortfn *) sort_ctime },
{ "space", 1,  0, J_RIGHT,    " ",          0, string_space,           NULL },
{ "dot",   1,  0, J_RIGHT,    " ",          0, string_dot,             NULL },
{ "mark",  1,  0, J_RIGHT,    " ",          1, string_marked,          NULL },
{ "inode", 5,  0, J_RIGHT, N_("Inode"),      1, string_inode,           (sortfn *) sort_inode },
};

static char *
to_buffer (char *dest, int just_mode, int len, char *txt)
{
    int txtlen = strlen (txt);
    int still;

    if (txtlen > len){
	if (just_mode != J_LEFT)
	    txt += txtlen - len;
	txtlen = len;
    }
    still = len - txtlen;
    if (just_mode == J_LEFT){
	strcpy (dest, txt);
	dest += txtlen;
	while (still--)
	    *dest++ = ' ';
	*dest = 0;
    } else {
	while (still--)
	    *dest++ = ' ';
	strcpy (dest, txt);
	dest += txtlen;
    }
    return dest;
}

int
file_compute_color (int attr, file_entry *fe)
{
    int color;
    
    switch (attr){
    case SELECTED:
	color = SELECTED_COLOR;
	break;
    case MARKED:
	color = MARKED_COLOR;
	break;
    case MARKED_SELECTED:
	color = MARKED_SELECTED_COLOR;
	break;
    case STATUS:
	color = NORMAL_COLOR;
	break;
    case NORMAL:
    default:
	color = file_entry_color(fe);
    }
    return color;
}

/* Formats the file number file_index of panel in the buffer dest */
void
format_file (char *dest, WPanel *panel, int file_index, int width, int attr, int isstatus)
{
    int      color, length, empty_line;
    char     *txt;
    char     *old_pos;
    char     *cdest = dest;
    format_e *format, *home;
    file_entry *fe;
    
    length     = 0;
    empty_line = (file_index >= panel->count);
    home       = (isstatus) ? panel->status_format : panel->format;   
    fe         = &panel->dir.list [file_index];

    if (!empty_line)
	color = file_compute_color (attr, fe);
    else
	color = NORMAL_COLOR;
    for (format = home; format; format = format->next){
	if (format->string_fn){
	    int len;
	    
	    if (empty_line)
		txt = " ";
	    else
		txt = (*format->string_fn)(fe, format->field_len);

	    old_pos = cdest;

	    len = format->field_len;
	    if (len + length > width)
		len = width - length;
	    cdest = to_buffer (cdest, format->just_mode, len, txt);
	    length += len;

#ifdef HAVE_X
	    if (length == width)
		    break;
#else
            /* What shall we do? Will we color each line according to
	     * the file type? Any suggestions to mc@timur.kazan.su
	     */
	    
            attrset (color);
 
            if (permission_mode && !strcmp(format->id, "perm"))
                add_permission_string (old_pos, format->field_len, fe, attr, color, 0);
            else if (permission_mode && !strcmp(format->id, "mode"))
                add_permission_string (old_pos, format->field_len, fe, attr, color, 1);
            else
		addstr (old_pos);
#endif
	} else {
#ifndef HAVE_X
            /* I'm preffer the view without this 3 lines, try to kill it :-) */
            if (attr == SELECTED || attr == MARKED_SELECTED)
                attrset (SELECTED_COLOR);
            else
                attrset (NORMAL_COLOR);
	    one_vline ();
#else
	    *cdest++ = ' ';
#endif
	    length++;
	}
    }
    
    if (length < width){
	int still = width - length;
	while (still--)
#ifdef HAVE_X
	    *cdest++ = ' ';
	*cdest = 0;
#else
	    addch (' ');
#endif
    }
}

#ifndef HAVE_X
void
repaint_file (WPanel *panel, int file_index, int mv, int attr, int isstatus)
{
    int    second_column = 0;
    int	   width, offset;
    char   buffer [255];

    offset = 0;
    if (!isstatus && panel->split){
	
	second_column = (file_index - panel->top_file) / llines (panel);
	width = (panel->widget.cols - 2)/2 - 1;
	
	if (second_column){
	    offset = 1 + width;
	    width = (panel->widget.cols-2) - (panel->widget.cols-2)/2 - 1;
	} 
    } else
        width = (panel->widget.cols - 2);

    if (mv){
	if (!isstatus && panel->split){
	    widget_move (&panel->widget,
			 (file_index - panel->top_file) %
			 llines (panel) + 2,
			 (offset + 1));
	} else
	    widget_move (&panel->widget, file_index - panel->top_file + 2, 1);
    }
    
    format_file (buffer, panel, file_index, width, attr, isstatus);
    
    if (!isstatus && panel->split){
	if (second_column)
	    addch (' ');
	else {
	    attrset (NORMAL_COLOR);
	    one_vline ();
	}
    }
}
#endif

#ifndef PORT_HAS_DISPLAY_MINI_INFO
void
display_mini_info (WPanel *panel)
{
    if (!show_mini_info)
	return;
    
    widget_move (&panel->widget, llines (panel)+3, 1);
    
    if (panel->searching){
	attrset (INPUT_COLOR);
	printw ("/%-*s", panel->widget.cols-3, panel->search_buffer);
	attrset (NORMAL_COLOR);
	return;
    }

    /* Status displays total marked size */
    if (panel->marked){
	char buffer [100];
	char *p;
	
	attrset (MARKED_COLOR);
	printw  ("%*s", panel->widget.cols-2, " ");
	widget_move (&panel->widget, llines (panel)+3, 1);
	sprintf (buffer, _("  %s bytes in %d file%s"),
		 size_trunc_sep (panel->total), panel->marked,
		 panel->marked == 1 ? "" : "s");
	p = buffer;
	if (strlen (buffer) > panel->widget.cols-4){
	    buffer [panel->widget.cols-4] = 0;
	    p += 2;
	}
	printw ("%-*s", panel->widget.cols-2, p);
	return;
    }

    /* Status resolves links and show them */
    set_colors (panel);
#ifndef OS2_NT    
    if (S_ISLNK (panel->dir.list [panel->selected].buf.st_mode)){
	char *link, link_target [MC_MAXPATHLEN];
	int  len;

	link = concat_dir_and_file (panel->cwd, panel->dir.list [panel->selected].fname);
	len = mc_readlink (link, link_target, MC_MAXPATHLEN);
	free (link);
	if (len > 0){
	    link_target[len] = 0;
	    printw ("-> %-*s", panel->widget.cols - 5,
		     name_trunc (link_target, panel->widget.cols - 5));
	} else 
	    addstr (_("<readlink failed>"));
	return;
    }
#endif
    /* Default behaviour */
    repaint_file (panel, panel->selected, 0, STATUS, 1);
    return;
}
#endif

#ifndef HAVE_X
void
paint_dir (WPanel *panel)
{
    int i;
    int color;			/* Color value of the line */
    int items;			/* Number of items */

    items = llines (panel) * (panel->split ? 2 : 1);
    
    for (i = 0; i < items; i++){
	if (i+panel->top_file >= panel->count)
	    color = 0;
	else {
	    color = 2 * (panel->dir.list [i+panel->top_file].f.marked);
	    color += (panel->selected==i+panel->top_file && panel->active);
	}
	repaint_file (panel, i+panel->top_file, 1, color, 0);	
    }
    standend ();
    panel->dirty = 0;
}
#endif

#ifdef HAVE_X
#define mini_info_separator(x)
#else
static void
mini_info_separator (WPanel *panel)
{
    if (!show_mini_info)
	return;
    
    standend ();
    widget_move (&panel->widget, llines (panel)+2, 1);
#ifdef HAVE_SLANG
    attrset (NORMAL_COLOR);
    hline (ACS_HLINE, panel->widget.cols-2);
#else
    hline ((slow_terminal ? '-' : ACS_HLINE) | NORMAL_COLOR,
	  panel->widget.cols-2);
#endif 
}

void
show_dir (WPanel *panel)
{
    char tmp [200];

    set_colors (panel);
    draw_double_box (panel->widget.parent,
	            panel->widget.y,    panel->widget.x,
	            panel->widget.lines, panel->widget.cols);

#ifdef HAVE_SLANG	            
    if (show_mini_info) {
#ifdef linux_unicode
    	if (SLtt_Unicode) {
            SLsmg_draw_unicode (panel->widget.y + llines (panel) + 2, 
                panel->widget.x, SLUNI_DSLTEE_CHAR);
            SLsmg_draw_unicode (panel->widget.y + llines (panel) + 2, 
                panel->widget.x + panel->widget.cols - 1, SLUNI_DSRTEE_CHAR);
    	} else
#endif /* linux_unicode */
	{
            SLsmg_draw_object (panel->widget.y + llines (panel) + 2,
                panel->widget.x, SLSMG_LTEE_CHAR);
            SLsmg_draw_object (panel->widget.y + llines (panel) + 2,
                panel->widget.x + panel->widget.cols - 1, SLSMG_RTEE_CHAR);
        }
    }
#endif /* have_slang */
    
    if (panel->active)
	attrset (REVERSE_COLOR);

    widget_move (&panel->widget, 0, 3);

    trim (strip_home_and_password (panel->cwd), tmp, panel->widget.cols-7);
    addstr (tmp);
    widget_move (&panel->widget, 0, 1);
    addstr ("<");
    widget_move (&panel->widget, 0, panel->widget.cols-2);
    addstr (">");
    widget_move (&panel->widget, 0, panel->widget.cols-3);
    addstr ("v");
    
    if (panel->active)
	standend ();
}
#endif

#ifndef HAVE_TK
/* To be used only by long_frame and full_frame to adjust top_file */
static void
adjust_top_file (WPanel *panel)
{
    int old_top = panel->top_file;
    
    if (panel->selected - old_top > llines (panel))
	panel->top_file = panel->selected;
    if (old_top - panel->count > llines (panel))
	panel->top_file = panel->count - llines (panel);

#ifdef HAVE_TK
    if (old_top != panel->top_file)
	x_adjust_top_file (panel);
#endif
}
#else
#define adjust_top_file(p)
#endif

extern void paint_info_panel (WPanel *panel);

/* Repaints the information that changes after a command */
void
panel_update_contents (WPanel *panel)
{
    show_dir (panel);
#ifdef HAVE_X
    x_fill_panel (panel);
#else
    paint_dir (panel);
#endif
    display_mini_info (panel);
}

void
paint_panel (WPanel *panel)
{
    paint_frame (panel);
    panel_update_contents (panel);
    mini_info_separator (panel);
}

void
Xtry_to_select (WPanel *panel, char *name)
{
    int i;
    char *subdir;
    
    if (!name){
	panel->selected = 0;
	panel->top_file = 0;
	x_adjust_top_file (panel);
	return;
    }

    /* We only want the last component of the directory */
    for (subdir = name + strlen (name) - 1; subdir >= name; subdir--){
	if (*subdir == PATH_SEP){
	    subdir++;
	    break;
	}
    }
    if (subdir < name)
	subdir = name;
    
    /* Search that subdirectory, if found select it */
    for (i = 0; i < panel->count; i++){
	if (strcmp (subdir, panel->dir.list [i].fname))
	    continue;

	if (i != panel->selected){
	    panel->selected = i;
	    panel->top_file = panel->selected - (panel->widget.lines-2)/2;
	    if (panel->top_file < 0)
		panel->top_file = 0;
	    x_adjust_top_file (panel);
	}
	return;
    }

    /* Try to select a file near the file that is missing */
    if (panel->selected >= panel->count){
	panel->selected = panel->count-1;
	panel->top_file = panel->selected - (panel->widget.lines)/2;
	if (panel->top_file < 0)
	    panel->top_file = 0;
	x_adjust_top_file (panel);
    } else
	return;
}

#ifndef PORT_HAS_PANEL_UPDATE_COLS
void
panel_update_cols (Widget *widget, int frame_size)
{
    int cols, origin;
    
    if (horizontal_split){
	widget->cols = COLS;
	return;
    }

    if (frame_size == frame_full){
	cols = COLS;
	origin = 0;
    } else {
	if (widget == get_panel_widget (0)){
	    cols   = first_panel_size;
	    origin = 0;
	} else {
	    cols   = COLS-first_panel_size;
	    origin = first_panel_size;
	}
    }
    
    widget->cols = cols;
    widget->x = origin;
}
#endif

static char *
panel_save_name (WPanel *panel)
{
    extern int saving_setup;
    
    /* If the program is shuting down */
    if ((midnight_shutdown && auto_save_setup) || saving_setup)
	return copy_strings (panel->panel_name, 0);
    else
	return copy_strings ("Temporal:", panel->panel_name, 0);
}

static void
panel_destroy (WPanel *p)
{
    int i;

    char *name = panel_save_name (p);

    panel_save_setup (p, name);
    x_panel_destroy (p);
    clean_dir (&p->dir, p->count);
    
/* save and clean history */
    if (p->dir_history){
	Hist *current, *old;
	history_put (p->hist_name, p->dir_history);
	current = p->dir_history;
	while (current->next)
	    current = current->next;
	while (current){
	    old = current;
	    current = current->prev;
	    free (old->text);
	    free (old);
	}
    }
    free (p->hist_name);

    delete_format (p->format);
    delete_format (p->status_format);
    
    free (p->user_format);
    for (i = 0; i < LIST_TYPES; i++)
	free (p->user_status_format [i]);
    free (p->dir.list);
    free (p->panel_name);
    free (name);
}

static void
panel_format_modified (WPanel *panel)
{
    panel->format_modified = 1;
    x_reset_sort_labels (panel);
}

int
is_a_panel (Widget *w)
{
	return (w->callback == (void *) panel_callback);
}

void directory_history_add (WPanel * panel, char *s);

/* Panel creation */
/* The parameter specifies the name of the panel for setup retieving */
WPanel *
panel_new (char *panel_name)
{
    WPanel *panel;
    char *section;
    int i, err;

    panel = xmalloc (sizeof (WPanel), "panel_new");
    memset (panel, 0, sizeof (WPanel));
    
    /* No know sizes of the panel at startup */
    init_widget (&panel->widget, 0, 0, 0, 0, (callback_fn)
		 panel_callback, (destroy_fn) panel_destroy,
		 (mouse_h) panel_event, NULL);

    /* We do not want the cursor */
    widget_want_cursor (panel->widget, 0);
    
    mc_get_current_wd (panel->cwd, sizeof (panel->cwd)-2);
    strcpy (panel->lwd, ".");

    panel->hist_name = copy_strings ("Dir Hist ", panel_name, 0);
    panel->dir_history = history_get (panel->hist_name);
    directory_history_add (panel, panel->cwd);

    panel->dir.list         = (file_entry *) malloc (MIN_FILES * sizeof (file_entry));
    panel->dir.size         = MIN_FILES;
    panel->active           = 0;
    panel->filter           = 0;
    panel->split            = 0;
    panel->top_file         = 0;
    panel->selected         = 0;
    panel->marked           = 0;
    panel->total            = 0;
    panel->reverse          = 0;
    panel->dirty            = 1;
    panel->searching        = 0;
    panel->dirs_marked      = 0;
    panel->is_panelized     = 0;
    panel->has_dir_sizes    = 0;
    panel->format	    = 0;
    panel->status_format    = 0;    
    panel->format_modified  = 1;
    
    panel->panel_name = strdup (panel_name);
    panel->user_format = strdup (DEFAULT_USER_FORMAT);
    
    for(i = 0; i < LIST_TYPES; i++)
	panel->user_status_format [i] = strdup (DEFAULT_USER_FORMAT);
    
    panel->search_buffer [0] = 0;
    panel->frame_size = frame_half;
    section = copy_strings ("Temporal:", panel->panel_name, 0);
    if (!profile_has_section (section, profile_name)){
	free (section);
	section = strdup (panel->panel_name);
    }
    panel_load_setup (panel, section);
    free (section);

    /* Load format strings */
    err = set_panel_formats (panel);
    if (err){
	if (err & 0x01){
	    free (panel->user_format);
	    panel->user_format = strdup (DEFAULT_USER_FORMAT);
	}
	if (err & 0x02){
	    free (panel->user_status_format [panel->list_type]);
	    panel->user_status_format [panel->list_type] = strdup (DEFAULT_USER_FORMAT);
	}
	set_panel_formats (panel);
    }
    
    /* Load the default format */
    panel->count = do_load_dir (&panel->dir, panel->sort_type,
				panel->reverse, panel->case_sensitive, panel->filter);
    return panel;
}

void
panel_reload (WPanel *panel)
{
    int i;
    struct stat current_stat;

    if (fast_reload
	&& !stat (panel->cwd, &current_stat)
	&& current_stat.st_ctime == panel->dir_stat.st_ctime
	&& current_stat.st_mtime == panel->dir_stat.st_mtime)
	return;

    while (mc_chdir (panel->cwd) == -1){
	char *last_slash;

	if (panel->cwd [0] == PATH_SEP && panel->cwd [1] == 0){
	    clean_dir (&panel->dir, panel->count);
	    panel->count = set_zero_dir (&panel->dir);
	    return;
	}
	last_slash = strrchr (panel->cwd, PATH_SEP);
	if (!last_slash || last_slash == panel->cwd)
	    strcpy (panel->cwd, PATH_SEP_STR);
	else
	    *last_slash = 0;
        bzero (&(panel->dir_stat), sizeof (panel->dir_stat));
	show_dir (panel);
    }
    
    panel->count = do_reload_dir (&panel->dir, panel->sort_type, panel->count,
				  panel->reverse, panel->case_sensitive, panel->filter);
    panel->marked = 0;
    panel->dirs_marked = 0;
    panel->total  = 0;
    panel->has_dir_sizes = 0;

    for (i = 0; i < panel->count; i++)
	if (panel->dir.list [i].f.marked){
	    /* do_file_mark will return immediately if newmark == oldmark.
	       So we have to first unmark it to get panel's summary information
	       updated. (Norbert) */
	    panel->dir.list [i].f.marked = 0;
	    do_file_mark (panel, i, 1);
	}
}

#ifndef PORT_HAS_PAINT_FRAME
void
paint_frame (WPanel *panel)
{
    int  header_len;
    int  spaces, extra;
    int  side, width;
    
    char *txt, buffer[30]; /*Hope that this is enough ;-) */
    if (!panel->split)
	adjust_top_file (panel);
    
    widget_erase (&panel->widget);
    show_dir (panel);

    widget_move (&panel->widget, 1, 1);

    for (side = 0; side <= panel->split; side++){
	format_e *format;
	
	if (side){
	    attrset (NORMAL_COLOR);
	    one_vline ();
	    width = panel->widget.cols - panel->widget.cols/2 - 1;
	} else if (panel->split)
	    width = panel->widget.cols/2 - 3;
	else
	    width = panel->widget.cols - 2;
	
	for (format = panel->format; format; format = format->next){
            if (format->string_fn){
                txt = format->title;
		
		header_len = strlen (txt);
		if (header_len > format->field_len){
		    strcpy (buffer, txt);
		    txt = buffer;
		    txt [format->field_len] = 0;
		    header_len = strlen (txt);
		}
		
                attrset (MARKED_COLOR);
                spaces = (format->field_len - header_len) / 2;
                extra  = (format->field_len - header_len) % 2;
		printw ("%*s%-s%*s", spaces, "",
			 txt, spaces+extra, "");
		width -= 2 * spaces + extra + header_len;
	    } else {
		attrset (NORMAL_COLOR);
		one_vline ();
		width --;
		continue;
	    }
	}
	
	if (width > 0)
	    printw ("%*s", width, "");
    }
}
#endif

static char *
parse_panel_size (WPanel *panel, char *format, int isstatus)
{
    int frame = frame_half;
    format = skip_separators (format);

    if (!strncmp (format, "full", 4)){
	frame = frame_full;
	format += 4;
    } else if (!strncmp (format, "half", 4)){
	frame = frame_half;
	format += 4;
    }

    if (!isstatus){
        panel->frame_size = frame;
        panel->split = 0;
    }
    
    /* Now, the optional column specifier */
    format = skip_separators (format);
    
    if (*format == '1' || *format == '2'){
	if (!isstatus)
	    panel->split = *format == '2';
	format++;
    }

    if (!isstatus)
        panel_update_cols (&(panel->widget), panel->frame_size);
    
    return skip_separators (format);
}

/* Format is:

   all              := panel_format? format
   panel_format     := [full|half] [1|2]
   format           := one_format_e
                     | format , one_format_e

   one_format_e     := just format.id [opt_size]
   just             := [<|>]
   opt_size         := : size [opt_expand]
   size             := [0-9]+
   opt_expand       := +
   
*/

format_e *
parse_display_format (WPanel *panel, char *format, char **error, int isstatus, int *res_total_cols)
{
    format_e *darr, *old, *home = 0;   	/* The formats we return */
    int  total_cols = 0;		/* Used columns by the format */
    int  set_justify;                  	/* flag: set justification mode? */
    int  justify = 0;                  	/* Which mode. */
    int  items = 0;			/* Number of items in the format */
    int  i;

    *error = 0;
    
    /*
     * This makes sure that the panel and mini status full/half mode
     * setting is equal
     */
    format = parse_panel_size (panel, format, isstatus);

    while (*format){       /* format can be an empty string */
	int found = 0;

        darr = xmalloc (sizeof (format_e), "parse_display_format");
	
        /* I'm so ugly, don't look at me :-) */
        if (!home)
            home = old = darr;
	
        old->next = darr;
        darr->next = 0;
        old = darr;
	
	format = skip_separators (format);

	if (*format == '<' || *format == '>'){
	    set_justify = 1;
	    justify = *format == '<' ? J_LEFT : J_RIGHT;
	    format = skip_separators (format+1);
	} else
	    set_justify = 0;
	
	for (i = 0; i < ELEMENTS(formats); i++){
	    int klen = strlen (formats [i].id);

	    if (strncmp (format, formats [i].id, klen) != 0)
		continue;

	    format += klen;

	    if (formats [i].use_in_gui)
	    	items++;

	    darr->use_in_gui          = formats [i].use_in_gui;
            darr->requested_field_len = formats [i].min_size;
            darr->string_fn           = formats [i].string_fn;
	    if (formats [i].title [0])
		    darr->title = _(formats [i].title);
	    else
		    darr->title = "";
            darr->id                  = formats [i].id;
	    darr->expand              = formats [i].expands;
	    
	    if (set_justify)
		darr->just_mode = justify;
	    else
		darr->just_mode = formats [i].default_just;

	    found = 1;

	    format = skip_separators (format);

	    /* If we have a size specifier */
	    if (*format == ':'){
		int req_length;

		/* If the size was specified, we don't want
		 * auto-expansion by default
		 */
		darr->expand = 0;
		format++;
		req_length = atoi (format);
		darr->requested_field_len = req_length;

		format = skip_numbers (format);

		/* Now, if they insist on expansion */
		if (*format == '+'){
		    darr->expand = 1;
		    format++;
		}

	    }

	    break;
	}
	if (!found){
	    char old_char;
	    
	    int pos = min (8, strlen (format));
	    delete_format (home);
	    old_char = format [pos];
	    format [pos] = 0;
	    *error = copy_strings(_("Unknow tag on display format: "), format, 0);
	    format [pos] = old_char;
	    return 0;
	}
	total_cols += darr->requested_field_len;
    }
    
    *res_total_cols = total_cols;
    if (home)
       home->items = items;
    return home;
}

format_e *
use_display_format (WPanel *panel, char *format, char **error, int isstatus)
{
#define MAX_EXPAND 4
    int  expand_top = 0;               /* Max used element in expand */
    int  usable_columns;               /* Usable columns in the panel */
    int  total_cols;
    char *expand_list [MAX_EXPAND];    /* Expand at most 4 fields. */
    int  i;
    format_e *darr, *home;
    
    if (!format)
	format = DEFAULT_USER_FORMAT;
    
    home = parse_display_format (panel, format, error, isstatus, &total_cols);

    if (*error)
	return 0;
	    
    /* Status needn't to be split */
    usable_columns = ((panel->widget.cols-2)/((isstatus)
					      ? 1
					      : (panel->split+1))) - (!isstatus && panel->split);
    
    /* Clean expand list */
    for (i = 0; i < MAX_EXPAND; i++)
        expand_list [i] = '\0';
    
    
    /* Look for the expandable fields and set field_len based on the requested field len */
    for (darr = home; darr && expand_top < MAX_EXPAND; darr = darr->next){
	darr->field_len = darr->requested_field_len;
	if (darr->expand)
	    expand_list [expand_top++] = darr->id;
    }
    
    /* If we used more columns than the available columns, adjust that */
    if (total_cols > usable_columns){
	int pdif, dif = total_cols - usable_columns;
	
        while (dif){
	    pdif = dif;
	    for (darr = home; darr; darr = darr->next){
		if (dif && darr->field_len - 1){
		    darr->field_len--;
		    dif--;
		}
	    }

	    /* avoid endless loop if num fields > 40 */
	    if (pdif == dif)
		break; 
	}
	total_cols  = usable_columns; /* give up, the rest should be truncated */
    }

    /* Expand the available space */
    if ((usable_columns > total_cols) && expand_top){
	int spaces = (usable_columns - total_cols) / expand_top;
	int extra  = (usable_columns - total_cols) % expand_top;

	for (i = 0, darr = home; darr && (i < expand_top); darr = darr->next)
		if (darr->expand){
			darr->field_len += (spaces + ((i == 0) ? extra : 0));
			i++;
		}
    }
    return home;
}

/* Switches the panel to the mode specified in the format 	    */
/* Seting up both format and status string. Return: 0 - on success; */
/* 1 - format error; 2 - status error; 3 - errors in both formats.  */
int
set_panel_formats (WPanel *p)
{
    format_e *form;
    char *err;    
    int retcode = 0;
    
    form = use_display_format (p, panel_format (p), &err, 0);
	
    if (err){
        free (err);
        retcode = 1;
    }
    else {
        if (p->format)
    	    delete_format (p->format);
		
	p->format = form;
    }
    
    if (show_mini_info){
    
	form = use_display_format (p, mini_status_format (p), &err, 1);
	
	if (err){
	    free (err);
	    retcode += 2;
	}
	else {
	    if (p->status_format)
		delete_format (p->status_format);
		
	    p->status_format = form;
	}
    }
    
    panel_format_modified (p);
    panel_update_cols (&(p->widget), p->frame_size);
    
    return retcode;
}

/* Given the panel->view_type returns the format string to be parsed */
char *
panel_format (WPanel *panel)
{
    switch (panel->list_type){

    case list_long:
	return "full perm,space,nlink,space,owner,space,group,space,size,space,mtime,space,name";

    case list_brief:
	return "half 2,type,name";

    case list_user:
	return panel->user_format;

    default:
    case list_full:
	return "half type,name,|,size,|,mtime";
    }
}

char *
mini_status_format (WPanel *panel)
{
    if (panel->user_mini_status)
       return panel->user_status_format [panel->list_type];

    switch (panel->list_type){

    case list_long:
       return "full perm,space,nlink,space,owner,space,group,space,size,space,mtime,space,name";

    case list_brief:
       return "half type,name,space,bsize,space,perm,space";

    case list_full:
       return "half type,name";

    default:
    case list_user:
       return panel->user_format;
    }
}

/*                          */
/* Panel operation commands */
/*                          */

/* Returns the number of items in the given panel */
int
ITEMS (WPanel *p)
{
#ifdef HAVE_TK
    return p->widget.lines;
#else
    if (p->split)
	return llines (p) * 2;
    else
	return llines (p);
#endif
}

/* This function sets redisplays the selection */
void
select_item (WPanel *panel)
{
    int repaint = 0;
    int items = ITEMS (panel);
    
    /* Although currently all over the code we set the selection and
       top file to decent values before calling select_item, I could
       forget it someday, so it's better to do the actual fitting here */

#ifdef HAVE_X
    int old_top;
    old_top = panel->top_file;
#endif
    
    if (panel->top_file < 0){
	repaint = 1;
	panel->top_file = 0;
    }

    if (panel->selected < 0)
	panel->selected = 0;

    if (panel->selected > panel->count-1)
	panel->selected = panel->count - 1;

    if (panel->top_file > panel->count-1){
	repaint = 1;
	panel->top_file = panel->count-1;
    }
    
    if ((panel->count - panel->top_file) < items){
	repaint = 1;
	panel->top_file = panel->count - items;
	if (panel->top_file < 0)
	    panel->top_file = 0;
    }
    
    if (panel->selected < panel->top_file){
	repaint = 1;
	panel->top_file = panel->selected;
    }

    if ((panel->selected - panel->top_file) >= items){
	repaint = 1;
	panel->top_file = panel->selected - items + 1;
    }

#ifndef HAVE_X    
    if (repaint)
	paint_panel (panel);
    else
	repaint_file (panel, panel->selected, 1, 2*selection (panel)->f.marked+1, 0);
#else
    if (old_top != panel->top_file)
	x_adjust_top_file (panel);
    x_select_item (panel);
#endif

    display_mini_info (panel);

    execute_hooks (select_file_hook);
}

/* Clears all files in the panel, used only when one file was marked */
void
unmark_files (WPanel *panel)
{
    int i;

    if (!panel->marked)
	return;
    for (i = 0; i < panel->count; i++)
	file_mark (panel, i, 0);

    panel->dirs_marked = 0;
    panel->marked = 0;
    panel->total = 0;
}

#ifdef HAVE_X
void
unselect_item (WPanel *panel)
{
    x_unselect_item (panel);
}
#else
void
unselect_item (WPanel *panel)
{
    repaint_file (panel, panel->selected, 1, 2*selection (panel)->f.marked, 0);
}
#endif

static void
do_move_down (WPanel *panel)
{
    if (panel->selected+1 == panel->count)
	return;
    
    unselect_item (panel);
    panel->selected++;

#ifndef HAVE_X
    if (panel->selected - panel->top_file == ITEMS (panel) &&
	panel_scroll_pages){
	/* Scroll window half screen */
	panel->top_file += ITEMS (panel)/2;
	if (panel->top_file > panel->count - ITEMS (panel))
		panel->top_file = panel->count - ITEMS (panel);
	paint_dir (panel);
	select_item (panel);
    }
#endif
    select_item (panel);
}

static void
do_move_up (WPanel *panel)
{
    if (panel->selected == 0)
	return;

    unselect_item (panel);
    panel->selected--;
#ifndef HAVE_X
    if (panel->selected < panel->top_file && panel_scroll_pages){
	/* Scroll window half screen */
	panel->top_file -= ITEMS (panel)/2;
	if (panel->top_file < 0) panel->top_file = 0;
	paint_dir (panel);
    }
#endif
    select_item (panel);
}

static int
move_rel (WPanel *panel, int rel)
{
    unselect_item (panel);
    
    if (rel < 0){
	if (panel->selected + rel < 0)
	    panel->selected = 0;
	else
	    panel->selected = panel->selected + rel;
    } else {
	if (panel->selected + rel >= panel->count)
	    panel->selected = panel->count - 1;
	else
	    panel->selected = panel->selected + rel;
    }
    select_item (panel);
}

/*                           */
/* Panel key binded commands */
/*                           */
static void
move_up (WPanel *panel)
{
    if (panel->list_type == list_icons){
	move_rel (panel, -ICONS_PER_ROW (panel));
    } else
	do_move_up (panel);
}

static void
move_down (WPanel *panel)
{
    if (panel->list_type == list_icons){
	move_rel (panel, ICONS_PER_ROW (panel));
    } else
	do_move_down (panel);
}

/* Changes the selection by lines (may be negative) */
static void
move_selection (WPanel *panel, int lines)
{
    int new_pos;
    int adjust = 0;

    new_pos = panel->selected + lines;
    if (new_pos >= panel->count)
	new_pos = panel->count-1;

    if (new_pos < 0)
	new_pos = 0;

    unselect_item (panel);
    panel->selected = new_pos;

#ifndef HAVE_X
    if (panel->selected - panel->top_file >= ITEMS (panel)){
	panel->top_file += lines;
	adjust = 1;
    }
    
    if (panel->selected - panel->top_file < 0){
	panel->top_file += lines;
	adjust = 1;
    }
    
    if (adjust){
	if (panel->top_file > panel->selected)
	    panel->top_file = panel->selected;
	if (panel->top_file < 0)
	    panel->top_file = 0;
	paint_dir (panel);
    }
#endif
    select_item (panel);
}

static int
move_left (WPanel *panel, int c_code)
{
    if (panel->list_type == list_icons){
	do_move_up (panel);
	return 1;
    } else {    
	if (panel->split){
	    move_selection (panel, -llines (panel));
	    return 1;
	} else 
	    return maybe_cd (c_code, 0);
    }
}

static int
move_right (WPanel *panel, int c_code)
{
    if (panel->list_type == list_icons){
	do_move_down (panel);
	return 1;
    } else {
	if (panel->split){
	    move_selection (panel, llines (panel));
	    return 1;
	} else
	    return maybe_cd (c_code, 1);
    }
}

static void
prev_page (WPanel *panel)
{
    int items;

    if (!panel->selected && !panel->top_file)
    	return;
    unselect_item (panel);
    items = ITEMS (panel);
    if (panel->top_file < items)
    	items = panel->top_file;
    if (!items)
    	panel->selected = 0;
    else
    	panel->selected -= items;
    panel->top_file -= items;

    /* This keeps the selection in a reasonable place */
    if (panel->selected < 0)
	panel->selected = 0;
    if (panel->top_file < 0)
	panel->top_file = 0;
    x_adjust_top_file (panel);
    select_item (panel);
#ifndef HAVE_X
    paint_dir (panel);
#endif
}

static void
prev_page_key (WPanel *panel)
{
    if (console_flag && ctrl_pressed ()){
	do_cd ("..", cd_exact);
    } else
	prev_page (panel);
}

static void
next_page (WPanel *panel)
{
    int items;

    if (panel->selected == panel->count - 1)
    	return;
    unselect_item (panel);
    items = ITEMS (panel);
    if (panel->top_file > panel->count - 2 * items)
    	items = panel->count - items - panel->top_file;
    if (panel->top_file + items < 0)
    	items = - panel->top_file;
    if (!items)
    	panel->selected = panel->count - 1;
    else
    	panel->selected += items;
    panel->top_file += items;

    /* This keeps the selection in it's relative position */
    if (panel->selected >= panel->count)
	panel->selected = panel->count - 1;
    if (panel->top_file >= panel->count)
	panel->top_file = panel->count - 1;
    x_adjust_top_file (panel);
    select_item (panel);
#ifndef HAVE_X
    paint_dir (panel);
#endif
}

static void next_page_key (WPanel *panel)
{
    if (console_flag&&ctrl_pressed()&&
	(S_ISDIR(selection (panel)->buf.st_mode) ||
	 link_isdir (selection (panel))))
        do_cd (selection (panel)->fname, cd_exact);
    else
	next_page (panel);
}

static void
goto_top_file (WPanel *panel)
{
    unselect_item (panel);
    panel->selected = panel->top_file;
    select_item (panel);
}

static void
goto_middle_file (WPanel *panel)
{
    unselect_item (panel);
    panel->selected = panel->top_file + (ITEMS (panel)/2);
    if (panel->selected >= panel->count)
	panel->selected = panel->count - 1;
    select_item (panel);
}

static void
goto_bottom_file (WPanel *panel)
{
    unselect_item (panel);
    panel->selected = panel->top_file + ITEMS (panel)-1;
    if (panel->selected >= panel->count)
	panel->selected = panel->count - 1;
    select_item (panel);
}

static void
move_home (WPanel *panel)
{
    if (panel->selected == 0)
	return;
    unselect_item (panel);
    
    if (torben_fj_mode){
	int middle_pos = panel->top_file + (ITEMS (panel)/2);

	if (panel->selected > middle_pos){
	    goto_middle_file (panel);
	    return;
	}
	if (panel->selected != panel->top_file){
	    goto_top_file (panel);
	    return;
	}
    }
    
    panel->top_file = 0;
    panel->selected = 0;

#ifndef HAVE_X
    paint_dir (panel);
#endif
    select_item (panel);
}

static void
move_end (WPanel *panel)
{
    if (panel->selected == panel->count-1)
	return;
    unselect_item (panel);
    if (torben_fj_mode){
	int middle_pos = panel->top_file + (ITEMS (panel)/2);

	if (panel->selected < middle_pos){
	    goto_middle_file (panel);
	    return;
	}
	if (panel->selected != (panel->top_file + ITEMS(panel)-1)){
	    goto_bottom_file (panel);
	    return;
	}
    }
    
    panel->selected = panel->count-1;
#ifndef HAVE_X
    paint_dir (panel);
#endif
    select_item (panel);
}

/* This routine marks a file or a directory */
void
do_file_mark (WPanel *panel, int idx, int mark)
{
    if (panel->dir.list [idx].f.marked == mark)
        return;
    /*
     * Only '..' can't be marked, '.' isn't visible.
     */
    if (strcmp (panel->dir.list [idx].fname, "..")){
	file_mark (panel, idx, mark);
        if (panel->dir.list [idx].f.marked){
            panel->marked++;
            if (S_ISDIR (panel->dir.list [idx].buf.st_mode)) {
		if (panel->has_dir_sizes)
		    panel->total += panel->dir.list [idx].buf.st_size;
                panel->dirs_marked++;
	    } else 
		panel->total += panel->dir.list [idx].buf.st_size;
#ifndef HAVE_XVIEW            
            set_colors (panel);
#endif
        } else {
            if (S_ISDIR(panel->dir.list [idx].buf.st_mode)) {
		if (panel->has_dir_sizes)
		    panel->total -= panel->dir.list [idx].buf.st_size;
                panel->dirs_marked--;
	    } else
		panel->total -= panel->dir.list [idx].buf.st_size;
            panel->marked--;
        }
    }
}

void
do_file_mark_range (WPanel *panel, int r1, int r2)
{
	const int start = min (r1, r2);
	const int end   = max (r1, r2);
	int i, mark;

	mark = !panel->dir.list [start].f.marked;
	
	for (i = start; i < end; i++)
	    do_file_mark (panel, i, mark);
}

static void
do_mark_file (WPanel *panel, int do_move)
{
    int idx = panel->selected;
    do_file_mark (panel, idx, selection (panel)->f.marked ? 0 : 1);
#ifndef HAVE_XVIEW	
    repaint_file (panel, idx, 1, 2*panel->dir.list [idx].f.marked+1, 0);
#endif        

    if (mark_moves_down && do_move)
	move_down (panel);
    display_mini_info (panel);
}

static void
mark_file (WPanel *panel)
{
    do_mark_file (panel, 1);
}

/* Incremental search of a file name in the panel */
static void
do_search (WPanel *panel, int c_code)
{
    int l, i;
    int wrapped = 0;
    int found;

    l = strlen (panel->search_buffer);
    if (l && (c_code == 8 || c_code == 0177 || c_code == KEY_BACKSPACE))
	panel->search_buffer [--l] = 0;
    else {
	if (c_code && l < sizeof (panel->search_buffer)){
	    panel->search_buffer [l] = c_code;
	    panel->search_buffer [l+1] = 0;
	    l++;
	}
    }

    found = 0;
    for (i = panel->selected; !wrapped || i != panel->selected; i++){
	if (i >= panel->count){
	    i = 0;
	    if (wrapped)
		break;
	    wrapped = 1;
	}
	if (STRNCOMP (panel->dir.list [i].fname, panel->search_buffer, l) == 0){
	    unselect_item (panel);
	    panel->selected = i;
	    select_item (panel);
	    found = 1;
	    break;
	}
    }
    if (!found)
	panel->search_buffer [--l] = 0;
#ifndef HAVE_X    
    paint_panel (panel);
#endif
}

static void
start_search (WPanel *panel)
{
    if (panel->searching){
	if (panel->selected+1 == panel->count)
	    panel->selected = 0;
	else
	    move_down (panel);
	do_search (panel, 0);
    } else {
	panel->searching = 1;
	panel->search_buffer [0] = 0;
	display_mini_info (panel);
	mc_refresh ();
    }
}

int
do_enter (WPanel *panel)
{
    if (S_ISDIR (selection (panel)->buf.st_mode)
	|| link_isdir (selection (panel))){
	do_cd (selection (panel)->fname, cd_exact);
	return 1;
    } else { 
	if (is_exe (selection (panel)->buf.st_mode) &&
	    if_link_is_exe (selection (panel))) {
#ifdef USE_VFS	    
	    if (vfs_current_is_local ()) 
#endif	    
	    {
	        char *tmp = name_quote (selection (panel)->fname, 0);
	        char *cmd = copy_strings (".", PATH_SEP_STR, tmp, 0);
	        if (!confirm_execute || (query_dialog (_(" The Midnight Commander "),
						       _(" Do you really want to execute? "),
						       0, 2, _("&Yes"), _("&No")) == 0))
	            execute (cmd);
	        free (tmp);
	        free (cmd);
	    } 
#ifdef USE_VFS	    	    
	    else if (vfs_current_is_extfs ()) {
	        char *tmp = vfs_get_current_dir();
	        char *tmp2;

		tmp2 = concat_dir_and_file (tmp, selection (panel)->fname);
	        extfs_run(tmp2);
	        free (tmp2);
	    }
#endif /* USE_VFS */	    
	    return 1;
	} else {
	    char *p;
	    
	    p = regex_command (selection (panel)->fname, "Open", NULL, 0);
	    if (p && (strcmp (p, "Success") == 0))
		    return 1;
	    else
		    return 0;
	}
    }
}

static void
chdir_other_panel (WPanel *panel)
{
    char *new_dir;

    if (get_other_type () != view_listing)
	return;

    if (!S_ISDIR (panel->dir.list [panel->selected].buf.st_mode))
	new_dir = concat_dir_and_file (panel->cwd, "..");
    else
	new_dir = concat_dir_and_file (panel->cwd, panel->dir.list [panel->selected].fname);

    change_panel ();
    do_cd (new_dir, cd_exact);
    change_panel ();

    move_down (panel);
    
    free (new_dir);
}

static void
chdir_to_readlink (WPanel *panel)
{
    char *new_dir;

    if (get_other_type () != view_listing)
	return;

    if (S_ISLNK (panel->dir.list [panel->selected].buf.st_mode)) {
	char buffer [MC_MAXPATHLEN], *p;
	int i;
	struct stat mybuf;
	
	i = readlink (selection (panel)->fname, buffer, MC_MAXPATHLEN);
	if (i < 0)
	    return;
	if (mc_stat (selection (panel)->fname, &mybuf) < 0)
	    return;
	buffer [i] = 0;
	if (!S_ISDIR (mybuf.st_mode)) {
	    p = strrchr (buffer, PATH_SEP);
	    if (p && !p[1]) {
		*p = 0;
		p = strrchr (buffer, PATH_SEP);
	    }
	    if (!p)
	        return;
	    p[1] = 0;
	}
	if (*buffer == PATH_SEP)
	    new_dir = strdup (buffer);
	else
	    new_dir = concat_dir_and_file (panel->cwd, buffer);

	change_panel ();
	do_cd (new_dir, cd_exact);
	change_panel ();
	
	move_down (panel);
	    
	free (new_dir);
    }
}

static key_map panel_keymap [] = {
    { KEY_DOWN,   move_down },
    { KEY_UP, 	move_up },

    /* The action button :-) */
    { '\n',       do_enter },
    { KEY_ENTER,  do_enter },

    { KEY_IC,     mark_file },
    { KEY_HOME,	  move_home },
    { KEY_C1,     move_end },
    { KEY_END,    move_end },
    { KEY_A1,     move_home },
    { KEY_NPAGE,  next_page_key },
    { KEY_PPAGE,  prev_page_key },

    /* To quickly move in the panel */
    { ALT('g'),   goto_top_file },
    { ALT('r'),   goto_middle_file }, /* M-r like emacs */
    { ALT('j'),   goto_bottom_file },

#ifdef OS2_NT
    { ALT(KEY_F(11)), drive_cmd_a },
    { ALT(KEY_F(12)), drive_cmd_b },
    { ALT('d'),   drive_chg },
#endif
    
    /* Emacs-like bindings */
    { XCTRL('v'), next_page },		/* C-v like emacs */
    { ALT('v'),   prev_page },		/* M-v like emacs */
    { XCTRL('p'), move_up },		/* C-p like emacs */
    { XCTRL('n'), move_down },		/* C-n like emacs */
    { XCTRL('s'), start_search },	/* C-s like emacs */
    { ALT('s'),   start_search },	/* M-s not like emacs */
    { XCTRL('t'), mark_file },
    { ALT('o'),   chdir_other_panel },
    { ALT('l'),   chdir_to_readlink },
    { KEY_F(13),  view_simple_cmd },
    { KEY_F(14),  edit_cmd_new },
    { ALT('y'),   directory_history_prev },
    { ALT('u'),   directory_history_next },
    { ALT('H'),   directory_history_list },
    { ALT('+'),	  select_cmd_panel },
    { KEY_KP_ADD, select_cmd_panel },
    { ALT('\\'),  unselect_cmd_panel },
    { ALT('-'),	  unselect_cmd_panel },
    { KEY_KP_SUBTRACT, unselect_cmd_panel },
    { ALT('*'),	  reverse_selection_cmd_panel },
    { KEY_KP_MULTIPLY, reverse_selection_cmd_panel },
    

#ifdef HAVE_GNOME
    { '+',        select_cmd_panel },
    { '\\',       unselect_cmd_panel },
    { '-',        unselect_cmd_panel },
    { '*',	  reverse_selection_cmd_panel },
    { XCTRL('r'), reread_cmd },
    { KEY_F(3),   view_panel_cmd },
    { KEY_F(5),   copy_cmd },
    { KEY_F(6),   ren_cmd },
    { KEY_F(7),   mkdir_panel_cmd },
    { KEY_F(8),   delete_cmd },
#endif
    
    { 0, 0 }
};
    
static inline int
panel_key (WPanel *panel, int key)
{
    int i;

    for (i = 0; panel_keymap [i].key_code; i++){
	if (key == panel_keymap [i].key_code){
	    if (panel_keymap [i].fn != start_search)
               panel->searching = 0;
	    (*panel_keymap [i].fn)(panel);
	    return 1;
	}
    }
    if (torben_fj_mode && key == ALT('h')) {
	goto_middle_file (panel);
	return 1;
    }

    /* We do not want to take a key press if nothing can be done with it */
    /* The command line widget may do something more usefull */
    if (key == KEY_LEFT)
	return move_left (panel, key);

    if (key == KEY_RIGHT)
	return move_right (panel, key);

    if (is_abort_char (key)) {
	panel->searching = 0;
	display_mini_info (panel);
	return 1;
    }

    /* Do not eat characters not meant for the panel below ' ' (e.g. C-l). */
    if ((key >= ' '&& key <= 255) || key == 8 || key == KEY_BACKSPACE) {
	if (panel->searching){
	    do_search (panel, key);
	    return 1;
	}

	if (!command_prompt) {
	    start_search (panel);
	    do_search (panel, key);
	    return 1;
	}
    }

    return 0;
}

static int
panel_callback (Dlg_head *h, WPanel *panel, int msg, int par)
{
    switch (msg){
    case WIDGET_INIT:
#ifdef HAVE_X
	define_label (h, (Widget *)panel, 1, _("Help"), help_cmd);
	define_label (h, (Widget *)panel, 2, _("Menu"), user_menu_cmd);
	define_label (h, (Widget *)panel, 3, _("View"), view_panel_cmd);
	define_label (h, (Widget *)panel, 4, _("Edit"), edit_panel_cmd);
	define_label (h, (Widget *)panel, 5, _("Copy"), copy_cmd);
	define_label (h, (Widget *)panel, 6, _("RenMov"), ren_cmd);
	define_label (h, (Widget *)panel, 7, _("Mkdir"), mkdir_panel_cmd);
	define_label (h, (Widget *)panel, 8, _("Delete"), delete_cmd);
	x_create_panel (h, h->wdata, panel);
#endif	
	return 1;
	
    case WIDGET_DRAW:
#ifndef HAVE_XVIEW
	paint_panel (panel);
#else
	show_dir (panel);	
#endif
	break;

    case WIDGET_FOCUS:
#ifndef HAVE_GNOME
	current_panel = panel;
#endif
	panel->active = 1;
	if (mc_chdir (panel->cwd) != 0){
	    message (1, " Error ", " Can't chdir to %s \n %s ",
		     panel->cwd, unix_error_string (errno));
	} else
	    subshell_chdir (panel->cwd);
	    
	show_dir (panel);
	focus_select_item (panel);
#ifndef HAVE_X	
	define_label (h, (Widget *)panel, 1, _("Help"), help_cmd);
	define_label (h, (Widget *)panel, 2, _("Menu"), user_menu_cmd);
	define_label (h, (Widget *)panel, 3, _("View"), view_panel_cmd);
	define_label (h, (Widget *)panel, 4, _("Edit"), edit_panel_cmd);
	define_label (h, (Widget *)panel, 5, _("Copy"), copy_cmd);
	define_label (h, (Widget *)panel, 6, _("RenMov"), ren_cmd);
	define_label (h, (Widget *)panel, 7, _("Mkdir"), mkdir_panel_cmd);
	define_label (h, (Widget *)panel, 8, _("Delete"), delete_cmd);
	redraw_labels (h, (Widget *)panel);
#endif
	/* Chain behaviour */
	default_proc (h, WIDGET_FOCUS, par);
	return 1;
	
    case WIDGET_UNFOCUS:
	/* Janne: look at this for the multiple panel options */
	if (panel->searching){
	    panel->searching = 0;
	    display_mini_info (panel);
	}
#ifdef HAVE_X	
	show_dir (panel);
	unfocus_unselect_item (panel);
	panel->active = 0;
#else
	panel->active = 0;
	show_dir (panel);
	unselect_item (panel);
#endif
	return 1;
	
    case WIDGET_KEY:
	return panel_key (panel, par);
	break;
    }
    return default_proc (h, msg, par);
}

/*                                     */
/* Panel mouse events support routines */
/*                                     */
static int mouse_marking = 0;

static void
mouse_toggle_mark (WPanel *panel)
{
    do_mark_file (panel, 0);
    mouse_marking = selection (panel)->f.marked;
}

static void
mouse_set_mark (WPanel *panel)
{
    if (mouse_marking && !(selection (panel)->f.marked))
	do_mark_file (panel, 0);
    else if (!mouse_marking && (selection (panel)->f.marked))
	do_mark_file (panel, 0);
}

static inline int
mark_if_marking (WPanel *panel, Gpm_Event *event)
{
    if (event->buttons & GPM_B_RIGHT){
	if (event->type & GPM_DOWN)
	    mouse_toggle_mark (panel);
	else
	    mouse_set_mark (panel);
	return 1;
    }
    return 0;
}

void
file_mark (WPanel *panel, int index, int val)
{
    panel->dir.list [index].f.marked = val;
    x_panel_select_item (panel, index, val);
}

#ifdef PORT_WANTS_GET_SORT_FN
sortfn *
get_sort_fn (char *name)
{
    int i;

    /* First, try the long name options, from dir.c */
    for (i = 0; i < SORT_TYPES_TOTAL; i++)
	if (strcmp (name, sort_orders [i].sort_name) == 0)
	    return (sortfn *)sort_orders [i].sort_fn;

    /* Then try the short name options, from our local table */
    for (i = 0; i < ELEMENTS (formats); i++){
	if (strcmp (name, formats [i].id) == 0 && formats [i].sort_routine)
	    return formats [i].sort_routine;
    }
    return NULL;
}

/* not static because it's called from Tk's version */
int
panel_event (Gpm_Event *event, WPanel *panel)
{
    const int lines = panel->count;
    
    int my_index;
    extern void change_panel (void);

    event->y -= 2;
    if ((event->type & (GPM_DOWN|GPM_DRAG))){

	if (panel != (WPanel *) current_dlg->current->widget)
	    change_panel ();

	if (event->y <= 0){
	    mark_if_marking (panel, event);
	    return MOU_REPEAT;
	}

	if (!((panel->top_file + event->y <= panel->count) &&
	      event->y <= lines)){
	    mark_if_marking (panel, event);
	    return MOU_REPEAT;
	}
	my_index = panel->top_file + event->y - 1;
	if (panel->split){
	    if (event->x > ((panel->widget.cols-2)/2))
		my_index += llines (panel);
	}

	if (my_index >= panel->count)
	    my_index = panel->count - 1;
	
	if (my_index != panel->selected){
	    unselect_item (panel);
	    panel->selected = my_index;
	    select_item (panel);
	}

	/* This one is new */
	mark_if_marking (panel, event);
	
    } else if ((event->type & (GPM_UP|GPM_DOUBLE)) == (GPM_UP|GPM_DOUBLE)){
            if (event->y > 0 && event->y <= lines)
		do_enter (panel);
    }
    return MOU_NORMAL;
}

#else

int
panel_event (Gpm_Event *event, WPanel *panel)
{
    const int lines = llines (panel);
    
    int my_index;
    extern void change_panel (void);

    if (event->type & GPM_DOWN && event->x == 1 + 1 && event->y == 0 + 1) {
	directory_history_prev (panel);
	return MOU_NORMAL;
    }

    if (event->type & GPM_DOWN && event->x == panel->widget.cols - 2 + 1 && event->y == 0 + 1) {
	directory_history_next (panel);
	return MOU_NORMAL;
    }

    if (event->type & GPM_DOWN && event->x == panel->widget.cols - 3 + 1 && event->y == 0 + 1) {
	directory_history_list (panel);
	return MOU_NORMAL;
    }

    event->y -= 2;
    if ((event->type & (GPM_DOWN|GPM_DRAG))){

	if (panel != (WPanel *) current_dlg->current->widget)
	    change_panel ();

	if (event->y <= 0){
	    mark_if_marking (panel, event);
	    if (mouse_move_pages)
		prev_page (panel);
	    else
		move_up (panel);
	    return MOU_REPEAT;
	}

	if (!((panel->top_file + event->y <= panel->count) &&
	      event->y <= lines)){
	    mark_if_marking (panel, event);
	    if (mouse_move_pages)
		next_page (panel);
	    else
		move_down (panel);
	    return MOU_REPEAT;
	}
	my_index = panel->top_file + event->y - 1;
	if (panel->split){
	    if (event->x > ((panel->widget.cols-2)/2))
		my_index += llines (panel);
	}

	if (my_index >= panel->count)
	    my_index = panel->count - 1;
	
	if (my_index != panel->selected){
	    unselect_item (panel);
	    panel->selected = my_index;
	    select_item (panel);
	}

	/* This one is new */
	mark_if_marking (panel, event);
	
    } else if ((event->type & (GPM_UP|GPM_DOUBLE)) == (GPM_UP|GPM_DOUBLE)){
            if (event->y > 0 && event->y <= lines)
		do_enter (panel);
    }
    return MOU_NORMAL;
}
#endif

void
panel_update_marks (WPanel *panel)
{
#ifdef PORT_HAS_UPDATE_MARKS
	x_panel_update_marks (panel);
#endif
}

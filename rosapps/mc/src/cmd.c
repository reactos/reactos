/* Routines invoked by a function key
   They normally operate on the current panel.
   
   Copyright (C) 1994, 1995 Miguel de Icaza
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
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#include <config.h>
#ifdef __os2__
#  define INCL_DOSFILEMGR
#  define INCL_DOSMISC
#  define INCL_DOSERROR
#endif
#ifdef HAVE_UNISTD_H
#    include <unistd.h>
#endif
#include "tty.h"
#include <stdio.h>
#include <stdlib.h>		/* getenv (), rand */
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#if defined(_MSC_VER)
#include <sys/time.h___>
#else
#include <time.h>
#endif
#include <malloc.h>
#include <string.h>
#include <fcntl.h>		/* open, O_RDWR */
#include <errno.h>

#ifdef OS2_NT
#   include <io.h>
#endif

#ifdef USE_NETCODE
#include <netdb.h>
#endif

#ifdef HAVE_MMAP
#   include <sys/mman.h>
#endif
#include "mad.h"
#include "dir.h"
#include "util.h"
#include "panel.h"
#include "cmd.h"		/* Our definitions */
#include "view.h"		/* view() */
#include "dialog.h"		/* query_dialog, message */
#include "file.h"		/* the file operations */
#include "find.h"		/* do_find */
#include "hotlist.h"
#include "tree.h"
#include "subshell.h"		/* use_subshell */
#include "cons.saver.h"
#include "global.h"
#include "dlg.h"		/* required by wtools.h */
#include "widget.h"		/* required by wtools.h */
#include "wtools.h"		/* listbox */
#include "command.h"		/* for input_w */
#include "win.h"		/* do_exit_ca_mode */
#include "layout.h"		/* get_current/other_type */
#include "ext.h"		/* regex_command */
#include "view.h"		/* view */
#include "key.h"		/* get_key_code */
#include "help.h"		/* interactive_display */
#include "fs.h"
#include "boxes.h"		/* cd_dialog */
#include "color.h"
#include "user.h"
#include "setup.h"
#include "x.h"
#include "profile.h"

#define MIDNIGHT
#ifdef  USE_INTERNAL_EDIT
    extern int edit (const char *file, int line);
#endif
#include "../vfs/vfs.h"
#define WANT_WIDGETS
#include "main.h"		/* global variables, global functions */
#ifndef MAP_FILE
#   define MAP_FILE 0
#endif

#ifdef HAVE_TK
#    include "tkscreen.h"
#endif

/* If set and you don't have subshell support,then C-o will give you a shell */
int output_starts_shell = 0;

/* Source routing destination */
int source_route = 0;

/* If set, use the builtin editor */
int use_internal_edit = 1;

/* Ugly hack in order to distinguish between left and right panel in menubar */
int is_right;
#define MENU_PANEL_IDX  (is_right ? 1 : 0)


#ifndef PORT_HAS_FILTER_CHANGED
#    define x_filter_changed(p)
#endif

/* This is used since the parameter panel on some of the commands */
/* defined in this file may receive a 0 parameter if they are invoked */
/* The drop down menu */

WPanel *get_a_panel (WPanel *panel)
{
    if (panel)
	return panel;
    if (get_current_type () == view_listing){
	return cpanel;
    } else
	return other_panel;
}

/* view_file (filename, normal, internal)
 *
 * Inputs:
 *   filename:   The file name to view
 *   plain_view: If set does not do any fancy pre-processing (no filtering) and
 *               always invokes the internal viewer.
 *   internal:   If set uses the internal viewer, otherwise an external viewer.
 */
int view_file_at_line (char *filename, int plain_view, int internal, int start_line)
{
    static char *viewer = 0;
    int move_dir = 0;


    if (plain_view) {
        int changed_hex_mode = 0;
        int changed_nroff_flag = 0;
        int changed_magic_flag = 0;
    
        altered_hex_mode = 0;
        altered_nroff_flag = 0;
        altered_magic_flag = 0;
        if (default_hex_mode)
            changed_hex_mode = 1;
        if (default_nroff_flag)
            changed_nroff_flag = 1;
        if (default_magic_flag)
            changed_magic_flag = 1;
        default_hex_mode = 0;
        default_nroff_flag = 0;
        default_magic_flag = 0;
        view (0, filename, &move_dir, start_line);
        if (changed_hex_mode && !altered_hex_mode)
            default_hex_mode = 1;
        if (changed_nroff_flag && !altered_nroff_flag)
            default_nroff_flag = 1;
        if (changed_magic_flag && !altered_magic_flag)
            default_magic_flag = 1;
        repaint_screen ();
        return move_dir;
    }
    if (internal){
	char view_entry [32];

	if (start_line != 0)
	    sprintf (view_entry, "View:%d", start_line);
	else
	    strcpy (view_entry, "View");
	
	if (!regex_command (filename, view_entry, NULL, &move_dir)){
	    view (0, filename, &move_dir, start_line);
	    repaint_screen ();
	}
    } else {
	char *localcopy;
	
	if (!viewer){
	    viewer = getenv ("PAGER");
	    if (!viewer)
		viewer = "view";
	}
	/* The file may be a non local file, get a copy */
	if (!vfs_file_is_local (filename)){
	    localcopy = mc_getlocalcopy (filename);
	    if (localcopy == NULL){
		message (1, MSG_ERROR, _(" Can not fetch a local copy of %s "), filename);
		return 0;
	    }
	    execute_internal (viewer, localcopy);
	    mc_ungetlocalcopy (filename, localcopy, 0);
	} else 
	    execute_internal (viewer, filename);
    }
    return move_dir;
}

int
view_file (char *filename, int plain_view, int internal)
{
    return view_file_at_line (filename, plain_view, internal, 0);
}

/* scan_for_file (panel, idx, direction)
 *
 * Inputs:
 *   panel:     pointer to the panel on which we operate
 *   idx:       starting file.
 *   direction: 1, or -1
 */
static int scan_for_file (WPanel *panel, int idx, int direction)
{
    int i = idx + direction;

    while (i != idx){
	if (i < 0)
	    i = panel->count - 1;
	if (i == panel->count)
	    i = 0;
	if (!S_ISDIR (panel->dir.list [i].buf.st_mode))
	    return i;
	i += direction;
    }
    return i;
}

/* do_view: Invoked as the F3/F13 key. */
static void do_view_cmd (WPanel *panel, int normal)
{
    int dir, file_idx;
    panel = get_a_panel (panel);
    
    /* Directories are viewed by changing to them */
    if (S_ISDIR (selection (panel)->buf.st_mode) ||
	link_isdir (selection (panel))){
	if (confirm_view_dir && (panel->marked || panel->dirs_marked)){
	    if (query_dialog (_(" CD "), _("Files tagged, want to cd?"),
			      0, 2, _("&Yes"), _("&No")) == 1){
		return;
	    }
	}
	do_cd (selection (panel)->fname, cd_exact);
	return;
	
    }

    file_idx = panel->selected;
    while (1) {
	char *filename;

	filename = panel->dir.list [file_idx].fname;

	dir = view_file (filename, normal, use_internal_view);
	if (dir == 0)
	    break;
	file_idx = scan_for_file (panel, file_idx, dir);
    }
}

void view_cmd (WPanel *panel)
{
    do_view_cmd (panel, 0);
}

void view_simple_cmd (WPanel *panel)
{
    do_view_cmd (panel, 1);
}

void filtered_view_cmd (WPanel *panel)
{
    char *command;

    panel = get_a_panel (panel);
    command = input_dialog (_(" Filtered view "), _(" Filter command and arguments:"),
			    selection (panel)->fname);
    if (!command)
	return;

    view (command, "", 0, 0);

    free (command);
}

void filtered_view_cmd_cpanel (void)
{
    filtered_view_cmd (cpanel);
}

void do_edit_at_line (const char *what, int start_line)
{
    static char *editor = 0;

#ifdef USE_INTERNAL_EDIT
    if (use_internal_edit){
	edit (what, start_line);
	reread_cmd ();
	return;
    }
#endif
    if (!editor){
	editor = getenv ("EDITOR");
	if (!editor)
	    editor = get_default_editor ();
    }
    execute_internal (editor, what);
    reread_cmd ();
}

void
do_edit (const char *what)
{
    do_edit_at_line (what, 1);
}

void edit_cmd (WPanel *panel)
{
    panel = get_a_panel(panel);
    if (!regex_command (selection (panel)->fname, "Edit", NULL, 0))
        do_edit (selection (panel)->fname);
}

void edit_cmd_new (WPanel *panel)
{
    do_edit ("");
}

void copy_cmd (void)
{
    save_cwds_stat ();
    if (panel_operate (cpanel, OP_COPY, NULL)){
	update_panels (UP_OPTIMIZE, UP_KEEPSEL);
	repaint_screen ();
    }
}

void ren_cmd (void)
{
    save_cwds_stat ();
    if (panel_operate (cpanel, OP_MOVE, NULL)){
	update_panels (UP_OPTIMIZE, UP_KEEPSEL);
	repaint_screen ();
    }
}

void copymove_cmd_with_default (int copy, char *thedefault)
{
    save_cwds_stat ();
    if (panel_operate (cpanel, copy ? OP_COPY : OP_MOVE, thedefault)){
	update_panels (UP_OPTIMIZE, UP_KEEPSEL);
	repaint_screen ();
    }
}

void mkdir_cmd (WPanel *panel)
{
    char *dir;
    
    panel = get_a_panel (panel);
    dir = input_expand_dialog (_(" Mkdir "), _(" Enter directory name:") , "");
    
    if (!dir)
	return;

    save_cwds_stat ();
    if (my_mkdir (dir, 0777) == 0){
	update_panels (UP_OPTIMIZE, dir);
	repaint_screen ();
	select_item (cpanel);
	free (dir);
	return;
    }
    free (dir);
    message (1, MSG_ERROR, "  %s  ", unix_error_string (errno));
}

void delete_cmd (void)
{
    save_cwds_stat ();

    if (panel_operate (cpanel, OP_DELETE, NULL)){
	update_panels (UP_OPTIMIZE, UP_KEEPSEL);
	repaint_screen ();
    }
}

void find_cmd (void)
{
    do_find ();
}

void
set_panel_filter_to (WPanel *p, char *allocated_filter_string)
{
    if (p->filter){
	free (p->filter);
	p->filter = 0;
    }
    if (!(allocated_filter_string [0] == '*' && allocated_filter_string [1] == 0))
	p->filter = allocated_filter_string;
    else
	free (allocated_filter_string);
    reread_cmd ();
    x_filter_changed (p);
}

/* Set a given panel filter expression */
void set_panel_filter (WPanel *p)
{
    char *reg_exp;
    char *x;
    
    x = p->filter ? p->filter : easy_patterns ? "*" : ".";
	
    reg_exp = input_dialog (_(" Filter "), _(" Set expression for filtering filenames"), x);
    if (!reg_exp)
	return;
    set_panel_filter_to (p, reg_exp);
}

/* Invoked from the left/right menus */
void filter_cmd (void)
{
    WPanel *p;

    if (!SELECTED_IS_PANEL)
	return;

    p = MENU_PANEL;
    set_panel_filter (p);
}

void reread_cmd (void)
{
    int flag;

    mad_check (__FILE__, __LINE__);
    if (get_current_type () == view_listing &&
	get_other_type () == view_listing)
	flag = strcmp (cpanel->cwd, opanel->cwd) ? UP_ONLY_CURRENT : 0;
    else
	flag = UP_ONLY_CURRENT;
	
    update_panels (UP_RELOAD|flag, UP_KEEPSEL);
    repaint_screen ();
}

/* Panel sorting related routines */
void do_re_sort (WPanel *panel)
{
    char *filename;
    int  i;

    panel = get_a_panel (panel);
    filename = strdup (selection (cpanel)->fname);
    unselect_item (panel);
    do_sort (&panel->dir, panel->sort_type, panel->count-1, panel->reverse, panel->case_sensitive);
    panel->selected = -1;
    for (i = panel->count; i; i--){
	if (!strcmp (panel->dir.list [i-1].fname, filename)){
	    panel->selected = i-1;
	    break;
	}
    }
    free (filename);
    cpanel->top_file = cpanel->selected - ITEMS (cpanel)/2;
    if (cpanel->top_file < 0)
	cpanel->top_file = 0;
    select_item (panel);
    panel_update_contents (panel);
}

void reverse_selection_cmd_panel (WPanel *panel)
{
    file_entry *file;
    int i;

    for (i = 0; i < panel->count; i++){
	file = &panel->dir.list [i];
	if (S_ISDIR (file->buf.st_mode))
	    continue;
	do_file_mark (panel, i, !file->f.marked);
    }
    paint_panel (panel);
}

void reverse_selection_cmd (void)
{
    reverse_selection_cmd_panel (cpanel);
}

void select_cmd_panel (WPanel *panel)
{
    char *reg_exp, *reg_exp_t;
    int i;
    int c;
    int dirflag = 0;

    reg_exp = input_dialog (_(" Select "), "", easy_patterns ? "*" : ".");
    if (!reg_exp)
	return;
    
    reg_exp_t = reg_exp;

    /* Check if they specified a directory */
    if (*reg_exp_t == PATH_SEP){
        dirflag = 1;
        reg_exp_t++;
    }
    if (reg_exp_t [strlen(reg_exp_t) - 1] == PATH_SEP){
        dirflag = 1;
        reg_exp_t [strlen(reg_exp_t) - 1] = 0;
    }

    for (i = 0; i < panel->count; i++){
        if (!strcmp (panel->dir.list [i].fname, ".."))
            continue;
	if (S_ISDIR (panel->dir.list [i].buf.st_mode)){
	    if (!dirflag)
                continue;
        } else {
            if (dirflag)
                continue;
	}
	c = regexp_match (reg_exp_t, panel->dir.list [i].fname, match_file);
	if (c == -1){
	    message (1, MSG_ERROR, _("  Malformed regular expression  "));
	    free (reg_exp);
	    return;
	}
	if (c){
	    do_file_mark (panel, i, 1);
	}
    }
    paint_panel (panel);
    free (reg_exp);
}

void select_cmd (void)
{
	select_cmd_panel (cpanel);
}

void unselect_cmd_panel (WPanel *panel)
{
    char *reg_exp, *reg_exp_t;
    int i;
    int c;
    int dirflag = 0;

    reg_exp = input_dialog (_(" Unselect "),"", easy_patterns ? "*" : ".");
    if (!reg_exp)
	return;
    
    reg_exp_t = reg_exp;
    
    /* Check if they specified directory matching */
    if (*reg_exp_t == PATH_SEP){
        dirflag = 1;
	reg_exp_t ++;
    }
    if (reg_exp_t [strlen(reg_exp_t) - 1] == PATH_SEP){
        dirflag = 1;
        reg_exp_t [strlen(reg_exp_t) - 1] = 0;
    }
    for (i = 0; i < panel->count; i++){
        if (!strcmp (panel->dir.list [i].fname, "..")) 
            continue;
	if (S_ISDIR (panel->dir.list [i].buf.st_mode)){
	    if (!dirflag)
	        continue;
        } else {
            if (dirflag)
                continue;
        }
	c = regexp_match (reg_exp_t, panel->dir.list [i].fname, match_file);
	if (c == -1){
	    message (1, MSG_ERROR, _("  Malformed regular expression  "));
	    free (reg_exp);
	    return;
	}
	if (c){
	    do_file_mark (panel, i, 0);
	}
    }
    paint_panel (panel);
    free (reg_exp);
}

void unselect_cmd (void)
{
	unselect_cmd_panel (cpanel);
}

/* Check if the file exists */
/* If not copy the default */
static int check_for_default(char *default_file, char *file)
{
    struct stat s;
    if (mc_stat (file, &s)){
	if (mc_stat (default_file, &s)){
	    return -1;
	}
	create_op_win (OP_COPY, 0);
        file_mask_defaults ();
	copy_file_file (default_file, file, 1);
	destroy_op_win ();
    }
    return 0;
}

void ext_cmd (void)
{
    char *buffer;
    char *extdir;
    int  dir;

    dir = 0;
    if (geteuid () == 0){
	dir = query_dialog (_("Extension file edit"),
			    _(" Which extension file you want to edit? "), 0, 2,
			    _("&User"), _("&System Wide"));
    }
    extdir = concat_dir_and_file (mc_home, MC_LIB_EXT);

    if (dir == 0){
	buffer = concat_dir_and_file (home_dir, MC_USER_EXT);
	check_for_default (extdir, buffer);
	do_edit (buffer);
	free (buffer);
    } else if (dir == 1)
	do_edit (extdir);

   free (extdir);
   flush_extension_file ();
}

void menu_edit_cmd (void)
{
    char *buffer;
    char *menufile;
    int dir = 0;
    
    dir = query_dialog (
	_("Menu file edit"),
	_(" Which menu file will you edit? "), 
	0, geteuid() ? 2 : 3,
	_("&Local"), _("&Home"), _("&System Wide")
    );

    menufile = concat_dir_and_file(mc_home, MC_GLOBAL_MENU);

    switch (dir){
	case 0:
	    buffer = strdup (MC_LOCAL_MENU);
	    check_for_default (menufile, buffer);
	    break;

	case 1:
	    buffer = concat_dir_and_file (home_dir, MC_HOME_MENU);
	    check_for_default (menufile, buffer);
	    break;
	
	case 2:
	    buffer = concat_dir_and_file (mc_home, MC_GLOBAL_MENU);
	    break;

	default:
	    free (menufile);
	    return;
    }
    do_edit (buffer);
	if (dir == 0)
		chmod(buffer, 0600);
    free (buffer);
    free (menufile);
}

void quick_chdir_cmd (void)
{
    char *target;

    target = hotlist_cmd (LIST_HOTLIST);
    if (!target)
	return;

    if (get_current_type () == view_tree)
	tree_chdir (the_tree, target);
    else
	do_cd (target, cd_exact);
    free (target);
}

#ifdef USE_VFS
void reselect_vfs (void)
{
    char *target;

    target = hotlist_cmd (LIST_VFSLIST);
    if (!target)
	return;

    do_cd (target, cd_exact);
    free (target);
}
#endif

static int compare_files (char *name1, char *name2, long size)
{
    int file1, file2;
    char *data1, *data2;
    int result = -1;		/* Different by default */

    file1 = open (name1, O_RDONLY);
    if (file1 >= 0){
	file2 = open (name2, O_RDONLY);
	if (file2 >= 0){
#ifdef HAVE_MMAP
	    /* Ugly if jungle */
	    data1 = mmap (0, size, PROT_READ, MAP_FILE | MAP_PRIVATE, file1, 0);
	    if (data1 != (char*) -1){
		data2 = mmap (0, size, PROT_READ, MAP_FILE | MAP_PRIVATE, file2, 0);
		if (data2 != (char*) -1){
		    rotate_dash ();
		    result = memcmp (data1, data2, size);
		    munmap (data2, size);
		}
		munmap (data1, size);
	    }
#else
	    /* Don't have mmap() :( Even more ugly :) */
	    char buf1[BUFSIZ], buf2[BUFSIZ];
	    int n1, n2;
	    rotate_dash ();
	    do
	    {
		while((n1 = read(file1,buf1,BUFSIZ)) == -1 && errno == EINTR);
		while((n2 = read(file2,buf2,BUFSIZ)) == -1 && errno == EINTR);
	    } while (n1 == n2 && n1 == BUFSIZ && !memcmp(buf1,buf2,BUFSIZ));
	    result = (n1 != n2) || memcmp(buf1,buf2,n1);
#endif
	    close (file2);
	}
	close (file1);
    }
    return result;
}

enum CompareMode {
    compare_quick, compare_size_only, compare_thourough
};

static void
compare_dir (WPanel *panel, WPanel *other, enum CompareMode mode)
{
    int i, j;
    char *src_name, *dst_name;

    panel = get_a_panel (panel);
    
    /* No marks by default */
    panel->marked = 0;
    panel->total = 0;
    panel->dirs_marked = 0;
    
    /* Handle all files in the panel */
    for (i = 0; i < panel->count; i++){
	file_entry *source = &panel->dir.list[i];

	/* Default: unmarked */
	file_mark (panel, i, 0);

	/* Skip directories */
	if (S_ISDIR (source->buf.st_mode))
	    continue;

	/* Search the corresponding entry from the other panel */
	for (j = 0; j < other->count; j++){
	    if (strcmp (source->fname,
			other->dir.list[j].fname) == 0)
		break;
	}
	if (j >= other->count)
	    /* Not found -> mark */
	    do_file_mark (panel, i, 1);
	else {
	    /* Found */
	    file_entry *target = &other->dir.list[j];

	    if (mode != compare_size_only){
		/* Older version is not marked */
		if (source->buf.st_mtime < target->buf.st_mtime)
		    continue;
	    }
	    
	    /* Newer version with different size is marked */
	    if (source->buf.st_size != target->buf.st_size){
		do_file_mark (panel, i, 1);
		continue;
		
	    }
	    if (mode == compare_size_only)
		continue;
	    
	    if (mode == compare_quick){
		/* Thorough compare off, compare only time stamps */
		/* Mark newer version, don't mark version with the same date */
		if (source->buf.st_mtime > target->buf.st_mtime){
		    do_file_mark (panel, i, 1);
		}
		continue;
	    }

	    /* Thorough compare on, do byte-by-byte comparison */
	    src_name = get_full_name (panel->cwd, source->fname);
	    dst_name = get_full_name (other->cwd, target->fname);
	    if (compare_files (src_name, dst_name, source->buf.st_size))
		do_file_mark (panel, i, 1);
	    free (src_name);
	    free (dst_name);
	}
    } /* for (i ...) */
}

void compare_dirs_cmd (void)
{
    enum CompareMode thorough_flag = compare_quick;

    thorough_flag = query_dialog (_(" Compare directories "), _(" Select compare method: "),
				  0, 3, _("&Quick"), _("&Size only"), _("&Thorough"), _("&Cancel"));
    if (thorough_flag < 0 || thorough_flag > 2)
	return;
    if (get_current_type () == view_listing &&
	get_other_type () == view_listing){
	compare_dir (cpanel, opanel, thorough_flag);
	compare_dir (opanel, cpanel, thorough_flag);
	paint_panel (cpanel);
	paint_panel (opanel);
    } else {
	message (1, MSG_ERROR, _(" Both panels should be on the listing view mode to use this command "));
    }
}

void history_cmd (void)
{
    Listbox *listbox;
    Hist *current;

    if (input_w (cmdline)->need_push){
	if (push_history (input_w (cmdline), input_w (cmdline)->buffer) == 2)
	    input_w (cmdline)->need_push = 0;
    }
    if (!input_w (cmdline)->history){
	message (1, MSG_ERROR, _(" The command history is empty "));
	return;
    }
    current = input_w (cmdline)->history;
    while (current->prev)
	current = current->prev;
    listbox = create_listbox_window (60, 10, _(" Command history "),
				     "[Command Menu]");
    while (current){
	LISTBOX_APPEND_TEXT (listbox, 0, current->text,
			     current);
	current = current->next;
    }
    run_dlg (listbox->dlg);
    if (listbox->dlg->ret_value == B_CANCEL)
	current = NULL;
    else
	current = listbox->list->current->data;
    destroy_dlg (listbox->dlg);
    free (listbox);

    if (!current)
	return;
    input_w (cmdline)->history = current;
    assign_text (input_w (cmdline), input_w (cmdline)->history->text);
    update_input (input_w (cmdline), 1);
}

#if !defined(HAVE_XVIEW) && !defined(HAVE_GNOME)
void swap_cmd (void)
{
    swap_panels ();
    touchwin (stdscr);
    repaint_screen ();
}
#endif

void
view_other_cmd (void)
{
    static int message_flag = TRUE;
#ifdef HAVE_SUBSHELL_SUPPORT
    char *new_dir = NULL;
    char **new_dir_p;
#endif

    if (!xterm_flag && !console_flag && !use_subshell){
	if (message_flag)
	    message (1, MSG_ERROR, _(" Not an xterm or Linux console; \n"
				     " the panels cannot be toggled. "));
	message_flag = FALSE;
    } else {
#ifndef HAVE_X
	if (use_mouse_p)
	    shut_mouse ();
	if (clear_before_exec)
	    clr_scr ();
        if (alternate_plus_minus)
            numeric_keypad_mode ();
#endif
#ifndef HAVE_SLANG
	/* With slang we don't want any of this, since there
	 * is no mc_raw_mode supported
	 */
	reset_shell_mode ();
	noecho ();
#endif
	keypad(stdscr, FALSE);
	endwin ();
	if (!status_using_ncurses)
	    do_exit_ca_mode ();
	mc_raw_mode ();
	if (console_flag)
	    restore_console ();

#ifdef HAVE_SUBSHELL_SUPPORT
	if (use_subshell){
	    new_dir_p = vfs_current_is_local () ? &new_dir : NULL;
	    if (invoke_subshell (NULL, VISIBLY, new_dir_p))
		quiet_quit_cmd();  /* User did `exit' or `logout': quit MC quietly */
	} else
#endif
	{
	    if (output_starts_shell){
		fprintf (stderr,
		 _("Type `exit' to return to the Midnight Commander\n\r\n\r"));
		my_system (EXECUTE_AS_SHELL, shell, NULL);
	    } else
		get_key_code (0);
	}
	if (console_flag)
	    handle_console (CONSOLE_SAVE);

	if (!status_using_ncurses)
	    do_enter_ca_mode ();

	reset_prog_mode ();
	keypad(stdscr, TRUE);
#ifndef HAVE_X	
	if (use_mouse_p)
	    init_mouse ();
        if (alternate_plus_minus)
            application_keypad_mode ();
#endif	    

#ifdef HAVE_SUBSHELL_SUPPORT
	if (use_subshell){
	    load_prompt (0, 0);
	    if (new_dir)
		do_possible_cd (new_dir);
	    if (console_flag && output_lines)
		show_console_contents (output_start_y,
				       LINES-keybar_visible-output_lines-1,
				       LINES-keybar_visible-1);
	}
#endif
        touchwin (stdscr);

	/* prevent screen flash when user did 'exit' or 'logout' within
	   subshell */
	if (!quit)
	    repaint_screen ();
    }
}

#ifndef OS2_NT
static void
do_link (int symbolic_link, char *fname)
{
    struct stat s;
    char *dest, *src;
    int  stat_r;

    if (!symbolic_link){
	stat_r = mc_stat (fname, &s);
	if (stat_r != 0){
	    message (1, MSG_ERROR, _(" Couldn't stat %s \n %s "),
		     fname, unix_error_string (errno));
	    return;
	}
	if (!S_ISREG (s.st_mode))
	    return;
    }
    
    if (!symbolic_link){
        src = copy_strings (_(" Link "), name_trunc (fname, 46), 
            _(" to:"), NULL);
	dest = input_expand_dialog (_(" Link "), src, "");
	free (src);
	if (!dest)
	    return;
	if (!*dest) {
	    free (dest);
	    return;
	}
	save_cwds_stat ();
	if (-1 == mc_link (fname, dest))
	    message (1, MSG_ERROR, _(" link: %s "), unix_error_string (errno));
    } else {
#ifdef OLD_SYMLINK_VERSION
        symlink_dialog (fname, "", &dest, &src);
#else
	/* suggest the full path for symlink */
        char s[MC_MAXPATHLEN];
        char d[MC_MAXPATHLEN];
	
        strcpy(s, cpanel->cwd);
        if ( ! ((s[0] == '/') && (s[1] == 0)))
            strcat(s, "/");
        strcat(s, fname);
	if (get_other_type () == view_listing)
	    strcpy(d, opanel->cwd);
	else
	    strcpy (d,"");
	
        if ( ! ((d[0] == '/') && (d[1] == 0)))
            strcat(d, "/");
        symlink_dialog (s, d, &dest, &src);
#endif
	if (!dest || !*dest) {
	    if (src)
	        free (src);
	    if (dest)
	        free (dest);
	    return;
	}
	if (src){
	    if (*src) {
	        save_cwds_stat ();
	        if (-1 == mc_symlink (dest, src))
		    message (1, MSG_ERROR, _(" symlink: %s "),
			     unix_error_string (errno));
	    }
	    free (src);
	}
    }
    free (dest);
    update_panels (UP_OPTIMIZE, UP_KEEPSEL);
    repaint_screen ();
}

void link_cmd (void)
{
    do_link (0, selection (cpanel)->fname);
}

void symlink_cmd (void)
{
    do_link (1, selection (cpanel)->fname);
}

void edit_symlink_cmd (void)
{
    if (S_ISLNK (selection (cpanel)->buf.st_mode)) {
	char buffer [MC_MAXPATHLEN], *p = selection (cpanel)->fname;
	int i;
	char *dest, *q = copy_strings (_(" Symlink "), name_trunc (p, 32), _(" points to:"), NULL);
	
	i = readlink (p, buffer, MC_MAXPATHLEN);
	if (i > 0) {
	    buffer [i] = 0;
	    dest = input_expand_dialog (_(" Edit symlink "), q, buffer);
	    if (dest) {
		if (*dest && strcmp (buffer, dest)) {
		    save_cwds_stat ();
		    mc_unlink (p);
		    if (-1 == mc_symlink (dest, p))
		        message (1, MSG_ERROR, _(" edit symlink: %s "),
				 unix_error_string (errno));
		    update_panels (UP_OPTIMIZE, UP_KEEPSEL);
		    repaint_screen ();
		}
		free (dest);
	    }
	}
	free (q);
    }
}

void other_symlink_cmd (void)
{
    char *dest, *q, *p, *r, *s, *t;

    if (get_other_type () != view_listing)
        return;

    if (!strcmp (selection (opanel)->fname, ".."))
        return;
    p = concat_dir_and_file (cpanel->cwd, selection (cpanel)->fname);
    r = concat_dir_and_file (opanel->cwd, selection (cpanel)->fname);
    
    q = copy_strings (_(" Link symbolically "), name_trunc (p, 32), _(" to:"), NULL);
    dest = input_expand_dialog (_(" Relative symlink "), q, r);
    if (dest) {
	if (*dest) {
	    t = strrchr (dest, PATH_SEP);
	    if (t) {
		t[1] = 0;
		s = diff_two_paths (dest, p);
		t[1] = PATH_SEP;
		if (s) {
		    save_cwds_stat ();
		    if (-1 == mc_symlink (dest, s))
		        message (1, MSG_ERROR, _(" relative symlink: %s "),
				 unix_error_string (errno));
		    update_panels (UP_OPTIMIZE, UP_KEEPSEL);
		    repaint_screen ();
		    free (s);
		}
	    }
	}
	free (dest);
    }
    free (q);
    free (p);
    free (r);
}
#endif

void help_cmd (void)
{
   char *hlpfile = concat_dir_and_file (mc_home, "mc.hlp");
   interactive_display (hlpfile, "[main]");
   free (hlpfile);
}

void view_panel_cmd (void)
{
    view_cmd (cpanel);
}

void edit_panel_cmd (void)
{
    edit_cmd (cpanel);
}

void mkdir_panel_cmd (void)
{
    mkdir_cmd (cpanel);
}

/* Returns a random hint */
char *get_random_hint (void)
{
    char *data, *result, *eol;
    char *hintfile;
    int  len;
    int start;
    
    /* Do not change hints more often than one minute */

#ifdef SCO_FLAVOR
    static time_t last;
    time_t now;

    time (&now);
    if ((now - last) < 60)
	return strdup ("");
    last = now;
#else
    static int last_sec;
    static struct timeval tv;
    
    gettimeofday (&tv, NULL);
    if (!(tv.tv_sec> last_sec+60))
	return strdup (""); 
    last_sec = tv.tv_sec;
#endif

    hintfile = concat_dir_and_file (mc_home, MC_HINT);
    data = load_file (hintfile);
    free (hintfile);
    if (!data)
	return 0;

#ifdef SCO_FLAVOR
    srand ((short) now);
#else
    srand (tv.tv_sec);
#endif
    /* get a random entry */
    len = strlen (data);
    start = rand () % len;
    
    for (;start; start--){
	if (data [start] == '\n'){
	    start++;
	    break;
	}
    }
    eol = strchr (&data [start], '\n');
    if (eol)
	*eol = 0;
    result = strdup (&data [start]);
    free (data);
    return result;
}

#ifndef USE_VFS
#ifdef USE_NETCODE
#undef USE_NETCODE
#endif
#endif

#ifdef USE_NETCODE

static char *machine_str = N_(" Enter machine name (F1 for details): ");

static void nice_cd (char *text, char *xtext, char *help, char *prefix, int to_home)
{
    char *machine;
    char *cd_path;

    if (!SELECTED_IS_PANEL)
	return;

    machine = input_dialog_help (text,
				 xtext,
				 help, "");
    if (!machine)
	return;

    if (strncmp (prefix, machine, strlen (prefix)) == 0)
	cd_path = copy_strings (machine, to_home ? "/~/" : NULL, NULL);
    else 
	cd_path = copy_strings (prefix, machine, to_home ? "/~/" : NULL, NULL);
    
    if (do_panel_cd (MENU_PANEL, cd_path, 0))
	directory_history_add (MENU_PANEL, (MENU_PANEL)->cwd);
    else
	message (1, MSG_ERROR, N_(" Could not chdir to %s "), cd_path);
    free (cd_path);
    free (machine);
}

void netlink_cmd (void)
{
    nice_cd (_(" Link to a remote machine "), _(machine_str),
	     "[Network File System]", "mc:", 1);
}

void ftplink_cmd (void)
{
    nice_cd (_(" FTP to machine "), _(machine_str),
	     "[FTP File System]", "ftp://", 1);
}

#ifdef HAVE_SETSOCKOPT
void source_routing (void)
{
    char *source;
    struct hostent *hp;
    
    source = input_dialog (_(" Socket source routing setup "),
			   _(" Enter host name to use as a source routing hop: ")n,
			   "");
    if (!source)
	return;

    hp = gethostbyname (source);
    if (!hp){
	message (1, _(" Host name "), _(" Error while looking up IP address "));
	return;
    }
    source_route = *((int *)hp->h_addr);
}
#endif /* HAVE_SETSOCKOPT */
#endif /* USE_NETCODE */

#ifdef USE_EXT2FSLIB
void undelete_cmd (void)
{
    nice_cd (_(" Undelete files on an ext2 file system "),
	     _(" Enter the file system name where you want to run the\n "
	       " undelete file system on: (F1 for details)"),
	     "[Undelete File System]", "undel:", 0);
}
#endif

void quick_cd_cmd (void)
{
    char *p = cd_dialog ();

    if (p && *p) {
        char *q = copy_strings ("cd ", p, NULL);
        
        do_cd_command (q);
        free (q);
    }
    if (p)
        free (p);
}

#ifdef SCO_FLAVOR
#undef DUSUM_USEB
#undef DUSUM_FACTOR
#endif /* SCO_FLAVOR */

#ifdef HAVE_DUSUM
void dirsizes_cmd (void)
{
    WPanel *panel = cpanel;
    int i, j = 0;
    char *cmd, *p, *q, *r;
    FILE *f;
#ifdef DUSUM_USEB
#   define dirsizes_command "du -s -b "
#else
#   define dirsizes_command "du -s "
#endif
#ifndef DUSUM_FACTOR
#    define DUSUM_FACTOR 512
#endif

    if (!vfs_current_is_local ())
        return;
    for (i = 0; i < panel->count; i++)
        if (S_ISDIR (panel->dir.list [i].buf.st_mode))
            j += strlen (panel->dir.list [i].fname) + 1;
    if (!j)
        return;
    cmd = xmalloc (strlen (dirsizes_command) + j + 1, "dirsizes_cmd");
    strcpy (cmd, dirsizes_command);
    p = strchr (cmd, 0);
    for (i = 0; i < panel->count; i++)
        if (S_ISDIR (panel->dir.list [i].buf.st_mode) && 
            strcmp (panel->dir.list [i].fname, "..")) {
            strcpy (p, panel->dir.list [i].fname);
            p = strchr (p, 0);
            *(p++) = ' ';
        }
    *(--p) = 0;
    open_error_pipe ();
    f = popen (cmd, "r");
    free (cmd);
    if (f != NULL) {
        /* Assume that du will display the directories in the order 
         * I've passed to it :( 
         */
        i = 0;
        p = xmalloc (1024, "dirsizes_cmd");
        while (fgets (p, 1024, f)) {
            j = atoi (p) * DUSUM_FACTOR;
            for (q = p; *q && *q != ' ' && *q != '\t'; q++);
            while (*q == ' ' || *q == '\t')
                q++;
            r = strchr (q, '\n');
            if (r == NULL)
                r = strchr (q, 0);
    	    for (; i < panel->count; i++)
                if (S_ISDIR (panel->dir.list [i].buf.st_mode))
                    if (!strncmp (q, panel->dir.list [i].fname,
                        r - q)) {
                        if (panel->dir.list [i].f.marked)
                            panel->total += j - 
			    ((panel->has_dir_sizes) ? panel->dir.list [i].buf.st_size : 0); 
                        panel->dir.list [i].buf.st_size = j;
                        break;
                    }
            if (i == panel->count)
                break;
        }
	free (p);
        if (pclose (f) < 0)
#ifndef SCO_FLAVOR
	    message (0, _("Show directory sizes"), _("Pipe close failed"));
	else
#else /* SCO_FLAVOR */
 	/* 
 	**	SCO reports about error while all seems to be ok. Just ignore it...
 	**	(alex@bcs.zaporizhzhe.ua)
 	*/
 	;
#endif /* SCO_FLAVOR */
	    panel->has_dir_sizes = 1;
	close_error_pipe (0, 0);
	paint_panel (panel);
    } else
        close_error_pipe (1, _("Cannot invoke du command."));
}
#endif

void
save_setup_cmd (void)
{
    char *str;
    
    save_setup ();
    sync_profiles ();
    str = copy_strings ( _(" Setup saved to ~/"), PROFILE_NAME, NULL);
    
#ifdef HAVE_GNOME
    set_hintbar (str);
#else
    message (0, _(" Setup "), str);
#endif
    free (str);
}

void
configure_panel_listing (WPanel *p, int view_type, int use_msformat, char *user, char *status)
{
    int err;
    
    p->user_mini_status = use_msformat; 
    p->list_type = view_type;
    
    if (view_type == list_user || use_msformat){
	free (p->user_format);
	p->user_format = user;
    
	free (p->user_status_format [view_type]);
	p->user_status_format [view_type] = status;
    
	err = set_panel_formats (p);
	
	if (err){
	    if (err & 0x01){
	    	free (p->user_format);
		p->user_format  = strdup (DEFAULT_USER_FORMAT);
	    }
		
	    if (err & 0x02){
		free (p->user_status_format [view_type]);
		p->user_status_format [view_type]  = strdup (DEFAULT_USER_FORMAT);
	    }
	}
    }
    else {
        free (user);
        free (status);
    }

    set_panel_formats (p);
    paint_panel (p);
    
    do_refresh ();
}

#ifndef HAVE_GNOME
void
info_cmd_no_menu (void)
{
    set_display_type (cpanel == left_panel ? 1 : 0, view_info);
}

void
quick_cmd_no_menu (void)
{
    set_display_type (cpanel == left_panel ? 1 : 0, view_quick);
}

void
switch_to_listing (int panel_index)
{
    if (get_display_type (panel_index) != view_listing)
	set_display_type (panel_index, view_listing);
}

void
listing_cmd (void)
{
    int   view_type, use_msformat;
    char  *user, *status;
    WPanel *p;
    int   display_type;

    display_type = get_display_type (MENU_PANEL_IDX);
    if (display_type == view_listing)
	p = MENU_PANEL_IDX == 0 ? left_panel : right_panel;
    else
	p = 0;

    view_type = display_box (p, &user, &status, &use_msformat, MENU_PANEL_IDX);

    if (view_type == -1)
	return;

    switch_to_listing (MENU_PANEL_IDX);

    p = MENU_PANEL_IDX == 0 ? left_panel : right_panel;

    configure_panel_listing (p, view_type, use_msformat, user, status);
}

void
tree_cmd (void)
{
    set_display_type (MENU_PANEL_IDX, view_tree);
}

void
info_cmd (void)
{
    set_display_type (MENU_PANEL_IDX, view_info);
}

void
quick_view_cmd (void)
{
    set_display_type (MENU_PANEL_IDX, view_quick);
}
#endif

/* Handle the tree internal listing modes switching */
static int
set_basic_panel_listing_to (int panel_index, int listing_mode)
{
    WPanel *p = (WPanel *) get_panel_widget (panel_index);

#ifndef HAVE_GNOME
    switch_to_listing (panel_index);
#endif
    p->list_type = listing_mode;
    if (set_panel_formats (p))
	return 0;
	
    paint_panel (p);
    do_refresh ();
    return 1;
}

void
toggle_listing_cmd (void)
{
    int current = get_current_index ();
    WPanel *p = (WPanel *) get_panel_widget (current);
    int list_mode = p->list_type;
    int m;
    
    switch (list_mode){
    case list_full:
    case list_brief:
	m = list_long;
	break;
    case list_long:
	m = list_user;
	break;
    default:
	m = list_full;
    }
    if (set_basic_panel_listing_to (current, m))
	return;
    set_basic_panel_listing_to (current, list_full);
}


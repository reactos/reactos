/* Main program for the Midnight Commander
   Copyright (C) 1994, 1995, 1996, 1997 The Free Software Foundation
   
   Written by: 1994, 1995, 1996, 1997 Miguel de Icaza
               1994, 1995 Janne Kukonlehto
	       1997 Norbert Warmuth
   
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
#include <locale.h>

#ifdef _OS_NT
#    include <windows.h>
#endif

#ifdef __os2__
#    define INCL_DOS
#    define INCL_DOSFILEMGR
#    define INCL_DOSERRORS
#    include <os2.h>
#endif

#include "tty.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/param.h>

#include <sys/stat.h>

#ifdef HAVE_UNISTD_H
#   include <unistd.h>
#endif

/* unistd.h defines _POSIX_VERSION on POSIX.1 systems. */
#if defined(HAVE_DIRENT_H) || defined(_POSIX_VERSION)
#   include <dirent.h>
#   define NLENGTH(dirent) (strlen ((dirent)->d_name))
#else
#   define dirent direct
#   define NLENGTH(dirent) ((dirent)->d_namlen)

#   ifdef HAVE_SYS_NDIR_H
#       include <sys/ndir.h>
#   endif /* HAVE_SYS_NDIR_H */

#   ifdef HAVE_SYS_DIR_H
#       include <sys/dir.h>
#   endif /* HAVE_SYS_DIR_H */

#   ifdef HAVE_NDIR_H
#       include <ndir.h>
#   endif /* HAVE_NDIR_H */
#endif /* not (HAVE_DIRENT_H or _POSIX_VERSION) */

#if HAVE_SYS_WAIT_H
#   include <sys/wait.h>	/* For waitpid() */
#endif

#include <errno.h>
#ifndef OS2_NT
#    include <pwd.h>
#endif
#include <ctype.h>
#include <fcntl.h>	/* For O_RDWR */
#include <signal.h>

/* Program include files */
#include "x.h"
#include "mad.h"
#include "dir.h"
#include "color.h"
#include "global.h"
#include "util.h"
#include "dialog.h"
#include "menu.h"
#include "file.h"
#include "panel.h"
#include "main.h"
#include "win.h"
#include "user.h"
#include "mem.h"
#include "mouse.h"
#include "option.h"
#include "tree.h"
#include "cons.saver.h"
#include "subshell.h"
#include "key.h"	/* For init_key() and mi_getch() */
#include "setup.h"	/* save_setup() */
#include "profile.h"	/* free_profiles() */
#include "boxes.h"
#include "layout.h"
#include "cmd.h"		/* Normal commands */
#include "hotlist.h"
#include "panelize.h"
#ifndef __os2__
#    include "learn.h"
#endif
#include "listmode.h"
#include "background.h"
#include "ext.h"	/* For flush_extension_file() */

/* Listbox for the command history feature */
#include "widget.h"
#include "command.h"
#include "wtools.h"
#include "complete.h"		/* For the free_completion */

#include "chmod.h"
#include "chown.h"

#ifdef OS2_NT
#    include <io.h>
#    include <drive.h>
#endif

#include "../vfs/vfs.h"
#include "../vfs/extfs.h"


#include "popt.h"

/* "$Id: main.c,v 1.1 2001/12/30 09:55:24 sedwards Exp $" */

/* When the modes are active, left_panel, right_panel and tree_panel */
/* Point to a proper data structure.  You should check with the functions */
/* get_current_type and get_other_type the types of the panels before using */
/* This pointer variables */

/* The structures for the panels */
WPanel *left_panel;
WPanel *right_panel;

/* The pointer to the tree */
WTree *the_tree;

/* The Menubar */
WMenu *the_menubar;

/* Pointers to the selected and unselected panel */
WPanel *current_panel = NULL;

/* Set when we want use advanced chmod command instead of chmod and/or chown */
int advanced_chfns = 0;

/* Set when main loop should be terminated */
volatile int quit = 0;

/* Set if you want the possible completions dialog for the first time */
int show_all_if_ambiguous = 0;

/* Set when cd symlink following is desirable (bash mode) */
int cd_symlinks = 1;

/* If set then dialogs just clean the screen when refreshing, else */
/* they do a complete refresh, refreshing all the parts of the program */
int fast_refresh = 0;

/* If true, marking a files moves the cursor down */
int   mark_moves_down = 1;

/* If true, at startup the user-menu is invoked */
int   auto_menu = 0;

/* If true, use + and \ keys normally and select/unselect do if M-+ / M-\ and M--
   and keypad + / - */
int   alternate_plus_minus = 0;

/* If true, then the +, - and \ keys have their special meaning only if the
 * command line is emtpy, otherwise they behave like regular letters
 */
int   only_leading_plus_minus = 1;

/* If true, after executing a command, wait for a keystroke */
enum { pause_never, pause_on_dumb_terminals, pause_always };

int   pause_after_run = pause_on_dumb_terminals;

/* It true saves the setup when quitting */
int auto_save_setup = 1;

/* If true, be eight bit clean */
int eight_bit_clean = 0;

/* If true, then display chars 0-255, else iso-8859-1,
   requires eight_bit_clean */
int full_eight_bits = 0;

/* If true use the internal viewer */
int use_internal_view = 1;

/* Have we shown the fast-reload warning in the past? */
int fast_reload_w = 0;

/* Move page/item? When clicking on the top or bottom of a panel */
int mouse_move_pages = 1;

/* If true: l&r arrows are used to chdir if the input line is empty */
int navigate_with_arrows = 0;

/* If it is set, the commander will iconify itself when executing a program */
int iconify_on_exec = 1;

/* If true use +, -, | for line drawing */
int force_ugly_line_drawing = 0;

/* If true message "The shell is already running a command" never */
int force_subshell_execution = 0;

/* If true program softkeys (HP terminals only) on startup and after every 
   command ran in the subshell to the description found in the termcap/terminfo 
   database */
int reset_hp_softkeys = 0;

/* The prompt */
char *prompt = 0;

/* The widget where we draw the prompt */
WLabel *the_prompt;

/* The hint bar */
WLabel *the_hint;

/* The button bar */
WButtonBar *the_bar;

#ifdef HAVE_X
WButtonBar *the_bar2;
#endif

/* For slow terminals */
int slow_terminal = 0;

/* use mouse? */
int use_mouse_p = GPM_MOUSE;

/* If true, assume we are running on an xterm terminal */
static int force_xterm = 0;

/* Controls screen clearing before an exec */
int clear_before_exec = 1;

/* Asks for confirmation before deleting a file */
int confirm_delete = 1;

/* Asks for confirmation before overwriting a file */
int confirm_overwrite = 1;

/* Asks for confirmation before executing a program by pressing enter */
int confirm_execute = 0;

/* Asks for confirmation before leaving the program */
int confirm_exit = 1;

/* Asks for confirmation when using F3 to view a directory and there
   are tagged files */
int confirm_view_dir = 0;

/* This flag indicates if the pull down menus by default drop down */
int drop_menus = 0;

/* The dialog handle for the main program */
Dlg_head *midnight_dlg;

/* Subshell: if set, then the prompt was not saved on CONSOLE_SAVE */
/* We need to paint it after CONSOLE_RESTORE, see: load_prompt */
int update_prompt = 0;

/* The name which was used to invoke mc */
char *program_name;

/* The home directory */
char *home_dir;

/* The value of the other directory, only used when loading the setup */
char *other_dir = 0;
char *this_dir = 0;

/* If true, then print on stdout the last directory we were at */
static int print_last_wd = 0;
static char *last_wd_string;
static int print_last_revert = 0;

/* On OS/2 and on Windows NT, we need a batch file to do the -P magic */
#ifdef OS2_NT
static char *batch_file_name = 0;
#endif

/* widget colors for the midnight commander */
int midnight_colors [4];

/* Force colors, only used by Slang */
int force_colors = 0;

/* colors specified on the command line: they override any other setting */
char *command_line_colors;

/* File name to view if argument was supplied */
char *view_one_file = 0;

/* File name to view if argument was supplied */
char *edit_one_file = 0;

/* Used so that widgets know if they are being destroyed or
   shut down */
int midnight_shutdown = 0;

/* to show nice prompts */
static int last_paused = 0;

/* Only used at program boot */
int boot_current_is_left = 1;

/* Used for keeping track of the original stdout */
int stdout_fd = 0;

/* The user's shell */
char *shell;

/* mc_home: The home of MC */
char *mc_home;

/* if on, it displays the information that files have been moved to ~/.mc */
int show_change_notice = 0;

char cmd_buf [512];

/* Used during argument processing */
int finish_program = 0;

/* Forward declarations */
char *get_mc_lib_dir ();
int panel_event    (Gpm_Event *event, WPanel *panel);
int menu_bar_event (Gpm_Event *event, void *);
static void menu_cmd (void);

#ifndef HAVE_GNOME
WPanel *
get_current_panel ()
{
	return current_panel;
}

WPanel *
get_other_panel ()
{
	return (WPanel *) get_panel_widget (get_other_index ());
}
#endif

void
try_to_select (WPanel *panel, char *name)
{
    Xtry_to_select (panel, name);
    select_item (panel);
    display_mini_info (panel);
}

/*
 * cd_try_to_select:
 *
 *  If we moved to the parent directory move the selection pointer to
 *  the old directory name
 */
void
cd_try_to_select (WPanel *panel)
{
    char *p, *q;
    int i, j = 4;

    if (strlen (panel->lwd) > strlen (panel->cwd)
	&& strncmp (panel->cwd, panel->lwd, strlen (panel->cwd)) == 0
	&& strchr (panel->lwd + strlen (panel->cwd) + 1, PATH_SEP) == 0)
	try_to_select (panel, panel->lwd);
    else
#ifdef USE_VFS
	if ((!strncmp (panel->lwd, "tar:", 4) && 
             !strncmp (panel->lwd + 4, panel->cwd, strlen (panel->cwd))) ||
             ((i = extfs_prefix_to_type (panel->lwd)) != -1 && 
             !strncmp (panel->lwd + (j = strlen (extfs_get_prefix (i)) + 1), 
             panel->cwd, strlen (panel->cwd)))) {
        p = strdup (panel->lwd + j + strlen (panel->cwd));
        q = strchr (p, PATH_SEP);
        if (q != NULL && (q != p || (q = strchr (q + 1, PATH_SEP)) != NULL))
            *q = 0;
        try_to_select (panel, p);
        free (p);
    } else
#endif
	try_to_select (panel, NULL);
}

void
reload_panelized (WPanel *panel)
{
    int i, j;
    dir_list *list = &panel->dir;
    
    if (panel != cpanel)
	mc_chdir (panel->cwd);

    for (i = 0, j = 0; i < panel->count; i++){
    	if (list->list [i].f.marked) { 
	    /* Unmark the file in advance. In case the following mc_lstat
	     * fails we are done, else we have to mark the file again
	     * (Note: do_file_mark depends on a valid "list->list [i].buf").
	     * IMO that's the best way to update the panel's summary status
	     * -- Norbert
	     */
	    do_file_mark (panel, i, 0);
	}
	if (mc_lstat (list->list [i].fname, &list->list [i].buf)){
	    free (list->list [i].fname);
	    continue;
	}
    	if (list->list [i].f.marked)
	    do_file_mark (panel, i, 1);
	if (j != i)
	    list->list [j] = list->list [i];
	j++;
    }
    if (j == 0)
	panel->count = set_zero_dir (list);
    else
	panel->count = j;

    if (panel != cpanel)
	mc_chdir (cpanel->cwd);
}

void
update_one_panel_widget (WPanel *panel, int force_update, char *current_file)
{
    int free_pointer;

    if (force_update & UP_RELOAD){
	panel->is_panelized = 0;

	ftpfs_flushdir ();
	bzero (&(panel->dir_stat), sizeof (panel->dir_stat));
    }
    
    /* If current_file == -1 (an invalid pointer) then preserve selection */
    if (current_file == UP_KEEPSEL){
	free_pointer = 1;
	current_file = strdup (panel->dir.list [panel->selected].fname);
    } else
	free_pointer = 0;
    
    if (panel->is_panelized)
	reload_panelized (panel);
    else
	panel_reload (panel);

    try_to_select (panel, current_file);
    panel->dirty = 1;

    if (free_pointer)
	free (current_file);
}

#ifndef PORT_HAS_UPDATE_PANELS
void
update_one_panel (int which, int force_update, char *current_file)
{
    WPanel *panel;

    if (get_display_type (which) != view_listing)
	return;

    panel = (WPanel *) get_panel_widget (which);
    update_one_panel_widget (panel, force_update, current_file);
}

/* This routine reloads the directory in both panels. It tries to
 * select current_file in current_panel and other_file in other_panel.
 * If current_file == -1 then it automatically sets current_file and
 * other_file to the currently selected files in the panels.
 *
 * if force_update has the UP_ONLY_CURRENT bit toggled on, then it
 * will not reload the other panel.
*/
void
update_panels (int force_update, char *current_file)
{
    int reload_other = !(force_update & UP_ONLY_CURRENT);
    WPanel *panel;

    update_one_panel (get_current_index (), force_update, current_file);
    if (reload_other)
	update_one_panel (get_other_index (), force_update, UP_KEEPSEL);

    if (get_current_type () == view_listing)
	panel = (WPanel *) get_panel_widget (get_current_index ());
    else
	panel = (WPanel *) get_panel_widget (get_other_index ());

    mc_chdir (panel->cwd);
}
#endif

#ifdef WANT_PARSE
static void select_by_index (WPanel *panel, int i);

/* Called by parse_control_file */
static int index_by_name (file_entry *list, int count)
{
    char *name;
    int i;

    name = strtok (NULL, " \t\n");
    if (!name || !*name)
	return -1;
    for (i = 0; i < count; i++){
	if (strcmp (name, list[i].fname) == 0)
	    return i;
    }
    return -1;
}

/* Called by parse_control_file */
static void select_by_index (WPanel *panel, int i)
{
    if (i >= panel->count)
	return;
    
    unselect_item (panel);
    panel->selected = i;

#ifndef HAVE_X    
    while (panel->selected - panel->top_file >= ITEMS (panel)){
	/* Scroll window half screen */
	panel->top_file += ITEMS (panel)/2;
	paint_dir (panel);
	select_item (panel);
    } 
    while (panel->selected < panel->top_file){
	/* Scroll window half screen */
	panel->top_file -= ITEMS (panel)/2;
	if (panel->top_file < 0) panel->top_file = 0;
	paint_dir (panel);
    } 
#endif
    select_item (panel);
}

/* Called by my_system
   No error reporting, just exits on the first sign of trouble */
static void parse_control_file (void)
{
    char *data, *current;
    WPanel *panel;
    file_entry *list;
    int i;
    FILE *file;
    struct stat s;
    
    if ((file = fopen (control_file, "r")) == NULL){
	return;
    }
    /* Use of fstat prevents race conditions */
    if (fstat (fileno (file), &s) != 0){
	fclose (file);
	return;
    }
#ifndef OS2_NT
    /* Security: Check that the user owns the control file to prevent
       other users from playing tricks on him/her. */
    if (s.st_uid != getuid ()){
	fclose (file);
	return;
    }
#endif
    data = (char *) xmalloc (s.st_size+1, "main, parse_control_file");
    if (!data){
	fclose (file);
	return;
    }
    if (s.st_size != fread (data, 1, s.st_size, file)){
	free (data);
	fclose (file);
	return;
    }
    data [s.st_size] = 0;
    fclose (file);
    
    /* The Control file has now been loaded to memory -> start parsing. */
    current = strtok (data, " \t\n");
    while (current && *current){
	if (isupper (*current)){
	    if (get_other_type () != view_listing)
		break;
	    else
		panel = other_panel;
	} else
	    panel = cpanel;

	list = panel->dir.list;
	*current = tolower (*current);

	if (strcmp (current, "clear_tags") == 0){
	    unmark_files (panel);
	} else if (strcmp (current, "tag") == 0){
	    i = index_by_name (list, panel->count);
	    if (i >= 0) {
		do_file_mark (panel, i, 1);
	    }
	} else if (strcmp (current, "untag") == 0){
	    i = index_by_name (list, panel->count);
	    if (i >= 0){
		do_file_mark (panel, i, 0);
	    }
	} else if (strcmp (current, "select") == 0){
	    i = index_by_name (list, panel->count);
	    if (i >= 0){
		select_by_index (panel, i);
	    }
	} else if (strcmp (current, "change_panel") == 0){
	    change_panel ();
	} else if (strcmp (current, "cd") == 0){
	    int change = 0;
	    current = strtok (NULL, " \t\n");
	    if (!current) break;
	    if (cpanel != panel){
		change_panel ();
		change = 1;
	    }
	    do_cd (current, cd_parse_command);
	    if (change)
		change_panel ();
	} else {
	    /* Unknown command -> let's give up */
	    break;
	}
	current = strtok (NULL, " \t\n");
    }

    free (data);
    paint_panel (cpanel);
    paint_panel (opanel);
}
#else
#define parse_control_file()
#endif /* WANT_PARSE */

/* Sets up the terminal before executing a program */
static void
pre_exec (void)
{
    use_dash (0);
    edition_pre_exec ();
}

/* Save current stat of directories to avoid reloading the panels */
/* when no modifications have taken place */
void
save_cwds_stat (void)
{
    if (fast_reload){
	mc_stat (cpanel->cwd, &(cpanel->dir_stat));
	if (get_other_type () == view_listing)
	    mc_stat (opanel->cwd, &(opanel->dir_stat));
    }
}

#ifdef HAVE_SUBSHELL_SUPPORT
void
do_possible_cd (char *new_dir)
{
    if (!do_cd (new_dir, cd_exact))
	message (1, _(" Warning "),
		 _(" The Commander can't change to the directory that \n"
		   " the subshell claims you are in.  Perhaps you have \n"
		   " deleted your working directory, or given yourself \n"
		   " extra access permissions with the \"su\" command? "));
}

void
do_update_prompt ()
{
    if (update_prompt){
	printf ("%s", subshell_prompt);
	fflush (stdout);
	update_prompt = 0;
    }
}
#endif

void
restore_console (void)
{
    handle_console (CONSOLE_RESTORE);
}

void
exec_shell ()
{
    do_execute (shell, 0, 0);
}

void
do_execute (const char *shell, const char *command, int flags)
{
#ifdef HAVE_SUBSHELL_SUPPORT
    char *new_dir = NULL;
#endif

#ifdef USE_VFS
    char *old_vfs_dir = 0;

    if (!vfs_current_is_local ())
	old_vfs_dir = strdup (vfs_get_current_dir ());
#endif

    save_cwds_stat ();
    pre_exec ();
    if (console_flag)
	restore_console ();

#ifndef __os2__
    unlink (control_file);
#endif
    if (!use_subshell && !(flags & EXECUTE_INTERNAL)){
	printf ("%s%s%s\n", last_paused ? "\r\n":"", prompt, command);
	last_paused = 0;
    }

#ifdef HAVE_SUBSHELL_SUPPORT
    if (use_subshell && !(flags & EXECUTE_INTERNAL)){
	do_update_prompt ();

	/* We don't care if it died, higher level takes care of this */
#ifdef USE_VFS
	invoke_subshell (command, VISIBLY, old_vfs_dir ? 0 : &new_dir);
#else
	invoke_subshell (command, VISIBLY, &new_dir);
#endif
    } else
#endif
	my_system (flags, shell, command);

#ifndef HAVE_GNOME
    if (!(flags & EXECUTE_INTERNAL)){
	if ((pause_after_run == pause_always ||
	    (pause_after_run == pause_on_dumb_terminals &&
	     !xterm_flag && !console_flag)) && !quit){
	    printf (_("Press any key to continue..."));
	    last_paused = 1;
	    fflush (stdout);
	    mc_raw_mode ();
	    xgetch ();
	}
	if (console_flag){
	    if (output_lines && keybar_visible) {
		putchar('\n');
		fflush(stdout);
	    }
	}
    }
#endif

    if (console_flag)
	handle_console (CONSOLE_SAVE);
    edition_post_exec ();

#ifdef HAVE_SUBSHELL_SUPPORT
	if (new_dir)
	    do_possible_cd (new_dir);

#endif
#ifdef USE_VFS
	if (old_vfs_dir){
	    mc_chdir (old_vfs_dir);
	    free (old_vfs_dir);
	}
#endif

    update_panels (UP_OPTIMIZE, UP_KEEPSEL);
    
    parse_control_file ();
#ifndef __os2__
    unlink (control_file);
#endif
    do_refresh ();
    use_dash (TRUE);
}

/* Executes a command */
void
shell_execute (char *command, int flags)
{
#ifdef HAVE_SUBSHELL_SUPPORT
    if (use_subshell)
        if (subshell_state == INACTIVE || force_subshell_execution)
	    do_execute (shell, command, flags | EXECUTE_AS_SHELL);
	else
	    message (1, MSG_ERROR, _(" The shell is already running a command "));
    else
#endif
	do_execute (shell, command, flags | EXECUTE_AS_SHELL);
}

void
execute (char *command)
{
    shell_execute (command, 0);
}

void
change_panel (void)
{
    free_completions (input_w (cmdline));
    dlg_one_down (midnight_dlg);
}

static int
quit_cmd_internal (int quiet)
{
    int q = quit;

    if (quiet || !confirm_exit){
	q = 1;
    } else  {
	if (query_dialog (_(" The Midnight Commander "),
		     _(" Do you really want to quit the Midnight Commander? "),
			  0, 2, _("&Yes"), _("&No")) == 0)
	    q = 1;
    }
    if (q){
#ifdef HAVE_SUBSHELL_SUPPORT
	if (!use_subshell)
	    midnight_dlg->running = 0;
	else
	    if ((q = exit_subshell ()))
#endif
		midnight_dlg->running = 0;
    }
    if (q)
        quit |= 1;
    return quit;
}

int quit_cmd (void)
{
    quit_cmd_internal (0);
    return quit;
}

int quiet_quit_cmd (void)
{
    print_last_revert = 1;
    quit_cmd_internal (1);
    return quit;
}

/*
 * Touch window and refresh window functions
 */

/* This routine untouches the first line on both panels in order */
/* to avoid the refreshing the menu bar */

#ifdef HAVE_X
void
untouch_bar (void)
{
}

void
repaint_screen (void)
{
    do_refresh ();
}

#else /* HAVE_X */
void
untouch_bar (void)
{
    do_refresh ();
}

void
repaint_screen (void)
{
    do_refresh ();
    mc_refresh ();
}
#endif /* HAVE_X */

/* Wrapper for do_subshell_chdir, check for availability of subshell */
void
subshell_chdir (char *directory)
{
#ifdef HAVE_SUBSHELL_SUPPORT
    if (use_subshell){
	if (vfs_current_is_local ())
	    do_subshell_chdir (directory, 0, 1);
    }
#endif
}

void
directory_history_add (WPanel * panel, char *s)
{
    if (!panel->dir_history) {
	panel->dir_history = malloc (sizeof (Hist));
	memset (panel->dir_history, 0, sizeof (Hist));
	panel->dir_history->text = strdup (s);
	return;
    }
    if (!strcmp (panel->dir_history->text, s))
	return;
    if (panel->dir_history->next) {
	if (panel->dir_history->next->text) {
	    free (panel->dir_history->next->text);
	    panel->dir_history->next->text = 0;
	}
    } else {
	panel->dir_history->next = malloc (sizeof (Hist));
	memset (panel->dir_history->next, 0, sizeof (Hist));
	panel->dir_history->next->prev = panel->dir_history;
    }
    panel->dir_history = panel->dir_history->next;
    panel->dir_history->text = strdup (s);
    
    panel_update_marks (panel);
}

/* Changes the current panel directory */
int
_do_panel_cd (WPanel *panel, char *new_dir, enum cd_enum cd_type)
{
    char *directory, *olddir;
    char temp [MC_MAXPATHLEN];
#ifdef USE_VFS    
    vfs *oldvfs;
    vfsid oldvfsid;
    struct vfs_stamping *parent;
#endif    
    olddir = strdup (panel->cwd);

    /* Convert *new_path to a suitable pathname, handle ~user */
    
    if (cd_type == cd_parse_command){
	while (*new_dir == ' ')
	    new_dir++;
    
	if (!strcmp (new_dir, "-")){
	    strcpy (temp, panel->lwd);
	    new_dir = temp;
	}
    }
    directory = *new_dir ? new_dir : home_dir;

    if (mc_chdir (directory) == -1){
	strcpy (panel->cwd, olddir);
	free (olddir);
	return 0;
    }

    /* Success: save previous directory, shutdown status of previous dir */
    strcpy (panel->lwd, olddir);
    free_completions (input_w (cmdline));
    
    mc_get_current_wd (panel->cwd, sizeof (panel->cwd) - 2);

#ifdef USE_VFS
    oldvfs = vfs_type (olddir);
    oldvfsid = vfs_ncs_getid (oldvfs, olddir, &parent);
    vfs_add_noncurrent_stamps (oldvfs, oldvfsid, parent);
    vfs_rm_parents (parent);
#endif
    free (olddir);

    subshell_chdir (panel->cwd);

    /* Reload current panel */
    clean_dir (&panel->dir, panel->count);
    panel->count = do_load_dir (&panel->dir, panel->sort_type,
				 panel->reverse, panel->case_sensitive, panel->filter);
    panel->top_file = 0;
    panel->selected = 0;
    panel->marked = 0;
    panel->dirs_marked = 0;
    panel->total = 0;
    panel->searching = 0;
    cd_try_to_select (panel);
    load_hint ();
    panel_update_contents (panel);
    return 1;
}

int
do_panel_cd (WPanel *panel, char *new_dir, enum cd_enum cd_type)
{
    int r;
    
    r = _do_panel_cd (panel, new_dir, cd_type);
    if (r)
	directory_history_add (cpanel, cpanel->cwd);
    return r;
}

int
do_cd (char *new_dir, enum cd_enum exact)
{
    return (do_panel_cd (cpanel, new_dir, exact));
}

void
directory_history_next (WPanel * panel)
{
    if (!panel->dir_history->next)
	return;
    if (_do_panel_cd (panel, panel->dir_history->next->text, cd_exact))
	panel->dir_history = panel->dir_history->next;
    panel_update_marks (panel);
}

void
directory_history_prev (WPanel * panel)
{
    if (!panel->dir_history->prev)
	return;
    if (_do_panel_cd (panel, panel->dir_history->prev->text, cd_exact))
	panel->dir_history = panel->dir_history->prev;
    panel_update_marks (panel);
}

void
directory_history_list (WPanel * panel)
{
    char *s;
/* must be at least two to show a history */
    if (panel->dir_history) {
	if (panel->dir_history->prev || panel->dir_history->next) {
	    s = show_hist (panel->dir_history, panel->widget.x, panel->widget.y);
	    if (s) {
		int r;
		r = _do_panel_cd (panel, s, cd_exact);
		if (r)
		    directory_history_add (panel, panel->cwd);
		free (s);
	    }
	}
    }
}

#ifdef HAVE_SUBSHELL_SUPPORT
int
load_prompt (int fd, void *unused)
{
    if (!read_subshell_prompt (QUIETLY))
	return 0;
    
    if (command_prompt){
	int  prompt_len;

	prompt = strip_ctrl_codes (subshell_prompt);
	prompt_len = strlen (prompt);

	/* Check for prompts too big */
	if (prompt_len > COLS - 8) {
	    prompt [COLS - 8 ] = 0;
	    prompt_len = COLS - 8;
	}
	label_set_text (the_prompt, prompt);
	winput_set_origin ((WInput *)cmdline, prompt_len, COLS-prompt_len);

	/* since the prompt has changed, and we are called from one of the 
	 * get_event channels, the prompt updating does not take place
	 * automatically: force a cursor update and a screen refresh
	 */
	if (current_dlg == midnight_dlg){
	    update_cursor (midnight_dlg);
	    mc_refresh ();
	}
    }
    update_prompt = 1;
    return 0;
}
#endif

/* The user pressed the enter key */
int
menu_bar_event (Gpm_Event *event, void *x)
{
    if (event->type != GPM_DOWN)
	return MOU_NORMAL;

    return MOU_ENDLOOP;
}

/* Used to emulate Lynx's entering leaving a directory with the arrow keys */
int
maybe_cd (int char_code, int move_up_dir)
{
    if (navigate_with_arrows){
	if (!input_w (cmdline)->buffer [0]){
	    if (!move_up_dir){
		do_cd ("..", cd_exact);
		return 1;
	    }
	    if (S_ISDIR (selection (cpanel)->buf.st_mode)
		|| link_isdir (selection (cpanel))){
		do_cd (selection (cpanel)->fname, cd_exact);
		return 1;
	    }
	}
    }
    return 0;
}

void
set_sort_to (WPanel *p, sortfn *sort_order)
{
    p->sort_type = sort_order;
    
    /* The directory is already sorted, we have to load the unsorted stuff */
    if (sort_order == (sortfn *) unsorted){
	char *current_file;
	
	current_file = strdup (cpanel->dir.list [cpanel->selected].fname);
	panel_reload (cpanel);
	try_to_select (cpanel, current_file);
	free (current_file);
    }
    do_re_sort (p);
}

void
sort_cmd (void)
{
    WPanel  *p;
    sortfn *sort_order;

    if (!SELECTED_IS_PANEL)
	return;

    p = MENU_PANEL;
    sort_order = sort_box (p->sort_type, &p->reverse, &p->case_sensitive);

    if (sort_order == 0)
	return;

    p->sort_type = sort_order;

    /* The directory is already sorted, we have to load the unsorted stuff */
    if (sort_order == (sortfn *) unsorted){
	char *current_file;
	
	current_file = strdup (cpanel->dir.list [cpanel->selected].fname);
	panel_reload (cpanel);
	try_to_select (cpanel, current_file);
	free (current_file);
    }
    do_re_sort (p);
}

static void
tree_box (void)
{
    char *sel_dir;

    sel_dir = tree (selection (cpanel)->fname);
    if (sel_dir){
	do_cd(sel_dir, cd_exact);
	free (sel_dir);
    }
}

#if SOMEDAY_WE_WILL_FINISH_THIS_CODE
static void
	listmode_cmd (void)
{
    char *newmode;
    newmode = listmode_edit ("half <type,>name,|,size:8,|,perm:4+");
    message (0, " Listing format edit ", " New mode is \"%s\" ", newmode);
    free (newmode);
}
#endif

#ifdef HAVE_GNOME
void init_menu () {};
void done_menu () {};
#else
/* NOTICE: hotkeys specified here are overriden in menubar_paint_idx (alex) */
static menu_entry PanelMenu [] = {
    { ' ', N_("&Listing mode..."),          'L', listing_cmd },
    { ' ', N_("&Quick view     C-x q"),     'Q', quick_view_cmd }, 
    { ' ', N_("&Info           C-x i"),     'I', info_cmd },
    { ' ', N_("&Tree"),                     'T', tree_cmd },
    { ' ', "", ' ', 0 },		    
    { ' ', N_("&Sort order..."),            'S', sort_cmd },
    { ' ', "", ' ', 0 },		    
    { ' ', N_("&Filter..."),                'F', filter_cmd },
#ifdef USE_NETCODE			    
    { ' ', "", ' ', 0 },		    
    { ' ', N_("&Network link..."),          'N', netlink_cmd },
    { ' ', N_("FT&P link..."),              'P', ftplink_cmd },
#endif
    { ' ', "", ' ', 0 },
#ifdef OS2_NT
    { ' ', N_("&Drive...       M-d"),       'D', drive_cmd_a },
#endif
    { ' ', N_("&Rescan         C-r"),       'R', reread_cmd }
};

static menu_entry RightMenu [] = {
    { ' ', N_("&Listing mode..."),       'L', listing_cmd },
    { ' ', N_("&Quick view     C-x q"),  'Q', quick_view_cmd }, 
    { ' ', N_("&Info           C-x i"),  'I', info_cmd },
    { ' ', N_("&Tree"),                  'T', tree_cmd },
    { ' ', "", ' ', 0 },		    
    { ' ', N_("&Sort order..."),         'S', sort_cmd },
    { ' ', "", ' ', 0 },		    
    { ' ', N_("&Filter..."),             'F', filter_cmd },
#ifdef USE_NETCODE			    
    { ' ', "", ' ', 0 },		    
    { ' ', N_("&Network link..."),       'N', netlink_cmd },
    { ' ', N_("FT&P link..."),           'P', ftplink_cmd },
#endif
    { ' ', "", ' ', 0 },
#ifdef OS2_NT
    { ' ', N_("&Drive...       M-d"),    'D', drive_cmd_b },
#endif
    { ' ', N_("&Rescan         C-r"),    'R', reread_cmd }
};

static menu_entry FileMenu [] = {
    { ' ', N_("&User menu          F2"), 'U', user_menu_cmd },
    { ' ', N_("&View               F3"), 'V', view_cmd },
    { ' ', N_("&Filtered view     M-!"), 'F', filtered_view_cmd },
    { ' ', N_("&Edit               F4"), 'E', edit_cmd },
    { ' ', N_("&Copy               F5"), 'C', copy_cmd },
    { ' ', N_("c&Hmod           C-x c"), 'H', chmod_cmd },
#ifndef OS2_NT				       
    { ' ', N_("&Link            C-x l"), 'L', link_cmd },
    { ' ', N_("&SymLink         C-x s"), 'S', symlink_cmd },
    { ' ', N_("edit s&Ymlink  C-x C-s"), 'Y', edit_symlink_cmd },
    { ' ', N_("ch&Own           C-x o"), 'O', chown_cmd },
    { ' ', N_("&Advanced chown       "), 'A', chown_advanced_cmd },
#endif					       
    { ' ', N_("&Rename/Move        F6"), 'R', ren_cmd },
    { ' ', N_("&Mkdir              F7"), 'M', mkdir_cmd },
    { ' ', N_("&Delete             F8"), 'D', delete_cmd },
    { ' ', N_("&Quick cd          M-c"), 'Q', quick_cd_cmd },
    { ' ', "", ' ', 0 },
    { ' ', N_("select &Group      M-+"), 'G', select_cmd },
    { ' ', N_("u&Nselect group    M-\\"),'N', unselect_cmd },
    { ' ', N_("reverse selec&Tion M-*"), 'T', reverse_selection_cmd },
    { ' ', "", ' ', 0 },
    { ' ', N_("e&Xit              F10"), 'X', (callfn) quit_cmd }
};

void external_panelize (void);
static menu_entry CmdMenu [] = {
    /* I know, I'm lazy, but the tree widget when it's not running
     * as a panel still has some problems, I have not yet finished
     * the WTree widget port, sorry.
     */
    { ' ', N_("&Directory tree"),               'D', tree_box },
    { ' ', N_("&Find file            M-?"),     'F', find_cmd },
#ifndef HAVE_XVIEW    
    { ' ', N_("s&Wap panels          C-u"),     'W', swap_cmd },
    { ' ', N_("switch &Panels on/off C-o"),     'P', view_other_cmd },
#endif    
    { ' ', N_("&Compare directories  C-x d"),   'C', compare_dirs_cmd },
    { ' ', N_("e&Xternal panelize    C-x !"),   'X', external_panelize },
#ifdef HAVE_DUSUM
    { ' ', N_("show directory s&Izes"),         'I', dirsizes_cmd },
#endif
    { ' ', "", ' ', 0 },
    { ' ', N_("command &History"),              'H', history_cmd },
    { ' ', N_("di&Rectory hotlist    C-\\"),    'R', quick_chdir_cmd },
#ifdef USE_VFS
    { ' ', N_("&Active VFS list      C-x a"),   'A', reselect_vfs },
#endif
#ifdef WITH_BACKGROUND
    { ' ', N_("&Background jobs      C-x j"),   'B', jobs_cmd },
#endif
    { ' ', "", ' ', 0 },
#ifdef USE_EXT2FSLIB
    { ' ', N_("&Undelete files (ext2fs only)"), 'U', undelete_cmd },
#endif
#ifdef VERSION_4
    { ' ', N_("&Listing format edit"),          'L', listmode_cmd},
#endif
    { ' ', N_("&Extension file edit"),          'E', ext_cmd },
    { ' ', N_("&Menu file edit"),               'M', menu_edit_cmd }
};

/* Must keep in sync with the constants in menu_cmd */
static menu_entry OptMenu [] = {
    { ' ', N_("&Configuration..."),    'C', configure_box },
    { ' ', N_("&Layout..."),           'L', layout_cmd },
    { ' ', N_("c&Onfirmation..."),     'O', confirm_box },
    { ' ', N_("&Display bits..."),     'D', display_bits_box },
#if !defined(HAVE_X) && !defined(OS2_NT)
    { ' ', N_("learn &Keys..."),       'K', learn_keys },
#endif
#ifdef USE_VFS    
    { ' ', N_("&Virtual FS..."),       'V', configure_vfs },
#endif
    { ' ', "", ' ', 0 }, 
    { ' ', N_("&Save setup"),          'S', save_setup_cmd }
};

#define menu_entries(x) sizeof(x)/sizeof(menu_entry)

Menu MenuBar [5];
#ifndef HAVE_X
static Menu MenuBarEmpty [5];
#endif

void
init_menu (void)
{
    int i;

#ifdef HAVE_X    
    MenuBar [0] = create_menu (_(" Left "), PanelMenu, menu_entries (PanelMenu));
#else
    MenuBar [0] = create_menu ( horizontal_split ? _(" Above ") : _(" Left "), 
                                PanelMenu, menu_entries (PanelMenu));
#endif
    MenuBar [1] = create_menu (_(" File "), FileMenu, menu_entries (FileMenu));
    MenuBar [2] = create_menu (_(" Command "), CmdMenu, menu_entries (CmdMenu));
    MenuBar [3] = create_menu (_(" Options "), OptMenu, menu_entries (OptMenu));
#ifndef HAVE_XVIEW
#ifdef HAVE_X
    MenuBar [4] = create_menu (_(" Right "), RightMenu, menu_entries (PanelMenu));
#else
    MenuBar [4] = create_menu (horizontal_split ? _(" Below ") : _(" Right "), 
			       RightMenu, menu_entries (PanelMenu));
    for (i = 0; i < 5; i++)
	MenuBarEmpty [i] = create_menu (MenuBar [i]->name, 0, 0);
#endif /* HAVE_X */
#endif /* ! HAVE_XVIEW */
}

void
done_menu (void)
{
    int i;

#ifndef HAVE_XVIEW
    for (i = 0; i < 5; i++){
	destroy_menu (MenuBar [i]);
#ifndef HAVE_X
	destroy_menu (MenuBarEmpty [i]);
#endif
#else
    for (i = 0; i < 4; i++){
	destroy_menu (MenuBar [i]);
#endif
    }
}
#endif
    
static void
menu_last_selected_cmd (void)
{
    the_menubar->active = 1;
    the_menubar->dropped = drop_menus;
    the_menubar->previous_selection = dlg_item_number (midnight_dlg);
    dlg_select_widget (midnight_dlg, the_menubar);
}

static void
menu_cmd (void)
{
    if (the_menubar->active)
	return;
    
    if (get_current_index () == 0)
	the_menubar->selected = 0;
    else
	the_menubar->selected = 4;
    menu_last_selected_cmd ();
}

/* Flag toggling functions */
void
toggle_confirm_delete (void)
{
    confirm_delete = !confirm_delete;
}

void
toggle_fast_reload (void)
{
    fast_reload = !fast_reload;
    if (fast_reload_w == 0 && fast_reload){
	message (0, _(" Information "),
		 _(" Using the fast reload option may not reflect the exact \n"
		   " directory contents. In this cases you'll need to do a  \n"
		   " manual reload of the directory. See the man page for   \n"
		   " the details.                                           "));
	fast_reload_w = 1;
    }
}

void
toggle_mix_all_files (void)
{
    mix_all_files = !mix_all_files;
    update_panels (UP_RELOAD, UP_KEEPSEL);
}

void
toggle_show_backup (void)
{
    show_backups = !show_backups;
    update_panels (UP_RELOAD, UP_KEEPSEL);
}

void
toggle_show_hidden (void)
{
    show_dot_files = !show_dot_files;
    update_panels (UP_RELOAD, UP_KEEPSEL);
}

void
toggle_show_mini_status (void)
{
    show_mini_info = !show_mini_info;
    paint_panel (cpanel);
    if (get_other_type () == view_listing)
	paint_panel (opanel);
}

void
toggle_align_extensions (void)
{
    align_extensions = !align_extensions;
}

#ifndef PORT_HAS_CREATE_PANELS
void
create_panels (void)
{
    int current_index;
    int other_index;
    int current_mode;
    int other_mode;
    char original_dir [1024];

    original_dir [0] = 0;
    
    if (boot_current_is_left){
	current_index = 0;
	other_index = 1;
	current_mode = startup_left_mode;
	other_mode = startup_right_mode;
    } else {
	current_index = 1;
	other_index = 0;
	current_mode = startup_right_mode;
	other_mode = startup_left_mode;
    }
    /* Creates the left panel */
    if (this_dir){
	if (other_dir){
	    /* Ok, user has specified two dirs, save the original one,
	     * since we may not be able to chdir to the proper
	     * second directory later
	     */
	    mc_get_current_wd (original_dir, sizeof (original_dir)-2);
	}
	mc_chdir (this_dir);
    }
    set_display_type (current_index, current_mode);

    /* The other panel */
    if (other_dir){
	if (original_dir [0])
	    mc_chdir (original_dir);
	mc_chdir (other_dir);
    }
    set_display_type (other_index, other_mode);

    if (startup_left_mode == view_listing){
	current_panel = left_panel;
    } else {
	if (right_panel)
	    current_panel = right_panel;
	else
	    current_panel = left_panel;
    }

    /* Create the nice widgets */
    cmdline     = command_new (0, 0, 0);
    the_prompt  = label_new (0, 0, prompt, NULL);
    the_prompt->transparent = 1;
    the_bar     = buttonbar_new (keybar_visible);

#ifndef HAVE_GNOME
    the_hint    = label_new (0, 0, 0, NULL);
    the_hint->transparent = 1;
    the_hint->auto_adjust_cols = 0;
    the_hint->widget.cols = COLS;
#endif
    
#ifndef HAVE_XVIEW
    the_menubar = menubar_new (0, 0, COLS, MenuBar, 5);
#else
    the_menubar = menubar_new (0, 0, COLS, MenuBar + 1, 3);
    the_bar2    = buttonbar_new (keybar_visible);
#endif
}
#endif

static void copy_current_pathname (void)
{
    if (!command_prompt)
	return;

    stuff (input_w (cmdline), cpanel->cwd, 0);
    if (cpanel->cwd [strlen (cpanel->cwd) - 1] != PATH_SEP)
        stuff (input_w (cmdline), PATH_SEP_STR, 0);
}

static void copy_other_pathname (void)
{
    if (get_other_type () != view_listing)
	return;

    if (!command_prompt)
	return;

    stuff (input_w (cmdline), opanel->cwd, 0);
    if (cpanel->cwd [strlen (opanel->cwd) - 1] != PATH_SEP)
        stuff (input_w (cmdline), PATH_SEP_STR, 0);
}

static void copy_readlink (WPanel *panel)
{
    if (!command_prompt)
	return;
    if (S_ISLNK (selection (panel)->buf.st_mode)) {
	char buffer [MC_MAXPATHLEN];
	char *p = concat_dir_and_file (panel->cwd, selection (panel)->fname);
	int i;
	
	i = mc_readlink (p, buffer, MC_MAXPATHLEN);
	free (p);
	if (i > 0) {
	    buffer [i] = 0;
	    stuff (input_w (cmdline), buffer, 0);
	}
    }
}

static void copy_current_readlink (void)
{
    copy_readlink (cpanel);
}

static void copy_other_readlink (void)
{
    if (get_other_type () != view_listing)
	return;
    copy_readlink (opanel);
}

/* Inserts the selected file name into the input line */
/* Exported so that the command modules uses it */
void copy_prog_name (void)
{
    char *tmp;
    if (!command_prompt)
	return;

    if (get_current_type () == view_tree){
	WTree *tree = (WTree *) get_panel_widget (get_current_index ());
	tmp = name_quote (tree->selected_ptr->name, 1);
    } else
	tmp = name_quote (selection (cpanel)->fname, 1);
    stuff (input_w (cmdline), tmp, 1);
    free (tmp);
}   

static void copy_tagged (WPanel *panel)
{
    int i;

    if (!command_prompt)
	return;
    input_disable_update (input_w (cmdline));
    if (panel->marked){
	for (i = 0; i < panel->count; i++)
	    if (panel->dir.list [i].f.marked) {
	    	char *tmp = name_quote (panel->dir.list [i].fname, 1);
		stuff (input_w (cmdline), tmp, 1);
		free (tmp);
	    }
    } else {
    	char *tmp = name_quote (panel->dir.list [panel->selected].fname, 1);
	stuff (input_w (cmdline), tmp, 1);
	free (tmp);
    }
    input_enable_update (input_w (cmdline));
}
    
static void copy_current_tagged (void)
{
    copy_tagged (cpanel);
}

static void copy_other_tagged (void)
{
    if (get_other_type () != view_listing)
	return;
    copy_tagged (opanel);
}

static void do_suspend_cmd (void)
{
    pre_exec ();

    if (console_flag && !use_subshell)
	restore_console ();

#ifndef OS2_NT
    {
	struct sigaction sigtstp_action;
	
	/* Make sure that the SIGTSTP below will suspend us directly,
	   without calling ncurses' SIGTSTP handler; we *don't* want
	   ncurses to redraw the screen immediately after the SIGCONT */
	sigaction (SIGTSTP, &startup_handler, &sigtstp_action);
    
	kill (getpid (), SIGTSTP);

	/* Restore previous SIGTSTP action */
	sigaction (SIGTSTP, &sigtstp_action, NULL);
    }
#endif
    
    if (console_flag && !use_subshell)
	handle_console (CONSOLE_SAVE);
    
    edition_post_exec ();
}

void suspend_cmd (void)
{
    save_cwds_stat ();
    do_suspend_cmd ();
    update_panels (UP_OPTIMIZE, UP_KEEPSEL);
    do_refresh ();
}

void init_labels (Widget *paneletc)
{
    define_label (midnight_dlg, paneletc, 1, _("Help"), help_cmd);
    define_label (midnight_dlg, paneletc, 2, _("Menu"), user_menu_cmd);
    define_label (midnight_dlg, paneletc, 9, _("PullDn"), menu_cmd);
    define_label (midnight_dlg, paneletc, 10, _("Quit"), (voidfn) quit_cmd);
}

#ifndef HAVE_XVIEW
static key_map ctl_x_map [] = {
    { XCTRL('c'),   (callfn) quit_cmd },
#ifdef USE_VFS
    { 'a',          reselect_vfs },
#endif
    { 'd',          compare_dirs_cmd },
#ifndef HAVE_GNOME
    { 'p',          copy_current_pathname },
    { XCTRL('p'),   copy_other_pathname },
    { 't',          copy_current_tagged },
    { XCTRL('t'),   copy_other_tagged },
#endif
    { 'c',          chmod_cmd },
#ifndef OS2_NT
    { 'o',          chown_cmd },
    { 'l',          link_cmd },
    { XCTRL('l'),   other_symlink_cmd },
    { 's',          symlink_cmd },
    { XCTRL('s'),   edit_symlink_cmd },
    { 'r',          copy_current_readlink },
    { XCTRL('r'),   copy_other_readlink },
#endif
#ifndef HAVE_GNOME
    { 'i',          info_cmd_no_menu },
    { 'q',          quick_cmd_no_menu },
#endif
    { 'h',          add2hotlist_cmd },
    { '!',          external_panelize },
#ifdef WITH_BACKGROUND
    { 'j',          jobs_cmd },
#endif
#ifdef HAVE_SETSOCKOPT
    { '%',          source_routing },
#endif
    { 0,  0 }
};

static int ctl_x_map_enabled = 0;

static void ctl_x_cmd (int ignore)
{
	ctl_x_map_enabled = 1;
}

static void nothing ()
{
}

static key_map default_map [] = {
#ifndef HAVE_GNOME
    { KEY_F(19),  menu_last_selected_cmd },
    { KEY_F(20),  (key_callback) quiet_quit_cmd },

    /* Copy useful information to the command line */
    { ALT('\n'),  copy_prog_name },
    { ALT('\r'),  copy_prog_name },
    { ALT('a'),   copy_current_pathname },
    { ALT('A'),   copy_other_pathname },
    
    { ALT('c'),	  quick_cd_cmd },

    /* To access the directory hotlist */
    { XCTRL('\\'), quick_chdir_cmd },

    /* Suspend */
    { XCTRL('z'), suspend_cmd },
#endif
    /* The filtered view command */
    { ALT('!'),   filtered_view_cmd_cpanel },
    
    /* Find file */
    { ALT('?'),	  find_cmd },
	
    /* Panel refresh */
    { XCTRL('r'), reread_cmd },

    { ALT('t'),   toggle_listing_cmd },
    
#ifndef HAVE_X
    /* Swap panels */
    { XCTRL('u'), swap_cmd },

    /* View output */
    { XCTRL('o'), view_other_cmd },
#endif
    
    /* Control-X keybindings */
    { XCTRL('x'), ctl_x_cmd },

    /* Trap dlg's exit commands */
    { ESC_CHAR,   nothing },
    { XCTRL('c'), nothing },
    { XCTRL('g'), nothing },
    { 0, 0 },
};
#endif

#ifndef HAVE_X
static void setup_sigwinch ()
{
#ifndef OS2_NT
    struct sigaction act, oact;
    
#   if defined(HAVE_SLANG) || NCURSES_VERSION_MAJOR >= 4
#       ifdef SIGWINCH
            act.sa_handler = flag_winch;
            sigemptyset (&act.sa_mask);
	    act.sa_flags = 0;
#           ifdef SA_RESTART
                act.sa_flags |= SA_RESTART;
#           endif
            sigaction (SIGWINCH, &act, &oact);
#       endif
#   endif
#endif
}

static void
setup_pre ()
{
    /* Call all the inits */
#ifndef HAVE_SLANG
    meta (stdscr, eight_bit_clean);
#else
    SLsmg_Display_Eight_Bit = full_eight_bits ? 128 : 160;
#endif
}
#else
#define setup_pre()
#define setup_sigwinch()
#endif

static void
setup_post ()
{
    setup_sigwinch ();
    
    init_uid_gid_cache ();

#ifndef HAVE_X
    if (baudrate () < 9600 || slow_terminal){
	verbose = 0;
    }
    if (use_mouse_p)
	init_mouse ();
#endif

    midnight_colors [0] = 0;
    midnight_colors [1] = REVERSE_COLOR;     /* FOCUSC */
    midnight_colors [2] = INPUT_COLOR;       /* HOT_NORMALC */
    midnight_colors [3] = NORMAL_COLOR;	     /* HOT_FOCUSC */
}

static void setup_mc (void)
{
    setup_pre ();
    init_menu ();
    create_panels ();

#ifdef HAVE_GNOME
    return;
#endif
    setup_panels ();
    
#ifdef HAVE_SUBSHELL_SUPPORT
    if (use_subshell)
	add_select_channel (subshell_pty, load_prompt, 0);
#endif
    
    setup_post ();
}

static void setup_dummy_mc (const char *file)
{
    char d[MC_MAXPATHLEN];

    mc_get_current_wd (d, MC_MAXPATHLEN);
    setup_mc ();
    mc_chdir (d);

    /* Create a fake current_panel, this is needed because the
     * expand_format routine will use current panel.
     */
    strcpy (cpanel->cwd, d);
    cpanel->selected = 0;
    cpanel->count = 1;
    cpanel->dir.list[0].fname = (char *) file;
}

static void done_mc ()
{
    done_menu ();
    
    /* Setup shutdown
     *
     * We sync the profiles since the hotlist may have changed, while
     * we only change the setup data if we have the auto save feature set
     */
    if (auto_save_setup)
	save_setup ();   /* does also call save_hotlist */
    else
	save_hotlist();
    done_screen ();
    vfs_add_current_stamps ();
    if (xterm_flag && xterm_hintbar)
        set_hintbar(_("Thank you for using GNU Midnight Commander"));
}

/* This should be called after destroy_dlg since panel widgets
 *  save their state on the profiles
 */
static void done_mc_profile ()
{
    if (!auto_save_setup)
	profile_forget_profile (profile_name);
    sync_profiles ();
    done_setup ();
    free_profiles ();
}

/* This routine only handles cpanel, and opanel, it is easy to
 * change to use npanels, just loop over the number of panels
 * and use get_panel_widget (i) and make the test done below
 */
void make_panels_dirty ()
{
    if (cpanel->dirty)
	panel_update_contents (cpanel);
    
    if ((get_other_type () == view_listing) && opanel->dirty)
	panel_update_contents (opanel);
}

/* In OS/2 and Windows NT people want to actually type the '\' key frequently */
#ifdef OS2_NT
#   define check_key_backslash(x) 0
#else
#   define check_key_backslash(x) ((x) == '\\')
#endif

int midnight_callback (struct Dlg_head *h, int id, int msg)
{
    int i;
    
    switch (msg){
#ifndef HAVE_XVIEW

	/* Speed up routine: now, we just set the  */
    case DLG_PRE_EVENT:
	make_panels_dirty ();
	return MSG_HANDLED;
	
    case DLG_KEY:
	if (ctl_x_map_enabled){
		ctl_x_map_enabled = 0;
		for (i = 0; ctl_x_map [i].key_code; i++)
			if (id == ctl_x_map [i].key_code){
				(*ctl_x_map [i].fn)(id);
				return MSG_HANDLED;
			}
	}
	    
	if (id == KEY_F(10) && !the_menubar->active){
	    quit_cmd ();
	    return MSG_HANDLED;
	}

	if (id == '\t')
	    free_completions (input_w (cmdline));

	/* On Linux, we can tell the difference */
	if (id == '\n' && ctrl_pressed ()){
	    copy_prog_name ();
	    return MSG_HANDLED;
	}

	if (id == '\n' && input_w (cmdline)->buffer [0]){
	    send_message_to (h, (Widget *) cmdline, WIDGET_KEY, id);
	    return MSG_HANDLED;
	}

	if ((!alternate_plus_minus || !(console_flag || xterm_flag)) &&
             !quote && !cpanel->searching) {
	    if(!only_leading_plus_minus) {
		/* Special treatement, since the input line will eat them */
		if (id == '+' ) {
		    select_cmd ();
		    return MSG_HANDLED;
		}

		if (check_key_backslash (id) || id == '-'){
		    unselect_cmd ();
		    return MSG_HANDLED;
		}

		if (id == '*') {
		    reverse_selection_cmd ();
		    return MSG_HANDLED;
		}
	    } else if (command_prompt && !strlen (input_w (cmdline)->buffer)) {
		/* Special treatement '+', '-', '\', '*' only when this is 
		 * first char on input line
		 */
		
		if (id == '+') {
		    select_cmd ();
		    return MSG_HANDLED;
		}
		
		if (check_key_backslash (id) || id == '-') {
		    unselect_cmd ();
		    return MSG_HANDLED;
		}
		
		if (id == '*') {
		    reverse_selection_cmd ();
		    return MSG_HANDLED;
		}
	    }   
	} 
	break;

    case DLG_HOTKEY_HANDLED:
	if (get_current_type () == view_listing)
	    cpanel->searching = 0;
	break;
	
    case DLG_UNHANDLED_KEY:
	if (command_prompt){
	    int v;
	    
	    v = send_message_to (h, (Widget *) cmdline, WIDGET_KEY, id);
	    if (v)
		return v;
	}
	if (ctl_x_map_enabled){
		ctl_x_map_enabled = 0;
		for (i = 0; ctl_x_map [i].key_code; i++)
			if (id == ctl_x_map [i].key_code){
				(*ctl_x_map [i].fn)(id);
				return MSG_HANDLED;
			}
	} else {
		for (i = 0; default_map [i].key_code; i++){
			if (id == default_map [i].key_code){
				(*default_map [i].fn)(id);
				return MSG_HANDLED;
			}
		}
	}
	return MSG_NOT_HANDLED;
	
#endif
#ifndef HAVE_X
	/* We handle the special case of the output lines */
    case DLG_DRAW:
	attrset (SELECTED_COLOR);
	if (console_flag && output_lines)
	    show_console_contents (output_start_y,
				   LINES-output_lines-keybar_visible-1,
				   LINES-keybar_visible-1);
	break;
#endif
	
    }
    return default_dlg_callback (h, id, msg);
}

#ifdef HAVE_X
/* This should be rewritten in order to support as many panel containers as
   the user wants */

#ifndef HAVE_GNOME
widget_data containers [2];
int containers_no = 2;

void
xtoolkit_panel_setup ()
{
    containers [0] = x_create_panel_container (0);
    containers [1] = x_create_panel_container (1);
    input_w (cmdline)->widget.wcontainer = containers [0];
    input_w (cmdline)->widget.area = AREA_BOTTOM;
    the_prompt->widget.wcontainer = containers [0];
    the_prompt->widget.area = AREA_BOTTOM;
    the_bar->widget.wcontainer = containers [0];
    the_bar->widget.area = AREA_TOP;
#ifdef HAVE_XVIEW
    the_bar2->widget.wcontainer = containers [1];
    the_bar2->widget.area = AREA_TOP;
#endif
    get_panel_widget (0)->wcontainer = containers [0];
    get_panel_widget (0)->area = AREA_RIGHT;
    get_panel_widget (1)->wcontainer = containers [1];
    get_panel_widget (1)->area = AREA_RIGHT;
    the_menubar->widget.wcontainer = (widget_data) NULL;
}
#else
#    define xtoolkit_panel_setup()
#endif

#else
#    define xtoolkit_panel_setup()
#endif

#ifndef PORT_HAS_LOAD_HINT
void load_hint ()
{
    char *hint;

    if (!the_hint->widget.parent)
	return;
	
    if (!message_visible && (!xterm_flag || !xterm_hintbar)){
        label_set_text (the_hint, 0);
	return;
    }

    if ((hint = get_random_hint ())){
	if (*hint)
	    set_hintbar (hint);
	free (hint);
    } else {
	set_hintbar ("The Midnight Commander " VERSION
			" (C) 1995-1997 the Free Software Foundation");
    }
}
#endif

static void
setup_panels_and_run_mc ()
{
    int first, second;

    xtoolkit_panel_setup ();
    tk_new_frame (midnight_dlg, "p.");
#ifndef HAVE_X
    add_widget (midnight_dlg, the_hint);
#endif /* HAVE_X */
    load_hint ();
    add_widgetl (midnight_dlg, cmdline, XV_WLAY_RIGHTOF);
    add_widgetl (midnight_dlg, the_prompt, XV_WLAY_DONTCARE);
    tk_end_frame ();
    add_widget (midnight_dlg, the_bar);
#ifdef HAVE_XVIEW
    add_widget (midnight_dlg, the_bar2);
#endif
    if (boot_current_is_left){
	first = 1;
	second = 0;
    } else {
	first = 0;
	second = 1;
    }
    add_widget (midnight_dlg, get_panel_widget (first));
    add_widget (midnight_dlg, get_panel_widget (second));
    add_widget (midnight_dlg, the_menubar);
    
    init_labels (get_panel_widget (0));
    init_labels (get_panel_widget (1));

    /* Run the Midnight Commander if no file was specified in the command line */
    run_dlg (midnight_dlg);
}

/* result must be free'd (I think this should go in util.c) */
char *
prepend_cwd_on_local (char *filename)
{
    char *d;
    int l;

    if (vfs_file_is_local (filename)){
	if (*filename == PATH_SEP)	/* an absolute pathname */
	    return strdup (filename);
	d = malloc (MC_MAXPATHLEN + strlen (filename) + 2);
	mc_get_current_wd (d, MC_MAXPATHLEN);
	l = strlen(d);
	d[l++] = PATH_SEP;
	strcpy (d + l, filename);
	return canonicalize_pathname (d);
    } else
	return strdup (filename);
}

#ifdef USE_INTERNAL_EDIT
void edit (const char *file_name, int startline);

int
mc_maybe_editor_or_viewer (void)
{
    char *path;

    if (!(view_one_file || edit_one_file))
	    return 0;
    
    /* Invoke the internal view/edit routine with:
     * the default processing and forcing the internal viewer/editor
     */
    if (view_one_file) {
	    path = prepend_cwd_on_local (view_one_file);
	    setup_dummy_mc (path);
	    view_file (path, 0, 1);
    }
#ifdef USE_INTERNAL_EDIT
    else {
	    path = prepend_cwd_on_local ("");
	    setup_dummy_mc (path);
	    edit (edit_one_file, 1);
    }
#endif
    free (path);
    midnight_shutdown = 1;
    done_mc ();
    return 1;
}

#endif

static void
do_nc (void)
{
    midnight_dlg = create_dlg (0, 0, LINES, COLS, midnight_colors, midnight_callback, "[main]", "midnight", 0);
    midnight_dlg->has_menubar = 1;

#ifdef USE_INTERNAL_EDIT
    /* Check if we were invoked as an editor or file viewer */
    if (mc_maybe_editor_or_viewer ())
	    return;
#endif
    
    setup_mc ();

#ifndef HAVE_GNOME
    setup_panels_and_run_mc ();
#endif
    
    /* Program end */
    midnight_shutdown = 1;

    /* destroy_dlg destroys even cpanel->cwd, so we have to save a copy :) */
    if (print_last_wd) {
	if (!vfs_current_is_local ())
	    last_wd_string = strdup (".");
	else
	    last_wd_string = strdup (cpanel->cwd);
    }
    done_mc ();
    
#ifndef HAVE_GNOME
    destroy_dlg (midnight_dlg);
    current_panel = 0;
#endif
    done_mc_profile ();
}

#include "features.inc"

static void
version (int verbose)
{
    fprintf (stderr, "The Midnight Commander %s\n", VERSION);
    if (!verbose)
	return;
    
#ifndef HAVE_X
    fprintf (stderr,
	    _("with mouse support on xterm%s.\n"),
	     status_mouse_support ? _(" and the Linux console") : "");
#endif /* HAVE_X */

    fprintf (stderr, features);
    if (print_last_wd)
	write (stdout_fd, ".", 1);
}

#if defined (_OS_NT)
/* Windows NT code */
#define CONTROL_FILE "\\mc.%d.control"
char control_file [sizeof (CONTROL_FILE) + 8];

void
OS_Setup ()
{
    SetConsoleTitle ("GNU Midnight Commander");
    
    shell = getenv ("COMSPEC");
    if (!shell || !*shell)
	shell = get_default_shell ();
    
    /* Default opening mode for files is binary, not text (CR/LF translation) */
#ifndef __EMX__
    _fmode = O_BINARY;
#endif

    mc_home = get_mc_lib_dir ();
}

static void
sigchld_handler_no_subshell (int sig)
{
}

void
init_sigchld (void)
{
}

void
init_sigfatals (void)
{
	/* Nothing to be done on the OS/2, Windows/NT */
}


#elif defined (__os2__)
/* OS/2 Version */
#   define CONTROL_FILE "\\mc.%d.control"
char control_file [sizeof (CONTROL_FILE) + 8];

void
OS_Setup (void)
{
    /* .ado: This does not work:  */
    /* DosSMSetTitle ((PSZ) "This is my app");  */
    /* In DEF: IMPORTS
                 DosSMSetTitle = SESMGR.DOSSMSETTITLE */
    shell = getenv ("COMSPEC");
    if (!shell || !*shell)
	shell = get_default_shell ();

    mc_home = get_mc_lib_dir ();
}

static void
sigchld_handler_no_subshell (int sig)
{
}

void
init_sigchld (void)
{
}

void
init_sigfatals (void)
{
	/* Nothing to be done on the OS/2, Windows/NT */
}
#else

/* Unix version */
#define CONTROL_FILE "/tmp/mc.%d.control"
char control_file [sizeof (CONTROL_FILE) + 8];

void
OS_Setup ()
{
    char   *termvalue;
    char   *mc_libdir;
	
    termvalue = getenv ("TERM");
    if (!termvalue){
	fprintf (stderr, _("The TERM environment variable is unset!\n"));
	termvalue = "";
    }
#ifndef HAVE_X
    if (force_xterm || (strncmp (termvalue, "xterm", 5) == 0 || strcmp (termvalue, "dtterm") == 0)){
	use_mouse_p = XTERM_MOUSE;
	xterm_flag = 1;
#    ifdef SET_TITLE
	printf ("\33]0;GNU Midnight Commander\7");
#    endif
    }
#endif /* ! HAVE_X */
    shell = getenv ("SHELL");
    if (!shell || !*shell)
	shell = strdup (getpwuid (geteuid ())->pw_shell);
    if (!shell || !*shell)
	shell = "/bin/sh";

    sprintf (control_file, CONTROL_FILE, getpid ());
    my_putenv ("MC_CONTROL_FILE", control_file);
    
    /* This is the directory, where MC was installed, on Unix this is LIBDIR */
    /* and can be overriden by the MCHOME environment variable */
    if ((mc_libdir = getenv ("MCHOME")) != NULL) {
	mc_home = strdup (mc_libdir);
    } else {
	mc_home = strdup (LIBDIR);
    }
}

static void
sigchld_handler_no_subshell (int sig)
{
#ifndef HAVE_X
    int pid, status;

    if (!console_flag)
	return;
    
    /* COMMENT: if it were true that after the call to handle_console(..INIT)
       the value of console_flag never changed, we could simply not install
       this handler at all if (!console_flag && !use_subshell). */

    /* That comment is no longer true.  We need to wait() on a sigchld
       handler (that's at least what the tarfs code expects currently). */

#ifndef SCO_FLAVOR
    pid = waitpid (cons_saver_pid, &status, WUNTRACED | WNOHANG);
    
    if (pid == cons_saver_pid){
	/* {{{ Someone has stopped or killed cons.saver; restart it */

	if (WIFSTOPPED (status))
	    kill (pid, SIGCONT);
	else
	{
	    handle_console (CONSOLE_DONE);
	    handle_console (CONSOLE_INIT);
	}
	/* }}} */
    }
#endif /* ! SCO_FLAVOR */

    /* If we get here, some other child exited; ignore it */
#endif /* ! HAVE_X  */
}

#if 0
void
mc_fatal_signal (int signum)
{
    volatile int x;
    char     cmd;
    pid_t    pid;
    char     *args [4] = { "gdb", NULL, NULL, NULL };
    char     pid [20];

    sprintf (buf, "%d", getpid ());
    fprintf (stderr,
	     "Midnight Commander fatal error, PID=%d\n"
	     "What to do: [e]xit, [s]tack trace, [a]ttach to process\n", getpid ());

    read (1, &cmd, 1);
    if (cmd == 'a'){
	for (x = 1; x;)
	    ;
    }
    if (cmd == 's'){
	args [1] = program_name;
	args [2] = buf;
	pid = fork ();
	if (pid == -1){
	    fprintf (stderr, "Could not fork, exiting\n");
	    exit (0);
	}
	if (pid == 0){
	    stack_trace (args);
	} else {
	    while (1)
		;
	}
    }
    exit (0);
}

void
init_sigfatals (void)
{
    struct sigaction sa;

    sa.sa_hanlder = mc_fatal_signal;
    sa.sa_mask    = 0;
    sa.sa_flags   = 0;
    
    sigaction (SIGSEGV, &sa, NULL);
    sigaction (SIGBUS,  &sa, NULL);
    sigaction (SIGFPE,  &sa, NULL);
}
#else
#define init_sigfatals() 
#endif

void
init_sigchld (void)
{
    struct sigaction sigchld_action;

    sigchld_action.sa_handler =
#ifdef HAVE_SUBSHELL_SUPPORT
	use_subshell ? sigchld_handler :
#endif
	sigchld_handler_no_subshell;

    sigemptyset (&sigchld_action.sa_mask);

#ifdef SA_RESTART
        sigchld_action.sa_flags = SA_RESTART;
#else
        sigchld_action.sa_flags = 0;
#endif

    sigaction (SIGCHLD, &sigchld_action, NULL);
}	

#endif /* _OS_NT, __os2__, UNIX */

#if defined(HAVE_SLANG) && !defined(OS2_NT)
    extern int SLtt_Try_Termcap;
#endif

#ifndef PORT_WANTS_ARGP
static void
print_mc_usage (void)
{
    version (0);
    fprintf (stderr,
	"Usage is:\n\n"
        "mc [flags] [this_dir] [other_panel_dir]\n\n"	
#if defined(HAVE_SLANG) && !defined(OS2_NT)
    "-a, --stickchars   Force use of +, -, | for line drawing.\n"
#endif
    "-b, --nocolor      Force black and white display.\n"
#ifdef WITH_BACKGROUND
    "-B, --background   [DEVEL-ONLY: Debug the background code]\n"
#endif
    "-c, --color        Force color mode.\n"
    "-C, --colors       Specify colors (use --help-colors to get a list).\n"
#ifdef USE_INTERNAL_EDIT
    "-e, --edit         Startup the internal editor.\n"
#endif
    "-d, --nomouse      Disable mouse support.\n"
    "-f, --libdir       Print configured paths.\n"
    "-h, --help         Shows this help message.\n"
    "-k, --resetsoft    Reset softkeys (HP terminals only) to their terminfo/termcap\n"
    "                   default.\n"
    "-P, --printwd      At exit, print the last working directory.\n"
    "-s, --slow         Disables verbose operation (for slow terminals).\n"
#if defined(HAVE_SLANG) && !defined(OS2_NT)
    "-t, --termcap      Activate support for the TERMCAP variable.\n"
#endif
#ifdef USE_NETCODE
    "-l, --ftplog file  Log ftpfs commands to the file.\n"
#endif
#if defined(HAVE_SLANG) && defined(OS2_NT)
    "-S, --createcmdile Create command file to set default directory upon exit.\n"
#endif
		 
#ifdef HAVE_SUBSHELL_SUPPORT
    "-u, --nosubshell   Disable the concurrent subshell mode.\n"
    "-U, --subshell     Force the concurrent subshell mode.\n"
    "-r, --forceexec    Force subshell execution.\n"
#endif
    "-v, --view fname   Start up into the viewer mode.\n"
    "-V, --version      Report version and configuration options.\n"
    "-x, --xterm        Force xterm mouse support and screen save/restore.\n"
#ifdef HAVE_SUBSHELL_SUPPORT
    "-X, --dbgsubshell  [DEVEL-ONLY: Debug the subshell].\n"
#endif
    );
}
#endif /* PORT_WANTS_ARGP */

static void
print_color_usage (void)
{
    fprintf (stderr,
	     "--colors KEYWORD={FORE},{BACK}\n\n"
	     "{FORE} and {BACK} can be ommited, and the default will be used\n"
	     "\n"
	     "Keywords:\n"
	     "   Global:       errors, reverse, gauge, input\n"
	     "   File display: normal, selected, marked, markselect\n"
	     "   Dialog boxes: dnormal, dfocus, dhotnormal, dhotfocus\n"
	     "   Menus:        menu, menuhot, menusel, menuhotsel\n"
	     "   Help:         helpnormal, helpitalic, helplink, helpslink\n"
	     "   File types:   directory, execute, link, device, special, core\n"
	     "\n"
	     "Colors:\n"
	     "   black, gray, red, brightred, green, brightgreen, brown,\n"
	     "   yellow, blue, brightblue, magenta, brightmagenta, cyan,\n"
	     "   brightcyan, lightgray and white\n\n");

}


static void
probably_finish_program (void)
{
    if (finish_program){
	if (print_last_wd)
	    printf (".");
	exit (1);
    }
}

enum {
	GEOMETRY_KEY = -1,
	NOWIN_KEY    = -2
};

static void
process_args (int c, char *option_arg)
{
    switch (c) {
    case 'V':
	version (1);
	finish_program = 1;
	break;
		
    case 'c':
	disable_colors = 0;
#ifdef HAVE_SLANG
	force_colors = 1;
#endif
	break;
		
    case 'f':
	fprintf (stderr, _("Library directory for the Midnight Commander: %s\n"), mc_home);
	finish_program = 1;
	break;
		
    case 'm':
	fprintf (stderr, _("Option -m is obsolete. Please look at Display Bits... in the Option's menu\n"));
	finish_program = 1;
	break;
		
#ifdef USE_NETCODE
    case 'l':
	ftpfs_set_debug (option_arg);
	break;
#endif
		
#ifdef OS2_NT
    case 'S':
	print_last_wd = 2;
	batch_file_name = option_arg;
	break;
#endif
		
    case 'd':
	use_mouse_p = NO_MOUSE;
	break;
		
    case 'X':
#ifdef HAVE_SUBSHELL_SUPPORT
	debug_subshell = 1;
#endif
	break;
		
    case 'U':
#ifdef HAVE_SUBSHELL_SUPPORT
	use_subshell = 1;
#endif
	break;
		
    case 'u':
#ifdef HAVE_SUBSHELL_SUPPORT
	use_subshell = 0;
#endif
	break;

    case 'r':
#ifdef HAVE_SUBSHELL_SUPPORT
	force_subshell_execution = 1;
#endif
	break;
	    
    case 'H':
	print_color_usage ();
	finish_program = 1;
	break;
	    
    case 'h':
#ifndef PORT_WANTS_ARGP
	print_mc_usage ();
	finish_program = 1;
#endif
    }
}

#ifdef PORT_WANTS_ARGP
static struct argp_option argp_options [] = {
#ifdef WITH_BACKGROUND
    { "background",	'B', NULL, 0, N_("[DEVEL-ONLY: Debug the background code]"), 0 },
#endif
#if defined(HAVE_SLANG) && defined(OS2_NT)
    { "createcmdfile",	'S', "CMDFILE", , 0, N_("Create command file to set default directory upon exit."), 1 },
#endif			     
    { "color",          'c', NULL, 0, N_("Force color mode."), 0 },
    { "colors", 	'C', "COLORS", 0, N_("Specify colors (use --help-colors to get a list)."), 1 },
#ifdef HAVE_SUBSHELL_SUPPORT
    { "dbgsubshell", 	'X', NULL, 0, N_("[DEVEL-ONLY: Debug the subshell."), 0 },
#endif
    { "edit", 		'e', "EDIT", 0, N_("Startup the internal editor."), 1 },
    { "help", 		'h', NULL, 0, N_("Shows this help message."), 0 },
    { "help-colors",	'H', NULL, 0, N_("Help on how to specify colors."), 0 },
#ifdef USE_NETCODE
    { "ftplog", 	'l', "FTPLOG", 0, N_("Log ftpfs commands to the file."), 1 },
#endif
    { "libdir", 	'f', NULL, 0, N_("Prints out the configured paths."), 0 },
    { NULL, 		'm', NULL, OPTION_HIDDEN, NULL, 0 },
    { "nocolor", 	'b', NULL, 0, N_("Force black and white display."), 0 },
    { "nomouse", 	'd', NULL, 0, N_("Disable mouse support."), 0 },
#ifdef HAVE_SUBSHELL_SUPPORT
    { "subshell", 	'U', NULL, 0, N_("Force the concurrent subshell mode"), 0 },
    { "nosubshell", 	'u', NULL, 0, N_("Disable the concurrent subshell mode."), 0 },
    { "forceexec", 	'r', NULL, 0, N_("Force subshell execution."), 0 },
#endif
    { "printwd", 	'P', NULL, 0, N_("At exit, print the last working directory."), 0 },
    { "resetsoft", 	'k', NULL, 0, N_("Reset softkeys (HP terminals only) to their terminfo/termcap default."), 0},
    { "slow",           's', NULL, 0, N_("Disables verbose operation (for slow terminals)."), 0 },
#if defined(HAVE_SLANG) && !defined(OS2_NT)
    { "stickchars",	'a', NULL, 0, N_("Use simple symbols for line drawing."), 0 },
#endif
#ifdef HAVE_SUBSHELL_SUPPORT
#endif
#if defined(HAVE_SLANG) && !defined(OS2_NT)
    { "termcap", 	't', NULL, 0, N_("Activate support for the TERMCAP variable."), 0 },
#endif
    { "version", 	'V', NULL, 0, N_("Report versionand configuration options."), 0 },
    { "view", 		'v', NULL, 0, N_("Start up into the viewer mode."), 0 },
    { "xterm", 		'x', NULL, 0, N_("Force xterm mouse support and screen save/restore"), 0 },
    { "geometry",       GEOMETRY_KEY,  "GEOMETRY", 0, N_("Geometry for the window"), 0 },
    { "nowindows",      NOWIN_KEY, NULL, 0, N_("No windows opened at startup"), 0 },
    { NULL, 		0,   NULL, 0, NULL },
};

GList *directory_list = 0;
GList *geometry_list  = 0;
int   nowindows;

static error_t
parse_an_arg (int key, char *arg, struct argp_state *state)
{
	switch (key){
#ifdef WITH_BACKGROUND
	case 'B':
	    background_wait = 1;
	    return 0;
#endif
	case 'b':
	    disable_colors = 1;
	    return 0;
	    
	case 'P':
	    print_last_wd = 1;
	    return 0;
	    
	case 'k':
	    reset_hp_softkeys = 1;
	    return 0;

	case 's':
	    slow_terminal = 1;
	    return 0;

	case 'a':
	    force_ugly_line_drawing = 1;
	    return 0;

	case 'x':
	    force_xterm = 1;
	    return 0;
	    
#if defined(HAVE_SLANG) && !defined(OS2_NT)
	case 't':
	    SLtt_Try_Termcap = 1;
	    return 0;
#endif

	case GEOMETRY_KEY:
	    geometry_list = g_list_append (geometry_list, arg);
	    return 0;

	case NOWIN_KEY:
	    nowindows = 1;
	    
	case ARGP_KEY_ARG:
	    break;

	case ARGP_KEY_INIT:
	case ARGP_KEY_FINI:
	    return 0;
	    
	default:
	    process_args (key, arg);
	}

	if (arg){
	    if (edit_one_file)
		edit_one_file = strdup (arg);
	    else if (view_one_file)
		view_one_file = strdup (arg);
	    else 
	        directory_list = g_list_append (directory_list, arg);
	}
	return 0;
}

static struct argp mc_argp_parser = {
	argp_options, parse_an_arg, N_("[this dir] [other dir]"), NULL, NULL, NULL, NULL
};

#else 

static struct poptOption argumentTable[] = {
#ifdef WITH_BACKGROUND
    { "background",	'B', POPT_ARG_NONE, 	&background_wait, 	 0 },
#endif
#if defined(HAVE_SLANG) && defined(OS2_NT)
    { "createcmdfile",	'S', POPT_ARG_STRING, 	NULL, 			 'S' },
#endif
    { "color",          'c', POPT_ARG_NONE, 	NULL, 			 'c' },
    { "colors", 	'C', POPT_ARG_STRING, 	&command_line_colors, 	 0 },
#ifdef HAVE_SUBSHELL_SUPPORT
    { "dbgsubshell", 	'X', POPT_ARG_NONE, 	&debug_subshell, 	 0 },
#endif
    { "edit", 		'e', POPT_ARG_STRING, 	&edit_one_file, 	 0 },

    { "help", 		'h', POPT_ARG_NONE, 	NULL, 			 'h' },
    { "help-colors",	'H', POPT_ARG_NONE, 	NULL, 			 'H' },
#ifdef USE_NETCODE
    { "ftplog", 	'l', POPT_ARG_STRING, 	NULL, 			 'l' },
#endif
    { "libdir", 	'f', POPT_ARG_NONE, 	NULL, 			 'f' },
    { NULL, 		'm', POPT_ARG_NONE, 	NULL, 			 'm' },
    { "nocolor", 	'b', POPT_ARG_NONE, 	&disable_colors, 0 },
    { "nomouse", 	'd', POPT_ARG_NONE, 	NULL, 			 'd' },
#ifdef HAVE_SUBSHELL_SUPPORT
    { "nosubshell", 	'u', POPT_ARG_NONE, 	NULL, 			 'u' },
    { "forceexec", 	'r', POPT_ARG_NONE, 	NULL, 			 'r' },
#endif
    { "printwd", 	'P', POPT_ARG_NONE, 	&print_last_wd, 	  0 },
    { "resetsoft", 	'k', POPT_ARG_NONE, 	&reset_hp_softkeys, 	  0 },
    { "slow", 's', POPT_ARG_NONE, 		&slow_terminal, 	  0 },
#if defined(HAVE_SLANG) && !defined(OS2_NT)
    { "stickchars",	'a', 0, 		&force_ugly_line_drawing, 0 },
#endif
#ifdef HAVE_SUBSHELL_SUPPORT
    { "subshell", 	'U', POPT_ARG_NONE, 	NULL, 			  'U' },
#endif
#if defined(HAVE_SLANG) && !defined(OS2_NT)
    { "termcap", 	't', 0, 		&SLtt_Try_Termcap, 	  0 },
#endif
    { "version", 	'V', 			POPT_ARG_NONE, NULL,      'V'},
    { "view", 		'v', 			POPT_ARG_STRING, &view_one_file, 0 },
    { "xterm", 		'x', 			POPT_ARG_NONE, &force_xterm, 0},

    { NULL, 		0,			0, NULL, 0 }
};

static void
handle_args (int argc, char *argv [])
{
    char   *tmp, *option_arg, *base;
    int    c;
    poptContext   optCon;

    optCon = poptGetContext ("mc", argc, argv, argumentTable, 0);

#ifdef USE_TERMCAP
    SLtt_Try_Termcap = 1;
#endif

    while ((c = poptGetNextOpt (optCon)) > 0) {
	option_arg = poptGetOptArg(optCon);

	process_args (c, option_arg);
    }

    if (c < -1){
	print_mc_usage ();
	fprintf(stderr, "%s: %s\n", 
		poptBadOption(optCon, POPT_BADOPTION_NOALIAS), 
		poptStrerror(c));
	finish_program = 1;
    }

    probably_finish_program ();

    /* Check for special invocation names mcedit and mcview,
     * if none apply then set the current directory and the other
     * directory from the command line arguments
     */
    tmp = poptGetArg (optCon);
    base = x_basename (argv[0]);
    if (!STRNCOMP (base, "mce", 3) || !STRCOMP(base, "vi")) {
	edit_one_file = "";
        if (tmp)
	    edit_one_file = strdup (tmp);
    } else
	if (!STRNCOMP (base, "mcv", 3) || !STRCOMP(base, "view")) {
	    if (tmp)
		view_one_file = strdup (tmp);
    } else {
       	/* sets the current dir and the other dir */
	if (tmp) {
    	    char buffer[MC_MAXPATHLEN + 2];
	    this_dir = strdup (tmp);
	    mc_get_current_wd (buffer, sizeof (buffer) - 2);
	    if ((tmp = poptGetArg (optCon)))
	        other_dir = strdup (tmp);
	}
    }
    poptFreeContext(optCon);
}
#endif

/*
 * The compatibility_move_mc_files routine is intended to
 * move all of the hidden .mc files into a private ~/.mc
 * directory in the home directory, to avoid cluttering the users.
 *
 * Previous versions of the program had all of their files in
 * the $HOME, we are now putting them in $HOME/.mc
 */
#ifdef OS2_NT
#    define compatibility_move_mc_files()
#else

int
do_mc_filename_rename (char *mc_dir, char *o_name, char *n_name)
{
	char *full_o_name = concat_dir_and_file (home_dir, o_name);
	char *full_n_name = copy_strings (home_dir, MC_BASE, n_name, NULL);
	int move;
	
	move = 0 == rename (full_o_name, full_n_name);
	free (full_o_name);
	free (full_n_name);
	return move;
}

void
do_compatibility_move (char *mc_dir)
{
	struct stat s;
	int move;
	
	if (stat (mc_dir, &s) == 0)
		return;
	if (errno != ENOENT)
		return;
	
	if (mkdir (mc_dir, 0777) == -1)
		return;

	move = do_mc_filename_rename (mc_dir, ".mc.ini", "ini");
	move += do_mc_filename_rename (mc_dir, ".mc.hot", "hotlist");
	move += do_mc_filename_rename (mc_dir, ".mc.ext", "ext");
	move += do_mc_filename_rename (mc_dir, ".mc.menu", "menu");
	move += do_mc_filename_rename (mc_dir, ".mc.bashrc", "bashrc");
	move += do_mc_filename_rename (mc_dir, ".mc.inputrc", "inputrc");
	move += do_mc_filename_rename (mc_dir, ".mc.tcshrc", "tcshrc");
	move += do_mc_filename_rename (mc_dir, ".mc.tree", "tree");

	if (!move)
		return;

	show_change_notice = 1;
}

void
compatibility_move_mc_files (void)
{
	char *mc_dir = concat_dir_and_file (home_dir, ".mc");
	
	do_compatibility_move (mc_dir);
	free (mc_dir);
}
#endif

int main (int argc, char *argv [])
{
#ifndef OS2_NT
    /* Backward compatibility: Gives up privileges in case someone
       installed the mc as setuid */
    setuid (getuid ());
#else
# define LOCALEDIR get_mc_lib_dir ()
#endif
#ifdef _OS_NT
  signal(SIGINT,SIG_IGN); /* Ignore CTRL-C */
#endif
    /* We had LC_CTYPE before, LC_ALL includs LC_TYPE as well */
    setlocale (LC_ALL, "");
    bindtextdomain ("mc", LOCALEDIR);
    textdomain ("mc");

    /* Initialize list of all user group for timur_clr_mode */
    init_groups ();
    
    OS_Setup ();

    vfs_init ();

#ifdef HAVE_X
    /* NOTE: This call has to be before any our argument handling :) */

#ifdef HAVE_GNOME
    session_management_setup (argv [0]);
    {
	char *base = x_basename (argv [0]);

	if (base){
	    if (strcmp (base, "mcedit") == 0)
		edit_one_file = "";
	    
	    if (strcmp (base, "mcview") == 0)
		view_one_file = "";
	}
    }
    gnome_init ("gmc", &mc_argp_parser, argc, argv, 0, NULL);
    probably_finish_program ();
#endif
    
    if (xtoolkit_init (&argc, argv) == -1)
	exit (1);
#endif /* HAVE_X */

    
#ifdef HAVE_SLANG
    SLtt_Ignore_Beep = 1;
#endif

    /* NOTE: This has to be called before slang_init or whatever routine
       calls any define_sequence */
    init_key ();

#ifndef PORT_WANTS_ARGP
    handle_args (argc, argv);
#endif
    
    /* Used to report the last working directory at program end */
    if (print_last_wd){
#ifndef OS2_NT	
	stdout_fd = dup (1);
	close (1);
	if (open (ttyname (0), O_RDWR) < 0)
	    if (open ("/dev/tty", O_RDWR) < 0) {
	    /* Try if stderr is not redirected as the last chance */
	        char *p = strdup (ttyname (0));
	        
	        if (!strcmp (p, ttyname (2)))
	            dup2 (2, 1);
	        else {
	            fprintf (stderr,
	            	     _("Couldn't open tty line. You have to run mc without the -P flag.\n"
			       "On some systems you may want to run # `which mc`\n"));
		    exit (1);
	        }
	        free (p);
	    }
#endif
    }

#   ifdef HAVE_X
    /* This is to avoid subshell trying to restard any child pid
     * that happends to have cons_saver_pid (a random startup value).
     * and PID 1 is init, unlikely we could be the parent of it.
     */
/*    cons_saver_pid = 1; */
#   else
    /* Must be done before installing the SIGCHLD handler [[FIXME]] */
    handle_console (CONSOLE_INIT);
#   endif
    
#   ifdef HAVE_SUBSHELL_SUPPORT
    subshell_get_console_attributes ();
#   endif
    
    /* Install the SIGCHLD handler; must be done before init_subshell() */
    init_sigchld ();
    init_sigfatals ();
    
    /* This variable is used by the subshell */
    home_dir = getenv ("HOME");
    if (!home_dir) {
#ifndef OS2_NT
    home_dir = PATH_SEP_STR;
#else
    home_dir = mc_home; /* LIBDIR, calculated in OS_Setup() */
#endif
    }

    compatibility_move_mc_files ();
    
#   ifdef HAVE_X
    /* We need this, since ncurses endwin () doesn't restore the signals */
    save_stop_handler ();
#   endif

    /* Must be done before init_subshell, to set up the terminal size: */
    /* FIXME: Should be removed and LINES and COLS computed on subshell */
    slang_init ();
    /* NOTE: This call has to be after slang_init. It's the small part from
    the previous init_key which had to be moved after the call of slang_init */ 
    init_key_input_fd ();

    load_setup ();

#ifdef HAVE_GNOME
    init_colors ();
#else
    init_curses ();
#endif

#ifdef HAVE_GNOME
    use_subshell = 0;
#endif
    
#   ifdef HAVE_SUBSHELL_SUPPORT
	/* Done here to ensure that the subshell doesn't  */
	/* inherit the file descriptors opened below, etc */

	if (use_subshell)
	    init_subshell ();  
#   endif

#   ifndef HAVE_X
    /* Removing this from the X code let's us type C-c */
    load_key_defs ();

    /* Also done after init_subshell, to save any shell init file messages */
    if (console_flag)
	handle_console (CONSOLE_SAVE);
    
    if (alternate_plus_minus)
        application_keypad_mode ();
#   endif

    /* The directory hot list */
    load_hotlist ();

    if (show_change_notice){
	message (1, _(" Notice "),
		 _(" The Midnight Commander configuration files \n"
		   " are now stored in the ~/.mc directory, the \n"
		   " files have been moved now\n"));
    }
    
#   ifdef HAVE_SUBSHELL_SUPPORT
	if (use_subshell){
	    prompt = strip_ctrl_codes (subshell_prompt);
	    if (!prompt)
		prompt = "";
	} else
#   endif
	    prompt = (geteuid () == 0) ? "# " : "$ ";

    /* Program main loop */
    do_nc ();
    
    /* Virtual File System shutdown */
    vfs_shut ();

    /* Delete list of all user groups*/
    delete_groups ();

    flush_extension_file (); /* does only free memory */

#   ifndef HAVE_X
    /* Miguel, maybe the fix in slang is not required and
     * it could be done by removing the slang_done_screen.
     * Do I need to call slang_reset_tty then?
     */
    endwin ();
    slang_shutdown ();

    if (console_flag && !(quit & SUBSHELL_EXIT))
	restore_console ();
    if (alternate_plus_minus)
        numeric_keypad_mode ();
#   endif

#ifndef OS2_NT
    signal (SIGCHLD, SIG_DFL);  /* Disable the SIGCHLD handler */
#endif
    
#   ifndef HAVE_X
    if (console_flag)
	handle_console (CONSOLE_DONE);
    putchar ('\r');  /* Hack to make shell's prompt start at left of screen */
#   endif

#ifdef _OS_NT
    /* On NT, home_dir is malloced */
//    free (home_dir);
#endif
#if defined(OS2_NT)
    if (print_last_wd == 2){
	FILE *bat_file;

	print_last_wd = 0;
	bat_file = fopen(batch_file_name, "w");
	if (bat_file != NULL){
            /* .ado: \r\n for Win95 */
	    fprintf(bat_file, "@echo off\r\n");
	    if (isalpha(last_wd_string[0]))
		fprintf(bat_file, "%c:\r\n", last_wd_string[0]);
	    fprintf(bat_file, "cd %s\r\n", last_wd_string);
	    fclose(bat_file);
	}
    }
#endif
    if (print_last_wd) {
        if (print_last_revert || edit_one_file || view_one_file)
            write (stdout_fd, ".", 1);
        else
	    write (stdout_fd, last_wd_string, strlen (last_wd_string));
	free (last_wd_string);
    }

#ifdef HAVE_MAD
    done_key ();
#endif
    mad_finalize (__FILE__, __LINE__);
#ifdef HAVE_X
    xtoolkit_end ();
#endif
    return 0;
}

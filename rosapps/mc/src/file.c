/* {{{ Copyright */

/* File managing.  Important notes on this file:
   
   About the use of dialogs in this file:
     If you want to add a new dialog box (or call a routine that pops
     up a dialog box), you have to provide a wrapper for background
     operations (ie, background operations have to up-call to the parent
     process).

     For example, instead of using the message() routine, in this
     file, you should use one of the stubs that call message with the
     proper number of arguments (ie, message_1s, message_2s and so on).

     Actually, that is a rule that should be followed by any routines
     that may be called from this module.

*/

/* File managing
   Copyright (C) 1994, 1995, 1996 The Free Software Foundation
   
   Written by: 1994, 1995       Janne Kukonlehto
               1994, 1995       Fred Leeflang
               1994, 1995, 1996 Miguel de Icaza
               1995, 1996       Jakub Jelinek
	       1997             Norbert Warmuth
	       1998		Pavel Machek

   The copy code was based in GNU's cp, and was written by:
   Torbjorn Granlund, David MacKenzie, and Jim Meyering.

   The move code was based in GNU's mv, and was written by:
   Mike Parker and David MacKenzie.

   Janne Kukonlehto added much error recovery to them for being used
   in an interactive program.

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

/* }}} */

/* {{{ Include files */

#include <config.h>
/* Hack: the vfs code should not rely on this */
#define WITH_FULL_PATHS 1

#include <sys/types.h>
#include <dirent.h>
#include <stdio.h>
#ifdef OS2_NT
#    include <io.h>
#endif 

#include <errno.h>
#include "tty.h"
#include <ctype.h>
#include <malloc.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#   include <unistd.h>
#endif
#include <sys/stat.h>
#include <sys/param.h>
#include <fcntl.h>
#ifdef SCO_FLAVOR
#	include <sys/timeb.h>	/* alex: for struct timeb, used in time.h */
#endif /* SCO_FLAVOR */
#if defined (__MINGW32__) || defined(_MSC_VER)
#include <sys/time.h___>
#else
#include <time.h>
#endif
#include <utime.h>
#include "mad.h"
#include "regex.h"
#include "util.h"
#include "dialog.h"
#include "global.h"
/* Needed by query_replace */
#include "color.h"
#include "win.h"
#include "dlg.h"
#include "widget.h"
#define WANT_WIDGETS
#include "main.h"		/* WANT_WIDGETS-> we get the the_hint def */
#include "file.h"
#include "layout.h"
#include "widget.h"
#include "wtools.h"
#include "background.h"

/* Needed for current_panel, other_panel and WTree */
#include "dir.h"
#include "panel.h"
#include "tree.h"
#include "key.h"
#include "../vfs/vfs.h"

#include "x.h"

/* }}} */

#if USE_VFS && USE_NETCODE
extern
#else
static
#endif

int do_reget;

/* rcsid [] = "$Id: file.c,v 1.1 2001/12/30 09:55:25 sedwards Exp $" */
int verbose = 1;

/* Recursive operation on subdirectories */
int dive_into_subdirs = 0;

/* When moving directories cross filesystem boundaries delete the successfull
   copied files when all files below the directory and its subdirectories 
   were processed. 
   If erase_at_end is zero files will be deleted immediately after their
   successful copy (Note: this behaviour is not tested and at the moment
   it can't be changed at runtime) */
int erase_at_end = 1;

/* Preserve the original files' owner, group, permissions, and
   timestamps (owner, group only as root). */
int preserve;

/* The value of the "preserve Attributes" checkbox in the copy file dialog.
   We can't use the value of "preserve" because it can change in order to
   preserve file attributs when moving files across filesystem boundaries
   (we want to keep the value of the checkbox between copy operations). */
int op_preserve = 1;

/* If running as root, preserve the original uid/gid
   (we don't want to try chwon for non root)
   preserve_uidgid = preserve && uid == 0 */
int preserve_uidgid = 0;

/* The bits to preserve in created files' modes on file copy */
int umask_kill = 0777777;

/* If on, it gets a little scrict with dangerous operations */
int know_not_what_am_i_doing = 0;

int stable_symlinks = 0;

/* The next two are not static, since they are used on background.c */
/* Controls appending to files, shared with filequery.c */
int do_append = 0;

/* With ETA on we have extra screen space */
int eta_extra = 0;

/* result from the recursive query */
int recursive_result;

/* The estimated time of arrival in seconds */
double eta_secs;

/* Used to save the hint line */
static int last_hint_line;

/* mapping operations into names */
char *operation_names [] = { "Copy", "Move", "Delete" };

/* This is a hard link cache */
struct link {
    struct link *next;
    vfs *vfs;
    dev_t dev;
    ino_t ino;
    short linkcount;
    umode_t st_mode;
    char name[1];
};

/* the hard link cache */
struct link *linklist = NULL;

/* the files-to-be-erased list */
struct link *erase_list;

/* In copy_dir_dir we use two additional single linked lists: The first - 
   variable name `parent_dirs' - holds information about already copied 
   directories and is used to detect cyclic symbolic links. 
   The second (`dest_dirs' below) holds information about just created 
   target directories and is used to detect when an directory is copied 
   into itself (we don't want to copy infinitly). 
   Both lists don't use the linkcount and name structure members of struct
   link. */
struct link *dest_dirs = 0;

struct re_pattern_buffer rx;
struct re_registers regs;
static char *dest_mask = NULL;

/* To symlinks the difference between `follow Links' checked and not
   checked is the stat call used (mc_stat resp. mc_lstat) */
int (*xstat)(char *, struct stat *) = mc_lstat;

static int op_follow_links = 0;

/* File operate window sizes */
#define WX 62
#define WY 10
#define BY 10
#define WX_ETA_EXTRA  12

#define FCOPY_GAUGE_X 14
#define FCOPY_LABEL_X 5

/* Used for button result values */
enum {
    REPLACE_YES = B_USER,
    REPLACE_NO,
    REPLACE_APPEND,
    REPLACE_ALWAYS,
    REPLACE_UPDATE,
    REPLACE_NEVER,
    REPLACE_ABORT,
    REPLACE_SIZE,
    REPLACE_REGET
};

enum {
    RECURSIVE_YES,
    RECURSIVE_NO,
    RECURSIVE_ALWAYS,
    RECURSIVE_NEVER,
    RECURSIVE_ABORT
};

/* Pointer to the operate dialog */
static   Dlg_head *op_dlg;
int      showing_eta;
int      showing_bps;
unsigned long bps = 0, bps_time = 0;

static char *op_names [] = { N_(" Copy "), N_(" Move "), N_(" Delete ") };
static int selected_button;
static int last_percentage [3];

/* Replace dialog: color set, descriptor and filename */
static int replace_colors [4];
static Dlg_head *replace_dlg;
static char *replace_filename;
static int replace_result;

static struct stat *s_stat, *d_stat;

static int recursive_erase (char *s);
static int erase_file (char *s);

/* Describe the components in the panel operations window */
static WLabel *FileLabel [2];
static WLabel *FileString [2];
static WLabel *ProgressLabel [3];
static WGauge *ProgressGauge [3];
static WLabel *eta_label;
static WLabel *bps_label;
static WLabel *stalled_label;

/* }}} */

/* {{{ File progress display routines */

#ifndef HAVE_X
static int
check_buttons (void)
{
    int c;
    Gpm_Event event;

    c = get_event (&event, 0, 0);
    if (c == EV_NONE)
      return FILE_CONT;
    dlg_process_event (op_dlg, c, &event);
    switch (op_dlg->ret_value) {
    case FILE_SKIP:
	return FILE_SKIP;
	break;
    case B_CANCEL:
    case FILE_ABORT:
	return FILE_ABORT;
	break;
    default:
	return FILE_CONT;
    }
}
#else

#ifdef HAVE_TK
static int
check_buttons (void)
{
    tk_dispatch_all ();
    if (op_dlg->running)
        return FILE_CONT;
}
#endif /* HAVE_TK */

#ifdef HAVE_XVIEW
static int
check_buttons (void)
{
    xv_dispatch_something ();
    if (op_dlg->running)
        return FILE_CONT;
}
#endif /* HAVE_XVIEW */

#ifdef HAVE_GNOME
#include <gtk/gtk.h>
static int
check_buttons (void)
{
    x_flush_events ();
    
    if (op_dlg->running)
        return FILE_CONT;

    if (op_dlg->ret_value == B_CANCEL)
	return FILE_ABORT;
    else
	return op_dlg->ret_value;
}
#endif /* HAVE_GNOME */

#endif /* HAVE_X */

static int
op_win_callback (struct Dlg_head *h, int id, int msg)
{
    switch (msg){
#ifndef HAVE_X    
    case DLG_DRAW:
	attrset (COLOR_NORMAL);
	dlg_erase (h);
	draw_box (h, 1, 2, h->lines-2, h->cols-4);
	return 1;
#endif
    }
    return 0;
}

void
create_op_win (int op, int with_eta)
{
    int i, x_size;
    int minus = verbose ? 0 : 3;
    int eta_offset = with_eta ? (WX_ETA_EXTRA) / 2 : 0;

#ifdef HAVE_XVIEW    
    char *sixty = "                                                                                   ";
    char *fifteen = "               ";
#else
    char *sixty = "";
    char *fifteen = "";
#endif    
    replace_result = 0;
    recursive_result = 0;
    showing_eta = with_eta;
    showing_bps = with_eta;
    eta_extra = with_eta ? WX_ETA_EXTRA : 0;
    x_size = (WX + 4) + eta_extra;

    op_dlg = create_dlg (0, 0, WY-minus+4, x_size, dialog_colors,
			 op_win_callback, "", "opwin", DLG_CENTER);

#ifndef HAVE_X
    last_hint_line = the_hint->widget.y;
    if ((op_dlg->y + op_dlg->lines) > last_hint_line)
	the_hint->widget.y = op_dlg->y + op_dlg->lines+1;
#endif
    
    x_set_dialog_title (op_dlg, "");

    tk_new_frame (op_dlg, "b.");
    add_widgetl (op_dlg, button_new (BY-minus, WX - 19 + eta_offset, FILE_ABORT,
				     NORMAL_BUTTON, _("&Abort"), 0, 0, "abort"),
        XV_WLAY_RIGHTOF);
    add_widgetl (op_dlg, button_new (BY-minus, 14 + eta_offset, FILE_SKIP,
				     NORMAL_BUTTON, _("&Skip"), 0, 0, "skip"),
        XV_WLAY_CENTERROW);

    tk_new_frame (op_dlg, "2.");
    add_widgetl (op_dlg, ProgressGauge [2] = gauge_new (7, FCOPY_GAUGE_X, 0, 100, 0, "g-1"), 
        XV_WLAY_RIGHTOF);
    add_widgetl (op_dlg, ProgressLabel [2] = label_new (7, FCOPY_LABEL_X, fifteen, "l-1"), 
        XV_WLAY_NEXTROW);
    add_widgetl (op_dlg, bps_label = label_new (7, WX, "", "bps-label"), XV_WLAY_NEXTROW);

        tk_new_frame (op_dlg, "1.");
    add_widgetl (op_dlg, ProgressGauge [1] = gauge_new (8, FCOPY_GAUGE_X, 0, 100, 0, "g-2"), 
        XV_WLAY_RIGHTOF);
    add_widgetl (op_dlg, ProgressLabel [1] = label_new (8, FCOPY_LABEL_X, fifteen, "l-2"), 
        XV_WLAY_RIGHTOF);
    add_widgetl (op_dlg, stalled_label = label_new (8, WX, "", "stalled"), XV_WLAY_NEXTROW);
	
    tk_new_frame (op_dlg, "0.");
    add_widgetl (op_dlg, ProgressGauge [0] = gauge_new (6, FCOPY_GAUGE_X, 0, 100, 0, "g-3"), 
        XV_WLAY_RIGHTOF);
    add_widgetl (op_dlg, ProgressLabel [0] = label_new (6, FCOPY_LABEL_X, fifteen, "l-3"), 
        XV_WLAY_RIGHTOF);
    add_widgetl (op_dlg, eta_label = label_new (6, WX, "", "eta_label"), XV_WLAY_NEXTROW);
    
    tk_new_frame (op_dlg, "f1.");
    add_widgetl (op_dlg, FileString [1] = label_new (4, FCOPY_GAUGE_X, sixty, "fs-l-1"),
        XV_WLAY_RIGHTOF);
    add_widgetl (op_dlg, FileLabel [1] = label_new (4, FCOPY_LABEL_X, fifteen, "fs-l-2"), 
        XV_WLAY_NEXTROW);
    tk_new_frame (op_dlg, "f0.");
    add_widgetl (op_dlg, FileString [0] = label_new (3, FCOPY_GAUGE_X, sixty, "fs-x-1"),
        XV_WLAY_RIGHTOF);
    add_widgetl (op_dlg, FileLabel [0] = label_new (3, FCOPY_LABEL_X, fifteen, "fs-x-2"), 
        XV_WLAY_NEXTROW);
	
    /* We will manage the dialog without any help, that's why
       we have to call init_dlg */
    init_dlg (op_dlg);
    op_dlg->running = 1;
    selected_button = FILE_SKIP;
    for (i = 0; i < 3; i++)
	last_percentage [i] = -99;
}

void
destroy_op_win (void)
{
#ifdef HAVE_XVIEW
    xtoolkit_kill_dialog (op_dlg);
#endif
    dlg_run_done (op_dlg);
    destroy_dlg (op_dlg);
#ifndef HAVE_X
    the_hint->widget.y = last_hint_line;
#endif
}

static int
show_no_bar (int n)
{
    if (n >= 0) {
    	label_set_text (ProgressLabel [n], "");
        gauge_show (ProgressGauge [n], 0);
    }
    return check_buttons ();
}

#ifndef HAVE_X
#define truncFileString(s) name_trunc (s, eta_extra + 47)
#else
#define truncFileString(s) s
#endif

static int
show_source (char *s)
{
    if (s != NULL){

#ifdef WITH_FULL_PATHS
    	int i = strlen (cpanel->cwd);

	/* We remove the full path we have added before */
        if (!strncmp (s, cpanel->cwd, i)){ 
            if (s[i] == PATH_SEP)
            	s += i + 1;
        }
#endif /* WITH_FULL_PATHS */
	
	label_set_text (FileLabel [0], _("Source"));
	label_set_text (FileString [0], truncFileString (s));
	return check_buttons ();
    } else {
	label_set_text (FileLabel [0], "");
	label_set_text (FileString [0], "");
	return check_buttons ();
    }
}

static int
show_target (char *s)
{
    if (s != NULL){
	label_set_text (FileLabel [1], _("Target"));
	label_set_text (FileString [1], truncFileString (s));
	return check_buttons ();
    } else {
	label_set_text (FileLabel [1], "");
	label_set_text (FileString [1], "");
	return check_buttons ();
    }
}

static int
show_deleting (char *s)
{
    label_set_text (FileLabel [0], _("Deleting"));
    label_set_text (FileString [0], truncFileString (s));
    return check_buttons ();
}

static int
show_bar (int n, long done, long total)
{
    gauge_set_value (ProgressGauge [n], (int) total, (int) done);
    gauge_show (ProgressGauge [n], 1);
    return check_buttons ();
}

static void
file_eta_show ()
{
    int eta_hours, eta_mins, eta_s;
    char eta_buffer [30];

    if (!showing_eta)
	return;
    
    eta_hours = eta_secs / (60 * 60);
    eta_mins  = (eta_secs - (eta_hours * 60 * 60)) / 60;
    eta_s     = eta_secs - ((eta_hours * 60 * 60) + eta_mins * 60 );
    sprintf (eta_buffer, "ETA %d:%02d.%02d", eta_hours, eta_mins, eta_s);
    label_set_text (eta_label, eta_buffer);
}

static void
file_bps_show ()
{
    char bps_buffer [30];

    if (!showing_bps)
	return;

    if (bps > 1024){
	    if (bps > 1024*1024){
		    sprintf (bps_buffer, "%.2f MBS", bps / (1024*1024.0));
	    } else
		    sprintf (bps_buffer, "%.2f KBS", bps / 1024.0);
    } else 
	    sprintf (bps_buffer, "%ld BPS", bps);
    label_set_text (bps_label, bps_buffer);
}

static int
show_file_progress (long done, long total)
{
    if (!verbose)
	return check_buttons ();
    if (total > 0){
	label_set_text (ProgressLabel [0], _("File"));
	file_eta_show ();
	file_bps_show ();
	return show_bar (0, done, total);
    } else
	return show_no_bar (0);
}

static int
show_count_progress (long done, long total)
{
    if (!verbose)
	return check_buttons ();
    if (total > 0){
	label_set_text (ProgressLabel [1], _("Count"));
	return show_bar (1, done, total);
    } else
	return show_no_bar (1);
}

static int
show_bytes_progress (long done, long total)
{
    if (!verbose)
	return check_buttons ();
    if (total > 0){
	label_set_text (ProgressLabel [2], _("Bytes"));
	return show_bar (2, done, total);
    } else
	return show_no_bar (2);
}

/* }}} */


/* {{{ Copy routines */

enum CaseConvs { NO_CONV=0, UP_CHAR=1, LOW_CHAR=2, UP_SECT=4, LOW_SECT=8 };

int
convert_case (int c, enum CaseConvs *conversion)
{
    if (*conversion & UP_CHAR){
	*conversion &= ~UP_CHAR;
	return toupper (c);
    } else if (*conversion & LOW_CHAR){
	*conversion &= ~LOW_CHAR;
	return tolower (c);
    } else if (*conversion & UP_SECT){
	return toupper (c);
    } else if (*conversion & LOW_SECT){
	return tolower (c);
    } else
	return c;
}

static int transform_error = 0;
static char *
do_transform_source (char *source)
{
    int j, k, l, len;
    char *fnsource = x_basename (source);
    int next_reg;
    enum CaseConvs case_conv = NO_CONV;
    static char fntarget [MC_MAXPATHLEN];
    
    len = strlen (fnsource);
    j = re_match (&rx, fnsource, len, 0, &regs);
    if (j != len) {
        transform_error = FILE_SKIP;
    	return NULL;
    }
    for (next_reg = 1, j = 0, k = 0; j < strlen (dest_mask); j++) {
        switch (dest_mask [j]) {
	case '\\':
	    j++;
	    if (! isdigit (dest_mask [j])){
		/* Backslash followed by non-digit */
		switch (dest_mask [j]){
		case 'U':
		    case_conv |= UP_SECT;
		    case_conv &= ~LOW_SECT;
		    break;
		case 'u':
		    case_conv |= UP_CHAR;
		    break;
		case 'L':
		    case_conv |= LOW_SECT;
		    case_conv &= ~UP_SECT;
		    break;
		case 'l':
		    case_conv |= LOW_CHAR;
		    break;
		case 'E':
		    case_conv = NO_CONV;
		    break;
		default:
		    /* Backslash as quote mark */
		    fntarget [k++] = convert_case (dest_mask [j], &case_conv);
		}
		break;
	    } else {
		/* Backslash followed by digit */
		next_reg = dest_mask [j] - '0';
		/* Fall through */
	    }
                
	case '*':
	    if (next_reg < 0 || next_reg >= RE_NREGS
		|| regs.start [next_reg] < 0) {
		message_1s (1, MSG_ERROR, _(" Invalid target mask "));
		transform_error = FILE_ABORT;
		return NULL;
	    }
	    for (l = regs.start [next_reg]; l < regs.end [next_reg]; l++)
		fntarget [k++] = convert_case (fnsource [l], &case_conv);
	    next_reg ++;
	    break;
            	
	default:
	    fntarget [k++] = convert_case (dest_mask [j], &case_conv);
	    break;
        }
    }
    fntarget [k] = 0;
    return fntarget;
}

static char *
transform_source (char *source)
{
    char *s = strdup (source);
    char *q;

    /* We remove \n from the filename since regex routines would use \n as an anchor */
    /* this is just to be allowed to maniupulate file names with \n on it */
    for (q = s; *q; q++){
	if (*q == '\n')
	    *q = ' ';
    }
    q = do_transform_source (s);
    free (s);
    return q;
}

void
free_linklist (struct link **linklist)
{
    struct link *lp, *lp2;
    
    for (lp = *linklist; lp != NULL; lp = lp2){
    	lp2 = lp -> next;
    	free (lp);
    }
    *linklist = NULL;
}

#ifdef USE_VFS
int 
is_in_linklist (struct link *lp, char *path, struct stat *sb)
{
   ino_t ino = sb->st_ino;
   dev_t dev = sb->st_dev;
   vfs *vfs = vfs_type (path);
   
   while (lp) {
      if (lp->vfs == vfs && lp->ino == ino && lp->dev == dev )
          return 1;
      lp = lp->next;
   }
   return 0;
}
#else
int 
is_in_linklist (struct link *lp, char *path, struct stat *sb)
{
   ino_t ino = sb->st_ino;
   dev_t dev = sb->st_dev;
   
   while (lp) {
      if (lp->ino == ino && lp->dev == dev )
          return 1;
      lp = lp->next;
   }
   return 0;
}
#endif

/* Returns 0 if the inode wasn't found in the cache and 1 if it was found
   and a hardlink was succesfully made */
int
check_hardlinks (char *src_name, char *dst_name, struct stat *pstat)
{
    struct link *lp;
    vfs *my_vfs = vfs_type (src_name);
    ino_t ino = pstat->st_ino;
    dev_t dev = pstat->st_dev;
    struct stat link_stat;
    char *p;

    if (vfs_file_is_ftp (src_name))
        return 0;
    for (lp = linklist; lp != NULL; lp = lp -> next)
        if (lp->vfs == my_vfs && lp->ino == ino && lp->dev == dev){
            if (!mc_stat (lp->name, &link_stat) && link_stat.st_ino == ino &&
                link_stat.st_dev == dev && vfs_type (lp->name) == my_vfs){
                p = strchr (lp->name, 0) + 1; /* i.e. where the `name' file
            				         was copied to */
                if (vfs_type (dst_name) == vfs_type (p)){
            	    if (!mc_stat (p, &link_stat)){
            	    	if (!mc_link (p, dst_name))
            	    	    return 1;
            	    }
            	}
            }
            /* FIXME: Announce we couldn't make the hardlink */
            return 0;
        }
    lp = (struct link *) xmalloc (sizeof (struct link) + strlen (src_name) 
                                  + strlen (dst_name) + 1, "Hardlink cache");
    if (lp){
    	lp->vfs = my_vfs;
    	lp->ino = ino;
    	lp->dev = dev;
    	strcpy (lp->name, src_name);
    	p = strchr (lp->name, 0) + 1;
    	strcpy (p, dst_name);
    	lp->next = linklist;
    	linklist = lp;
    }
    return 0;
}

/* Duplicate the contents of the symbolic link src_path in dst_path.
   Try to make a stable symlink if the option "stable symlink" was
   set in the file mask dialog.
   If dst_path is an existing symlink it will be deleted silently
   (upper levels take already care of existing files at dst_path).
  */
static int 
make_symlink (char *src_path, char *dst_path)
{
    char link_target[MC_MAXPATHLEN];
    int len;
    int return_status;
    struct stat sb;
    int dst_is_symlink;
    
    if (mc_lstat (dst_path, &sb) == 0 && S_ISLNK (sb.st_mode)) 
	dst_is_symlink = 1;
    else
	dst_is_symlink = 0;

  retry_src_readlink:
    len = mc_readlink (src_path, link_target, MC_MAXPATHLEN);
    if (len < 0) {
	return_status = file_error
	    (_(" Cannot read source link \"%s\" \n %s "), src_path);
	if (return_status == FILE_RETRY)
	    goto retry_src_readlink;
	return return_status;
    }
    link_target[len] = 0;

    if (stable_symlinks && (!vfs_file_is_local (src_path) || 
			    !vfs_file_is_local (dst_path))) {
	message_1s (1, MSG_ERROR, _(" Cannot make stable symlinks across "
				    "non-local filesystems: \n\n"
				    " Option Stable Symlinks will be disabled "));
	stable_symlinks = 0;
    }
    
    if (stable_symlinks && *link_target != PATH_SEP) {
	char *p, *q, *r, *s;

	p = strdup (src_path);
	r = strrchr (p, PATH_SEP);
	if (r) {
	    r[1] = 0;
	    if (*dst_path == PATH_SEP)
		q = strdup (dst_path);
	    else
		q = copy_strings (p, dst_path, 0);
	    r = strrchr (q, PATH_SEP);
	    if (r) {
		r[1] = 0;
		s = copy_strings (p, link_target, NULL);
		strcpy (link_target, s);
		free (s);
		s = diff_two_paths (q, link_target);
		if (s) {
		    strcpy (link_target, s);
		    free (s);
		}
	    }
	    free (q);
	}
	free (p);
    }
  retry_dst_symlink:
    if (mc_symlink (link_target, dst_path) == 0)
	/* Success */
	return FILE_CONT;
    /*
     * if dst_exists, it is obvious that this had failed.
     * We can delete the old symlink and try again...
     */
    if (dst_is_symlink) {
	if (!mc_unlink (dst_path))
	    if (mc_symlink (link_target, dst_path) == 0)
		/* Success */
		return FILE_CONT;
    }
    return_status = file_error
	(_(" Cannot create target symlink \"%s\" \n %s "), dst_path);
    if (return_status == FILE_RETRY)
	goto retry_dst_symlink;
    return return_status;
}


int
copy_file_file (char *src_path, char *dst_path, int ask_overwrite)
{
#ifndef OS2_NT
    uid_t src_uid;
    gid_t src_gid;
#endif
    char *buf = 0;
    int  buf_size = 8*1024;
    int  dest_desc = 0;
    int  source_desc;
    int  n_read;
    int  n_written;
    int  src_mode;		/* The mode of the source file */
    struct stat sb, sb2;
    struct utimbuf utb;
    int  dst_exists = 0;
    long n_read_total = 0;
    long file_size;
    int  return_status, temp_status;
    int do_remote_copy = 0;
    int appending = 0;
    /* bitmask used to remember which resourses we should release on return 
       A single goto label is much easier to handle than a bunch of gotos ;-). */ 
    unsigned resources = 0; 

    return_status = FILE_RETRY;

    if (show_source (src_path) == FILE_ABORT
	|| show_target (dst_path) == FILE_ABORT)
	return FILE_ABORT;
    mc_refresh ();

 retry_dst_stat:
    if (mc_stat (dst_path, &sb2) == 0){
	if (S_ISDIR (sb2.st_mode)){
	    return_status = file_error (_(" Cannot overwrite directory \"%s\" \n %s "),
					dst_path);
	    if (return_status == FILE_RETRY)
		goto retry_dst_stat;
	    return return_status;
	}
	dst_exists = 1;
    }

 retry_src_xstat:
    if ((*xstat)(src_path, &sb)){
	return_status = file_error (_(" Cannot stat source file \"%s\" \n %s "),
				    src_path);
	if (return_status == FILE_RETRY)
	    goto retry_src_xstat;
	return return_status;
    } 
    
    if (dst_exists){
	    /* .ado: For OS/2 or NT: no st_ino exists, it is better to just try to
	     * overwrite the target file
	     */
#ifndef OS2_NT
	/* Destination already exists */
	if (sb.st_dev == sb2.st_dev && sb.st_ino == sb2.st_ino){
	    message_3s (1, MSG_ERROR, _(" `%s' and `%s' are the same file. "),
		     src_path, dst_path);
	    do_refresh ();
	    return FILE_SKIP;
	}
#endif

	/* Should we replace destination? */
	if (ask_overwrite) {
	    if (vfs_file_is_ftp (src_path))
		do_reget = -1;
	    else
		do_reget = 0;
		    
	    return_status = query_replace (dst_path, &sb, &sb2);
	    if (return_status != FILE_CONT)
	        return return_status;
	}
    }

    if (!do_append) {
       /* .ado: OS2 and NT don't have hardlinks */
#ifndef OS2_NT    
        /* Check the hardlinks */
        if (!op_follow_links && sb.st_nlink > 1 && 
	     check_hardlinks (src_path, dst_path, &sb) == 1) {
    	    /* We have made a hardlink - no more processing is necessary */
    	    return return_status;
        }
	
        if (S_ISLNK (sb.st_mode))
	    return make_symlink (src_path, dst_path);
	    
#endif /* !OS_NT */

        if (S_ISCHR (sb.st_mode) || S_ISBLK (sb.st_mode) || S_ISFIFO (sb.st_mode)
            || S_ISSOCK (sb.st_mode)){
        retry_mknod:        
            if (mc_mknod (dst_path, sb.st_mode & umask_kill, sb.st_rdev) < 0){
	        return_status = file_error
		    (_(" Cannot create special file \"%s\" \n %s "), dst_path);
	        if (return_status == FILE_RETRY)
		    goto retry_mknod;
		return return_status;
	    }
	    /* Success */
	    
#ifndef OS2_NT
	retry_mknod_uidgid:
	    if (preserve_uidgid && mc_chown (dst_path, sb.st_uid, sb.st_gid)){
		temp_status = file_error
		    (_(" Cannot chown target file \"%s\" \n %s "), dst_path);
		if (temp_status == FILE_RETRY)
		    goto retry_mknod_uidgid;
		return temp_status;
	    }
#endif
#ifndef __os2__
	retry_mknod_chmod:
	    if (preserve && mc_chmod (dst_path, sb.st_mode & umask_kill) < 0){
		temp_status = file_error (_(" Cannnot chmod target file \"%s\" \n %s "), dst_path);
		if (temp_status == FILE_RETRY)
		    goto retry_mknod_chmod;
		return temp_status;
	    }
#endif
	    return FILE_CONT;
        }
    }
    
    if (!do_append && !vfs_file_is_local (src_path) && vfs_file_is_local (dst_path)){
	mc_setctl (src_path, MCCTL_SETREMOTECOPY, dst_path);
    }
 retry_src_open:
    if ((source_desc = mc_open (src_path, O_RDONLY)) < 0){
	return_status = file_error
	    (_(" Cannot open source file \"%s\" \n %s "), src_path);
	if (return_status == FILE_RETRY)
	    goto retry_src_open;
	do_append = 0;
	return return_status;
    }

    resources |= 1;
    do_remote_copy = mc_ctl (source_desc, MCCTL_ISREMOTECOPY, 0);
    
    if (!do_remote_copy) {
 retry_src_fstat:
        if (mc_fstat (source_desc, &sb)){
	    return_status = file_error
	        (_(" Cannot fstat source file \"%s\" \n %s "), src_path);
	    if (return_status == FILE_RETRY)
	        goto retry_src_fstat;
	    do_append = 0;
            goto ret;
        }
#if 0	
    /* Im not sure if we can delete this. sb is already filled by 
    (*xstat)() - Norbert. */
    } else {
 retry_src_rstat:
        if (mc_stat (src_path, &sb)){
	    return_status = file_error
	        (_(" Cannot stat source file \"%s\" \n %s "), src_path);
	    if (return_status == FILE_RETRY)
	        goto retry_src_rstat;
	    do_append = 0;
            goto ret;
        }
#endif    
    }
    src_mode = sb.st_mode;
#ifndef OS2_NT
    src_uid = sb.st_uid;
    src_gid = sb.st_gid;
#endif
    utb.actime = sb.st_atime;
    utb.modtime = sb.st_mtime;
    file_size = sb.st_size;

    /* Create the new regular file with small permissions initially,
       do not create a security hole.  */

    if (!do_remote_copy) {
 retry_dst_open:
        if ((do_append && 
            (dest_desc = mc_open (dst_path, O_WRONLY | O_APPEND)) < 0) ||
            (!do_append &&
            (dest_desc = mc_open (dst_path, O_WRONLY | O_CREAT | O_TRUNC, 0600)) < 0)) {
	    return_status = file_error
	        (_(" Cannot create target file \"%s\" \n %s "), dst_path);
	    if (return_status == FILE_RETRY)
	        goto retry_dst_open;
	    do_append = 0;
	    goto ret;
        }
	resources |= 2; /* dst_path exists/dst_path opened */
	resources |= 4; /* remove short file */
    }
    appending = do_append;
    do_append = 0;

    if (!do_remote_copy) {
 retry_dst_fstat:
        /* Find out the optimal buffer size.  */
        if (mc_fstat (dest_desc, &sb)){
	    return_status = file_error
	        (_(" Cannot fstat target file \"%s\" \n %s "), dst_path);
	    if (return_status == FILE_RETRY)
	        goto retry_dst_fstat;
	    goto ret;
        }
        buf_size = 8*1024;

        buf = (char *) xmalloc (buf_size, "copy_file_file");
    }

    return_status = show_file_progress (0, file_size);
    mc_refresh ();
    if (return_status != FILE_CONT)
	goto ret;

    if (!do_remote_copy){
        for (;;){
    retry_src_read:
	    n_read = mc_read (source_desc, buf, buf_size);
	    if (n_read < 0){
	        return_status = file_error
		    (_(" Cannot read source file \"%s\" \n %s "), src_path);
	        if (return_status == FILE_RETRY)
		    goto retry_src_read;
	        goto ret;
	    }
	    if (n_read == 0)
	        break;

	    n_read_total += n_read;

    retry_dst_write:
	    n_written = mc_write (dest_desc, buf, n_read);
	    if (n_written < n_read){
	        return_status = file_error
		    (_(" Cannot write target file \"%s\" \n %s "), dst_path);
	        if (return_status == FILE_RETRY)
		    goto retry_dst_write;
	        goto ret;
	    }
	    return_status = show_file_progress (n_read_total, file_size);
	    mc_refresh ();
	    if (return_status != FILE_CONT)
	        goto ret;
        }
    } else {
	struct timeval tv_current;
	struct timeval tv_transfer_start;
	struct timeval tv_last_update;
	struct timeval tv_last_input;
        int    i, size, secs, update_secs;
	long   dt;
	char   *stalled_msg;
	
	gettimeofday (&tv_transfer_start, (struct timezone *) NULL);
	tv_last_update = tv_transfer_start;
	eta_secs = 0.0;
	
        for (i = 1; i;) {
            switch (size = mc_ctl (source_desc, MCCTL_REMOTECOPYCHUNK, 8192)) {
                case MCERR_TARGETOPEN:
		    message_1s (1, MSG_ERROR, _(" Can't open target file "));
		    goto ret;
                case MCERR_READ:
		    goto ret;
                case MCERR_WRITE:
		    message_1s (1, MSG_ERROR, _(" Can't write to local target file "));
		    goto ret;
	        case MCERR_DATA_ON_STDIN:
		    break;
                case MCERR_FINISH:
                    resources |= 8;  
                    i = 0;
                    break;
            }

            /* the first time we reach this line the target file has been created 
	       or truncated and we actually have a short target file.
	       Do we really want to delete the target file when the ftp transfer
	       fails? If we don't delete it we would be able to use reget later.
               (Norbert) */
	    resources |= 4; /* remove short file */

	    if (i && size != MCERR_DATA_ON_STDIN){
		n_read_total += size;

		/* Windows NT ftp servers report that files have no
		 * permissions: -------, so if we happen to have actually
		 * read something, we should fix the permissions.
		 */
		if (!(src_mode &
		      ((S_IRUSR|S_IWUSR|S_IXUSR)    /* user */
		       |(S_IXOTH|S_IWOTH|S_IROTH)  /* other */
		       |(S_IXGRP|S_IWGRP|S_IRGRP)))) /* group */
		    src_mode = S_IRUSR|S_IWUSR|S_IROTH|S_IRGRP;
		
		gettimeofday (&tv_last_input, NULL);
	    }
	    /* Timed operations: */
	    gettimeofday (&tv_current, NULL);

	    /* 1. Update rotating dash after some time (hardcoded to 2 seconds) */
	    secs = (tv_current.tv_sec - tv_last_update.tv_sec);
	    if (secs > 2){
		rotate_dash ();
		tv_last_update = tv_current;
	    }

	    /* 2. Check for a stalled condition */
	    update_secs = (tv_current.tv_sec - tv_last_input.tv_sec);
	    stalled_msg = "";
	    if (update_secs > 4){
		stalled_msg = _("(stalled)");
	    }

	    /* 3. Compute ETA */
	    if (secs > 2 || eta_secs == 0.0){
		dt = (tv_current.tv_sec - tv_transfer_start.tv_sec);
		
		if (n_read_total){
		    eta_secs = ((dt / (double) n_read_total) * file_size) - dt;
		    bps = n_read_total / ((dt < 1) ? 1 : dt);
		} else
		    eta_secs = 0.0;
	    }

	    /* 4. Compute BPS rate */
	    if (secs > 2){
		bps_time = (tv_current.tv_sec - tv_transfer_start.tv_sec);
		if (bps_time < 1)
		    bps_time = 1;
		bps = n_read_total / bps_time;
	    }
	    
	    label_set_text (stalled_label, stalled_msg);

	    return_status = show_file_progress (n_read_total, file_size);
	    mc_refresh ();
	    if (return_status != FILE_CONT)
	        goto ret;
        }
    }
    resources &= ~4; /* copy successful, don't remove target file */

ret:
    if (buf)
	free (buf);
	
 retry_src_close:
    if ((resources & 1) && mc_close (source_desc) < 0){
	temp_status = file_error
	    (_(" Cannot close source file \"%s\" \n %s "), src_path);
	if (temp_status == FILE_RETRY)
	    goto retry_src_close;
	if (temp_status == FILE_ABORT)
	    return_status = temp_status;
    }

 retry_dst_close:
    if ((resources & 2) && mc_close (dest_desc) < 0){
	temp_status = file_error
	    (_(" Cannot close target file \"%s\" \n %s "), dst_path);
	if (temp_status == FILE_RETRY)
	    goto retry_dst_close;
	return_status = temp_status;
    }
	
    if (resources & 4) {
        /* Remove short file */
	mc_unlink (dst_path);
	if (do_remote_copy) {
    	    mc_ctl (source_desc, MCCTL_FINISHREMOTE, -1);
	}
    } else if (resources & (2|8)) {
        /* no short file and destination file exists */
#ifndef OS2_NT 
	if (!appending && preserve_uidgid) {
         retry_dst_chown:
    	    if (mc_chown (dst_path, src_uid, src_gid)){
		temp_status = file_error
	    	    (_(" Cannot chown target file \"%s\" \n %s "), dst_path);
		if (temp_status == FILE_RETRY)
	    	    goto retry_dst_chown;
		return_status = temp_status;
    	    }
	}
#endif

     /* .ado: according to the XPG4 standard, the file must be closed before
      * chmod can be invoked
      */
     retry_dst_chmod:
	if (!appending && mc_chmod (dst_path, src_mode & umask_kill)){
	    temp_status = file_error
		(_(" Cannot chmod target file \"%s\" \n %s "), dst_path);
	    if (temp_status == FILE_RETRY)
		goto retry_dst_chmod;
	    return_status = temp_status;
	}
    
	if (!appending && preserve)
	    mc_utime (dst_path, &utb);
    }
    return return_status;
}

/*
 * I think these copy_*_* functions should have a return type.
 * anyway, this function *must* have two directories as arguments.
 */
/* FIXME: This function needs to check the return values of the
   function calls */
int
copy_dir_dir (char *s, char *d, int toplevel, int move_over, int delete,
              struct link *parent_dirs)
{
#ifdef __os2__
    DIR    *next;
#else
    struct dirent *next;
#endif
    struct stat   buf, cbuf;
    DIR    *reading;
    char   *path, *mdpath, *dest_file, *dest_dir;
    int    return_status = FILE_CONT;
    struct utimbuf utb;
    struct link *lp;

    /* First get the mode of the source dir */
 retry_src_stat:
    if ((*xstat) (s, &cbuf)){
	return_status = file_error (_(" Cannot stat source directory \"%s\" \n %s "), s);
	if (return_status == FILE_RETRY)
	    goto retry_src_stat;
	return return_status;
    }
    
    if (is_in_linklist (dest_dirs, s, &cbuf)) {
	/* Don't copy a directory we created before (we don't want to copy 
	   infinitely if a directory is copied into itself) */
	/* FIXME: should there be an error message and FILE_SKIP? - Norbert */
	return FILE_CONT;
    }

/* Hmm, hardlink to directory??? - Norbert */    
/* FIXME: In this step we should do something
   in case the destination already exist */    
    /* Check the hardlinks */
    if (preserve && cbuf.st_nlink > 1 && check_hardlinks (s, d, &cbuf) == 1) {
    	/* We have made a hardlink - no more processing is necessary */
    	return return_status;
    }

    if (!S_ISDIR (cbuf.st_mode)){
	return_status = file_error (_(" Source directory \"%s\" is not a directory \n %s "), s);
	if (return_status == FILE_RETRY)
	    goto retry_src_stat;
	return return_status;
    }
    
#ifndef OS2_NT 
    if (is_in_linklist (parent_dirs, s, &cbuf)) {
 	/* we found a cyclic symbolic link */
	   message_2s (1, MSG_ERROR, _(" Cannot copy cyclic symbolic link \n `%s' "), s);
	return FILE_SKIP;
    }
#endif    
    
    lp = xmalloc (sizeof (struct link), "parent_dirs");
    lp->vfs = vfs_type (s);
    lp->ino = cbuf.st_ino;
    lp->dev = cbuf.st_dev;
    lp->next = parent_dirs;
    parent_dirs = lp;

    /* Now, check if the dest dir exists, if not, create it. */
    if (mc_stat (d, &buf)){
    	/* Here the dir doesn't exist : make it !*/

    	if (move_over) {
            if (mc_rename (s, d) == 0) {
		free (parent_dirs);
		return FILE_CONT;
	    }
	}
	dest_dir = copy_strings (d, 0);
    } else {
        /*
         * If the destination directory exists, we want to copy the whole
         * directory, but we only want this to happen once.
	 *
	 * Escape sequences added to the * to avoid compiler warnings.
         * so, say /bla exists, if we copy /tmp/\* to /bla, we get /bla/tmp/\*
         * or ( /bla doesn't exist )       /tmp/\* to /bla     ->  /bla/\*
         */
#if 1
/* Again, I'm getting curious. Is not d already what we wanted, incl.
 *  masked source basename? Is not this just a relict of the past versions? 
 *  I'm afraid this will lead into a two level deep dive :(
 *
 * I think this is indeed the problem.  I can not remember any case where
 * we actually would like that behaviour -miguel
 *
 * It's a documented feature (option `Dive into subdir if exists' in the
 * copy/move dialog). -Norbert
 */
        if (toplevel && dive_into_subdirs){
	    dest_dir = concat_dir_and_file (d, x_basename (s));
	} else 
#endif
	{
	    dest_dir = copy_strings (d, 0);
	    goto dont_mkdir;
	}
    }
 retry_dst_mkdir:
    if (my_mkdir (dest_dir, (cbuf.st_mode & umask_kill) | S_IRWXU)){
	return_status = file_error (_(" Cannot create target directory \"%s\" \n %s "), dest_dir);
	if (return_status == FILE_RETRY)
	    goto retry_dst_mkdir;
	goto ret;
    }
    
    lp = xmalloc (sizeof (struct link), "dest_dirs");
    mc_stat (dest_dir, &buf);
    lp->vfs = vfs_type (dest_dir);
    lp->ino = buf.st_ino;
    lp->dev = buf.st_dev;
    lp->next = dest_dirs;
    dest_dirs = lp;
    
#ifndef OS2_NT 
    if (preserve_uidgid) {
     retry_dst_chown:
        if (mc_chown (dest_dir, cbuf.st_uid, cbuf.st_gid)){
	    return_status = file_error
	        (_(" Cannot chown target directory \"%s\" \n %s "), dest_dir);
	    if (return_status == FILE_RETRY)
	        goto retry_dst_chown;
	    goto ret;
        }
    }
#endif

 dont_mkdir:
    /* open the source dir for reading */
    if ((reading = mc_opendir (s)) == 0){
	goto ret;
    }
    
    while ((next = mc_readdir (reading)) && return_status != FILE_ABORT){
        /*
         * Now, we don't want '.' and '..' to be created / copied at any time 
         */
        if (!strcmp (next->d_name, "."))
            continue;
        if (!strcmp (next->d_name, ".."))
           continue;

        /* get the filename and add it to the src directory */
	path = concat_dir_and_file (s, next->d_name);
	
        (*xstat)(path, &buf);
        if (S_ISDIR (buf.st_mode)){
            mdpath = concat_dir_and_file (dest_dir, next->d_name);
            /*
             * From here, we just intend to recursively copy subdirs, not
             * the double functionality of copying different when the target
             * dir already exists. So, we give the recursive call the flag 0
             * meaning no toplevel.
             */
            return_status = copy_dir_dir (path, mdpath, 0, 0, delete, parent_dirs);
	    free (mdpath);
	} else {
	    dest_file = concat_dir_and_file (dest_dir, x_basename (path));
            return_status = copy_file_file (path, dest_file, 1);
	    free (dest_file);
	}    
	if (delete && return_status == FILE_CONT) {
	    if (erase_at_end) {
                static struct link *tail;
		lp = xmalloc (sizeof (struct link) + strlen (path), "erase_list");
		strcpy (lp->name, path);
		lp->st_mode = buf.st_mode;
		lp->next = 0;
                if (erase_list) {
                   tail->next = lp;
                   tail = lp;
                } else 
		   erase_list = tail = lp;
	    } else {
	        if (S_ISDIR (buf.st_mode)) {
		    return_status = erase_dir_iff_empty (path);
		} else
		    return_status = erase_file (path);
	    }
	}
	
#ifdef __os2__
	/* The OS/2 mc_readdir returns a block of memory DIR
	 * next should be freed: .ado
	 */
	if (!next)
		free (next);
#endif
        free (path);
    }
    mc_closedir (reading);
    
    /* .ado: Directories can not have permission set in OS/2 */
#ifndef __os2__
    if (preserve) {
	mc_chmod (dest_dir, cbuf.st_mode & umask_kill);
	utb.actime = cbuf.st_atime;
	utb.modtime = cbuf.st_mtime;
	mc_utime(dest_dir, &utb);
    }

#endif
ret:
    free (dest_dir);
    free (parent_dirs);
    return return_status;
}

/* }}} */

/* {{{ Move routines */

int
move_file_file (char *s, char *d)
{
    struct stat src_stats, dst_stats;
    int return_status = FILE_CONT;

    if (show_source (s) == FILE_ABORT
	|| show_target (d) == FILE_ABORT)
	return FILE_ABORT;

    mc_refresh ();

 retry_src_lstat:
    if (mc_lstat (s, &src_stats) != 0){
	/* Source doesn't exist */
	return_status = file_error (_(" Cannot stat file \"%s\" \n %s "), s);
	if (return_status == FILE_RETRY)
	    goto retry_src_lstat;
	return return_status;
    }

    if (mc_lstat (d, &dst_stats) == 0){
	/* Destination already exists */
	/* .ado: for OS/2 and NT, no st_ino exists */
#ifndef OS2_NT
	if (src_stats.st_dev == dst_stats.st_dev
	    && src_stats.st_ino == dst_stats.st_ino){
	    int msize = COLS - 36;
            char st[MC_MAXPATHLEN];
            char dt[MC_MAXPATHLEN];

	    if (msize < 0)
		msize = 40;
	    msize /= 2;

	    strcpy (st, name_trunc (s, msize));
	    strcpy (dt, name_trunc (d, msize));
	    message_3s (1, MSG_ERROR, _(" `%s' and `%s' are the same file "),
		     st, dt );
	    do_refresh ();
	    return FILE_SKIP;
	}
#endif /* OS2_NT */
	if (S_ISDIR (dst_stats.st_mode)){
	    message_2s (1, MSG_ERROR, _(" Cannot overwrite directory `%s' "), d);
	    do_refresh ();
	    return FILE_SKIP;
	}

	if (confirm_overwrite){
	    if (vfs_file_is_ftp (s))
		do_reget = -1;
	    else
		do_reget = 0;
	    
	    return_status = query_replace (d, &src_stats, &dst_stats);
	    if (return_status != FILE_CONT)
		return return_status;
	}
	/* Ok to overwrite */
    }
#if 0
 retry_rename: 
#endif
    if (!do_append) {
	if (S_ISLNK (src_stats.st_mode) && stable_symlinks) {
	    if ((return_status = make_symlink (s, d)) == FILE_CONT)
		goto retry_src_remove;
	    else
		return return_status;
	}

        if (mc_rename (s, d) == 0)
	    return FILE_CONT;
    }
#if 0
/* Comparison to EXDEV seems not to work in nfs if you're moving from
   one nfs to the same, but on the server it is on two different
   filesystems. Then nfs returns EIO instead of EXDEV. 
   Hope it will not hurt if we always in case of error try to copy/delete. */
     else
    	errno = EXDEV; /* Hack to copy (append) the file and then delete it */

    if (errno != EXDEV){
	return_status = files_error (_(" Cannot move file \"%s\" to \"%s\" \n %s "), s, d);
	if (return_status == FILE_RETRY)
	    goto retry_rename;
	return return_status;
    }
#endif    

    /* Failed because filesystem boundary -> copy the file instead */
    if ((return_status = copy_file_file (s, d, 0)) != FILE_CONT)
	return return_status;
    if ((return_status = show_source (NULL)) != FILE_CONT
	|| (return_status = show_file_progress (0, 0)) != FILE_CONT)
	return return_status;

    mc_refresh ();

 retry_src_remove:
    if (mc_unlink (s)){
	return_status = file_error (_(" Cannot remove file \"%s\" \n %s "), s);
	if (return_status == FILE_RETRY)
	    goto retry_src_remove;
	return return_status;
    }

    return FILE_CONT;
}

int
move_dir_dir (char *s, char *d)
{
    struct stat sbuf, dbuf, destbuf;
    struct link *lp;
    char *destdir;
    int return_status;
    int move_over = 0;

    if (show_source (s) == FILE_ABORT
	|| show_target (d) == FILE_ABORT)
	return FILE_ABORT;

    mc_refresh ();

    mc_stat (s, &sbuf);
    if (mc_stat (d, &dbuf))
	destdir = copy_strings (d, 0);  /* destination doesn't exist */
    else if (!dive_into_subdirs) {
	destdir = copy_strings (d, 0);
	move_over = 1;
    } else
	destdir = concat_dir_and_file (d, x_basename (s));

    /* Check if the user inputted an existing dir */
 retry_dst_stat:
    if (!mc_stat (destdir, &destbuf)){
	if (move_over) {
	    if ((return_status = copy_dir_dir (s, destdir, 0, 1, 1, 0)) != FILE_CONT)
		goto ret;
	    goto oktoret;
	} else {
	    if (S_ISDIR (destbuf.st_mode))
	        return_status = file_error (_(" Cannot overwrite directory \"%s\" %s "), destdir);
	    else
	        return_status = file_error (_(" Cannot overwrite file \"%s\" %s "), destdir);
	    if (return_status == FILE_RETRY)
	        goto retry_dst_stat;
	}
        free (destdir);
        return return_status;
    }
    
 retry_rename:
    if (mc_rename (s, destdir) == 0){
	return_status = FILE_CONT;
	goto ret;
    }
/* .ado: Drive, Do we need this anymore? */
#ifdef WIN32
        else {
        /* EXDEV: cross device; does not work everywhere */
                if (toupper(s[0]) != toupper(destdir[0]))
			goto w32try;
        }
#endif

    if (errno != EXDEV){
	return_status = files_error (_(" Cannot move directory \"%s\" to \"%s\" \n %s "), s, d);
	if (return_status == FILE_RETRY)
	    goto retry_rename;
	goto ret;
    }

w32try:
    /* Failed because of filesystem boundary -> copy dir instead */
    if ((return_status = copy_dir_dir (s, destdir, 0, 0, 1, 0)) != FILE_CONT)
	goto ret;
oktoret:
    if ((return_status = show_source (NULL)) != FILE_CONT
	|| (return_status = show_file_progress (0, 0)) != FILE_CONT)
	goto ret;

    mc_refresh ();
    if (erase_at_end) {
	for ( ; erase_list && return_status != FILE_ABORT; ) {
    	    if (S_ISDIR (erase_list->st_mode)) {
		return_status = erase_dir_iff_empty (erase_list->name);
	    } else
		return_status = erase_file (erase_list->name);
	    lp = erase_list;
	    erase_list = erase_list->next;
	    free (lp);
	}
    }
    erase_dir_iff_empty (s);

 ret:
    free (destdir);
    for ( ; erase_list; ) {
        lp = erase_list;
	erase_list = erase_list->next;
	free (lp);
    }
    return return_status;
}

/* }}} */

/* {{{ Erase routines */

static int
erase_file (char *s)
{
    int return_status;

    if (show_deleting (s) == FILE_ABORT)
	return FILE_ABORT;

    mc_refresh ();

 retry_unlink:
    if (mc_unlink (s)){
	return_status = file_error (_(" Cannot delete file \"%s\" \n %s "), s);
	if (return_status == FILE_RETRY)
	    goto retry_unlink;
	return return_status;
    }
    return FILE_CONT;
}

static int
recursive_erase (char *s)
{
    struct dirent *next;
    struct stat	buf;
    DIR    *reading;
    char   *path;
    int    return_status = FILE_CONT;

    if (!strcmp (s, ".."))
	return 1;
    
    reading = mc_opendir (s);
    
    if (!reading)
	return 1;
	
    while ((next = mc_readdir (reading)) && return_status == FILE_CONT){
	if (!strcmp (next->d_name, "."))
	    continue;
   	if (!strcmp (next->d_name, ".."))
	    continue;
	path = concat_dir_and_file (s, next->d_name);
   	if (mc_lstat (path, &buf)){
	    free (path);
	    return 1;
	} 
	if (S_ISDIR (buf.st_mode))
	    return_status = (recursive_erase (path) != FILE_CONT);
	else
	    return_status = erase_file (path);
	free (path);
	/* .ado: OS/2 returns a block of memory DIR to next and must be freed */
#ifdef __os2__
	if (!next)
	    free (next);
#endif
    }
    mc_closedir (reading);
    if (return_status != FILE_CONT)
	return return_status;
    if (show_deleting (s) == FILE_ABORT)
	return FILE_ABORT;
    mc_refresh ();
 retry_rmdir:
    if (my_rmdir (s)){
	return_status = file_error (_(" Cannot remove directory \"%s\" \n %s "), s);
	if (return_status == FILE_RETRY)
	    goto retry_rmdir;
	return return_status;
    }
    return FILE_CONT;
}

/* Return -1 on error, 1 if there are no entries besides "." and ".." 
   in the directory path points to, 0 else. */
static int 
check_dir_is_empty(char *path)
{
    DIR *dir;
    struct dirent *d;
    int i;

    dir = mc_opendir (path);
    if (!dir)
	return -1;

    for (i = 1, d = mc_readdir (dir); d; d = mc_readdir (dir)) {
	if (d->d_name[0] == '.' && (d->d_name[1] == '\0' ||
            (d->d_name[1] == '.' && d->d_name[2] == '\0')))
	    continue; /* "." or ".." */
	i = 0;
	break;
    }

    mc_closedir (dir);
    return i;
}

int
erase_dir (char *s)
{
    int error;

    if (strcmp (s, "..") == 0)
	return FILE_SKIP;

    if (strcmp (s, ".") == 0)
	return FILE_SKIP;

    if (show_deleting (s) == FILE_ABORT)
	return FILE_ABORT;
    mc_refresh ();

    /* The old way to detect a non empty directory was:
            error = my_rmdir (s);
            if (error && (errno == ENOTEMPTY || errno == EEXIST))) {
       For the linux user space nfs server (nfs-server-2.2beta29-2)
       we would have to check also for EIO. I hope the new way is
       fool proof. (Norbert)
     */
    error = check_dir_is_empty (s);
    if (error == 0) { /* not empty */
	error = query_recursive (s);
	if (error == FILE_CONT)
	    return recursive_erase (s);
	else
	    return error;
    }

 retry_rmdir:
    error = my_rmdir (s);
    if (error == -1){
	error = file_error (_(" Cannot remove directory \"%s\" \n %s "), s);
	if (error == FILE_RETRY)
	    goto retry_rmdir;
	return error;
    }
    return FILE_CONT;
}

int
erase_dir_iff_empty (char *s)
{
    int error;

    if (strcmp (s, "..") == 0)
	return FILE_SKIP;

    if (strcmp (s, ".") == 0)
	return FILE_SKIP;

    if (show_deleting (s) == FILE_ABORT)
	return FILE_ABORT;
    mc_refresh ();

    if (1 != check_dir_is_empty (s)) /* not empty or error */
	return FILE_CONT;

 retry_rmdir:
    error = my_rmdir (s);
    if (error) {
	error = file_error (_(" Cannot remove directory \"%s\" \n %s "), s);
	if (error == FILE_RETRY)
	    goto retry_rmdir;
	return error;
    }
    return FILE_CONT;
}

/* }}} */

/* {{{ Panel operate routines */

/* Returns currently selected file or the first marked file if there is one */
static char *
get_file (WPanel *panel, struct stat *stat_buf)
{
    int i;

    /* No problem with Gnome, as get_current_type never returns view_tree there */
    if (get_current_type () == view_tree){
	WTree *tree = (WTree *)get_panel_widget (get_current_index ());
	
	mc_stat (tree->selected_ptr->name, stat_buf);
	return tree->selected_ptr->name;
    } 

    if (panel->marked){
	for (i = 0; i < panel->count; i++)
	    if (panel->dir.list [i].f.marked){
		*stat_buf = panel->dir.list [i].buf;
		return panel->dir.list [i].fname;
	    }
    } else {
	*stat_buf = panel->dir.list [panel->selected].buf;
	return panel->dir.list [panel->selected].fname;
    }
    fprintf (stderr, _(" Internal error: get_file \n"));
    mi_getch ();
    return "";
}

static int
is_wildcarded (char *p)
{
    for (; *p; p++) {
        if (*p == '*')
            return 1;
        else if (*p == '\\' && p [1] >= '1' && p [1] <= '9')
            return 1;
    }
    return 0;
}

/* Sets all global variables used by copy_file_file/move_file_file to a 
   resonable default
   (file_mask_dialog sets these global variables interactively)
 */
void
file_mask_defaults (void)
{
    stable_symlinks = 0;
    op_follow_links = 0;
    dive_into_subdirs = 0;
    xstat = mc_lstat;
    
    preserve = 1;
    umask_kill = 0777777;
    preserve_uidgid = (geteuid () == 0) ? 1 : 0;
}

#define FMDY 13
#define	FMD_XLEN 64
static int fmd_xlen = FMD_XLEN, fmd_i18n_flag = 0;
static QuickWidget fmd_widgets [] = {

#define	FMCB0  FMDC
#define	FMCB12 0
#define	FMCB11 1
	/* follow symlinks and preserve Attributes must be the first */
    { quick_checkbox, 3, 64, 8, FMDY, N_("preserve &Attributes"), 9, 0,
      &op_preserve, 0, XV_WLAY_BELOWCLOSE, "preserve" },
    { quick_checkbox, 3, 64, 7, FMDY, N_("follow &Links"), 7, 0, 
      &op_follow_links, 0, XV_WLAY_BELOWCLOSE, "follow" },
#ifdef HAVE_XVIEW
#define FMDI1 5
#define FMDI2 2
#define FMDC  4
    { quick_input, 3, 64, 6, FMDY, "", 58, 0, 
      0, 0, XV_WLAY_BELOWCLOSE, "input2" },
#endif
    { quick_label, 3, 64, 5, FMDY, N_("to:"), 0, 0, 0, 0, XV_WLAY_BELOWOF,"to"},
    { quick_checkbox, 37, 64, 4, FMDY, N_("&Using shell patterns"), 0, 0, 
      0/* &source_easy_patterns */, 0, XV_WLAY_BELOWCLOSE, "using-shell" },
    { quick_input, 3, 64, 3, FMDY, "", 58, 
      0, 0, 0, XV_WLAY_BELOWCLOSE, "input-def" },
#ifndef HAVE_XVIEW      
#define FMDI1 4
#define FMDI2 5
#define FMDC 3
    { quick_input, 3, 64, 6, FMDY, "", 58, 0, 
      0, 0, XV_WLAY_BELOWCLOSE, "input2" },
#endif      
#define FMDI0 6	  
    { quick_label, 3, 64, 2, FMDY, "", 0, 0, 0, 0, XV_WLAY_DONTCARE, "ql" },
#define	FMBRGT 7
    { quick_button, 42, 64, 9, FMDY, N_("&Cancel"), 0, B_CANCEL, 0, 0, XV_WLAY_DONTCARE,
	  "cancel" },
#undef SKIP
#ifdef WITH_BACKGROUND
# define SKIP 5
# define FMCB21 11
# define FMCB22 10
# define FMBLFT 9
# define FMBMID 8
    { quick_button, 25, 64, 9, FMDY, N_("&Background"), 0, B_USER, 0, 0, XV_WLAY_DONTCARE, "back" },
#else /* WITH_BACKGROUND */
# define SKIP 4
# define FMCB21 10
# define FMCB22 9
# define FMBLFT 8
# undef  FMBMID
#endif
    { quick_button, 14, 64, 9, FMDY, N_("&Ok"), 0, B_ENTER, 0, 0, XV_WLAY_NEXTROW, "ok" },
    { quick_checkbox, 42, 64, 8, FMDY, N_("&Stable Symlinks"), 0, 0,
      &stable_symlinks, 0, XV_WLAY_BELOWCLOSE, "stab-sym" },
    { quick_checkbox, 31, 64, 7, FMDY, N_("&Dive into subdir if exists"), 0, 0, 
      &dive_into_subdirs, 0, XV_WLAY_BELOWOF, "dive" },
    { 0 } };

void
fmd_init_i18n()
{
#ifdef ENABLE_NLS

	register int i;
	int len;

	for (i = sizeof (op_names) / sizeof (op_names[0]); i--;)
		op_names [i] = _(op_names [i]);

	i = sizeof (fmd_widgets) / sizeof (fmd_widgets [0]) - 1;
	while (i--)
		if (fmd_widgets [i].text[0] != '\0')
			fmd_widgets [i].text = _(fmd_widgets [i].text);

	len = strlen (fmd_widgets [FMCB11].text)
		+ strlen (fmd_widgets [FMCB21].text) + 15;
	fmd_xlen = max (fmd_xlen, len);

	len = strlen (fmd_widgets [FMCB12].text)
		+ strlen (fmd_widgets [FMCB22].text) + 15;
	fmd_xlen = max (fmd_xlen, len);
		
	len = strlen (fmd_widgets [FMBRGT].text)
		+ strlen (fmd_widgets [FMBLFT].text) + 11;

#ifdef FMBMID
	len += strlen (fmd_widgets [FMBMID].text) + 6;
#endif

	fmd_xlen = max (fmd_xlen, len + 4);

	len = (fmd_xlen - (len + 6)) / 2;
	i = fmd_widgets [FMBLFT].relative_x = len + 3;
	i += strlen (fmd_widgets [FMBLFT].text) + 8;

#ifdef FMBMID
	fmd_widgets [FMBMID].relative_x = i;
	i += strlen (fmd_widgets [FMBMID].text) + 6;
#endif

	fmd_widgets [FMBRGT].relative_x = i;

#define	chkbox_xpos(i) \
	fmd_widgets [i].relative_x = fmd_xlen - strlen (fmd_widgets [i].text) - 6

	chkbox_xpos(FMCB0);
	chkbox_xpos(FMCB21);
	chkbox_xpos(FMCB22);

	if (fmd_xlen != FMD_XLEN)
	{
		i = sizeof (fmd_widgets) / sizeof (fmd_widgets [0]) - 1;
		while (i--)
			fmd_widgets [i].x_divisions = fmd_xlen;

		fmd_widgets [FMDI1].hotkey_pos =
			fmd_widgets [FMDI2].hotkey_pos = fmd_xlen - 6;
	}
#undef chkbox_xpos
#endif /* ENABLE_NLS */

	fmd_i18n_flag = 1;
}

char *
file_mask_dialog (int operation, char *text, char *def_text, int only_one, int *do_background)
{
    int source_easy_patterns = easy_patterns;
    char *source_mask, *orig_mask, *dest_dir;
    const char *error;
    struct stat buf;
    int val;
    
    QuickDialog Quick_input;

	if (!fmd_i18n_flag)
		fmd_init_i18n();

    stable_symlinks = 0;
    fmd_widgets [FMDC].result = &source_easy_patterns;
    fmd_widgets [FMDI1].text = easy_patterns ? "*" : "^\\(.*\\)$";
    Quick_input.xlen  = fmd_xlen;
    Quick_input.xpos  = -1;
    Quick_input.title = op_names [operation];
    Quick_input.help  = "[Mask Copy/Rename]";
    Quick_input.ylen  = FMDY;
    Quick_input.i18n  = 1;
    
    if (operation == OP_COPY) {
	Quick_input.class = "quick_file_mask_copy";
	Quick_input.widgets = fmd_widgets;
    } else { /* operation == OP_MOVE */
	Quick_input.class = "quick_file_mask_move";
	Quick_input.widgets = fmd_widgets + 2;
    }
    fmd_widgets [FMDI0].text = text;
    fmd_widgets [FMDI2].text = def_text;
    fmd_widgets [FMDI2].str_result = &dest_dir;
    fmd_widgets [FMDI1].str_result = &source_mask;

    *do_background = 0;
ask_file_mask:

    if ((val = quick_dialog_skip (&Quick_input, SKIP)) == B_CANCEL)
	return 0;

    if (op_follow_links && operation != OP_MOVE)
	xstat = mc_stat;
    else
	xstat = mc_lstat;
    
    if (op_preserve || operation == OP_MOVE) {
	preserve = 1;
	umask_kill = 0777777;
	preserve_uidgid = (geteuid () == 0) ? 1 : 0;
    }
    else {
        int i;
	preserve = preserve_uidgid = 0;
	i = umask (0);
	umask (i);
	umask_kill = i ^ 0777777;
    }

    orig_mask = source_mask;
    if (!dest_dir || !*dest_dir) {
	if (source_mask)
	    free (source_mask);
	return dest_dir;
    }
    if (source_easy_patterns) {
	source_easy_patterns = easy_patterns;
	easy_patterns = 1;
	source_mask = convert_pattern (source_mask, match_file, 1);
	easy_patterns = source_easy_patterns;
        error = re_compile_pattern (source_mask, strlen (source_mask), &rx);
        free (source_mask);
    } else
        error = re_compile_pattern (source_mask, strlen (source_mask), &rx);

    if (error) {
	message_3s (1, MSG_ERROR, _("Invalid source pattern `%s' \n %s "),
		 orig_mask, error);
	if (orig_mask)
	    free (orig_mask);
	goto ask_file_mask;
    }
    if (orig_mask)
	free (orig_mask);
    dest_mask = strrchr (dest_dir, PATH_SEP);
    if (dest_mask == NULL)
	dest_mask = dest_dir;
    else
	dest_mask++;
    orig_mask = dest_mask;
    if (!*dest_mask || (!dive_into_subdirs && !is_wildcarded (dest_mask) &&
                        (!only_one || (!mc_stat (dest_dir, &buf) && S_ISDIR (buf.st_mode)))) ||
	(dive_into_subdirs && ((!only_one && !is_wildcarded (dest_mask)) ||
			       (only_one && !mc_stat (dest_dir, &buf) && S_ISDIR (buf.st_mode)))))
	dest_mask = strdup ("*");
    else {
	dest_mask = strdup (dest_mask);
	*orig_mask = 0;
    }
    if (!*dest_dir) {
	free (dest_dir);
	dest_dir = strdup ("./");
    }
    if (val == B_USER)
	*do_background = 1;
    return dest_dir;
}

/*
 * This array introduced to avoid translation problems. The former (op_names)
 * is assumed to be nouns, suitable in dialog box titles; this one should
 * contain whatever is used in prompt itself (i.e. in russian, it's verb).
 * Notice first symbol - it is to fool gettext and force these strings to
 * be different for it. First symbol is skipped while building a prompt.
 * (I don't use spaces around the words, because someday they could be
 * dropped, when widgets get smarter)
 */
static char *op_names1 [] = { N_("1Copy"), N_("1Move"), N_("1Delete") };

/*
 * These are formats for building a prompt. Parts encoded as follows:
 * %o - operation from op_names1
 * %f - file/files or files/directories, as appropriate
 * %m - "with source mask" or question mark for delete
 * %s - source name (truncated)
 * %d - number of marked files
 */
static char* one_format  = N_("%o %f \"%s\"%m");
static char* many_format = N_("%o %d %f%m");

static char* prompt_parts [] =
{
	N_("file"), N_("files"), N_("directory"), N_("directories"),
	N_("files/directories"), N_(" with source mask:")
};

static char*
generate_prompt(char* cmd_buf, WPanel* panel, int operation, int only_one,
	struct stat* src_stat)
{
	register char *sp, *cp;
	register int i;
	char format_string [200];
	char *dp = format_string;
	char* source = NULL;

#ifdef ENABLE_NLS
	static int i18n_flag = 0;
	if (!i18n_flag)
	{
		if (!fmd_i18n_flag)
			fmd_init_i18n(); /* to get proper fmd_xlen */

		for (i = sizeof (op_names1) / sizeof (op_names1 [0]); i--;)
			op_names1 [i] = _(op_names1 [i]);

		for (i = sizeof (prompt_parts) / sizeof (prompt_parts [0]); i--;)
			prompt_parts [i] = _(prompt_parts [i]);

		one_format = _(one_format);
		many_format = _(many_format);
		i18n_flag = 1;
	}
#endif /* ENABLE_NLS */

	sp = only_one ? one_format : many_format;

	if (only_one)
		source = get_file (panel, src_stat);

	while (*sp)
	{
		switch (*sp)
		{
			case '%':
				cp = NULL;
				switch (sp[1])
				{
					case 'o':
						cp = op_names1 [operation] + 1;
						break;
					case 'm':
						cp = operation == OP_DELETE ? "?" : prompt_parts [5];
						break;
					case 'f':
						if (only_one)
						{
							cp = S_ISDIR (src_stat->st_mode) ? 
								prompt_parts [2] : prompt_parts [0];
						}
						else
						{
							cp = (panel->marked == panel->dirs_marked) 
							? prompt_parts [3] 
							: (panel->dirs_marked ? prompt_parts [4] 
							: prompt_parts [1]);
						}
						break;
					default:
						*dp++ = *sp++;
				}
				if (cp)
				{
					sp += 2;
					while (*cp)
						*dp++ = *cp++;
				}
				break;
			default:
				*dp++ = *sp++;
		}
	}
	*dp = '\0';

	if (only_one)
	{
		i = fmd_xlen - strlen(format_string) - 4;
		sprintf (cmd_buf, format_string, name_trunc (source, i));
	}
	else
	{
		sprintf (cmd_buf, format_string, panel->marked);
		i = strlen (cmd_buf) + 6 - fmd_xlen;
		if (i > 0)
		{
			fmd_xlen += i;
			fmd_init_i18n(); /* to recalculate positions of child widgets */
		}
	}

	return source;
}


/* Returns 1 if did change the directory structure,
   Returns 0 if user aborted */
int
panel_operate (void *source_panel, int operation, char *thedefault)
{
    WPanel *panel = source_panel;
#ifdef WITH_FULL_PATHS
    char *source_with_path = NULL;
#else
#   define source_with_path source
#endif
    char *source = NULL;
    char *dest = NULL;
    char *temp = NULL;
    int only_one = (get_current_type () == view_tree) || (panel->marked <= 1);
    struct stat src_stat, dst_stat;
    int i, value;
    long marked, total;
    long count = 0, bytes = 0;
    int  dst_result;
    int  do_bg;			/* do background operation? */

    do_bg = 0;
    rx.buffer = NULL;
    free_linklist (&linklist);
    free_linklist (&dest_dirs);
    if (get_current_type () == view_listing)
	if (!panel->marked && !strcmp (selection (panel)->fname, "..")){
	    message (1, MSG_ERROR, _(" Can't operate on \"..\"! "));
	    return 0;
	}
    
    if (operation < OP_COPY || operation > OP_DELETE)
	return 0;
    
    /* Generate confirmation prompt */
    source = generate_prompt(cmd_buf, panel, operation, only_one, &src_stat);   
    
    /* Show confirmation dialog */
    if (operation == OP_DELETE && confirm_delete){
        if (know_not_what_am_i_doing)
	    query_set_sel (1);
	i = query_dialog (_(op_names [operation]), cmd_buf,
			  D_ERROR, 2, _("&Yes"), _("&No"));
	if (i != 0)
	    return 0;
    } else if (operation != OP_DELETE) {
	char *dest_dir;
	
	if (thedefault != NULL)
	    dest_dir = thedefault;
	else if (get_other_type () == view_listing)
	    dest_dir = opanel->cwd;
	else
	    dest_dir = panel->cwd;

	rx.buffer = (char *) xmalloc (MC_MAXPATHLEN, "mask copying");
	rx.allocated = MC_MAXPATHLEN;
	rx.translate = 0;
	dest = file_mask_dialog (operation, cmd_buf, dest_dir, only_one, &do_bg);
	if (!dest) {
	    free (rx.buffer);
	    return 0;
	}
	if (!*dest){
	    free (rx.buffer);
	    free (dest);
	    return 0;
	}
    }

#ifdef WITH_BACKGROUND
    /* Did the user select to do a background operation? */
    if (do_bg){
	int v;
	
	v = do_background (copy_strings (operation_names [operation], ": ", panel->cwd, 0));
	if (v == -1){
	    message (1, MSG_ERROR, _(" Sorry, I could not put the job in background "));
	}

	/* If we are the parent */
	if (v == 1){
	    vfs_force_expire (panel->cwd);
	    vfs_force_expire (dest);
	    return 0;
	}
    }
#endif
    /* Initialize things */
    /* We turn on ETA display if the source is an ftp file system */
    create_op_win (operation, vfs_file_is_ftp (panel->cwd));
    ftpfs_hint_reread (0);
    
    /* Now, let's do the job */
    /* This code is only called by the tree and panel code */
    if (only_one){
	/* One file: FIXME mc_chdir will take user out of any vfs */
	if (operation != OP_COPY && get_current_type () == view_tree)
	    mc_chdir (PATH_SEP_STR);
	
	/* The source and src_stat variables have been initialized before */
#ifdef WITH_FULL_PATHS
	source_with_path = concat_dir_and_file (panel->cwd, source);
#endif
	
	if (operation == OP_DELETE){
	    /* Delete operation */
	    if (S_ISDIR (src_stat.st_mode))
		value = erase_dir (source_with_path);
	    else
		value = erase_file (source_with_path);
	} else {
	    /* Copy or move operation */
	    temp = transform_source (source_with_path);
	    if (temp == NULL) {
		value = transform_error;
	    } else {
		temp = get_full_name (dest, temp);
		free (dest);
		dest = temp;
		temp = 0;

	        switch (operation){
	        case OP_COPY:
		    /* we use op_follow_links only with OP_COPY,
		      */
		    (*xstat) (source_with_path, &src_stat);
		    if (S_ISDIR (src_stat.st_mode))
		        value = copy_dir_dir (source_with_path, dest, 1, 0, 0, 0);
		    else
		        value = copy_file_file (source_with_path, dest, 1);
		    break;
	        case OP_MOVE:
		    if (S_ISDIR (src_stat.st_mode))
		        value = move_dir_dir (source_with_path, dest);
		    else
		        value = move_file_file (source_with_path, dest);
		    break;
	        default:
		    value = FILE_CONT;
		    message_1s (1, _(" Internal failure "), _(" Unknown file operation "));
	        }
	    }
	} /* Copy or move operation */

	if (value == FILE_CONT)
	    unmark_files (panel);

    } else {
	/* Many files */

	if (operation != OP_DELETE){
	    /* Check destination for copy or move operation */
	retry_many_dst_stat:
	    dst_result = mc_stat (dest, &dst_stat);
	    if (dst_result == 0 && !S_ISDIR (dst_stat.st_mode)){
		if (file_error (_(" Destination \"%s\" must be a directory \n %s "), dest) == FILE_RETRY)
		    goto retry_many_dst_stat;
		goto clean_up;
	    }
	}

	/* Initialize variables for progress bars */
	marked = panel->marked;
	total = panel->total;

	/* Loop for every file */
	for (i = 0; i < panel->count; i++){
	    if (!panel->dir.list [i].f.marked)
		continue;	/* Skip the unmarked ones */
	    source = panel->dir.list [i].fname;
	    src_stat = panel->dir.list [i].buf;	/* Inefficient, should we use pointers? */
#ifdef WITH_FULL_PATHS
	    if (source_with_path)
	    	free (source_with_path);
	    source_with_path = concat_dir_and_file (panel->cwd, source);
#endif
	    if (operation == OP_DELETE){
		/* Delete operation */
		if (S_ISDIR (src_stat.st_mode))
		    value = erase_dir (source_with_path);
		else
		    value = erase_file (source_with_path);
	    } else {
		/* Copy or move operation */
		if (temp)
		    free (temp);
		temp = transform_source (source_with_path);
		if (temp == NULL) {
		    value = transform_error;
		} else {
		    temp = get_full_name (dest, temp);
		    switch (operation){
		    case OP_COPY:
		       /* we use op_follow_links only with OP_COPY,
		        */
			(*xstat) (source_with_path, &src_stat);
		    	if (S_ISDIR (src_stat.st_mode))
			    value = copy_dir_dir (source_with_path, temp, 1, 0, 0, 0);
		    	else
			    value = copy_file_file (source_with_path, temp, 1);
			free_linklist (&dest_dirs);
		    	break;
		    case OP_MOVE:
		    	if (S_ISDIR (src_stat.st_mode))
			    value = move_dir_dir (source_with_path, temp);
		    	else
		            value = move_file_file (source_with_path, temp);
		        break;
		    default:
		    	message_1s (1, _(" Internal failure "),
			         _(" Unknown file operation "));
		    	goto clean_up;
		    }
		}
	    } /* Copy or move operation */

	    if (value == FILE_ABORT)
		goto clean_up;
	    if (value == FILE_CONT){
		do_file_mark (panel, i, 0);
	    }
	    count ++;
	    if (show_count_progress (count, marked) == FILE_ABORT)
		goto clean_up;
	    bytes += src_stat.st_size;
	    if (verbose &&
	        show_bytes_progress (bytes, total) == FILE_ABORT)
		goto clean_up;
	    if (operation != OP_DELETE && verbose
		&& show_file_progress (0, 0) == FILE_ABORT)
		goto clean_up;
	    mc_refresh ();
	} /* Loop for every file */
    } /* Many files */

 clean_up:
    /* Clean up */
    destroy_op_win ();
    ftpfs_hint_reread (1);
    free_linklist (&linklist);
    free_linklist (&dest_dirs);
#if WITH_FULL_PATHS
    if (source_with_path)
    	free (source_with_path);
#endif
    if (dest)
	free (dest);
    if (temp)
	free (temp);
    if (rx.buffer) {
	free (rx.buffer);
	rx.buffer = NULL;
    }
    if (dest_mask) {
	free (dest_mask);
	dest_mask = NULL;
    }

#ifdef WITH_BACKGROUND
    /* Let our parent know we are saying bye bye */
    if (we_are_background){
	vfs_shut ();
	tell_parent (MSG_CHILD_EXITING);
	exit (1);
    } 
#endif
    return 1;
}

/* }}} */

/* {{{ Query/status report routines */

static int
real_do_file_error (enum OperationMode mode, char *error)
{
    int result;
    char *msg;

    msg = mode == Foreground ? MSG_ERROR : _(" Background process error ");
    result = query_dialog (msg, error, D_ERROR, 3, _("&Skip"), _("&Retry"), _("&Abort"));

    switch (result){
    case 0:
	do_refresh ();
	return FILE_SKIP;
    case 1:
	do_refresh ();
	return FILE_RETRY;
    case 2:
    default:
	return FILE_ABORT;
    }
}

/* Report error with one file */
int
file_error (char *format, char *file)
{
    sprintf (cmd_buf, format,
	     name_trunc (file, 30), unix_error_string (errno));
    return do_file_error (cmd_buf);
}

/* Report error with two files */
int
files_error (char *format, char *file1, char *file2)
{
    char nfile1 [16];
    char nfile2 [16];
    
    strcpy (nfile1, name_trunc (file1, 15));
    strcpy (nfile2, name_trunc (file2, 15));
    
    sprintf (cmd_buf, format, nfile1, nfile2, unix_error_string (errno));
    return do_file_error (cmd_buf);
}

static char *format = N_("Target file \"%s\" already exists!");
static int
replace_callback (struct Dlg_head *h, int Id, int Msg)
{
#ifndef HAVE_X

    switch (Msg){
    case DLG_DRAW:
	dialog_repaint (h, ERROR_COLOR, ERROR_COLOR);
	break;
    }
#endif
    return 0;
}

#ifdef HAVE_X
#define X_TRUNC 128
#else
#define X_TRUNC 52
#endif

/*
 * FIXME: probably it is better to replace this with quick dialog machinery,
 * but actually I'm not familiar with it and have not much time :(
 *   alex
 */
static struct
{
	char* text;
	int   ypos, xpos;	
	int   value;		/* 0 for labels */
	char* tkname;
	WLay  layout;
}
rd_widgets [] =
{
	{N_("Target file \"%s\" already exists!"),
	                 3,      4,  0,              "target-e",   XV_WLAY_CENTERROW},
	{N_("&Abort"),   BY + 3, 25, REPLACE_ABORT,  "abort",      XV_WLAY_CENTERROW},
	{N_("if &Size differs"),
	                 BY + 1, 28, REPLACE_SIZE,   "if-size",    XV_WLAY_RIGHTOF},
	{N_("non&E"),    BY,     47, REPLACE_NEVER,  "none",       XV_WLAY_RIGHTOF},
	{N_("&Update"),  BY,     36, REPLACE_UPDATE, "update",     XV_WLAY_RIGHTOF},
	{N_("al&L"),     BY,     28, REPLACE_ALWAYS, "all",        XV_WLAY_RIGHTOF},
	{N_("Overwrite all targets?"),
	                 BY,     4,  0,              "over-label", XV_WLAY_CENTERROW},
	{N_("&Reget"),   BY - 1, 28, REPLACE_REGET,  "reget",      XV_WLAY_RIGHTOF},
	{N_("ap&Pend"),  BY - 2, 45, REPLACE_APPEND, "append",     XV_WLAY_RIGHTOF},
	{N_("&No"),      BY - 2, 37, REPLACE_NO,     "no",         XV_WLAY_RIGHTOF},
	{N_("&Yes"),     BY - 2, 28, REPLACE_YES,    "yes",        XV_WLAY_RIGHTOF},
	{N_("Overwrite this target?"),
	                 BY - 2, 4,  0,              "overlab",    XV_WLAY_CENTERROW},
	{N_("Target date: %s, size %d"),
	                 6,      4,  0,              "target-date",XV_WLAY_CENTERROW},
	{N_("Source date: %s, size %d"),
	                 5,      4,  0,              "source-date",XV_WLAY_CENTERROW}
}; 

#define ADD_RD_BUTTON(i)\
	add_widgetl (replace_dlg,\
		button_new (rd_widgets [i].ypos, rd_widgets [i].xpos, rd_widgets [i].value,\
		NORMAL_BUTTON, rd_widgets [i].text, 0, 0, rd_widgets [i].tkname), \
		rd_widgets [i].layout)

#define ADD_RD_LABEL(i,p1,p2)\
	sprintf (buffer, rd_widgets [i].text, p1, p2);\
	add_widgetl (replace_dlg,\
		label_new (rd_widgets [i].ypos, rd_widgets [i].xpos, buffer, rd_widgets [i].tkname),\
		rd_widgets [i].layout)

static void
init_replace (enum OperationMode mode)
{
    char buffer [128];
	static int rd_xlen = 60, rd_trunc = X_TRUNC;

#ifdef ENABLE_NLS
	static int i18n_flag;
	if (!i18n_flag)
	{
		int l1, l2, l, row;
		register int i = sizeof (rd_widgets) / sizeof (rd_widgets [0]); 
		while (i--)
			rd_widgets [i].text = _(rd_widgets [i].text);

		/* 
		 *longest of "Overwrite..." labels 
		 * (assume "Target date..." are short enough)
		 */
		l1 = max (strlen (rd_widgets [6].text), strlen (rd_widgets [11].text));

		/* longest of button rows */
		i = sizeof (rd_widgets) / sizeof (rd_widgets [0]);
		for (row = l = l2 = 0; i--;)
		{
			if (rd_widgets [i].value != 0)
			{
				if (row != rd_widgets [i].ypos)
				{
					row = rd_widgets [i].ypos;
					l2 = max (l2, l);
					l = 0;
				}
				l += strlen (rd_widgets [i].text) + 4;
			}
		}
		l2 = max (l2, l); /* last row */
		rd_xlen = max (rd_xlen, l1 + l2 + 8);
		rd_trunc = rd_xlen - 6;

		/* Now place buttons */
		l1 += 5; /* start of first button in the row */
		i = sizeof (rd_widgets) / sizeof (rd_widgets [0]);
		
		for (l = l1, row = 0; --i > 1;)
		{
			if (rd_widgets [i].value != 0)
			{
				if (row != rd_widgets [i].ypos)
				{
					row = rd_widgets [i].ypos;
					l = l1;
				}
				rd_widgets [i].xpos = l;
				l += strlen (rd_widgets [i].text) + 4;
			}
		}
		/* Abort button is centered */
		rd_widgets [1].xpos = (rd_xlen - strlen (rd_widgets [1].text) - 3) / 2;

	}
#endif /* ENABLE_NLS */

    replace_colors [0] = ERROR_COLOR;
    replace_colors [1] = COLOR_NORMAL;
    replace_colors [2] = ERROR_COLOR;
    replace_colors [3] = COLOR_NORMAL;
    
    replace_dlg = create_dlg (0, 0, 16, rd_xlen, replace_colors, replace_callback,
			      "[ Replace ]", "replace", DLG_CENTER);
    
    x_set_dialog_title (replace_dlg,
        mode == Foreground ? _(" File exists ") : _(" Background process: File exists "));


	ADD_RD_LABEL(0, name_trunc (replace_filename, rd_trunc - strlen (rd_widgets [0].text)), 0 );
	ADD_RD_BUTTON(1);    
    
    tk_new_frame (replace_dlg, "a.");

	ADD_RD_BUTTON(2);
	ADD_RD_BUTTON(3);
	ADD_RD_BUTTON(4);
	ADD_RD_BUTTON(5);
	ADD_RD_LABEL(6,0,0);

    /* "this target..." widgets */
    tk_new_frame (replace_dlg, "p.");
	if (!S_ISDIR (d_stat->st_mode)){
		if ((do_reget == -1 && d_stat->st_size && s_stat->st_size > d_stat->st_size))
			ADD_RD_BUTTON(7);

		ADD_RD_BUTTON(8);
    }
	ADD_RD_BUTTON(9);
	ADD_RD_BUTTON(10);
	ADD_RD_LABEL(11,0,0);
    
    tk_new_frame (replace_dlg, "i.");
	ADD_RD_LABEL(12, file_date (d_stat->st_mtime), (int) d_stat->st_size);
	ADD_RD_LABEL(13, file_date (s_stat->st_mtime), (int) s_stat->st_size);
    tk_end_frame ();
}

static int
real_query_replace (enum OperationMode mode, char *destname, struct stat *_s_stat,
		    struct stat *_d_stat)
{
    if (replace_result < REPLACE_ALWAYS){
	replace_filename = destname;
	s_stat = _s_stat;
	d_stat = _d_stat;
	init_replace (mode);
	run_dlg (replace_dlg);
	replace_result = replace_dlg->ret_value;
	if (replace_result == B_CANCEL)
	    replace_result = REPLACE_ABORT;
	destroy_dlg (replace_dlg);
    }

    switch (replace_result){
    case REPLACE_UPDATE:
	do_refresh ();
	if (_s_stat->st_mtime > _d_stat->st_mtime)
	    return FILE_CONT;
	else
	    return FILE_SKIP;

    case REPLACE_SIZE:
	do_refresh ();
	if (_s_stat->st_size == _d_stat->st_size)
	    return FILE_SKIP;
	else
	    return FILE_CONT;
	
    case REPLACE_REGET:
	do_reget = _d_stat->st_size;
	
    case REPLACE_APPEND:
        do_append = 1;
	
    case REPLACE_YES:
    case REPLACE_ALWAYS:
	do_refresh ();
	return FILE_CONT;
    case REPLACE_NO:
    case REPLACE_NEVER:
	do_refresh ();
	return FILE_SKIP;
    case REPLACE_ABORT:
    default:
	return FILE_ABORT;
    }
}

int
real_query_recursive (enum OperationMode mode, char *s)
{
    char *confirm, *text;

    if (recursive_result < RECURSIVE_ALWAYS){
	char *msg =
	    mode == Foreground ? _("\n   Directory not empty.   \n   Delete it recursively? ")
	                       : _("\n   Background process: Directory not empty \n   Delete it recursively? ");
	text = copy_strings (" Delete: ", name_trunc (s, 30), " ", 0);

        if (know_not_what_am_i_doing)
	    query_set_sel (1);
        recursive_result = query_dialog (text, msg, D_ERROR, 5,
				     _("&Yes"), _("&No"), _("a&ll"), _("non&E"), _("&Abort"));
	
	
	if (recursive_result != RECURSIVE_ABORT)
	    do_refresh ();
	free (text);
	if (know_not_what_am_i_doing && (recursive_result == RECURSIVE_YES
	    || recursive_result == RECURSIVE_ALWAYS)){
	    text = copy_strings (_(" Type 'yes' if you REALLY want to delete "),
				 recursive_result == RECURSIVE_YES
				 ? name_trunc (s, 19) : _("all the directories "), " ", 0);
	    confirm = input_dialog (mode == Foreground ? _(" Recursive Delete ")
				                       : _(" Background process: Recursive Delete "),
				    text, "no");
	    do_refresh ();
	    if (!confirm || strcmp (confirm, "yes"))
		recursive_result = RECURSIVE_NEVER;
	    free (confirm);
	    free (text);
	}
    }
    switch (recursive_result){
    case RECURSIVE_YES:
    case RECURSIVE_ALWAYS:
	return FILE_CONT;
    case RECURSIVE_NO:
    case RECURSIVE_NEVER:
	return FILE_SKIP;
    case RECURSIVE_ABORT:
    default:
	return FILE_ABORT;
    }
}

#ifdef WITH_BACKGROUND
int
do_file_error (char *str)
{
    return call_1s (real_do_file_error, str);
}

int
query_recursive (char *s)
{
    return call_1s (real_query_recursive, s);
}

int
query_replace (char *destname, struct stat *_s_stat, struct stat *_d_stat)
{
    if (we_are_background)
	return parent_call ((void *)real_query_replace, 3, strlen(destname), destname,
			    sizeof (struct stat), _s_stat, sizeof(struct stat), _d_stat);
    else
	return real_query_replace (Foreground, destname, _s_stat, _d_stat);
}

#else
do_file_error (char *str)
{
    return real_do_file_error (Foreground, str);
}

int
query_recursive (char *s)
{
    return real_query_recursive (Foreground, s);
}

int
query_replace (char *destname, struct stat *_s_stat, struct stat *_d_stat)
{
    return real_query_replace (Foreground, destname, _s_stat, _d_stat);
}

#endif

/*
  Cause emacs to enter folding mode for this file:
  Local variables:
  end:
*/

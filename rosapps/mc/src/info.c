/* Panel managing.
   Copyright (C) 1994, 1995 Janne Kukonlehto
   Copyright (C) 1995 Miguel de Icaza
   
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
#include <stdio.h>
#include "tty.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <malloc.h>
#include "mad.h"
#include "util.h"		/* statfs calls */
#include "mouse.h"		/* Gpm_Event */
#include "color.h"
#include "dlg.h"
#include "info.h"
#include "dir.h"		/* required by panel */
#include "panel.h"		/* for the panel structure */
#include "main.h"		/* opanel, cpanel definitions */
#include "win.h"		/* print_bytesize */
#include "layout.h"
#include "key.h"		/* is_idle() */

#ifdef HAVE_TK
#   include "tkmain.h"
#   include "tkinfo.h"
#endif

/* Have we called the init_my_statfs routine? */
static int initialized;
struct my_statfs myfs_stats;

static int info_event (Gpm_Event *event, WInfo *info)
{
    return 0;
}

static void info_box (Dlg_head *h, WInfo *info)
{
    standend ();
    if (hascolors)
	attrset (NORMAL_COLOR);
    widget_erase (&info->widget);
    draw_double_box (h, info->widget.y,  info->widget.x,
	             info->widget.lines, info->widget.cols);
}

#ifndef VERSION
#   define VERSION "undefined"
#endif

void info_show_info (WInfo *info)
{
    struct stat buf;

#ifndef HAVE_X
    if (!is_idle ())
	return;

    info_box (info->widget.parent, info);
    attrset (MARKED_COLOR);
    widget_move (&info->widget, 1, 3);
    printw (_("Midnight Commander %s"), VERSION);
    attrset (NORMAL_COLOR);
    widget_move (&info->widget, 2, 1);
    /* .ado: info->widget.x has wrong value (==0) on NT and OS/2, why? */
#if !defined(OS2_NT)
    hline (ACS_HLINE|NORMAL_COLOR, info->widget.x-2);
#endif
#endif
    if (get_current_type () != view_listing)
	return;

    if (!info->ready)
	return;
    
    my_statfs (&myfs_stats, cpanel->cwd);
    buf = cpanel->dir.list [cpanel->selected].buf;
#ifdef OS2_NT
    /* .ado: for OS/2 and NT, st_dev must > 0 */
    if ((signed char) buf.st_dev < 0)
	    return;
#endif
    
#ifndef HAVE_X
    /* Print only lines which fit */
    switch (info->widget.y-2){
	/* Note: all cases are fall-throughs */
	
    default:

    case 16:
	widget_move (&info->widget, 16, 3);
	if (myfs_stats.nfree >0 || myfs_stats.nodes > 0)
	    printw (_("Free nodes %d (%d%%) of %d"),
		    myfs_stats.nfree,
		    myfs_stats.total
		    ? 100 * myfs_stats.nfree / myfs_stats.nodes : 0,
		    myfs_stats.nodes);
	else
	    addstr (_("No node information"));
	
    case 15:
	widget_move (&info->widget, 15, 3);
	if (myfs_stats.avail > 0 || myfs_stats.total > 0){
	    addstr (_("Free space "));
	    print_bytesize (myfs_stats.avail, 1);
	    printw (_(" (%d%%) of "), myfs_stats.total
		    ? 100 * myfs_stats.avail / myfs_stats.total : 0);
	    print_bytesize (myfs_stats.total, 1);
	} else
	    addstr (_("No space information"));

    case 14:
	widget_move (&info->widget, 14, 3);
	printw (_("Type:      %s "), myfs_stats.typename ? myfs_stats.typename : _("non-local vfs"));
	if (myfs_stats.type != 0xffff && myfs_stats.type != 0xffffffff)
	    printw (" (%Xh)", myfs_stats.type);

    case 13:
	widget_move (&info->widget, 13, 3);
	printw (_("Device:    %s"),
		name_trunc (myfs_stats.device, info->widget.cols - 15));
    case 12:
	widget_move (&info->widget, 12, 3);
	printw (_("Filesystem: %s"),
		name_trunc (myfs_stats.mpoint, info->widget.cols - 15));

    case 11:
	widget_move (&info->widget, 11, 3);
	printw (_("Accessed:  %s"), file_date (buf.st_atime));
	
    case 10:
	widget_move (&info->widget, 10, 3);
	printw (_("Modified:  %s"), file_date (buf.st_mtime));
	
    case 9:
	widget_move (&info->widget, 9, 3);
	printw (_("Created:   %s"), file_date (buf.st_ctime));

    case 8:
	widget_move (&info->widget, 8, 3);
#if 0
#ifdef HAVE_ST_RDEV
	if (buf.st_rdev)
	    printw ("Inode dev: major: %d, minor: %d",
		    buf.st_rdev >> 8, buf.st_rdev & 0xff);
	else
#endif
#endif
	{
	    printw (_("Size:      "));
	    print_bytesize (buf.st_size, 0);
#ifdef HAVE_ST_BLOCKS
	    printw (_(" (%d blocks)"), buf.st_blocks);
#endif
	}
	
    case 7:
	widget_move (&info->widget, 7, 3);
	printw (_("Owner:     %s/%s"), get_owner (buf.st_uid),
		get_group (buf.st_gid));
	
    case 6:
	widget_move (&info->widget, 6, 3);
	printw (_("Links:     %d"), buf.st_nlink);
	
    case 5:
	widget_move (&info->widget, 5, 3);
	printw (_("Mode:      %s (%04o)"),
		string_perm (buf.st_mode), buf.st_mode & 07777);
	
    case 4:
	widget_move (&info->widget, 4, 3);
	printw (_("Location:  %Xh:%Xh"), buf.st_dev, buf.st_ino);
	
    case 3:
	widget_move (&info->widget, 3, 2);
	/* .ado: fname is invalid if selected == 0 && ifno called from current panel */
	if (cpanel->selected){
		printw (_("File:       %s"),
			name_trunc (cpanel->dir.list [cpanel->selected].fname,
				    info->widget.cols - 15));
	} else
		printw (_("File:       None"));
     
    case 2:
    case 1:
    case 0:
	;
    } /* switch */
#else /* HAVE_X */
    x_show_info (info, &myfs_stats, &buf);
#endif
}

static void info_hook (void *data)
{
    WInfo *info = (WInfo *) data;
    Widget *other_widget;
    
    other_widget = get_panel_widget (get_current_index ());
    if (!other_widget)
	return;
    if (dlg_overlap (&info->widget, other_widget))
	return;
    
    info->ready = 1;
    info_show_info (info);
}

static void info_destroy (WInfo *info)
{
    delete_hook (&select_file_hook, info_hook);
}

static int info_callback (Dlg_head *h, WInfo *info, int msg, int par)
{
    switch (msg){

    case WIDGET_INIT:
	add_hook (&select_file_hook, info_hook, info);
	info->ready = 0;
#ifdef HAVE_X
	x_create_info (h, h->wdata, info);
#endif
	break;

#ifndef HAVE_X		  
    case WIDGET_DRAW:
	info_hook (info);
	info_show_info (info);
	return 1;
#endif	

    case WIDGET_FOCUS:
	return 0;
    }
    return default_proc (h, msg, par);
}
			   
WInfo *info_new ()
{
    WInfo *info = xmalloc (sizeof (WInfo), "panel_info");

    init_widget (&info->widget, 0, 0, 0, 0, (callback_fn)
		 info_callback, (destroy_fn) info_destroy,
		 (mouse_h) info_event, NULL);

    /* We do not want the cursor */
    widget_want_cursor (info->widget, 0);

    if (!initialized){
	initialized = 1;
	init_my_statfs ();
    }

    return info;
}


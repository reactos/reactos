/* Directory tree browser for the Midnight Commander
   Copyright (C) 1994, 1995, 1996, 1997 The Free Software Foundation

   Written: 1994, 1996 Janne Kukonlehto
            1997 Norbert Warmuth
            1996 Miguel de Icaza
	    
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

   This module has been converted to be a widget.

   The program load and saves the tree each time the tree widget is
   created and destroyed.  This is required for the future vfs layer,
   it will be possible to have tree views over virtual file systems.

   */
#include <config.h>
#include <sys/types.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <fcntl.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <errno.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>	/* For free() and atoi() */
#include <string.h>
#include "tty.h"
#include "mad.h"
#include "global.h"
#include "util.h"
#include "color.h"
#include "dialog.h"
#include "dir.h"
#include "dlg.h"
#include "widget.h"
#include "panel.h"
#include "mouse.h"
#include "main.h"
#include "file.h"	/* For copy_dir_dir(), move_dir_dir(), erase_dir() */
#include "help.h"
#include "key.h"	/* For mi_getch() */
#include "tree.h"
#include "cmd.h"
#include "../vfs/vfs.h"
#ifdef OS2_NT
#   include <io.h>
#endif

extern int command_prompt;

#define TREE_NORMALC HOT_FOCUSC

/* Specifies the display mode: 1d or 2d */
int tree_navigation_flag;

/* If this is true, then when browsing the tree the other window will
 * automatically reload it's directory with the contents of the currently
 * selected directory.
 */
int xtree_mode = 0;

/* Forwards */
static int tree_callback (Dlg_head *h, WTree *tree, int msg, int par);
#define tcallback (callback_fn) tree_callback

/* "$Id: tree.c,v 1.1 2001/12/30 09:55:21 sedwards Exp $" */

/* Returns number of common characters */
static inline int str_common (char *s1, char *s2)
{
    int result = 0;

    while (*s1++ == *s2++)
	result++;
    return result;
}

static tree_entry *back_ptr (tree_entry *ptr, int *count)
{
    int i = 0;

    while (ptr && ptr->prev && i < *count){
	ptr = ptr->prev;
	i ++;
    }
    *count = i;
    return ptr;
}

static tree_entry *forw_ptr (tree_entry *ptr, int *count)
{
    int i = 0;

    while (ptr && ptr->next && i < *count){
	ptr = ptr->next;
	i ++;
    }
    *count = i;
    return ptr;
}

/* The directory names are arranged in a single linked list in the same
   order as they are displayed. When the tree is displayed the expected
   order is like this:
        /
        /bin
        /etc
        /etc/X11
        /etc/rc.d
        /etc.old/X11
        /etc.old/rc.d
        /usr
       
   i.e. the required collating sequence when comparing two directory names is 
        '\0' < PATH_SEP < all-other-characters-in-encoding-order
   
    Since strcmp doesn't fulfil this requirement we use pathcmp when
    inserting directory names into the list. The meaning of the return value 
    of pathcmp and strcmp are the same (an integer less than, equal to, or 
    greater than zero if p1 is found to be less than, to match, or be greater 
    than p2.
 */
int
pathcmp (const char *p1, const char *p2)
{
    for ( ;*p1 == *p2; p1++, p2++)
        if (*p1 == '\0' )
    	    return 0;
	    
    if (*p1 == '\0')
        return -1;
    if (*p2 == '\0')
        return 1;
    if (*p1 == PATH_SEP)
        return -1;
    if (*p2 == PATH_SEP)
        return 1;
    return (*p1 - *p2);
}

/* Searches for specified directory */
static tree_entry *whereis (WTree *tree, char *name)
{
    tree_entry *current = tree->tree_first;
    int flag = -1;

#if 0
    if (tree->tree_last){
	flag = strcmp (tree->tree_last->name, name);
	if (flag <= 0){
	    current = tree->tree_last;
	} else if (tree->selected_ptr){
	    flag = strcmp (tree->selected_ptr->name, name);
	    if (flag <= 0){
		current = tree->selected_ptr;
	    }
	}
    }
#endif
    while (current && (flag = pathcmp (current->name, name)) < 0)
	current = current->next;

    if (flag == 0)
	return current;
    else
	return NULL;
}

/* Add a directory to the list of directories */
tree_entry *tree_add_entry (WTree *tree, char *name)
{
    int flag = -1;
    tree_entry *current = tree->tree_first;
    tree_entry *old = NULL;
    tree_entry *new;
    int i, len;
    int submask = 0;

    if (!tree)
	return 0;
    
    if (tree->tree_last && tree->tree_last->next)
	abort ();
#if 0
    if (tree->tree_last){
	flag = strcmp (tree->tree_last->name, name);
	if (flag <= 0){
	    current = tree->tree_last;
	    old = current->prev;
	} else if (tree->selected_ptr){
	    flag = strcmp (tree->selected_ptr->name, name);
	    if (flag <= 0){
		current = tree->selected_ptr;
		old = current->prev;
	    }
	}
    }
#endif
    /* Search for the correct place */
    while (current && (flag = pathcmp (current->name, name)) < 0){
	old = current;
	current = current->next;
    }
    
    if (flag == 0)
	return current; /* Already in the list */

    /* Not in the list -> add it */
    new = xmalloc (sizeof (tree_entry), "tree, tree_entry");
    if (!current){
	/* Append to the end of the list */
	if (!tree->tree_first){
	    /* Empty list */
	    tree->tree_first = new;
	    new->prev = NULL;
	} else {
	    old->next = new;
	    new->prev = old;
	}
	new->next = NULL;
	tree->tree_last = new;
    } else {
	/* Insert in to the middle of the list */
	new->prev = old;
	if (old){
	    /* Yes, in the middle */
	    new->next = old->next;
	    old->next = new;
	} else {
	    /* Nope, in the beginning of the list */
	    new->next = tree->tree_first;
	    tree->tree_first = new;
	}
	new->next->prev = new;
    }
    /* tree_count++; */

    /* Calculate attributes */
    new->name = strdup (name);
    len = strlen (new->name);
    new->sublevel = 0;
    for (i = 0; i < len; i++)
	if (new->name [i] == PATH_SEP){
	    new->sublevel++;
	    new->subname = new->name + i + 1;
	}
    if (new->next)
	submask = new->next->submask;
    else
	submask = 0;
    submask |= 1 << new->sublevel;
    submask &= (2 << new->sublevel) - 1;
    new->submask = submask;
    new->mark = 0;

    /* Correct the submasks of the previous entries */
    current = new->prev;
    while (current && current->sublevel > new->sublevel){
	current->submask |= 1 << new->sublevel;
	current = current->prev;
    }

    /* The entry has now been added */

    if (new->sublevel > 1){
	/* Let's check if the parent directory is in the tree */
	char *parent = strdup (new->name);
	int i;

	for (i = strlen (parent) - 1; i > 1; i--){
	    if (parent [i] == PATH_SEP){
		parent [i] = 0;
		tree_add_entry (tree, parent);
		break;
	    }
	}
	free (parent);
    }

    return new;
}

#if 0
/* Append a directory to the list of directories */
static tree_entry *tree_append_entry (WTree *tree, char *name)
{
    tree_entry *current, *new;
    int i, len;
    int submask = 0;

    /* We assume the directory is not yet in the list */

    new = xmalloc (sizeof (tree_entry), "tree, tree_entry");
    if (!tree->tree_first){
        /* Empty list */
        tree->tree_first = new;
        new->prev = NULL;
    } else {
        tree->tree_last->next = new;
        new->prev = tree->tree_last;
    }
    new->next = NULL;
    tree->tree_last = new;

    /* Calculate attributes */
    new->name = strdup (name);
    len = strlen (new->name);
    new->sublevel = 0;
    for (i = 0; i < len; i++)
	if (new->name [i] == PATH_SEP){
	    new->sublevel++;
	    new->subname = new->name + i + 1;
	}
    submask = 1 << new->sublevel;
    submask &= (2 << new->sublevel) - 1;
    new->submask = submask;
    new->mark = 0;

    /* Correct the submasks of the previous entries */
    current = new->prev;
    while (current && current->sublevel > new->sublevel){
	current->submask |= 1 << new->sublevel;
	current = current->prev;
    }

    /* The entry has now been appended */
    return new;
}
#endif

static void remove_entry (WTree *tree, tree_entry *entry)
{
    tree_entry *current = entry->prev;
    long submask = 0;

    if (tree->selected_ptr == entry){
	if (tree->selected_ptr->next)
	    tree->selected_ptr = tree->selected_ptr->next;
	else
	    tree->selected_ptr = tree->selected_ptr->prev;
    }

    /* Correct the submasks of the previous entries */
    if (entry->next)
	submask = entry->next->submask;
    while (current && current->sublevel > entry->sublevel){
	submask |= 1 << current->sublevel;
	submask &= (2 << current->sublevel) - 1;
	current->submask = submask;
	current = current->prev;
    }

    /* Unlink the entry from the list */
    if (entry->prev)
	entry->prev->next = entry->next;
    else
	tree->tree_first = entry->next;
    if (entry->next)
	entry->next->prev = entry->prev;
    else
	tree->tree_last = entry->prev;
    /* tree_count--; */

    /* Free the memory used by the entry */
    free (entry->name);
    free (entry);
}

void tree_remove_entry (WTree *tree, char *name)
{
    tree_entry *current, *base, *old;
    int len, base_sublevel;

    /* Miguel Ugly hack */
    if (name [0] == PATH_SEP && name [1] == 0)
	return;
    /* Miguel Ugly hack end */
    
    base = whereis (tree, name);
    if (!base)
	return;	/* Doesn't exist */
    if (tree->check_name [0] == PATH_SEP && tree->check_name [1] == 0)
	base_sublevel = base->sublevel;
    else
	base_sublevel = base->sublevel + 1;
    len = strlen (base->name);
    current = base->next;
    while (current
	   && strncmp (current->name, base->name, len) == 0
	   && (current->name[len] == '\0' || current->name[len] == PATH_SEP)){
	old = current;
	current = current->next;
	remove_entry (tree, old);
    }
    remove_entry (tree, base);
}

void tree_destroy (WTree *tree)
{
    tree_entry *current, *old;

    save_tree (tree);
    current = tree->tree_first;
    while (current){
	old = current;
	current = current->next;
	free (old->name);
	free (old);
    }
    if (tree->tree_shown){
	free (tree->tree_shown);
	tree->tree_shown = 0;
    }
    tree->selected_ptr = tree->tree_first = tree->tree_last = NULL;
}

/* Mark the subdirectories of the current directory for delete */
void start_tree_check (WTree *tree)
{
    tree_entry *current;
    int len;

    if (!tree)
	tree = (WTree *) find_widget_type (current_dlg, tcallback);
    if (!tree)
	return;
    
    /* Search for the start of subdirectories */
    mc_get_current_wd (tree->check_name, MC_MAXPATHLEN);
    tree->check_start = NULL;
    current = whereis (tree, tree->check_name);
    if (!current){
	/* Cwd doesn't exist -> add it */
	current = tree_add_entry (tree, tree->check_name);
	return;
    }

    /* Mark old subdirectories for delete */
    tree->check_start = current->next;
    len = strlen (tree->check_name);

    current = tree->check_start;
    while (current
	   && strncmp (current->name, tree->check_name, len) == 0
	   && (current->name[len] == '\0' || current->name[len] == PATH_SEP || len == 1)){
	current->mark = 1;
	current = current->next;
    }
}

/* This subdirectory exists -> clear deletion mark */
void do_tree_check (WTree *tree, const char *subname)
{
    char *name;
    tree_entry *current, *base;
    int flag = 1, len;

    /* Calculate the full name of the subdirectory */
    if (subname [0] == '.' &&
	(subname [1] == 0 || (subname [1] == '.' && subname [2] == 0)))
	return;
    if (tree->check_name [0] == PATH_SEP && tree->check_name [1] == 0)
	name = copy_strings (PATH_SEP_STR, subname, 0);
    else
	name = concat_dir_and_file (tree->check_name, subname);

    /* Search for the subdirectory */
    current = tree->check_start;
    while (current && (flag = pathcmp (current->name, name)) < 0)
	current = current->next;
    
    if (flag != 0)
	/* Doesn't exist -> add it */
	current = tree_add_entry (tree, name);
    free (name);

    /* Clear the deletion mark from the subdirectory and its children */
    base = current;
    if (base){
	len = strlen (base->name);
	base->mark = 0;
	current = base->next;
	while (current
	       && strncmp (current->name, base->name, len) == 0
	       && (current->name[len] == '\0' || current->name[len] == PATH_SEP || len == 1)){
	    current->mark = 0;
	    current = current->next;
	}
    }
}

/* Tree check searchs a tree widget in the current dialog and
 * if it finds it, it calls do_tree_check on the subname
 */
void tree_check (const char *subname)
{
    WTree *tree;

    tree = (WTree *) find_widget_type (current_dlg, tcallback);
    if (!tree)
	return;
    do_tree_check (tree, subname);
}


/* Delete subdirectories which still have the deletion mark */
void end_tree_check (WTree *tree)
{
    tree_entry *current, *old;
    int len;

    if (!tree)
	tree = (WTree *) find_widget_type (current_dlg, tcallback);
    if (!tree)
	return;
    
    /* Check delete marks and delete if found */
    len = strlen (tree->check_name);

    current = tree->check_start;
    while (current
	   && strncmp (current->name, tree->check_name, len) == 0
	   && (current->name[len] == '\0' || current->name[len] == PATH_SEP || len == 1)){
	old = current;
	current = current->next;
	if (old->mark)
	    remove_entry (tree, old);
    }
}

/* Loads the .mc.tree file */
void load_tree (WTree *tree)
{
    char *filename;
    FILE *file;
    char name [MC_MAXPATHLEN], oldname[MC_MAXPATHLEN];
    char *different;
    int len, common;

    filename = concat_dir_and_file (home_dir, MC_TREE);
    file = fopen (filename, "r");
    free (filename);
    if (!file){
	/* No new tree file -> let's try the old file */
	filename = concat_dir_and_file (home_dir, MC_TREE);
	file = fopen (filename, "r");
	free (filename);
    }
    
    if (file){
	/* File open -> read contents */
	oldname [0] = 0;
	while (fgets (name, MC_MAXPATHLEN, file)){
	    len = strlen (name);
	    if (name [len - 1] == '\n'){
		name [--len] = 0;
	    }
#ifdef OS2_NT
            /* .ado: Drives for NT and OS/2 */
            if ((len > 2)         && 
                 isalpha(name[0]) && 
                 (name[1] == ':') && 
                 (name[2] == '\\')) {
        		tree_add_entry (tree, name);
        		strcpy (oldname, name);
            } else
#endif
            /* UNIX Version */
	    if (name [0] != PATH_SEP){
		/* Clear-text decompression */
		char *s = strtok (name, " ");

		if (s){
		    common = atoi (s);
		    different = strtok (NULL, "");
		    if (different){
			strcpy (oldname + common, different);
			tree_add_entry (tree, oldname);
		    }
		}
	    } else {
		tree_add_entry (tree, name);
		strcpy (oldname, name);
	    }
	}
	fclose (file);
    }
    if (!tree->tree_first){
	/* Nothing loaded -> let's add some standard directories */
	tree_add_entry (tree, PATH_SEP_STR);
	tree->selected_ptr = tree->tree_first;
	tree_rescan_cmd (tree);
	tree_add_entry (tree, home_dir);
	tree_chdir (tree, home_dir);
	tree_rescan_cmd (tree);
    }
}

/* Save the .mc.tree file */
void save_tree (WTree *tree)
{
    tree_entry *current;
    char *filename;
    FILE *file;
    int i, common;

    filename = concat_dir_and_file (home_dir, MC_TREE);
    file = fopen (filename, "w");
    free (filename);
    if (!file){
	fprintf (stderr, _("Can't open the %s file for writing:\n%s\n"), MC_TREE,
		 unix_error_string (errno));
	return;
    }

    current = tree->tree_first;
    while (current){
	if (current->prev && (common = str_common (current->prev->name, current->name)) > 2)
	    /* Clear-text compression */
	    i = fprintf (file, "%d %s\n", common, current->name + common);
	else
	    i = fprintf (file, "%s\n", current->name);
	if (i == EOF){
	    fprintf (stderr, _("Can't write to the %s file:\n%s\n"), MC_TREE,
		 unix_error_string (errno));
	    break;
	}
	current = current->next;
    }
    fclose (file);
}

static void tree_show_mini_info (WTree *tree, int tree_lines, int tree_cols)
{
    Dlg_head *h = tree->widget.parent;
    int      line;

    /* Show mini info */
    if (tree->is_panel){
	if (!show_mini_info)
	    return;
	line = tree_lines+2;
    } else
	line = tree_lines+1;
    
    widget_move (&tree->widget, line, 1);
    hline (' ', tree_cols);
    widget_move (&tree->widget, line, 1);
    
    if (tree->searching){
	/* Show search string */
	attrset (TREE_NORMALC);
	attrset (FOCUSC);
	addch (PATH_SEP);
	
	addstr (name_trunc (tree->search_buffer, tree_cols-2));
	addch (' ');
	attrset (FOCUSC);
    } else {
	/* Show full name of selected directory */
	addstr (name_trunc (tree->selected_ptr->name, tree_cols));
    }
}

void show_tree (WTree *tree)
{
    Dlg_head *h = tree->widget.parent;
    tree_entry *current;
    int i, j, topsublevel;
    int x, y;
    int tree_lines, tree_cols;

    /* Initialize */
    x = y = 0;
    tree_lines = tlines (tree);
    tree_cols  = tree->widget.cols;

    attrset (TREE_NORMALC);
    widget_move ((Widget*)tree, y, x);
    if (tree->is_panel){
	tree_cols  -= 2;
	x = y = 1;
    }

    if (tree->tree_shown)
	free (tree->tree_shown);
    tree->tree_shown = (tree_entry**)xmalloc (sizeof (tree_entry*)*tree_lines,
					      "tree, show_tree");
    for (i = 0; i < tree_lines; i++)
	tree->tree_shown [i] = NULL;
    if (tree->tree_first)
	topsublevel = tree->tree_first->sublevel;
    else
	topsublevel = 0;
    if (!tree->selected_ptr){
	tree->selected_ptr = tree->tree_first;
	tree->topdiff = 0;
    }
    current = tree->selected_ptr;
    
    /* Calculate the directory which is to be shown on the topmost line */
    if (tree_navigation_flag){
	i = 0;
	while (current->prev && i < tree->topdiff){
	    current = current->prev;
	    if (current->sublevel < tree->selected_ptr->sublevel){
		if (strncmp (current->name, tree->selected_ptr->name,
			     strlen (current->name)) == 0)
		    i++;
	    } else if (current->sublevel == tree->selected_ptr->sublevel){
		for (j = strlen (current->name) - 1; current->name [j] != PATH_SEP; j--);
		if (strncmp (current->name, tree->selected_ptr->name, j) == 0)
		    i++;
	    } else if (current->sublevel == tree->selected_ptr->sublevel + 1
		       && strlen (tree->selected_ptr->name) > 1){
		if (strncmp (current->name, tree->selected_ptr->name,
			     strlen (tree->selected_ptr->name)) == 0)
		    i++;
	    }
	}
	tree->topdiff = i;
    } else
	current = back_ptr (current, &tree->topdiff);

    /* Loop for every line */
    for (i = 0; i < tree_lines; i++){
	/* Move to the beginning of the line */
	widget_move (&tree->widget, y+i, x);

	hline (' ', tree_cols);
	widget_move (&tree->widget, y+i, x);

	if (!current)
	    continue;
	
	tree->tree_shown [i] = current;
	if (current->sublevel == topsublevel){

	    /* Top level directory */
	    if (tree->active && current == tree->selected_ptr)
		if (!use_colors && !tree->is_panel)
			attrset (MARKED_COLOR);
		else
			attrset (SELECTED_COLOR);

	    /* Show full name */
	    addstr (name_trunc (current->name, tree_cols - 6));
	} else{
	    /* Sub level directory */

	    acs ();
	    /* Output branch parts */
	    for (j = 0; j < current->sublevel - topsublevel - 1; j++){
		if (tree_cols - 8 - 3 * j < 9)
		    break;
		addch (' ');
		if (current->submask & (1 << (j + topsublevel + 1)))
		    addch (ACS_VLINE);
		else
		    addch (' ');
		addch (' ');
	    }
	    addch (' '); j++;
	    if (!current->next || !(current->next->submask & (1 << current->sublevel)))
		addch (ACS_LLCORNER);
	    else
		addch (ACS_LTEE);
	    addch (ACS_HLINE);
	    noacs ();
	    
	    if (tree->active && current == tree->selected_ptr)
		/* Selected directory -> change color */
		if (!use_colors && !tree->is_panel)
		    attrset (MARKED_COLOR);
		else
		    attrset (SELECTED_COLOR);

	    /* Show sub-name */
	    addch (' ');
	    addstr (name_trunc (current->subname,
				tree_cols - 2 - 4 - 3 * j));
	}
	addch (' ');
	
	/* Return to normal color */
	attrset (TREE_NORMALC);

	/* Calculate the next value for current */
	if (tree_navigation_flag){
	    current = current->next;
	    while (current){
		if (current->sublevel < tree->selected_ptr->sublevel){
		    if (strncmp (current->name, tree->selected_ptr->name,
				 strlen (current->name)) == 0)
			break;
		} else if (current->sublevel == tree->selected_ptr->sublevel){
		    for (j = strlen (current->name) - 1; current->name [j] != PATH_SEP; j--);
		    if (strncmp (current->name,tree->selected_ptr->name,j)== 0)
			break;
		} else if (current->sublevel == tree->selected_ptr->sublevel+1
			   && strlen (tree->selected_ptr->name) > 1){
		    if (strncmp (current->name, tree->selected_ptr->name,
				 strlen (tree->selected_ptr->name)) == 0)
			break;
		}
		current = current->next;
	    }
	} else
	    current = current->next;
    }
    tree_show_mini_info (tree, tree_lines, tree_cols);
}

static void check_focus (WTree *tree)
{
    if (tree->topdiff < 3)
	tree->topdiff = 3;
    else if (tree->topdiff >= tlines (tree) - 3)
	tree->topdiff = tlines (tree) - 3 - 1;
}

void tree_move_backward (WTree *tree, int i)
{
    tree_entry *current;
    int j = 0;
    
    if (tree_navigation_flag){
	current = tree->selected_ptr;
	while (j < i && current->prev
	       && current->prev->sublevel >= tree->selected_ptr->sublevel){
	    current = current->prev;
	    if (current->sublevel == tree->selected_ptr->sublevel){
		tree->selected_ptr = current;
		j ++;
	    }
	}
	i = j;
    } else
	tree->selected_ptr = back_ptr (tree->selected_ptr, &i);
    tree->topdiff -= i;
    check_focus (tree);
}

void tree_move_forward (WTree *tree, int i)
{
    tree_entry *current;
    int j = 0;

    if (tree_navigation_flag){
	current = tree->selected_ptr;
	while (j < i && current->next
	       && current->next->sublevel >= tree->selected_ptr->sublevel){
	    current = current->next;
	    if (current->sublevel == tree->selected_ptr->sublevel){
		tree->selected_ptr = current;
		j ++;
	    }
	}
	i = j;
    } else
	tree->selected_ptr = forw_ptr (tree->selected_ptr, &i);
    tree->topdiff += i;
    check_focus (tree);
}

void tree_move_to_child (WTree *tree)
{
    tree_entry *current;

    /* Do we have a starting point? */
    if (!tree->selected_ptr)
	return;
    /* Take the next entry */
    current = tree->selected_ptr->next;
    /* Is it the child of the selected entry */
    if (current && current->sublevel > tree->selected_ptr->sublevel){
	/* Yes -> select this entry */
	tree->selected_ptr = current;
	tree->topdiff++;
	check_focus (tree);
    } else {
	/* No -> rescan and try again */
	tree_rescan_cmd (tree);
	current = tree->selected_ptr->next;
	if (current && current->sublevel > tree->selected_ptr->sublevel){
	    tree->selected_ptr = current;
	    tree->topdiff++;
	    check_focus (tree);
	}
    }
}

int tree_move_to_parent (WTree *tree)
{
    tree_entry *current;
    tree_entry *old;
    
    if (!tree->selected_ptr)
	return 0;
    old = tree->selected_ptr;
    current = tree->selected_ptr->prev;
    while (current && current->sublevel >= tree->selected_ptr->sublevel){
	current = current->prev;
	tree->topdiff--;
    }
    if (!current)
	current = tree->tree_first;
    tree->selected_ptr = current;
    check_focus (tree);
    return tree->selected_ptr != old;
}

void tree_move_to_top (WTree *tree)
{
    tree->selected_ptr = tree->tree_first;
    tree->topdiff = 0;
}

void tree_move_to_bottom (WTree *tree)
{
    tree->selected_ptr = tree->tree_last;
    tree->topdiff = tlines (tree) - 3 - 1;
}

void tree_chdir (WTree *tree, char *dir)
{
    tree_entry *current;

    current = whereis (tree, dir);
    if (current){
	tree->selected_ptr = current;
	check_focus (tree);
    }
}

/* Handle mouse click */
void tree_event (WTree *tree, int y)
{
    if (tree->tree_shown [y]){
	tree->selected_ptr = tree->tree_shown [y];
	tree->topdiff = y;
    }
    show_tree (tree);
}

static void chdir_sel (WTree *tree);

static void maybe_chdir (WTree *tree)
{
    if (!(xtree_mode && tree->is_panel))
	return;
    if (is_idle ())
	chdir_sel (tree);
}

/* Mouse callback */
static int event_callback (Gpm_Event *event, WTree *tree)
{
    if (!(event->type & GPM_UP))
	return MOU_ENDLOOP;

    if (tree->is_panel)
	event->y--;
    
    event->y--;

    if (!tree->active)
	change_panel ();

    if (event->y < 0){
	tree_move_backward (tree, tlines (tree) - 1);
	show_tree (tree);
    }
    else if (event->y >= tlines (tree)){
	tree_move_forward (tree, tlines (tree) - 1);
	show_tree (tree);
    } else {
	tree_event (tree, event->y);
	if ((event->type & (GPM_UP|GPM_DOUBLE)) == (GPM_UP|GPM_DOUBLE)){
	    chdir_sel (tree);
	}
    }
    return MOU_ENDLOOP;
}

/* Search tree for text */
int search_tree (WTree *tree, char *text)
{
    tree_entry *current;
    int len;
    int wrapped = 0;
    int found = 0;

    len = strlen (text);
    current = tree->selected_ptr;
    found = 0;
    while (!wrapped || current != tree->selected_ptr){
	if (strncmp (current->subname, text, len) == 0){
	    tree->selected_ptr = current;
	    found = 1;
	    break;
	}
	current = current->next;
	if (!current){
	    current = tree->tree_first;
	    wrapped = 1;
	}
	tree->topdiff++;
    }
    check_focus (tree);
    return found;
}

static void tree_do_search (WTree *tree, int key)
{
    int l;

    l = strlen (tree->search_buffer);
    if (l && (key == 8 || key == 0177 || key == KEY_BACKSPACE))
	tree->search_buffer [--l] = 0;
    else {
	if (key && l < sizeof (tree->search_buffer)){
	    tree->search_buffer [l] = key;
	    tree->search_buffer [l+1] = 0;
	    l++;
	}
    }

    if (!search_tree (tree, tree->search_buffer))
	tree->search_buffer [--l] = 0;
    
    show_tree (tree);
    maybe_chdir (tree);
}

void tree_rescan_cmd (WTree *tree)
{
    DIR *dirp;
    struct dirent *dp;
    struct stat buf;
    char old_dir [MC_MAXPATHLEN];

    if (!tree->selected_ptr || !mc_get_current_wd (old_dir, MC_MAXPATHLEN) ||
	mc_chdir (tree->selected_ptr->name))
	return;
    
    start_tree_check (tree);
    dirp = opendir (".");
    if (dirp){
	for (dp = readdir (dirp); dp; dp = readdir (dirp)){
	    lstat (dp->d_name, &buf);
	    if (S_ISDIR (buf.st_mode))
		do_tree_check (tree, dp->d_name);
	}
	closedir (dirp);
    }
    end_tree_check (tree);
    mc_chdir (old_dir);
}

int tree_forget_cmd (WTree *tree)
{
    if (tree->selected_ptr)
	tree_remove_entry (tree, tree->selected_ptr->name);
    return 1;
}

#if 0
static int toggle_nav_mode (void)
{
    tree_navigation_flag = 1 - tree_navigation_flag;

    return 1;
}
#endif

void tree_copy (WTree *tree, char *default_dest)
{
    char *dest;

    if (!tree->selected_ptr)
	return;
    sprintf (cmd_buf, _("Copy \"%s\" directory to:"),
	     name_trunc (tree->selected_ptr->name, 50));
    dest = input_expand_dialog (_(" Copy "), cmd_buf, default_dest);
    if (!dest || !*dest){
	return;
    }
    create_op_win (OP_COPY, 0);
    file_mask_defaults ();
    copy_dir_dir (tree->selected_ptr->name, dest, 1, 0, 0, 0);
    destroy_op_win ();
    free (dest);
}

static void tree_help_cmd (void)
{
    char *hlpfile = concat_dir_and_file (mc_home, "mc.hlp");
    interactive_display (hlpfile,  "[Directory Tree]");
    free (hlpfile);
}

static int tree_copy_cmd (WTree *tree)
{
    tree_copy (tree, "");
    return 1;
}

void tree_move (WTree *tree, char *default_dest)
{
    char *dest;
    struct stat buf;

    if (!tree->selected_ptr)
	return;
    sprintf (cmd_buf, _("Move \"%s\" directory to:"),
	     name_trunc (tree->selected_ptr->name, 50));
    dest = input_expand_dialog (_(" Move "), cmd_buf, default_dest);
    if (!dest || !*dest){
	return;
    }
    if (stat (dest, &buf)){
	message (1, _(" Error "), _(" Can't stat the destination \n %s "),
		 unix_error_string (errno));
	free (dest);
	return;
    }
    if (!S_ISDIR (buf.st_mode)){
	message (1, _(" Error "), _(" The destination isn't a directory "));
	free (dest);
	return;
    }
    create_op_win (OP_MOVE, 0);
    file_mask_defaults ();
    move_dir_dir (tree->selected_ptr->name, dest);
    destroy_op_win ();
    free (dest);
}

static int tree_move_cmd (WTree *tree)
{
    tree_move (tree, "");
    return 1;
}

static int tree_mkdir_cmd (WTree *tree)
{
    char old_dir [MC_MAXPATHLEN];

    if (!tree->selected_ptr)
	return 0;
    if (!mc_get_current_wd (old_dir, MC_MAXPATHLEN))
	return 0;
    if (chdir (tree->selected_ptr->name))
	return 0;
    /* FIXME
    mkdir_cmd (tree);
    */
    tree_rescan_cmd (tree);
    chdir (old_dir);
    return 1;
}

static void tree_rmdir_cmd (WTree *tree)
{
    char old_dir [MC_MAXPATHLEN];

    if (tree->selected_ptr){
	if (!mc_get_current_wd (old_dir, MC_MAXPATHLEN))
	    return;
	if (mc_chdir (PATH_SEP_STR))
	    return;
	if (confirm_delete){
	    char *cmd_buf;
	    int result;

	    cmd_buf = xmalloc (strlen (tree->selected_ptr->name) + 20,
			       "tree, rmdir_cmd");
	    sprintf (cmd_buf, _("  Delete %s?  "), tree->selected_ptr->name);
	    result = query_dialog (_(" Delete "), cmd_buf, 3, 2, _("&Yes"), _("&No"));
	    free (cmd_buf);
	    if (result != 0){
		return;
	    }
	}
	create_op_win (OP_DELETE, 0);
	if (erase_dir (tree->selected_ptr->name) == FILE_CONT)
	    tree_forget_cmd (tree);
	destroy_op_win ();
	mc_chdir (old_dir);
	return;
    } else
	return;
}

#if 0
static int tree_quit_cmd (void)
{
    /*
       FIXME
    return done = 1;
    */
    return 1;
}
#endif

static void set_navig_label (Dlg_head *h);
static void tree_toggle_navig (Dlg_head *h)
{
    tree_navigation_flag = 1 - tree_navigation_flag;
    set_navig_label (h);
}

void set_navig_label (Dlg_head *h)
{
    define_label_data (h, (Widget *)tree,
		       4, tree_navigation_flag ? _("Static") : _("Dynamc"),
		       (void (*)(void *))tree_toggle_navig, h);
}

static void move_down (WTree *tree)
{
    tree_move_forward (tree, 1);
    show_tree (tree);
    maybe_chdir (tree);
}

static void move_up (WTree *tree)
{
    tree_move_backward (tree, 1);
    show_tree (tree);
    maybe_chdir (tree);
}

static void move_home (WTree *tree)
{
    tree_move_to_top (tree);
    show_tree (tree);
    maybe_chdir (tree);
}

static void move_end (WTree *tree)
{
    tree_move_to_bottom (tree);
    show_tree (tree);
    maybe_chdir (tree);
}

static int move_left (WTree *tree)
{
    int v;
    
    if (tree_navigation_flag){
	v = tree_move_to_parent (tree);
	show_tree (tree);
	maybe_chdir (tree);
	return v;
    }
    return 0;
}

static int move_right (WTree *tree)
{
    if (tree_navigation_flag){
	tree_move_to_child (tree);
	show_tree (tree);
	maybe_chdir (tree);
	return 1;
    }
    return 0;
}

static void move_prevp (WTree *tree)
{
    tree_move_backward (tree, tlines (tree) - 1);
    show_tree (tree);
    maybe_chdir (tree);
}

static void move_nextp (WTree *tree)
{
    tree_move_forward (tree, tlines (tree) - 1);
    show_tree (tree);
    maybe_chdir (tree);
}

static void chdir_sel (WTree *tree)
{
    if (!tree->is_panel){
	tree->done = 1;
	return;
    }
    change_panel ();
    if (do_cd (tree->selected_ptr->name, cd_exact)){
	paint_panel (cpanel);
	select_item (cpanel);
    } else {
	message (1, MSG_ERROR, _(" Can't chdir to \"%s\" \n %s "),
		 tree->selected_ptr->name, unix_error_string (errno));
    }
    change_panel ();
    show_tree (tree);
    return;
}

static void start_search (WTree *tree)
{
    int i;

    if (tree->searching){
    
	if (tree->selected_ptr == tree->tree_last)
	    tree_move_to_top(tree);
	else {
	/* set navigation mode temporarily to 'Static' because in 
	 * dynamic navigation mode tree_move_forward will not move
	 * to a lower sublevel if necessary (sequent searches must
	 * start with the directory followed the last found directory)
         */	 
	    i = tree_navigation_flag;
	    tree_navigation_flag = 0;
	    tree_move_forward (tree, 1);
	    tree_navigation_flag = i;
	}
	tree_do_search (tree, 0);
    }
    else {
	tree->searching = 1;
	tree->search_buffer[0] = 0;
    }
}

static key_map tree_keymap [] = {
    { XCTRL('n'), move_down    },
    { XCTRL('p'), move_up      },
    { KEY_DOWN,   move_down    },
    { KEY_UP,     move_up      },
    { '\n',       chdir_sel    },
    { KEY_ENTER,  chdir_sel    },
    { KEY_HOME,   move_home    },
    { KEY_C1,     move_end     },
    { KEY_END,    move_end     },
    { KEY_A1,     move_home    },
    { KEY_NPAGE,  move_nextp   },
    { KEY_PPAGE,  move_prevp   },
    { XCTRL('v'), move_nextp   },
    { ALT('v'),   move_prevp   },
    { XCTRL('p'), move_up      },
    { XCTRL('p'), move_down    },
    { XCTRL('s'), start_search },
    { ALT('s'),   start_search },
    { XCTRL('r'), tree_rescan_cmd },
    { KEY_DC,     tree_rmdir_cmd },
    { 0, 0 }
    };

static inline int tree_key (WTree *tree, int key)
{
    int i;

    for (i = 0; tree_keymap [i].key_code; i++){
	if (key == tree_keymap [i].key_code){
	    if (tree_keymap [i].fn != start_search)
	        tree->searching = 0;
	    (*tree_keymap [i].fn)(tree);
	    show_tree (tree);
	    return 1;
	}
    }

    /* We do not want to use them if we do not need to */
    /* Input line may want to take the motion key event */
    if (key == KEY_LEFT)
	return move_left (tree);

    if (key == KEY_RIGHT)
	return move_right (tree);

    if (is_abort_char (key)) {
	if (tree->is_panel) {
	    tree->searching = 0;
	    show_tree (tree);
	    return 1;  /* eat abort char */
	}
	return 0;  /* modal tree dialog: let upper layer see the
		      abort character and close the dialog */
    }

    /* Do not eat characters not meant for the tree below ' ' (e.g. C-l). */
    if ((key >= ' '&& key <= 255) || key == 8 || key == KEY_BACKSPACE) {
	if (tree->searching){
	    tree_do_search (tree, key);
	    show_tree (tree);
	    return 1;
	}

	if (!command_prompt) {
	    start_search (tree);
	    tree_do_search (tree, key);
	    return 1;
	}
	return tree->is_panel;
    }

    return 0;
}

static void tree_frame (Dlg_head *h, WTree *tree)
{
    attrset (NORMAL_COLOR);
    widget_erase ((Widget*) tree);
    if (tree->is_panel)
	draw_double_box (h, tree->widget.y, tree->widget.x, tree->widget.lines,
		         tree->widget.cols);
    
    if (show_mini_info && tree->is_panel){
	widget_move (tree, tlines (tree) + 1, 1);
	hline (ACS_HLINE, tree->widget.cols - 2);
    }
}


static int tree_callback (Dlg_head *h, WTree *tree, int msg, int par)
{
    switch (msg){
    case WIDGET_DRAW:
	tree_frame (h, tree);
	show_tree (tree);
	return 1;

    case WIDGET_KEY:
	return tree_key (tree, par);

    case WIDGET_FOCUS:
	tree->active = 1;
	define_label (h, (Widget *)tree, 1, _("Help"), (voidfn) tree_help_cmd);
	define_label_data (h, (Widget *)tree, 
	    2, _("Rescan"), (buttonbarfn)tree_rescan_cmd, tree);
	define_label_data (h, (Widget *)tree, 
	    3, _("Forget"), (buttonbarfn)tree_forget_cmd, tree);
	define_label_data (h, (Widget *)tree, 
	    5, _("Copy"),   (buttonbarfn) tree_copy_cmd, tree);
	define_label_data (h, (Widget *)tree, 
	    6, _("RenMov"), (buttonbarfn) tree_move_cmd, tree);
#if 0
	/* FIXME: mkdir is currently defunct */
	define_label_data (h, (Widget *)tree, 
	    7, _("Mkdir"),  (buttonbarfn) tree_mkdir_cmd, tree);
#else
        define_label (h, (Widget *)tree, 7, "", 0);
#endif
	define_label_data (h, (Widget *)tree, 
	    8, _("Rmdir"),  (buttonbarfn) tree_rmdir_cmd, tree);
	set_navig_label (h);
	redraw_labels (h, (Widget *)tree);

	
	/* FIXME: Should find a better way of only displaying the
	   currently selected item */ 
	show_tree (tree);
	return 1;

	/* FIXME: Should find a better way of changing the color of the
	   selected item */
    case WIDGET_UNFOCUS:
	tree->active = 0;
	show_tree (tree);
	return 1;
    }
    return default_proc (h, msg, par);
}

WTree *tree_new (int is_panel, int y, int x, int lines, int cols)
{
    WTree *tree = xmalloc (sizeof (WTree), "tree_new");

    init_widget (&tree->widget, y, x, lines, cols, tcallback,
		 (destroy_fn) tree_destroy, (mouse_h) event_callback, NULL);
    tree->is_panel = is_panel;
    tree->selected_ptr = 0;
    tree->tree_shown = 0;
    tree->search_buffer [0] = 0;
    tree->tree_first = tree->tree_last = 0;
    tree->topdiff = tree->widget.lines / 2;
    tree->searching = 0;
    tree->done = 0;
    tree->active = 0;
    
    /* We do not want to keep the cursor */
    widget_want_cursor (tree->widget, 0);
    load_tree (tree);
    return tree;
}

static char *get_absolute_name (char *file)
{
    char dir [MC_MAXPATHLEN];

    if (file [0] == PATH_SEP)
	return strdup (file);
    mc_get_current_wd (dir, MC_MAXPATHLEN);
    return get_full_name (dir, file);
}

static int my_mkdir_rec (char *s, mode_t mode)
{
    char *p, *q;
    int result;
    
    if (!mc_mkdir (s, mode))
        return 0;

    /* FIXME: should check instead if s is at the root of that filesystem */
    if (!vfs_file_is_local (s))
	return -1;
    if (!strcmp (vfs_path(s), PATH_SEP_STR))
        return ENOTDIR;
    p = concat_dir_and_file (s, "..");
    q = vfs_canon (p);
    free (p);
    if (!(result = my_mkdir_rec (q, mode))) {
    	result = mc_mkdir (s, mode);
    } 
    free (q);
    return result;
}

int my_mkdir (char *s, mode_t mode)
{
    int result;
#if FIXME    
    WTree *tree = 0;
#endif    

    result = mc_mkdir (s, mode);
#ifdef OS2_NT
    /* .ado: it will be disabled in OS/2 and NT */
    /* otherwise crash if directory already exists. */
    return result;
#endif
    if (result) {
        char *p = vfs_canon (s);
        
        result = my_mkdir_rec (p, mode);
        free (p);
    }
    if (result == 0){
	s = get_absolute_name (s);
#if FIXME
	/* FIXME: Should receive a Wtree! */

	tree_add_entry (tree, s);
#endif
	free (s);
    }
    return result;
}

int my_rmdir (char *s)
{
    int result;
#if FIXME    
    WTree *tree = 0;
#endif    

    /* FIXME: Should receive a Wtree! */
    result = mc_rmdir (s);
    if (result == 0){
	s = get_absolute_name (s);
#if FIXME
	tree_remove_entry (tree, s);
#endif
	free (s);
    }
    return result;
}



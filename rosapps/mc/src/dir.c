/* Directory routines
   Copyright (C) 1994 Miguel de Icaza.
   
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
#include "tty.h"
#include "fs.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>
#include "x.h"
#include "mad.h"
#include "global.h"
#define DIR_H_INCLUDE_HANDLE_DIRENT
#include "dir.h"
#include "util.h"
#include "tree.h"
#include "../vfs/vfs.h"

/* "$Id: dir.c,v 1.1 2001/12/30 09:55:26 sedwards Exp $" */

/* If true show files starting with a dot */
int show_dot_files = 1;

/* If true show files ending in ~ */
int show_backups = 0;

/* If false then directories are shown separately from files */
int mix_all_files = 0;

/* Reverse flag */
static int reverse = 1;

/* Are the files sorted case sensitively? */
static int case_sensitive = OS_SORT_CASE_SENSITIVE_DEFAULT;

#define MY_ISDIR(x) ( (S_ISDIR (x->buf.st_mode) || x->f.link_to_dir) ? 1 : 0)

sort_orders_t sort_orders [SORT_TYPES_TOTAL] = {
    { N_("&Unsorted"),    unsorted },
    { N_("&Name"),        sort_name },
    { N_("&Extension"),   sort_ext },
    { N_("&Modify time"), sort_time },
    { N_("&Access time"), sort_atime },
    { N_("&Change time"), sort_ctime },
    { N_("&Size"),        sort_size },
    { N_("&Inode"),       sort_inode },

    /* New sort orders */
    { N_("&Type"),        sort_type },
    { N_("&Links"),       sort_links },
    { N_("N&GID"),        sort_ngid },
    { N_("N&UID"),        sort_nuid },
    { N_("&Owner"),       sort_owner },
    { N_("&Group"),       sort_group }
};

#define string_sortcomp(a,b) (case_sensitive ? strcmp (a,b) : strcasecmp (a,b))

int
unsorted (const file_entry *a, const file_entry *b)
{
    return 0;
}

int
sort_name (const file_entry *a, const file_entry *b)
{
    int ad = MY_ISDIR (a);
    int bd = MY_ISDIR (b);
    
    if (ad == bd || mix_all_files)
	return string_sortcomp (a->fname, b->fname) * reverse;
    return bd-ad;
}

int
sort_ext (const file_entry *a, const file_entry *b)
{
    char *exta, *extb;
    int r;
    int ad = MY_ISDIR (a);
    int bd = MY_ISDIR (b);

    if (ad == bd || mix_all_files){
	exta = extension (a->fname);
	extb = extension (b->fname);
	r = string_sortcomp (exta, extb);
	if (r)
	    return r * reverse;
	else
	    return sort_name (a, b);
    } else
	return bd-ad;
}

int
sort_owner (const file_entry *a, const file_entry *b)
{
    int ad = MY_ISDIR (a);
    int bd = MY_ISDIR (b);
    
    if (ad == bd || mix_all_files)
	return string_sortcomp (get_owner (a->buf.st_uid), get_owner (a->buf.st_uid)) * reverse;
    return bd-ad;
}

int
sort_group (const file_entry *a, const file_entry *b)
{
    int ad = MY_ISDIR (a);
    int bd = MY_ISDIR (b);
    
    if (ad == bd || mix_all_files)
	return string_sortcomp (get_group (a->buf.st_gid), get_group (a->buf.st_gid)) * reverse;
    return bd-ad;
}

int
sort_time (const file_entry *a, const file_entry *b)
{
    int ad = MY_ISDIR (a);
    int bd = MY_ISDIR (b);

    if (ad == bd || mix_all_files)
	return (a->buf.st_mtime - b->buf.st_mtime) * reverse;
    else
	return bd-ad;
}

int
sort_ctime (const file_entry *a, const file_entry *b)
{
    int ad = MY_ISDIR (a);
    int bd = MY_ISDIR (b);

    if (ad == bd || mix_all_files)
	return (a->buf.st_ctime - b->buf.st_ctime) * reverse;
    else
	return bd-ad;
}

int
sort_atime (const file_entry *a, const file_entry *b)
{
    int ad = MY_ISDIR (a);
    int bd = MY_ISDIR (b);

    if (ad == bd || mix_all_files)
	return (a->buf.st_atime - b->buf.st_atime) * reverse;
    else
	return bd-ad;
}

int
sort_inode (const file_entry *a, const file_entry *b)
{
    int ad = MY_ISDIR (a);
    int bd = MY_ISDIR (b);

    if (ad == bd || mix_all_files)
	return (a->buf.st_ino - b->buf.st_ino) * reverse;
    else
	return bd-ad;
}

int
sort_size (const file_entry *a, const file_entry *b)
{
    int ad = MY_ISDIR (a);
    int bd = MY_ISDIR (b);

    if (ad == bd || mix_all_files)
	return (b->buf.st_size - a->buf.st_size) * reverse;
    else
	return bd-ad;
}

int
sort_links (const file_entry *a, const file_entry *b)
{
    int ad = MY_ISDIR (a);
    int bd = MY_ISDIR (b);

    if (ad == bd || mix_all_files)
	return (b->buf.st_nlink - a->buf.st_nlink) * reverse;
    else
	return bd-ad;
}

int
sort_ngid (const file_entry *a, const file_entry *b)
{
    int ad = MY_ISDIR (a);
    int bd = MY_ISDIR (b);

    if (ad == bd || mix_all_files)
	return (b->buf.st_gid - a->buf.st_gid) * reverse;
    else
	return bd-ad;
}

int
sort_nuid (const file_entry *a, const file_entry *b)
{
    int ad = MY_ISDIR (a);
    int bd = MY_ISDIR (b);

    if (ad == bd || mix_all_files)
	return (b->buf.st_uid - a->buf.st_uid) * reverse;
    else
	return bd-ad;
}

inline static int
file_type_to_num (const file_entry *fe)
{
    const struct stat *s = &fe->buf;
    
    if (S_ISDIR (s->st_mode))
	return 0;
    if (S_ISLNK (s->st_mode)){
	if (fe->f.link_to_dir)
	    return 1;
	if (fe->f.stalled_link)
	    return 2;
	else
	    return 3;
    }
    if (S_ISSOCK (s->st_mode))
	return 4;
    if (S_ISCHR (s->st_mode))
	return 5;
    if (S_ISBLK (s->st_mode))
	return 6;
    if (S_ISFIFO (s->st_mode))
	return 7;
    if (is_exe (s->st_mode))
	return 8;
    return 9;
}

int
sort_type (const file_entry *a, const file_entry *b)
{
    int aa  = file_type_to_num (a);
    int bb  = file_type_to_num (b);
    
    return bb-aa;
}


void
do_sort (dir_list *list, sortfn *sort, int top, int reverse_f, int case_sensitive_f)
{
    int i;
    file_entry tmp_fe;

    for (i = 0; i < top + 1; i++) {             /* put ".." first in list */
	if (!strcmp (list->list [i].fname, "..")) {
            if (i > 0) {                        /* swap [i] and [0] */
                memcpy (&tmp_fe, &(list->list [0]), sizeof (file_entry));
                memcpy (&(list->list [0]), &(list->list [i]), sizeof (file_entry));
                memcpy (&(list->list [i]), &tmp_fe, sizeof (file_entry));
            }
            break;
        }
    }

    reverse = reverse_f ? -1 : 1;
    case_sensitive = case_sensitive_f;
    qsort (&(list->list) [1], top, sizeof (file_entry), sort);
}

void clean_dir (dir_list *list, int count)
{
    int i;

    for (i = 0; i < count; i++){
	free (list->list [i].fname);
	list->list [i].fname = 0;
	if (list->list [i].cache != NULL) {
	    free (list->list [i].cache);
	    list->list [i].cache = NULL;
	}
    }
}

static int 
add_dotdot_to_list (dir_list *list, int index)
{
    char buffer [MC_MAXPATHLEN + MC_MAXPATHLEN];
    char *p, *s;
    int i = 0;
    
    /* Need to grow the *list? */
    if (index == list->size) {
	list->list = realloc (list->list, sizeof (file_entry) *
			      (list->size + RESIZE_STEPS));
	if (!list->list)
	    return 0;
	list->size += RESIZE_STEPS;
    }

    (list->list) [index].fnamelen = 2;
    (list->list) [index].fname = strdup ("..");
    (list->list) [index].cache = NULL;
    (list->list) [index].f.link_to_dir = 0;
    (list->list) [index].f.stalled_link = 0;
    
    /* FIXME: We need to get the panel definition! to use file_mark */
    (list->list) [index].f.marked = 0;
    mc_get_current_wd (buffer, sizeof (buffer) - 1 );
    if (buffer [strlen (buffer) - 1] == PATH_SEP)
    	buffer [strlen (buffer) - 1] = 0;
    for (;;) {
    	strcat (buffer, PATH_SEP_STR "..");
        p = vfs_canon (buffer);
        if (mc_stat (p, &((list->list) [index].buf)) != -1){
	    free (p);
            break;
	}
        i = 1;
        if ((s = vfs_path (p)) && !strcmp (s, PATH_SEP_STR)){
	    free (p);
            return 1;
	}
	strcpy (buffer, p);
	free (p);
    }

/* Commented out to preserve a usable '..'. What's the purpose of this
 * three lines? (Norbert) */
#if 0
    if (i) { /* So there is bogus information on the .. directory's stat */
        (list->list) [index].buf.st_mode &= ~0444;
    }
#endif
    return 1;
}

/* Used to set up a directory list when there is no access to a directory */
int set_zero_dir (dir_list *list)
{
    return (add_dotdot_to_list (list, 0));
}

/* If you change handle_dirent then check also handle_path. */
/* Return values: -1 = failure, 0 = don't add, 1 = add to the list */
int handle_dirent (dir_list *list, char *filter, struct dirent *dp,
		   struct stat *buf1, int next_free, int *link_to_dir,
		   int *stalled_link)
{
    if (dp->d_name [0] == '.' && dp->d_name [1] == 0)
	return 0;
    if (!show_dot_files){
	if (dp->d_name [0] == '.'){
	    if (!(dp->d_name [1] == 0))
		if (!(dp->d_name [1] == '.' && NLENGTH (dp) == 2))
		    return 0;
	}
    }
    if (!show_backups && dp->d_name [NLENGTH (dp)-1] == '~')
	return 0;
    if (mc_lstat (dp->d_name, buf1) == -1)
        return 0;

    if (S_ISDIR (buf1->st_mode))
	tree_check (dp->d_name);
    
    /* A link to a file or a directory? */
    *link_to_dir = 0;
    *stalled_link = 0;
    if (S_ISLNK(buf1->st_mode)){
	struct stat buf2;
	if (!mc_stat (dp->d_name, &buf2))
	    *link_to_dir = S_ISDIR(buf2.st_mode) != 0;
	else
	    *stalled_link = 1;
    }
    if (!(S_ISDIR(buf1->st_mode) || *link_to_dir) && filter &&
	!regexp_match (filter, dp->d_name, match_file))
	return 0;

    /* Need to grow the *list? */
    if (next_free == list->size){
	list->list = realloc (list->list, sizeof (file_entry) *
			      (list->size + RESIZE_STEPS));
	if (!list->list)
	    return -1;
	list->size += RESIZE_STEPS;
    }
    return 1;
}

/* handle_path is a simplified handle_dirent. The difference is that 
   handle_path doesn't pay attention to show_dot_files and show_backups.
   Moreover handle_path can't be used with a filemask. 
   If you change handle_path then check also handle_dirent. */
/* Return values: -1 = failure, 0 = don't add, 1 = add to the list */
int handle_path (dir_list *list, char *path,
		   struct stat *buf1, int next_free, int *link_to_dir,
		   int *stalled_link)
{
    if (path [0] == '.' && path [1] == 0)
	return 0;
    if (mc_lstat (path, buf1) == -1)
        return 0;

    if (S_ISDIR (buf1->st_mode))
	tree_check (path);
    
    /* A link to a file or a directory? */
    *link_to_dir = 0;
    *stalled_link = 0;
    if (S_ISLNK(buf1->st_mode)){
	struct stat buf2;
	if (!mc_stat (path, &buf2))
	    *link_to_dir = S_ISDIR(buf2.st_mode) != 0;
	else
	    *stalled_link = 1;
    }

    /* Need to grow the *list? */
    if (next_free == list->size){
	list->list = realloc (list->list, sizeof (file_entry) *
			      (list->size + RESIZE_STEPS));
	if (!list->list)
	    return -1;
	list->size += RESIZE_STEPS;
    }
    return 1;
}

int do_load_dir(dir_list *list, sortfn *sort, int reverse, int case_sensitive, char *filter)
{
    DIR           *dirp;
    struct dirent *dp;
    int           status, link_to_dir, stalled_link;
    int           next_free = 0;
    struct stat   buf;
    int dotdot_found = 0;

    start_tree_check (NULL);
    
    dirp = mc_opendir (".");
    if (!dirp){
	return set_zero_dir (list);
    }
    for (dp = mc_readdir (dirp); dp; dp = mc_readdir (dirp)){
	status = handle_dirent (list, filter, dp, &buf, next_free, &link_to_dir,
	    &stalled_link);
	if (status == 0)
	    continue;
	if (status == -1)
	    return next_free;
	list->list [next_free].fnamelen = NLENGTH (dp);
	list->list [next_free].fname = strdup (dp->d_name);
	list->list [next_free].cache = NULL; 
	list->list [next_free].f.marked = 0;
	list->list [next_free].f.link_to_dir = link_to_dir;
	list->list [next_free].f.stalled_link = stalled_link;
	list->list [next_free].buf = buf;
	if (strcmp (dp->d_name, ".." ) == 0)
	    dotdot_found = 1;
	next_free++;
	if (!(next_free % 32))
	    rotate_dash ();
    }

    if (next_free) {
	if (!dotdot_found)
	    add_dotdot_to_list (list, next_free++);
	do_sort (list, sort, next_free-1, reverse, case_sensitive);
    }
    else
	return set_zero_dir (list);
    
    mc_closedir (dirp);
    end_tree_check (NULL);
    return next_free;
}

int link_isdir (file_entry *file)
{
    struct stat b;
    
    if (S_ISLNK (file->buf.st_mode)){
	mc_stat (file->fname, &b);
	if (S_ISDIR (b.st_mode))
	    return 1;
    }
    return 0;
}
 
int if_link_is_exe (file_entry *file)
{
    struct stat b;

    if (S_ISLNK (file->buf.st_mode)){
	mc_stat (file->fname, &b);
	return is_exe (b.st_mode);
    }
    return 1;
}

static dir_list dir_copy = { 0, 0 };

static void alloc_dir_copy (int size)
{
    int i;
	    
    if (dir_copy.size < size){
	if (dir_copy.list){

	    for (i = 0; i < dir_copy.size; i++) {
		if (dir_copy.list [i].fname)
		    free (dir_copy.list [i].fname);
		if (dir_copy.list [i].cache)
		    free (dir_copy.list [i].cache);
	    }
	    free (dir_copy.list);
	    dir_copy.list = 0;
	}

	dir_copy.list = xmalloc (sizeof (file_entry) * size, "alloc_dir_copy");
	for (i = 0; i < size; i++) {
	    dir_copy.list [i].fname = 0;
	    dir_copy.list [i].cache = NULL;
	}
	dir_copy.size = size;
    }
}

/* If filter is null, then it is a match */
int do_reload_dir (dir_list *list, sortfn *sort, int count, int rev,
		   int case_sensitive, char *filter)
{
    DIR           *dirp;
    struct dirent *dp;
    int           next_free = 0;
    int           i, found, status, link_to_dir, stalled_link;
    struct stat   buf;
    int		  tmp_len;  /* For optimisation */
    int 	  dotdot_found = 0; 

    start_tree_check (NULL);
    dirp = mc_opendir (".");
    if (!dirp) {
 	clean_dir (list, count);
	return set_zero_dir (list);
    }

    alloc_dir_copy (list->size);
    for (i = 0; i < count; i++){
	dir_copy.list [i].fnamelen = list->list [i].fnamelen;
	dir_copy.list [i].fname =    list->list [i].fname;
	dir_copy.list [i].cache =    list->list [i].cache;
	dir_copy.list [i].f.marked = list->list [i].f.marked;
	dir_copy.list [i].f.link_to_dir = list->list [i].f.link_to_dir;
	dir_copy.list [i].f.stalled_link = list->list [i].f.stalled_link;
    }

    for (dp = mc_readdir (dirp); dp; dp = mc_readdir (dirp)){
	status = handle_dirent (list, filter, dp, &buf, next_free, &link_to_dir,
	    &stalled_link);
	if (status == 0)
	    continue;
	if (status == -1) {
	    mc_closedir (dirp);
	    /* Norbert (Feb 12, 1997): 
	     Just in case someone finds this memory leak:
	     -1 means big trouble (at the moment no memory left), 
	     I don't bother with further cleanup because if one gets to
	     this point he will have more problems than a few memory
	     leaks and because one 'clean_dir' would not be enough (and
	     because I don't want to spent the time to make it working, 
             IMHO it's not worthwhile). 
	    clean_dir (&dir_copy, count);
             */
	    return next_free;
	}
	
	tmp_len = NLENGTH (dp);
	for (found = i = 0; i < count; i++)
	    if (tmp_len == dir_copy.list [i].fnamelen
		&& !strcmp (dp->d_name, dir_copy.list [i].fname)){
		list->list [next_free].f.marked = dir_copy.list [i].f.marked;
		found = 1;
		break;
	    }
	
	if (!found)
	    list->list [next_free].f.marked = 0;
	
	list->list [next_free].fnamelen = tmp_len;
	list->list [next_free].fname = strdup (dp->d_name);
	list->list [next_free].cache = NULL;
	list->list [next_free].f.link_to_dir = link_to_dir;
	list->list [next_free].f.stalled_link = stalled_link;
	list->list [next_free].buf = buf;
	if (strcmp (dp->d_name, ".." ) == 0)
	    dotdot_found = 1;
	next_free++;
	if (!(next_free % 16))
	    rotate_dash ();
    }
    mc_closedir (dirp);
    end_tree_check (NULL);
    if (next_free) {
	if (!dotdot_found)
	    add_dotdot_to_list (list, next_free++);
	do_sort (list, sort, next_free-1, rev, case_sensitive);
    }
    else
	next_free = set_zero_dir (list);
    clean_dir (&dir_copy, count);
    return next_free;
}

char *sort_type_to_name (sortfn *sort_fn)
{
    int i;

    for (i = 0; i < SORT_TYPES; i++)
	if ((sortfn *) (sort_orders [i].sort_fn) == sort_fn)
	    return _(sort_orders [i].sort_name);

    return _("Unknown");
}

sortfn *sort_name_to_type (char *sname)
{
    int i;

    for (i = 0; i < SORT_TYPES; i++)
	if (strcasecmp (sort_orders [i].sort_name, sname) == 0)
	    return (sortfn *) sort_orders [i].sort_fn;

    /* default case */
    return (sortfn *) sort_name;
}


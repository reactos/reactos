#ifndef __DIR_H
#define __DIR_H

#define MIN_FILES 128
#define RESIZE_STEPS 128

typedef struct {

    /* File attributes */

    int  fnamelen;
    char *fname;
    struct stat  buf;

    /* Flags */
    struct { 
	unsigned int marked:1;		/* File marked in pane window */
	unsigned int exists:1;		/* Use for rereading file */
	unsigned int link_to_dir:1;	/* If this is a link, does it point to directory? */
	unsigned int stalled_link:1;    /* If this is a symlink and points to Charon's land */
    } f;
    char *cache;
} file_entry;

typedef struct {
    file_entry *list;
    int         size;
} dir_list;

typedef int sortfn (const void *, const void *);
int do_load_dir (dir_list *list, sortfn *sort, int reverse, int case_sensitive, char *filter);
void do_sort (dir_list *list, sortfn *sort, int top, int reverse, int case_sensitive);
dir_list *do_collect_stat (dir_list *dir, int top);
int do_reload_dir (dir_list *list, sortfn *sort, int count, int reverse, int case_sensitive, char *filter);
void clean_dir (dir_list *list, int count);
int set_zero_dir (dir_list *list);

#ifdef DIR_H_INCLUDE_HANDLE_DIRENT
int handle_dirent (dir_list *list, char *filter, struct dirent *dp,
		   struct stat *buf1, int next_free, int *link_to_dir, int *stalled_link);
int handle_path (dir_list *list, char *path, struct stat *buf1, int next_free, 
		   int *link_to_dir, int *stalled_link);
#endif

/* Sorting functions */
int unsorted   (const file_entry *a, const file_entry *b);
int sort_name  (const file_entry *a, const file_entry *b);
int sort_ext   (const file_entry *a, const file_entry *b);
int sort_time  (const file_entry *a, const file_entry *b);
int sort_atime (const file_entry *a, const file_entry *b);
int sort_ctime (const file_entry *a, const file_entry *b);
int sort_size  (const file_entry *a, const file_entry *b);
int sort_inode (const file_entry *a, const file_entry *b);
int sort_type  (const file_entry *a, const file_entry *b);
int sort_links (const file_entry *a, const file_entry *b);
int sort_nuid  (const file_entry *a, const file_entry *b);
int sort_ngid  (const file_entry *a, const file_entry *b);
int sort_owner (const file_entry *a, const file_entry *b);
int sort_group (const file_entry *a, const file_entry *b);

/* SORT_TYPES is used to build the nice dialog box entries */
#define SORT_TYPES 8

/* This is the number of sort types not available in that dialog box */
#define SORT_TYPES_EXTRA 6

/* The total, used by Tk version */
#define SORT_TYPES_TOTAL (SORT_TYPES + SORT_TYPES_EXTRA)

typedef struct {
    char    *sort_name;
    int     (*sort_fn)(const file_entry *, const file_entry *);
} sort_orders_t;

extern sort_orders_t sort_orders [SORT_TYPES_TOTAL];

int link_isdir (file_entry *);
int if_link_is_exe (file_entry *file);

extern int show_backups;
extern int show_dot_files;
extern int show_backups;
extern int mix_all_files;

char   *sort_type_to_name (sortfn *);
sortfn *sort_name_to_type (char *type);

#endif	/* __DIR_H */

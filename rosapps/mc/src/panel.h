#ifndef __PANEL_H
#define __PANEL_H

#include "dir.h"     /* file_entry */
#include "dlg.h"
#include "widget.h"	/* for history loading and saving */

#define LIST_TYPES	5

enum list_types {
    list_full,			/* Name, size, perm/date */
    list_brief,			/* Name */
    list_long,			/* Like ls -l */
    list_user,			/* User defined */
    list_icons			/* iconic display */
};

enum view_modes {
    view_listing,		/* Directory listing */
    view_info,			/* Information panel */
    view_tree,			/* Tree view */
    view_quick,			/* Quick view */
    view_nothing		/* Undefined */
};

enum panel_display_enum {
    frame_full,			/* full screen frame */
    frame_half			/* half screen frame */
};

#define is_view_special(x) ((x == view_info) || (x == view_quick))

#define J_LEFT  0
#define J_RIGHT 1

#define NORMAL		0
#define SELECTED	1
#define MARKED		2
#define MARKED_SELECTED	3
#define STATUS		5

/*
 * This describes a format item.  The parse_display_format routine parses
 * the user specified format and creates a linked list of format_e structures.
 *
 * parse_display_format computes the actual field allocations if
 * the COMPUTE_FORMAT_ALLOCATIONs define is set.  MC frontends that are
 * just interested in the parsed display format should not set this define.
 */
typedef struct format_e {
    struct format_e *next;
    int    requested_field_len;
    int    field_len;
    int    just_mode;
    int    expand;
    char   *(*string_fn)(file_entry *, int len);
    char   *title;
    char   *id;

    /* first format_e has the number of items */
    int    items;
    int    use_in_gui;
} format_e;

typedef struct {
    Widget   widget;
    dir_list dir;		/* Directory contents */
    
    int      list_type;		/* listing type (was view_type) */
    int      active;		/* If panel is currently selected */
    char     cwd [MC_MAXPATHLEN];/* Current Working Directory */
    char     lwd [MC_MAXPATHLEN];/* Last Working Directory */
    Hist     *dir_history;	/* directory history */
    char     *hist_name;	/* directory history name for history file */
    int      count;		/* Number of files in dir structure */
    int      marked;		/* Count of marked files */
    int      dirs_marked;	/* Count of marked directories */
    long int total;		/* Bytes in marked files */
    int      top_file;		/* The file showed on the top of the panel */
    int      selected;		/* Index to the selected file */
    int      reverse;		/* Show listing in reverse? */
    int      case_sensitive;    /* Listing is case sensitive? */
    int      split;		/* Split panel to allow two columns */
    int      is_panelized;	/* Flag: special filelisting, can't reload */
    int      frame_size;	/* half or full frame */
    int      icons_per_row;     /* Icon view; how many icons displayed per row */
    sortfn   *sort_type;	/* Sort type */
    char     *filter;		/* File name filter */

    int      dirty;		/* Should we redisplay the panel? */

    int	     user_mini_status;  	/* Is user_status_format used */
    char     *user_format;      	/* User format */
    char     *user_status_format[LIST_TYPES];/* User format for status line */

    format_e *format;		/* Display format */
    format_e *status_format;    /* Mini status format */

    int      format_modified;	/* If the format was changed this is set */
    
    char     *panel_name;	/* The panel name */
    struct   stat dir_stat;	/* Stat of current dir: used by execute () */

    char     *gc;
    void     *font;
    int	     item_height;
    int	     total_width;
    int	     ascent;
    int	     descent;
    
    int      searching;
    char     search_buffer [256];
   
    int      has_dir_sizes;	/* Set if directories have sizes = to du -s */

#ifdef HAVE_GNOME
    /* These are standard GtkWidgets */
	
    void *xwindow;		/* The toplevel window */
	
    void *table;
    void *list;
    void *icons;
    void *status;
    void *ministatus;

    void *filter_w;		/* A WInput* */
    void *current_dir;		/* A WInput* */
    int estimated_total;

    /* navigation buttons */
    void *back_b;
    void *fwd_b;
    void *up_b;
#endif
} WPanel;

WPanel *panel_new (char *panel_name);
void panel_set_size (WPanel *panel, int x1, int y1, int x2, int y2);
void paint_paint (WPanel *panel);
void panel_refresh (WPanel *panel);
void Xtry_to_select (WPanel *panel, char *name);

int is_a_panel (Widget *);

extern int torben_fj_mode;
extern int permission_mode;
extern int filetype_mode;
extern int show_mini_info;
extern int panel_scroll_pages;

#define selection(p) (&(p->dir.list [p->selected]))

extern int fast_reload;

extern int extra_info;

/*#define ITEMS(p) ((p)->view_type == view_brief ? (p)->lines *2 : (p)->lines)
*/
/* The return value of panel_reload */
#define CHANGED 1

#define PANEL_ISVIEW(p) (p->view_type == view_brief || \
			 p->view_type == view_full  || \
			 p->view_type == view_long  || \
			 p->view_type == view_user  || \
			 p->view_type == view_tree)

#define RP_ONLY_PAINT 0
#define RP_SETPOS 1

void set_colors (WPanel *panel);
void paint_panel (WPanel *panel);
void format_file (char *dest, WPanel *panel, int file_index, int panel_width, int attr, int isstatus);
void repaint_file (WPanel *panel, int file_index, int move, int attr, int isstatus);
void display_mini_info (WPanel *panel);
void panel_reload (WPanel *panel);
void paint_dir (WPanel *panel);
void show_dir (WPanel *panel);

/* NOTE: Have to be ifdefed for HAVE_X */
void x_panel_set_size        (int index);
void x_create_panel          (Dlg_head *h, widget_data parent, WPanel *panel);
void x_fill_panel            (WPanel *panel);
void x_adjust_top_file       (WPanel *panel);
void x_filter_changed        (WPanel *panel);
void x_add_sort_label        (WPanel *panel, int index, char *text, char *tag, void *sr);
void x_sort_label_start      (WPanel *panel);
void x_reset_sort_labels     (WPanel *panel);
void x_panel_destroy         (WPanel *panel);
void change_view             (WPanel *panel, int view_type);
void x_panel_update_marks    (WPanel *panel);

extern void paint_info_panel (WPanel *);
extern void paint_quick_view_panel (WPanel *);
void info_frame (WPanel *panel);
extern WPanel *the_info_panel;
void paint_frame (WPanel *panel);
void panel_update_contents (WPanel *panel);
void panel_update_cols (Widget *widget, int frame_size);
format_e *use_display_format (WPanel *panel, char *format, char **error, int isstatus);
char *panel_format (WPanel *panel);
char *mini_status_format (WPanel *panel);
int set_panel_formats (WPanel *p);

WPanel *get_current_panel (void);
WPanel *get_other_panel (void);

#define other_panel get_other_panel()

extern WPanel *left_panel;
extern WPanel *right_panel;
extern WPanel *current_panel;

void try_to_select (WPanel *panel, char *name);

#define DEFAULT_USER_FORMAT "half type,name,|,size,|,perm"

/* This were in main: */
void unmark_files (WPanel *panel);
void select_item (WPanel *panel);
int ITEMS (WPanel *p);
void unselect_item (WPanel *panel);

extern Hook *select_file_hook;

char *string_file_type (file_entry *fe, int len);
char *string_file_size_brief (file_entry *fe, int len);
char *string_file_permission (file_entry *fe, int len);
char *string_file_nlinks (file_entry *fe, int len);
char *string_file_owner (file_entry *fe, int len);
char *string_file_group (file_entry *fe, int len);
char *string_file_size (file_entry *fe, int len);
char *string_file_mtime (file_entry *fe, int len);
char *string_file_atime (file_entry *fe, int len);
char *string_file_ctime (file_entry *fe, int len);
char *string_file_name (file_entry *fe, int len);
char *string_space (file_entry *fe, int len);
char *string_dot (file_entry *fe, int len);
char *string_marked (file_entry *fe, int len);
char *string_file_perm_octal (file_entry *fe, int len);
char *string_inode (file_entry *fe, int len);
char *string_file_ngid (file_entry *fe, int len);
char *string_file_nuid (file_entry *fe, int len);

void file_mark (WPanel *panel, int index, int val);
void do_file_mark (WPanel *panel, int index, int val);
int  file_compute_color (int attr, file_entry *fe);
int  file_entry_color (file_entry *fe);
void do_file_mark_range (WPanel *panel, int r1, int r2);
int do_enter (WPanel *panel);

/* NOTE: Have to be ifdefed for HAVE_X */
void x_panel_select_item (WPanel *panel, int index, int val);
void x_select_item (WPanel *panel);
void x_unselect_item (WPanel *panel);
sortfn *get_sort_fn (char *name);
void update_one_panel_widget (WPanel *panel, int force_update, char *current_file);
void panel_update_marks (WPanel *panel);

void directory_history_next (WPanel * panel);
void directory_history_prev (WPanel * panel);
void directory_history_list (WPanel * panel);

#endif	/* __PANEL_H */

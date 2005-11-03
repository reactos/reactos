#ifndef __FILE_H
#define __FILE_H

enum { OP_COPY, OP_MOVE, OP_DELETE };
enum { FILE_CONT, FILE_RETRY, FILE_SKIP, FILE_ABORT };

extern int verbose;
extern int know_not_what_am_i_doing;

struct link;

int copy_file_file (char *s, char *d, int ask_overwrite);
int move_file_file (char *s, char *d);
int erase_dir (char *s);
int erase_dir_iff_empty (char *s);
int move_dir_dir (char *s, char *d);
int copy_dir_dir (char *s, char *d, int toplevel, int move_over, int delete, struct link *parent_dirs);

void create_op_win (int op, int with_eta);
void destroy_op_win (void);
void refresh_op_win (void);
int panel_operate (void *source_panel, int op, char *thedefault);
void file_mask_defaults (void);

extern int dive_into_subdirs;

/* Error reporting routines */
    /* Skip/Retry/Abort routine */
    int do_file_error (char *error);

    /* Report error with one file */
    int file_error (char *format, char *file);

    /* Report error with two files */
    int files_error (char *format, char *file1, char *file2);

    /* This one just displays buf */
    int do_file_error (char *buf);

/* Query routines */
    /* Replace existing file */
    int query_replace (char *destname, struct stat *_s_stat, struct stat *_d_stat);

    /* Query recursive delete */
    int query_recursive (char *s);

/* Callback routine for background activity */
int background_attention (int fd, void *info);
extern int background_wait;

#endif

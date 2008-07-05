#ifndef __UTIL_H
#define __UTIL_H


#include <sys/types.h>

/* String managing functions */

#if defined(SCO_FLAVOR) && defined(__GNUC__)
extern char* strdup(const char*);
#endif

int  is_printable (int c);
int  msglen (char *text, int *lines);
char *trim (char *s, char *d, int len);
char *name_quote (const char *c, int quote_percent);
char *fake_name_quote (const char *c, int quote_percent);
char *name_trunc (char *txt, int trunc_len);
char *size_trunc (long int size);
char *size_trunc_sep (long int size);
int  is_exe (mode_t mode);
char *string_perm (mode_t mode_bits);
char *strip_home_and_password(char *dir);
char *extension (char *);
char *split_extension (char *, int pad);
char *get_full_name (char *dir, char *file);
char *copy_strings (const char *first, ...);
char *concat_dir_and_file (const char *dir, const char *file);
char *unix_error_string (int error_num);
char *skip_separators (char *s);
char *skip_numbers (char *s);
char *strip_ctrl_codes (char *s);
char *convert_controls (char *s);
void wipe_password (char *passwd);
char *reverse_string (char *string);
char *resolve_symlinks (char *path);
char *diff_two_paths (char *first, char *second);
int  set_nonblocking (int fd);

#ifndef HAVE_STRCASECMP
int strcasecmp (const char *s, const char *d);
#endif

char *x_basename (char *s);

extern int align_extensions;
#ifndef HAVE_MAD
void *do_xmalloc (int);
#define xmalloc(a,b) do_xmalloc (a)
#endif

/* Profile managing functions */
int set_int (char *, char *, int);
int get_int (char *, char *, int);

char *load_file (char *filename);

#include "fs.h"
#ifndef S_ISLNK
#   define S_ISLNK(x) (((x) & S_IFLNK) == S_IFLNK)
#endif

#ifndef S_ISSOCK
#   ifdef S_IFSOCK
#       define S_ISSOCK(x) (((x) & S_IFSOCK) == S_IFSOCK)
#   else
#       define S_ISSOCK(x) 0
#   endif
#endif

/* uid/gid managing */
typedef struct user_in_groups{
    struct user_in_groups *next;
    int gid;
} user_in_groups;

void init_groups (void);
void delete_groups (void);
int get_user_rights (struct stat *buf);

void init_uid_gid_cache (void);
char *get_group (int);
char *get_owner (int);

char *file_date (time_t);
char *file_date_pck (time_t);
int exist_file (char *name);

/* Returns a copy of *s until a \n is found and is below top */
char *extract_line (char *s, char *top);
char *_icase_search (char *text, char *data, int *lng);
#define icase_search(T,D) _icase_search((T), (D), NULL)

/* Matching */
enum { match_file, match_normal };
extern int easy_patterns;
char *convert_pattern (char *pattern, int match_type, int do_group);
int regexp_match (char *pattern, char *string, int match_type);

/* Error pipes */
void open_error_pipe (void);
void check_error_pipe (void);
void close_error_pipe (int error, char *text);

/* Process spawning */
void my_putenv (char*, char*);
#define EXECUTE_INTERNAL   1
#define EXECUTE_TEMPFILE   2
#define EXECUTE_AS_SHELL   4
int my_system (int flags, const char *shell, const char *command);
int my_system_get_child_pid (int flags, const char *shell, const char *command, pid_t *pid);
void save_stop_handler (void);
extern struct sigaction startup_handler;

/* Tilde expansion */
char *tilde_expand (char *);

/* Pathname canonicalization */
char *canonicalize_pathname (char *);

/* Misc Unix functions */
long blocks2kilos (int blocks, int bsize);
char *get_current_wd (char *buffer, int size);
int my_mkdir (char *s, mode_t mode);
int my_rmdir (char *s);

/* Filesystem status */
struct my_statfs {
    int type;
    char *typename;
    char *mpoint;
    char *device;
    unsigned int avail;
    unsigned int total;
    unsigned int nfree;
    unsigned int nodes;
};

void init_my_statfs (void);
void my_statfs (struct my_statfs *myfs_stats, char *path);

/* Rotating dash routines */
void use_dash (int flag); /* Disable/Enable rotate_dash routines */
void rotate_dash (void);
void remove_dash (void);

extern char app_text [];

enum {
	ISGUNZIPABLE_GUNZIP,
	ISGUNZIPABLE_BZIP,
	ISGUNZIPABLE_BZIP2
};

long int is_gunzipable (int fd, int *type);
char *decompress_command (int type);
void decompress_command_and_arg (int type, char **cmd, char **flags);

int mc_doublepopen (int inhandle, int inlen, pid_t *tp, char *command, ...);
int mc_doublepclose (int pipehandle, pid_t pid);

/* Hook functions */

typedef struct hook {
    void (*hook_fn)(void *);
    void *hook_data;
    struct hook *next;
} Hook;

void add_hook (Hook **hook_list, void (*hook_fn)(void *), void *data);
void execute_hooks (Hook *hook_list);
void delete_hook (Hook **hook_list, void (*hook_fn)(void *));
int hook_present (Hook *hook_list, void (*hook_fn)(void *));

/* message stubs: used by those routines that may output something during the panel_operate process */
void message_1s (int flags, char *title, char *str1);
void message_2s (int flags, char *title, char *str1, char *str2);
void message_3s (int flags, char *title, char *str1, char *str2, const char *str3);
void message_1s1d (int flags, char *title, char *str, int d);
void tell_parent (int msg);
int max_open_files (void);

#ifdef OS2_NT
#    define PATH_SEP '\\'
#    define PATH_SEP_STR "\\"
#    define PATH_ENV_SEP ';'
#    define OS_SORT_CASE_SENSITIVE_DEFAULT 0
#    define STRCOMP stricmp
#    define STRNCOMP strnicmp
#    define MC_ARCH_FLAGS REG_ICASE
     char *get_default_shell (void);
     char *get_default_editor (void);
     int lstat (const char* pathname, struct stat *buffer);
#else
#    define PATH_SEP '/'
#    define PATH_SEP_STR "/"
#    define PATH_ENV_SEP ':'
#    define get_default_editor() "vi"
#    define OS_SORT_CASE_SENSITIVE_DEFAULT 1
#    define STRCOMP strcmp
#    define STRNCOMP strncmp
#    define MC_ARCH_FLAGS 0
#endif

#include "i18n.h"

/* taken from regex.c: */
/* Jim Meyering writes:

   "... Some ctype macros are valid only for character codes that
   isascii says are ASCII (SGI's IRIX-4.0.5 is one such system --when
   using /bin/cc or gcc but without giving an ansi option).  So, all
   ctype uses should be through macros like ISPRINT...  If
   STDC_HEADERS is defined, then autoconf has verified that the ctype
   macros don't need to be guarded with references to isascii. ...
   Defining isascii to 1 should let any compiler worth its salt
   eliminate the && through constant folding."  */

#if defined (STDC_HEADERS) || (!defined (isascii) && !defined (HAVE_ISASCII))
#define ISASCII(c) 1
#else
#define ISASCII(c) isascii(c)
#endif

#endif

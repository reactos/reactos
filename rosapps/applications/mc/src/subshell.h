#ifndef __SUBSHELL_H
#define __SUBSHELL_H

/* Used to distinguish between a normal MC termination and */
/* one caused by typing `exit' or `logout' in the subshell */
#define SUBSHELL_EXIT 128

#ifdef HAVE_SUBSHELL_SUPPORT

/* If using a subshell for evaluating commands this is true */
extern int use_subshell;

/* File descriptor of the pseudoterminal used by the subshell */
extern int subshell_pty;

/* If true, the child forked in init_subshell will wait in a loop to be attached by gdb */
extern int debug_subshell;

/* The key to switch back to MC from the subshell */
extern char subshell_switch_key;

/* State of the subshell; see subshell.c for an explanation */
enum subshell_state_enum {INACTIVE, ACTIVE, RUNNING_COMMAND};
extern enum subshell_state_enum subshell_state;

/* Holds the latest prompt captured from the subshell */
extern char *subshell_prompt;

/* For the `how' argument to various functions */
enum {QUIETLY, VISIBLY};

/* Exported functions */
void init_subshell (void);
int invoke_subshell (const char *command, int how, char **new_dir);
int read_subshell_prompt (int how);
void resize_subshell (void);
int exit_subshell (void);
void do_subshell_chdir (char *command, int update_prompt, int reset_prompt);
void subshell_get_console_attributes (void);
void sigchld_handler (int sig);

#else
#define use_subshell 0
#endif /* not HAVE_SUBSHELL_SUPPORT */

#endif /* __SUBSHELL_H */

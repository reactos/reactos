#ifndef GETLINE_H
#define GETLINE_H

/* unix systems can #define POSIX to use termios, otherwise 
 * the bsd or sysv interface will be used 
 */

#define GL_BUF_SIZE 1024

/* Result codes available for gl_get_result() */
#define GL_OK 0			/* Valid line of input entered */
#define GL_EOF (-1)		/* End of input */
#define GL_INTERRUPT (-2)	/* User hit Ctrl+C */

typedef size_t (*gl_strwidth_proc)(char *);
typedef int (*gl_in_hook_proc)(char *);
typedef int (*gl_out_hook_proc)(char *);
typedef int (*gl_tab_hook_proc)(char *, int, int *, size_t);
typedef size_t (*gl_strlen_proc)(const char *);
typedef char * (*gl_tab_completion_proc)(const char *, int);

char *getline(char *);		/* read a line of input */
void gl_setwidth(int);		/* specify width of screen */
void gl_setheight(int);		/* specify height of screen */
void gl_histadd(char *);		/* adds entries to hist */
void gl_strwidth(gl_strwidth_proc);	/* to bind gl_strlen */
void gl_tab_completion(gl_tab_completion_proc);
char *gl_local_filename_completion_proc(const char *, int);
void gl_set_home_dir(const char *homedir);
void gl_histsavefile(const char *const path);
void gl_histloadfile(const char *const path);
char *gl_getpass(const char *const prompt, char *const pass, int dsize);
int gl_get_result(void);

#ifndef _getline_c_

extern gl_in_hook_proc gl_in_hook;
extern gl_out_hook_proc gl_out_hook;
extern gl_tab_hook_proc gl_tab_hook;
extern gl_strlen_proc gl_strlen;
extern gl_tab_completion_proc gl_completion_proc;
extern int gl_filename_quoting_desired;
extern const char *gl_filename_quote_characters;
extern int gl_ellipses_during_completion;
extern int gl_completion_exact_match_extra_char;
extern char gl_buf[GL_BUF_SIZE];

#endif /* ! _getline_c_ */

#endif /* GETLINE_H */

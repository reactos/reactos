/* Based on: "$Id: getline.c,v 1.1 2001/12/30 06:30:21 sedwards Exp $"; */
static const char copyright[] = "getline:  Copyright (C) 1991, 1992, 1993, Chris Thewalt";

/*
 * Copyright (C) 1991, 1992, 1993 by Chris Thewalt (thewalt@ce.berkeley.edu)
 *
 * Permission to use, copy, modify, and distribute this software 
 * for any purpose and without fee is hereby granted, provided
 * that the above copyright notices appear in all copies and that both the
 * copyright notice and this permission notice appear in supporting
 * documentation.  This software is provided "as is" without express or
 * implied warranty.
 *
 * Thanks to the following people who have provided enhancements and fixes:
 *   Ron Ueberschaer, Christoph Keller, Scott Schwartz, Steven List,
 *   DaviD W. Sanderson, Goran Bostrom, Michael Gleason, Glenn Kasten,
 *   Edin Hodzic, Eric J Bivona, Kai Uwe Rommel, Danny Quah, Ulrich Betzler
 */

/*
 * Note:  This version has been updated by Mike Gleason <mgleason@ncftp.com>
 */

#if defined(WIN32) || defined(_WINDOWS)
#	include <windows.h>
#	include <sys/types.h>
#	include <sys/stat.h>
#	include <errno.h>
#	include <conio.h>
#	include <io.h>
#	include <fcntl.h>
#	define strcasecmp stricmp
#	define strncasecmp strnicmp
#	define sleep(a) Sleep(a * 1000)
#	ifndef S_ISREG
#		define S_ISREG(m)      (((m) & _S_IFMT) == _S_IFREG)
#		define S_ISDIR(m)      (((m) & _S_IFMT) == _S_IFDIR)
#	endif
#	ifndef open
#		define open _open
#		define write _write
#		define read _read
#		define close _close
#		define lseek _lseek
#		define stat _stat
#		define lstat _stat
#		define fstat _fstat
#		define dup _dup
#		define utime _utime
#		define utimbuf _utimbuf
#	endif
#	ifndef unlink
#		define unlink remove
#	endif
#	define NO_SIGNALS 1
#	define LOCAL_PATH_DELIM '\\'
#	define LOCAL_PATH_DELIM_STR "\\"
#	define LOCAL_PATH_ALTDELIM '/'
#	define IsLocalPathDelim(c) ((c == LOCAL_PATH_DELIM) || (c == LOCAL_PATH_ALTDELIM))
#	define UNC_PATH_PREFIX "\\\\"
#	define IsUNCPrefixed(s) (IsLocalPathDelim(s[0]) && IsLocalPathDelim(s[1]))
#	define __windows__ 1
#else
#	ifndef __unix__
#		define __unix__ 1
#	endif
#	if defined(AIX) || defined(_AIX)
#		define _ALL_SOURCE 1
#	endif
#	if defined(HAVE_CONFIG_H)
#		include <config.h>
#	else
#		/* guess */
#		define HAVE_TERMIOS_H 1
#		define HAVE_UNISTD_H 1
#	endif
#	ifdef HAVE_UNISTD_H
#		include <unistd.h>
#	endif
#	include <sys/types.h>
#	include <sys/time.h>
#	include <sys/stat.h>
#	ifdef CAN_USE_SYS_SELECT_H
#		include <sys/select.h>
#	endif
#	include <fcntl.h>
#	include <errno.h>
#	include <dirent.h>
#	include <pwd.h>
#	ifdef HAVE_TERMIOS_H		/* use HAVE_TERMIOS_H interface */
#		include <termios.h>
		struct termios  new_termios, old_termios;
#	else /* not HAVE_TERMIOS_H */
#		include <sys/ioctl.h>
#		ifdef TIOCSETN		/* use BSD interface */
#			include <sgtty.h>
			struct sgttyb   new_tty, old_tty;
			struct tchars   tch;
			struct ltchars  ltch;
#		else			/* use SYSV interface */
#			include <termio.h>
			struct termio   new_termio, old_termio;
#		endif /* TIOCSETN */
#	endif /* HAVE_TERMIOS_H */
#	define LOCAL_PATH_DELIM '/'
#	define LOCAL_PATH_DELIM_STR "/"
#	define _StrFindLocalPathDelim(a) strchr(a, LOCAL_PATH_DELIM)
#	define _StrRFindLocalPathDelim(a) strrchr(a, LOCAL_PATH_DELIM)
#	define IsLocalPathDelim(c) (c == LOCAL_PATH_DELIM)
#endif

/********************* C library headers ********************************/

#include <stdio.h>
#include <string.h>
#ifdef HAVE_STRINGS_H
#	include <strings.h>
#endif
#include <stdlib.h>
#include <ctype.h>
#include <signal.h>

#define _getline_c_ 1
#include "getline.h"

static int gl_tab(char *buf, int offset, int *loc, size_t bufsize);

/******************** external interface *********************************/

gl_in_hook_proc gl_in_hook = 0;
gl_out_hook_proc gl_out_hook = 0;
gl_tab_hook_proc gl_tab_hook = gl_tab;
gl_strlen_proc gl_strlen = (gl_strlen_proc) strlen;
gl_tab_completion_proc gl_completion_proc = 0;
int gl_filename_quoting_desired = -1;	/* default to unspecified */
const char *gl_filename_quote_characters = " \t*?<>|;&()[]$`";
int gl_ellipses_during_completion = 1;
int gl_completion_exact_match_extra_char;
char gl_buf[GL_BUF_SIZE];       /* input buffer */

/******************** internal interface *********************************/


static int      gl_init_done = -1;	/* terminal mode flag  */
static int      gl_termw = 80;		/* actual terminal width */
static int      gl_termh = 24;		/* actual terminal height */
static int      gl_scroll = 27;		/* width of EOL scrolling region */
static int      gl_width = 0;		/* net size available for input */
static int      gl_extent = 0;		/* how far to redraw, 0 means all */
static int      gl_overwrite = 0;	/* overwrite mode */
static int      gl_pos = 0, gl_cnt = 0; /* position and size of input */
static char     gl_killbuf[GL_BUF_SIZE]=""; /* killed text */
static const char *gl_prompt;		/* to save the prompt string */
static char     gl_intrc = 0;		/* keyboard SIGINT char */
static char     gl_quitc = 0;		/* keyboard SIGQUIT char */
static char     gl_suspc = 0;		/* keyboard SIGTSTP char */
static char     gl_dsuspc = 0;		/* delayed SIGTSTP char */
static int      gl_search_mode = 0;	/* search mode flag */
static char   **gl_matchlist = 0;
static char    *gl_home_dir = NULL;
static int      gl_vi_preferred = -1;
static int      gl_vi_mode = 0;
static int	gl_result = GL_OK;

static void     gl_init(void);		/* prepare to edit a line */
static void     gl_cleanup(void);	/* to undo gl_init */
static void     gl_char_init(void);	/* get ready for no echo input */
static void     gl_char_cleanup(void);	/* undo gl_char_init */
					/* returns printable prompt width */

static void     gl_addchar(int c);	/* install specified char */
static void     gl_del(int loc, int);	/* del, either left (-1) or cur (0) */
static void     gl_error(const char *const buf);	/* write error msg and die */
static void     gl_fixup(const char *prompt, int change, int cursor);		/* fixup state variables and screen */
static int      gl_getc(void);		/* read one char from terminal */
static int      gl_getcx(int);		/* read one char from terminal, if available before timeout */
static void     gl_kill(int pos);	/* delete to EOL */
static void     gl_newline(void);	/* handle \n or \r */
static void     gl_putc(int c);		/* write one char to terminal */
static void     gl_puts(const char *const buf);	/* write a line to terminal */
static void     gl_redraw(void);	/* issue \n and redraw all */
static void     gl_transpose(void);	/* transpose two chars */
static void     gl_yank(void);		/* yank killed text */
static void     gl_word(int direction);	/* move a word */
static void     gl_killword(int direction);

static void     hist_init(void);	/* initializes hist pointers */
static char    *hist_next(void);	/* return ptr to next item */
static char    *hist_prev(void);	/* return ptr to prev item */
static char    *hist_save(char *p);	/* makes copy of a string, without NL */

static void     search_addchar(int c);	/* increment search string */
static void     search_term(void);	/* reset with current contents */
static void     search_back(int new_search);		/* look back for current string */
static void     search_forw(int new_search);		/* look forw for current string */
static void     gl_beep(void);          /* try to play a system beep sound */

static int      gl_do_tab_completion(char *buf, int *loc, size_t bufsize, int tabtab);

/************************ nonportable part *********************************/

#ifdef MSDOS
#include <bios.h>
#endif

static void
gl_char_init(void)			/* turn off input echo */
{
#ifdef __unix__
#	ifdef HAVE_TERMIOS_H		/* Use POSIX */
		if (tcgetattr(0, &old_termios) == 0) {
			gl_intrc = old_termios.c_cc[VINTR];
			gl_quitc = old_termios.c_cc[VQUIT];
#		ifdef VSUSP
			gl_suspc = old_termios.c_cc[VSUSP];
#		endif
#		ifdef VDSUSP
			gl_dsuspc = old_termios.c_cc[VDSUSP];
#		endif
		}
		new_termios = old_termios;
		new_termios.c_iflag &= ~(BRKINT|ISTRIP|IXON|IXOFF);
		new_termios.c_iflag |= (IGNBRK|IGNPAR);
		new_termios.c_lflag &= ~(ICANON|ISIG|IEXTEN|ECHO);
		new_termios.c_cc[VMIN] = 1;
		new_termios.c_cc[VTIME] = 0;
		tcsetattr(0, TCSANOW, &new_termios);
#	elif defined(TIOCSETN)		/* BSD */
		if (ioctl(0, TIOCGETC, &tch) == 0) {
			gl_intrc = tch.t_intrc;
			gl_quitc = tch.t_quitc;
		}
		ioctl(0, TIOCGLTC, &ltch);
		gl_suspc = ltch.t_suspc;
		gl_dsuspc = ltch.t_dsuspc;
		ioctl(0, TIOCGETP, &old_tty);
		new_tty = old_tty;
		new_tty.sg_flags |= RAW;
		new_tty.sg_flags &= ~ECHO;
		ioctl(0, TIOCSETN, &new_tty);
#	else				/* SYSV */
		if (ioctl(0, TCGETA, &old_termio) == 0) {
			gl_intrc = old_termio.c_cc[VINTR];
			gl_quitc = old_termio.c_cc[VQUIT];
		}
		new_termio = old_termio;
		new_termio.c_iflag &= ~(BRKINT|ISTRIP|IXON|IXOFF);
		new_termio.c_iflag |= (IGNBRK|IGNPAR);
		new_termio.c_lflag &= ~(ICANON|ISIG|ECHO);
		new_termio.c_cc[VMIN] = 1;
		new_termio.c_cc[VTIME] = 0;
		ioctl(0, TCSETA, &new_termio);
#	endif
#endif /* __unix__ */
}

static void
gl_char_cleanup(void)		/* undo effects of gl_char_init */
{
#ifdef __unix__
#	ifdef HAVE_TERMIOS_H 
		tcsetattr(0, TCSANOW, &old_termios);
#	elif defined(TIOCSETN)		/* BSD */
		ioctl(0, TIOCSETN, &old_tty);
#	else			/* SYSV */
		ioctl(0, TCSETA, &old_termio);
#	endif
#endif /* __unix__ */
}



int
gl_get_result(void)
{
	return (gl_result);
}	/* gl_get_result */




#if defined(MSDOS) || defined(__windows__)

#define K_UP				0x48
#define K_DOWN				0x50
#define K_LEFT				0x4B
#define K_RIGHT				0x4D
#define K_DELETE			0x53
#define K_INSERT			0x52
#define K_HOME				0x47
#define K_END				0x4F
#define K_PGUP				0x49
#define K_PGDN				0x51

int pc_keymap(int c)
{
    switch (c) {
	case K_UP:
	case K_PGUP:
		c = 16;		/* up -> ^P */
        break;
	case K_DOWN:
	case K_PGDN:
		c = 14;		/* down -> ^N */
        break;
    case K_LEFT:
		c = 2;		/* left -> ^B */
        break;
    case K_RIGHT:
		c = 6;		/* right -> ^F */
        break;
	case K_END:
		c = 5;		/* end -> ^E */
		break;
	case K_HOME:
		c = 1;		/* home -> ^A */
		break;
	case K_INSERT:
		c = 15;		/* insert -> ^O */
		break;
	case K_DELETE:
		c = 4;		/* del -> ^D */
		break;
    default:
		c = 0;    /* make it garbage */
    }
    return c;
}
#endif /* defined(MSDOS) || defined(__windows__) */

static int
gl_getc(void)
/* get a character without echoing it to screen */
{
    int             c;
#ifdef __unix__
    char            ch;
#endif

#ifdef __unix__
    ch = '\0';
    while ((c = (int) read(0, &ch, 1)) == -1) {
	if (errno != EINTR)
	    break;
    }
    if (c != (-1))
	    c = (int) ch;
#endif	/* __unix__ */
#ifdef MSDOS
    c = _bios_keybrd(_NKEYBRD_READ);
    if ((c & 0377) == 224) {
	c = pc_keymap((c >> 8) & 0377);
    } else {
	c &= 0377;
    }
#endif /* MSDOS */
#ifdef __windows__
	c = (int) _getch();
	if ((c == 0) || (c == 0xE0)) {
		/* Read key code */
		c = (int) _getch();
		c = pc_keymap(c);
	} else if (c == '\r') {
		/* Note: we only get \r from the console,
		 * and not a matching \n.
		 */
		c = '\n';
	}
#endif
    return c;
}



#ifdef __unix__

static int
gl_getcx(int tlen)
/* Get a character without echoing it to screen, timing out
 * after tlen tenths of a second.
 */
{
	int c, result;
	char ch;
	fd_set ss;
	struct timeval tv;

	for (errno = 0;;) {
		FD_ZERO(&ss);
		FD_SET(0, &ss);		/* set STDIN_FILENO */
		tv.tv_sec = tlen / 10;
		tv.tv_usec = (tlen % 10) * 100000L;
		result = select(1, &ss, NULL, NULL, &tv);
		if (result == 1) {
			/* ready */
			break;
		} else if (result == 0) {
			errno = ETIMEDOUT;
			return (-2);
		} else if (errno != EINTR) {
			return (-1);
		}
	}

	for (errno = 0;;) {
		c = (int) read(0, &ch, 1);
		if (c == 1)
			return ((int) ch);
		if (errno != EINTR)
			break;
	}

	return (-1);
}	/* gl_getcx */

#endif	/* __unix__ */




#ifdef __windows__

static int
gl_getcx(int tlen)
{	
	int i, c;

	c = (-2);
	tlen -= 2;	/* Adjust for 200ms overhead */
	if (tlen < 1)
		tlen = 1;
	for (i=0; i<tlen; i++) {
		if (_kbhit()) {
			c = (int) _getch();
			if ((c == 0) || (c == 0xE0)) {
				/* Read key code */
				c = (int) _getch();
				c = pc_keymap(c);
				break;
			}
		}
		(void) SleepEx((DWORD) (tlen * 100), FALSE);
	}
	return (c);
}	/* gl_getcx */

#endif	/* __windows__ */




static void
gl_putc(int c)
{
    char   ch = (char) (unsigned char) c;

    write(1, &ch, 1);
    if (ch == '\n') {
	ch = '\r';
        write(1, &ch, 1);	/* RAW mode needs '\r', does not hurt */
    }
}

/******************** fairly portable part *********************************/

static void
gl_puts(const char *const buf)
{
    int len; 
    
    if (buf) {
        len = (int) strlen(buf);
        write(1, buf, len);
    }
}

static void
gl_error(const char *const buf)
{
    int len = (int) strlen(buf);

    gl_cleanup();
    write(2, buf, len);
    exit(1);
}

static void
gl_init(void)
/* set up variables and terminal */
{
    const char *cp;
    int w;

    if (gl_init_done < 0) {		/* -1 only on startup */
	cp = (const char *) getenv("COLUMNS");
	if (cp != NULL) {
	    w = atoi(cp);
	    if (w > 20)
	        gl_setwidth(w);
	}
	cp = (const char *) getenv("ROWS");
	if (cp != NULL) {
	    w = atoi(cp);
	    if (w > 10)
	        gl_setheight(w);
	}
        hist_init();
    }
    if (isatty(0) == 0 || isatty(1) == 0)
	gl_error("\n*** Error: getline(): not interactive, use stdio.\n");
    gl_char_init();
    gl_init_done = 1;
}

static void
gl_cleanup(void)
/* undo effects of gl_init, as necessary */
{
    if (gl_init_done > 0)
        gl_char_cleanup();
    gl_init_done = 0;
#ifdef __windows__
	Sleep(40);
	FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));
#endif
}


static void
gl_check_inputrc_for_vi(void)
{
	FILE *fp;
	char path[256];

	/* If the user has a ~/.inputrc file,
	 * check it to see if it has a line like
	 * "set editing-mode vi".  If it does,
	 * we know that the user wants vi
	 * emulation rather than emacs.  If the
	 * file doesn't exist, it's no big
	 * deal since we can also check the
	 * $EDITOR environment variable.
	 */
	gl_set_home_dir(NULL);
	if (gl_home_dir == NULL)
		return;

#ifdef HAVE_SNPRINTF
	snprintf(path, sizeof(path), "%s/%s", gl_home_dir, ".inputrc");
#else
	if (sizeof(path) >= (strlen(gl_home_dir) + strlen("/.inputrc")))
		return;

	sprintf(path, "%s%s", gl_home_dir, "/.inputrc");
#endif

	fp = fopen(
		path,
#if defined(__windows__) || defined(MSDOS)
		"rt"
#else
		"r"
#endif
	);

	if (fp == NULL)
		return;

	while (fgets(path, sizeof(path) - 1, fp) != NULL) {
		if ((strstr(path, "editing-mode") != NULL) && (strstr(path, "vi") != NULL)) {
			gl_vi_preferred = 1;
			break;
		}
	}

	(void) fclose(fp);
}	/* gl_check_inputrc_for_vi */



void
gl_setwidth(int w)
{
    if (w > 250)
    	w = 250;
    if (w > 20) {
	gl_termw = w;
	gl_scroll = w / 3;
    } else {
	gl_error("\n*** Error: minimum screen width is 21\n");
    }
}	/* gl_setwidth */



void
gl_setheight(int w)
{
    if (w > 10) {
	gl_termh = w;
    } else {
	gl_error("\n*** Error: minimum screen height is 10\n");
    }
}	/* gl_setheight */




char *
getline(char *prompt)
{
    int             c, loc, tmp, lastch;
    int vi_count, count;
    int vi_delete;
    char vi_countbuf[32];
    char *cp;

#ifdef __unix__
    int	            sig;
#endif

    /* We'll change the result code only if something happens later. */
    gl_result = GL_OK;

	/* Even if it appears that "vi" is preferred, we
	 * don't start in gl_vi_mode.  They need to hit
	 * ESC to go into vi command mode.
	 */
	gl_vi_mode = 0;
	vi_count = 0;
	vi_delete = 0;
	if (gl_vi_preferred < 0) {
		gl_vi_preferred = 0;
		cp = (char *) getenv("EDITOR");
		if (cp != NULL)
			gl_vi_preferred = (strstr(cp, "vi") != NULL);
		if (gl_vi_preferred == 0)
			gl_check_inputrc_for_vi();
	}

    gl_init();	
    gl_prompt = (prompt)? prompt : "";
    gl_buf[0] = 0;
    if (gl_in_hook)
	gl_in_hook(gl_buf);
    gl_fixup(gl_prompt, -2, GL_BUF_SIZE);
    lastch = 0;

#ifdef __windows__
	FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));
#endif

    while ((c = gl_getc()) != (-1)) {
	gl_extent = 0;  	/* reset to full extent */
	/* Note: \n may or may not be considered printable */
	if ((c != '\t') && ((isprint(c) != 0) || ((c & 0x80) != 0))) {
	    if (gl_vi_mode > 0) {
	    	/* "vi" emulation -- far from perfect,
		 * but reasonably functional.
		 */
vi:
		for (count = 0; ; ) {
			if (isdigit(c)) {
				if (vi_countbuf[sizeof(vi_countbuf) - 2] == '\0')
					vi_countbuf[strlen(vi_countbuf)] = (char) c;
			} else if (vi_countbuf[0] != '\0') {
				vi_count = atoi(vi_countbuf);
				memset(vi_countbuf, 0, sizeof(vi_countbuf));
			}
			switch (c) {
				case 'b':
					gl_word(-1);
					break;
				case 'w':
					if (vi_delete) {
						gl_killword(1);
					} else {
						gl_word(1);
					}
					break;
				case 'h':	/* left */
					if (vi_delete) {
						if (gl_pos > 0) {
	      						gl_fixup(gl_prompt, -1, gl_pos-1);
						    	gl_del(0, 1);
						}
					} else {
	      					gl_fixup(gl_prompt, -1, gl_pos-1);
					}
					break;
				case ' ':
				case 'l':	/* right */
					if (vi_delete) {
						gl_del(0, 1);
					} else {
						gl_fixup(gl_prompt, -1, gl_pos+1);
					}
					break;
				case 'k':	/* up */
					strcpy(gl_buf, hist_prev());
					if (gl_in_hook)
					    gl_in_hook(gl_buf);
					gl_fixup(gl_prompt, 0, GL_BUF_SIZE);
					break;
				case 'j':	/* down */
					strcpy(gl_buf, hist_next());
					if (gl_in_hook)
					    gl_in_hook(gl_buf);
					gl_fixup(gl_prompt, 0, GL_BUF_SIZE);
					break;
				case 'd':
					if (vi_delete == 1) {
	      					gl_kill(0);
						vi_count = 1;
						vi_delete = 0;
						gl_vi_mode = 0;
						goto vi_break;
					}
					vi_delete = 1;
					goto vi_break;
				case '^':	/* start of line */
					if (vi_delete) {
						vi_count = gl_pos;
						gl_fixup(gl_prompt, -1, 0);
						for (c = 0; c < vi_count; c++) {
							if (gl_cnt > 0)
								gl_del(0, 0);
						}
						vi_count = 1;
						vi_delete = 0;
					} else {
						gl_fixup(gl_prompt, -1, 0);
					}
					break;
				case '$':	/* end of line */
					if (vi_delete) {
	      					gl_kill(gl_pos);
					} else {
						loc = (int) strlen(gl_buf);
						if (loc > 1)
							loc--;
						gl_fixup(gl_prompt, -1, loc);
					}
					break;
				case 'p':	/* paste after */
					gl_fixup(gl_prompt, -1, gl_pos+1);
	      				gl_yank();
					break;
				case 'P':	/* paste before */
	      				gl_yank();
					break;
				case 'r':	/* replace character */
					gl_buf[gl_pos] = (char) gl_getc();
					gl_fixup(gl_prompt, gl_pos, gl_pos);
					vi_count = 1;
					break;
				case 'R':
					gl_overwrite = 1;
					gl_vi_mode = 0;
					break;
				case 'i':
				case 'I':
					gl_overwrite = 0;
					gl_vi_mode = 0;
					break;
				case 'o':
				case 'O':
				case 'a':
				case 'A':
					gl_overwrite = 0;
					gl_fixup(gl_prompt, -1, gl_pos+1);
					gl_vi_mode = 0;
					break;
			}
			count++;
			if (count >= vi_count)
				break;
		}
		vi_count = 1;
		vi_delete = 0;
vi_break:
		continue;
	    } else if (gl_search_mode) {
	       search_addchar(c);
	    } else {
	       gl_addchar(c);
	    }
	} else {
	    if (gl_search_mode) {
	        if (c == '\033' || c == '\016' || c == '\020') {
	            search_term();
	            c = 0;     		/* ignore the character */
		} else if (c == '\010' || c == '\177') {
		    search_addchar(-1); /* unwind search string */
		    c = 0;
		} else if (c != '\022' && c != '\023') {
		    search_term();	/* terminate and handle char */
		}
	    }
	    switch (c) {
	      case '\n': case '\r': 			/* newline */
		gl_newline();
		gl_cleanup();
		return gl_buf;
	      case '\001': gl_fixup(gl_prompt, -1, 0);		/* ^A */
		break;
	      case '\002': gl_fixup(gl_prompt, -1, gl_pos-1);	/* ^B */
		break;
	      case '\004':					/* ^D */
		if (gl_cnt == 0) {
		    gl_buf[0] = 0;
		    gl_cleanup();
		    gl_putc('\n');
		    gl_result = GL_EOF;
		    return gl_buf;
		} else {
		    gl_del(0, 1);
		}
		break;
	      case '\005': gl_fixup(gl_prompt, -1, gl_cnt);	/* ^E */
		break;
	      case '\006': gl_fixup(gl_prompt, -1, gl_pos+1);	/* ^F */
		break;
	      case '\010': case '\177': gl_del(-1, 0);	/* ^H and DEL */
		break;
	      case '\t':        				/* TAB */
	        if (gl_completion_proc) {
		    tmp = gl_pos;
		    gl_buf[sizeof(gl_buf) - 1] = '\0';
	            loc = gl_do_tab_completion(gl_buf, &tmp, sizeof(gl_buf), (lastch == '\t'));
		    gl_buf[sizeof(gl_buf) - 1] = '\0';
	            if (loc >= 0 || tmp != gl_pos)
	                gl_fixup(gl_prompt, /* loc */ -2, tmp);
		    if (lastch == '\t') {
			    c = 0;
			    lastch = 0;
		    }
		} else if (gl_tab_hook) {
		    tmp = gl_pos;
		    gl_buf[sizeof(gl_buf) - 1] = '\0';
	            loc = gl_tab_hook(gl_buf, (int) gl_strlen(gl_prompt), &tmp, sizeof(gl_buf));
		    gl_buf[sizeof(gl_buf) - 1] = '\0';
	            if (loc >= 0 || tmp != gl_pos)
	                gl_fixup(gl_prompt, loc, tmp);
                }
		break;
	      case '\013': gl_kill(gl_pos);			/* ^K */
		break;
	      case '\014': gl_redraw();				/* ^L */
		break;
	      case '\016': 					/* ^N */
		strcpy(gl_buf, hist_next());
                if (gl_in_hook)
	            gl_in_hook(gl_buf);
		gl_fixup(gl_prompt, 0, GL_BUF_SIZE);
		break;
	      case '\017': gl_overwrite = !gl_overwrite;       	/* ^O */
		break;
	      case '\020': 					/* ^P */
		strcpy(gl_buf, hist_prev());
                if (gl_in_hook)
	            gl_in_hook(gl_buf);
		gl_fixup(gl_prompt, 0, GL_BUF_SIZE);
		break;
	      case '\022': search_back(1);			/* ^R */
		break;
	      case '\023': search_forw(1);			/* ^S */
		break;
	      case '\024': gl_transpose();			/* ^T */
		break;
              case '\025': gl_kill(0);				/* ^U */
		break;
              case '\027': gl_killword(-1);			/* ^W */
		break;
	      case '\031': gl_yank();				/* ^Y */
		break;
	      case '\033':				/* ansi arrow keys */
		c = gl_getcx(3);
		if ((c == '[') || (c == 'O')) {
ansi:
		    switch(c = gl_getc()) {
		      case 'A':             			/* up */
		        strcpy(gl_buf, hist_prev());
                        if (gl_in_hook)
	                    gl_in_hook(gl_buf);
		        gl_fixup(gl_prompt, 0, GL_BUF_SIZE);
		        break;
		      case 'B':                         	/* down */
		        strcpy(gl_buf, hist_next());
                        if (gl_in_hook)
	                    gl_in_hook(gl_buf);
		        gl_fixup(gl_prompt, 0, GL_BUF_SIZE);
		        break;
		      case 'C':
		        gl_fixup(gl_prompt, -1, gl_pos+1); /* right */
		        break;
		      case 'D':
		        gl_fixup(gl_prompt, -1, gl_pos-1); /* left */
		        break;
		      case '0':
		      case '1':
		        goto ansi;
		      default: gl_beep();         /* who knows */
		        break;
		    }
		} else if ((gl_vi_preferred == 0) && ((c == 'f') || (c == 'F'))) {
		    gl_word(1);
		} else if ((gl_vi_preferred == 0) && ((c == 'b') || (c == 'B'))) {
		    gl_word(-1);
		} else if (c != (-1)) {
			/* enter vi command mode */
#if defined(__windows__) || defined(MSDOS)
			if (gl_vi_preferred == 0) {
				/* On Windows, ESC acts like a line kill,
				 * so don't use vi mode unless they prefer
				 * vi mode.
				 */
              			gl_kill(0);
			} else
#endif
			if (gl_vi_mode == 0) {
				gl_vi_mode = 1;
				vi_count = 1;
				vi_delete = 0;
				memset(vi_countbuf, 0, sizeof(vi_countbuf));
				if (gl_pos > 0)
					gl_fixup(gl_prompt, -2, gl_pos-1);	/* left 1 char */
				/* Don't bother if the line is empty and we don't
				 * know for sure if the user wants vi mode.
				 */
				if ((gl_cnt > 0) || (gl_vi_preferred == 1)) {
					/* We still have to use the char read! */
					goto vi;
				}
				gl_vi_mode = 0;
			} else {
				gl_beep();
			}
		}
		break;
	      default:		/* check for a terminal signal */
	        if (c > 0) {	/* ignore 0 (reset above) */
	            if (c == gl_intrc) {
			gl_result = GL_INTERRUPT;
			gl_buf[0] = 0;
	                gl_cleanup();
#ifdef SIGINT
	                raise(SIGINT);
	                gl_init();
	                gl_redraw();
#endif
			return gl_buf;
		    }

	            if (c == gl_quitc) {
			gl_result = GL_INTERRUPT;
			gl_buf[0] = 0;
	                gl_cleanup();
#ifdef SIGQUIT
	                raise(SIGQUIT);
	                gl_init();
	                gl_redraw();
#endif
			return gl_buf;
		    }

#ifdef __unix__
	            if (c == gl_suspc || c == gl_dsuspc) {
#ifdef SIGTSTP
			gl_result = GL_INTERRUPT;
			gl_buf[0] = 0;
			gl_cleanup();
	                sig = SIGTSTP;
	                kill(0, sig);
	                gl_init();
	                gl_redraw();
			return gl_buf;
#endif
		    }
#endif /* __unix__ */
		}
                if (c > 0)
		    gl_beep();
		break;
	    }
	}
	if (c > 0)
	    lastch = c;
    }
    gl_buf[0] = 0;
    gl_cleanup();
    return gl_buf;
}

static void
gl_addchar(int c)
      
/* adds the character c to the input buffer at current location */
{
    int  i;

    if (gl_cnt >= GL_BUF_SIZE - 1)
	gl_error("\n*** Error: getline(): input buffer overflow\n");
    if (gl_overwrite == 0 || gl_pos == gl_cnt) {
        for (i=gl_cnt; i >= gl_pos; i--)
            gl_buf[i+1] = gl_buf[i];
        gl_buf[gl_pos] = (char) c;
        gl_fixup(gl_prompt, gl_pos, gl_pos+1);
    } else {
	gl_buf[gl_pos] = (char) c;
	gl_extent = 1;
        gl_fixup(gl_prompt, gl_pos, gl_pos+1);
    }
}

static void
gl_yank(void)
/* adds the kill buffer to the input buffer at current location */
{
    int  i, len;

    len = (int) strlen(gl_killbuf);
    if (len > 0) {
	if (gl_overwrite == 0) {
            if (gl_cnt + len >= GL_BUF_SIZE - 1) 
	        gl_error("\n*** Error: getline(): input buffer overflow\n");
            for (i=gl_cnt; i >= gl_pos; i--)
                gl_buf[i+len] = gl_buf[i];
	    for (i=0; i < len; i++)
                gl_buf[gl_pos+i] = gl_killbuf[i];
            gl_fixup(gl_prompt, gl_pos, gl_pos+len);
	} else {
	    if (gl_pos + len > gl_cnt) {
                if (gl_pos + len >= GL_BUF_SIZE - 1) 
	            gl_error("\n*** Error: getline(): input buffer overflow\n");
		gl_buf[gl_pos + len] = 0;
            }
	    for (i=0; i < len; i++)
                gl_buf[gl_pos+i] = gl_killbuf[i];
	    gl_extent = len;
            gl_fixup(gl_prompt, gl_pos, gl_pos+len);
	}
    } else
	gl_beep();
}

static void
gl_transpose(void)
/* switch character under cursor and to left of cursor */
{
    int    c;

    if (gl_pos > 0 && gl_cnt > gl_pos) {
	c = gl_buf[gl_pos-1];
	gl_buf[gl_pos-1] = gl_buf[gl_pos];
	gl_buf[gl_pos] = (char) c;
	gl_extent = 2;
	gl_fixup(gl_prompt, gl_pos-1, gl_pos);
    } else
	gl_beep();
}

static void
gl_newline(void)
/*
 * Cleans up entire line before returning to caller. A \n is appended.
 * If line longer than screen, we redraw starting at beginning
 */
{
    int change = gl_cnt;
    int len = gl_cnt;
    int loc = gl_width - 5;	/* shifts line back to start position */

    if (gl_cnt >= GL_BUF_SIZE - 1) 
        gl_error("\n*** Error: getline(): input buffer overflow\n");
    if (gl_out_hook) {
	change = gl_out_hook(gl_buf);
        len = (int) strlen(gl_buf);
    } 
    if (loc > len)
	loc = len;
    gl_fixup(gl_prompt, change, loc);	/* must do this before appending \n */
    gl_buf[len] = '\n';
    gl_buf[len+1] = '\0';
    gl_putc('\n');
}

static void
gl_del(int loc, int killsave)
        
/*
 * Delete a character.  The loc variable can be:
 *    -1 : delete character to left of cursor
 *     0 : delete character under cursor
 */
{
    int i, j;

    if ((loc == -1 && gl_pos > 0) || (loc == 0 && gl_pos < gl_cnt)) {
        for (j=0, i=gl_pos+loc; i < gl_cnt; i++) {
	    if ((j == 0) && (killsave != 0) && (gl_vi_mode != 0)) {
	    	gl_killbuf[0] = gl_buf[i];
	    	gl_killbuf[1] = '\0';
		j = 1;
	    }
	    gl_buf[i] = gl_buf[i+1];
	}
	gl_fixup(gl_prompt, gl_pos+loc, gl_pos+loc);
    } else
	gl_beep();
}

static void
gl_kill(int pos)
        
/* delete from pos to the end of line */
{
    if (pos < gl_cnt) {
	strcpy(gl_killbuf, gl_buf + pos);
	gl_buf[pos] = '\0';
	gl_fixup(gl_prompt, pos, pos);
    } else
	gl_beep();
}

static void
gl_killword(int direction)
{
    int pos = gl_pos;
    int startpos = gl_pos;
    int tmp;
    int i;

    if (direction > 0) {		/* forward */
        while (!isspace(gl_buf[pos]) && pos < gl_cnt) 
	    pos++;
	while (isspace(gl_buf[pos]) && pos < gl_cnt)
	    pos++;
    } else {				/* backward */
	if (pos > 0)
	    pos--;
	while (isspace(gl_buf[pos]) && pos > 0)
	    pos--;
        while (!isspace(gl_buf[pos]) && pos > 0) 
	    pos--;
	if (pos < gl_cnt && isspace(gl_buf[pos]))   /* move onto word */
	    pos++;
    }
    if (pos < startpos) {
    	tmp = pos;
	pos = startpos;
	startpos = tmp;
    }
    memcpy(gl_killbuf, gl_buf + startpos, (size_t) (pos - startpos));
    gl_killbuf[pos - startpos] = '\0';
    if (isspace(gl_killbuf[pos - startpos - 1]))
    	gl_killbuf[pos - startpos - 1] = '\0';
    gl_fixup(gl_prompt, -1, startpos);
    for (i=0, tmp=pos - startpos; i<tmp; i++)
    	gl_del(0, 0);
}	/* gl_killword */

static void
gl_word(int direction)
              
/* move forward or backword one word */
{
    int pos = gl_pos;

    if (direction > 0) {		/* forward */
        while (!isspace(gl_buf[pos]) && pos < gl_cnt) 
	    pos++;
	while (isspace(gl_buf[pos]) && pos < gl_cnt)
	    pos++;
    } else {				/* backword */
	if (pos > 0)
	    pos--;
	while (isspace(gl_buf[pos]) && pos > 0)
	    pos--;
        while (!isspace(gl_buf[pos]) && pos > 0) 
	    pos--;
	if (pos < gl_cnt && isspace(gl_buf[pos]))   /* move onto word */
	    pos++;
    }
    gl_fixup(gl_prompt, -1, pos);
}

static void
gl_redraw(void)
/* emit a newline, reset and redraw prompt and current input line */
{
    if (gl_init_done > 0) {
        gl_putc('\n');
        gl_fixup(gl_prompt, -2, gl_pos);
    }
}

static void
gl_fixup(const char *prompt, int change, int cursor)
              
                      
/*
 * This function is used both for redrawing when input changes or for
 * moving within the input line.  The parameters are:
 *   prompt:  compared to last_prompt[] for changes;
 *   change : the index of the start of changes in the input buffer,
 *            with -1 indicating no changes, -2 indicating we're on
 *            a new line, redraw everything.
 *   cursor : the desired location of the cursor after the call.
 *            A value of GL_BUF_SIZE can be used  to indicate the cursor should
 *            move just past the end of the input line.
 */
{
    static int   gl_shift;	/* index of first on screen character */
    static int   off_right;	/* true if more text right of screen */
    static int   off_left;	/* true if more text left of screen */
    static char  last_prompt[80] = "";
    int          left = 0, right = -1;		/* bounds for redraw */
    int          pad;		/* how much to erase at end of line */
    int          backup;        /* how far to backup before fixing */
    int          new_shift;     /* value of shift based on cursor */
    int          extra;         /* adjusts when shift (scroll) happens */
    int          i;
    int          new_right = -1; /* alternate right bound, using gl_extent */
    int          l1, l2;

    if (change == -2) {   /* reset */
	gl_pos = gl_cnt = gl_shift = off_right = off_left = 0;
	gl_putc('\r');
	gl_puts(prompt);
	strcpy(last_prompt, prompt);
	change = 0;
        gl_width = gl_termw - (int) gl_strlen(prompt);
    } else if (strcmp(prompt, last_prompt) != 0) {
	l1 = (int) gl_strlen(last_prompt);
	l2 = (int) gl_strlen(prompt);
	gl_cnt = gl_cnt + l1 - l2;
	strcpy(last_prompt, prompt);
	gl_putc('\r');
	gl_puts(prompt);
	gl_pos = gl_shift;
        gl_width = gl_termw - l2;
	change = 0;
    }
    pad = (off_right)? gl_width - 1 : gl_cnt - gl_shift;   /* old length */
    backup = gl_pos - gl_shift;
    if (change >= 0) {
        gl_cnt = (int) strlen(gl_buf);
        if (change > gl_cnt)
	    change = gl_cnt;
    }
    if (cursor > gl_cnt) {
	if (cursor != GL_BUF_SIZE) {		/* GL_BUF_SIZE means end of line */
	    if (gl_ellipses_during_completion == 0) {
	        gl_beep();
	    }
	}
	cursor = gl_cnt;
    }
    if (cursor < 0) {
	gl_beep();
	cursor = 0;
    }
    if (off_right || (off_left && cursor < gl_shift + gl_width - gl_scroll / 2))
	extra = 2;			/* shift the scrolling boundary */
    else 
	extra = 0;
    new_shift = cursor + extra + gl_scroll - gl_width;
    if (new_shift > 0) {
	new_shift /= gl_scroll;
	new_shift *= gl_scroll;
    } else
	new_shift = 0;
    if (new_shift != gl_shift) {	/* scroll occurs */
	gl_shift = new_shift;
	off_left = (gl_shift)? 1 : 0;
	off_right = (gl_cnt > gl_shift + gl_width - 1)? 1 : 0;
        left = gl_shift;
	new_right = right = (off_right)? gl_shift + gl_width - 2 : gl_cnt;
    } else if (change >= 0) {		/* no scroll, but text changed */
	if (change < gl_shift + off_left) {
	    left = gl_shift;
	} else {
	    left = change;
	    backup = gl_pos - change;
	}
	off_right = (gl_cnt > gl_shift + gl_width - 1)? 1 : 0;
	right = (off_right)? gl_shift + gl_width - 2 : gl_cnt;
	new_right = (gl_extent && (right > left + gl_extent))? 
	             left + gl_extent : right;
    }
    pad -= (off_right)? gl_width - 1 : gl_cnt - gl_shift;
    pad = (pad < 0)? 0 : pad;
    if (left <= right) {		/* clean up screen */
	for (i=0; i < backup; i++)
	    gl_putc('\b');
	if (left == gl_shift && off_left) {
	    gl_putc('$');
	    left++;
        }
	for (i=left; i < new_right; i++)
	    gl_putc(gl_buf[i]);
	gl_pos = new_right;
	if (off_right && new_right == right) {
	    gl_putc('$');
	    gl_pos++;
	} else { 
	    for (i=0; i < pad; i++)	/* erase remains of prev line */
		gl_putc(' ');
	    gl_pos += pad;
	}
    }
    i = gl_pos - cursor;		/* move to final cursor location */
    if (i > 0) {
	while (i--)
	   gl_putc('\b');
    } else {
	for (i=gl_pos; i < cursor; i++)
	    gl_putc(gl_buf[i]);
    }
    gl_pos = cursor;
}

static int
gl_tab(char *buf, int offset, int *loc, size_t bufsize)
/* default tab handler, acts like tabstops every 8 cols */
{
    int i, count, len;

    len = (int) strlen(buf);
    count = 8 - (offset + *loc) % 8;
    for (i=len; i >= *loc; i--)
    	if (i+count < (int) bufsize)
		buf[i+count] = buf[i];
    for (i=0; i < count; i++)
    	if (*loc+i < (int) bufsize)
		buf[*loc+i] = ' ';
    i = *loc;
    *loc = i + count;
    return i;
}

/******************* History stuff **************************************/

#ifndef HIST_SIZE
#define HIST_SIZE 100
#endif

static int      hist_pos = 0, hist_last = 0;
static char    *hist_buf[HIST_SIZE];
static char     hist_empty_elem[2] = "";

static void
hist_init(void)
{
    int i;

    hist_buf[0] = hist_empty_elem;
    for (i=1; i < HIST_SIZE; i++)
	hist_buf[i] = (char *)0;
}

void
gl_histadd(char *buf)
{
    static char *prev = 0;
    char *p = buf;
    int len;

    /* in case we call gl_histadd() before we call getline() */
    if (gl_init_done < 0) {		/* -1 only on startup */
        hist_init();
        gl_init_done = 0;
    }
    while (*p == ' ' || *p == '\t' || *p == '\n') 
	p++;
    if (*p) {
	len = (int) strlen(buf);
	if (strchr(p, '\n')) 	/* previously line already has NL stripped */
	    len--;
	if ((prev == 0) || ((int) strlen(prev) != len) || 
			    strncmp(prev, buf, (size_t) len) != 0) {
            hist_buf[hist_last] = hist_save(buf);
	    prev = hist_buf[hist_last];
            hist_last = (hist_last + 1) % HIST_SIZE;
            if (hist_buf[hist_last] && *hist_buf[hist_last]) {
	        free(hist_buf[hist_last]);
            }
	    hist_buf[hist_last] = hist_empty_elem;
	}
    }
    hist_pos = hist_last;
}

static char *
hist_prev(void)
/* loads previous hist entry into input buffer, sticks on first */
{
    char *p = 0;
    int   next = (hist_pos - 1 + HIST_SIZE) % HIST_SIZE;

    if (hist_buf[hist_pos] != 0 && next != hist_last) {
        hist_pos = next;
        p = hist_buf[hist_pos];
    } 
    if (p == 0) {
	p = hist_empty_elem;
	gl_beep();
    }
    return p;
}

static char *
hist_next(void)
/* loads next hist entry into input buffer, clears on last */
{
    char *p = 0;

    if (hist_pos != hist_last) {
        hist_pos = (hist_pos+1) % HIST_SIZE;
	p = hist_buf[hist_pos];
    } 
    if (p == 0) {
	p = hist_empty_elem;
	gl_beep();
    }
    return p;
}

static char *
hist_save(char *p)
        
/* makes a copy of the string */
{
    char *s = 0;
    size_t len = strlen(p);
    char *nl = strpbrk(p, "\n\r");

    if (nl) {
        if ((s = (char *) malloc(len)) != 0) {
            strncpy(s, p, len-1);
	    s[len-1] = 0;
	}
    } else {
        if ((s = (char *) malloc(len+1)) != 0) {
            strcpy(s, p);
        }
    }
    if (s == 0) 
	gl_error("\n*** Error: hist_save() failed on malloc\n");
    return s;
}




void
gl_histsavefile(const char *const path)
{
	FILE *fp;
	const char *p;
	int i, j;

	fp = fopen(path,
#if defined(__windows__) || defined(MSDOS)
		"wt"
#else
		"w"
#endif
	);
	if (fp != NULL) {
		for (i=2; i<HIST_SIZE; i++) {
        		j = (hist_pos+i) % HIST_SIZE;
			p = hist_buf[j];
			if ((p == NULL) || (*p == '\0'))
				continue;
			fprintf(fp, "%s\n", p);
		}
		fclose(fp);
	}	
}	/* gl_histsavefile */




void
gl_histloadfile(const char *const path)
{
	FILE *fp;
	char line[256];

	fp = fopen(path,
#if defined(__windows__) || defined(MSDOS)
		"rt"
#else
		"r"
#endif
	);
	if (fp != NULL) {
		memset(line, 0, sizeof(line));
		while (fgets(line, sizeof(line) - 2, fp) != NULL) {
			gl_histadd(line);
		}
		fclose(fp);
	}
}	/* gl_histloadfile */




/******************* Search stuff **************************************/

static char  search_prompt[101];  /* prompt includes search string */
static char  search_string[100];
static int   search_pos = 0;      /* current location in search_string */
static int   search_forw_flg = 0; /* search direction flag */
static int   search_last = 0;	  /* last match found */

static void  
search_update(int c)
{
    if (c == 0) {
	search_pos = 0;
        search_string[0] = 0;
        search_prompt[0] = '?';
        search_prompt[1] = ' ';
        search_prompt[2] = 0;
    } else if (c > 0) {
        search_string[search_pos] = (char) c;
        search_string[search_pos+1] = (char) 0;
        search_prompt[search_pos] = (char) c;
        search_prompt[search_pos+1] = (char) '?';
        search_prompt[search_pos+2] = (char) ' ';
        search_prompt[search_pos+3] = (char) 0;
	search_pos++;
    } else {
	if (search_pos > 0) {
	    search_pos--;
            search_string[search_pos] = (char) 0;
            search_prompt[search_pos] = (char) '?';
            search_prompt[search_pos+1] = (char) ' ';
            search_prompt[search_pos+2] = (char) 0;
	} else {
	    gl_beep();
	    hist_pos = hist_last;
	}
    }
}

static void 
search_addchar(int c)
{
    char *loc;

    search_update(c);
    if (c < 0) {
	if (search_pos > 0) {
	    hist_pos = search_last;
	} else {
	    gl_buf[0] = 0;
	    hist_pos = hist_last;
	}
	strcpy(gl_buf, hist_buf[hist_pos]);
    }
    if ((loc = strstr(gl_buf, search_string)) != 0) {
	gl_fixup(search_prompt, 0, (int) (loc - gl_buf));
    } else if (search_pos > 0) {
        if (search_forw_flg) {
	    search_forw(0);
        } else {
	    search_back(0);
        }
    } else {
	gl_fixup(search_prompt, 0, 0);
    }
}

static void     
search_term(void)
{
    gl_search_mode = 0;
    if (gl_buf[0] == 0)		/* not found, reset hist list */
        hist_pos = hist_last;
    if (gl_in_hook)
	gl_in_hook(gl_buf);
    gl_fixup(gl_prompt, 0, gl_pos);
}

static void     
search_back(int new_search)
{
    int    found = 0;
    char  *p, *loc;

    search_forw_flg = 0;
    if (gl_search_mode == 0) {
	search_last = hist_pos = hist_last;	
	search_update(0);	
	gl_search_mode = 1;
        gl_buf[0] = 0;
	gl_fixup(search_prompt, 0, 0);
    } else if (search_pos > 0) {
	while (!found) {
	    p = hist_prev();
	    if (*p == 0) {		/* not found, done looking */
	       gl_buf[0] = 0;
	       gl_fixup(search_prompt, 0, 0);
	       found = 1;
	    } else if ((loc = strstr(p, search_string)) != 0) {
	       strcpy(gl_buf, p);
	       gl_fixup(search_prompt, 0, (int) (loc - p));
	       if (new_search)
		   search_last = hist_pos;
	       found = 1;
	    } 
	}

    } else {
        gl_beep();
    }
}

static void     
search_forw(int new_search)
{
    int    found = 0;
    char  *p, *loc;

    search_forw_flg = 1;
    if (gl_search_mode == 0) {
	search_last = hist_pos = hist_last;	
	search_update(0);	
	gl_search_mode = 1;
        gl_buf[0] = 0;
	gl_fixup(search_prompt, 0, 0);
    } else if (search_pos > 0) {
	while (!found) {
	    p = hist_next();
	    if (*p == 0) {		/* not found, done looking */
	       gl_buf[0] = 0;
	       gl_fixup(search_prompt, 0, 0);
	       found = 1;
	    } else if ((loc = strstr(p, search_string)) != 0) {
	       strcpy(gl_buf, p);
	       gl_fixup(search_prompt, 0, (int) (loc - p));
	       if (new_search)
		   search_last = hist_pos;
	       found = 1;
	    } 
	}
    } else {
        gl_beep();
    }
}


static void
gl_beep(void)
{
#ifdef __windows__
	MessageBeep(MB_OK);
#else
	gl_putc('\007');
#endif
}	/* gl_beep */



static int
gl_display_matches_sort_proc(const void *a, const void *b)
{
	return (strcasecmp(
		* ((const char **) a),
		* ((const char **) b)
	));
}	/* gl_display_matches_sort_proc */



static void
gl_display_matches(int nused)
{
	char buf[256];
	char buf2[256];
	size_t ilen, imaxlen;
	int i, j, k, l;
	int glen, allmatch;
	int nmax, ncol, colw, nrow;
	char *cp1, *cp2, *lim, *itemp;

	gl_putc('\n');
	if (nused == 0) {
		gl_beep();
		gl_puts("    (no matches)");
		gl_putc('\n');
	} else {
		qsort(gl_matchlist, (size_t) nused, sizeof(char *), gl_display_matches_sort_proc);

		/* Find the greatest amount that matches. */
		for (glen = 0; ; glen++) {
			allmatch = 1;
			for (i=1; i<nused; i++) {
				if (gl_matchlist[0][glen] != gl_matchlist[i][glen]) {
					allmatch = 0;
					break;
				}
			}
			if (allmatch == 0)
				break;
		}

		while (glen > 0) {
			if (!isalnum(gl_matchlist[0][glen - 1]))
				break;
			--glen;
		}

		nmax = nused;
		imaxlen = strlen(gl_matchlist[0]);
		for (i=1; i<nused; i++) {
			ilen = strlen(gl_matchlist[i]);
			if (ilen > imaxlen)
				imaxlen = ilen;
		}

		/* Subtract amount we'll skip for each item. */
		imaxlen -= glen;

		ncol = (gl_termw - 8) / ((int) imaxlen + 2);
		if (ncol < 1)
			ncol = 1;

		colw = (gl_termw - 8) / ncol; 
		nrow = nmax / ncol;
		if ((nused % ncol) != 0)
			nrow++;

		if (nrow > (gl_termh - 4)) {
			nrow = gl_termh - 4;
			nmax = ncol * nrow; 
		}

		for (i=0; i<(int) sizeof(buf2); i++)
			buf2[i] = ' ';

		for (j=0; j<nrow; j++) {
			(void) memcpy(buf, buf2, sizeof(buf));
			for (i=0, k=j, l=4; i<ncol; i++, k += nrow, l += colw) {
				if (k >= nmax)
					continue;
				itemp = gl_matchlist[k] + glen;
				cp1 = buf + l;
				lim = cp1 + (int) strlen(itemp);
				if (lim > (buf + sizeof(buf) - 1))
					continue;
				cp2 = itemp;
				while (cp1 < lim)
					*cp1++ = *cp2++;
			}
			for (cp1 = buf + sizeof(buf); *--cp1 == ' '; )
				;
			++cp1;
			*cp1 = '\0';
			gl_puts(buf);
			gl_putc('\n');
		}

		if (nused > nmax) {
			(void) sprintf(buf, "    ... %d others omitted ...", (nused - nmax));
			gl_puts(buf);
			gl_putc('\n');
		}
	}
	gl_fixup(gl_prompt, -2, GL_BUF_SIZE);
}	/* gl_display_matches */




static int
gl_do_tab_completion(char *buf, int *loc, size_t bufsize, int tabtab)
{
	char *startp;
	size_t startoff, amt;
	int c;
	int qmode;
	char *qstart;
	char *lastspacestart;
	char *cp;
	int ntoalloc, nused, nprocused, nalloced, i;
	char **newgl_matchlist;
	char *strtoadd, *strtoadd1;
	int addquotes;
	size_t llen, mlen, glen;
	int allmatch;
	char *curposp;
	size_t lenaftercursor;
	char *matchpfx;
	int wasateol;
	char ellipsessave[4];

	/* Zero out the rest of the buffer, so we can move stuff around
	 * and know we'll still be NUL-terminated.
	 */
	llen = strlen(buf);
	memset(buf + llen, 0, bufsize - llen);
	bufsize -= 4;	/* leave room for a NUL, space, and two quotes. */
	curposp = buf + *loc;
	wasateol = (*curposp == '\0');
	lenaftercursor = llen - (curposp - buf);
	if (gl_ellipses_during_completion != 0) {
		memcpy(ellipsessave, curposp, (size_t) 4);
		memcpy(curposp, "... ", (size_t) 4);
		gl_fixup(gl_prompt, gl_pos, gl_pos + 3);
		memcpy(curposp, ellipsessave, (size_t) 4);
	}
	
	qmode = 0;
	qstart = NULL;
	lastspacestart = NULL;
	matchpfx = NULL;

	cp = buf;
	while (cp < curposp) {
		c = (int) *cp++;
		if (c == '\0')
			break;
		if ((c == '"') || (c == '\'')) {
			if (qmode == c) {
				/* closing quote; end it. */
				qstart = NULL;
				qmode = 0;
			} else if (qmode != 0) {
				/* just treat it as a regular char. */
			} else {
				/* start new quote group. */
				qmode = c;
				qstart = cp - 1;
			}
		} else if ((isspace(c)) && (qmode == 0)) {
			/* found a non-quoted space. */
			lastspacestart = cp - 1;
		} else {
			/* regular char */
		}
	}

	if (qstart != NULL)
		startp = qstart + 1;
	else if (lastspacestart != NULL)
		startp = lastspacestart + 1;
	else
		startp = buf;
	
	cp = startp;
	mlen = (curposp - cp);

	matchpfx = (char *) malloc(mlen + 1);
	memcpy(matchpfx, cp, mlen);
	matchpfx[mlen] = '\0';

#define GL_COMPLETE_VECTOR_BLOCK_SIZE 64

	nused = 0;
	ntoalloc = GL_COMPLETE_VECTOR_BLOCK_SIZE;
	newgl_matchlist = (char **) malloc((size_t) (sizeof(char *) * (ntoalloc + 1)));
	if (newgl_matchlist == NULL) {
		free(matchpfx);
		gl_beep();
		return 0;
	}
	gl_matchlist = newgl_matchlist;
	nalloced = ntoalloc;
	for (i=nused; i<=nalloced; i++)
		gl_matchlist[i] = NULL;
	
	gl_completion_exact_match_extra_char = ' ';
	for (nprocused = 0;; nprocused++) {
		if (nused == nalloced) {
			ntoalloc += GL_COMPLETE_VECTOR_BLOCK_SIZE;
			newgl_matchlist = (char **) realloc((char *) gl_matchlist, (size_t) (sizeof(char *) * (ntoalloc + 1)));
			if (newgl_matchlist == NULL) {
				/* not enough memory to expand list -- abort */
				for (i=0; i<nused; i++)
					free(gl_matchlist[i]);
				free(gl_matchlist);
				gl_matchlist = NULL;
				gl_beep();
				free(matchpfx);
				return 0;
			}
			gl_matchlist = newgl_matchlist;
			nalloced = ntoalloc;
			for (i=nused; i<=nalloced; i++)
				gl_matchlist[i] = NULL;
		}
	        cp = gl_completion_proc(matchpfx, nprocused);
		if (cp == NULL)
			break;
		if ((cp[0] == '.') && ((cp[1] == '\0') || ((cp[1] == '.') && (cp[2] == '\0'))))
			continue;	/* Skip . and .. */
		gl_matchlist[nused++] = cp;
	}

	if (gl_ellipses_during_completion != 0) {
		gl_fixup(gl_prompt, gl_pos, gl_pos);
		gl_puts("    ");
	}

	/* We now have an array strings, whose last element is NULL. */
	strtoadd = NULL;
	strtoadd1 = NULL;
	amt = 0;

	addquotes = (gl_filename_quoting_desired > 0) || ((gl_filename_quoting_desired < 0) && (gl_completion_proc == gl_local_filename_completion_proc));

	if (nused == 1) {
		/* Exactly one match. */
		strtoadd = gl_matchlist[0];
	} else if (tabtab != 0) {
		/* TAB-TAB: print all matches */
		gl_display_matches(nused);
	} else if ((nused > 1) && (mlen > 0)) {
		/* Find the greatest amount that matches. */
		for (glen = strlen(matchpfx); ; glen++) {
			allmatch = 1;
			for (i=1; i<nused; i++) {
				if (gl_matchlist[0][glen] != gl_matchlist[i][glen]) {
					allmatch = 0;
					break;
				}
			}
			if (allmatch == 0)
				break;
		}
		strtoadd1 = (char *) malloc(glen + 1);
		if (strtoadd1 != NULL) {
			memcpy(strtoadd1, gl_matchlist[0], glen);
			strtoadd1[glen] = '\0';
			strtoadd = strtoadd1;
		}
	}

	if (strtoadd != NULL) {
		if ((qmode == 0) && (addquotes != 0)) {
			if (strpbrk(strtoadd, gl_filename_quote_characters) != NULL) {
				qmode = (strchr(strtoadd, '"') == NULL) ? '"' : '\'';
				memmove(curposp + 1, curposp, lenaftercursor + 1 /* NUL */);
				curposp++;
				*startp++ = (char) qmode;
			}
		}
		startoff = (size_t) (startp - buf);
		amt = strlen(strtoadd);
		if ((amt + startoff + lenaftercursor) >= bufsize)
			amt = bufsize - (amt + startoff + lenaftercursor);
		memmove(curposp + amt - mlen, curposp, lenaftercursor + 1 /* NUL */);
		curposp += amt - mlen;
		memcpy(startp, strtoadd, amt);
		if (nused == 1) {
			/* Exact match. */
			if (qmode != 0) {
				/* Finish the quoting. */
				memmove(curposp + 1, curposp, lenaftercursor + 1 /* NUL */);
				curposp++;
				buf[amt + startoff] = (char) qmode;
				amt++;
			}
			memmove(curposp + 1, curposp, lenaftercursor + 1 /* NUL */);
			curposp++;
			buf[amt + startoff] = (char) gl_completion_exact_match_extra_char;
			amt++;
		} else if ((!wasateol) && (!isspace(*curposp))) {
			/* Not a full match, but insert a
			 * space for better readability.
			 */
			memmove(curposp + 1, curposp, lenaftercursor + 1 /* NUL */);
			curposp++;
			buf[amt + startoff] = ' ';
		}
		*loc = (int) (startoff + amt);

		if (strtoadd1 != NULL)
			free(strtoadd1);
	}

	/* Don't need this any more. */
	for (i=0; i<nused; i++)
		free(gl_matchlist[i]);
	free(gl_matchlist);
	gl_matchlist = NULL;
	free(matchpfx);

	return 0;
}	/* gl_do_tab_completion */




void
gl_tab_completion(gl_tab_completion_proc proc)
{
	if (proc == NULL)
		proc = gl_local_filename_completion_proc;	/* default proc */
	gl_completion_proc = proc;
}	/* gl_tab_completion */




#ifndef _StrFindLocalPathDelim
static char *
_StrRFindLocalPathDelim(const char *src)	/* TODO: optimize */
{
	const char *last;
	int c;

	last = NULL;
	for (;;) {
		c = *src++;
		if (c == '\0')
			break;
		if (IsLocalPathDelim(c))
			last = src - 1;
	}

	return ((char *) last);
}	/* StrRFindLocalPathDelim */
#endif	/* Windows */




void
gl_set_home_dir(const char *homedir)
{
	size_t len;
#ifdef __windows__
	const char *homedrive, *homepath;
	char wdir[64];
#else
	struct passwd *pw;
	char *cp;
#endif

	if (gl_home_dir != NULL) {
		free(gl_home_dir);
		gl_home_dir = NULL;
	}

	if (homedir == NULL) {
#ifdef __windows__
		homedrive = getenv("HOMEDRIVE");
		homepath = getenv("HOMEPATH");
		if ((homedrive != NULL) && (homepath != NULL)) {
			len = strlen(homedrive) + strlen(homepath) + 1;
			gl_home_dir = (char *) malloc(len);
			if (gl_home_dir != NULL) {
				strcpy(gl_home_dir, homedrive);
				strcat(gl_home_dir, homepath);
				return;
			}
		}
		
		wdir[0] = '\0';
		if (GetWindowsDirectory(wdir, sizeof(wdir) - 1) < 1)
			(void) strncpy(wdir, ".", sizeof(wdir));
		else if (wdir[1] == ':') {
			wdir[2] = '\\';
			wdir[3] = '\0';
		}
		homedir = wdir;
#else
		cp = (char *) getlogin();
		if (cp == NULL) {
			cp = (char *) getenv("LOGNAME");
			if (cp == NULL)
				cp = (char *) getenv("USER");
		}
		pw = NULL;
		if (cp != NULL)
			pw = getpwnam(cp);
		if (pw == NULL)
			pw = getpwuid(getuid());
		if (pw == NULL)
			return;	/* hell with it */
		homedir = pw->pw_dir;
#endif
	}

	len = strlen(homedir) + /* NUL */ 1;
	gl_home_dir = (char *) malloc(len);
	if (gl_home_dir != NULL) {
		memcpy(gl_home_dir, homedir, len);
	}
}	/* gl_set_home_dir */




char *gl_getpass(const char *const prompt, char *const pass, int dsize)
{
#ifdef __unix__
	char *cp;
	int c;

	memset(pass, 0, (size_t) sizeof(dsize));
	dsize--;
	gl_init();

	/* Display the prompt first. */
	if ((prompt != NULL) && (prompt[0] != '\0'))
		gl_puts(prompt);

	cp = pass;
	while ((c = gl_getc()) != (-1)) {
		if ((c == '\r') || (c == '\n'))
			break;
		if ((c == '\010') || (c == '\177'))	{
			/* ^H and DEL */
			if (cp > pass) {
				*--cp = '\0';
				gl_putc('\010');
				gl_putc(' ');
				gl_putc('\010');
			}
		} else if (cp < (pass + dsize)) {
			gl_putc('*');
			*cp++ = c;
		}
	}
	*cp = '\0';
	gl_putc('\n');
	gl_cleanup();
	return (pass);
#else
#ifdef __windows__
	char *cp;
	int c;

	FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));
	ZeroMemory(pass, (DWORD) sizeof(dsize));
	dsize--;

	if ((prompt != NULL) && (prompt[0] != '\0'))
		_cputs(prompt);

	for (cp = pass;;) {
		c = (int) _getch();
		if ((c == '\r') || (c == '\n'))
			break;
		if ((c == '\010') || (c == '\177'))	{
			/* ^H and DEL */
			if (cp > pass) {
				*--cp = '\0';
				_putch('\010');
				_putch(' ');
				_putch('\010');
			}
		} else if (cp < (pass + dsize)) {
			_putch('*');
			*cp++ = c;
		}
	}
	_putch('\r');
	_putch('\n');
	Sleep(40);
	FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));

	*cp = '\0';
	return (pass);
#endif	/* __windows__ */
#endif	/* ! __unix__ */
}	/* gl_getpass */




#ifdef __unix__

char *
gl_local_filename_completion_proc(const char *start, int idx)
{
	static DIR *dir = NULL;
	static int filepfxoffset;
	static size_t filepfxlen;

	const char *filepfx;
	struct dirent *dent;
	char *cp;
	const char *dirtoopen, *name;
	char *dirtoopen1;
	size_t len, len2;
	struct stat st;

	if (idx == 0) {
		if (dir != NULL) {
			/* shouldn't get here! */
			closedir(dir);
			dir = NULL;
		}
	}

	if (dir == NULL) {
		dirtoopen1 = NULL;
		cp = _StrRFindLocalPathDelim(start);
		if (cp == start) {
			dirtoopen = LOCAL_PATH_DELIM_STR;	/* root dir */
			filepfxoffset = 1;
		} else if (cp == NULL) {
			dirtoopen = ".";
			filepfxoffset = 0;
		} else {
			len = strlen(start) + 1;
			dirtoopen1 = (char *) malloc(len);
			if (dirtoopen1 == NULL)
				return NULL;
			memcpy(dirtoopen1, start, len);
			len = (cp - start);
			dirtoopen1[len] = '\0';
			dirtoopen = dirtoopen1;
			filepfxoffset = (int) ((cp + 1) - start);
		}

		if (strcmp(dirtoopen, "~") == 0) {
			if (gl_home_dir == NULL)
				gl_set_home_dir(NULL);
			if (gl_home_dir == NULL)
				return (NULL);
			dirtoopen = gl_home_dir;
		}

		dir = opendir(dirtoopen);
		if (dirtoopen1 != NULL)
			free(dirtoopen1);

		filepfx = start + filepfxoffset;
		filepfxlen = strlen(filepfx);
	}

	if (dir != NULL) {
		/* assumes "start" is same for each iteration. */
		filepfx = start + filepfxoffset;

		for (;;) {
			dent = readdir(dir);
			if (dent == NULL) {
				/* no more items */
				closedir(dir);
				dir = NULL;

				if (idx == 1) {
					/* There was exactly one match.
					 * In this special case, we
					 * want to append a / instead
					 * of a space.
					 */
					cp = gl_matchlist[0];
					if ((cp[0] == '~') && ((cp[1] == '\0') || (IsLocalPathDelim(cp[1])))) {
						len = strlen(cp + 1) + /* NUL */ 1;
						len2 = strlen(gl_home_dir);
						if (IsLocalPathDelim(gl_home_dir[len2 - 1]))
							len2--;
						cp = (char *) realloc(gl_matchlist[0], len + len2);
						if (cp == NULL) {
							cp = gl_matchlist[0];
						} else {
							memmove(cp + len2, cp + 1, len);
							memcpy(cp, gl_home_dir, len2);
							gl_matchlist[0] = cp;
						}
					}
					if ((lstat(cp, &st) == 0) && (S_ISDIR(st.st_mode)))
						gl_completion_exact_match_extra_char = LOCAL_PATH_DELIM;
				}
				return NULL;
			}

			name = dent->d_name;
			if ((name[0] == '.') && ((name[1] == '\0') || ((name[1] == '.') && (name[2] == '\0'))))
				continue;	/* Skip . and .. */

			if ((filepfxlen == 0) || (strncmp(name, filepfx, filepfxlen) == 0)) {
				/* match */
				len = strlen(name);
				cp = (char *) malloc(filepfxoffset + len + 1 /* spare */ + 1 /* NUL */);
				*cp = '\0';
				if (filepfxoffset > 0)
					memcpy(cp, start, (size_t) filepfxoffset);
				memcpy(cp + filepfxoffset, name, len + 1);
				return (cp);
			}
		}
	}

	return NULL;
}	/* gl_local_filename_completion_proc */

#endif	/* __unix__ */





#ifdef __windows__

char *
gl_local_filename_completion_proc(const char *start, int idx)
{
	static HANDLE searchHandle = NULL;
	static int filepfxoffset;
	static size_t filepfxlen;

	WIN32_FIND_DATA ffd;
	DWORD dwErr;
	char *cp, *c2, ch;
	const char *filepfx;
	const char *dirtoopen, *name;
	char *dirtoopen1, *dirtoopen2;
	size_t len, len2;

	if (idx == 0) {
		if (searchHandle != NULL) {
			/* shouldn't get here! */
			FindClose(searchHandle);
			searchHandle = NULL;
		}
	}


	if (searchHandle == NULL) {
		dirtoopen1 = NULL;
		dirtoopen2 = NULL;
		cp = _StrRFindLocalPathDelim(start);
		if (cp == start) {
			dirtoopen = LOCAL_PATH_DELIM_STR;	/* root dir */
			filepfxoffset = 1;
		} else if (cp == NULL) {
			dirtoopen = ".";
			filepfxoffset = 0;
		} else {
			len = strlen(start) + 1;
			dirtoopen1 = (char *) malloc(len);
			if (dirtoopen1 == NULL)
				return NULL;
			memcpy(dirtoopen1, start, len);
			len = (cp - start);
			dirtoopen1[len] = '\0';
			dirtoopen = dirtoopen1;
			filepfxoffset = (int) ((cp + 1) - start);
		}

		if (strcmp(dirtoopen, "~") == 0) {
			if (gl_home_dir == NULL)
				gl_set_home_dir(NULL);
			if (gl_home_dir == NULL)
				return (NULL);
			dirtoopen = gl_home_dir;
		}

		len = strlen(dirtoopen);
		dirtoopen2 = (char *) malloc(len + 8);
		if (dirtoopen2 == NULL) {
			if (dirtoopen1 != NULL)
				free(dirtoopen1);
			return NULL;
		}

		memcpy(dirtoopen2, dirtoopen, len + 1);
		if (dirtoopen2[len - 1] == LOCAL_PATH_DELIM)
			memcpy(dirtoopen2 + len, "*.*", (size_t) 4);
		else
			memcpy(dirtoopen2 + len, "\\*.*", (size_t) 5);
				
		/* "Open" the directory. */
		memset(&ffd, 0, sizeof(ffd));
		searchHandle = FindFirstFile(dirtoopen2, &ffd);

		free(dirtoopen2);
		if (dirtoopen1 != NULL)
			free(dirtoopen1);

		if (searchHandle == INVALID_HANDLE_VALUE) {
			return NULL;
		}

		filepfx = start + filepfxoffset;
		filepfxlen = strlen(filepfx);
	} else {
		/* assumes "start" is same for each iteration. */
		filepfx = start + filepfxoffset;
		goto next;
	}
	
	for (;;) {
		
		name = ffd.cFileName;
		if ((name[0] == '.') && ((name[1] == '\0') || ((name[1] == '.') && (name[2] == '\0'))))
			goto next;	/* Skip . and .. */
		
		if ((filepfxlen == 0) || (strnicmp(name, filepfx, filepfxlen) == 0)) {
			/* match */
			len = strlen(name);
			cp = (char *) malloc(filepfxoffset + len + 4 /* spare */ + 1 /* NUL */);
			*cp = '\0';
			if (filepfxoffset > 0)
				memcpy(cp, start, filepfxoffset);
			memcpy(cp + filepfxoffset, name, len + 1);
			if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
				/* Embed file type with name. */
				c2 = cp + filepfxoffset + len + 1;
				*c2++ = '\0';
				*c2++ = 'd';
				*c2 = '\0';
			} else {
				c2 = cp + filepfxoffset + len + 1;
				*c2++ = '\0';
				*c2++ = '-';
				*c2 = '\0';
			}
			return (cp);
		}

next:
		if (!FindNextFile(searchHandle, &ffd)) {
			dwErr = GetLastError();
			if (dwErr != ERROR_NO_MORE_FILES) {
				FindClose(searchHandle);
				searchHandle = NULL;
				return NULL;
			}
				
			/* no more items */
			FindClose(searchHandle);
			searchHandle = NULL;
			
			if (idx == 1) {
				/* There was exactly one match.
				 * In this special case, we
				 * want to append a \ instead
				 * of a space.
				 */
				cp = gl_matchlist[0];
				ch = (char) cp[strlen(cp) + 2];
				if (ch == (char) 'd')
					gl_completion_exact_match_extra_char = LOCAL_PATH_DELIM;

				if ((cp[0] == '~') && ((cp[1] == '\0') || (IsLocalPathDelim(cp[1])))) {
					len = strlen(cp + 1) + /* NUL */ 1;
					len2 = strlen(gl_home_dir);
					if (IsLocalPathDelim(gl_home_dir[len2 - 1]))
						len2--;
					cp = (char *) realloc(gl_matchlist[0], len + len2 + 4);
					if (cp == NULL) {
						cp = gl_matchlist[0];
					} else {
						memmove(cp + len2, cp + 1, len);
						memcpy(cp, gl_home_dir, len2);
						c2 = cp + len + len2;
						*c2++ = '\0';
						*c2++ = ch;
						*c2 = '\0';
						gl_matchlist[0] = cp;
					}
				}
			}
			break;
		}
	}
	return (NULL);
}	/* gl_local_filename_completion_proc */

#endif	/* __windows__ */

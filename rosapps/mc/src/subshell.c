/* {{{ Copyright notice */

/* Concurrent shell support for the Midnight Commander
   Copyright (C) 1994, 1995 Dugan Porter

   This program is free software; you can redistribute it and/or
   modify it under the terms of Version 2 of the GNU General Public
   License, as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/* }}} */

#include <config.h>
#ifdef HAVE_SUBSHELL_SUPPORT

/* {{{ Declarations */

#include <stdio.h>      
#include <stdlib.h>	/* For errno, putenv, etc.	      */
#include <errno.h>	/* For errno on SunOS systems	      */
#include <termios.h>	/* tcgetattr(), struct termios, etc.  */
#if (!defined(__IBMC__) && !defined(__IBMCPP__))
#include <sys/types.h>	/* Required by unistd.h below	      */
#endif
#include <sys/ioctl.h>	/* For ioctl() (surprise, surprise)   */
#include <fcntl.h>	/* For open(), etc.		      */
#include <string.h>	/* strstr(), strcpy(), etc.	      */
#include <signal.h>	/* sigaction(), sigprocmask(), etc.   */
#ifndef SCO_FLAVOR
#	include <sys/time.h>	/* select(), gettimeofday(), etc.     */
#endif /* SCO_FLAVOR */
#include <sys/stat.h>	/* Required by dir.h & panel.h below  */
#include <sys/param.h>	/* Required by panel.h below	      */
#include "tty.h"

#ifdef HAVE_UNISTD_H
#   include <unistd.h>	/* For pipe, fork, setsid, access etc */
#endif

#ifdef HAVE_SYS_SELECT_H
#   include <sys/select.h>
#endif

#ifdef HAVE_SYS_WAIT_H
#   include <sys/wait.h> /* For waitpid() */
#endif

#ifndef WEXITSTATUS
#   define WEXITSTATUS(stat_val) ((unsigned)(stat_val) >> 8)
#endif

#ifndef WIFEXITED
#   define WIFEXITED(stat_val) (((stat_val) & 255) == 0)
#endif

#ifdef HAVE_GRANTPT
#   include <stropts.h> /* For I_PUSH			      */
#else
#   include <grp.h>	/* For the group struct & getgrnam()  */
#endif

#ifdef SCO_FLAVOR
#   include <grp.h>	/* For the group struct & getgrnam()  */
#endif /* SCO_FLAVOR */

#ifdef __QNX__
#   include <unix.h>	/* exec*() from <process.h> */
#endif

#include "dir.h"	/* Required by panel.h below	      */
#include "util.h"	/* Required by panel.h		      */
#include "panel.h"	/* For WPanel and current_panel	      */
#include "dialog.h"	/* For query_dialog()		      */
#include "main.h"	/* For cpanel, quit & init_sigchld()  */
#include "global.h"	/* For home_dir			      */
#include "cons.saver.h"	/* For handle_console(), etc.	      */
#include "key.h"	/* XCTRL and ALT macros		      */
#include "subshell.h"

/* Local functions */
static int feed_subshell (int how, int fail_on_error);
static void synchronize (void);
static int pty_open_master (char *pty_name);
static int pty_open_slave (const char *pty_name);

/* }}} */
/* {{{ Definitions */

#ifndef STDIN_FILENO
#    define STDIN_FILENO 0
#endif

#ifndef STDOUT_FILENO
#    define STDOUT_FILENO 1
#endif

#ifndef STDERR_FILENO
#    define STDERR_FILENO 2
#endif

/* If using a subshell for evaluating commands this is true */
int use_subshell =
#ifdef SUBSHELL_OPTIONAL
FALSE;
#else
TRUE;
#endif

/* File descriptor of the pseudoterminal used by the subshell */
int subshell_pty = 0;

/* If true, the child forked in init_subshell will wait in a loop to be attached by gdb */
int debug_subshell = 0;

/* The key for switching back to MC from the subshell */
char subshell_switch_key = XCTRL('o');

/* State of the subshell:
 * INACTIVE: the default state; awaiting a command
 * ACTIVE: remain in the shell until the user hits `subshell_switch_key'
 * RUNNING_COMMAND: return to MC when the current command finishes */
enum subshell_state_enum subshell_state;

/* Holds the latest prompt captured from the subshell */
char *subshell_prompt = NULL;

/* Initial length of the buffer for the subshell's prompt */
#define INITIAL_PROMPT_SIZE 10

/* Used by the child process to indicate failure to start the subshell */
#define FORK_FAILURE 69  /* Arbitrary */

/* Initial length of the buffer for all I/O with the subshell */
#define INITIAL_PTY_BUFFER_SIZE 100  /* Arbitrary; but keep it >= 80 */

/* For pipes */
enum {READ=0, WRITE=1};


/* Local variables */

static char *pty_buffer;	/* For reading/writing on the subshell's pty */
static int pty_buffer_size;	/* The buffer grows as needed */
static int subshell_pipe[2];	/* To pass CWD info from the subshell to MC */
static pid_t subshell_pid = 1;	/* The subshell's process ID */
static char subshell_cwd[MC_MAXPATHLEN+1];  /* One extra char for final '\n' */

/* Subshell type (gleaned from the SHELL environment variable, if available) */
static enum {BASH, TCSH, ZSH} subshell_type;

/* Flag to indicate whether the subshell is ready for next command */
static int subshell_ready;

/* The following two flags can be changed by the SIGCHLD handler. This is */
/* OK, because the `int' type is updated atomically on all known machines */
static volatile int subshell_alive, subshell_stopped;

/* We store the terminal's initial mode here so that we can configure
   the pty similarly, and also so we can restore the real terminal to
   sanity if we have to exit abruptly */
static struct termios shell_mode;

/* This counter indicates how many characters of prompt we have read */
/* FIXME: try to figure out why this had to become global */
static int prompt_pos;

/* }}} */

/* {{{ init_subshell */

/*
 *  Fork the subshell, and set up many, many things.
 *
 *  Possibly modifies the global variables:
 *	shell_mode
 *	subshell_type, subshell_alive, subshell_stopped, subshell_pid
 *	use_subshell - Is set to FALSE if we can't run the subshell
 *	quit - Can be set to SUBSHELL_EXIT by the SIGCHLD handler
 */

#ifdef HAVE_GRANTPT
#    define SYNC_PTY_SIDES
#else
#    define SYNC_PTY_SIDES
#endif

#undef SYNC_PTY_SIDES

#ifdef SYNC_PTY_SIDES
/* Handler for SIGUSR1 (used below), does nothing but accept the signal */
static void sigusr1_handler (int sig)
{
}
#endif

void init_subshell (void)
{
    /* {{{ Local variables */

    /* This must be remembered across calls to init_subshell() */
    static char pty_name[40];
    int pty_slave;

    /* Braindead tcsh can't redirect output to a file descriptor? */
    char tcsh_fifo[sizeof "/tmp/mc.pipe.1234567890"];

    
#ifdef SYNC_PTY_SIDES
	/* Used to wait for a SIGUSR1 signal from the subprocess */
	sigset_t sigusr1_mask, old_mask;
#endif

    /* }}} */

    if (subshell_pty == 0)  /* First time through */
    {
	/* {{{ Find out what type of shell we have */

	if (strstr (shell, "/zsh"))
	    subshell_type = ZSH;
	else if (strstr (shell, "/tcsh"))
	    subshell_type = TCSH;
	else if (strstr (shell, "/bash") || getenv ("BASH"))
	    subshell_type = BASH;
	else
	{
	    use_subshell = FALSE;
	    return;
	}

	/* }}} */
	/* {{{ Open a pty for talking to the subshell */

	/* FIXME: We may need to open a fresh pty each time on SVR4 */

	subshell_pty = pty_open_master (pty_name);
	if (subshell_pty == -1)
	{
	    fputs (__FILE__": couldn't open master side of pty\n", stderr);
	    perror ("pty_open_master");
	    use_subshell = FALSE;
	    return;
	}
	pty_slave = pty_open_slave (pty_name);
	if (pty_slave == -1)
	{
	    fprintf (stderr, "couldn't open slave side of pty (%s)\n\r",
		     pty_name);
	    use_subshell = FALSE;
	    return;
	}


	/* }}} */
	/* {{{ Initialise the pty's I/O buffer */

	pty_buffer_size = INITIAL_PTY_BUFFER_SIZE;
	pty_buffer = (char *) malloc (pty_buffer_size);

	/* }}} */
	/* {{{ Create a pipe for receiving the subshell's CWD */

	if (subshell_type == TCSH)
	{
	    sprintf (tcsh_fifo, "/tmp/mc.pipe.%d", getpid ());
	    if (mkfifo (tcsh_fifo, 0600) == -1)
	    {
		perror (__FILE__": mkfifo");
		use_subshell = FALSE;
		return;
	    }

	    /* Opening the FIFO as O_RDONLY or O_WRONLY causes deadlock */

	    if ((subshell_pipe[READ] = open (tcsh_fifo, O_RDWR)) == -1 ||
		(subshell_pipe[WRITE] = open (tcsh_fifo, O_RDWR)) == -1)
	    {
		fprintf (stderr, _("Couldn't open named pipe %s\n"), tcsh_fifo);
		perror (__FILE__": open");
		use_subshell = FALSE;
		return;
	    }
	}
	else  /* subshell_type is BASH or ZSH */
	    if (pipe (subshell_pipe))
	    {
		perror (__FILE__": couldn't create pipe");
		use_subshell = FALSE;
		return;
	    }

	/* }}} */
    }

    /* {{{ Define a handler for the sigusr1 signal */

#ifdef SYNC_PTY_SIDES
	sigemptyset (&sigusr1_mask);
	sigaddset (&sigusr1_mask, SIGUSR1);
	sigprocmask (SIG_BLOCK, &sigusr1_mask, &old_mask);
	signal (SIGUSR1, sigusr1_handler);
#endif

    /* }}} */
    /* {{{ Fork the subshell */

    subshell_alive = TRUE;
    subshell_stopped = FALSE;
    subshell_pid = fork ();
    
    if (subshell_pid == -1)
    {
	perror (__FILE__": couldn't spawn the subshell process");
	/* We exit here because, if the process table is full, the */
	/* other method of running user commands won't work either */
	exit (1);
    }

   /* }}} */

    if (subshell_pid == 0)  /* We are in the child process */
    {
	char *init_file = NULL;
	
	setsid ();  /* Get a fresh terminal session */

	/* {{{ Open the slave side of the pty: again */
	pty_slave = pty_open_slave (pty_name);

	/* This must be done before closing the master side of the pty, */
	/* or it will fail on certain idiotic systems, such as Solaris.	*/

	/* Close master side of pty.  This is important; apart from	*/
	/* freeing up the descriptor for use in the subshell, it also	*/
	/* means that when MC exits, the subshell will get a SIGHUP and	*/
	/* exit too, because there will be no more descriptors pointing	*/
	/* at the master side of the pty and so it will disappear.	*/

	close (subshell_pty);

#ifdef SYNC_PTY_SIDES
	    /* Give our parent process (MC) the go-ahead */
	    kill (getppid (), SIGUSR1);
#endif

	/* }}} */
	/* {{{ Make sure that it has become our controlling terminal */

	/* Redundant on Linux and probably most systems, but just in case: */

#	ifdef TIOCSCTTY
	ioctl (pty_slave, TIOCSCTTY, 0);
#	endif

	/* }}} */
	/* {{{ Configure its terminal modes and window size */

	/* Set up the pty with the same termios flags as our own tty, plus  */
	/* TOSTOP, which keeps background processes from writing to the pty */

	shell_mode.c_lflag |= TOSTOP;  /* So background writers get SIGTTOU */
	if (tcsetattr (pty_slave, TCSANOW, &shell_mode))
	{
	    perror (__FILE__": couldn't set pty terminal modes");
	    _exit (FORK_FAILURE);
	}

	/* Set the pty's size (80x25 by default on Linux) according to the */
	/* size of the real terminal as calculated by ncurses, if possible */
#	if defined TIOCSWINSZ && !defined SCO_FLAVOR
	{
	    struct winsize tty_size;
	    tty_size.ws_row = LINES;
	    tty_size.ws_col = COLS;
	    tty_size.ws_xpixel = tty_size.ws_ypixel = 0;

	    if (ioctl (pty_slave, TIOCSWINSZ, &tty_size))
		perror (__FILE__": couldn't set pty size");
	}
#	endif

	/* }}} */
	/* {{{ Set up the subshell's environment and init file name */

	/* It simplifies things to change to our home directory here, */
	/* and the user's startup file may do a `cd' command anyway   */
	chdir (home_dir);  /* FIXME? What about when we re-run the subshell? */

	switch (subshell_type)
	{
	    case BASH:
		init_file = ".mc/bashrc";
		if (access (init_file, R_OK) == -1)
		    init_file = ".bashrc";

		/* Make MC's special commands not show up in bash's history */
		putenv ("HISTCONTROL=ignorespace");

		/* Allow alternative readline settings for MC */
		if (access (".mc/inputrc", R_OK) == 0)
		    putenv ("INPUTRC=.mc/inputrc");

		break;

	    case TCSH:
		init_file = ".mc/tcshrc";
		if (access (init_file, R_OK) == -1)
		    init_file += 3;
		break;

	    case ZSH:
		break;

	    default:
		fprintf (stderr, __FILE__": unimplemented subshell type %d\n",
			 subshell_type);
		_exit (FORK_FAILURE);
	}

	/* }}} */
	/* {{{ Attach all our standard file descriptors to the pty */

	/* This is done just before the fork, because stderr must still	 */
	/* be connected to the real tty during the above error messages; */
	/* otherwise the user will never see them.			 */

	dup2 (pty_slave, STDIN_FILENO);
	dup2 (pty_slave, STDOUT_FILENO);
	dup2 (pty_slave, STDERR_FILENO);

	/* }}} */
	/* {{{ Execute the subshell at last */

	close (subshell_pipe[READ]);
	close (pty_slave);  /* These may be FD_CLOEXEC, but just in case... */

	switch (subshell_type)
	{
	    case BASH:
	    execl (shell, "bash", "-rcfile", init_file, NULL);
	    break;

	    case TCSH:
	    execl (shell, "tcsh", NULL);  /* What's the -rcfile equivalent? */
	    break;

	    case ZSH:
	    execl (shell, "zsh", "+Z", NULL);
	    break;
	}

	/* If we get this far, everything failed miserably */
	_exit (FORK_FAILURE);

	/* }}} */
    }

    close(pty_slave);

#ifdef SYNC_PTY_SIDES
	sigsuspend (&old_mask);
	signal (SIGUSR1, SIG_DFL);
	sigprocmask (SIG_SETMASK, &old_mask, NULL);
	/* ...before installing our handler for SIGCHLD. */
#endif

#if 0
    /* {{{ Install our handler for SIGCHLD */

    init_sigchld ();

    /* We could have received the SIGCHLD signal for the subshell 
     * before installing the init_sigchld */
    pid = waitpid (subshell_pid, &status, WUNTRACED | WNOHANG);
    if (pid == subshell_pid){
	use_subshell = FALSE;
	return;
    }

    /* }}} */
#endif

    /* {{{ Set up `precmd' or equivalent for reading the subshell's CWD */

    switch (subshell_type)
    {
	char precmd[80];

	case BASH:
	sprintf (precmd, " PROMPT_COMMAND='pwd>&%d;kill -STOP $$'\n",
		 subshell_pipe[WRITE]);
	goto write_it;

	case ZSH:
	sprintf (precmd, "precmd(){ pwd>&%d;kill -STOP $$ }\n",
		 subshell_pipe[WRITE]);
	goto write_it;

	case TCSH:
	sprintf (precmd, "alias precmd 'echo $cwd:q >>%s;kill -STOP $$'\n", tcsh_fifo);

	write_it:
	write (subshell_pty, precmd, strlen (precmd));
    }

    /* }}} */
    /* {{{ Wait until the subshell has started up and processed the command */

    subshell_state = RUNNING_COMMAND;
    enable_interrupt_key ();
    if (!feed_subshell (QUIETLY, TRUE)){
	use_subshell = FALSE;
    }
    disable_interrupt_key ();
    if (!subshell_alive)
	use_subshell = FALSE;  /* Subshell died instantly, so don't use it */

    /* }}} */
}

/* }}} */
/* {{{ invoke_subshell */

int invoke_subshell (const char *command, int how, char **new_dir)
{
    /* {{{ Fiddle with terminal modes */

    static struct termios raw_mode = {0};
    
    /* MC calls reset_shell_mode() in pre_exec() to set the real tty to its */
    /* original settings.  However, here we need to make this tty very raw, */
    /* so that all keyboard signals, XON/XOFF, etc. will get through to the */
    /* pty.  So, instead of changing the code for execute(), pre_exec(),    */
    /* etc, we just set up the modes we need here, before each command.     */

    if (raw_mode.c_iflag == 0)  /* First time: initialise `raw_mode' */
    {
	tcgetattr (STDOUT_FILENO, &raw_mode);
	raw_mode.c_lflag &= ~ICANON;  /* Disable line-editing chars, etc.   */
	raw_mode.c_lflag &= ~ISIG;    /* Disable intr, quit & suspend chars */
	raw_mode.c_lflag &= ~ECHO;    /* Disable input echoing		    */
	raw_mode.c_iflag &= ~IXON;    /* Pass ^S/^Q to subshell undisturbed */
	raw_mode.c_iflag &= ~ICRNL;   /* Don't translate CRs into LFs	    */
	raw_mode.c_oflag &= ~OPOST;   /* Don't postprocess output	    */
	raw_mode.c_cc[VTIME] = 0;     /* IE: wait forever, and return as    */
	raw_mode.c_cc[VMIN] = 1;      /* soon as a character is available   */
    }

    tcsetattr (STDOUT_FILENO, TCSANOW, &raw_mode);

    /* }}} */
    
    /* Make the subshell change to MC's working directory */
    if (new_dir)
	do_subshell_chdir (cpanel->cwd, TRUE, 1);
    
    if (command == NULL)  /* The user has done "C-o" from MC */
    {
	if (subshell_state == INACTIVE)
	{
	    subshell_state = ACTIVE;
	    /* FIXME: possibly take out this hack; the user can
	       re-play it by hitting C-hyphen a few times! */
	    write (subshell_pty, " \b", 2);  /* Hack to make prompt reappear */
	}
    }
    else  /* MC has passed us a user command */
    {
	if (how == QUIETLY)
	    write (subshell_pty, " ", 1);
	write (subshell_pty, command, strlen (command));
	write (subshell_pty, "\n", 1);
	subshell_state = RUNNING_COMMAND;
	subshell_ready = FALSE;
    }

    feed_subshell (how, FALSE);

    if (new_dir && subshell_alive && strcmp (subshell_cwd, cpanel->cwd))
	*new_dir = subshell_cwd;  /* Make MC change to the subshell's CWD */

    /* Restart the subshell if it has died by SIGHUP, SIGQUIT, etc. */
    while (!subshell_alive && !quit && use_subshell)
	init_subshell ();

    prompt_pos = 0;

    return quit;
}

/* }}} */
/* {{{ read_subshell_prompt */

int read_subshell_prompt (int how)
{
    /* {{{ Local variables */

    int clear_now = FALSE;
    static int prompt_size = INITIAL_PROMPT_SIZE;
    int bytes = 0, i, rc = 0;
    struct timeval timeleft = {0, 0};

    fd_set tmp;
    FD_ZERO (&tmp);
    FD_SET (subshell_pty, &tmp);

    /* }}} */

    if (subshell_prompt == NULL)  /* First time through */
    {
	subshell_prompt = (char *) malloc (prompt_size);
	*subshell_prompt = '\0';
	prompt_pos = 0;
    }

    while (subshell_alive &&
	   (rc = select (FD_SETSIZE, &tmp, NULL, NULL, &timeleft)))
    {
	/* {{{ Check for `select' errors */

	if (rc == -1)
	    if (errno == EINTR)
		continue;
	    else
	    {
		tcsetattr (STDOUT_FILENO, TCSANOW, &shell_mode);
		perror ("\n"__FILE__": select (FD_SETSIZE, &tmp...)");
		exit (1);
	    }

	/* }}} */

	bytes = read (subshell_pty, pty_buffer, pty_buffer_size);
	if (how == VISIBLY)
	    write (STDOUT_FILENO, pty_buffer, bytes);

	/* {{{ Extract the prompt from the shell output */

	for (i=0; i<bytes; ++i)
	    if (pty_buffer[i] == '\n' || pty_buffer[i] == '\r'){
		prompt_pos = 0;
		clear_now = FALSE;
	    } else {
		clear_now = TRUE;
		if (!pty_buffer [i])
		    continue;
		
		subshell_prompt[prompt_pos++] = pty_buffer[i];
		if (prompt_pos == prompt_size)
		    subshell_prompt = (char *) realloc (subshell_prompt,
							prompt_size *= 2);
	    }

	/* Sometimes we get an empty new line and then nothing,
	 * we better just keep the old prompt instead. */
	if (clear_now)
	    subshell_prompt[prompt_pos] = '\0';

	/* }}} */
    }
    if (rc == 0 && bytes == 0)
	return FALSE;
    return TRUE;
}

/* }}} */
/* {{{ resize_subshell */

void resize_subshell (void)
{
#if defined TIOCSWINSZ && !defined SCO_FLAVOR
    struct winsize tty_size;

    tty_size.ws_row = LINES;
    tty_size.ws_col = COLS;
    tty_size.ws_xpixel = tty_size.ws_ypixel = 0;

    if (ioctl (subshell_pty, TIOCSWINSZ, &tty_size))
	perror (__FILE__": couldn't set pty size");
#endif
}

/* }}} */
/* {{{ exit_subshell */

int exit_subshell (void)
{
    int quit = TRUE;
    
    if (subshell_state != INACTIVE && subshell_alive)
	quit = !query_dialog (_(" Warning "), _(" The shell is still active. Quit anyway? "),
			      0, 2, _("&Yes"), _("&No"));

#if AIX_TCSH_CODE_BELOW_IS_IT_FIXED
    /* New Test code */
    else
    {
	if (subshell_type == TCSH)
	    sprintf (pty_buffer, " echo -n Jobs:>/tmp/mc.pipe.%d;jobs>/tmp/"
		     "mc.pipe.%d;kill -STOP $$\n", getpid (), getpid ());
	else
	    sprintf (pty_buffer, " echo -n Jobs:>&%d;jobs>&%d;kill -STOP $$\n",
		     subshell_pipe[WRITE], subshell_pipe[WRITE]);
	write (subshell_pty, pty_buffer, strlen (pty_buffer));

#ifndef HAVE_GRANTPT  /* FIXME */
	if (subshell_type == ZSH)
	    /* Here we have to drain the shell output, because zsh does a  */
	    /* tcsetattr(SHTTY, TCSADRAIN...) which will block if we don't */
	    read (subshell_pty, pty_buffer, pty_buffer_size);
#endif

	/* TCSH + AIX hang here, fix this before removing the ifdef above */
	if (read (subshell_pipe[READ], pty_buffer, pty_buffer_size) == 5)
	    quit = TRUE;
	else
	    quit = !query_dialog (_(" Warning "), _(" There are stopped jobs.")
				  _(" Quit anyway? "), 0, 2, _("&Yes"), _("&No"));

	synchronize ();
	subshell_state = RUNNING_COMMAND;
	feed_subshell (QUIETLY, FALSE);  /* Drain the shell output (again) */
    }
#endif

    if (quit && subshell_type == TCSH)
    {
	/* We abuse of pty_buffer here, but it doesn't matter at this stage */
	sprintf (pty_buffer, "/tmp/mc.pipe.%d", getpid ());
	if (unlink (pty_buffer) == -1)
	    perror (__FILE__": couldn't remove named pipe /tmp/mc.pipe.NNN");
    }
    
    return quit;
}

/* }}} */

/* {{{ do_subshell_chdir */
/* If it actually changed the directory it returns true */
void do_subshell_chdir (char *directory, int do_update, int reset_prompt)
{
    char *temp;
    
    if (!(subshell_state == INACTIVE && strcmp (subshell_cwd, cpanel->cwd))){
	/* We have to repaint the subshell prompt if we read it from
	 * the main program.  Please note that in the code after this
	 * if, the cd command that is sent will make the subshell
	 * repaint the prompt, so we don't have to paint it. */
	if (do_update)
	    do_update_prompt ();
	return;
    }
    
    /* The initial space keeps this out of the command history (in bash
       because we set "HISTCONTROL=ignorespace") */
    write (subshell_pty, " cd ", 4);
    if (*directory) {
	temp = name_quote (directory, 0);
	write (subshell_pty, temp, strlen (temp));    
        free (temp);
    } else {
	write (subshell_pty, "/", 1);
    }
    write (subshell_pty, "\n", 1);
    
    subshell_state = RUNNING_COMMAND;
    feed_subshell (QUIETLY, FALSE);
    
    if (subshell_alive && strcmp (subshell_cwd, cpanel->cwd) && strcmp (cpanel->cwd, "."))
	fprintf (stderr, _("Warning: Couldn't change to %s.\n"), cpanel->cwd);

    if (reset_prompt)
	prompt_pos = 0;
    update_prompt = FALSE;
    /* Make sure that MC never stores the CWD in a silly format */
    /* like /usr////lib/../bin, or the strcmp() above will fail */
}

/* }}} */
/* {{{ subshell_get_console_attributes */

void subshell_get_console_attributes (void)
{
    /* {{{ Get our current terminal modes */

    if (tcgetattr (STDOUT_FILENO, &shell_mode))
    {
	perror (__FILE__": couldn't get terminal settings");
	use_subshell = FALSE;
	return;
    }

    /* }}} */
}

/* }}} */
/* {{{ sigchld_handler */

/* Figure out whether the subshell has stopped, exited or been killed */
/* Possibly modifies: `subshell_alive', `subshell_stopped' and `quit' */

void sigchld_handler (int sig)
{
    int pid, status;

    pid = waitpid (subshell_pid, &status, WUNTRACED | WNOHANG);

    if (pid == subshell_pid) {
	/* {{{ Figure out what has happened to the subshell */

	if (WIFSTOPPED (status))
	{
	    if (WSTOPSIG (status) == SIGTSTP)
		/* The user has suspended the subshell.  Revive it */
		kill (subshell_pid, SIGCONT);
	    else
		/* The subshell has received a SIGSTOP signal */
		subshell_stopped = TRUE;
	}
	else  /* The subshell has either exited normally or been killed */
	{
	    subshell_alive = FALSE;
	    if (WIFEXITED (status) && WEXITSTATUS (status) != FORK_FAILURE)
		quit |= SUBSHELL_EXIT;  /* Exited normally */
	}

	/* }}} */
    }

#ifndef HAVE_X
#ifndef SCO_FLAVOR
    pid = waitpid (cons_saver_pid, &status, WUNTRACED | WNOHANG);
    
    if (pid == cons_saver_pid) {
	/* {{{ Someone has stopped or killed cons.saver; restart it */

	if (WIFSTOPPED (status))
	    kill (pid, SIGCONT);
	else
	{
	    handle_console (CONSOLE_DONE);
	    handle_console (CONSOLE_INIT);
	    /* Ought to do: if (in_subshell) handle_console (CONSOLE_SAVE)
	       Can't do this without adding a new variable `in_subshell';
	       it hardly seems to be worth the trouble. */
	}

	/* }}} */
    }
#endif /* ! SCO_FLAVOR */
#endif /* ! HAVE_X */
    /* If we get here, some other child exited; ignore it */
}

/* }}} */

/* {{{ feed_subshell */

/* Feed the subshell our keyboard input until it says it's finished */

static int feed_subshell (int how, int fail_on_error)
{
    /* {{{ Local variables */
    fd_set read_set;	/* For `select' */
    int bytes;		/* For the return value from `read' */
    int i;		/* Loop counter */
    
    struct timeval wtime; /* Maximum time we wait for the subshell */
    struct timeval *wptr;
    /* }}} */

    /* we wait up to 10 seconds if fail_on_error */
    wtime.tv_sec = 10;
    wtime.tv_usec = 0;
    
    for (wptr = fail_on_error ? &wtime : NULL;;)
    {
	if (!subshell_alive)
	    return FALSE;

	/* {{{ Prepare the file-descriptor set and call `select' */

	FD_ZERO (&read_set);
	FD_SET (subshell_pty, &read_set);
	FD_SET (subshell_pipe[READ], &read_set);
	if (how == VISIBLY)
	    FD_SET (STDIN_FILENO, &read_set);

	if (select (FD_SETSIZE, &read_set, NULL, NULL, wptr) == -1){

	    /* Despite using SA_RESTART, we still have to check for this */
	    if (errno == EINTR)
		continue;	/* try all over again */
	    tcsetattr (STDOUT_FILENO, TCSANOW, &shell_mode);
	    perror ("\n"__FILE__": select (FD_SETSIZE, &read_set...)");
	    exit (1);
	}
	/* }}} */

	/* From now on: block forever on the select call */
	wptr = NULL;
	
	if (FD_ISSET (subshell_pty, &read_set))
	    /* {{{ Read from the subshell, write to stdout */

	    /* This loop improves performance by reducing context switches
               by a factor of 20 or so... unfortunately, it also hangs MC
               randomly, because of an apparent Linux bug.  Investigate. */
	    /* for (i=0; i<5; ++i)  * FIXME -- experimental */
	    {
		bytes = read (subshell_pty, pty_buffer, pty_buffer_size);
		if (bytes == -1 && errno != EIO)
		{
		    tcsetattr (STDOUT_FILENO, TCSANOW, &shell_mode);
		    perror ("\n"__FILE__": read (subshell_pty...)");
		    exit (1);
		}
		if (how == VISIBLY)
		    write (STDOUT_FILENO, pty_buffer, bytes);
	    }

	    /* }}} */

	else if (FD_ISSET (subshell_pipe[READ], &read_set))
	    /* {{{ Read the subshell's CWD and capture its prompt */

	    {
		bytes = read (subshell_pipe[READ], subshell_cwd, MC_MAXPATHLEN+1);
		if (bytes == -1)
		{
		    tcsetattr (STDOUT_FILENO, TCSANOW, &shell_mode);
		    perror ("\n"__FILE__": read (subshell_pipe[READ]...)");
		    exit (1);
		}
		if (bytes >= 1)
		    subshell_cwd[bytes-1] = 0;  /* Squash the final '\n' */

		synchronize ();

		subshell_ready = TRUE;
		if (subshell_state == RUNNING_COMMAND)
		{
		    subshell_state = INACTIVE;
		    return 1;
		}
	    }

	    /* }}} */

	else if (FD_ISSET (STDIN_FILENO, &read_set))
	    /* {{{ Read from stdin, write to the subshell */

	    {
		bytes = read (STDIN_FILENO, pty_buffer, pty_buffer_size);
		if (bytes == -1)
		{
		    tcsetattr (STDOUT_FILENO, TCSANOW, &shell_mode);
		    perror ("\n"__FILE__": read (STDIN_FILENO, pty_buffer...)");
		    exit (1);
		}

		for (i=0; i<bytes; ++i)
		    if (pty_buffer[i] == subshell_switch_key)
		    {
			write (subshell_pty, pty_buffer, i);
			if (subshell_ready)
			    subshell_state = INACTIVE;
			return TRUE;
		    }

		write (subshell_pty, pty_buffer, bytes);
		subshell_ready = FALSE;
	    } else {
		return FALSE;
	    }

	    /* }}} */
    }
}

/* }}} */
/* {{{ synchronize */

/* Wait until the subshell dies or stops.  If it stops, make it resume.  */
/* Possibly modifies the globals `subshell_alive' and `subshell_stopped' */

static void synchronize (void)
{
    sigset_t sigchld_mask, old_mask;

    sigemptyset (&sigchld_mask);
    sigaddset (&sigchld_mask, SIGCHLD);
    sigprocmask (SIG_BLOCK, &sigchld_mask, &old_mask);

    /* Wait until the subshell has stopped */
    while (subshell_alive && !subshell_stopped)
	sigsuspend (&old_mask);
    subshell_stopped = FALSE;
    kill (subshell_pid, SIGCONT);

    sigprocmask (SIG_SETMASK, &old_mask, NULL);
    /* We can't do any better without modifying the shell(s) */
}

/* }}} */
/* {{{ pty opening functions */

#ifdef SCO_FLAVOR

/* {{{ SCO version of pty_open_master */

static int pty_open_master (char *pty_name)
{
    int pty_master;
    int num;
    char *ptr;

    strcpy (pty_name, "/dev/ptyp");
    ptr = pty_name+9;
    for (num=0;;num++)
    {
	sprintf(ptr,"%d",num);	/* surpriiise ... SCO lacks itoa() */
	/* Try to open master */
	if ((pty_master = open (pty_name, O_RDWR)) == -1)
	    if (errno == ENOENT)  /* Different from EIO */
		return -1;	      /* Out of pty devices */
	    else
		continue;	      /* Try next pty device */
	pty_name [5] = 't';	      /* Change "pty" to "tty" */
	if (access (pty_name, 6)){
	    close (pty_master);
	    pty_name [5] = 'p';
	    continue;
	}
	return pty_master;
    }
    return -1;  /* Ran out of pty devices */
}

/* }}} */
/* {{{ SCO version of pty_open_slave */

static int pty_open_slave (const char *pty_name)
{
    int pty_slave;
    struct group *group_info = getgrnam ("terminal");
    
    if (group_info != NULL)
    {
	/* The following two calls will only succeed if we are root */
	/* [Commented out while permissions problem is investigated] */
	/* chown (pty_name, getuid (), group_info->gr_gid);  FIXME */
	/* chmod (pty_name, S_IRUSR | S_IWUSR | S_IWGRP);   FIXME */
    }
    if ((pty_slave = open (pty_name, O_RDWR)) == -1)
	perror ("open (pty_name, O_RDWR)");
    return pty_slave;
}

/* }}} */

#elif HAVE_GRANTPT

/* {{{ System V version of pty_open_master */

static int pty_open_master (char *pty_name)
{
    char *slave_name;
    int pty_master;

    strcpy (pty_name, "/dev/ptmx");
    if ((pty_master = open (pty_name, O_RDWR)) == -1
	|| grantpt (pty_master) == -1		  /* Grant access to slave */
	|| unlockpt (pty_master) == -1		  /* Clear slave's lock flag */
	|| !(slave_name = ptsname (pty_master)))  /* Get slave's name */
    {
	close (pty_master);
	return -1;
    }
    strcpy (pty_name, slave_name);
    return pty_master;
}

/* }}} */
/* {{{ System V version of pty_open_slave */

static int pty_open_slave (const char *pty_name)
{
    int pty_slave = open (pty_name, O_RDWR);

    if (pty_slave == -1)
    {
	perror ("open (pty_name, O_RDWR)");
	return -1;
    }

#if !defined(__osf__)
    if (!ioctl (pty_slave, I_FIND, "ptem"))
	if (ioctl (pty_slave, I_PUSH, "ptem") == -1)
	{
	    perror ("ioctl (pty_slave, I_PUSH, \"ptem\")");
	    close (pty_slave);
	    return -1;
	}
	
    if (!ioctl (pty_slave, I_FIND, "ldterm"))
        if (ioctl (pty_slave, I_PUSH, "ldterm") == -1)
	{
	    perror ("ioctl (pty_slave, I_PUSH, \"ldterm\")");
	    close (pty_slave);
	    return -1;
	}

#if !defined(sgi) && !defined(__sgi)
    if (!ioctl (pty_slave, I_FIND, "ttcompat"))
        if (ioctl (pty_slave, I_PUSH, "ttcompat") == -1)
	{
	    perror ("ioctl (pty_slave, I_PUSH, \"ttcompat\")");
	    close (pty_slave);
	    return -1;
	}
#endif /* sgi || __sgi */
#endif /* __osf__ */

    return pty_slave;
}

/* }}} */

#else

/* {{{ BSD version of pty_open_master */

static int pty_open_master (char *pty_name)
{
    int pty_master;
    char *ptr1, *ptr2;

    strcpy (pty_name, "/dev/ptyXX");
    for (ptr1 = "pqrstuvwxyzPQRST"; *ptr1; ++ptr1)
    {
	pty_name [8] = *ptr1;
	for (ptr2 = "0123456789abcdef"; *ptr2; ++ptr2)
	{
	    pty_name [9] = *ptr2;

	    /* Try to open master */
	    if ((pty_master = open (pty_name, O_RDWR)) == -1)
		if (errno == ENOENT)  /* Different from EIO */
		    return -1;	      /* Out of pty devices */
		else
		    continue;	      /* Try next pty device */
	    pty_name [5] = 't';	      /* Change "pty" to "tty" */
	    if (access (pty_name, 6)){
		close (pty_master);
		pty_name [5] = 'p';
		continue;
	    }
	    return pty_master;
	}
    }
    return -1;  /* Ran out of pty devices */
}

/* }}} */
/* {{{ BSD version of pty_open_slave */

static int pty_open_slave (const char *pty_name)
{
    int pty_slave;
    struct group *group_info = getgrnam ("tty");

    if (group_info != NULL)
    {
	/* The following two calls will only succeed if we are root */
	/* [Commented out while permissions problem is investigated] */
	/* chown (pty_name, getuid (), group_info->gr_gid);  FIXME */
	/* chmod (pty_name, S_IRUSR | S_IWUSR | S_IWGRP);   FIXME */
    }
    if ((pty_slave = open (pty_name, O_RDWR)) == -1)
	perror ("open (pty_name, O_RDWR)");
    return pty_slave;
}

/* }}} */

#endif

/* }}} */

#endif /* HAVE_SUBSHELL_SUPPORT */

/* {{{ Emacs local variables */

/*
  Cause emacs to enter folding mode for this file:
  Local variables:
  end:
*/

/* }}} */

/* Various utilities - Unix variants
   Copyright (C) 1994, 1995, 1996 the Free Software Foundation.
   Written 1994, 1995, 1996 by:
   Miguel de Icaza, Janne Kukonlehto, Dugan Porter,
   Jakub Jelinek, Mauricio Plaza.

   The file_date routine is mostly from GNU's fileutils package,
   written by Richard Stallman and David MacKenzie.

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
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <fcntl.h>
#include <signal.h>		/* my_system */
#include <limits.h>		/* INT_MAX */
#include <sys/time.h>		/* select: timeout */
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdarg.h>
#ifdef HAVE_SYS_WAIT_H
#    include <sys/wait.h>	/* my_system */
#endif
#include <errno.h>		/* my_system */
#include <pwd.h>
#include <grp.h>
#include <string.h>
#include <ctype.h>
#ifdef HAVE_SYS_SELECT_H
#  include <sys/select.h>
#endif
#ifdef SCO_FLAVOR
#   include <sys/timeb.h>
#endif
#include <time.h>
#ifdef __linux__
#    if defined(__GLIBC__) && (__GLIBC__ < 2)
#        include <linux/termios.h>	/* This is needed for TIOCLINUX */
#    else
#        include <termios.h>
#    endif
#  include <sys/ioctl.h>
#endif
#ifdef __QNX__
#   include <unix.h>		/* exec*() from <process.h> */
#endif
#include "util.h"
#include "global.h"
#include "fsusage.h"
#include "fsusage.h"
#include "mountlist.h"
#include "mad.h"
#include "dialog.h"		/* message() */
#include "../vfs/vfs.h"		/* mc_read() */
#include "x.h"

struct sigaction startup_handler;
extern struct mount_entry *mount_list;

uid_t current_user_uid;
user_in_groups *current_user_gid;

int
max_open_files (void)
{
	static int files;

	if (files)
		return files;

#ifdef HAVE_SYSCONF
	files = sysconf (_SC_OPEN_MAX);
	if (files != -1)
		return files;
#endif
#ifdef OPEN_MAX
	return files = OPEN_MAX;
#else
	return files = 256;
#endif
}

void init_groups (void)
{
    int i;
    struct passwd *pwd;
    struct group *grp;
    user_in_groups *cug, *pug;

    pwd = getpwuid (current_user_uid=getuid ());

    current_user_gid = (pug = xmalloc (sizeof (user_in_groups), "init_groups"));
    current_user_gid->gid = getgid (); current_user_gid->next = 0;

    if (pwd == 0)
       return;
    
    setgrent ();
    while ((grp = getgrent ()))
       for (i = 0; grp->gr_mem[i]; i++)
           if (!strcmp (pwd->pw_name,grp->gr_mem[i]))
               {
               cug = xmalloc (sizeof (user_in_groups), "init_groups");
               cug->gid  = grp->gr_gid;
               pug->next = cug;
               cug->next = 0;
               pug = cug;
               break;
               }
    endgrent ();
}

/* Return the index of permission triplet */
int
get_user_rights (struct stat *buf)
{
    user_in_groups *cug;

    if (buf->st_uid == current_user_uid || current_user_uid == 0)
       return 0;

    for (cug = current_user_gid; cug; cug = cug->next)
       if (cug->gid == buf->st_gid) return 1;

    return 2;
}


void
delete_groups (void)
{
    user_in_groups *pug, *cug = current_user_gid;

    while (cug){
       pug = cug->next;
       free (cug);
       cug = pug;
    }
}

#define UID_CACHE_SIZE 200
#define GID_CACHE_SIZE 30

typedef struct {
    int  index;
    char *string;
} int_cache;

int_cache uid_cache [UID_CACHE_SIZE];
int_cache gid_cache [GID_CACHE_SIZE];

void init_uid_gid_cache (void)
{
    int i;

    for (i = 0; i < UID_CACHE_SIZE; i++)
	uid_cache [i].string = 0;

    for (i = 0; i < GID_CACHE_SIZE; i++)
	 gid_cache [i].string = 0;
}

static char *i_cache_match (int id, int_cache *cache, int size)
{
    int i;

    for (i = 0; i < size; i++)
	if (cache [i].index == id)
	    return cache [i].string;
    return 0;
}

static void i_cache_add (int id, int_cache *cache, int size, char *text,
			 int *last)
{
    if (cache [*last].string)
	free (cache [*last].string);
    cache [*last].string = strdup (text);
    cache [*last].index = id;
    *last = ((*last)+1) % size;
}

char *get_owner (int uid)
{
    struct passwd *pwd;
    static char ibuf [8];
    char   *name;
    static int uid_last;
    
    if ((name = i_cache_match (uid, uid_cache, UID_CACHE_SIZE)) != NULL)
	return name;
    
    pwd = getpwuid (uid);
    if (pwd){
	i_cache_add (uid, uid_cache, UID_CACHE_SIZE, pwd->pw_name, &uid_last);
	return pwd->pw_name;
    }
    else {
	sprintf (ibuf, "%d", uid);
	return ibuf;
    }
}

char *get_group (int gid)
{
    struct group *grp;
    static char gbuf [8];
    char *name;
    static int  gid_last;
    
    if ((name = i_cache_match (gid, gid_cache, GID_CACHE_SIZE)) != NULL)
	return name;
    
    grp = getgrgid (gid);
    if (grp){
	i_cache_add (gid, gid_cache, GID_CACHE_SIZE, grp->gr_name, &gid_last);
	return grp->gr_name;
    } else {
	sprintf (gbuf, "%d", gid);
	return gbuf;
    }
}

/* Since ncurses uses a handler that automatically refreshes the */
/* screen after a SIGCONT, and we don't want this behavior when */
/* spawning a child, we save the original handler here */
void save_stop_handler (void)
{
    sigaction (SIGTSTP, NULL, &startup_handler);
}

#ifdef HAVE_GNOME
#define PORT_HAS_MY_SYSTEM 1
#endif

#ifndef PORT_HAS_MY_SYSTEM
int my_system (int flags, const char *shell, const char *command)
{
    struct sigaction ignore, save_intr, save_quit, save_stop;
    pid_t pid;
    int status = 0;
    int as_shell_command = flags & EXECUTE_AS_SHELL;
    
    ignore.sa_handler = SIG_IGN;
    sigemptyset (&ignore.sa_mask);
    ignore.sa_flags = 0;
    
    sigaction (SIGINT, &ignore, &save_intr);    
    sigaction (SIGQUIT, &ignore, &save_quit);

    /* Restore the original SIGTSTP handler, we don't want ncurses' */
    /* handler messing the screen after the SIGCONT */
    sigaction (SIGTSTP, &startup_handler, &save_stop);

    if ((pid = fork ()) < 0){
	fprintf (stderr, "\n\nfork () = -1\n");
	return -1;
    }
    if (pid == 0){
	sigaction (SIGINT,  &save_intr, NULL);
	sigaction (SIGQUIT, &save_quit, NULL);

#if 0
	prepare_environment ();
#endif
	
	if (as_shell_command)
	    execl (shell, shell, "-c", command, (char *) 0);
	else
	    execlp (shell, shell, command, (char *) 0);

	_exit (127);		/* Exec error */
    } else {
	while (waitpid (pid, &status, 0) < 0)
	    if (errno != EINTR){
		status = -1;
		break;
	    }
    }
    sigaction (SIGINT,  &save_intr, NULL);
    sigaction (SIGQUIT, &save_quit, NULL);
    sigaction (SIGTSTP, &save_stop, NULL);

#ifdef SCO_FLAVOR 
	waitpid(-1, NULL, WNOHANG);
#endif /* SCO_FLAVOR */

    return WEXITSTATUS(status);
}
#endif

/* Returns a newly allocated string, if directory does not exist, return 0 */
char *tilde_expand (char *directory)
{
    struct passwd *passwd;
    char *p;
    char *name;
    int  len;
    
    if (*directory != '~')
	return strdup (directory);

    directory++;
    
    p = strchr (directory, PATH_SEP);
    
    /* d = "~" or d = "~/" */
    if (!(*directory) || (*directory == PATH_SEP)){
	passwd = getpwuid (geteuid ());
	p = (*directory == PATH_SEP) ? directory+1 : "";
    } else {
	if (!p){
	    p = "";
	    passwd = getpwnam (directory);
	} else {
	    name = xmalloc (p - directory + 1, "tilde_expand");
	    strncpy (name, directory, p - directory);
	    name [p - directory] = 0;
	    passwd = getpwnam (name);
	    free (name);
	}
    }

    /* If we can't figure the user name, return NULL */
    if (!passwd)
	return 0;

    len = strlen (passwd->pw_dir) + strlen (p) + 2;
    directory = xmalloc (len, "tilde_expand");
    strcpy (directory, passwd->pw_dir);
    strcat (directory, PATH_SEP_STR);
    strcat (directory, p);
    return directory;
}

int
set_nonblocking (int fd)
{
    int val;

    val = fcntl (fd, F_GETFL, 0);
    val |= O_NONBLOCK;
    return fcntl (fd, F_SETFL, val) != -1;
}

/* Pipes are guaranteed to be able to hold at least 4096 bytes */
/* More than that would be unportable */
#define MAX_PIPE_SIZE 4096

static int error_pipe[2];	/* File descriptors of error pipe */
static int old_error;		/* File descriptor of old standard error */

/* Creates a pipe to hold standard error for a later analysis. */
/* The pipe can hold 4096 bytes. Make sure no more is written */
/* or a deadlock might occur. */
void open_error_pipe (void)
{
    if (pipe (error_pipe) < 0){
	message (0, _(" Warning "), _(" Pipe failed "));
    }
    old_error = dup (2);
    if(old_error < 0 || close(2) || dup (error_pipe[1]) != 2){
	message (0, _(" Warning "), _(" Dup failed "));
	close (error_pipe[0]);
	close (error_pipe[1]);
    }
    close (error_pipe[1]);
}

void close_error_pipe (int error, char *text)
{
    char *title;
    char msg[MAX_PIPE_SIZE];
    int len = 0;

    if (error)
	title = MSG_ERROR;
    else
	title = " Warning ";
    if (old_error >= 0){
	close (2);
	dup (old_error);
	close (old_error);
	len = read (error_pipe[0], msg, MAX_PIPE_SIZE);

	if (len >= 0)
	    msg[len] = 0;
	close (error_pipe[0]);
    }
    if (error < 0)
	return;		/* Just ignore error message */
    if (text == NULL){
	if (len == 0) return;	/* Nothing to show */

	/* Show message from pipe */
	message (error, title, msg);
    } else {
	/* Show given text and possible message from pipe */
	message (error, title, " %s \n %s ", text, msg);
    }
}

/* Checks for messages in the error pipe,
 * closes the pipe and displays an error box if needed
 */
void check_error_pipe (void)
{
    char error[MAX_PIPE_SIZE];
    int len = 0;
    if (old_error >= 0){
	while (len < MAX_PIPE_SIZE)
	{
            fd_set select_set;
            struct timeval timeout;
            FD_ZERO (&select_set);
            FD_SET (error_pipe[0], &select_set);
            timeout.tv_sec = 0;
            timeout.tv_usec = 0;
            select (FD_SETSIZE, &select_set, 0, 0, &timeout);
            if (!FD_ISSET (0, &select_set))
		break;
	    read (error_pipe[0], error + len, 1);
	    len ++;
	}
	error[len] = 0;
	close (error_pipe[0]);
    }
    if (len > 0)
        message (0, _(" Warning "), error);
}

static struct sigaction ignore, save_intr, save_quit, save_stop;

/* INHANDLE is a result of some mc_open call to any vfs, this function
   returns a normal handle (to be used with read) of a pipe for reading
   of the output of COMMAND with arguments ... (must include argv[0] as
   well) which gets as its input at most INLEN bytes from the INHANDLE
   using mc_read. You have to call mc_doublepclose to close the returned
   handle afterwards. If INLEN is -1, we read as much as we can :) */
int mc_doublepopen (int inhandle, int inlen, pid_t *the_pid, char *command, ...)
{
    int pipe0 [2], pipe1 [2];
    pid_t pid;

#define closepipes() close(pipe0[0]);close(pipe0[1]);close(pipe1[0]);close(pipe1[1])
#define is_a_pipe_fd(f) ((pipe0[0] == f) || (pipe0[1] == f) || (pipe1[0] == f) || (pipe1[1] == f))
    
    pipe (pipe0); pipe (pipe1);
    ignore.sa_handler = SIG_IGN;
    sigemptyset (&ignore.sa_mask);
    ignore.sa_flags = 0;
        
    sigaction (SIGINT, &ignore, &save_intr);
    sigaction (SIGQUIT, &ignore, &save_quit);
    sigaction (SIGTSTP, &startup_handler, &save_stop);

    switch (pid = fork ()) {
    case -1:
            closepipes ();
	    return -1;
    case 0: {
	    sigaction (SIGINT, &save_intr, NULL);
	    sigaction (SIGQUIT, &save_quit, NULL);

	    switch (pid = fork ()) {
	    	case -1:
	    	    closepipes ();
	    	    exit (1);
	    	case 0: {
#define MAXARGS 16
		     int argno;
		     char *args[MAXARGS];
		     va_list ap;
		     int nulldevice;

		     port_shutdown_extra_fds ();
	    
		     nulldevice = open ("/dev/null", O_WRONLY);
		     close (0);
		     dup (pipe0 [0]);
		     close (1);
		     dup (pipe1 [1]);
		     close (2);
		     dup (nulldevice);
		     close (nulldevice);
		     closepipes ();
		     va_start (ap, command);
		     argno = 0;
		     while ((args[argno++] = va_arg(ap, char *)) != NULL)
		         if (argno == (MAXARGS - 1)) {
			     args[argno] = NULL;
			     break;
		         }
		     va_end (ap);
		     execvp (command, args);
		     exit (0);
		}
	    	default:
	    	    {
	    	    	char buffer [8192];
	    	    	int i;

	    	    	close (pipe0 [0]);
	    	    	close (pipe1 [0]);
	    	    	close (pipe1 [1]);
	    	    	while ((i = mc_read (inhandle, buffer,
                                             (inlen == -1 || inlen > 8192) 
                                             ? 8192 : inlen)) > 0) {
	    	    	    write (pipe0 [1], buffer, i);
	    	    	    if (inlen != -1) {
			        inlen -= i;
			        if (!inlen)
				    break;
			    }
	    	    	}
	    	    	close (pipe0 [1]);
	   		while (waitpid (pid, &i, 0) < 0)
			    if (errno != EINTR)
				break;

			port_shutdown_extra_fds ();
	   		exit (i);
	    	    }
	    }
    }
    default:
	*the_pid = pid;
	break;
    }
    close (pipe0 [0]);
    close (pipe0 [1]);
    close (pipe1 [1]);
    return pipe1 [0];
}

int mc_doublepclose (int pipe, pid_t pid)
{
    int status = 0;
    
    close (pipe);
    waitpid (pid, &status, 0);
#ifdef SCO_FLAVOR
    waitpid (-1, NULL, WNOHANG);
#endif /* SCO_FLAVOR */
    sigaction (SIGINT, &save_intr, NULL);
    sigaction (SIGQUIT, &save_quit, NULL);
    sigaction (SIGTSTP, &save_stop, NULL);

    return status;	
}

/* Canonicalize path, and return a new path. Do everything in situ.
   The new path differs from path in:
	Multiple `/'s are collapsed to a single `/'.
	Leading `./'s and trailing `/.'s are removed.
	Trailing `/'s are removed.
	Non-leading `../'s and trailing `..'s are handled by removing
	portions of the path. */
char *canonicalize_pathname (char *path)
{
    int i, start;
    char stub_char;

    stub_char = (*path == PATH_SEP) ? PATH_SEP : '.';

    /* Walk along path looking for things to compact. */
    i = 0;
    for (;;) {
        if (!path[i])
	    break;

      	while (path[i] && path[i] != PATH_SEP)
	    i++;

      	start = i++;

      	/* If we didn't find any slashes, then there is nothing left to do. */
      	if (!path[start])
	    break;

#if defined(__QNX__)
		/*
		** QNX accesses the directories of nodes on its peer-to-peer
		** network by prefixing their directories with "//[nid]".
		** We don't want to collapse two '/'s if they're at the start
		** of the path, followed by digits, and ending with a '/'.
		*/
		if (start == 0 && i == 1)
		{
			char *p = path + 2;
			char *q = strchr(p, PATH_SEP);

			if (q > p)
			{
				*q = 0;
				if (!strcspn(p, "0123456789"))
				{
					start = q - path;
					i = start + 1;
				}
				*q = PATH_SEP;
			}
		}
#endif

        /* Handle multiple `/'s in a row. */
        while (path[i] == PATH_SEP)
	    i++;

        if ((start + 1) != i) {
	    strcpy (path + start + 1, path + i);
	    i = start + 1;
	}

        /* Handle backquoted `/'. */
        if (start > 0 && path[start - 1] == '\\')
	    continue;

        /* Check for trailing `/'. */
        if (start && !path[i]) {
	zero_last:
	    path[--i] = '\0';
	    break;
	}

        /* Check for `../', `./' or trailing `.' by itself. */
        if (path[i] == '.') {
	    /* Handle trailing `.' by itself. */
	    if (!path[i + 1])
	        goto zero_last;

	    /* Handle `./'. */
	    if (path[i + 1] == PATH_SEP) {
	        strcpy (path + i, path + i + 1);
	        i = start;
	        continue;
	    }

	    /* Handle `../' or trailing `..' by itself. 
	       Remove the previous ?/ part with the exception of
	       ../, which we should leave intact. */
	    if (path[i + 1] == '.' && (path[i + 2] == PATH_SEP || !path[i + 2])) {
	        while (--start > -1 && path[start] != PATH_SEP);
	        if (!strncmp (path + start + 1, "../", 3))
	            continue;
	        strcpy (path + start + 1, path + i + 2);
	        i = start;
	        continue;
	    }
	}
    }

    if (!*path) {
        *path = stub_char;
        path[1] = '\0';
    }
    return path;
}

#ifdef SCO_FLAVOR
int gettimeofday( struct timeval * tv, struct timezone * tz)
{
    struct timeb tb;
    struct tm * l;
    
    ftime( &tb );
    if (errno == EFAULT)
	return -1;
    l = localtime(&tb.time);
    tv->tv_sec = l->tm_sec;
    tv->tv_usec = (long) tb.millitm;
    return 0;
}
#endif /* SCO_FLAVOR */

#ifndef HAVE_PUTENV

/* The following piece of code was copied from the GNU C Library */
/* And is provided here for nextstep who lacks putenv */

extern char **environ;

#ifndef	HAVE_GNU_LD
#define	__environ	environ
#endif


/* Put STRING, which is of the form "NAME=VALUE", in the environment.  */
int
putenv (const char *string)
{
    const char *const name_end = strchr (string, '=');
    register size_t size;
    register char **ep;
    
    if (name_end == NULL){
	/* Remove the variable from the environment.  */
	size = strlen (string);
	for (ep = __environ; *ep != NULL; ++ep)
	    if (!strncmp (*ep, string, size) && (*ep)[size] == '='){
		while (ep[1] != NULL){
		    ep[0] = ep[1];
		    ++ep;
		}
		*ep = NULL;
		return 0;
	    }
    }
    
    size = 0;
    for (ep = __environ; *ep != NULL; ++ep)
	if (!strncmp (*ep, string, name_end - string) &&
	    (*ep)[name_end - string] == '=')
	    break;
	else
	    ++size;
    
    if (*ep == NULL){
	static char **last_environ = NULL;
	char **new_environ = (char **) malloc ((size + 2) * sizeof (char *));
	if (new_environ == NULL)
	    return -1;
	(void) memcpy ((void *) new_environ, (void *) __environ,
		       size * sizeof (char *));
	new_environ[size] = (char *) string;
	new_environ[size + 1] = NULL;
	if (last_environ != NULL)
	    free ((void *) last_environ);
	last_environ = new_environ;
	__environ = new_environ;
    }
    else
	*ep = (char *) string;
    
    return 0;
}
#endif /* !HAVE_PUTENV */

void my_statfs (struct my_statfs *myfs_stats, char *path)
{
    int i, len = 0;

#ifndef NO_INFOMOUNT
    struct mount_entry *entry = NULL;
    struct mount_entry *temp = mount_list;
    struct fs_usage fs_use;

    while (temp){
	i = strlen (temp->me_mountdir);
	if (i > len && (strncmp (path, temp->me_mountdir, i) == 0))
	    if (!entry || (path [i] == PATH_SEP || path [i] == 0)){
		len = i;
		entry = temp;
	    }
	temp = temp->me_next;
    }

    if (entry){
	get_fs_usage (entry->me_mountdir, &fs_use);

	myfs_stats->type = entry->me_dev;
	myfs_stats->typename = entry->me_type;
	myfs_stats->mpoint = entry->me_mountdir;
	myfs_stats->device = entry->me_devname;
	myfs_stats->avail = getuid () ? fs_use.fsu_bavail/2 : fs_use.fsu_bfree/2;
	myfs_stats->total = fs_use.fsu_blocks/2;
	myfs_stats->nfree = fs_use.fsu_ffree;
	myfs_stats->nodes = fs_use.fsu_files;
    } else
#endif
#if defined(NO_INFOMOUNT) && defined(__QNX__)
/*
** This is the "other side" of the hack to read_filesystem_list() in
** mountlist.c.
** It's not the most efficient approach, but consumes less memory. It
** also accomodates QNX's ability to mount filesystems on the fly.
*/
	struct mount_entry	*entry;
    struct fs_usage		fs_use;

	if ((entry = read_filesystem_list(0, 0)) != NULL)
	{
		get_fs_usage(entry->me_mountdir, &fs_use);

		myfs_stats->type = entry->me_dev;
		myfs_stats->typename = entry->me_type;
		myfs_stats->mpoint = entry->me_mountdir;
		myfs_stats->device = entry->me_devname;

		myfs_stats->avail = fs_use.fsu_bfree / 2;
		myfs_stats->total = fs_use.fsu_blocks / 2;
		myfs_stats->nfree = fs_use.fsu_ffree;
		myfs_stats->nodes = fs_use.fsu_files;
	}
	else
#endif
    {
	myfs_stats->type = 0;
	myfs_stats->mpoint = "unknown";
	myfs_stats->device = "unknown";
	myfs_stats->avail = 0;
	myfs_stats->total = 0;
	myfs_stats->nfree = 0;
	myfs_stats->nodes = 0;
    }
}

#ifdef HAVE_GET_PROCESS_STATS
#    include <sys/procstats.h>

int gettimeofday (struct timeval *tp, void *tzp)
{
  return get_process_stats(tp, PS_SELF, 0, 0);
}
#endif

#ifdef SCO_FLAVOR
/* Define this only for SCO */
#ifdef USE_NETCODE
#ifndef HAVE_SOCKETPAIR

/*
 The code for s_pipe function is adapted from Section 7.9
 of the "UNIX Network Programming" by W. Richard Stevens,
 published by Prentice Hall, ISBN 0-13-949876-1
 (c) 1990 by P T R Prentice Hall

 It is used to implement socketpair function for SVR3 systems
 that lack it.
*/

#include <sys/types.h>
#include <sys/stream.h>  /* defines queue_t */
#include <stropts.h>     /* defines struct strtdinsert */
#include <fcntl.h>

#define SPX_DEVICE "/dev/spx"
#define S_PIPE_HANDLE_ERRNO 1
/* if the above is defined to 1, we will attempt to
   save and restore errno to indicate failure
   reason to the caller;
   Please note that this will not work in environments
   where errno is not just an integer
*/

#if S_PIPE_HANDLE_ERRNO
#include <errno.h>
/* This is for "extern int errno;" */
#endif

 /* s_pipe returns 0 if OK, -1 on error */
 /* two file descriptors are returned   */
int s_pipe(int fd[2])
{
   struct strfdinsert  ins;  /* stream I_FDINSERT ioctl format */
   queue_t             *pointer;
   #if S_PIPE_HANDLE_ERRNO
   int err_save;
   #endif
   /*
    * First open the stream clone device "dev/spx" twice,
    * obtaining the two file descriptors
    */

   if ( (fd[0] = open(SPX_DEVICE, O_RDWR)) < 0)
      return -1;
   if ( (fd[1] = open(SPX_DEVICE, O_RDWR)) < 0) {
      #if S_PIPE_HANDLE_ERRNO
      err_save = errno;
      #endif
      close(fd[0]);
      #if S_PIPE_HANDLE_ERRNO
      errno = err_save;
      #endif
      return -1;
   }
   
   /*
    * Now link these two stream together with an I_FDINSERT ioctl.
    */
   
   ins.ctlbuf.buf     = (char *) &pointer;   /* no control information, just the pointer */
   ins.ctlbuf.maxlen  = sizeof pointer;
   ins.ctlbuf.len     = sizeof pointer;
   ins.databuf.buf    = (char *) 0;   /* no data to be sent */
   ins.databuf.maxlen = 0;
   ins.databuf.len    = -1;  /* magic: must be -1 rather than 0 for stream pipes */

   ins.fildes = fd[1];  /* the fd to connect with fd[0] */
   ins.flags  = 0;      /* nonpriority message */
   ins.offset = 0;      /* offset of pointer in control buffer */

   if (ioctl(fd[0], I_FDINSERT, (char *) &ins) < 0) {
      #if S_PIPE_HANDLE_ERRNO
      err_save = errno;
      #endif
      close(fd[0]);
      close(fd[1]);
      #if S_PIPE_HANDLE_ERRNO
      errno = err_save;
      #endif
      return -1;
   }
   /* all is OK if we came here, indicate success */
   return 0;
}

int socketpair(int dummy1, int dummy2, int dummy3, int fd[2])
{
   return s_pipe(fd);
}

#endif /* ifndef HAVE_SOCKETPAIR */
#endif /* ifdef USE_NETCODE */
#endif /* SCO_FLAVOR */

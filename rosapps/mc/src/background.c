/* {{{ Copyright */

/* Background support.
   Copyright (C) 1996 The Free Software Foundation
   
   Written by: 1996 Miguel de Icaza

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

/* }}} */

#include <config.h>
#include <stdarg.h>
#include <sys/types.h>
#ifdef HAVE_SYS_WAIT_H
#    include <sys/wait.h>
#endif
#include <errno.h>
#ifdef HAVE_UNISTD_H
#    include <unistd.h>
#endif
#include <sys/stat.h>
#include <sys/param.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include "dlg.h"
#include "widget.h"
#include "wtools.h"

/*
 * We currenlty only support one way of comunicating the background
 * and foreground process by using the socketpair system call
 */
#ifdef USE_NETCODE
#    include <sys/socket.h>
#endif
#include "tty.h"
#include "util.h"
#include "dialog.h"
#include "file.h"
#include "background.h"
#include "mad.h"
#include "key.h"	/* For add_select_channel(), delete_select_channel() */

/* If true, this is a background process */
int we_are_background = 0;

/* Ugh, ugly hack */
extern int do_append;
extern int recursive_result;

#ifdef WITH_BACKGROUND
/* If set background tasks wait to be attached */
int background_wait = 0;

#ifndef HAVE_SOCKETPAIR
int socketpair(int, int, int, int fd[2]);
#endif

/* File descriptor for talking to our parent */
static int parent_fd;

#define MAXCALLARGS 4		/* Number of arguments supported */
#define mymsg "Desde el hijo\n\r"

struct TaskList *task_list = NULL;
    
void
register_task_running (pid_t pid, int fd, char *info)
{
    TaskList *new;

    new = xmalloc (sizeof (TaskList), "note_task_running");
    new->pid   = pid;
    new->info  = info;
    new->state = Task_Running;
    new->next  = task_list;
    new->fd    = fd;
    task_list  = new;

    add_select_channel (fd, background_attention, (void *) pid);
}

void
unregister_task_running (pid_t pid, int fd)
{
    TaskList *p = task_list;
    TaskList *prev = 0;

    while (p){
	if (p->pid == pid){
	    if (prev)
		prev->next = p->next;
	    else
		task_list = p->next;
	    free (p->info);
	    free (p);
	    break;
	}
	prev = p;
	p = p->next;
    }
    delete_select_channel (fd);
}

/*
 * Try to make the Midnight Commander a background job
 *
 * Returns:
 *  1 for parent
 *  0 for child
 * -1 on failure
 */
int
do_background (char *info)
{
    int comm [2];		/* control connection stream */
    int pid;
    
    if (socketpair (AF_UNIX, SOCK_STREAM, 0, comm) == -1)
	return -1;
    
    if ((pid = fork ()) == -1)
	return -1;
    
    if (pid == 0){
	int nullfd;

	parent_fd = comm [1];
	we_are_background = 1;
	
	/* Make stdin/stdout/stderr point somewhere */
	close (0);
	close (1);
	close (2);

	if ((nullfd = open ("/dev/null", O_RDONLY)) != -1){
	    dup2 (nullfd, 0);
	    dup2 (nullfd, 1);
	    dup2 (nullfd, 2);
	}

	/* To make it obvious if it fails, there is a bug report on this */
	write (2, mymsg, sizeof (mymsg));
	write (1, mymsg, sizeof (mymsg));
	
	/* Just for debugging the background back end */
	if (background_wait){
	    volatile int i = 1;
	    
	    while (i)
		;
	}
	return 0;
    } else {
	close (comm [1]);
	register_task_running (pid, comm [0], info);
	return 1;
    }
}

char *
background_title (char *str)
{
    char *result = copy_strings (_("Background process:"), str, NULL);

    return result;
}

/* {{{ Routines that do the real job */
void
real_message_1s (enum OperationMode mode, int *flags, char *title, char *str1)
{
    char *full_title;
    
    if (mode == Background)
	full_title = background_title (title);
    else
        full_title = title;
    
    message (*flags, title, str1);

    if (title != full_title)
	free (full_title);
}

void
real_message_2s (enum OperationMode mode, int *flags, char *title, char *str1, char *str2)
{
    char *full_title;
    
    if (mode == Background)
	full_title = background_title (title);
    else
        full_title = title;
    
    message (*flags, title, str1, str2);
    
    if (title != full_title)
	free (full_title);
}

void
real_message_3s (enum OperationMode mode, int *flags, char *title, char *str1, char *str2, const char *str3)
{
    char *full_title;
    
    if (mode == Background)
	full_title = background_title (title);
    else
        full_title = title;
    
    message (*flags, title, str1, str2, str3);
    
    if (title != full_title)
	free (full_title);
}
/* }}} */

/* {{{ Parent handlers */

/* Parent/child protocol
 *
 * the child (the background) process send the following:
 * void *routine -- routine to be invoked in the parent
 * int  nargc    -- number of arguments
 * int  type     -- Return argument type.
 *
 * If the routine is zero, then it is a way to tell the parent
 * that the process is dying.
 *
 * nargc arguments in the following format:
 * int size of the coming block
 * size bytes with the block
 *
 * Now, the parent loads all those and then invokes
 * the routine with pointers to the information passed
 * (we just support pointers).
 *
 * If the return type is integer:
 *
 *     the parent then writes an int to the child with
 *     the return value from the routine and the values
 *     of any global variable that is modified in the parent
 *     currently: do_append and recursive_result.
 *
 * If the return type is a string:
 *
 *     the parent writes the resulting string lenght
 *     if the result string was NULL or the empty string,
 *     then the lenght is zero.
 *     The parent then writes the string lenght and frees
 *     the result string.
 */
/*
 * Receive requests from background process and invoke the
 * specified routine 
 */

int
background_attention (int fd, void *xpid)
{
    void *routine;
    int  argc, i, result, status;
    char *data [MAXCALLARGS];
    char *resstr;
    pid_t  pid = (pid_t) xpid;
    int bytes;
    enum ReturnType type;
    char *background_process_error = _(" Background process error ");
    
    bytes = read (fd, &routine, sizeof (routine));
    if (bytes < (sizeof (routine))){
	if (errno == ECHILD)
	    message (1, background_process_error, _(" Child died unexpectedly "));
	else
	    message (1, background_process_error, _(" Unknown error in child "));
	unregister_task_running (pid, fd);
	waitpid (pid, &status, 0);
	return 0;
    }

    /* If the routine is zero, then the child is telling us that he is dying */
    if ((int) routine == MSG_CHILD_EXITING){
	unregister_task_running (pid, fd);
	waitpid (pid, &status, 0);
	return 0;
    }
    
    read (fd, &argc, sizeof (argc));
    if (argc > MAXCALLARGS){
	message (1, _(" Background protocol error "),
		 _(" Background process sent us a request for more arguments \n"
		 " than we can handle. \n"));
    }
    read (fd, &type, sizeof (type));
    
    for (i = 0; i < argc; i++){
	int size;
	
	read (fd, &size, sizeof (size));
	data [i] = xmalloc (size+1, "RPC Arguments");
	read (fd, data [i], size);
	
	data [i][size] = 0;	/* NULL terminate the blocks (they could be strings) */
    }

    /* Handle the call */
    if (type == Return_Integer){
	switch (argc){
	case 1:
	    result = (*(int (*)(int, char *))routine)(Background, data [0]);
	    break;
	case 2:
	    result = (*(int (*)(int, char *, char *))routine)
		(Background, data [0], data [1]);
	    break;
	case 3:
	    result = (*(int (*)(int, char *, char *, char *))routine)
		(Background, data [0], data [1], data [2]);
	    break;
	case 4:
	    result = (*(int (*)(int, char *, char *, char *, char *))routine)
		(Background, data [0], data [1], data [2], data [3]);
	    break;
	}
	
	/* Send the result code and the value for shared variables */
	write (fd, &result,           sizeof (int));
	write (fd, &do_append,        sizeof (do_append));
	write (fd, &recursive_result, sizeof (recursive_result));
	
    } else if (type == Return_String) {
	int len;

	/* FIXME: string routines should also use the Foreground/Background
	 * parameter.  Currently, this is not used here
	 */
	switch (argc){
	case 1:
	    resstr = (*(char * (*)(char *))routine)(data [0]);
	    break;
	case 2:
	    resstr = (*(char * (*)(char *, char *))routine)
		(data [0], data [1]);
	    break;
	case 3:
	    resstr = (*(char * (*)(char *, char *, char *))routine)
		(data [0], data [1], data [2]);
	    break;
	case 4:
	    resstr = (*(char * (*)(char *, char *, char *, char *))routine)
		(data [0], data [1], data [2], data [3]);
	    break;
	}
	if (resstr){
	    len = strlen (resstr);
	    write (fd, &len, sizeof (len));
	    if (len){
		write (fd, resstr, len);
		free (resstr);
	    }
	} else {
	    len = 0;
	    write (fd, &len, sizeof (len));
	}
    }
    for (i = 0; i < argc; i++)
	free (data [i]);

    do_refresh ();
    mc_refresh ();
#ifndef HAVE_X
    doupdate ();
#endif
    return 0;
}


/* }}} */

/* {{{ client RPC routines */
void
parent_call_header (void *routine, int argc, enum ReturnType type)
{
    write (parent_fd, &routine, sizeof (routine));
    write (parent_fd, &argc, sizeof (int));
    write (parent_fd, &type, sizeof (type));
}

int
parent_call (void *routine, int argc, ...)
{
    va_list ap;
    int i;
    
    va_start (ap, argc);
    parent_call_header (routine, argc, Return_Integer);
    for (i = 0; i < argc; i++){
	int  len;
	void *value;

	len   = va_arg (ap, int);
	value = va_arg (ap, void *);
	write (parent_fd, &len, sizeof (int));
	write (parent_fd, value, len);
    }
    /* Besides the regular result, get the value for 
     * variables that may be modified in the parent that affect our behaviour
     */
    read (parent_fd, &i,         sizeof (int));
    read (parent_fd, &do_append, sizeof (do_append));
    read (parent_fd, &recursive_result, sizeof (recursive_result));
    return i;
}

char *
parent_call_string (void *routine, int argc, ...)
{
    va_list ap;
    char *str;
    int i;
    
    va_start (ap, argc);
    parent_call_header (routine, argc, Return_String);
    for (i = 0; i < argc; i++){
	int  len;
	void *value;

	len   = va_arg (ap, int);
	value = va_arg (ap, void *);
	write (parent_fd, &len, sizeof (int));
	write (parent_fd, value, len);
    }
    read (parent_fd, &i,         sizeof (int));
    if (!i)
	return NULL;
    str = xmalloc (i + 1, "parent_return");
    read (parent_fd, str, i);
    str [i] = 0;
    return str;
}

void
tell_parent (int msg)
{
    write (parent_fd, &msg, sizeof (int));
}

int
call_1s (int (*routine)(enum OperationMode, char *), char *str)
{
    if (we_are_background)
	return parent_call ((void *)routine, 1, strlen (str), str);
    else 
	return (*routine)(Foreground, str);
}

void
message_1s (int flags, char *title, char *str1)
{
    if (we_are_background)
	parent_call ((void *)real_message_1s, 3, sizeof (flags), &flags,
		    strlen (title), title, strlen (str1), str1);
    else
	real_message_1s (Foreground, &flags, title, str1);
}

void
message_2s (int flags, char *title, char *str1, char *str2)
{
    if (we_are_background)
	parent_call ((void *)real_message_2s, 4, sizeof (flags), &flags,
		     strlen (title), title, strlen (str1), str1,
	             strlen (str2), str2);
    else
	real_message_2s (Foreground, &flags, title, str1, str2);
}

void
message_3s (int flags, char *title, char *str1, char *str2, const char *str3)
{
    if (we_are_background)
	parent_call ((void *)real_message_3s, 3, sizeof (flags), &flags,
		     strlen (title), title, strlen (str1), str1,
	             strlen (str2), str2, strlen (str3), str3);
    else
	real_message_3s (Foreground, &flags, title, str1, str2, str3);
}

char *
input_dialog_help (char *header, char *text, char *help, char *def_text)
{
    extern char *real_input_dialog_help (char *header, char *text, char *help, char *def_text);
    if (we_are_background)
	return parent_call_string ((void *)real_input_dialog_help, 4,
				   strlen (header),   header,
				   strlen (text),     text,
				   strlen (help),     help,
				   strlen (def_text), def_text);
    else
	return real_input_dialog_help (header, text, help, def_text);
}

#else /* Else => No background code support */

/* {{{ Stubs if background code is not supported */
void
message_1s (int flags, char *title, char *str1)
{
    message (flags, title, str1);
}

void
message_2s (int flags, char *title, char *str1, char *str2)
{
    message (flags, title, str1, str2);
}

void
message_3s (int flags, char *title, char *str1, char *str2, const char *str3)
{
    message (flags, title, str1, str2, str3);
}

char *
input_dialog_help (char *header, char *text, char *help, char *def_text)
{
    return real_input_dialog_help (header, text, help, def_text);
}
/* }}} */

#endif

/* {{{ Functions shared between background and foreground */

void
message_1s1d (int flags, char *title, char *str, int d)
{
    char *p;

    p = xmalloc (strlen (str) + 30, "1s1d");
    sprintf (p, str, d);
    message_1s (flags, title, p);
    free (p);
}

/* }}} */

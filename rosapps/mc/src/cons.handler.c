/* Client interface for General purpose Linux console save/restore server
   Copyright (C) 1994 Janne Kukonlehto <jtklehto@stekt.oulu.fi>
   
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

/* The cons saver can't have a pid of 1, used to prevent bunches of */
/*#ifdef linux */
#include <config.h>

int    cons_saver_pid = 1;
extern int rxvt_extensions;
signed char console_flag = 0;

#if defined(linux) || defined(__linux__)

#include "tty.h"
#ifdef HAVE_UNISTD_H
#    include <unistd.h>
#endif
#include <fcntl.h>
#include <sys/types.h>
#ifdef HAVE_SYS_WAIT_H
#    include <sys/wait.h>
#endif
#include <signal.h>
#include "util.h"
#include "win.h"
#include "cons.saver.h"
#include "main.h"

static int pipefd1 [2] = {-1, -1}, pipefd2 [2] = {-1, -1};

void show_console_contents (int starty, unsigned char begin_line, unsigned char end_line)
{
    unsigned char message = 0;
    unsigned short bytes = 0;
    int i;

    standend ();

    if (look_for_rxvt_extensions ()) {
	show_rxvt_contents (starty, begin_line, end_line);
	return;
    }

    /* Is tty console? */
    if (!console_flag)
	return;
    /* Paranoid: Is the cons.saver still running? */
    if (cons_saver_pid < 1 || kill (cons_saver_pid, SIGCONT)){
	cons_saver_pid = 0;
	console_flag = 0;
	return;
    }

    /* Send command to the console handler */
    message = CONSOLE_CONTENTS;
    write (pipefd1[1], &message, 1);
    /* Check for outdated cons.saver */
    read (pipefd2[0], &message, 1);
    if (message != CONSOLE_CONTENTS)
	return;

    /* Send the range of lines that we want */
    write (pipefd1[1], &begin_line, 1);
    write (pipefd1[1], &end_line, 1);
    /* Read the corresponding number of bytes */
    read (pipefd2[0], &bytes, 2);

    /* Read the bytes and output them */
    for (i = 0; i < bytes; i++){
	if ((i % COLS) == 0)
	    move (starty+(i/COLS), 0);
	read (pipefd2[0], &message, 1);
	addch (message);
    }

    /* Read the value of the console_flag */
    read (pipefd2[0], &message, 1);
}

void handle_console (unsigned char action)
{
    char *tty_name;
    char *mc_conssaver;
    int status;

    switch (action){
    case CONSOLE_INIT:
	if (look_for_rxvt_extensions ())
	    return;
	/* Close old pipe ends in case it is the 2nd time we run cons.saver */
	close (pipefd1[1]);
	close (pipefd2[0]);
	/* Create two pipes for communication */
	pipe (pipefd1);
	pipe (pipefd2);
	/* Get the console saver running */
	cons_saver_pid = fork ();
	if (cons_saver_pid < 0){
	    /* Can't fork */
	    /* Delete pipes */
	    close (pipefd1[1]);
	    close (pipefd1[0]);
	    close (pipefd2[1]);
	    close (pipefd2[0]);
	    console_flag = 0;
	} else if (cons_saver_pid > 0){
	    /* Parent */
	    /* Close the extra pipe ends */
	    close (pipefd1[0]);
	    close (pipefd2[1]);
	    /* Was the child successful? */
	    read (pipefd2[0], &console_flag, 1);
	    if (!console_flag){
		close (pipefd1[1]);
		close (pipefd2[0]);
		waitpid (cons_saver_pid, &status, 0);
	    }
	} else {
	    /* Child */
	    /* Close the extra pipe ends */
	    close (pipefd1[1]);
	    close (pipefd2[0]);
	    tty_name = ttyname (0);
	    /* Bind the pipe 0 to the standard input */
	    close (0);
	    dup (pipefd1[0]);
	    close (pipefd1[0]);
	    /* Bind the pipe 1 to the standard output */
	    close (1);
	    dup (pipefd2[1]);
	    close (pipefd2[1]);
	    /* Bind standard error to /dev/null */
	    close (2);
	    open ("/dev/null", O_WRONLY);
	    /* Exec the console save/restore handler */
	    mc_conssaver = concat_dir_and_file (mc_home, "bin/cons.saver");
	    execl (mc_conssaver, "cons.saver", tty_name, NULL);
	    /* Exec failed */
	    console_flag = 0;
	    write (1, &console_flag, 1);
	    close (1);
	    close (0);
	    exit (3);
	} /* if (cons_saver_pid ...) */
	break;

    case CONSOLE_DONE:
    case CONSOLE_SAVE:
    case CONSOLE_RESTORE:
	if (look_for_rxvt_extensions ())
	    return;
	/* Is tty console? */
	if (!console_flag)
	    return;
	/* Paranoid: Is the cons.saver still running? */
	if (cons_saver_pid < 1 || kill (cons_saver_pid, SIGCONT)){
	    cons_saver_pid = 0;
	    console_flag = 0;
	    return;
	}
	/* Send command to the console handler */
	write (pipefd1[1], &action, 1);
	if (action != CONSOLE_DONE){
	    /* Wait the console handler to do its job */
	    read (pipefd2[0], &console_flag, 1);
	}
	if (action == CONSOLE_DONE || !console_flag){
	    /* We are done -> Let's clean up */
	    close (pipefd1 [1]);
	    close (pipefd2 [0]);
	    waitpid (cons_saver_pid, &status, 0);
	    console_flag = 0;
	}
	break;
    default:
	/* Nothing */
    }
}

#endif /* #ifdef linux */

#ifdef SCO_FLAVOR
/* 
**	SCO console save/restore handling routines
**	Copyright (C) 1997 Alex Tkachenko <alex@bcs.zaporizhzhe.ua>
*/

#include <stdio.h>
#include <sys/types.h>
#include <sys/vid.h>
#include <sys/console.h>
#include <sys/vtkd.h>
#include <memory.h>
#include <signal.h>
#include "tty.h"
#include "util.h"
#include "color.h"
#include "cons.saver.h"

static int FD_OUT = 2;

static unsigned short* vidbuf = NULL;
static unsigned short* screen = NULL;
static int height = 0, width = 0, saved_attr = 0;
static int mode = 0;

#define	SIG_ACQUIRE 21 /* originally: handset, line status change (?) */

static int
vt_active()
{
	struct vid_info vi;
	int adapter = ioctl(FD_OUT, CONS_CURRENT, 0);

	vi.size = sizeof(struct vid_info);
	ioctl(FD_OUT, CONS_GETINFO, &(vi));
	return (vi.m_num == ioctl(FD_OUT,CONSADP,adapter));
}

static void
console_acquire_vt()
{
	struct vt_mode smode;

	signal(SIG_ACQUIRE, SIG_DFL);
	smode.mode = VT_AUTO;
	smode.waitv = smode.relsig = smode.acqsig = smode.frsig = 0;
	ioctl(FD_OUT, VT_SETMODE, &smode);
	ioctl(FD_OUT, VT_RELDISP, VT_ACKACQ);
}

static void
console_shutdown()
{
	if (!console_flag)
	{
		return;
	}
	if (screen != NULL)
	{
		free(screen);
	}
	console_flag = 0;
}

static void
console_save()
{
	struct m6845_info mi;

	if (!console_flag)
	{
		return;
	}

	if (!vt_active())
	{
		struct vt_mode smode;

		/* 
		**	User switched out of our vt. Let's wait until we get SIG_ACQUIRE,
		**	otherwise we could save wrong screen image
		*/
		signal(SIG_ACQUIRE, console_acquire_vt);
		smode.mode = VT_PROCESS;
		smode.waitv = 0;
		smode.waitv = smode.relsig = smode.acqsig = smode.frsig = SIG_ACQUIRE;
		ioctl(FD_OUT, VT_SETMODE, &smode);

		pause();
	}

	saved_attr = ioctl(FD_OUT, GIO_ATTR, 0);

	vidbuf = (unsigned short*) ioctl(FD_OUT, MAPCONS, 0);

	mi.size = sizeof(struct m6845_info);
	ioctl(FD_OUT, CONS_6845INFO, &mi);

	{
		unsigned short* start = vidbuf + mi.screen_top;
		memcpy(screen, start, width * height * 2);
	}

	write(FD_OUT,"\0337",2);                        /* save cursor position */
}

static void
console_restore()
{
	struct m6845_info mi;
	unsigned short* start;

	if (!console_flag)
	{
		return;
	}

    write (FD_OUT, "\033[2J", 4);

	mi.size = sizeof(struct m6845_info);
	ioctl(FD_OUT, CONS_6845INFO, &mi);

	start = vidbuf + mi.screen_top;
 	memcpy(start, screen, width * height * 2);
	write(FD_OUT,"\0338",2);                 /* restore cursor position */
}

static void
console_init()
{
	struct vid_info vi;
	int adapter = ioctl(FD_OUT, CONS_CURRENT, 0);

	console_flag = 0;

	if (adapter != -1)
	{
		vi.size = sizeof(struct vid_info);
		ioctl(FD_OUT, CONS_GETINFO, &(vi));

		if (vt_active())
		{
			console_flag = 1;

			height = vi.mv_rsz;
			width = vi.mv_csz;

			screen = (unsigned short*) xmalloc(height * width * 2,"console_init");
			if (screen == NULL)
			{
				console_shutdown();
				return;
			}
			console_save();
			mode = ioctl(FD_OUT, CONS_GET, 0);
			ioctl(FD_OUT, MODESWITCH | mode, 0);
			console_restore();
		}
	}
}

void
handle_console (unsigned char action)
{
	if (look_for_rxvt_extensions ())
		return;
	switch (action){
		case CONSOLE_INIT:
			console_init();
			break;

		case CONSOLE_DONE: 
			console_shutdown();
			break;

		case CONSOLE_SAVE:
			console_save();
			break;

		case CONSOLE_RESTORE:
			console_restore();
			break;
		default:
			/* Nothing */;
    }
}

void
show_console_contents (int starty, unsigned char begin_line, unsigned char end_line)
{
	register int i, len = (end_line - begin_line) * width;

	if (look_for_rxvt_extensions ()) {
		show_rxvt_contents (starty, begin_line, end_line);
		return;
	}
	attrset(DEFAULT_COLOR);
	for (i = 0; i < len; i++)
	{
		if ((i % width) == 0)
		    move (starty+(i/width), 0);
		addch ((unsigned char)screen[width*starty + i]);
	}
}

#endif /* SCO_FLAVOR */


#if !defined(linux) && !defined(__linux__) && !defined(SCO_FLAVOR)

#include "tty.h"

void show_console_contents (int starty, unsigned char begin_line, unsigned char end_line)
{
    standend ();

    if (look_for_rxvt_extensions ()) {
	show_rxvt_contents (starty, begin_line, end_line);
	return;
    }
    console_flag = 0;
}

void handle_console (unsigned char action)
{
    look_for_rxvt_extensions ();
}

#endif				/* !defined(linux) && !defined(__linux__) && !defined(SCO_FLAVOR) */



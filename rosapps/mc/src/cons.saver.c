#if defined(linux) || defined(__linux__)

/* General purpose Linux console screen save/restore server
   Copyright (C) 1994 Janne Kukonlehto <jtklehto@stekt.oulu.fi>
   Original idea from Unix Interactive Tools version 3.2b (tty.c)
   This code requires root privileges.
   You may want to make the cons.saver setuid root.
   The code should be safe even if it is setuid but who knows?
   
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
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#ifdef HAVE_UNISTD_H
#    include <unistd.h>
#endif
#include <termios.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>	/* For isdigit() */
typedef struct WINDOW WINDOW;
#include "cons.saver.h"

#define cmd_input 0
#define cmd_output 1

/* Meaning of console_flag:
   -1 == to be detected,
   0  == not a console
   1  == is a console, Linux < 1.1.67 (black & white)
   2  == is a console, Linux >= 1.1.67 (color)
   3  == is a console, Linux >= 1.1.92 (color, use /dev/vcsa$num
   */
static signed char console_flag = -1;
/*
   Meaning of console_fd:
   -1  == not opened,
   >=0 == opened
   */
static int console_fd = -1;
static char *tty_name;
static int len;
static char *buffer = NULL;
static int buffer_size = 0;
static int columns, rows;
static char vcs_name [40];
static int vcs_fd;

static void dwrite (int fd, char *buffer)
{
    write (fd, buffer, strlen (buffer));
}

static void tty_getsize ()
{
    struct winsize winsz;

    winsz.ws_col = winsz.ws_row = 0;
    ioctl (console_fd, TIOCGWINSZ, &winsz);
    if (winsz.ws_col && winsz.ws_row){
	columns = winsz.ws_col;
	rows    = winsz.ws_row;
    } else {
	/* Never happens (I think) */
	dwrite (2, "TIOCGWINSZ failed\n");
	columns = 80;
	rows = 25;
	console_flag = 0;
    }
}

inline void tty_cursormove(int y, int x)
{
    char buffer [20];

    /* Standard ANSI escape sequence for cursor positioning */
    sprintf (buffer,"\33[%d;%dH", y + 1, x + 1);
    dwrite (console_fd, buffer);
}

int check_file (char *filename, int check_console, char **msg)
{
    int fd;
    struct stat stat_buf;

    /* Avoiding race conditions: use of fstat makes sure that
       both 'open' and 'stat' operate on the same file */

    *msg = 0;
    
    fd = open (filename, O_RDWR);
    if (fd == -1)
	return -1;
    
    if (fstat (fd, &stat_buf) == -1)
	return -1;

    /* Must be character device */
    if (!S_ISCHR (stat_buf.st_mode)){
	*msg = "Not a character device";
	return -1;
    }

#ifdef DEBUG
    fprintf (stderr, "Device: %x\n", stat_buf.st_rdev);
#endif
    if (check_console){
	/* Second time: must be console */
	if ((stat_buf.st_rdev & 0xff00) != 0x0400){
	    *msg = "Not a console";
	    return -1;
	}
    
	if ((stat_buf.st_rdev & 0x00ff) > 63){
	    *msg = "Minor device number too big";
	    return -1;
	}
	
	/* Must be owned by the user */
	if (stat_buf.st_uid != getuid ()){
	    *msg = "Not a owner";
	    return -1;
	}
    }
    
    /* Everything seems to be okay */
    return fd;
}

/* Detect console */
/* Because the name of the tty is supplied by the user and this
   can be a setuid program a lot of checks has to done to avoid
   creating a security hole */
char *detect_console (void)
{
    char *msg;
    int  xlen;
    
    /* Must be console */
    /* Handle the case for /dev/tty?? */
    if (tty_name[len-5] == 't')
	xlen = len - 1;
    else
	xlen = len;

    /* General: /dev/ttyn */
    if (tty_name[xlen - 5] != '/' ||
	tty_name[xlen - 4] != 't' ||
	tty_name[xlen - 3] != 't' ||
	tty_name[xlen - 2] != 'y' ||
	!isdigit(tty_name[xlen - 1]) ||
	!isdigit(tty_name[len - 1]))
	return "Doesn't look like console";

    sprintf (vcs_name, "/dev/vcsa%s", tty_name + xlen - 1);
    vcs_fd = check_file (vcs_name, 0, &msg);
    console_fd = check_file (tty_name, 1, &msg);

#ifdef DEBUG
    fprintf (stderr, "vcs_fd = %d console_fd = %d\n", vcs_fd, console_fd);
#endif
    
    if (vcs_fd != -1){
	console_flag = 3;
    }

    if (console_fd == -1)
	return msg;

    return NULL;
}

void save_console (void)
{
    int i;

    if (!console_flag)
	return;
    buffer [1] = tty_name [len-1] - '0';
    if (console_flag >= 2){
	/* Linux >= 1.1.67 */
	/* Get screen contents and cursor position */
	buffer [0] = 8;
	if (console_flag == 2){
	    if ((i = ioctl (console_fd, TIOCLINUX, buffer)) == -1){
		/* Oops, this is not Linux 1.1.67 */
		console_flag = 1;
	    }
	} else {
	    lseek (vcs_fd, 0, 0);
	    read (vcs_fd, buffer, buffer_size);
	}
    }
    if (console_flag == 1){
	int index, x, y;

	/* Linux < 1.1.67 */
	/* Get screen contents */
	buffer [0] = 0;
	if (ioctl(console_fd, TIOCLINUX, buffer) == -1){
	    buffer[0] = buffer[1] = 0;

	    /* Linux bug: bad ioctl on console 8 */
	    if (ioctl(console_fd, TIOCLINUX, buffer) == -1){
		/* Oops, this is not a console after all */
		console_flag = 0;
		return;
	    }
	}
	/* Select the beginning of the bottommost empty line
	   to be the cursor position */
	index = 2 + rows * columns;
	for (y = rows - 1; y >= 0; y--)
	    for (x = columns - 1; x >= 0; x--)
		if (buffer[--index] != ' ')
		    goto non_space_found;
    non_space_found:
	buffer[0] = y + 1;
	buffer[1] = 0;
	/*tty_cursormove(y + 1, 0);*/
    }
}

void restore_console (void)
{
    if (!console_flag)
	return;
    if (console_flag == 2){
	/* Linux >= 1.1.67 */
	/* Restore screen contents and cursor position */
	buffer [0] = 9;
	buffer [1] = tty_name [len-1] - '0';
	ioctl (console_fd, TIOCLINUX, buffer);
    }
    if (console_flag == 3){
	lseek (vcs_fd, 0, 0);
	write (vcs_fd, buffer, buffer_size);
    }
    if (console_flag == 1){
	/* Clear screen */
	write(console_fd, "\033[H\033[2J", 7);
	/* Output saved screen contents */
	write(console_fd, buffer + 2, rows * columns);
	/* Move the cursor to the previously selected position */
	tty_cursormove(buffer[0], buffer[1]);
    }
}

void send_contents ()
{
    unsigned char begin_line=0, end_line=0;
    int index, x, y;
    int lastline;
    unsigned char message;
    unsigned short bytes;
    int bytes_per_char;
    
    bytes_per_char = console_flag == 1 ? 1 : 2;
    
    /* Calculate the number of used lines */
    if (console_flag == 2 || console_flag == 1 || console_flag == 3){
	index = (2 + rows * columns) * bytes_per_char;
	for (y = rows - 1; y >= 0; y--)
	    for (x = columns - 1; x >= 0; x--){
		index -= bytes_per_char;
		if (buffer[index] != ' ')
		    goto non_space_found;
	    }
    non_space_found:
	lastline = y + 1;
    } else
	return;

    /* Inform the invoker that we can handle this command */
    message = CONSOLE_CONTENTS;
    write (cmd_output, &message, 1);

    /* Read the range of lines wanted */
    read (cmd_input, &begin_line, 1);
    read (cmd_input, &end_line, 1);
    if (begin_line > lastline)
	begin_line = lastline;
    if (end_line > lastline)
	end_line = lastline;

    /* Tell the invoker how many bytes it will be */
    bytes = (end_line - begin_line) * columns;
    write (cmd_output, &bytes, 2);

    /* Send the contents */
    for (index = (2 + begin_line * columns) * bytes_per_char;
	 index < (2 + end_line * columns) * bytes_per_char;
	 index += bytes_per_char)
	write (cmd_output, buffer + index, 1);

    /* All done */
}

int main (int argc, char **argv)
{
    char *error;
    unsigned char action = 0;

    if (argc != 2){
	/* Wrong number of arguments */

	dwrite (2, "Usage: cons.saver <ttyname>\n");
	console_flag = 0;
	write (cmd_output, &console_flag, 1);
	return 3;
    }

    /* Lose the control terminal */
    setsid ();
    
    /* Check that the argument is a legal console */
    tty_name = argv [1];
    len = strlen(tty_name);
    error = detect_console ();

    if (error){
	/* Not a console -> no need for privileges */
	setuid (getuid ());
	dwrite (2, error);
	console_flag = 0;
	if (console_fd >= 0)
	    close (console_fd);
    } else {
	/* Console was detected */
	if (console_flag != 3)
	    console_flag = 2; /* Default to Linux >= 1.1.67 */
	/* Allocate buffer for screen image */
	tty_getsize ();
	buffer_size = 4 + 2 * columns * rows;
	buffer = (char*) malloc (buffer_size);
    }

    /* If using /dev/vcs*, we don't need anymore the console fd */
    if (console_flag == 3)
	close (console_fd);
    
    /* Inform the invoker about the result of the tests */
    write (cmd_output, &console_flag, 1);

    /* Read commands from the invoker */
    while (console_flag && read (cmd_input, &action, 1)){
	/* Handle command */
	switch (action){
	case CONSOLE_DONE:
	    console_flag = 0;
	    continue; /* Break while loop instead of switch clause */
	case CONSOLE_SAVE:
	    save_console ();
	    break;
	case CONSOLE_RESTORE:
	    restore_console ();
	    break;
	case CONSOLE_CONTENTS:
	    send_contents ();
	    break;
	} /* switch (action) */
		
	/* Inform the invoker that command is handled */
	write (cmd_output, &console_flag, 1);
    } /* while (read ...) */

    if (buffer)
	free (buffer);
    return 0;   
}

#else

#error The Linux console screen saver works only on Linux.

#endif /* #ifdef linux */

/* slutty.c --- Unix Low level terminal (tty) functions for S-Lang */
/* Copyright (c) 1992, 1995 John E. Davis
 * All rights reserved.
 * 
 * You may distribute under the terms of either the GNU General Public
 * License or the Perl Artistic License.
 */


#include "config.h"

#include <stdio.h>
#include <signal.h>
/* sequent support thanks to Kenneth Lorber <keni@oasys.dt.navy.mil> */
/* SYSV (SYSV ISC R3.2 v3.0) provided by iain.lea@erlm.siemens.de */

#if defined (_AIX) && !defined (_ALL_SOURCE)
# define _ALL_SOURCE	/* so NBBY is defined in <sys/types.h> */
#endif

#ifdef HAVE_STDLIB_H
# include <stdlib.h>
#endif

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#include <sys/time.h>
#include <sys/types.h>

#ifdef SYSV
# include <fcntl.h>
# ifndef CRAY
#  include <sys/termio.h>
#  include <sys/stream.h>
#  include <sys/ptem.h>
#  include <sys/tty.h>
# endif
#endif


#ifdef __BEOS__
/* Prototype for select */
# include <net/socket.h>
#endif

#include <sys/file.h>

#ifndef sun
# include <sys/ioctl.h>
#endif

#ifdef __QNX__
# include <sys/select.h>
#endif

#include <sys/stat.h>
#include <errno.h>

#if defined (_AIX) && !defined (FD_SET)
# include <sys/select.h>	/* for FD_ISSET, FD_SET, FD_ZERO */
#endif

#ifndef O_RDWR
# include <fcntl.h>
#endif


#include "slang.h"
#include "_slang.h"

int SLang_TT_Read_FD = -1;
int SLang_TT_Baud_Rate;


#ifdef HAVE_TERMIOS_H
# if !defined(HAVE_TCGETATTR) || !defined(HAVE_TCSETATTR)
#   undef HAVE_TERMIOS_H
# endif
#endif

#ifndef HAVE_TERMIOS_H

# if !defined(CBREAK) && defined(sun)
#  ifndef BSD_COMP
#   define BSD_COMP 1
#  endif
#  include <sys/ioctl.h>
# endif

typedef struct
  {
      struct tchars t;
      struct ltchars lt;
      struct sgttyb s;
  }
TTY_Termio_Type;
#else
# include <termios.h>
typedef struct termios TTY_Termio_Type;
#endif

static TTY_Termio_Type Old_TTY;

#ifdef HAVE_TERMIOS_H
static struct 
{
    speed_t key;
    int value;
} Baud_Rates[] = 
{
   {B0, 0},
   {B50, 50}, 
   {B75, 75}, 
   {B110, 110}, 
   {B134, 134}, 
   {B150, 150}, 
   {B200, 200}, 
   {B300, 300}, 
   {B600, 600}, 
   {B1200, 1200}, 
   {B1800, 1800}, 
   {B2400, 2400}, 
   {B4800, 4800}, 
   {B9600, 9600}, 
   {B19200, 19200}, 
   {B38400, 38400}
#ifdef B57600
   , {B57600, 57600}
#endif
#ifdef B115200
   , {B115200, 115200}
#endif
#ifdef B230400
   , {B230400, 230400}
#endif
};
#endif

#ifdef HAVE_TERMIOS_H
# define GET_TERMIOS(fd, x) tcgetattr(fd, x)
# define SET_TERMIOS(fd, x) tcsetattr(fd, TCSADRAIN, x)
#else
# ifdef TCGETS
#  define GET_TERMIOS(fd, x) ioctl(fd, TCGETS, x)
#  define SET_TERMIOS(fd, x) ioctl(fd, TCSETS, x)
# else
#  define X(x,m)  &(((TTY_Termio_Type *)(x))->m)
#  define GET_TERMIOS(fd, x)	\
    ((ioctl(fd, TIOCGETC, X(x,t)) || \
      ioctl(fd, TIOCGLTC, X(x,lt)) || \
      ioctl(fd, TIOCGETP, X(x,s))) ? -1 : 0)
#  define SET_TERMIOS(fd, x)	\
    ((ioctl(fd, TIOCSETC, X(x,t)) ||\
      ioctl(fd, TIOCSLTC, X(x,lt)) || \
      ioctl(fd, TIOCSETP, X(x,s))) ? -1 : 0)
# endif
#endif

static int TTY_Inited = 0;
static int TTY_Open = 0;

#ifdef ultrix   /* Ultrix gets _POSIX_VDISABLE wrong! */
# define NULL_VALUE -1
#else
# ifdef _POSIX_VDISABLE
#  define NULL_VALUE _POSIX_VDISABLE
# else
#  define NULL_VALUE 255
# endif
#endif

static int
speed_t2baud_rate (speed_t s)
{
    int i;
    
    for (i = 0; i < sizeof (Baud_Rates)/sizeof (Baud_Rates[0]); i++)
	if (Baud_Rates[i].key == s)
	    return (Baud_Rates[i].value);
    return 0;
}

int SLang_init_tty (int abort_char, int no_flow_control, int opost)
{
   TTY_Termio_Type newtty;
   
   SLsig_block_signals ();
   
   if (TTY_Inited) 
     {
	SLsig_unblock_signals ();
	return 0;
     }
   
   TTY_Open = 0;
   
   if ((SLang_TT_Read_FD == -1)
       || (1 != isatty (SLang_TT_Read_FD)))
     {
#if 0
#ifdef O_RDWR
# ifndef __BEOS__  /* I have been told that BEOS will HANG if passed /dev/tty */
	if ((SLang_TT_Read_FD = open("/dev/tty", O_RDWR)) >= 0)
	  {
	     TTY_Open = 1;
	  }
# endif
#endif
#endif /* 0 */
	if (TTY_Open == 0)
	  {

#if 0
/* In the Midnight Commander we bind stderr sometimes to a pipe. If we
   use stderr for terminal input and call SLang_getkey while stderr is
   bound to a pipe MC will hang completly in SLsys_input_pending. 
   NOTE: There's an independent fix for this problem in src/slint.c for
   the case that the Midnight Commander is linked against a shared slang 
   library compiled from different sources.
 */
	     SLang_TT_Read_FD = fileno (stderr);
	     if (1 != isatty (SLang_TT_Read_FD))
#endif	     
	       {
		  SLang_TT_Read_FD = fileno (stdin);
		  if (1 != isatty (SLang_TT_Read_FD))
		    {
		       fprintf (stderr, "Failed to open terminal.");
		       return -1;
		    }
	       }
	  }
     }
   
   SLang_Abort_Char = abort_char;
   
   /* Some systems may not permit signals to be blocked.  As a result, the 
    * return code must be checked.
    */
   while (-1 == GET_TERMIOS(SLang_TT_Read_FD, &Old_TTY))
     {
	if (errno != EINTR)
	  {
	     SLsig_unblock_signals ();
	     return -1;
	  }
     }
   
   while (-1 == GET_TERMIOS(SLang_TT_Read_FD, &newtty))
     {
	if (errno != EINTR)
	  {
	     SLsig_unblock_signals ();
	     return -1;
	  }
     }
   
#ifndef HAVE_TERMIOS_H
   newtty.s.sg_flags &= ~(ECHO);
   newtty.s.sg_flags &= ~(CRMOD);
   /*   if (Flow_Control == 0) newtty.s.sg_flags &= ~IXON; */
   newtty.t.t_eofc = 1;
   if (abort_char == -1) SLang_Abort_Char = newtty.t.t_intrc;
   newtty.t.t_intrc = SLang_Abort_Char;	/* ^G */
   newtty.t.t_quitc = 255;
   newtty.lt.t_suspc = 255;   /* to ignore ^Z */
   newtty.lt.t_dsuspc = 255;    /* to ignore ^Y */
   newtty.lt.t_lnextc = 255;
   newtty.s.sg_flags |= CBREAK;		/* do I want cbreak or raw????? */
#else
   
   /* get baud rate */
   
   /* [not only QNX related !?!]
    * ECHO(0x08) is a c_lflag bit, it means here PARMRK(0x08) in c_iflag!!!
    */
   /*newtty.c_iflag &= ~(ECHO | INLCR | ICRNL);*/
   newtty.c_iflag &= ~(INLCR | ICRNL);
#ifdef ISTRIP
   /* newtty.c_iflag &= ~ISTRIP; */
#endif
   if (opost == 0) newtty.c_oflag &= ~OPOST;

   if (SLang_TT_Baud_Rate == 0)
     {
/* Note:  if this generates an compiler error, simply remove 
   the statement */
#ifdef HAVE_CFGETOSPEED
	SLang_TT_Baud_Rate = cfgetospeed (&newtty);
#endif	
	SLang_TT_Baud_Rate = speed_t2baud_rate (SLang_TT_Baud_Rate);
     }
   if (no_flow_control) newtty.c_iflag &= ~IXON; else newtty.c_iflag |= IXON;

   newtty.c_cc[VMIN] = 1;
   newtty.c_cc[VTIME] = 0;
   newtty.c_cc[VEOF] = 1;
   newtty.c_lflag = ISIG | NOFLSH;
   if (abort_char == -1) SLang_Abort_Char = newtty.c_cc[VINTR];
   newtty.c_cc[VINTR] = SLang_Abort_Char;   /* ^G */
   newtty.c_cc[VQUIT] = NULL_VALUE;
   newtty.c_cc[VSUSP] = NULL_VALUE;   /* to ignore ^Z */
#ifdef VSWTCH
   newtty.c_cc[VSWTCH] = NULL_VALUE;   /* to ignore who knows what */
#endif
#endif /* NOT HAVE_TERMIOS_H */
   
   while (-1 == SET_TERMIOS(SLang_TT_Read_FD, &newtty))
     {
	if (errno != EINTR)
	  {
	     SLsig_unblock_signals ();
	     return -1;
	  }
     }
   
   TTY_Inited = 1;
   SLsig_unblock_signals ();
   return 0;
}

void SLtty_set_suspend_state (int mode)
{
   TTY_Termio_Type newtty;
   
   SLsig_block_signals ();
   
   if (TTY_Inited == 0)
     {
	SLsig_unblock_signals ();
	return;
     }
      
   while ((-1 == GET_TERMIOS (SLang_TT_Read_FD, &newtty))
	  && (errno == EINTR))
     ;
   
#ifndef HAVE_TERMIOS_H
   if (mode == 0) newtty.lt.t_suspc = 255;
   else newtty.lt.t_suspc = Old_TTY.lt.t_suspc;
#else
   if (mode == 0) newtty.c_cc[VSUSP] = NULL_VALUE;
   else newtty.c_cc[VSUSP] = Old_TTY.c_cc[VSUSP];
#endif
   
   while ((-1 == SET_TERMIOS (SLang_TT_Read_FD, &newtty))
	  && (errno == EINTR))
     ;

   SLsig_unblock_signals ();
}

void SLang_reset_tty (void)
{
   SLsig_block_signals ();
   
   if (TTY_Inited == 0)
     {
	SLsig_unblock_signals ();
	return;
     }
   
   while ((-1 == SET_TERMIOS(SLang_TT_Read_FD, &Old_TTY))
	  && (errno == EINTR))
     ;
   
   if (TTY_Open)
     {
	while ((-1 == close (SLang_TT_Read_FD))
	       && (errno == EINTR))
	  ;
	  
	TTY_Open = 0;
	SLang_TT_Read_FD = -1;
     }
   
   TTY_Inited = 0;
   SLsig_unblock_signals ();
}

static void default_sigint (int sig)
{
   sig = errno;			       /* use parameter */
   
   SLKeyBoard_Quit = 1;
   if (SLang_Ignore_User_Abort == 0) SLang_Error = USER_BREAK;
   SLsignal_intr (SIGINT, default_sigint);
   errno = sig;
}

void SLang_set_abort_signal (void (*hand)(int))
{
   int save_errno = errno;
   
   if (hand == NULL) hand = default_sigint;
   SLsignal_intr (SIGINT, hand);
   
   errno = save_errno;
}

#ifndef FD_SET
#define FD_SET(fd, tthis) *(tthis) = 1 << (fd)
#define FD_ZERO(tthis)    *(tthis) = 0
#define FD_ISSET(fd, tthis) (*(tthis) & (1 << fd))
typedef int fd_set;
#endif

static fd_set Read_FD_Set;


/* HACK: If > 0, use 1/10 seconds.  If < 0, use 1/1000 seconds */

int SLsys_input_pending(int tsecs)
{
   struct timeval wait;
   long usecs, secs;

   if (TTY_Inited == 0) return -1;
   
   if (tsecs >= 0)
     {
	secs = tsecs / 10;
	usecs = (tsecs % 10) * 100000;
     }
   else
     {
	tsecs = -tsecs;
	secs = tsecs / 1000;
	usecs = (tsecs % 1000) * 1000;
     }
   
   wait.tv_sec = secs;
   wait.tv_usec = usecs;

   FD_ZERO(&Read_FD_Set);
   FD_SET(SLang_TT_Read_FD, &Read_FD_Set);
   
   return select(SLang_TT_Read_FD + 1, &Read_FD_Set, NULL, NULL, &wait);
}


int (*SLang_getkey_intr_hook) (void);

static int handle_interrupt (void)
{
   if (SLang_getkey_intr_hook != NULL)
     {
	int save_tty_fd = SLang_TT_Read_FD;
   
	if (-1 == (*SLang_getkey_intr_hook) ())
	  return -1;
	
	if (save_tty_fd != SLang_TT_Read_FD)
	  return -1;
     }
   
   return 0;
}

unsigned int SLsys_getkey (void)
{
   unsigned char c;
   
   if (TTY_Inited == 0)
     {
	int ic = fgetc (stdin);
	if (ic == EOF) return SLANG_GETKEY_ERROR;
	return (unsigned int) ic;
     }
   
   while (1)
     {
	int ret;
	
	if (SLKeyBoard_Quit) 
	  return SLang_Abort_Char;
	
	if (0 == (ret = SLsys_input_pending (100)))
	  continue;
	
	if (ret != -1)
	  break;
	
	if (SLKeyBoard_Quit) 
	  return SLang_Abort_Char;
	
	if (errno == EINTR)
	  {
	     if (-1 == handle_interrupt ())
	       return SLANG_GETKEY_ERROR;
	     
	     continue;
	  }
	
	break;			       /* let read handle it */
     }
   
   while (-1 == read(SLang_TT_Read_FD, (char *) &c, 1))
     {
	if (errno == EINTR) 
	  {
	     if (-1 == handle_interrupt ())
	       return SLANG_GETKEY_ERROR;
	     
	     if (SLKeyBoard_Quit) 
	       return SLang_Abort_Char;
	     
	     continue;
	  }
#ifdef EAGAIN
	if (errno == EAGAIN) 
	  {
	     sleep (1);
	     continue;
	  }
#endif
#ifdef EWOULDBLOCK
	if (errno == EWOULDBLOCK)
	  {
	     sleep (1);
	     continue;
	  }
#endif
#ifdef EIO
	if (errno == EIO)
	  {
	     SLang_exit_error ("SLsys_getkey: EIO error.");
	  }
#endif
	return SLANG_GETKEY_ERROR;
     }

   return((unsigned int) c);
}


/* Copyright (c) 1992, 1995 John E. Davis
 * All rights reserved.
 * 
 * You may distribute under the terms of either the GNU General Public
 * License or the Perl Artistic License.
 */

#include "config.h"

#include <stdio.h>

#include "slang.h"
#include "_slang.h"

#define INCL_BASE
#define INCL_NOPM
#define INCL_VIO
#define INCL_KBD
#define INCL_DOS
#if 0
# define INCL_DOSSEMAPHORES
#endif
#ifdef LONG
#undef LONG
#endif
#ifdef VOID
#undef VOID
#endif
#include <os2.h>

#include <signal.h>
#include <process.h>

KBDINFO	initialKbdInfo;	/* keyboard info */

/* Code to read keystrokes in a separate thread */

typedef struct kbdcodes {
  UCHAR ascii;
  UCHAR scan;
/*  USHORT shift; */
} KBDCODES;

#define BUFFER_LEN 4096
static KBDCODES threadKeys[BUFFER_LEN];
static int atEnd = 0;
static int startBuf;
static int endBuf;

/* Original code used semaphores to control access to threadKeys.
 * It is expected that the semaphore code will be deleted after 0.97.
*/
#if 0

#ifdef __os2_16__

typedef USHORT APIRET;
static HSEM Hmtx;

#define DosRequestMutexSem(hmtx,timeout) DosSemRequest(hmtx,timeout)
#define DosReleaseMutexSem(hmtx) DosSemClear(hmtx)
#define DosCloseMutexSem(hmtx) DosCloseSem(hmtx)

#else /* !defined(__os2_16__) */

static HMTX Hmtx;     /* Mutex Semaphore */

#endif


static APIRET CreateSem(void)
{
#ifdef __os2_16__
  char SemName[32];
  sprintf(SemName, "\\SEM\\jed\\%u", getpid());
  return ( DosCreateSem (0, &Hmtx, SemName) );
#else
  return ( DosCreateMutexSem (NULL, &Hmtx, 0, 0) );
#endif
}

static APIRET RequestSem(void)
{
  return ( DosRequestMutexSem (Hmtx, -1) );
}

static APIRET ReleaseSem(void)
{
  return ( DosReleaseMutexSem (Hmtx) );
}

static APIRET CloseSem(void)
{
  return( DosCloseMutexSem (Hmtx) );
}

#else

#define CreateSem()
#define RequestSem()
#define ReleaseSem()
#define CloseSem()

#endif


static void set_kbd(void)
{
  KBDINFO kbdInfo;

  kbdInfo = initialKbdInfo;
  kbdInfo.fsMask &= ~0x0001;		/* not echo on		*/
  kbdInfo.fsMask |= 0x0002;		/* echo off		*/
  kbdInfo.fsMask &= ~0x0008;		/* cooked mode off	*/
  kbdInfo.fsMask |= 0x0004;		/* raw mode		*/
  kbdInfo.fsMask &= ~0x0100;		/* shift report	off	*/
  KbdSetStatus(&kbdInfo, 0);
}

static void thread_getkey ()
{
   KBDKEYINFO keyInfo;
   int n;

   while (!atEnd) {     /* at end is a flag */
      set_kbd();
      KbdCharIn(&keyInfo, IO_NOWAIT, 0);       /* get a character	*/
      if (keyInfo.fbStatus & 0x040) {          /* found a char process it */
	if (keyInfo.chChar == SLang_Abort_Char) {
	  if (SLang_Ignore_User_Abort == 0) SLang_Error = USER_BREAK;
	  SLKeyBoard_Quit = 1;
	}
	n = (endBuf + 1) % BUFFER_LEN;
	if (n == startBuf) {
	  DosBeep (500, 20);
	  KbdFlushBuffer(0);
	  continue;
	}
	RequestSem();
	threadKeys [n].ascii = keyInfo.chChar;
	threadKeys [n].scan = keyInfo.chScan;
/*	threadKeys [n].shift = keyInfo.fsState; */
	endBuf = n;
	ReleaseSem();
      } else                    /* no char available*/
	DosSleep (20);
   }
}

static void thread_code (void *Args)
{
  (void) Args;
  startBuf = -1;      /* initialize the buffer pointers */
  endBuf = -1;
  thread_getkey ();
  atEnd = 0;          /* reset the flag */
  _endthread();
}


/* The code below is in the main thread */

int SLang_init_tty(int abort_char, int dum2, int dum3)
{
  VIOCURSORINFO cursorInfo, OldcursorInfo;

  (void) dum2; (void) dum3;
   if (abort_char == -1) abort_char = 3;   /* ^C */
  SLang_Abort_Char = abort_char;

  /*  set ^C off */
  signal (SIGINT, SIG_IGN);
  signal (SIGBREAK, SIG_IGN);

  /* set up the keyboard */

  initialKbdInfo.cb = sizeof(initialKbdInfo);
  KbdGetStatus(&initialKbdInfo, 0);
  set_kbd();

  /* open a semaphore */
  CreateSem();

  /* start a separate thread to read the keyboard */
#if defined(__BORLANDC__)
  _beginthread (thread_code, 8096, NULL);
#else
  _beginthread (thread_code, NULL,  8096, NULL);
#endif

  VioGetCurType (&OldcursorInfo, 0);
  cursorInfo.yStart = 1;
  cursorInfo.cEnd = 15;
  cursorInfo.cx = 1;
  cursorInfo.attr = 1;
  if (VioSetCurType (&cursorInfo, 0))
    VioSetCurType (&OldcursorInfo, 0);   /* reset to previous value */

  return 0;
}

void SLang_reset_tty (void)
{
  atEnd = 1;                      /* set flag and wait until thread ends */
  while (atEnd) {DosSleep (0);}

  CloseSem();

  /* close the keyboard */
  KbdSetStatus(&initialKbdInfo, 0); /* restore original state	*/
}

#define keyWaiting() (endBuf != startBuf)

/* sleep for *tsecs tenths of a sec waiting for input */

int SLsys_input_pending(int tsecs)
{
   int count = tsecs * 5;

   if (count)
     {
	while(count > 0)
	  {
	     DosSleep(20);		       /* 20 ms or 1/50 sec */
	     if (keyWaiting ()) break;
	     count--;
	  }
	return(count);
     }
   else return(keyWaiting ());
}

unsigned int SLsys_getkey ()
{
   unsigned int c;
   unsigned char scan;
   
   int tsecs = 300;
   
   if (!keyWaiting()) 
     while (!SLsys_input_pending(tsecs));

   /* read codes from buffer */
   RequestSem();
   startBuf = (startBuf + 1) % BUFFER_LEN;
   c = threadKeys [startBuf].ascii;
   scan = threadKeys [startBuf].scan;
   ReleaseSem();

   if ((c == 8) && (scan == 0x0e)) c = 127;
   if (c == 0xE0) c = 0;
   if (c == 0) SLang_ungetkey (scan);
   return (c);
}


void SLang_set_abort_signal (void (*dum)(int))
{
   (void) dum;
}

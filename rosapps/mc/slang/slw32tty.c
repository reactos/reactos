/* Copyright (c) 1992, 1995 John E. Davis
 * All rights reserved.
 * 
 * You may distribute under the terms of either the GNU General Public
 * License or the Perl Artistic License.
 */

#include "config.h"
#include <stdio.h>

#include <windows.h>
#include <winbase.h>

#include "slang.h"
#include "_slang.h"

#ifdef __cplusplus
# define _DOTS_ ...
#else
# define _DOTS_ void
#endif



/*----------------------------------------------------------------------*\
 *  Function:	static void set_ctrl_break (int state);
 *
 * set the control-break setting
\*----------------------------------------------------------------------*/
static void set_ctrl_break (int state)
{
}


/*----------------------------------------------------------------------*\
 *  Function:	int SLang_init_tty (int abort_char, int no_flow_control,
 *				    int opost);
 *
 * initialize the keyboard interface and attempt to set-up the interrupt 9
 * handler if ABORT_CHAR is non-zero.
 * NO_FLOW_CONTROL and OPOST are only for compatiblity and are ignored.
\*----------------------------------------------------------------------*/

HANDLE hStdout, hStdin;
CONSOLE_SCREEN_BUFFER_INFO csbiInfo;

int SLang_init_tty (int abort_char, int no_flow_control, int opost)
{
  SMALL_RECT windowRect;
  COORD newPosition;
  long flags;

#ifndef SLANG_SAVES_CONSOLE
  /* first off, create a new console so the old one can be restored on exit */
  HANDLE console = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE,
					     FILE_SHARE_READ |FILE_SHARE_WRITE,
					     0,
					     CONSOLE_TEXTMODE_BUFFER,
					     0);
  if (SetConsoleActiveScreenBuffer(console) == FALSE) {
    return -1;
  }
#endif

  /* start things off at the origin */
  newPosition.X = 0;
  newPosition.Y = 0;

  /* still read in characters from stdin, but output to the new console */
  /* this way, on program exit, the original screen is restored */
  hStdin = GetStdHandle(STD_INPUT_HANDLE);
/*   hStdin = console; */

#ifndef SLANG_SAVES_CONSOLE
  hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
#else
  hStdout = console;
#endif

  if (hStdin == INVALID_HANDLE_VALUE || hStdout == INVALID_HANDLE_VALUE) {
    return -1; /* failure */
  }

  if (!GetConsoleScreenBufferInfo(hStdout, &csbiInfo)) {
    return -1; /* failure */
  } // if

  windowRect.Left = 0;
  windowRect.Top = 0;
  windowRect.Right = csbiInfo.srWindow.Right - csbiInfo.srWindow.Left; //dwMaximumWindowSize.X - 1;
  windowRect.Bottom = csbiInfo.srWindow.Bottom - csbiInfo.srWindow.Top; //dwMaximumWindowSize.Y - 1;
  if (!SetConsoleWindowInfo(hStdout, TRUE, &windowRect)) {
    return -1;
  }

  if (SetConsoleMode(hStdin, 0) == FALSE) {
    return -1; /* failure */
  }

  if (SetConsoleMode(hStdout, 0) == FALSE) {
    return -1; /* failure */
  }

  if (GetConsoleMode(hStdin, &flags)) {
    if (flags & ENABLE_PROCESSED_INPUT) {
      return -1;
    }
  } 

  (void) SetConsoleCursorPosition(hStdout, newPosition);  

  /* success */
  return 0;
} /* SLang_init_tty */

/*----------------------------------------------------------------------*\
 *  Function:	void SLang_reset_tty (void);
 *
 * reset the tty before exiting
\*----------------------------------------------------------------------*/
void SLang_reset_tty (void)
{
   set_ctrl_break (1);
}

/*----------------------------------------------------------------------*\
 *  Function:	int SLsys_input_pending (int tsecs);
 *
 *  sleep for *tsecs tenths of a sec waiting for input
\*----------------------------------------------------------------------*/
int SLsys_input_pending (int tsecs)
{
   INPUT_RECORD record;
   long one = 1;
   long bytesRead;

   while (1)
     {
	if (PeekConsoleInput(hStdin, &record, 1, &bytesRead)) 
	  {
	     if (bytesRead == 1) 
	       {
		  if ((record.EventType == KEY_EVENT)
		      && record.Event.KeyEvent.bKeyDown) 
		    {
		       /* ok, there is a keypress here */
		       return 1;
		    } 
		  else 
		    {
		       /* something else is here, so read it and try again */
		       (void) ReadConsoleInput(hStdin, &record, 1, &bytesRead);
		    }
	       }
	     else
	       {
		  /* no Pending events */
		  return 0;
	       }
	  }
	else
	  {
	     /* function failed */
	     return 0;
	  }
     }
#if 0
  /* no delays yet */
  /* use Sleep */
  /*
   int count = tsecs * 5;

   if (keyWaiting()) return 1;
   while (count > 0)
     {
	delay (20);	 20 ms or 1/50 sec 
	if (keyWaiting()) break;
	count--;
     }
   return (count);
   */
#endif
}

/*----------------------------------------------------------------------*\
 *  Function:	unsigned int SLsys_getkey (void);
 *
 * wait for and get the next available keystroke.
 * Also re-maps some useful keystrokes.
 *
 *	Backspace (^H)	=>	Del (127)
 *	Ctrl-Space	=>	^@	(^@^3 - a pc NUL char)
 *	extended keys are prefixed by a null character
\*----------------------------------------------------------------------*/
unsigned int SLsys_getkey (void)
{
  unsigned int scan, ch, shift;
  long key, bytesRead;
  INPUT_RECORD record;

  while (1) {
    if (!ReadConsoleInput(hStdin, &record, 1, &bytesRead)) {
      return 0;
    }
    if (record.EventType == KEY_EVENT && record.Event.KeyEvent.bKeyDown) {
/*#ifndef __MINGW32__*/
      return record.Event.KeyEvent.uChar.AsciiChar;
/*#else
      return record.Event.KeyEvent.AsciiChar;
#endif*/
    }
  }
/*   ReadFile(hStdin, &key, 1, &bytesRead, NULL); */

/*   return key; */
}

/*----------------------------------------------------------------------*\
 *  Function:	void SLang_set_abort_signal (void (*handler)(int));
\*----------------------------------------------------------------------*/
void SLang_set_abort_signal (void (*handler)(int))
{

}

/* trace_nt.c - Debugging routines
		  for Midnight Commander, under Win32
   
   Written 951215 by Juan Grigera <grigera@isis.unlp.edu.ar>
 
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
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  
 */
#include <config.h>
#ifdef HAVE_TRACE

#include <stdio.h>
#ifdef _OS_NT
#include <windows.h>
#endif
#include <errno.h>
#include "trace_nt.h"

/* Global variables */
int __win32_tracing_enabled = 1;  

static int _win32_tracing_started = 0;
static FILE *__win32_trace_f = NULL;

/* Definitions */
#define TRACE_FILE   "mcTrace.out"

/* Prototypes - static funcs */
static void _win32InitTrace (void);
static void _win32EndTrace (void);
static const char* GetLastErrorText(void);
static char *visbuf(const char *buf);


/*
    void _win32InitTrace()
	This func will open file TRACE_FILE for output and add _win32EndTrace to onexit 
	list of funcs.
 */
static void _win32InitTrace()
{
	if (!_win32_tracing_started) {
	   	_win32_tracing_started = 1;

	   	__win32_trace_f = fopen(TRACE_FILE, "wt");
		if (__win32_trace_f == NULL) {
			printf("Midnight Commander[DEBUG]: Can't open trace file '" TRACE_FILE "': %s \n", strerror(errno));
	   	}
		atexit (&_win32EndTrace);
	}
}

/*
    void _win32EndTrace()
	This func closes file TRACE_FILE if opened.
 */
static void _win32EndTrace()
{
	if (_win32_tracing_started) {
		_win32_tracing_started = 0;
		if (__win32_trace_f)
			fclose (__win32_trace_f);
	}	

}

/*
    void _win32Trace (char *fmt, ...)
	Format and output debug strings. They are written to TRACE_FILE.
	Debug Output is controlled by SetTrace (see below).
	Win32: Output is sent to Debug Output also.
 */
void _win32Trace (const char *fmt, ...)
{
	va_list ap;
	char buffer[256];
	char *vp;


	if (!_win32_tracing_started)
		_win32InitTrace();

	va_start(ap, fmt);
	vsprintf(buffer, fmt, ap);
	vp = buffer;

#ifdef _OS_NT					/* Write Output to Debug monitor also */
	OutputDebugString (vp);
	#if (_MSC_VER > 800)			/* Don't write newline in MSVC++ 1.0, has a dammed bug in Debug Output screen */
		OutputDebugString ("\n");
	#endif
#endif

	if(__win32_trace_f) 
		fprintf (__win32_trace_f, "%s\n", vp);
}

/*
	void SetTrace (int trace) 
	Control debug output. Turn it of or on.
		trace: 0 = off, 1 = on.
 */
void _win32SetTrace (int trace)
{
	/* Prototypes - interlan funcs */
	__win32_tracing_enabled = trace;
}
void _win32TraceOn ()
{
	__win32_tracing_enabled = 1;
}
void _win32TraceOff()
{
   	__win32_tracing_enabled = 0;
}


#ifdef _OS_NT
/*
	void DebugFailedWin32APICall (const char* name, int line, const char* file)
	Report a System call failure.
		name - text containing the source code that called the offending API func
		line, file - place of "name" in code

	See Also:  definition of win32APICALL macro.
 */
void _win32DebugFailedWin32APICall (const char* name, int line, const char* file)
{
	_win32Trace ("%s(%d): Call to Win32 API Failed. \"%s\".", file, line, name);
	_win32Trace ("        System Error (%d): %s. ", GetLastError(), GetLastErrorText());
}
#endif

/*
	void DebugAssertionFailed (const char* name, int line, const char* file)
	Report a logical condition failure. (e.g. a bad argument to a func)
		name - text containing the logical condition
		line, file - place of "name" in code

	See Also:  definition of ASSERT macro.
 */
void _win32DebugAssertionFailed (const char* name, int line, const char* file)
{
	_win32Trace ("%s(%d): Assertion failed! \"%s\".", file, line, name);
}


/*  const char* GetLastErrorText()
	Retrieves the text associated with the last system error.

	Returns pointer to static buffer. Contents valid till next call
*/
static const char* GetLastErrorText()
{
#define MAX_MSG_SIZE 256
    static char szMsgBuf[MAX_MSG_SIZE];
    DWORD dwError, dwRes;

    dwError = GetLastError ();

    dwRes = FormatMessage (
				FORMAT_MESSAGE_FROM_SYSTEM,
                NULL,
                dwError,
                MAKELANGID (LANG_ENGLISH, SUBLANG_ENGLISH_US),
				szMsgBuf,
				MAX_MSG_SIZE,
				NULL);
    if (0 == dwRes) {
		sprintf (szMsgBuf, "FormatMessage failed with %d", GetLastError());
    }
    return szMsgBuf;
}

#endif /*HAVE_TRACE*/

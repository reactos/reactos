/* trace_nt.h - Debugging routines

   Written by Juan Grigera<grigera@isis.unlp.edu.ar>

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

/* ------------------------------------------------------------------------------------------ *
   TRACER FUNCTIONS
 * ------------------------------------------------------------------------------------------ */

#ifdef HAVE_TRACE
/************************/
/*    Debug version     */
/************************/

/* Macros
   ------ 
   	win32Trace(x)	  - Trace macro. Use double in parenthesis for x. Same args as printf.
	win32ASSERT(x)	  - assert macro, but will not abort program and output sent to trace routine.	
	win32APICALL(x)  - Use to enclose a Win32 system call that should return TRUE. 
	win32APICALL_HANDLE(h,api)  - Use to enclose a Win32 system call that should return a handle. 
*/
#define win32Trace(x)				if (__win32_tracing_enabled) 	_win32Trace x
#define win32ASSERT(x)				if (!(x))    _win32DebugAssertionFailed (#x, __LINE__, __FILE__)
#define win32APICALL(x) 			if (!(x))    _win32DebugFailedWin32APICall (#x, __LINE__, __FILE__)
#define win32APICALL_HANDLE(h,api)	h=api; if (h==INVALID_HANDLE_VALUE)    _win32DebugFailedWin32APICall (#h" = "#api, __LINE__, __FILE__)

/* Prototypes        */
void _win32Trace (const char *, ...);
void _win32DebugFailedWin32APICall (const char *name, int line, const char *file);
void _win32DebugAssertionFailed (const char *name, int line, const char *file);

void _win32SetTrace (int trace);
void _win32TraceOn (void);
void _win32TraceOff (void);

#define SetTrace 	_win32SetTrace
#define TraceOn 	_win32TraceOn
#define TraceOff	_win32TraceOff

/* Global variables  */
extern int __win32_tracing_enabled;  

#else
/************************/
/*  Non-debug version   */
/************************/

/* Wipe-out these macros */
#define win32Trace(x)		
#define win32ASSERT(x)		
#define win32APICALL(x) 			x
#define win32APICALL_HANDLE(h,api)	h=api;

/* Wipe-out these funcs */
#define SetTrace(x)
#define TraceOn()
#define TraceOff()
#endif

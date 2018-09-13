/* common.h
 *
 * Common functions useful for Windows programs.
 *
 * InitializeDebugOutput(szAppName):
 *
 *	Read debug level for this application (named <szAppName>) from
 *	win.ini's [debug] section, which should look like this:
 *
 *	   [debug]
 *	   location=aux			; use OutputDebugString() to output
 *	   foobar=2			; this app has debug level 2
 *	   blorg=0			; this app has debug output disabled
 *
 *	If you want debug output to go to a file instead of to the AUX
 *	device (or the debugger), use "location=>filename".  To append to
 *	the file instead of rewriting the file, use "location=>>filename".
 *
 *	If DEBUG is not #define'd, then the call to InitializeDebugOutput()
 *	generates no code,
 *
 * TerminateDebugOutput():
 *
 *	End debug output for this application.  If DEBUG is not #define'd,
 *	then the call to InitializeDebugOutput() generates no code.
 *
 * DPF((szFormat, args...))
 *
 *	If debugging output for this applicaton is enabled (see
 *	InitializeDebugOutput()), print debug output specified by format
 *	string <szFormat>, which may contain wsprintf()-style formatting
 *	codes corresponding to arguments <args>.  Example:
 *
 *		DPF(("in WriteFile(): szFile='%s', dwFlags=0x%08lx\n",
 *	     	(LSPTR) szFile, dwFlags));
 *
 * DPF2((szFormat, args...))
 * DPF3((szFormat, args...))
 * DPF4((szFormat, args...))
 *
 *	Like DPF, but only output the debug string if the debug level for
 *	this application is at least 2, 3, or 4, respectively.
 *
 * Assert(fExpr)
 *
 *	If DEBUG is #define'd, then generate an "assertion failed" message box
 *	allowing the user to abort the program, enter the debugger (the "Retry"
 *	button), or ignore the error.  If DEBUG is not #define'd then Assert()
 *	generates no code.
 *
 * AssertEval(fExpr)
 *
 *	Like Assert(), but evaluate and return <fExpr>, even if DEBUG
 *	is not #define'd.  (Use if you want the BOOL expression to be
 *	evaluated even in a retail build.)
 */

#ifdef DEBUG
	/* Assert() macros */
	#undef Assert
	#undef AssertSz
	#undef AssertEval
        #define AssertSz(x,sz)           ((x) ? (void)0 : (void)_Assert(sz, __FILE__, __LINE__))
        #define Assert(expr)             AssertSz(expr, #expr)
        #define AssertEval(expr)         Assert(expr)

	/* debug printf macros */
	#define DPF( _x_ )	if (giDebugLevel >= 1) _DebugPrintf _x_
        #define DPF0( _x_ )                            _DebugPrintf _x_
	#define DPF1( _x_ )	if (giDebugLevel >= 1) _DebugPrintf _x_
	#define DPF2( _x_ )	if (giDebugLevel >= 2) _DebugPrintf _x_
	#define DPF3( _x_ )	if (giDebugLevel >= 3) _DebugPrintf _x_
        #define DPF4( _x_ )     if (giDebugLevel >= 4) _DebugPrintf _x_

        #define DOUT( _x_ )      if (giDebugLevel >= 1) {static char _based(_segname("_CODE")) smag[] = _x_; _DebugPrintf(smag); }
        #define DOUT0( _x_ )                            {static char _based(_segname("_CODE")) smag[] = _x_; _DebugPrintf(smag); }
        #define DOUT1( _x_ )     if (giDebugLevel >= 1) {static char _based(_segname("_CODE")) smag[] = _x_; _DebugPrintf(smag); }
        #define DOUT2( _x_ )     if (giDebugLevel >= 2) {static char _based(_segname("_CODE")) smag[] = _x_; _DebugPrintf(smag); }
        #define DOUT3( _x_ )     if (giDebugLevel >= 3) {static char _based(_segname("_CODE")) smag[] = _x_; _DebugPrintf(smag); }
        #define DOUT4( _x_ )     if (giDebugLevel >= 4) {static char _based(_segname("_CODE")) smag[] = _x_; _DebugPrintf(smag); }

	/* prototypes */
	void FAR PASCAL InitializeDebugOutput(LPSTR szAppName);
	void FAR PASCAL TerminateDebugOutput(void);
        void FAR PASCAL _Assert(char *szExp, char *szFile, int iLine);
        void FAR CDECL _DebugPrintf(LPSTR szFormat, ...);
        extern int	giDebugLevel;	// current debug level

#else
	/* Assert() macros */
        #define AssertSz(expr,x)         ((void)0)
        #define Assert(expr)             ((void)0)
	#define AssertEval(expr)	 (expr)

	/* debug printf macros */
	#define DPF( x )
	#define DPF0( x )
	#define DPF1( x )
	#define DPF2( x )
	#define DPF3( x )
        #define DPF4( x )

        #define DOUT( x )
        #define DOUT0( x )
        #define DOUT1( x )
        #define DOUT2( x )
        #define DOUT3( x )
        #define DOUT4( x )

	/* stubs for debugging function prototypes */
	#define InitializeDebugOutput(szAppName)	0
	#define TerminateDebugOutput()			0
#endif


/* flags for _llseek() */
#ifndef SEEK_SET
	#define SEEK_SET	0	// seek relative to start of file
	#define SEEK_CUR	1	// seek relative to current position
	#define SEEK_END	2	// seek relative to end of file
#endif


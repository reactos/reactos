/* common.c
 *
 * Common functions useful for Windows programs.
 */



//#include <windows.h>
#include <stdarg.h>
#include "graphic.h"
#include "profile.h"

#ifdef DEBUG  // On NT, ntavi.h might do an undef debug...
#include "common.h"

/* globals */
int		giDebugLevel = -1;	// current debug level (0 = disabled)
int		gfhDebugFile = -1;	// file handle for debug output (or -1)
int		giTimingLevel = -1;


/* InitializeDebugOutput(szAppName)
 *
 * Read the current debug level of this application (named <szAppName>)
 * from the [debug] section of win.ini, as well as the current location
 * for debug output.
 */
void FAR PASCAL
InitializeDebugOutput(LPSTR szAppName)
{
    char	achLocation[300]; // debug output location

    /* debugging is disabled by default (and if an error occurs below) */
    giDebugLevel = -1;
    gfhDebugFile = -1;

    /* get the debug output location */
    if ( (mmGetProfileStringA("debug", "Location", "", achLocation,
                         sizeof(achLocation)) == sizeof(achLocation)) ||
         (achLocation[0] == 0) )
    	return;

    if (achLocation[0] == '>')
    {
    	/* <achLocation> is the name of a file to overwrite (if
    	 * a single '>' is given) or append to (if '>>' is given)
    	 */
    	if (achLocation[1] == '>')
    		gfhDebugFile = _lopen(achLocation + 2, OF_WRITE);
    	else
    		gfhDebugFile = _lcreat(achLocation + 1, 0);
    	
    	if (gfhDebugFile < 0)
    		return;
    	
    	if (achLocation[1] == '>')
    		_llseek(gfhDebugFile, 0, SEEK_END);
    }
    else
    if (lstrcmpiA(achLocation, "aux") == 0)
    {
    	/* use OutputDebugString() for debug output */
    }
    else
    if ((lstrcmpiA(achLocation, "com1") == 0)
       || (lstrcmpiA(achLocation, "com2") == 0))
    {
            gfhDebugFile = _lopen(achLocation, OF_WRITE);
    }
    else
    {
    	/* invalid "location=" -- keep debugging disabled */
    	return;
    }

    /* get the debug level */
    giDebugLevel = mmGetProfileIntA("debug", szAppName, 0);
    giTimingLevel = mmGetProfileIntA("debug", "Timing", 0);
}


/* TerminateDebugOutput()
 *
 * Terminate debug output for this application.
 */
void FAR PASCAL
TerminateDebugOutput(void)
{
	if (gfhDebugFile >= 0)
		_lclose(gfhDebugFile);
	gfhDebugFile = -1;
	giDebugLevel = -1;
}


/* _Assert(szExpr, szFile, iLine)
 *
 * If <fExpr> is TRUE, then do nothing.  If <fExpr> is FALSE, then display
 * an "assertion failed" message box allowing the user to abort the program,
 * enter the debugger (the "Retry" button), or igore the error.
 *
 * <szFile> is the name of the source file; <iLine> is the line number
 * containing the _Assert() call.
 */
#ifndef _WIN32
#pragma optimize("", off)
#define ASSERTPREFIX
#else
#define ASSERTPREFIX "(NT) "
#endif

void FAR PASCAL
_Assert(char *szExp, char *szFile, int iLine)
{
	static char	ach[300];	// debug output (avoid stack overflow)
	int		id;
	int		iExitCode;
	void FAR PASCAL DebugBreak(void);

	/* display error message */

        if (szExp)
            wsprintfA(ach, ASSERTPREFIX "(%s)\nFile %s, line %d", (LPSTR)szExp, (LPSTR)szFile, iLine);
        else
            wsprintfA(ach, ASSERTPREFIX "File %s, line %d", (LPSTR)szFile, iLine);

	MessageBeep(MB_ICONHAND);
	id = MessageBoxA(NULL, ach, "Assertion Failed",
#ifdef BIDI
		MB_RTL_READING |
#endif
		MB_SYSTEMMODAL | MB_ICONHAND | MB_ABORTRETRYIGNORE);

	/* abort, debug, or ignore */
	switch (id)
	{

	case IDABORT:

		/* kill this application */
		iExitCode = 0;
#ifndef _WIN32
		_asm
		{
			mov	ah, 4Ch
			mov	al, BYTE PTR iExitCode
			int     21h
		}
#else
                FatalAppExit(0, TEXT("Good Bye"));
#endif // WIN16
		break;

	case IDRETRY:
		/* break into the debugger */
		DebugBreak();
		break;

	case IDIGNORE:
		/* ignore the assertion failure */
		break;
	}
}
#ifndef _WIN32
#pragma optimize("", on)
#endif

/* _DebugPrintf(szFormat, ...)
 *
 * If the application's debug level is at or above <iDebugLevel>,
 * then output debug string <szFormat> with formatting codes
 * replaced with arguments in the argument list pointed to by <szArg1>.
 */
#define MODNAME "MCIAVI"
void FAR CDECL
_DebugPrintf(LPSTR szFormat, ...)
{
	static char	ach[300];	// debug output (avoid stack overflow)
	int		cch;		// length of debug output string

#ifndef _WIN32
        NPSTR           pchSrc, pchDst;
        if (*szFormat == '!') {
            ++szFormat;
        }
        wvsprintf(ach, szFormat, (LPVOID)(&szFormat+1));
#else
        UINT n;
        va_list va;

        va_start(va, szFormat);
        if (*szFormat == '!') {
            ++szFormat;
            n=0;
        } else if (*szFormat=='.') {
            n=0;
        } else {
            n = wsprintfA(ach, MODNAME ": (tid %x) ", GetCurrentThreadId());
        }
        wvsprintfA(ach+n, szFormat, va);
        va_end(va);
#endif

#ifndef _WIN32
	/* expand the newlines into carrige-return-line-feed pairs;
	 * first, figure out how long the new (expanded) string will be
	 */
	for (pchSrc = pchDst = ach; *pchSrc != 0; pchSrc++, pchDst++)
		if (*pchSrc == '\n')
			pchDst++;
	
	/* is <ach> large enough? */
	cch = pchDst - ach;
        Assert(cch < sizeof(ach));
	*pchDst-- = 0;

	/* working backwards, expand \n's to \r\n's */
	while (pchSrc-- > ach)
		if ((*pchDst-- = *pchSrc) == '\n')
			*pchDst-- = '\r';

#else
       cch = strlen(ach);
#endif //no expansion on Win32
	/* output the debug string */
	if (gfhDebugFile > 0)
            _lwrite(gfhDebugFile, ach, cch);
	else {
            OutputDebugStringA(ach);
        }
}

#endif


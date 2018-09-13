/* Debug.c
 *
 * Debug functions for compression manager
 */


#include <windows.h>
#include <stdarg.h>
#include <win32.h>

#ifdef DEBUG  // See comments in NTAVI.H about DEBUG

#ifdef _WIN32
#include <profile.key>

/*
 * read a UINT from the profile, or return default if
 * not found.
 */
UINT mmGetProfileIntA(LPSTR appname, LPSTR valuename, INT uDefault)
{
    CHAR achName[MAX_PATH];
    HKEY hkey;
    DWORD dwType;
    INT value = uDefault;
    DWORD dwData;
    int cbData;

    lstrcpyA(achName, KEYNAMEA);
    lstrcatA(achName, appname);
    if (RegOpenKeyA(ROOTKEY, achName, &hkey) == ERROR_SUCCESS) {

        cbData = sizeof(dwData);
        if (RegQueryValueExA(
            hkey,
            valuename,
            NULL,
            &dwType,
            (PBYTE) &dwData,
            &cbData) == ERROR_SUCCESS) {

            if (dwType == REG_DWORD || dwType == REG_BINARY) {
                value = (INT)dwData;
#ifdef USESTRINGSALSO
            } else if (dwType == REG_SZ) {
	        value = atoi((LPSTR) &dwData);
#endif
    	    }
        }

        RegCloseKey(hkey);
    }

    return((UINT)value);
}
#endif

/* _Assert(fExpr, szFile, iLine)
 *
 * If <fExpr> is TRUE, then do nothing.  If <fExpr> is FALSE, then display
 * an "assertion failed" message box allowing the user to abort the program,
 * enter the debugger (the "Retry" button), or igore the error.
 *
 * <szFile> is the name of the source file; <iLine> is the line number
 * containing the _Assert() call.
 */
#pragma optimize("", off)
BOOL FAR PASCAL
_Assert(BOOL fExpr, LPSTR szFile, int iLine)
{
#ifdef _WIN32
	char	ach[300];	
#else
	static char	ach[300];	// debug output (avoid stack overflow)
#endif
	int		id;
	int		iExitCode;
	void FAR PASCAL DebugBreak(void);

	/* check if assertion failed */
	if (fExpr)
		return fExpr;

	/* display error message */
	wsprintfA(ach, "File %s, line %d", (LPSTR) szFile, iLine);
	MessageBeep(MB_ICONHAND);
	id = MessageBoxA(NULL, ach, "Assertion Failed",
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
	
	return FALSE;
}
#pragma optimize("", on)

#endif

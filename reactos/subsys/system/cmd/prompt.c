/*
 *  PROMPT.C - prompt handling.
 *
 *
 *  History:
 *
 *    14/01/95 (Tim Normal)
 *        started.
 *
 *    08/08/95 (Matt Rains)
 *        i have cleaned up the source code. changes now bring this source
 *        into guidelines for recommended programming practice.
 *
 *    01/06/96 (Tim Norman)
 *        added day of the week printing (oops, forgot about that!)
 *
 *    08/07/96 (Steffan Kaiser)
 *        small changes for speed
 *
 *    20-Jul-1998 (John P Price <linux-guru@gcfl.net>)
 *        removed redundant day strings. Use ones defined in date.c.
 *
 *    27-Jul-1998 (John P Price <linux-guru@gcfl.net>)
 *        added config.h include
 *
 *    28-Jul-1998 (John P Price <linux-guru@gcfl.net>)
 *        moved cmd_prompt from internal.c to here
 *
 *    09-Dec-1998 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Added help text ("/?").
 *
 *    14-Dec-1998 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Added "$+" option.
 *
 *    09-Jan-1999 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Added "$A", "$C" and "$F" option.
 *        Added locale support.
 *        Fixed "$V" option.
 *
 *    20-Jan-1999 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Unicode and redirection safe!
 *
 *    24-Jan-1999 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Fixed Win32 environment handling.
 */

#include "config.h"

#include <windows.h>
#include <tchar.h>
#include <string.h>
#include <ctype.h>

#include "cmd.h"


/*
 * print the command-line prompt
 *
 */
VOID PrintPrompt(VOID)
{
	static TCHAR default_pr[] = _T("$P$G");
	TCHAR  szPrompt[256];
	LPTSTR pr;

	if (GetEnvironmentVariable (_T("PROMPT"), szPrompt, 256))
		pr = szPrompt;
	else
		pr = default_pr;

	while (*pr)
	{
		if (*pr != _T('$'))
		{
			ConOutChar (*pr);
		}
		else
		{
			pr++;

			switch (_totupper (*pr))
			{
				case _T('A'):
					ConOutChar (_T('&'));
					break;

				case _T('B'):
					ConOutChar (_T('|'));
					break;

				case _T('C'):
					ConOutChar (_T('('));
					break;

				case _T('D'):
					PrintDate ();
					break;

				case _T('E'):
					ConOutChar (_T('\x1B'));
					break;

				case _T('F'):
					ConOutChar (_T(')'));
					break;

				case _T('G'):
					ConOutChar (_T('>'));
					break;

				case _T('H'):
					ConOutChar (_T('\x08'));
					break;

				case _T('L'):
					ConOutChar (_T('<'));
					break;

				case _T('N'):
					{
						TCHAR szPath[MAX_PATH];
						GetCurrentDirectory (MAX_PATH, szPath);
						ConOutChar (szPath[0]);
					}
					break;

				case _T('P'):
					{
						TCHAR szPath[MAX_PATH];
						GetCurrentDirectory (MAX_PATH, szPath);
						ConOutPrintf (_T("%s"), szPath);
					}
					break;

				case _T('Q'):
					ConOutChar (_T('='));
					break;

				case _T('T'):
					PrintTime ();
					break;

				case _T('V'):
					switch (osvi.dwPlatformId)
					{
						case VER_PLATFORM_WIN32_WINDOWS:
							if (osvi.dwMajorVersion == 4 &&
								osvi.dwMinorVersion == 1)
								ConOutPrintf (_T("Windows 98"));
							else
								ConOutPrintf (_T("Windows 95"));
							break;

						case VER_PLATFORM_WIN32_NT:
							ConOutPrintf (_T("Windows NT Version %lu.%lu"),
										  osvi.dwMajorVersion, osvi.dwMinorVersion);
							break;
					}
					break;

				case _T('_'):
					ConOutChar (_T('\n'));
					break;

				case '$':
					ConOutChar (_T('$'));
					break;

#ifdef FEATURE_DIRECTORY_STACK
				case '+':
					{
						INT i;
						for (i = 0; i < GetDirectoryStackDepth (); i++)
							ConOutChar (_T('+'));
					}
					break;
#endif
			}
		}
		pr++;
	}
}


#ifdef INCLUDE_CMD_PROMPT

INT cmd_prompt (LPTSTR cmd, LPTSTR param)
{
	if (!_tcsncmp (param, _T("/?"), 2))
	{
		ConOutPuts (_T("Changes the command prompt.\n\n"
			  "PROMPT [text]\n\n"
			  "  text    Specifies a new command prompt.\n\n"
			  "Prompt can be made up of normal characters and the following special codes:\n\n"
			  "  $A   & (Ampersand)\n"
			  "  $B   | (pipe)\n"
			  "  $C   ( (Left parenthesis)\n"
			  "  $D   Current date\n"
			  "  $E   Escape code (ASCII code 27)\n"
			  "  $F   ) (Right parenthesis)\n"
			  "  $G   > (greater-than sign)\n"
			  "  $H   Backspace (erases previous character)\n"
			  "  $L   < (less-than sign)\n"
			  "  $N   Current drive\n"
			  "  $P   Current drive and path\n"
			  "  $Q   = (equal sign)\n"
			  "  $T   Current time\n"
			  "  $V   OS version number\n"
			  "  $_   Carriage return and linefeed\n"
			  "  $$   $ (dollar sign)"));
#ifdef FEATURE_DIRECTORY_STACK
		ConOutPuts (_T("  $+   Displays the current depth of the directory stack"));
#endif
		ConOutPuts (_T("\nType PROMPT without parameters to reset the prompt to the default setting."));
		return 0;
	}

	/* set PROMPT environment variable */
	if (!SetEnvironmentVariable (_T("PROMPT"), param))
		return 1;

	return 0;
}
#endif

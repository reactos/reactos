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
 *    09-Dec-1998 (Eric Kohl)
 *        Added help text ("/?").
 *
 *    14-Dec-1998 (Eric Kohl)
 *        Added "$+" option.
 *
 *    09-Jan-1999 (Eric Kohl)
 *        Added "$A", "$C" and "$F" option.
 *        Added locale support.
 *        Fixed "$V" option.
 *
 *    20-Jan-1999 (Eric Kohl)
 *        Unicode and redirection safe!
 *
 *    24-Jan-1999 (Eric Kohl)
 *        Fixed Win32 environment handling.
 *
 *    30-Apr-2005 (Magnus Olsen) <magnus@greatlord.com>)
 *        Remove all hardcode string to En.rc
 */
#include <precomp.h>

/*
 * print the command-line prompt
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
					ConOutPrintf(_T("%s"), GetDateString());
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
          ConOutChar (_T(' '));
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

        case _T('S'):
					ConOutChar (_T(' '));
					break;

				case _T('T'):
					ConOutPrintf(_T("%s"), GetTimeString());
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

INT cmd_prompt (LPTSTR param)
{
	if (!_tcsncmp (param, _T("/?"), 2))
	{
		ConOutResPaging(TRUE,STRING_PROMPT_HELP1);

#ifdef FEATURE_DIRECTORY_STACK
		ConOutResPaging(FALSE,STRING_PROMPT_HELP2);
#endif
		ConOutResPaging(FALSE,STRING_PROMPT_HELP3);
		return 0;
	}

	/* if it is null, then it needs to set to default,
	   because that means the user entered "prompt" only.
		so even if param is null you _must_ still set prompt
		to the default.  There seems to be some kinda difference
		between winxp and 2k in this matter and this way will
		cover both. Do not use fixed size of szParam for param the buffer are 8192bytes
		and will later change to dymatic buffer */

	/* set PROMPT environment variable */
	if (param[0] != _T('\0'))
	{
		if (!SetEnvironmentVariable (_T("PROMPT"), param))
		return 1;
	}
	else
	{
		TCHAR szParam[5];
		_tcscpy(szParam,_T("$P$G"));
		if (!SetEnvironmentVariable (_T("PROMPT"),szParam))
		return 1;
	}



	return 0;
}
#endif

/* EOF */

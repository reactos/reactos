/*
 *  WHERE.C - file search functions.
 *
 *
 *  History:
 *
 *    07/15/95 (Tim Norman)
 *        started.
 *
 *    08/08/95 (Matt Rains)
 *        i have cleaned up the source code. changes now bring this source
 *        into guidelines for recommended programming practice.
 *
 *    12/12/95 (Steffan Kaiser & Tim Norman)
 *        added some patches to fix some things and make more efficient
 *
 *    1/6/96 (Tim Norman)
 *        fixed a stupid pointer mistake...
 *        Thanks to everyone who noticed it!
 *
 *    8/1/96 (Tim Norman)
 *        fixed a bug when getenv returns NULL
 *
 *    8/7/96 (Steffan Kaiser and Tim Norman)
 *        speed improvements and bug fixes
 *
 *    8/27/96 (Tim Norman)
 *        changed code to use pointers directly into PATH environment
 *        variable rather than making our own copy.  This saves some memory,
 *        but requires we write our own function to copy pathnames out of
 *        the variable.
 *
 *    12/23/96 (Aaron Kaufman)
 *        Fixed a bug in get_paths() that did not point to the first PATH
 *        in the environment variable.
 *
 *    7/12/97 (Tim Norman)
 *        Apparently, Aaron's bugfix got lost, so I fixed it again.
 *
 *    16 July 1998 (John P. Price)
 *        Added stand alone code.
 *
 *    17 July 1998 (John P. Price)
 *        Rewrote find_which to use searchpath function
 *
 *    24-Jul-1998 (John P Price <linux-guru@gcfl.net>)
 *        fixed bug where didn't check all extensions when path was specified
 *
 *    27-Jul-1998 (John P Price <linux-guru@gcfl.net>)
 *        added config.h include
 *
 *    30-Jul-1998 (John P Price <linux-guru@gcfl.net>)
 *        fixed so that it find_which returns NULL if filename is not
 *        executable (does not have .bat, .com, or .exe extention).
 *        Before command would to execute any file with any extension (opps!)
 *
 *    03-Dec-1998 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Changed find_which().
 *
 *    07-Dec-1998 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Added ".CMD" extension.
 *        Replaced numeric constant by _NR_OF_EXTENSIONS.
 *
 *    26-Feb-1999 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Replaced find_which() by SearchForExecutable().
 *        Now files are searched using the right extension order.
 *
 *    20-Apr-1999 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Some minor changes and improvements.
 */

#include "config.h"

#include <windows.h>
#include <string.h>
#include <stdlib.h>

#include "cmd.h"


/* initial size of environment variable buffer */
#define ENV_BUFFER_SIZE  1024


static LPTSTR ext[]  = {".bat", ".cmd", ".com", ".exe"};
static INT nExtCount = sizeof(ext) / sizeof(LPTSTR);


/* searches for file using path info. */

BOOL
SearchForExecutable (LPCTSTR pFileName, LPTSTR pFullName)
{
	TCHAR  szPathBuffer[MAX_PATH];
	LPTSTR pszBuffer = NULL;
	DWORD  dwBuffer, len;
	INT    n;
	LPTSTR p,s,f;


	/* initialize full name buffer */
	*pFullName = _T('\0');

#ifdef _DEBUG
        DebugPrintf (_T("SearchForExecutable: \'%s\'\n"), pFileName);
#endif

        if (_tcschr (pFileName, _T('\\')) != NULL)
        {
                LPTSTR pFilePart;
#ifdef _DEBUG
                DebugPrintf (_T("Absolute or relative path is given.\n"));
#endif
                

				
				
				if (GetFullPathName (pFileName,
						             MAX_PATH,
							         szPathBuffer,
								     &pFilePart)  ==0)								
					return FALSE;

				
				if(pFilePart == 0)
					return FALSE;
				


                if (_tcschr (pFilePart, _T('.')) != NULL)
                {
#ifdef _DEBUG
                        DebugPrintf (_T("Filename extension!\n"));
#endif
                        _tcscpy (pFullName, szPathBuffer);
                        return TRUE;

                }
                else
                {
#ifdef _DEBUG
                        DebugPrintf (_T("No filename extension!\n"));
#endif

                        p = szPathBuffer + _tcslen (szPathBuffer);

                        for (n = 0; n < nExtCount; n++)
                        {
                                _tcscpy (p, ext[n]);

#ifdef _DEBUG
                                DebugPrintf (_T("Testing: \'%s\'\n"), szPathBuffer);
#endif

                                if (IsValidFileName (szPathBuffer))
                                {
#ifdef _DEBUG
                                        DebugPrintf (_T("Found: \'%s\'\n"), szPathBuffer);
#endif
                                        _tcscpy (pFullName, szPathBuffer);
                                        return TRUE;
                                }
                        }
                        return FALSE;
                }
        }

	/* load environment varable PATH into buffer */
	pszBuffer = (LPTSTR)malloc (ENV_BUFFER_SIZE * sizeof(TCHAR));
	dwBuffer = GetEnvironmentVariable (_T("PATH"), pszBuffer, ENV_BUFFER_SIZE);
	if (dwBuffer > ENV_BUFFER_SIZE)
	{
		pszBuffer = (LPTSTR)realloc (pszBuffer, dwBuffer * sizeof (TCHAR));
		GetEnvironmentVariable (_T("PATH"), pszBuffer, dwBuffer * sizeof (TCHAR));
	}

	if (!(p = _tcsrchr (pFileName, _T('.'))) ||
		_tcschr (p + 1, _T('\\')))
	{
		/* There is no extension ==> test all the extensions. */
#ifdef _DEBUG
		DebugPrintf (_T("No filename extension!\n"));
#endif

		/* search in current directory */
		len = GetCurrentDirectory (MAX_PATH, szPathBuffer);
		if (szPathBuffer[len - 1] != _T('\\'))
		{
			szPathBuffer[len] = _T('\\');
			szPathBuffer[len + 1] = _T('\0');
		}
		_tcscat (szPathBuffer, pFileName);

		p = szPathBuffer + _tcslen (szPathBuffer);

		for (n = 0; n < nExtCount; n++)
		{
			_tcscpy (p, ext[n]);

#ifdef _DEBUG
			DebugPrintf (_T("Testing: \'%s\'\n"), szPathBuffer);
#endif

			if (IsValidFileName (szPathBuffer))
			{
#ifdef _DEBUG
				DebugPrintf (_T("Found: \'%s\'\n"), szPathBuffer);
#endif
				free (pszBuffer);
				_tcscpy (pFullName, szPathBuffer);
				return TRUE;
			}
		}

		/* search in PATH */
		s = pszBuffer;
		while (s && *s)
		{
			f = _tcschr (s, _T(';'));

			if (f)
			{
				_tcsncpy (szPathBuffer, s, (size_t)(f-s));
				szPathBuffer[f-s] = _T('\0');
				s = f + 1;
			}
			else
			{
				_tcscpy (szPathBuffer, s);
				s = NULL;
			}

			len = _tcslen(szPathBuffer);
			if (szPathBuffer[len - 1] != _T('\\'))
			{
				szPathBuffer[len] = _T('\\');
				szPathBuffer[len + 1] = _T('\0');
			}
			_tcscat (szPathBuffer, pFileName);

			p = szPathBuffer + _tcslen (szPathBuffer);

			for (n = 0; n < nExtCount; n++)
			{
				_tcscpy (p, ext[n]);

#ifdef _DEBUG
				DebugPrintf (_T("Testing: \'%s\'\n"), szPathBuffer);
#endif

				if (IsValidFileName (szPathBuffer))
				{
#ifdef _DEBUG
					DebugPrintf (_T("Found: \'%s\'\n"), szPathBuffer);
#endif
					free (pszBuffer);
					_tcscpy (pFullName, szPathBuffer);
					return TRUE;
				}
			}
		}
	}
	else
	{
		/* There is an extension and it is in the last path component, */
		/* so don't test all the extensions. */
#ifdef _DEBUG
		DebugPrintf (_T("Filename extension!\n"));
#endif

		/* search in current directory */
		len = GetCurrentDirectory (MAX_PATH, szPathBuffer);
		if (szPathBuffer[len - 1] != _T('\\'))
		{
			szPathBuffer[len] = _T('\\');
			szPathBuffer[len + 1] = _T('\0');
		}
		_tcscat (szPathBuffer, pFileName);

#ifdef _DEBUG
		DebugPrintf (_T("Testing: \'%s\'\n"), szPathBuffer);
#endif
		if (IsValidFileName (szPathBuffer))
		{
#ifdef _DEBUG
			DebugPrintf (_T("Found: \'%s\'\n"), szPathBuffer);
#endif
			free (pszBuffer);
			_tcscpy (pFullName, szPathBuffer);
			return TRUE;
		}


		/* search in PATH */
		s = pszBuffer;
		while (s && *s)
		{
			f = _tcschr (s, _T(';'));

			if (f)
			{
				_tcsncpy (szPathBuffer, s, (size_t)(f-s));
				szPathBuffer[f-s] = _T('\0');
				s = f + 1;
			}
			else
			{
				_tcscpy (szPathBuffer, s);
				s = NULL;
			}

			len = _tcslen(szPathBuffer);
			if (szPathBuffer[len - 1] != _T('\\'))
			{
				szPathBuffer[len] = _T('\\');
				szPathBuffer[len + 1] = _T('\0');
			}
			_tcscat (szPathBuffer, pFileName);

#ifdef _DEBUG
			DebugPrintf (_T("Testing: \'%s\'\n"), szPathBuffer);
#endif
			if (IsValidFileName (szPathBuffer))
			{
#ifdef _DEBUG
				DebugPrintf (_T("Found: \'%s\'\n"), szPathBuffer);
#endif
				free (pszBuffer);
				_tcscpy (pFullName, szPathBuffer);
				return TRUE;
			}
		}
	}

	free (pszBuffer);

	return FALSE;
}

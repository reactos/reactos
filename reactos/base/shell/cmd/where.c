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
 *    03-Dec-1998 (Eric Kohl)
 *        Changed find_which().
 *
 *    07-Dec-1998 (Eric Kohl)
 *        Added ".CMD" extension.
 *        Replaced numeric constant by _NR_OF_EXTENSIONS.
 *
 *    26-Feb-1999 (Eric Kohl)
 *        Replaced find_which() by SearchForExecutable().
 *        Now files are searched using the right extension order.
 *
 *    20-Apr-1999 (Eric Kohl)
 *        Some minor changes and improvements.
 *
 *    10-Jul-2004 (Jens Collin <jens.collin@lakhei.com>)
 *        Fixed searxhing for files with specific extensions in PATHEXT order..
 *
 */

#include <precomp.h>


/* initial size of environment variable buffer */
#define ENV_BUFFER_SIZE  1024


/* searches for file using path info. */

BOOL
SearchForExecutableSingle (LPCTSTR pFileName, LPTSTR pFullName, LPTSTR pExtension)
{
	TCHAR  szPathBuffer[CMDLINE_LENGTH];
	LPTSTR pszBuffer = NULL;
	DWORD  dwBuffer, len;
	LPTSTR s,f;
	/* initialize full name buffer */
	*pFullName = _T('\0');

#ifdef _DEBUG
	DebugPrintf (_T("SearchForExecutableSingle: \'%s\' with ext: \'%s\'\n"), pFileName, pExtension);
#endif

	/* Check if valid directly on specified path */
	if (_tcschr (pFileName, _T('\\')) != NULL)
	{
		LPTSTR pFilePart;
#ifdef _DEBUG
		DebugPrintf (_T("Absolute or relative path is given.\n"));
#endif

		if (GetFullPathName (pFileName,
			             CMDLINE_LENGTH,
			             szPathBuffer,
			             &pFilePart)  ==0)
			return FALSE;

		if(pFilePart == 0)
			return FALSE;
		/* Add extension and test file: */
		if (pExtension)
			_tcscat(szPathBuffer, pExtension);

		if (IsExistingFile (szPathBuffer))
		{
#ifdef _DEBUG
			DebugPrintf (_T("Found: \'%s\'\n"), szPathBuffer);
#endif
			_tcscpy (pFullName, szPathBuffer);
			return TRUE;
		}
		return FALSE;
	}

	/* search in current directory */
	len = GetCurrentDirectory (CMDLINE_LENGTH, szPathBuffer);
	if (szPathBuffer[len - 1] != _T('\\'))
	{
		szPathBuffer[len] = _T('\\');
		szPathBuffer[len + 1] = _T('\0');
	}
	_tcscat (szPathBuffer, pFileName);

	if (pExtension)
		_tcscat (szPathBuffer, pExtension);

	if (IsExistingFile (szPathBuffer))
	{
#ifdef _DEBUG
		DebugPrintf (_T("Found: \'%s\'\n"), szPathBuffer);
#endif
		_tcscpy (pFullName, szPathBuffer);
		return TRUE;
	}



	/* load environment varable PATH into buffer */
	pszBuffer = (LPTSTR)cmd_alloc (ENV_BUFFER_SIZE * sizeof(TCHAR));
	dwBuffer = GetEnvironmentVariable (_T("PATH"), pszBuffer, ENV_BUFFER_SIZE);
	if (dwBuffer > ENV_BUFFER_SIZE)
	{
		pszBuffer = (LPTSTR)cmd_realloc (pszBuffer, dwBuffer * sizeof (TCHAR));
		GetEnvironmentVariable (_T("PATH"), pszBuffer, dwBuffer);
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

		if (pExtension)
			_tcscat (szPathBuffer, pExtension);

		if (IsExistingFile (szPathBuffer))
		{
#ifdef _DEBUG
			DebugPrintf (_T("Found: \'%s\'\n"), szPathBuffer);
#endif
			cmd_free (pszBuffer);
			_tcscpy (pFullName, szPathBuffer);
			return TRUE;
		}
	}
	cmd_free (pszBuffer);
	return FALSE;
}


BOOL
SearchForExecutable (LPCTSTR pFileName, LPTSTR pFullName)
{
	static TCHAR pszDefaultPathExt[] = _T(".COM;.EXE;.BAT;.CMD");
	LPTSTR pszBuffer = NULL;
	LPTSTR pCh;
	LPTSTR pExt;
	DWORD  dwBuffer;
#ifdef _DEBUG
	DebugPrintf (_T("SearchForExecutable: \'%s\'\n"), pFileName);
#endif
	/* load environment varable PATHEXT */
	pszBuffer = (LPTSTR)cmd_alloc (ENV_BUFFER_SIZE * sizeof(TCHAR));
	dwBuffer = GetEnvironmentVariable (_T("PATHEXT"), pszBuffer, ENV_BUFFER_SIZE);
	if (dwBuffer > ENV_BUFFER_SIZE)
	{
		pszBuffer = (LPTSTR)cmd_realloc (pszBuffer, dwBuffer * sizeof (TCHAR));
		GetEnvironmentVariable (_T("PATHEXT"), pszBuffer, dwBuffer);
	}
	else if (0 == dwBuffer)
	{
		_tcscpy(pszBuffer, pszDefaultPathExt);
	}

#ifdef _DEBUG
	DebugPrintf (_T("SearchForExecutable(): Loaded PATHEXT: %s\n"), pszBuffer);
#endif

	pExt = _tcsrchr(pFileName, _T('.'));
	if (pExt != NULL)
	{
		LPTSTR pszBuffer2;
		pszBuffer2 = cmd_dup(pszBuffer);
		if (pszBuffer2)
		{
			pCh = _tcstok(pszBuffer2, _T(";"));
			while (pCh)
			{
				if (0 == _tcsicmp(pCh, pExt))
				{
					cmd_free(pszBuffer);
					cmd_free(pszBuffer2);
					return SearchForExecutableSingle(pFileName, pFullName, NULL);
				}
				pCh = _tcstok(NULL, _T(";"));
			}
			cmd_free(pszBuffer2);
		}
	}

	pCh = _tcstok(pszBuffer, _T(";"));
	while (pCh)
	{
		if (SearchForExecutableSingle(pFileName, pFullName, pCh))
		{
			cmd_free(pszBuffer);
			return TRUE;
		}
		pCh = _tcstok(NULL, _T(";"));
	}

	cmd_free(pszBuffer);
	return FALSE;
}

/* EOF */

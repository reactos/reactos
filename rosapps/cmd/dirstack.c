/*
 *  DIRSTACK.C - pushd / pop (directory stack) internal commands.
 *
 *
 *  History:
 *
 *    14-Dec-1998 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Implemented PUSHD and POPD command.
 *
 *    20-Jan-1999 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Unicode and redirection safe!
 */

#define WIN32_LEAN_AND_MEAN

#include "config.h"

#ifdef FEATURE_DIRECTORY_STACK

#include <windows.h>
#include <tchar.h>
#include <string.h>
#include <stdlib.h>

#include "cmd.h"


typedef struct tagDIRENTRY
{
	struct tagDIRENTRY *next;
	LPTSTR pszPath;
} DIRENTRY, *LPDIRENTRY;


static INT nStackDepth;
static LPDIRENTRY lpStackTop;


static INT
PushDirectory (LPTSTR pszPath)
{
	LPDIRENTRY lpDir;

	lpDir = (LPDIRENTRY)malloc (sizeof (DIRENTRY));
	if (!lpDir)
	{
		error_out_of_memory ();
		return -1;
	}

	lpDir->next = (lpStackTop) ? lpStackTop : NULL;
	lpStackTop = lpDir;

	lpDir->pszPath = (LPTSTR)malloc ((_tcslen(pszPath)+1)*sizeof(TCHAR));
	if (!lpDir->pszPath)
	{
		free (lpDir);
		error_out_of_memory ();
		return -1;
	}

	_tcscpy (lpDir->pszPath, pszPath);

	nStackDepth++;

	return 0;
}


static VOID
PopDirectory (VOID)
{
	LPDIRENTRY lpDir;

	if (nStackDepth == 0)
		return;

	lpDir = lpStackTop;
	lpStackTop = lpDir->next;
	free (lpDir->pszPath);
	free (lpDir);

	nStackDepth--;
}


static VOID
GetDirectoryStackTop (LPTSTR pszPath)
{
	if (lpStackTop)
		_tcsncpy (pszPath, lpStackTop->pszPath, MAX_PATH);
	else
		*pszPath = _T('\0');
}


/*
 * initialize directory stack
 */
VOID InitDirectoryStack (VOID)
{
	nStackDepth = 0;
	lpStackTop = NULL;
}


/*
 * destroy directory stack
 */
VOID DestroyDirectoryStack (VOID)
{
	while (nStackDepth)
		PopDirectory ();
}


INT GetDirectoryStackDepth (VOID)
{
	return nStackDepth;
}


/*
 * pushd command
 */
INT cmd_pushd (LPTSTR first, LPTSTR rest)
{
	TCHAR curPath[MAX_PATH];
	TCHAR newPath[MAX_PATH];
	BOOL  bChangePath = FALSE;

	if (!_tcsncmp (rest, _T("/?"), 2))
	{
		ConOutPuts (_T("Stores the current directory for use by the POPD command, then\n"
			  "changes to the specified directory.\n\n"
			  "PUSHD [path | ..]\n\n"
			  "  path        Specifies the directory to make the current directory"));
		return 0;
	}

	if (rest[0] != _T('\0'))
	{
		GetFullPathName (rest, MAX_PATH, newPath, NULL);
		bChangePath = IsValidPathName (newPath);
	}

	GetCurrentDirectory (MAX_PATH, curPath);
	if (PushDirectory (curPath))
		return -1;

	if (bChangePath)
		SetCurrentDirectory (newPath);

	return 0;
}


/*
 * popd command
 */
INT cmd_popd (LPTSTR first, LPTSTR rest)
{
	TCHAR szPath[MAX_PATH];

	if (!_tcsncmp(rest, _T("/?"), 2))
	{
		ConOutPuts (_T("Changes to the directory stored by the PUSHD command.\n\n"
					   "POPD"));
		return 0;
	}

	if (GetDirectoryStackDepth () == 0)
		return 0;

	GetDirectoryStackTop (szPath);
	PopDirectory ();

	SetCurrentDirectory (szPath);

	return 0;
}

#endif /* FEATURE_DIRECTORY_STACK */
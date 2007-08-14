/*
 *  DIRSTACK.C - pushd / pop (directory stack) internal commands.
 *
 *
 *  History:
 *
 *    14-Dec-1998 (Eric Kohl)
 *        Implemented PUSHD and POPD command.
 *
 *    20-Jan-1999 (Eric Kohl)
 *        Unicode and redirection safe!
 *
 *    20-Jan-1999 (Eric Kohl)
 *        Added DIRS command.
 */

#include <precomp.h>

#ifdef FEATURE_DIRECTORY_STACK

typedef struct tagDIRENTRY
{
	struct tagDIRENTRY *prev;
	struct tagDIRENTRY *next;
	LPTSTR pszPath;
} DIRENTRY, *LPDIRENTRY;


static INT nStackDepth;
static LPDIRENTRY lpStackTop;
static LPDIRENTRY lpStackBottom;


static INT
PushDirectory (LPTSTR pszPath)
{
	LPDIRENTRY lpDir;

	nErrorLevel = 0;

	lpDir = (LPDIRENTRY)cmd_alloc (sizeof (DIRENTRY));
	if (!lpDir)
	{
		error_out_of_memory ();
		return -1;
	}

	lpDir->prev = NULL;
	if (lpStackTop == NULL)
	{
		lpDir->next = NULL;
		lpStackBottom = lpDir;
	}
	else
	{
		lpDir->next = lpStackTop;
		lpStackTop->prev = lpDir;
	}
	lpStackTop = lpDir;

	lpDir->pszPath = (LPTSTR)cmd_alloc ((_tcslen(pszPath)+1)*sizeof(TCHAR));
	if (!lpDir->pszPath)
	{
		cmd_free (lpDir);
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

    nErrorLevel = 0;

	if (nStackDepth == 0)
		return;

	lpDir = lpStackTop;
	lpStackTop = lpDir->next;
	if (lpStackTop != NULL)
		lpStackTop->prev = NULL;
	else
		lpStackBottom = NULL;

	cmd_free (lpDir->pszPath);
	cmd_free (lpDir);

	nStackDepth--;
}


static VOID
GetDirectoryStackTop (LPTSTR pszPath)
{
	nErrorLevel = 0;

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
	lpStackBottom = NULL;
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
INT CommandPushd (LPTSTR first, LPTSTR rest)
{
	TCHAR curPath[MAX_PATH];
	TCHAR newPath[MAX_PATH];
	BOOL  bChangePath = FALSE;

	if (!_tcsncmp (rest, _T("/?"), 2))
	{
		ConOutResPuts(STRING_DIRSTACK_HELP1);
		return 0;
	}

	nErrorLevel = 0;

	if (rest[0] != _T('\0'))
	{
		GetFullPathName (rest, MAX_PATH, newPath, NULL);
		bChangePath = IsValidPathName (newPath);
	}

	GetCurrentDirectory (MAX_PATH, curPath);
	if (PushDirectory (curPath))
		return 0;

	if (bChangePath)
		SetCurrentDirectory (newPath);

	return 0;
}


/*
 * popd command
 */
INT CommandPopd (LPTSTR first, LPTSTR rest)
{
	TCHAR szPath[MAX_PATH];

	if (!_tcsncmp(rest, _T("/?"), 2))
	{
		ConOutResPuts(STRING_DIRSTACK_HELP2);
		return 0;
	}

	nErrorLevel = 0;

	if (GetDirectoryStackDepth () == 0)
		return 0;

	GetDirectoryStackTop (szPath);
	PopDirectory ();

	SetCurrentDirectory (szPath);

	return 0;
}


/*
 * dirs command
 */
INT CommandDirs (LPTSTR first, LPTSTR rest)
{
	LPDIRENTRY lpDir;

	if (!_tcsncmp(rest, _T("/?"), 2))
	{
		ConOutResPuts(STRING_DIRSTACK_HELP3);
		return 0;
	}

    nErrorLevel = 0;

	lpDir = lpStackBottom;

	if (lpDir == NULL)
	{
		ConOutResPuts(STRING_DIRSTACK_HELP4);
		return 0;
	}

	while (lpDir != NULL)
	{
		ConOutPuts (lpDir->pszPath);

		lpDir = lpDir->prev;
	}

	return 0;
}

#endif /* FEATURE_DIRECTORY_STACK */

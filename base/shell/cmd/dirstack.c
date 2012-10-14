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
	TCHAR szPath[1];
} DIRENTRY, *LPDIRENTRY;


static INT nStackDepth;
static LPDIRENTRY lpStackTop;
static LPDIRENTRY lpStackBottom;


static INT
PushDirectory (LPTSTR pszPath)
{
	LPDIRENTRY lpDir = cmd_alloc(FIELD_OFFSET(DIRENTRY, szPath[_tcslen(pszPath) + 1]));
	if (!lpDir)
	{
		error_out_of_memory ();
		return -1;
	}

	lpDir->prev = NULL;
	lpDir->next = lpStackTop;
	if (lpStackTop == NULL)
		lpStackBottom = lpDir;
	else
		lpStackTop->prev = lpDir;
	lpStackTop = lpDir;

	_tcscpy(lpDir->szPath, pszPath);

	nStackDepth++;

	return nErrorLevel = 0;
}


static VOID
PopDirectory (VOID)
{
	LPDIRENTRY lpDir = lpStackTop;
	lpStackTop = lpDir->next;
	if (lpStackTop != NULL)
		lpStackTop->prev = NULL;
	else
		lpStackBottom = NULL;

	cmd_free (lpDir);

	nStackDepth--;
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
INT CommandPushd (LPTSTR rest)
{
	TCHAR curPath[MAX_PATH];

	if (!_tcsncmp (rest, _T("/?"), 2))
	{
		ConOutResPuts(STRING_DIRSTACK_HELP1);
		return 0;
	}

	GetCurrentDirectory (MAX_PATH, curPath);

	if (rest[0] != _T('\0'))
	{
		if (!SetRootPath(NULL, rest))
			return 1;
	}

	return PushDirectory(curPath);
}


/*
 * popd command
 */
INT CommandPopd (LPTSTR rest)
{
	INT ret = 0;
	if (!_tcsncmp(rest, _T("/?"), 2))
	{
		ConOutResPuts(STRING_DIRSTACK_HELP2);
		return 0;
	}

	if (nStackDepth == 0)
		return 1;

	ret = _tchdir(lpStackTop->szPath) != 0;
	PopDirectory ();

	return ret;
}


/*
 * dirs command
 */
INT CommandDirs (LPTSTR rest)
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
		ConOutPuts(lpDir->szPath);
		lpDir = lpDir->prev;
	}

	return 0;
}

#endif /* FEATURE_DIRECTORY_STACK */

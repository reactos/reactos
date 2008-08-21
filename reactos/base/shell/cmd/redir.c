/*
 *  REDIR.C - redirection handling.
 *
 *
 *  History:
 *
 *    12/15/95 (Tim Norman)
 *        started.
 *
 *    12 Jul 98 (Hans B Pufal)
 *        Rewrote to make more efficient and to conform to new command.c
 *        and batch.c processing.
 *
 *    27-Jul-1998 (John P Price <linux-guru@gcfl.net>)
 *        Added config.h include
 *
 *    22-Jan-1999 (Eric Kohl)
 *        Unicode safe!
 *        Added new error redirection "2>" and "2>>".
 *
 *    26-Jan-1999 (Eric Kohl)
 *        Added new error AND output redirection "&>" and "&>>".
 *
 *    24-Jun-2005 (Brandon Turner <turnerb7@msu.edu>)
 *        simple check to fix > and | bug with 'rem'
 */

#include <precomp.h>

#ifdef FEATURE_REDIRECTION


static BOOL
IsRedirection (TCHAR c)
{
	return (c == _T('<')) || (c == _T('>')) || (c == _T('|'));
}


/*
 * Gets the redirection info from the command line and copies the
 * file names into ifn, ofn and efn removing them from the command
 * line.
 *
 * Converts remaining command line into a series of null terminated
 * strings defined by the pipe char '|'. Each string corresponds
 * to a single executable command. A double null terminates the
 * command strings.
 *
 * Return number of command strings found.
 *
 */

INT GetRedirection (LPTSTR s, REDIRECTION **RedirList)
{
	INT num = 1;
	LPTSTR dp = s;
	LPTSTR sp = s;
	TCHAR Filename[MAX_PATH];

#ifdef INCLUDE_CMD_REM

	TCHAR * line = s;


	while (_istspace (*line))
			line++;

	/*first thing first.  check to see if this is "rem" and hope out*/
	if(!_tcsncmp (line, _T("rem "), 4))
	{
		return 1;
	}
#endif
	/* find and remove all the redirections first */
	while (*sp)
	{
		if (*sp == _T('^'))
		{
			*dp++ = *sp++;
			*dp++ = *sp++;
			continue;
		}
		if ((*sp == _T('"')) || (*sp == _T('\'')))
		{
			/* No redirects inside quotes */
			TCHAR qc = *sp;

			do
				*dp++ = *sp++;
			while (*sp && *sp != qc);

			*dp++ = *sp++;
			continue;
		}

		int NumberGiven = (*sp >= _T('0') && *sp <= _T('9')) ? 1 : 0;
		if (sp[NumberGiven] == _T('<') || sp[NumberGiven] == _T('>'))
		{
			BYTE HandleNumber;
			BYTE Type;
			BOOL bInQuote = FALSE;
			TCHAR *fn = Filename;
			REDIRECTION *Redir, **ListPtr;

			if (NumberGiven)
				HandleNumber = *sp++ - _T('0');
			else
				HandleNumber = *sp == _T('<') ? 0 : 1;

			if (*sp == _T('<'))
			{
				/* input redirection */
				Type = REDIR_READ;
				sp++;
			}
			else
			{
				/* output redirection */
				Type = REDIR_WRITE;
				sp++;

				/* append request ? */
				if (*sp == _T('>'))
				{
					Type = REDIR_APPEND;
					sp++;
				}
			}

			while (_istspace(*sp))
				sp++;

			/* copy file name */
			while (*sp && (bInQuote || (!IsRedirection (*sp) && !_istspace (*sp))))
			{
				bInQuote ^= (*sp == _T('"'));
				*fn++ = *sp++;
			}
			*fn++ = _T('\0');

			/* Delete any existing redirection for the same handle number */
			ListPtr = RedirList;
			while ((Redir = *ListPtr))
			{
				if (Redir->Number == HandleNumber)
				{
					*ListPtr = Redir->Next;
					cmd_free(Redir);
					continue;
				}
				ListPtr = &Redir->Next;
			}

			Redir = cmd_alloc(FIELD_OFFSET(REDIRECTION, Filename[fn - Filename]));
			Redir->Next = NULL;
			Redir->OldHandle = INVALID_HANDLE_VALUE;
			Redir->Number = HandleNumber;
			Redir->Type = Type;
			_tcscpy(Redir->Filename, Filename);
			*ListPtr = Redir;
		}
		else
			*dp++ = *sp++;
	}

	*dp++ = _T('\0');
	*dp = _T('\0');

	/* now go for the pipes */
	sp = s;
	while (*sp)
	{
		if (*sp == _T('^'))
		{
			sp++;
			sp++;
			continue;
		}
		else if ((*sp == _T('"')) || (*sp == _T('\'')))
		{
			TCHAR qc = *sp;

			do
				sp++;
			while (*sp && *sp != qc);

			sp++;
		}
		else if (*sp == _T('|'))
		{
			*sp++ = _T('\0');
			num++;
		}
		else
			sp++;
	}

	return num;
}

/* cmd allows redirection of handles numbered 3-9 even though these don't
 * correspond to any STD_ constant. */
static HANDLE ExtraHandles[10 - 3];

static HANDLE GetHandle(UINT Number)
{
	if (Number < 3)
		return GetStdHandle(STD_INPUT_HANDLE - Number);
	else
		return ExtraHandles[Number - 3];
}

static VOID SetHandle(UINT Number, HANDLE Handle)
{
	if (Number < 3)
		SetStdHandle(STD_INPUT_HANDLE - Number, Handle);
	else
		ExtraHandles[Number - 3] = Handle;
}

BOOL
PerformRedirection(REDIRECTION *RedirList)
{
	REDIRECTION *Redir;
	TCHAR Filename[MAX_PATH];
	HANDLE hNew;
	UINT DupNumber;
	static SECURITY_ATTRIBUTES SecAttr = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };

	/* Some parameters used for read, write, and append, respectively */
	static const DWORD dwAccess[] = {
		GENERIC_READ, 
		GENERIC_WRITE,
		GENERIC_WRITE
	};
	static const DWORD dwShareMode[] = {
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		FILE_SHARE_READ,
		FILE_SHARE_READ
	};
	static const DWORD dwCreationDisposition[] = {
		OPEN_EXISTING,
		CREATE_ALWAYS,
		OPEN_ALWAYS
	};

	for (Redir = RedirList; Redir; Redir = Redir->Next)
	{
		*Filename = _T('\0');
		_tcsncat(Filename, Redir->Filename, MAX_PATH - 1);
		StripQuotes(Filename);

		if (*Filename == _T('&'))
		{
			DupNumber = Filename[1] - _T('0');
			if (DupNumber >= 10 ||
			    !DuplicateHandle(GetCurrentProcess(),
			                     GetHandle(DupNumber),
			                     GetCurrentProcess(),
			                     &hNew,
			                     0,
			                     TRUE,
			                     DUPLICATE_SAME_ACCESS))
			{
				hNew = INVALID_HANDLE_VALUE;
			}
		}
		else
		{
			hNew = CreateFile(Filename,
			                  dwAccess[Redir->Type],
			                  dwShareMode[Redir->Type],
			                  &SecAttr,
			                  dwCreationDisposition[Redir->Type],
			                  0,
			                  NULL);
		}

		if (hNew == INVALID_HANDLE_VALUE)
		{
			ConErrResPrintf(Redir->Type == REDIR_READ ? STRING_CMD_ERROR1 : STRING_CMD_ERROR3,
			                Filename);
			/* Undo all the redirections before this one */
			UndoRedirection(RedirList, Redir);
			return FALSE;
		}

		if (Redir->Type == REDIR_APPEND)
			SetFilePointer(hNew, 0, NULL, FILE_END);
		Redir->OldHandle = GetHandle(Redir->Number);
		SetHandle(Redir->Number, hNew);

		TRACE("%d redirected to: %s\n", Redir->Number, debugstr_aw(Filename));
	}
	return TRUE;
}

VOID
UndoRedirection(REDIRECTION *Redir, REDIRECTION *End)
{
	for (; Redir != End; Redir = Redir->Next)
	{
		CloseHandle(GetHandle(Redir->Number));
		SetHandle(Redir->Number, Redir->OldHandle);
		Redir->OldHandle = INVALID_HANDLE_VALUE;
	}
}

VOID
FreeRedirection(REDIRECTION *Redir)
{
	REDIRECTION *Next;
	for (; Redir; Redir = Next)
	{
		Next = Redir->Next;
		ASSERT(Redir->OldHandle == INVALID_HANDLE_VALUE);
		cmd_free(Redir);
	}
}

#endif /* FEATURE_REDIRECTION */

/*
 *  COPY.C -- copy internal command.
 *
 *
 *  History:
 *
 *    01-Aug-98 (Rob Lake z63rrl@morgan.ucs.mun.ca)
 *        started
 *
 *    13-Aug-1998 (John P. Price)
 *        fixed memory leak problem in copy function.
 *        fixed copy function so it would work with wildcards in the source
 *
 *    13-Dec-1998 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Added COPY command to CMD.
 *
 *    26-Jan-1998 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Replaced CRT io functions by Win32 io functions.
 */

#include "config.h"

#ifdef INCLUDE_CMD_COPY

#include <windows.h>
#include <tchar.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "cmd.h"


#define VERIFY  1               /* VERIFY Switch */
#define BINARY  2               /* File is to be copied as BINARY */
#define ASCII   4               /* File is to be copied as ASCII */
#define PROMPT  8               /* Prompt before overwriting files */
#define NPROMPT 16              /* Do not prompt before overwriting files */
#define HELP    32              /* Help was asked for */
#define SOURCE  128             /* File is a source */


typedef struct tagFILES
{
	struct tagFILES *next;
	TCHAR szFile[MAX_PATH];
	DWORD dwFlag;			/* BINARY -xor- ASCII */
} FILES, *LPFILES;


static BOOL DoSwitches (LPTSTR, LPDWORD);
static BOOL AddFile (LPFILES, char *, int *, int *, LPDWORD);
static BOOL AddFiles (LPFILES, char *, int *, int *, int *, LPDWORD);
static BOOL GetDestination (LPFILES, LPFILES);
static INT  ParseCommand (LPFILES, int, char **, LPDWORD);
static VOID DeleteFileList (LPFILES);
static INT  Overwrite (LPTSTR);



static BOOL
IsDirectory (LPTSTR fn)
{
	if (!IsValidFileName (fn))
		return FALSE;
	return (GetFileAttributes (fn) & FILE_ATTRIBUTE_DIRECTORY);
}


static BOOL
DoSwitches (LPTSTR arg, LPDWORD lpdwFlags)
{
	if (!_tcsicmp (arg, _T("/-Y")))
	{
		*lpdwFlags |= PROMPT;
		*lpdwFlags &= ~NPROMPT;
		return TRUE;
	}
	else if (_tcslen (arg) > 2)
	{
		error_too_many_parameters ("");
		return FALSE;
	}

	switch (_totupper (arg[1]))
	{
		case _T('V'):
			*lpdwFlags |= VERIFY;
			break;

		case _T('A'):
			*lpdwFlags |= ASCII;
			*lpdwFlags &= ~BINARY;
			break;

		case _T('B'):
			*lpdwFlags |= BINARY;
			*lpdwFlags &= ~ASCII;
			break;

		case _T('Y'):
			*lpdwFlags &= ~PROMPT;
			*lpdwFlags |= NPROMPT;
			break;

		default:
			error_invalid_switch (arg[1]);
			return FALSE;
	}
	return TRUE;
}


static BOOL
AddFile (LPFILES f, char *arg, int *source, int *dest, LPDWORD flags)
{
	if (*dest)
	{
		error_too_many_parameters ("");
		return FALSE;
	}
	if (*source)
	{
		*dest = 1;
		f->dwFlag = 0;
	}
	else
	{
		*source = 1;
		f->dwFlag = SOURCE;
	}
	_tcscpy(f->szFile, arg);
	f->dwFlag |= *flags & ASCII ? ASCII : BINARY;
	if ((f->next = (LPFILES)malloc (sizeof (FILES))) == NULL)
	{
		error_out_of_memory ();
		return FALSE;
	}
	f = f->next;
	f->dwFlag = 0;
	f->next = NULL;
	return TRUE;
}


static BOOL
AddFiles (LPFILES f, char *arg, int *source, int *dest,
		  int *count, LPDWORD flags)
{
	char t[128];
	int j;
	int k;

	if (*dest)
	{
		error_too_many_parameters ("");
		return FALSE;
	}

	j = 0;
	k = 0;

	while (arg[j] == _T('+'))
		j++;

	while (arg[j] != _T('\0'))
	{
		t[k] = arg[j++];
		if (t[k] == '+' || arg[j] == _T('\0'))
		{
			if (!k)
				continue;
			if (arg[j] == _T('\0') && t[k] != _T('+'))
				k++;
			t[k] = _T('\0');
			*count += 1;
			_tcscpy (f->szFile, t);
			*source = 1;
			if (*flags & ASCII)
				f->dwFlag |= *flags | SOURCE | ASCII;
			else
				f->dwFlag |= *flags | BINARY | SOURCE;

			if ((f->next = (LPFILES)malloc (sizeof (FILES))) == NULL)
			{
				error_out_of_memory ();
				return FALSE;
			}
			f = f->next;
			f->next = NULL;
			k = 0;
			f->dwFlag = 0;
			continue;
		}
		k++;
	}

	if (arg[--j] == _T('+'))
		*source = 0;

	return 1;
}


static BOOL
GetDestination (LPFILES f, LPFILES dest)
{
	LPFILES p;
	LPFILES start = f;

	while (f->next != NULL)
	{
		p = f;
		f = f->next;
	}

	f = p;

	if ((f->dwFlag & SOURCE) == 0)
	{
		free (p->next);
		p->next = NULL;
		_tcscpy (dest->szFile, f->szFile);
		dest->dwFlag = f->dwFlag;
		dest->next = NULL;
		f = start;
		return TRUE;
	}

	return FALSE;
}


static INT
ParseCommand (LPFILES f, int argc, char **arg, LPDWORD lpdwFlags)
{
	INT i;
	INT dest;
	INT source;
	INT count;

	dest = 0;
	source = 0;
	count = 0;

	for (i = 0; i < argc; i++)
	{
		if (arg[i][0] == _T('/'))
		{
			if (!DoSwitches (arg[i], lpdwFlags))
				return -1;
		}
		else
		{
			if (!_tcscmp(arg[i], _T("+")))
				source = 0;
			else if (!_tcschr(arg[i], _T('+')) && source)
			{
				if (!AddFile(f, arg[i], &source, &dest, lpdwFlags))
					return -1;
				f = f->next;
				count++;
			}
			else
			{
				if (!AddFiles(f, arg[i], &source, &dest, &count, lpdwFlags))
					return -1;
				while (f->next != NULL)
					f = f->next;
			}
		}
	}
#ifdef _DEBUG
	DebugPrintf ("ParseCommand: flags has %s\n",
				 *lpdwFlags & ASCII ? "ASCII" : "BINARY");
#endif
	return count;
}


static VOID DeleteFileList (LPFILES f)
{
	LPFILES temp;

	while (f != NULL)
	{
		temp = f;
		f = f->next;
		free (temp);
	}
}


static INT
Overwrite (LPTSTR fn)
{
	TCHAR inp[10];
	LPTSTR p;

	ConOutPrintf (_T("Overwrite %s (Yes/No/All)? "), fn);
	ConInString (inp, 10);

	_tcsupr (inp);
	for (p = inp; _istspace (*p); p++)
		;

	if (*p != _T('Y') && *p != _T('A'))
		return 0;
	if (*p == _T('A'))
		return 2;

	return 1;
}


#define BUFF_SIZE 16384         /* 16k = max buffer size */


int copy (LPTSTR source, LPTSTR dest, int append, LPDWORD lpdwFlags)
{
	FILETIME srctime;
	HANDLE hFileSrc;
	HANDLE hFileDest;
	LPBYTE buffer;
	DWORD  dwAttrib;
	DWORD  dwRead;
	DWORD  dwWritten;
	DWORD  i;
	BOOL   bEof = FALSE;

#ifdef _DEBUG
	DebugPrintf ("checking mode\n");
#endif

	dwAttrib = GetFileAttributes (source);

	hFileSrc = CreateFile (source, GENERIC_READ, FILE_SHARE_READ,
						   NULL, OPEN_EXISTING, 0, NULL);
	if (hFileSrc == INVALID_HANDLE_VALUE)
	{
		ConErrPrintf (_T("Error: Cannot open source - %s!\n"), source);
		return 0;
	}

#ifdef _DEBUG
	DebugPrintf (_T("getting time\n"));
#endif

	GetFileTime (hFileSrc, &srctime, NULL, NULL);

#ifdef _DEBUG
	DebugPrintf (_T("copy: flags has %s\n"),
				 *lpdwFlags & ASCII ? "ASCII" : "BINARY");
#endif

	if (!IsValidFileName (dest))
	{
#ifdef _DEBUG
		DebugPrintf (_T("opening/creating\n"));
#endif
		hFileDest =
			CreateFile (dest, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
	}
	else if (!append)
	{
		if (!_tcscmp (dest, source))
		{
			ConErrPrintf (_T("Error: Can't copy onto itself!\n"));
			CloseHandle (hFileSrc);
			return 0;
		}

#ifdef _DEBUG
		DebugPrintf (_T("SetFileAttributes (%s, FILE_ATTRIBUTE_NORMAL);\n"), dest);
#endif
		SetFileAttributes (dest, FILE_ATTRIBUTE_NORMAL);

#ifdef _DEBUG
		DebugPrintf (_T("DeleteFile (%s);\n"), dest);
#endif
		DeleteFile (dest);

		hFileDest =
			CreateFile (dest, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
	}
	else
	{
		LONG lFilePosHigh = 0;

		if (!_tcscmp (dest, source))
		{
			CloseHandle (hFileSrc);
			return 0;
		}

#ifdef _DEBUG
		DebugPrintf (_T("opening/appending\n"));
#endif
		hFileDest =
			CreateFile (dest, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
		SetFilePointer (hFileDest, 0, &lFilePosHigh,FILE_END);
	}

	if (hFileDest == INVALID_HANDLE_VALUE)
	{
		CloseHandle (hFileSrc);
		error_path_not_found ();
		return 0;
	}

	buffer = (LPBYTE)malloc (BUFF_SIZE);
	if (buffer == NULL)
	{
		CloseHandle (hFileDest);
		CloseHandle (hFileSrc);
		error_out_of_memory ();
		return 0;
	}

	ConOutPuts (source);

	do
	{
		ReadFile (hFileSrc, buffer, BUFF_SIZE, &dwRead, NULL);
		if (*lpdwFlags & ASCII)
		{
			for (i = 0; i < dwRead; i++)
			{
				if (((LPTSTR)buffer)[i] == 0x1A)
				{
					bEof = TRUE;
					break;
				}
			}
			dwRead = i;
		}

		if (dwRead == 0)
			break;

		WriteFile (hFileDest, buffer, dwRead, &dwWritten, NULL);
		if (dwWritten != dwRead)
		{
			ConErrPrintf (_T("Error writing destination!\n"));
			free (buffer);
			CloseHandle (hFileDest);
			CloseHandle (hFileSrc);
			return 0;
		}
	}
	while (dwRead && !bEof);

#ifdef _DEBUG
	DebugPrintf (_T("setting time\n"));
#endif
	SetFileTime (hFileDest, &srctime, NULL, NULL);

	if (*lpdwFlags & ASCII)
	{
		((LPTSTR)buffer)[0] = 0x1A;
		((LPTSTR)buffer)[1] = _T('\0');
#ifdef _DEBUG
		DebugPrintf (_T("appending ^Z\n"));
#endif
		WriteFile (hFileDest, buffer, sizeof(TCHAR), &dwWritten, NULL);
	}

	free (buffer);
	CloseHandle (hFileDest);
	CloseHandle (hFileSrc);

#ifdef _DEBUG
	DebugPrintf (_T("setting mode\n"));
#endif
	SetFileAttributes (dest, dwAttrib);

	return 1;
}


int setup_copy (LPFILES sources, char **p, BOOL bMultiple,
               char *drive_d, char *dir_d, char *file_d,
               char *ext_d, int *append, LPDWORD lpdwFlags)
{
	WIN32_FIND_DATA find;

  char drive_s[_MAX_DRIVE],
    dir_s[_MAX_DIR],
    file_s[_MAX_FNAME],
    ext_s[_MAX_EXT];
  char from_merge[_MAX_PATH];

	LPTSTR real_source;
	LPTSTR real_dest;

	INT  nCopied = 0;
	BOOL bAll = FALSE;
	BOOL bDone;
	HANDLE hFind;


	real_source = (LPTSTR)malloc (MAX_PATH);
	real_dest = (LPTSTR)malloc (MAX_PATH);

	if (!real_source || !real_dest)
	{
		error_out_of_memory ();
		DeleteFileList (sources);
		free (real_source);
		free (real_dest);
		freep (p);
		return 0;
	}

	while (sources->next != NULL)
	{
		_splitpath (sources->szFile, drive_s, dir_s, file_s, ext_s);
		hFind = FindFirstFile (sources->szFile, &find);
		if (hFind == INVALID_HANDLE_VALUE)
		{
			error_file_not_found();
			DeleteFileList (sources);
			freep(p);
			free(real_source);
			free(real_dest);
			return 0;
		}

		do
		{
			if (find.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				goto next;

			_makepath(from_merge, drive_d, dir_d, file_d, ext_d);
			if (from_merge[_tcslen(from_merge) - 1] == _T('\\'))
				from_merge[_tcslen(from_merge) - 1] = 0;

			if (IsDirectory (from_merge))
			{
				bMultiple = FALSE;
				_tcscat (from_merge, _T("\\"));
				_tcscat (from_merge, find.cFileName);
			}
			else
				bMultiple = TRUE;

			_tcscpy (real_dest, from_merge);
			_makepath (real_source, drive_s, dir_s, find.cFileName, NULL);

#ifdef _DEBUG
			DebugPrintf (_T("copying %s -> %s (%sappending%s)\n"),
						 real_source, real_dest,
						 *append ? "" : "not ",
						 sources->dwFlag & ASCII ? ", ASCII" : ", BINARY");
#endif

			if (IsValidFileName (real_dest) && !bAll)
			{
				int over = Overwrite (real_dest);
				if (over == 2)
					bAll = TRUE;
				else if (over == 0)
					goto next;
				else if (bMultiple)
					bAll = TRUE;
			}
			if (copy (real_source, real_dest, *append, lpdwFlags))
				nCopied++;
		next:
			bDone = FindNextFile (hFind, &find);

			if (bMultiple)
				*append = 1;
		}
		while (bDone);

		FindClose (hFind);
		sources = sources->next;

	}
	free (real_source);
	free (real_dest);

	return nCopied;
}


INT cmd_copy (LPTSTR first, LPTSTR rest)
{
  char **p;
  char drive_d[_MAX_DRIVE],
    dir_d[_MAX_DIR],
    file_d[_MAX_FNAME],
    ext_d[_MAX_EXT];

  int argc,
    append,
    files,
    copied;

	LPFILES sources;
	FILES dest;
	LPFILES start;
	BOOL bMultiple;
	BOOL bWildcards;
	BOOL bDestFound;
	DWORD dwFlags = 0;

	if (!_tcsncmp (rest, _T("/?"), 2))
	{
		ConOutPuts (_T("Copies one or more files to another location.\n"
					   "\n"
					   "COPY [/V][/Y|/-Y][/A|/B] source [/A|/B]\n"
					   "     [+ source [/A|/B] [+ ...]] [destination [/A|/B]]\n"
					   "\n"
					   "  source       Specifies the file or files to be copied.\n"
					   "  /A           Indicates an ASCII text file.\n"
					   "  /B           Indicates a binary file.\n"
					   "  destination  Specifies the directory and/or filename for the new file(s).\n"
					   "  /V           Verifies that new files are written correctly.\n"
					   "  /Y           Suppresses prompting to confirm you want to overwrite an\n"
					   "               existing destination file.\n"
					   "  /-Y          Causes prompting to confirm you want to overwrite an\n"
					   "               existing destination file.\n"
					   "\n"
					   "The switch /Y may be present in the COPYCMD environment variable.\n"
					   "..."));
		return 1;
	}

	p = split (rest, &argc);

	if (argc == 0)
	{
		error_req_param_missing ();
		return 0;
	}

	sources = (LPFILES)malloc (sizeof (FILES));
	if (!sources)
	{
		error_out_of_memory ();
		return 0;
	}
	sources->next = NULL;
	sources->dwFlag = 0;

	if ((files = ParseCommand (sources, argc, p, &dwFlags)) == -1)
	{
		DeleteFileList (sources);
		freep (p);
		return 0;
	}
	else if (files == 0)
	{
		error_req_param_missing();
		DeleteFileList (sources);
		freep (p);
		return 0;
	}
	start = sources;

	bDestFound = GetDestination (sources, &dest);
	if (bDestFound)
	{
		_splitpath (dest.szFile, drive_d, dir_d, file_d, ext_d);
		if (IsDirectory (dest.szFile))
		{
			_tcscat (dir_d, file_d);
			_tcscat (dir_d, ext_d);
			file_d[0] = _T('\0');
			ext_d[0] = _T('\0');
		}
	}

	if (_tcschr (dest.szFile, _T('*')) || _tcschr (dest.szFile, _T('?')))
		bWildcards = TRUE;
	else
		bWildcards = FALSE;

	if (strchr(rest, '+'))
		bMultiple = TRUE;
	else
		bMultiple = FALSE;

	append = 0;
	copied = 0;

	if (bDestFound && !bWildcards)
	{
		copied = setup_copy (sources, p, bMultiple, drive_d, dir_d, file_d, ext_d, &append, &dwFlags);
	}
	else if (bDestFound && bWildcards)
	{
		ConErrPrintf (_T("Error: Not implemented yet!\n"));
		DeleteFileList (sources);
		freep (p);
		return 0;
	}
	else if (!bDestFound && !bMultiple)
	{
		_splitpath (sources->szFile, drive_d, dir_d, file_d, ext_d);
		if (IsDirectory (sources->szFile))
		{
			_tcscat (dir_d, file_d);
			_tcscat (dir_d, ext_d);
			file_d[0] = _T('\0');
			ext_d[0] = _T('\0');
		}
		copied = setup_copy (sources, p, FALSE, "", "", file_d, ext_d, &append, &dwFlags);
	}
	else
	{
		_splitpath(sources->szFile, drive_d, dir_d, file_d, ext_d);
		if (IsDirectory (sources->szFile))
		{
			_tcscat (dir_d, file_d);
			_tcscat (dir_d, ext_d);
			file_d[0] = _T('\0');
			ext_d[0] = _T('\0');
		}

		ConOutPuts (sources->szFile);
		append = 1;
		copied = setup_copy (sources->next, p, bMultiple, drive_d, dir_d, file_d, ext_d, &append, &dwFlags) + 1;
	}

	DeleteFileList (start);
	freep (p);
	ConOutPrintf (_T("        %d file(s) copied\n"), copied);

	return 1;
}
#endif /* INCLUDE_CMD_COPY */

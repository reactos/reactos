/*
 *  FILECOMP.C - handles filename completion.
 *
 *
 *  Comments:
 *
 *    30-Jul-1998 (John P Price <linux-guru@gcfl.net>)
 *       moved from command.c file
 *       made second TAB display list of filename matches
 *       made filename be lower case if last character typed is lower case
 *
 *    25-Jan-1999 (Eric Kohl)
 *       Cleanup. Unicode safe!
 *
 *    30-Apr-2004 (Filip Navara <xnavara@volny.cz>)
 *       Make the file listing readable when there is a lot of long names.
 *

 *    05-Jul-2004 (Jens Collin <jens.collin@lakhei.com>)
 *       Now expands lfn even when trailing " is omitted.
 */

#include <precomp.h>

#ifdef FEATURE_UNIX_FILENAME_COMPLETION

VOID CompleteFilename (LPTSTR str, UINT charcount)
{
	WIN32_FIND_DATA file;
	HANDLE hFile;
	INT   curplace = 0;
	INT   start;
	INT   count;
	INT step;
	INT c = 0;
	BOOL  found_dot = FALSE;
	BOOL  perfectmatch = TRUE;
	TCHAR path[MAX_PATH];
	TCHAR fname[MAX_PATH];
	TCHAR maxmatch[MAX_PATH] = _T("");
	TCHAR directory[MAX_PATH];
	LPCOMMAND cmds_ptr;

	/* expand current file name */
        count = charcount - 1;
	if (count < 0)
		count = 0;

	/* find how many '"'s there is typed already.*/
	step = count;
	while (step > 0)
	{
		if (str[step] == _T('"'))
			c++;
		step--;
	}
	/* if c is odd, then user typed " before name, else not.*/

	/* find front of word */
	if (str[count] == _T('"') || (c % 2))
	{
		count--;
		while (count > 0 && str[count] != _T('"'))
			count--;
	}
	else
	{
		while (count > 0 && str[count] != _T(' '))
			count--;
	}

	/* if not at beginning, go forward 1 */
	if (str[count] == _T(' '))
		count++;

	start = count;

	if (str[count] == _T('"'))
		count++;	/* don't increment start */

	/* extract directory from word */
	_tcscpy (directory, &str[count]);
	curplace = _tcslen (directory) - 1;

	if (curplace >= 0 && directory[curplace] == _T('"'))
		directory[curplace--] = _T('\0');

	_tcscpy (path, directory);

	while (curplace >= 0 && directory[curplace] != _T('\\') &&
                   directory[curplace] != _T('/') &&
		   directory[curplace] != _T(':'))
	{
		directory[curplace] = 0;
		curplace--;
	}

	/* look for a '.' in the filename */
	for (count = _tcslen (directory); path[count] != _T('\0'); count++)
	{
		if (path[count] == _T('.'))
		{
			found_dot = TRUE;
			break;
		}
	}

	if (found_dot)
		_tcscat (path, _T("*"));
	else
		_tcscat (path, _T("*.*"));

	/* current fname */
	curplace = 0;

	hFile = FindFirstFile (path, &file);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		/* find anything */
		do
		{
			/* ignore "." and ".." */
			if (!_tcscmp (file.cFileName, _T(".")) ||
				!_tcscmp (file.cFileName, _T("..")))
				continue;

			_tcscpy (fname, file.cFileName);

			if (file.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				_tcscat (fname, _T("\\"));

			if (!maxmatch[0] && perfectmatch)
			{
				_tcscpy(maxmatch, fname);
			}
			else
			{
				for (count = 0; maxmatch[count] && fname[count]; count++)
				{
					if (tolower(maxmatch[count]) != tolower(fname[count]))
					{
						perfectmatch = FALSE;
						maxmatch[count] = 0;
						break;
					}
				}

				if (maxmatch[count] == _T('\0') &&
				    fname[count] != _T('\0'))
					perfectmatch = FALSE;
			}
		}
		while (FindNextFile (hFile, &file));

		FindClose (hFile);

		/* only quote if the filename contains spaces */
		if (_tcschr(directory, _T(' ')) ||
		    _tcschr(maxmatch, _T(' ')))
		{
			str[start] = _T('\"');
			_tcscpy (&str[start+1], directory);
			_tcscat (&str[start], maxmatch);
			_tcscat (&str[start], _T("\"") );
		}
		else
		{
			_tcscpy (&str[start], directory);
			_tcscat (&str[start], maxmatch);
		}

		if(!perfectmatch)
		{
			MessageBeep (-1);
		}
	}
	else
	{
		/* no match found - search for internal command */
		for (cmds_ptr = cmds; cmds_ptr->name; cmds_ptr++)
		{
			if (!_tcsnicmp (&str[start], cmds_ptr->name,
				_tcslen (&str[start])))
			{
				/* return the mach only if it is unique */
				if (_tcsnicmp (&str[start], (cmds_ptr+1)->name, _tcslen (&str[start])))
					_tcscpy (&str[start], cmds_ptr->name);
				break;
			}
		}

		MessageBeep (-1);
	}
}


/*
 * returns 1 if at least one match, else returns 0
 */

BOOL ShowCompletionMatches (LPTSTR str, INT charcount)
{
	WIN32_FIND_DATA file;
	HANDLE hFile;
	BOOL  found_dot = FALSE;
	INT   curplace = 0;
	INT   start;
	INT   count;
	TCHAR path[MAX_PATH];
	TCHAR fname[MAX_PATH];
	TCHAR directory[MAX_PATH];
	UINT   longestfname = 0;
	SHORT screenwidth;

	/* expand current file name */
	count = charcount - 1;
	if (count < 0)
		count = 0;

	/* find front of word */
	if (str[count] == _T('"'))
	{
		count--;
		while (count > 0 && str[count] != _T('"'))
			count--;
	}
	else
	{
		while (count > 0 && str[count] != _T(' '))
			count--;
	}

	/* if not at beginning, go forward 1 */
	if (str[count] == _T(' '))
		count++;

	start = count;

	if (str[count] == _T('"'))
		count++;	/* don't increment start */

	/* extract directory from word */
	_tcscpy (directory, &str[count]);
	curplace = _tcslen (directory) - 1;

	if (curplace >= 0 && directory[curplace] == _T('"'))
		directory[curplace--] = _T('\0');

	_tcscpy (path, directory);

	while (curplace >= 0 &&
		   directory[curplace] != _T('\\') &&
		   directory[curplace] != _T(':'))
	{
		directory[curplace] = 0;
		curplace--;
	}

	/* look for a . in the filename */
	for (count = _tcslen (directory); path[count] != _T('\0'); count++)
	{
		if (path[count] == _T('.'))
		{
			found_dot = TRUE;
			break;
		}
	}

	if (found_dot)
		_tcscat (path, _T("*"));
	else
		_tcscat (path, _T("*.*"));

	/* current fname */
	curplace = 0;

	hFile = FindFirstFile (path, &file);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		/* Get the size of longest filename first. */
		do
		{
			if (_tcslen(file.cFileName) > longestfname)
			{
				longestfname = _tcslen(file.cFileName);
				/* Directories get extra brackets around them. */
				if (file.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
					longestfname += 2;
			}
		}
		while (FindNextFile (hFile, &file));
		FindClose (hFile);

		hFile = FindFirstFile (path, &file);

		/* Count the highest number of columns */
		GetScreenSize(&screenwidth, 0);

		/* For counting columns of output */
		count = 0;

		/* Increase by the number of spaces behind file name */
		longestfname += 3;

		/* find anything */
		ConOutChar (_T('\n'));
		do
		{
			/* ignore . and .. */
			if (!_tcscmp (file.cFileName, _T(".")) ||
				!_tcscmp (file.cFileName, _T("..")))
				continue;

			if (file.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				_stprintf (fname, _T("[%s]"), file.cFileName);
			else
				_tcscpy (fname, file.cFileName);

			ConOutPrintf (_T("%*s"), - longestfname, fname);
			count++;
			/* output as much columns as fits on the screen */
			if (count >= (screenwidth / longestfname))
			{
				/* print the new line only if we aren't on the
				 * last column, in this case it wraps anyway */
				if (count * longestfname != (UINT)screenwidth)
					ConOutPrintf (_T("\n"));
				count = 0;
			}
		}
		while (FindNextFile (hFile, &file));

		FindClose (hFile);

		if (count)
			ConOutChar (_T('\n'));
	}
	else
	{
		/* no match found */
		MessageBeep (-1);
		return FALSE;
	}

	return TRUE;
}
#endif

#ifdef FEATURE_4NT_FILENAME_COMPLETION

typedef struct _FileName
{
	TCHAR Name[MAX_PATH];
} FileName;

VOID FindPrefixAndSuffix(LPTSTR strIN, LPTSTR szPrefix, LPTSTR szSuffix)
{
	/* String that is to be examined */
	TCHAR str[MAX_PATH];
	/* temp pointers to used to find needed parts */
	TCHAR * szSearch;	
	TCHAR * szSearch1;
	TCHAR * szSearch2;
	TCHAR * szSearch3;
	/* number of quotes in the string */
	INT nQuotes = 0;
	/* used in for loops */
	UINT i;
	/* Char number to break the string at */
	INT PBreak = 0;
	INT SBreak = 0;
	/* when phrasing a string, this tells weather
	   you are inside quotes ot not. */
	BOOL bInside = FALSE;
	
  szPrefix[0] = _T('\0');
  szSuffix[0] = _T('\0');

	/* Copy over the string to later be edited */
	_tcscpy(str,strIN);

	/* Count number of " */
	for(i = 0; i < _tcslen(str); i++)
		if(str[i] == _T('\"'))
			nQuotes++;

	/* Find the prefix and suffix */
	if(nQuotes % 2 && nQuotes >= 1)
	{
		/* Odd number of quotes.  Just start from the last " */
		/* THis is the way MS does it, and is an easy way out */
		szSearch = _tcsrchr(str, _T('\"'));
		/* Move to the next char past the " */
		szSearch++;
		_tcscpy(szSuffix,szSearch);
		/* Find the one closest to end */
		szSearch1 = _tcsrchr(str, _T('\"'));
		szSearch2 = _tcsrchr(str, _T('\\'));
		szSearch3 = _tcsrchr(str, _T('.'));
		if(szSearch2 != NULL && _tcslen(szSearch1) > _tcslen(szSearch2))
			szSearch = szSearch2;
		else if(szSearch3 != NULL && _tcslen(szSearch1) > _tcslen(szSearch3))
			szSearch = szSearch3;
		else
			szSearch = szSearch1;
		/* Move one char past */
		szSearch++;		
    szSearch[0] = _T('\0');
		_tcscpy(szPrefix,str);
		return;
	
	}

	if(!_tcschr(str, _T(' ')))
	{
		/* No spaces, everything goes to Suffix */
		_tcscpy(szSuffix,str);
		/* look for a slash just in case */
		szSearch = _tcsrchr(str, _T('\\'));		
		if(szSearch)
		{
			szSearch++;			
      szSearch[0] = _T('\0');
			_tcscpy(szPrefix,str);
		}
		else
		{
			szPrefix[0] = _T('\0');
		}
		return;
	}

	if(!nQuotes)
	{
		/* No quotes, and there is a space*/
		/* Take it after the last space */
		szSearch = _tcsrchr(str, _T(' '));
		szSearch++;
		_tcscpy(szSuffix,szSearch);
		/* Find the closest to the end space or \ */
		_tcscpy(str,strIN);
		szSearch1 = _tcsrchr(str, _T(' '));
		szSearch2 = _tcsrchr(str, _T('\\'));
		szSearch3 = _tcsrchr(str, _T('/'));
		if(szSearch2 != NULL && _tcslen(szSearch1) > _tcslen(szSearch2))
			szSearch = szSearch2;
		else if(szSearch3 != NULL && _tcslen(szSearch1) > _tcslen(szSearch3))
			szSearch = szSearch3;
		else
			szSearch = szSearch1;
		szSearch++;		
    szSearch[0] = _T('\0');
		_tcscpy(szPrefix,str);		
		return;
	}
	
	/* All else fails and there is a lot of quotes, spaces and | 
	   Then we search through and find the last space or \ that is
		not inside a quotes */
	for(i = 0; i < _tcslen(str); i++)
	{
		if(str[i] == _T('\"'))
			bInside = !bInside;
		if(str[i] == _T(' ') && !bInside)
			SBreak = i;
		if((str[i] == _T(' ') || str[i] == _T('\\')) && !bInside)
			PBreak = i;

	}
	SBreak++;
	PBreak++;
	_tcscpy(szSuffix,&strIN[SBreak]);	
  strIN[PBreak] = _T('\0');
	_tcscpy(szPrefix,strIN);
	if (szPrefix[_tcslen(szPrefix) - 2] == _T('\"') &&
        szPrefix[_tcslen(szPrefix) - 1] != _T(' '))
	{
		/* need to remove the " right before a \ at the end to
		   allow the next stuff to stay inside one set of quotes
			otherwise you would have multiple sets of quotes*/
		_tcscpy(&szPrefix[_tcslen(szPrefix) - 2],_T("\\"));

	}

}
 int __cdecl compare(const void *arg1,const void *arg2)
 {
	FileName * File1;
	FileName * File2;
	INT ret;

	File1 = cmd_alloc(sizeof(FileName));
	File2 = cmd_alloc(sizeof(FileName));
	if(!File1 || !File2)
		return 0;

	memcpy(File1,arg1,sizeof(FileName));
	memcpy(File2,arg2,sizeof(FileName));

	 /* ret = _tcsicmp(File1->Name, File2->Name); */
	 ret = lstrcmpi(File1->Name, File2->Name);

	cmd_free(File1);
	cmd_free(File2);
	return ret;
 }

VOID CompleteFilename (LPTSTR strIN, BOOL bNext, LPTSTR strOut, UINT cusor)
{
	/* Length of string before we complete it */
	INT StartLength;
	/* Length of string after completed */
	INT EndLength;
	/* The number of chars added too it */
	static INT DiffLength = 0;
	/* Used to find and assemble the string that is returned */
	TCHAR szBaseWord[MAX_PATH];
	TCHAR szPrefix[MAX_PATH];
	TCHAR szOrginal[MAX_PATH];
	TCHAR szSearchPath[MAX_PATH];
	/* Save the strings used last time, so if they hit tab again */
	static TCHAR LastReturned[MAX_PATH];
	static TCHAR LastSearch[MAX_PATH];
	static TCHAR LastPrefix[MAX_PATH];
	/* Used to search for files */
	HANDLE hFile;
	WIN32_FIND_DATA file;
	/* List of all the files */
	FileName * FileList = NULL;
	/* Number of files */
	INT FileListSize = 0;
	/* Used for loops */
	UINT i;
	/* Editable string of what was passed in */
	TCHAR str[MAX_PATH];
	/* Keeps track of what element was last selected */
	static INT Sel;
	BOOL NeededQuote = FALSE;
	BOOL ShowAll = TRUE;
	TCHAR * line = strIN; 

	strOut[0] = _T('\0');

	while (_istspace (*line))
			line++;	
	if(!_tcsnicmp (line, _T("rd "), 3) || !_tcsnicmp (line, _T("cd "), 3))
		ShowAll = FALSE;

	/* Copy the string, str can be edited and orginal should not be */
	_tcscpy(str,strIN);
	_tcscpy(szOrginal,strIN);

	/* Look to see if the cusor is not at the end of the string */
	if((cusor + 1) < _tcslen(str))
		str[cusor] = _T('\0');

	/* Look to see if they hit tab again, if so cut off the diff length */
	if(_tcscmp(str,LastReturned) || !_tcslen(str))
	{
		/* We need to know how many chars we added from the start */
		StartLength = _tcslen(str);

		/* no string, we need all files in that directory */
		if(!StartLength)
		{
			_tcscat(str,_T("*"));
		}

		/* Zero it out first */
		szBaseWord[0] = _T('\0');
		szPrefix[0] = _T('\0');

		/*What comes out of this needs to be:
			szBaseWord =  path no quotes to the object
			szPrefix = what leads up to the filename
			no quote at the END of the full name */
		FindPrefixAndSuffix(str,szPrefix,szBaseWord);
		/* Strip quotes */
		for(i = 0; i < _tcslen(szBaseWord); )
		{
			if(szBaseWord[i] == _T('\"'))
				memmove(&szBaseWord[i],&szBaseWord[i + 1], _tcslen(&szBaseWord[i]) * sizeof(TCHAR));
			else
				i++;
		}

		/* clear it out */
		memset(szSearchPath, 0, sizeof(szSearchPath));

		/* Start the search for all the files */
		GetFullPathName(szBaseWord, MAX_PATH, szSearchPath, NULL);
		if(StartLength > 0)
    {
			_tcscat(szSearchPath,_T("*"));
    }
		_tcscpy(LastSearch,szSearchPath);
		_tcscpy(LastPrefix,szPrefix);
	}
	else
	{
		_tcscpy(szSearchPath, LastSearch);
		_tcscpy(szPrefix, LastPrefix);
		StartLength = 0;
	}
	/* search for the files it might be */
	hFile = FindFirstFile (szSearchPath, &file);
 	if(hFile == INVALID_HANDLE_VALUE)
	{
		/* Assemble the orginal string and return */
		_tcscpy(strOut,szOrginal);
		return;
	}

	/* aseemble a list of all files names */
	do
	{
 		if(!_tcscmp (file.cFileName, _T(".")) ||
			!_tcscmp (file.cFileName, _T("..")))
			continue;
		
		/* Don't show files when they are doing 'cd' or 'rd' */
		if(!ShowAll &&
       file.dwFileAttributes != 0xFFFFFFFF &&
       !(file.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
		{
				continue;
		}

		/* Add the file to the list of files */
		FileList = cmd_realloc(FileList, ++FileListSize * sizeof(FileName));
 
		if(FileList == NULL) 
		{
			/* Assemble the orginal string and return */
			_tcscpy(strOut,szOrginal);
			FindClose(hFile);
			ConOutFormatMessage (GetLastError());
			return;
		}
		/* Copies the file name into the struct */
		_tcscpy(FileList[FileListSize-1].Name,file.cFileName);
 
	} while(FindNextFile(hFile,&file));

    FindClose(hFile);

	/* Check the size of the list to see if we
	   found any matches */
	if(FileListSize == 0)
	{
		_tcscpy(strOut,szOrginal);
		if(FileList != NULL) 
			cmd_free(FileList);
		return;

	}
	/* Sort the files */
	qsort(FileList,FileListSize,sizeof(FileName), compare);

	/* Find the next/previous */
	if(!_tcscmp(szOrginal,LastReturned))
	{
		if(bNext)
		{
			if(FileListSize - 1 == Sel)
				Sel = 0;
			else
				Sel++;
		}
		else
		{
			if(!Sel)
				Sel = FileListSize - 1;
			else
				Sel--;
		}
	}
	else
	{
		Sel = 0;
	}

	/* nothing found that matched last time 
	   so return the first thing in the list */
	strOut[0] = _T('\0');
	
	/* space in the name */
	if(_tcschr(FileList[Sel].Name, _T(' ')))
	{
		INT LastSpace;
		BOOL bInside;
		/* It needs a " at the end */
		NeededQuote = TRUE;
		LastSpace = -1;
		bInside = FALSE;
		/* Find the place to put the " at the start */
		for(i = 0; i < _tcslen(szPrefix); i++)
		{
			if(szPrefix[i] == _T('\"'))
				bInside = !bInside;
			if(szPrefix[i] == _T(' ') && !bInside)
				LastSpace = i;

		}
		/* insert the quoation and move things around */
		if(szPrefix[LastSpace + 1] != _T('\"') && LastSpace != -1)
		{
			memmove ( &szPrefix[LastSpace+1], &szPrefix[LastSpace], (_tcslen(szPrefix)-LastSpace+1) * sizeof(TCHAR) );
			
			if((UINT)(LastSpace + 1) == _tcslen(szPrefix))
			{
				_tcscat(szPrefix,_T("\""));
			}
				szPrefix[LastSpace + 1] = _T('\"');
		}
		else if(LastSpace == -1)
		{
			_tcscpy(szBaseWord,_T("\""));
			_tcscat(szBaseWord,szPrefix);
			_tcscpy(szPrefix,szBaseWord);

		}
	}

	_tcscpy(strOut,szPrefix);
	_tcscat(strOut,FileList[Sel].Name);

	/* check for odd number of quotes means we need to close them */
	if(!NeededQuote)
	{
		for(i = 0; i < _tcslen(strOut); i++)
			if(strOut[i] == _T('\"'))
				NeededQuote = !NeededQuote;
	}

	if(szPrefix[_tcslen(szPrefix) - 1] == _T('\"') || NeededQuote)
		_tcscat(strOut,_T("\""));

	_tcscpy(LastReturned,strOut);
	EndLength = _tcslen(strOut);
	DiffLength = EndLength - StartLength;
	if(FileList != NULL) 
		cmd_free(FileList);
	
}
#endif

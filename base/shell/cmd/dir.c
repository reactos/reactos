/*
 *  DIR.C - dir internal command.
 *
 *
 *  History:
 *
 *    01/29/97 (Tim Norman)
 *        started.
 *
 *    06/13/97 (Tim Norman)
 *      Fixed code.
 *
 *    07/12/97 (Tim Norman)
 *        Fixed bug that caused the root directory to be unlistable
 *
 *    07/12/97 (Marc Desrochers)
 *        Changed to use maxx, maxy instead of findxy()
 *
 *    06/08/98 (Rob Lake)
 *        Added compatibility for /w in dir
 *
 *    06/09/98 (Rob Lake)
 *        Compatibility for dir/s started
 *        Tested that program finds directories off root fine
 *
 *    06/10/98 (Rob Lake)
 *        do_recurse saves the cwd and also stores it in Root
 *        build_tree adds the cwd to the beginning of its' entries
 *        Program runs fine, added print_tree -- works fine.. as EXE,
 *        program won't work properly as COM.
 *
 *    06/11/98 (Rob Lake)
 *        Found problem that caused COM not to work
 *
 *    06/12/98 (Rob Lake)
 *        debugged...
 *        added free mem routine
 *
 *    06/13/98 (Rob Lake)
 *        debugged the free mem routine
 *        debugged whole thing some more
 *        Notes:
 *        ReadDir stores Root name and _Read_Dir does the hard work
 *        PrintDir prints Root and _Print_Dir does the hard work
 *        KillDir kills Root _after_ _Kill_Dir does the hard work
 *        Integrated program into DIR.C(this file) and made some same
 *        changes throughout
 *
 *    06/14/98 (Rob Lake)
 *        Cleaned up code a bit, added comments
 *
 *    06/16/98 (Rob Lake)
 *        Added error checking to my previously added routines
 *
 *    06/17/98 (Rob Lake)
 *        Rewrote recursive functions, again! Most other recursive
 *        functions are now obsolete -- ReadDir, PrintDir, _Print_Dir,
 *        KillDir and _Kill_Dir.  do_recurse does what PrintDir did
 *        and _Read_Dir did what it did before along with what _Print_Dir
 *        did.  Makes /s a lot faster!
 *        Reports 2 more files/dirs that MS-DOS actually reports
 *        when used in root directory(is this because dir defaults
 *        to look for read only files?)
 *        Added support for /b, /a and /l
 *        Made error message similar to DOS error messages
 *        Added help screen
 *
 *    06/20/98 (Rob Lake)
 *        Added check for /-(switch) to turn off previously defined
 *        switches.
 *        Added ability to check for DIRCMD in environment and
 *        process it
 *
 *    06/21/98 (Rob Lake)
 *        Fixed up /B
 *        Now can dir *.ext/X, no spaces!
 *
 *    06/29/98 (Rob Lake)
 *        error message now found in command.h
 *
 *    07/08/1998 (John P. Price)
 *        removed extra returns; closer to MSDOS
 *        fixed wide display so that an extra return is not displayed
 *        when there is five filenames in the last line.
 *
 *    07/12/98 (Rob Lake)
 *        Changed error messages
 *
 *    27-Jul-1998 (John P Price <linux-guru@gcfl.net>)
 *        added config.h include
 *
 *
 *    04-Dec-1998 (Eric Kohl)
 *        Converted source code to Win32, except recursive dir ("dir /s").
 *
 *    10-Dec-1998 (Eric Kohl)
 *        Fixed recursive dir ("dir /s").
 *
 *    14-Dec-1998 (Eric Kohl)
 *        Converted to Win32 directory functions and
 *        fixed some output bugs. There are still some more ;)
 *
 *    10-Jan-1999 (Eric Kohl)
 *        Added "/N" and "/4" options, "/O" is a dummy.
 *        Added locale support.
 *
 *    20-Jan-1999 (Eric Kohl)
 *        Redirection safe!
 *
 *    01-Mar-1999 (Eric Kohl)
 *        Replaced all runtime io functions by their Win32 counterparts.
 *
 *    23-Feb-2001 (Carl Nettelblad <cnettel@hem.passagen.se>)
 *        dir /s now works in deeper trees
 *
 *    28-Jan-2004 (Michael Fritscher <michael@fritscher.net>)
 *        Fix for /p, so it is working under Windows in GUI-mode, too.
 *
 *    30-Apr-2004 (Filip Navara <xnavara@volny.cz>)
 *        Fix /w to print long names.
 *
 *    27-Feb-2005 (Konstantinos Paliouras <squarious@gmail.com>)
 *        Implemented all the switches that were missing, and made
 *        the ros dir very similar to windows dir. Major part of
 *        the code is rewritten. /p is removed, to be rewriten in
 *        the main cmd code.
 *
 *    1-Jul-2004 (Brandon Turner <turnerb7@msu.edu>)
 *        Added /p back in using ConOutPrintfPaging
 *
 *    3-feb-2007 (Paolo Devoti devotip at gmail)
 *        Removed variables formerly in use to handle pagination
 *		  Pagination belongs to ConOutPrintfPaging
 *		  Removed already commented out code of old pagination
 */

#include <precomp.h>

#ifdef INCLUDE_CMD_DIR



/* Time Field enumeration */
enum ETimeField
{
	TF_CREATIONDATE		= 0,
	TF_MODIFIEDDATE		= 1,
	TF_LASTACCESSEDDATE	= 2
};

/* Ordered by enumeration */
enum EOrderBy
{
	ORDER_NAME		= 0,
	ORDER_SIZE		= 1,
	ORDER_DIRECTORY	= 2,
	ORDER_EXTENSION	= 3,
	ORDER_TIME		= 4
};

/* The struct for holding the switches */
typedef struct _DirSwitchesFlags
{
	BOOL bBareFormat;	/* Bare Format */
	BOOL bTSeperator;	/* Thousands seperator */
	BOOL bWideList;		/* Wide list format	*/
	BOOL bWideListColSort;	/* Wide list format but sorted by column */
	BOOL bLowerCase;	/* Uses lower case */
	BOOL bNewLongList;	/* New long list */
	BOOL bPause;		/* Pause per page */
	BOOL bUser;			/* Displays the owner of file */
	BOOL bRecursive;	/* Displays files in specified directory and all sub */
	BOOL bShortName;	/* Displays the sort name of files if exist	*/
	BOOL b4Digit;		/* Four digit year	*/
	struct
	{
		DWORD dwAttribVal;	/* The desired state of attribute */
		DWORD dwAttribMask;	/* Which attributes to check */
		BOOL bUnSet;		/* A helper flag if "-" was given with the switch */
		BOOL bParSetted;	/* A helper flag if parameters of switch were given */
	} stAttribs;		/* Displays files with this attributes only */
	struct
	{
		enum EOrderBy eCriteria[3];	/* Criterias used to order by */
		BOOL bCriteriaRev[3];		/* If the criteria is in reversed order */
		short sCriteriaCount;		/* The quantity of criterias */
		BOOL bUnSet;				/* A helper flag if "-" was given with the switch */
		BOOL bParSetted;			/* A helper flag if parameters of switch were given */
	} stOrderBy;		/* Ordered by criterias */
	struct
	{
		enum ETimeField eTimeField;	/* The time field that will be used for */
		BOOL bUnSet;				/* A helper flag if "-" was given with the switch */
		BOOL bParSetted;			/* A helper flag if parameters of switch were given */
	} stTimeField;		/* The time field to display or use for sorting */
} DIRSWITCHFLAGS, *LPDIRSWITCHFLAGS;


typedef struct _DIRFINDLISTNODE
{
  WIN32_FIND_DATA stFindInfo;
  struct _DIRFINDLISTNODE *ptrNext;
} DIRFINDLISTNODE, *PDIRFINDLISTNODE;


typedef BOOL
(WINAPI *PGETFREEDISKSPACEEX)(LPCTSTR, PULARGE_INTEGER, PULARGE_INTEGER, PULARGE_INTEGER);


/* Globally save the # of dirs, files and bytes,
 * probabaly later pass them to functions. Rob Lake  */
static ULONG recurse_dir_cnt;
static ULONG recurse_file_cnt;
static ULARGE_INTEGER recurse_bytes;


/*
 * help
 *
 * displays help screen for dir
 * Rob Lake
 */
static VOID
DirHelp(VOID)
{
  ConOutResPaging(TRUE, STRING_DIR_HELP1);
}



/*
 * DirReadParameters
 *
 * Parse the parameters and switches of the command line and exports them
 */
static BOOL
DirReadParam(LPTSTR Line,				/* [IN] The line with the parameters & switches */
			LPTSTR** params,			/* [OUT] The parameters after parsing */
			LPINT entries,				/* [OUT] The number of parameters after parsing */
			LPDIRSWITCHFLAGS lpFlags)	/* [IN/OUT] The flags after calculating switches */
{
	TCHAR cCurSwitch;	/* The current switch */
	TCHAR cCurChar;		/* Current examing character */
	TCHAR cCurUChar;	/* Current upper examing character */
	BOOL bNegative;		/* Negative switch */
	BOOL bPNegative;	/* Negative switch parameter */
	BOOL bIntoQuotes;	/* A flag showing if we are in quotes (") */
	LPTSTR ptrStart;	/* A pointer to the first character of a parameter */
	LPTSTR ptrEnd;		/* A pointer to the last character of a parameter */
	LPTSTR temp;

	/* Initialize parameter array */
	if(!params)
		return FALSE;
	*params = NULL;
	*entries = 0;
	ptrStart = NULL;
	ptrEnd = NULL;

	/* Initialize variables; */
	cCurSwitch = _T(' ');
	bNegative = FALSE;
	bPNegative = FALSE;
	bIntoQuotes = FALSE;

	/* We suppose that switch parameters
	   were given to avoid setting them to default
	   if the switch was not given */
	lpFlags->stAttribs.bParSetted = TRUE;
	lpFlags->stOrderBy.bParSetted = TRUE;
	lpFlags->stTimeField.bParSetted = TRUE;


	/* Main Loop (see README_DIR.txt) */
	/* scan the command line char per char, and we process its char */
	while (*Line)
	{
		/* we save current character as it is and its upper case */
		cCurChar = *Line;
		cCurUChar = _totupper(*Line);

		/* 1st section (see README_DIR.txt) */
		/* When a switch is expecting */
		if (cCurSwitch == _T('/'))
		{
			if ((cCurUChar == _T('A')) ||(cCurUChar == _T('T')) || (cCurUChar == _T('O')))
			{
				cCurSwitch = cCurUChar;
				switch (cCurUChar)
				{
				case _T('A'):
					lpFlags->stAttribs.bUnSet = bNegative;
					lpFlags->stAttribs.bParSetted = FALSE;
					break;
				case _T('T'):
					lpFlags->stTimeField.bUnSet = bNegative;
					lpFlags->stTimeField.bParSetted = FALSE;
					break;
				case _T('O'):
					lpFlags->stOrderBy.bUnSet = bNegative;
					lpFlags->stOrderBy.bParSetted = FALSE;
					break;
				}
			}
			else if (cCurUChar == _T('L'))
				lpFlags->bLowerCase = ! bNegative;
			else if (cCurUChar == _T('B'))
				lpFlags->bBareFormat = ! bNegative;
			else if (cCurUChar == _T('C'))
				lpFlags->bTSeperator = ! bNegative;
			else if (cCurUChar == _T('W'))
				lpFlags->bWideList = ! bNegative;
			else if (cCurUChar == _T('D'))
				lpFlags->bWideListColSort = ! bNegative;
			else if (cCurUChar == _T('N'))
				lpFlags->bNewLongList = ! bNegative;
			else if (cCurUChar == _T('P'))
				lpFlags->bPause = ! bNegative;
			else if (cCurUChar == _T('Q'))
				lpFlags->bUser = ! bNegative;
			else if (cCurUChar == _T('S'))
				lpFlags->bRecursive = ! bNegative;
			else if (cCurUChar == _T('X'))
				lpFlags->bShortName = ! bNegative;
			else if (cCurChar == _T('4'))
				lpFlags->b4Digit = ! bNegative;
			else if (cCurChar == _T('?'))
			{
				DirHelp();
				return FALSE;
			}
			else if (cCurChar ==  _T('-'))
			{
				bNegative = TRUE;
			}
			else
			{
				error_invalid_switch ((TCHAR)_totupper (*Line));
				return FALSE;
			}

			/* We check if we calculated the negative value and realese the flag */
			if ((cCurChar != _T('-')) && bNegative)
				bNegative = FALSE;

			/* if not a,o,t or - option then next parameter is not a switch */
			if ((cCurSwitch == _T('/')) && (!bNegative))
				cCurSwitch = _T(' ');

		}
		else if ((cCurSwitch == _T(' ')) || (cCurSwitch == _T('P')))
		{
			/* 2nd section (see README_DIR.txt) */
			/* We are expecting parameter or the unknown */

			if (cCurChar == _T('/'))
				cCurSwitch = _T('/');

			/* Process a spacer */
			else if (cCurChar == _T(' '))
			{
				if (!bIntoQuotes)
				{
					cCurSwitch = _T(' ');
					if(ptrStart && ptrEnd)
					{
						temp = cmd_alloc(((ptrEnd - ptrStart) + 2) * sizeof (TCHAR));
						if(!temp)
							return FALSE;
						memcpy(temp, ptrStart, ((ptrEnd - ptrStart) + 2) * sizeof (TCHAR));
						temp[(ptrEnd - ptrStart + 1)] = _T('\0');
						if(!add_entry(entries, params, temp))
						{
							cmd_free(temp);
							freep(*params);
							return FALSE;
						}

						cmd_free(temp);

						ptrStart = NULL;
						ptrEnd = NULL;
					}
				}

			}
			else if (cCurChar == _T('\"'))
			{
				/* Process a quote */
				bIntoQuotes = !bIntoQuotes;
				if(!bIntoQuotes)
					ptrEnd = Line;
			}
			else
			{
				/* Process a character for parameter */
				if ((cCurSwitch == _T(' ')) && ptrStart && ptrEnd)
				{
					temp = cmd_alloc(((ptrEnd - ptrStart) + 2) * sizeof (TCHAR));
					if(!temp)
						return FALSE;
					memcpy(temp, ptrStart, ((ptrEnd - ptrStart) + 2) * sizeof (TCHAR));
					temp[(ptrEnd - ptrStart + 1)] = _T('\0');
					if(!add_entry(entries, params, temp))
					{
						cmd_free(temp);
						freep(*params);
						return FALSE;
					}

					cmd_free(temp);

					ptrStart = NULL;
					ptrEnd = NULL;
				}
				cCurSwitch = _T('P');
				if(!ptrStart)
					ptrStart = ptrEnd = Line;
				ptrEnd = Line;
			}
		}
		else
		{
			/* 3rd section (see README_DIR.txt) */
			/* We are waiting for switch parameters */

			/* Check if there are no more switch parameters */
			if ((cCurChar == _T('/')) || ( cCurChar == _T(' ')))
			{
				/* Wrong desicion path, reprocess current character */
				cCurSwitch = cCurChar;
				continue;
			}
			/* Process parameter switch */
			switch(cCurSwitch)
			{
			case _T('A'):	/* Switch parameters for /A (attributes filter) */
				/* Ok a switch parameter was given */
				lpFlags->stAttribs.bParSetted = TRUE;

				if (cCurChar == _T(':'))
					/* =V= dead command, used to make the "if" work */
					cCurChar = cCurChar;
				else if(cCurChar == _T('-'))
					bPNegative = TRUE;
				else if(cCurUChar == _T('D'))
				{
					lpFlags->stAttribs.dwAttribMask |= FILE_ATTRIBUTE_DIRECTORY;
					if (bPNegative)
						lpFlags->stAttribs.dwAttribVal &= ~FILE_ATTRIBUTE_DIRECTORY;
					else
						lpFlags->stAttribs.dwAttribVal |= FILE_ATTRIBUTE_DIRECTORY;
				}
				else if(cCurUChar == _T('R'))
				{
					lpFlags->stAttribs.dwAttribMask |= FILE_ATTRIBUTE_READONLY;
					if (bPNegative)
						lpFlags->stAttribs.dwAttribVal &= ~FILE_ATTRIBUTE_READONLY;
					else
						lpFlags->stAttribs.dwAttribVal |= FILE_ATTRIBUTE_READONLY;
				}
				else if(cCurUChar == _T('H'))
				{
					lpFlags->stAttribs.dwAttribMask |= FILE_ATTRIBUTE_HIDDEN;
					if (bPNegative)
						lpFlags->stAttribs.dwAttribVal &= ~FILE_ATTRIBUTE_HIDDEN;
					else
						lpFlags->stAttribs.dwAttribVal |= FILE_ATTRIBUTE_HIDDEN;
				}
				else if(cCurUChar == _T('A'))
				{
					lpFlags->stAttribs.dwAttribMask |= FILE_ATTRIBUTE_ARCHIVE;
					if (bPNegative)
						lpFlags->stAttribs.dwAttribVal &= ~FILE_ATTRIBUTE_ARCHIVE;
					else
						lpFlags->stAttribs.dwAttribVal |= FILE_ATTRIBUTE_ARCHIVE;
				}
				else if(cCurUChar == _T('S'))
				{
					lpFlags->stAttribs.dwAttribMask |= FILE_ATTRIBUTE_SYSTEM;
					if (bPNegative)
						lpFlags->stAttribs.dwAttribVal &= ~FILE_ATTRIBUTE_SYSTEM;
					else
						lpFlags->stAttribs.dwAttribVal |= FILE_ATTRIBUTE_SYSTEM;
				}
				else
				{
					error_parameter_format((TCHAR)_totupper (*Line));
					return FALSE;
				}
				break;
			case _T('T'):	/* Switch parameters for /T (time field) */

				/* Ok a switch parameter was given */
				lpFlags->stTimeField.bParSetted = TRUE;

				if (cCurChar == _T(':'))
					/* =V= dead command, used to make the "if" work */
					cCurChar = cCurChar;
				else if(cCurUChar == _T('C'))
					lpFlags->stTimeField.eTimeField= TF_CREATIONDATE ;
				else if(cCurUChar == _T('A'))
					lpFlags->stTimeField.eTimeField= TF_LASTACCESSEDDATE ;
				else if(cCurUChar == _T('W'))
					lpFlags->stTimeField.eTimeField= TF_MODIFIEDDATE  ;
				else
				{
					error_parameter_format((TCHAR)_totupper (*Line));
					return FALSE;
				}
				break;
			case _T('O'):	/* Switch parameters for /O (order) */
				/* Ok a switch parameter was given */
				lpFlags->stOrderBy.bParSetted = TRUE;

				if (cCurChar == _T(':'))
					/* <== dead command, used to make the "if" work */
					cCurChar = cCurChar;
				else if(cCurChar == _T('-'))
					bPNegative = TRUE;
				else if(cCurUChar == _T('N'))
				{
					if (lpFlags->stOrderBy.sCriteriaCount < 3) lpFlags->stOrderBy.sCriteriaCount++;
					lpFlags->stOrderBy.bCriteriaRev[lpFlags->stOrderBy.sCriteriaCount - 1] = bPNegative;
					lpFlags->stOrderBy.eCriteria[lpFlags->stOrderBy.sCriteriaCount - 1] = ORDER_NAME;
				}
				else if(cCurUChar == _T('S'))
				{
					if (lpFlags->stOrderBy.sCriteriaCount < 3) lpFlags->stOrderBy.sCriteriaCount++;
					lpFlags->stOrderBy.bCriteriaRev[lpFlags->stOrderBy.sCriteriaCount - 1] = bPNegative;
					lpFlags->stOrderBy.eCriteria[lpFlags->stOrderBy.sCriteriaCount - 1] = ORDER_SIZE;
				}
				else if(cCurUChar == _T('G'))
				{
					if (lpFlags->stOrderBy.sCriteriaCount < 3) lpFlags->stOrderBy.sCriteriaCount++;
					lpFlags->stOrderBy.bCriteriaRev[lpFlags->stOrderBy.sCriteriaCount - 1] = bPNegative;
					lpFlags->stOrderBy.eCriteria[lpFlags->stOrderBy.sCriteriaCount - 1] = ORDER_DIRECTORY;
				}
				else if(cCurUChar == _T('E'))
				{
					if (lpFlags->stOrderBy.sCriteriaCount < 3) lpFlags->stOrderBy.sCriteriaCount++;
					lpFlags->stOrderBy.bCriteriaRev[lpFlags->stOrderBy.sCriteriaCount - 1] = bPNegative;
					lpFlags->stOrderBy.eCriteria[lpFlags->stOrderBy.sCriteriaCount - 1] = ORDER_EXTENSION;
				}
				else if(cCurUChar == _T('D'))
				{
					if (lpFlags->stOrderBy.sCriteriaCount < 3) lpFlags->stOrderBy.sCriteriaCount++;
					lpFlags->stOrderBy.bCriteriaRev[lpFlags->stOrderBy.sCriteriaCount - 1] = bPNegative;
					lpFlags->stOrderBy.eCriteria[lpFlags->stOrderBy.sCriteriaCount - 1] = ORDER_TIME;
				}

				else
				{
					error_parameter_format((TCHAR)_totupper (*Line));
					return FALSE;
				}


			}
			/* We check if we calculated the negative value and realese the flag */
			if ((cCurChar != _T('-')) && bPNegative)
				bPNegative = FALSE;
		}

		Line++;
	}
	/* Terminate the parameters */
	if(ptrStart && ptrEnd)
	{
		temp = cmd_alloc((ptrEnd - ptrStart + 2) * sizeof(TCHAR));
		if(!temp)
			return FALSE;
		memcpy(temp, ptrStart, (ptrEnd - ptrStart + 1) * sizeof(TCHAR));
		temp[(ptrEnd - ptrStart + 1)] = _T('\0');
		if(!add_entry(entries, params, temp))
		{
			cmd_free(temp);
			freep(*params);
			return FALSE;
		}

		cmd_free(temp);

		ptrStart = NULL;
		ptrEnd = NULL;
	}

	/* Calculate the switches with no switch paramater  */
	if (!(lpFlags->stAttribs.bParSetted))
	{
		lpFlags->stAttribs.dwAttribVal = 0L;
		lpFlags->stAttribs.dwAttribMask = lpFlags->stAttribs.dwAttribVal;
	}
	if (!(lpFlags->stOrderBy.bParSetted))
	{
		lpFlags->stOrderBy.sCriteriaCount = 1;
		lpFlags->stOrderBy.eCriteria[0] = ORDER_NAME;
		lpFlags->stOrderBy.bCriteriaRev[0] = FALSE;
	}
	if (!(lpFlags->stOrderBy.bParSetted))
		lpFlags->stTimeField.eTimeField = TF_MODIFIEDDATE ;

	/* Calculate the unsetted switches (the "-" prefixed)*/
	if (lpFlags->stAttribs.bUnSet)
	{
		lpFlags->stAttribs.bUnSet = FALSE;
		lpFlags->stAttribs.dwAttribVal = 0L;
		lpFlags->stAttribs.dwAttribMask = FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM;
	}
	if (lpFlags->stOrderBy.bUnSet)
	{
		lpFlags->stOrderBy.bUnSet = FALSE;
		lpFlags->stOrderBy.sCriteriaCount = 0;
	}
	if (lpFlags->stTimeField.bUnSet )
	{
		lpFlags->stTimeField.bUnSet = FALSE;
		lpFlags->stTimeField.eTimeField = TF_MODIFIEDDATE;
	}
	return TRUE;
}


/*
 * PrintDirectoryHeader
 *
 * print the header for the dir command
 */
static BOOL
PrintDirectoryHeader(LPTSTR szPath, LPDIRSWITCHFLAGS lpFlags)
{
  TCHAR szMsg[RC_STRING_MAX_SIZE];
  TCHAR szFullDir[MAX_PATH];
  TCHAR szRootName[MAX_PATH];
  TCHAR szVolName[80];
  LPTSTR pszFilePart;
  DWORD dwSerialNr;

  if (lpFlags->bBareFormat)
    return TRUE;

  if (GetFullPathName(szPath, sizeof(szFullDir) / sizeof(TCHAR), szFullDir, &pszFilePart) == 0)
    {
      ErrorMessage(GetLastError(), _T("Failed to build full directory path"));
      return FALSE;
    }

  if (pszFilePart != NULL)
      *pszFilePart = _T('\0');

  /* get the media ID of the drive */
  if (!GetVolumePathName(szFullDir, szRootName, sizeof(szRootName) / sizeof(TCHAR)) ||
      !GetVolumeInformation(szRootName, szVolName, 80, &dwSerialNr,
			    NULL, NULL, NULL, 0))
    {
      return(TRUE);
    }

  /* print drive info */
  if (szVolName[0] != _T('\0'))
    {
      LoadString(CMD_ModuleHandle, STRING_DIR_HELP2, szMsg, RC_STRING_MAX_SIZE);
      //needs to have first paramter as TRUE because
	  //this is the first output and need to clear the static
	  if(lpFlags->bPause)
		 ConOutPrintfPaging(TRUE,szMsg, szRootName[0], szVolName);
	  else
		 ConOutPrintf(szMsg, szRootName[0], szVolName);

    }
  else
    {
      LoadString(CMD_ModuleHandle, STRING_DIR_HELP3, szMsg, RC_STRING_MAX_SIZE);
	if(lpFlags->bPause)
		 ConOutPrintfPaging(TRUE,szMsg, szRootName[0]);
	else
		 ConOutPrintf(szMsg, szRootName[0]);
    }

  /* print the volume serial number if the return was successful */
  LoadString(CMD_ModuleHandle, STRING_DIR_HELP4, (LPTSTR) szMsg, RC_STRING_MAX_SIZE);
  if(lpFlags->bPause)
	 ConOutPrintfPaging(FALSE,szMsg,
               HIWORD(dwSerialNr),
               LOWORD(dwSerialNr));
  else
	 ConOutPrintf(szMsg,
               HIWORD(dwSerialNr),
               LOWORD(dwSerialNr));


  return TRUE;
}


/*
 * convert
 *
 * insert commas into a number
 *
 */
#if 0
static INT
ConvertULong (ULONG num, LPTSTR des, INT len)
{
	TCHAR temp[32];
	INT c = 0;
	INT n = 0;

	if (num == 0)
	{
		des[0] = _T('0');
		des[1] = _T('\0');
		n = 1;
	}
	else
	{
		temp[31] = 0;
		while (num > 0)
		{
			if (((c + 1) % (nNumberGroups + 1)) == 0)
				temp[30 - c++] = cThousandSeparator;
			temp[30 - c++] = (TCHAR)(num % 10) + _T('0');
			num /= 10;
		}

		for (n = 0; n <= c; n++)
			des[n] = temp[31 - c + n];
	}

	return n;
}
#endif

static VOID
DirPrintFileDateTime(TCHAR *lpDate,
                     TCHAR *lpTime,
                     LPWIN32_FIND_DATA lpFile,
                     LPDIRSWITCHFLAGS lpFlags)
{
	FILETIME ft;
	SYSTEMTIME dt;
	TCHAR szDate[30];
	TCHAR szTime[30];
	WORD wYear;

	/* Select the right time field */
	switch (lpFlags->stTimeField.eTimeField)
	{
		case TF_CREATIONDATE:
			if (!FileTimeToLocalFileTime(&lpFile->ftCreationTime, &ft))
				return;
			FileTimeToSystemTime(&ft, &dt);
			break;

		case TF_LASTACCESSEDDATE :
			if (!FileTimeToLocalFileTime(&lpFile->ftLastAccessTime, &ft))
				return;
			FileTimeToSystemTime(&ft, &dt);
			break;

		case TF_MODIFIEDDATE:
			if (!FileTimeToLocalFileTime(&lpFile->ftLastWriteTime, &ft))
				return;
			FileTimeToSystemTime(&ft, &dt);
			break;
	}

	/* Format date */
	wYear = (lpFlags->b4Digit) ? dt.wYear : dt.wYear%100;
	switch (nDateFormat)
	{
		case 0: /* mmddyy */
		default:
			_stprintf (szDate, _T("%02d%c%02d%c%0*d"),
					dt.wMonth, cDateSeparator,
					dt.wDay, cDateSeparator,
					lpFlags->b4Digit?4:2, wYear);
			break;

		case 1: /* ddmmyy */
			_stprintf (szDate, _T("%02d%c%02d%c%0*d"),
					dt.wDay, cDateSeparator, dt.wMonth,
					cDateSeparator,lpFlags->b4Digit?4:2, wYear);
			break;

		case 2: /* yymmdd */
			_stprintf (szDate, _T("%0*d%c%02d%c%02d"),
					lpFlags->b4Digit?4:2, wYear, cDateSeparator,
					dt.wMonth, cDateSeparator, dt.wDay);
			break;
	}
	/* Format Time */
	switch (nTimeFormat)
	{
		case 0: /* 12 hour format */
		default:
			_stprintf (szTime,_T("%02d%c%02u%c"),
					(dt.wHour == 0 ? 12 : (dt.wHour <= 12 ? dt.wHour : dt.wHour - 12)),
					cTimeSeparator,
					 dt.wMinute, (dt.wHour <= 11 ? _T('a') : _T('p')));
			break;

		case 1: /* 24 hour format */
			_stprintf (szTime, _T("%02d%c%02u"),
					dt.wHour, cTimeSeparator, dt.wMinute);
			break;
	}
	/* Copy results */
	_tcscpy(lpDate, szDate);
	_tcscpy(lpTime, szTime);
}


static VOID
GetUserDiskFreeSpace(LPCTSTR lpRoot,
		     PULARGE_INTEGER lpFreeSpace)
{
  PGETFREEDISKSPACEEX pGetFreeDiskSpaceEx;
  HINSTANCE hInstance;
  DWORD dwSecPerCl;
  DWORD dwBytPerSec;
  DWORD dwFreeCl;
  DWORD dwTotCl;
  ULARGE_INTEGER TotalNumberOfBytes, TotalNumberOfFreeBytes;

  lpFreeSpace->QuadPart = 0;

  hInstance = LoadLibrary(_T("KERNEL32"));
  if (hInstance != NULL)
    {
      pGetFreeDiskSpaceEx = (PGETFREEDISKSPACEEX)GetProcAddress(hInstance,
#ifdef _UNICODE
					                        "GetDiskFreeSpaceExW");
#else
				                                "GetDiskFreeSpaceExA");
#endif
      if (pGetFreeDiskSpaceEx != NULL)
	{
	  if (pGetFreeDiskSpaceEx(lpRoot, lpFreeSpace, &TotalNumberOfBytes, &TotalNumberOfFreeBytes) == TRUE)
	    return;
	}
      FreeLibrary(hInstance);
    }

  GetDiskFreeSpace(lpRoot,
		   &dwSecPerCl,
		   &dwBytPerSec,
		   &dwFreeCl,
		   &dwTotCl);

  lpFreeSpace->QuadPart = dwSecPerCl * dwBytPerSec * dwFreeCl;
}


/*
 * print_summary: prints dir summary
 * Added by Rob Lake 06/17/98 to compact code
 * Just copied Tim's Code and patched it a bit
 *
 */
static INT
PrintSummary(LPTSTR szPath,
	     ULONG ulFiles,
	     ULONG ulDirs,
	     ULARGE_INTEGER u64Bytes,
	     LPDIRSWITCHFLAGS lpFlags,
	     BOOL TotalSummary)
{
	TCHAR szMsg[RC_STRING_MAX_SIZE];
	TCHAR szBuffer[64];
	ULARGE_INTEGER uliFree;


	/* Here we check if we didn't find anything */
	if (!(ulFiles + ulDirs))
	{
		if (!lpFlags->bRecursive || (TotalSummary && lpFlags->bRecursive))
			error_file_not_found();
		return 1;
	}


	/* In bare format we don't print results */
	if (lpFlags->bBareFormat)
		return 0;

	/* Print recursive specific results */

    /* Take this code offline to fix /S does not print duoble info */
   if (TotalSummary && lpFlags->bRecursive)
   {
      ConvertULargeInteger(u64Bytes, szBuffer, sizeof(szBuffer), lpFlags->bTSeperator);
      LoadString(CMD_ModuleHandle, STRING_DIR_HELP5, szMsg, RC_STRING_MAX_SIZE);
      if(lpFlags->bPause)
         ConOutPrintfPaging(FALSE,szMsg,ulFiles, szBuffer);
      else
         ConOutPrintf(szMsg,ulFiles, szBuffer);
   }
   else
   {
      /* Print File Summary */
      /* Condition to print summary is:
      If we are not in bare format and if we have results! */
      ConvertULargeInteger(u64Bytes, szBuffer, 20, lpFlags->bTSeperator);
      LoadString(CMD_ModuleHandle, STRING_DIR_HELP8, szMsg, RC_STRING_MAX_SIZE);
      if(lpFlags->bPause)
         ConOutPrintfPaging(FALSE,szMsg,ulFiles, szBuffer);
      else
         ConOutPrintf(szMsg,ulFiles, szBuffer);
   }

	/* Print total directories and freespace */
	if (!lpFlags->bRecursive || (TotalSummary && lpFlags->bRecursive))
	{
		GetUserDiskFreeSpace(szPath, &uliFree);
		ConvertULargeInteger(uliFree, szBuffer, sizeof(szBuffer), lpFlags->bTSeperator);
		LoadString(CMD_ModuleHandle, STRING_DIR_HELP6, (LPTSTR) szMsg, RC_STRING_MAX_SIZE);
		if(lpFlags->bPause)
			ConOutPrintfPaging(FALSE,szMsg,ulDirs, szBuffer);
		else
			ConOutPrintf(szMsg,ulDirs, szBuffer);
	}

	return 0;
}

/*
 * getExt
 *
 * Get the extension of a filename
 */
TCHAR* getExt(const TCHAR* file)
{
        static TCHAR *NoExt = _T("");
        TCHAR* lastdot = _tcsrchr(file, _T('.'));
	return (lastdot != NULL ? lastdot + 1 : NoExt);
}

/*
 * getName
 *
 * Get the name of the file without extension
 */
static LPTSTR
getName(const TCHAR* file, TCHAR * dest)
{
	int iLen;
	LPTSTR end;

	/* Check for "." and ".." folders */
	if ((_tcscmp(file, _T(".")) == 0) ||
	    (_tcscmp(file, _T("..")) == 0))
	{
		_tcscpy(dest,file);
		return dest;
	}

	end = _tcsrchr(file, _T('.'));
	if (!end)
		iLen = _tcslen(file);
	else
		iLen = (end - file);


	_tcsncpy(dest, file, iLen);
	*(dest + iLen) = _T('\0');

	return dest;
}


/*
 *  DirPrintNewList
 *
 * The function that prints in new style
 */
static VOID
DirPrintNewList(LPWIN32_FIND_DATA ptrFiles[],	/* [IN]Files' Info */
		DWORD dwCount,			/* [IN] The quantity of files */
		TCHAR *szCurPath,		/* [IN] Full path of current directory */
		LPDIRSWITCHFLAGS lpFlags)	/* [IN] The flags used */
{
  DWORD i;
  TCHAR szSize[30];
  TCHAR szShortName[15];
  TCHAR szDate[20];
  TCHAR szTime[20];
  INT iSizeFormat;
  ULARGE_INTEGER u64FileSize;

  for (i = 0;i < dwCount;i++)
  {
    /* Calculate size */
    if (ptrFiles[i]->dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
    {
      /* Junction */
      iSizeFormat = -14;
      _tcscpy(szSize, _T("<JUNCTION>"));
    }
    else if (ptrFiles[i]->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    {
      /* Directory */
      iSizeFormat = -14;
      _tcscpy(szSize, _T("<DIR>"));
    }
    else
    {
      /* File */
      iSizeFormat = 14;
      u64FileSize.HighPart = ptrFiles[i]->nFileSizeHigh;
      u64FileSize.LowPart = ptrFiles[i]->nFileSizeLow;
      ConvertULargeInteger(u64FileSize, szSize, 20, lpFlags->bTSeperator);
    }

    /* Calculate short name */
    szShortName[0] = _T('\0');
    if (lpFlags->bShortName)
      _stprintf(szShortName, _T(" %-12s"), ptrFiles[i]->cAlternateFileName);

    /* Format date and time */
    DirPrintFileDateTime(szDate, szTime, ptrFiles[i], lpFlags);

    /* Print the line */
    if(lpFlags->bPause)
	{
		if (ConOutPrintfPaging(FALSE,_T("%10s  %-6s    %*s%s %s\n"),
							szDate,
							szTime,
							iSizeFormat,
							szSize,
							szShortName,
							ptrFiles[i]->cFileName) == 1)
			return ;
	}
	else
		ConOutPrintf(_T("%10s  %-6s    %*s%s %s\n"),
							szDate,
							szTime,
							iSizeFormat,
							szSize,
							szShortName,
							ptrFiles[i]->cFileName);
  }
}


/*
 *  DirPrintWideList
 *
 * The function that prints in wide list
 */
static VOID
DirPrintWideList(LPWIN32_FIND_DATA ptrFiles[],	/* [IN] Files' Info */
				 DWORD dwCount,			/* [IN] The quantity of files */
				 TCHAR *szCurPath,		/* [IN] Full path of current directory */
				 LPDIRSWITCHFLAGS lpFlags)	/* [IN] The flags used */
{
  SHORT iScreenWidth;
  USHORT iColumns;
  USHORT iLines;
  UINT iLongestName;
  TCHAR szTempFname[MAX_PATH];
  DWORD i;
  DWORD j;
  DWORD temp;

  /* Calculate longest name */
  iLongestName = 1;
  for (i = 0; i < dwCount; i++)
  {
    if (ptrFiles[i]->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    {
      /* Directories need 2 additinal characters for brackets */
      if ((_tcslen(ptrFiles[i]->cFileName) + 2) > iLongestName)
        iLongestName = _tcslen(ptrFiles[i]->cFileName) + 2;
    }
    else
    {
      if (_tcslen(ptrFiles[i]->cFileName) > iLongestName)
        iLongestName = _tcslen(ptrFiles[i]->cFileName);
    }
  }

  /* Count the highest number of columns */
  GetScreenSize(&iScreenWidth, 0);
  iColumns = iScreenWidth / iLongestName;

  /* Check if there is enough space for spaces between names */
  if (((iLongestName * iColumns) + iColumns) >= (UINT)iScreenWidth)
    iColumns --;

  /* A last check at iColumns to avoid division by zero */
  if (!(iColumns))
    iColumns = 1;

  /* Print Column sorted */
  if (lpFlags->bWideListColSort)
  {
    /* Calculate the lines that will be printed */
//    iLines = ceil((float)dwCount/(float)iColumns);
    iLines = (USHORT)(dwCount / iColumns);

    for (i = 0;i < iLines;i++)
    {
      for (j = 0; j < iColumns; j++)
      {
        temp = (j * iLines) + i;
        if (temp >= dwCount)
           break;

        if (ptrFiles[temp]->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
          _stprintf(szTempFname, _T("[%s]"), ptrFiles[temp]->cFileName);
        else
          _stprintf(szTempFname, _T("%s"), ptrFiles[temp]->cFileName);

        if(lpFlags->bPause)
		   ConOutPrintfPaging(FALSE,_T("%-*s"), iLongestName + 1 , szTempFname);
		else
		   ConOutPrintf(_T("%-*s"), iLongestName + 1 , szTempFname);
      }

      if(lpFlags->bPause)
		 ConOutPrintfPaging(FALSE,_T("\n"));
	  else
		 ConOutPrintf(_T("\n"));
    }
  }
  else
  {
    /* Print Line sorted */
    for (i = 0; i < dwCount; i++)
    {
      if (ptrFiles[i]->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        _stprintf(szTempFname, _T("[%s]"), ptrFiles[i]->cFileName);
      else
        _stprintf(szTempFname, _T("%s"), ptrFiles[i]->cFileName);

      if(lpFlags->bPause)
		 ConOutPrintfPaging(FALSE,_T("%-*s"), iLongestName + 1, szTempFname );
	  else
		 ConOutPrintf(_T("%-*s"), iLongestName + 1, szTempFname );

      /*
       * We print a new line at the end of each column
       * except for the case that it is the last item.
       */
	  if (!((i + 1) % iColumns) && (i < (dwCount - 1)))
	  {
		  if(lpFlags->bPause)
			 ConOutPrintfPaging(FALSE,_T("\n"));
		  else
		     ConOutPrintf(_T("\n"));
	  }
    }

    /* Add a new line after the last item */
    if(lpFlags->bPause)
	   ConOutPrintfPaging(FALSE,_T("\n"));
	else
	   ConOutPrintf(_T("\n"));
  }
}


/*
 *  DirPrintOldList
 *
 * The function that prints in old style
 */
static VOID
DirPrintOldList(LPWIN32_FIND_DATA ptrFiles[],	/* [IN] Files' Info */
				DWORD dwCount,					/* [IN] The quantity of files */
				TCHAR * szCurPath,				/* [IN] Full path of current directory */
				LPDIRSWITCHFLAGS lpFlags)		/* [IN] The flags used */
{
DWORD i;						/* An indexer for "for"s */
TCHAR szName[10];				/* The name of file */
TCHAR szExt[5];					/* The extension of file */
TCHAR szDate[30],szTime[30];	/* Used to format time and date */
TCHAR szSize[30];				/* The size of file */
int iSizeFormat;				/* The format of size field */
ULARGE_INTEGER u64FileSize;		/* The file size */

	for(i = 0;i < dwCount;i++)
	{
		/* Broke 8.3 format */
		if (*ptrFiles[i]->cAlternateFileName )
		{
			/* If the file is long named then we read the alter name */
			getName( ptrFiles[i]->cAlternateFileName, szName);
			_tcscpy(szExt, getExt( ptrFiles[i]->cAlternateFileName));
		}
		else
		{
			/* If the file is not long name we read its original name */
			getName( ptrFiles[i]->cFileName, szName);
			_tcscpy(szExt, getExt( ptrFiles[i]->cFileName));
		}

		/* Calculate size */
		if (ptrFiles[i]->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			/* Directory, no size it's a directory*/
			iSizeFormat = -17;
			_tcscpy(szSize, _T("<DIR>"));
		}
		else
		{
			/* File */
			iSizeFormat = 17;
			u64FileSize.HighPart = ptrFiles[i]->nFileSizeHigh;
			u64FileSize.LowPart = ptrFiles[i]->nFileSizeLow;
			ConvertULargeInteger(u64FileSize, szSize, 20, lpFlags->bTSeperator);
		}

		/* Format date and time */
		DirPrintFileDateTime(szDate,szTime,ptrFiles[i],lpFlags);

		/* Print the line */
		if(lpFlags->bPause)
		{
		   if (ConOutPrintfPaging(FALSE,_T("%-8s %-3s  %*s %s  %s\n"),
								szName,			/* The file's 8.3 name */
								szExt,			/* The file's 8.3 extension */
								iSizeFormat,	/* print format for size column */
								szSize,			/* The size of file or "<DIR>" for dirs */
								szDate,			/* The date of file/dir */
								szTime) == 1)		/* The time of file/dir */
			{
				return ;
			}
		}
		else
		   ConOutPrintf(_T("%-8s %-3s  %*s %s  %s\n"),
								szName,			/* The file's 8.3 name */
								szExt,			/* The file's 8.3 extension */
								iSizeFormat,	/* print format for size column */
								szSize,			/* The size of file or "<DIR>" for dirs */
								szDate,			/* The date of file/dir */
								szTime);		/* The time of file/dir */
	}
}

/*
 *  DirPrintBareList
 *
 * The function that prints in bare format
 */
static VOID
DirPrintBareList(LPWIN32_FIND_DATA ptrFiles[],	/* [IN] Files' Info */
				 DWORD dwCount,			/* [IN] The number of files */
				 LPTSTR lpCurPath,		/* [IN] Full path of current directory */
				 LPDIRSWITCHFLAGS lpFlags)	/* [IN] The flags used */
{
	TCHAR szFullName[MAX_PATH];
	DWORD i;

	for (i = 0; i < dwCount; i++)
	{
		if ((_tcscmp(ptrFiles[i]->cFileName, _T(".")) == 0) ||
		    (_tcscmp(ptrFiles[i]->cFileName, _T("..")) == 0))
		{
			/* at bare format we don't print "." and ".." folder */
			continue;
		}
		if (lpFlags->bRecursive)
		{
			/* at recursive mode we print full path of file */
			_tcscpy(szFullName, lpCurPath);
			_tcscat(szFullName, ptrFiles[i]->cFileName);
			if(lpFlags->bPause)
			{
			   if (ConOutPrintfPaging(FALSE,_T("%s\n"), szFullName) == 1)
				{
					return ;
				}
			}
			else
			   ConOutPrintf(_T("%s\n"), szFullName);
		}
		else
		{
			/* if we are not in recursive mode we print the file names */
			if(lpFlags->bPause)
			{
			   if (ConOutPrintfPaging(FALSE,_T("%s\n"),ptrFiles[i]->cFileName) == 1)
				{
					return ;
				}
			}
			else
			   ConOutPrintf(_T("%s\n"),ptrFiles[i]->cFileName);
		}
	}
}


/*
 * DirPrintFiles
 *
 * The functions that prints the files list
 */
static VOID
DirPrintFiles(LPWIN32_FIND_DATA ptrFiles[],	/* [IN] Files' Info */
			  DWORD dwCount,			/* [IN] The quantity of files */
			  TCHAR *szCurPath,			/* [IN] Full path of current directory */
			  LPDIRSWITCHFLAGS lpFlags)		/* [IN] The flags used */
{
	TCHAR szMsg[RC_STRING_MAX_SIZE];
	TCHAR szTemp[MAX_PATH];			/* A buffer to format the directory header */
	LPTSTR pszFilePart;
	SIZE_T len;

	/* We cut the trailing \ of the full path, unless the path is a drive */
	if (GetFullPathName(szCurPath, sizeof(szTemp) / sizeof(TCHAR), szTemp, &pszFilePart) == 0)
	{
		pszFilePart = NULL;
		_tcscpy(szTemp, szCurPath);
	}
	else if (pszFilePart != NULL)
		*pszFilePart = _T('\0');
	else
	{
		len = _tcslen(szTemp);
		if (len > 0 && szTemp[len - 1] != _T('\\') &&
		    GetFileAttributes(szTemp) == INVALID_FILE_ATTRIBUTES &&
		    GetLastError() == ERROR_PATH_NOT_FOUND)
		{
			/* Special case for some fake dos devices, such as con:
			   GetFullPathName doesn't return a pszFilePart pointer
			   so we're going to fix this ourselves */
			while (len > 0 && szTemp[len - 1] != _T('\\'))
				szTemp[--len] = _T('\0');
		}
	}

	len = _tcslen(szTemp);
	if ((len != 3 || szTemp[len - 2] != _T(':')) && szTemp[len - 1] == _T('\\'))
		szTemp[len-1] = _T('\0');

	/* Condition to print header:
	   We are not printing in bare format
	   and if we are in recursive mode... we must have results */
	if (!(lpFlags->bBareFormat ) && !((lpFlags->bRecursive) && (dwCount <= 0)))
	{
		LoadString(CMD_ModuleHandle, STRING_DIR_HELP7, szMsg, RC_STRING_MAX_SIZE);
		if(lpFlags->bPause)
		{
		   if (ConOutPrintfPaging(FALSE,szMsg, szTemp) == 1)
			{
				return ;
			}
		}
		else
		   ConOutPrintf(szMsg, szTemp);
	}

	if (lpFlags->bBareFormat)
	{
		/* Bare format */
		DirPrintBareList(ptrFiles, dwCount, szCurPath, lpFlags);
	}
	else if(lpFlags->bShortName)
	{
		/* New list style / Short names */
		DirPrintNewList(ptrFiles, dwCount, szCurPath, lpFlags);
	}
	else if(lpFlags->bWideListColSort || lpFlags->bWideList)
	{
		/* Wide list */
		DirPrintWideList(ptrFiles, dwCount, szCurPath, lpFlags);
	}
	else if (lpFlags->bNewLongList )
	{
		/* New list style*/
		DirPrintNewList(ptrFiles, dwCount, szCurPath, lpFlags);
	}
	else
	{
		/* If nothing is selected old list is the default */
		DirPrintOldList(ptrFiles, dwCount, szCurPath, lpFlags);
	}
}



/*
 * CompareFiles
 *
 * Compares 2 files based on the order criteria
 */
static BOOL
CompareFiles(LPWIN32_FIND_DATA lpFile1,	/* [IN] A pointer to WIN32_FIND_DATA of file 1 */
			 LPWIN32_FIND_DATA lpFile2,	/* [IN] A pointer to WIN32_FIND_DATA of file 2 */
			 LPDIRSWITCHFLAGS lpFlags)	/* [IN] The flags that we use to list */
{
  ULARGE_INTEGER u64File1;
  ULARGE_INTEGER u64File2;
  int i;
  long iComp = 0;					/* The comparison result */

	/* Calculate critiries by order given from user */
	for (i = 0;i < lpFlags->stOrderBy.sCriteriaCount;i++)
	{

		/* Calculate criteria */
		switch(lpFlags->stOrderBy.eCriteria[i])
		{
		case ORDER_SIZE:		/* Order by size /o:s */
			/* concat the 32bit integers to a 64bit */
			u64File1.LowPart = lpFile1->nFileSizeLow;
			u64File1.HighPart = lpFile1->nFileSizeHigh;
			u64File2.LowPart = lpFile2->nFileSizeLow;
			u64File2.HighPart = lpFile2->nFileSizeHigh;

			/* In case that differnce is too big for a long */
			if (u64File1.QuadPart < u64File2.QuadPart)
				iComp = -1;
			else if (u64File1.QuadPart > u64File2.QuadPart)
				iComp = 1;
			else
				iComp = 0;
			break;

		case ORDER_DIRECTORY:	/* Order by directory attribute /o:g */
			iComp = ((lpFile2->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)-
				(lpFile1->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY));
			break;

		case ORDER_EXTENSION:	/* Order by extension name /o:e */
			iComp = _tcsicmp(getExt(lpFile1->cFileName),getExt(lpFile2->cFileName));
			break;

		case ORDER_NAME:		/* Order by filename /o:n */
			iComp = _tcsicmp(lpFile1->cFileName, lpFile2->cFileName);
			break;

		case ORDER_TIME:		/* Order by file's time /o:t */
			/* We compare files based on the time field selected by /t */
			switch(lpFlags->stTimeField.eTimeField)
			{
			case TF_CREATIONDATE:
				/* concat the 32bit integers to a 64bit */
				u64File1.LowPart = lpFile1->ftCreationTime.dwLowDateTime;
				u64File1.HighPart = lpFile1->ftCreationTime.dwHighDateTime ;
				u64File2.LowPart = lpFile2->ftCreationTime.dwLowDateTime;
				u64File2.HighPart = lpFile2->ftCreationTime.dwHighDateTime ;
				break;
			case TF_LASTACCESSEDDATE :
				/* concat the 32bit integers to a 64bit */
				u64File1.LowPart = lpFile1->ftLastAccessTime.dwLowDateTime;
				u64File1.HighPart = lpFile1->ftLastAccessTime.dwHighDateTime ;
				u64File2.LowPart = lpFile2->ftLastAccessTime.dwLowDateTime;
				u64File2.HighPart = lpFile2->ftLastAccessTime.dwHighDateTime ;
				break;
			case TF_MODIFIEDDATE:
				/* concat the 32bit integers to a 64bit */
				u64File1.LowPart = lpFile1->ftLastWriteTime.dwLowDateTime;
				u64File1.HighPart = lpFile1->ftLastWriteTime.dwHighDateTime ;
				u64File2.LowPart = lpFile2->ftLastWriteTime.dwLowDateTime;
				u64File2.HighPart = lpFile2->ftLastWriteTime.dwHighDateTime ;
				break;
			}

			/* In case that differnce is too big for a long */
			if (u64File1.QuadPart < u64File2.QuadPart)
				iComp = -1;
			else if (u64File1.QuadPart > u64File2.QuadPart)
				iComp = 1;
			else
				iComp = 0;
			break;
		}

		/* Reverse if desired */
		if (lpFlags->stOrderBy.bCriteriaRev[i])
			iComp *= -1;

		/* If that criteria was enough for distinguishing
		   the files/dirs,there is no need to calculate the others*/
		if (iComp != 0) break;
	}

	/* Translate the value of iComp to boolean */
	if (iComp > 0)
		return TRUE;
	else
		return FALSE;
}

/*
 * QsortFiles
 *
 * Sort files by the order criterias using quicksort method
 */
static VOID
QsortFiles(LPWIN32_FIND_DATA ptrArray[],	/* [IN/OUT] The array with file info pointers */
	   int i,				/* [IN]     The index of first item in array */
	   int j,				/* [IN]     The index to last item in array */
	   LPDIRSWITCHFLAGS lpFlags)		/* [IN]     The flags that we will use to sort */
{
	LPWIN32_FIND_DATA lpTemp;	/* A temporary pointer */
	int First, Last, Temp;
	BOOL Way;

	if (i < j)
	{
		First = i;
		Last = j;
		Way = TRUE;
		while (i != j)
		{
			if (Way == CompareFiles(ptrArray[i], ptrArray[j], lpFlags))
			{
				/* Swap the pointers of the array */
				lpTemp = ptrArray[i];
				ptrArray[i]= ptrArray[j];
				ptrArray[j] = lpTemp;

				/* Swap the indexes for inverting sorting */
				Temp = i;
				i = j;
				j =Temp;

				Way = !Way;
			}

			j += (!Way - Way);
		}

		QsortFiles(ptrArray,First, i-1, lpFlags);
		QsortFiles(ptrArray,i+1,Last, lpFlags);
	}
}



/*
 * DirList
 *
 * The functions that does everything except for printing results
 */
static INT
DirList(LPTSTR szPath,			/* [IN] The path that dir starts */
		LPDIRSWITCHFLAGS lpFlags)	/* [IN] The flags of the listing */
{
	HANDLE hSearch;							/* The handle of the search */
	HANDLE hRecSearch;						/* The handle for searching recursivly */
	WIN32_FIND_DATA wfdFileInfo;			/* The info of file that found */
	LPWIN32_FIND_DATA * ptrFileArray;		/* An array of pointers with all the files */
	PDIRFINDLISTNODE ptrStartNode;	/* The pointer to the first node */
	PDIRFINDLISTNODE ptrNextNode;	/* A pointer used for relatives refernces */
TCHAR szFullPath[MAX_PATH];				/* The full path that we are listing with trailing \ */
TCHAR szSubPath[MAX_PATH];
LPTSTR pszFilePart;
DWORD dwCount;							/* A counter of files found in directory */
DWORD dwCountFiles;						/* Counter for files */
DWORD dwCountDirs;						/* Counter for directories */
ULARGE_INTEGER u64CountBytes;			/* Counter for bytes */
ULARGE_INTEGER u64Temp;					/* A temporary counter */

	/* Initialize Variables */
	ptrStartNode = NULL;
	ptrNextNode = NULL;
	dwCount = 0;
	dwCountFiles = 0;
	dwCountDirs = 0;
	u64CountBytes.QuadPart = 0;

	/* Create szFullPath */
	if (GetFullPathName(szPath, sizeof(szFullPath) / sizeof(TCHAR), szFullPath, &pszFilePart) == 0)
	{
		_tcscpy (szFullPath, szPath);
		if (szFullPath[_tcslen(szFullPath) - 1] != _T('\\'))
			_tcscat (szFullPath, _T("\\"));
		pszFilePart = NULL;
	}

	/* If no wildcard or file was specified and this is a directory, then
	   display all files in it */
	wfdFileInfo.dwFileAttributes = GetFileAttributes(szFullPath);
	if (wfdFileInfo.dwFileAttributes != INVALID_FILE_ATTRIBUTES &&
	    (wfdFileInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
	{
		_tcscat(szFullPath, _T("\\*"));
	}

	/* Prepare the linked list, first node is allocated */
	ptrStartNode = cmd_alloc(sizeof(DIRFINDLISTNODE));
	if (ptrStartNode == NULL)
	{
		WARN("DEBUG: Cannot allocate memory for ptrStartNode!\n");
		return 1;	/* Error cannot allocate memory for 1st object */
	}
	ptrNextNode = ptrStartNode;

	/* Collect the results for the current folder */
	hSearch = FindFirstFile(szFullPath, &wfdFileInfo);
	do
	{
		if (hSearch != INVALID_HANDLE_VALUE)
		{
			/* Here we filter all the specified attributes */
			if ((wfdFileInfo.dwFileAttributes & lpFlags->stAttribs.dwAttribMask )
				== (lpFlags->stAttribs.dwAttribMask & lpFlags->stAttribs.dwAttribVal ))
			{
				ptrNextNode->ptrNext = cmd_alloc(sizeof(DIRFINDLISTNODE));
				if (ptrNextNode->ptrNext == NULL)
				{
					WARN("DEBUG: Cannot allocate memory for ptrNextNode->ptrNext!\n");
					while (ptrStartNode)
					{
						ptrNextNode = ptrStartNode->ptrNext;
						cmd_free(ptrStartNode);
						ptrStartNode = ptrNextNode;
						dwCount --;
					}
					return 1;
				}

				/* If cmd_alloc fails we go to next file in hope it works,
				   without braking the linked list! */
				if (ptrNextNode->ptrNext)
				{
					/* Copy the info of search at linked list */
					memcpy(&ptrNextNode->ptrNext->stFindInfo,
					       &wfdFileInfo,
					       sizeof(WIN32_FIND_DATA));

					/* If lower case is selected do it here */
					if (lpFlags->bLowerCase)
					{
						_tcslwr(ptrNextNode->ptrNext->stFindInfo.cAlternateFileName);
						_tcslwr(ptrNextNode->ptrNext->stFindInfo.cFileName);
					}

					/* Continue at next node at linked list */
					ptrNextNode = ptrNextNode->ptrNext;
					dwCount ++;

					/* Grab statistics */
					if (wfdFileInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
					{
						/* Directory */
						dwCountDirs++;
					}
					else
					{
						/* File */
						dwCountFiles++;
						u64Temp.HighPart = wfdFileInfo.nFileSizeHigh;
						u64Temp.LowPart = wfdFileInfo.nFileSizeLow;
						u64CountBytes.QuadPart += u64Temp.QuadPart;
					}
				}
			}
		}
	} while(FindNextFile(hSearch, &wfdFileInfo));
	FindClose(hSearch);

	/* Terminate list */
	ptrNextNode->ptrNext = NULL;

	/* Calculate and allocate space need for making an array of pointers */
	ptrFileArray = cmd_alloc(sizeof(LPWIN32_FIND_DATA) * dwCount);
	if (ptrFileArray == NULL)
	{
		WARN("DEBUG: Cannot allocate memory for ptrFileArray!\n");
		while (ptrStartNode)
		{
			ptrNextNode = ptrStartNode->ptrNext;
			cmd_free(ptrStartNode);
			ptrStartNode = ptrNextNode;
			dwCount --;
		}
		return 1;
	}

	/*
	 * Create an array of pointers from the linked list
	 * this will be used to sort and print data, rather than the list
	 */
	ptrNextNode = ptrStartNode;
	dwCount = 0;
	while (ptrNextNode->ptrNext)
	{
		*(ptrFileArray + dwCount) = &ptrNextNode->ptrNext->stFindInfo;
		ptrNextNode = ptrNextNode->ptrNext;
		dwCount++;
	}

	/* Sort Data if requested*/
	if (lpFlags->stOrderBy.sCriteriaCount > 0)
		QsortFiles(ptrFileArray, 0, dwCount-1, lpFlags);

	/* Print Data */
	DirPrintFiles(ptrFileArray, dwCount, szFullPath, lpFlags);

	if (lpFlags->bRecursive)
	{
		PrintSummary(szFullPath,
			dwCountFiles,
			dwCountDirs,
			u64CountBytes,
			lpFlags,
			FALSE);
	}

	/* Free array */
	cmd_free(ptrFileArray);
	if (CheckCtrlBreak(BREAK_INPUT))
		return 1;


	/* Add statistics to recursive statistics*/
	recurse_dir_cnt += dwCountDirs;
	recurse_file_cnt += dwCountFiles;
	recurse_bytes.QuadPart += u64CountBytes.QuadPart;

	/* Do the recursive job if requested
	   the recursive is be done on ALL(indepent of their attribs)
	   directoried of the current one.*/
	if (lpFlags->bRecursive)
	{
		/* The new search is involving any *.* file */
		if (pszFilePart != NULL)
		{
			memcpy(szSubPath, szFullPath, (pszFilePart - szFullPath) * sizeof(TCHAR));
			szSubPath[pszFilePart - szFullPath] = _T('\0');
		}
		else
			_tcscpy(szSubPath, szFullPath);

		_tcscat(szSubPath, _T("*.*"));

		hRecSearch = FindFirstFile (szSubPath, &wfdFileInfo);
		do
		{
			if (hRecSearch != INVALID_HANDLE_VALUE)
			{
				/* We search for directories other than "." and ".." */
				if ((_tcsicmp(wfdFileInfo.cFileName, _T(".")) != 0) &&
				    (_tcsicmp(wfdFileInfo.cFileName, _T("..")) != 0 ) &&
				    (wfdFileInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
				{
					/* Concat the path and the directory to do recursive */
					if (pszFilePart != NULL)
					{
						memcpy(szSubPath, szFullPath, (pszFilePart - szFullPath) * sizeof(TCHAR));
						szSubPath[pszFilePart - szFullPath] = _T('\0');
					}
					else
						_tcscpy(szSubPath, szFullPath);

					_tcscat(szSubPath, wfdFileInfo.cFileName);
					_tcscat(szSubPath, _T("\\"));
					if (pszFilePart != NULL)
						_tcscat(szSubPath, pszFilePart);

					/* We do the same for the folder */
					if (DirList(szSubPath, lpFlags) != 0)
					{
						return 1;
					}
				}
			}
		}while(FindNextFile(hRecSearch,&wfdFileInfo));
		FindClose(hRecSearch);
	}

	/* Free linked list */
	while (ptrStartNode)
	{
		ptrNextNode = ptrStartNode->ptrNext;
		cmd_free(ptrStartNode);
		ptrStartNode = ptrNextNode;
		dwCount --;
	}

	return 0;
}



/*
 * dir
 *
 * internal dir command
 */
INT
CommandDir(LPTSTR rest)
{
	TCHAR	dircmd[256];	/* A variable to store the DIRCMD enviroment variable */
	TCHAR	path[MAX_PATH];
	TCHAR	prev_volume[MAX_PATH];
	LPTSTR*	params = NULL;
	LPTSTR	pszFilePart;
	INT		entries = 0;
	UINT	loop = 0;
	DIRSWITCHFLAGS stFlags;
	INT	ret = 1;
	BOOL ChangedVolume;

	/* Initialize Switch Flags < Default switches are setted here!> */
	stFlags.b4Digit = TRUE;
	stFlags.bBareFormat = FALSE;
	stFlags.bLowerCase = FALSE;
	stFlags.bNewLongList = TRUE;
	stFlags.bPause = FALSE;
	stFlags.bRecursive = FALSE;
	stFlags.bShortName = FALSE;
	stFlags.bTSeperator = TRUE;
	stFlags.bUser = FALSE;
	stFlags.bWideList = FALSE;
	stFlags.bWideListColSort = FALSE;
	stFlags.stTimeField.eTimeField = TF_MODIFIEDDATE;
	stFlags.stTimeField.bUnSet = FALSE;
	stFlags.stAttribs.dwAttribMask = FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM;
	stFlags.stAttribs.dwAttribVal = 0L;
	stFlags.stAttribs.bUnSet = FALSE;
	stFlags.stOrderBy.sCriteriaCount = 0;
	stFlags.stOrderBy.bUnSet = FALSE;

	nErrorLevel = 0;

	/* read the parameters from the DIRCMD environment variable */
	if (GetEnvironmentVariable (_T("DIRCMD"), dircmd, 256))
		if (!DirReadParam(dircmd, &params, &entries, &stFlags))
		{
			nErrorLevel = 1;
			goto cleanup;
		}

	/* read the parameters */
	if (!DirReadParam(rest, &params, &entries, &stFlags) || CheckCtrlBreak(BREAK_INPUT))
	{
		nErrorLevel = 1;
		goto cleanup;
	}

	/* default to current directory */
	if(entries == 0) {
		if(!add_entry(&entries, &params, _T("*"))) {
			nErrorLevel = 1;
			goto cleanup;
		}
	}

	prev_volume[0] = _T('\0');

	for(loop = 0; loop < (UINT)entries; loop++)
	{
		if (CheckCtrlBreak(BREAK_INPUT))
		{
			nErrorLevel = 1;
			goto cleanup;
		}

		recurse_dir_cnt = 0L;
		recurse_file_cnt = 0L;
		recurse_bytes.QuadPart = 0;

	/* <Debug :>
	   Uncomment this to show the final state of switch flags*/
	{
		int i;
		TRACE("Attributes mask/value %x/%x\n",stFlags.stAttribs.dwAttribMask,stFlags.stAttribs.dwAttribVal  );
		TRACE("(B) Bare format : %i\n", stFlags.bBareFormat );
		TRACE("(C) Thousand : %i\n", stFlags.bTSeperator );
		TRACE("(W) Wide list : %i\n", stFlags.bWideList );
		TRACE("(D) Wide list sort by column : %i\n", stFlags.bWideListColSort );
		TRACE("(L) Lowercase : %i\n", stFlags.bLowerCase );
		TRACE("(N) New : %i\n", stFlags.bNewLongList );
		TRACE("(O) Order : %i\n", stFlags.stOrderBy.sCriteriaCount );
		for (i =0;i<stFlags.stOrderBy.sCriteriaCount;i++)
			TRACE(" Order Criteria [%i]: %i (Reversed: %i)\n",i, stFlags.stOrderBy.eCriteria[i], stFlags.stOrderBy.bCriteriaRev[i] );
		TRACE("(P) Pause : %i\n", stFlags.bPause  );
		TRACE("(Q) Owner : %i\n", stFlags.bUser );
		TRACE("(S) Recursive : %i\n", stFlags.bRecursive );
		TRACE("(T) Time field : %i\n", stFlags.stTimeField.eTimeField );
		TRACE("(X) Short names : %i\n", stFlags.bShortName );
		TRACE("Parameter : %s\n", debugstr_aw(params[loop]) );
	}

		/* Print the drive header if the volume changed */
		ChangedVolume = TRUE;

		if (!stFlags.bBareFormat &&
		    GetVolumePathName(params[loop], path, sizeof(path) / sizeof(TCHAR)))
		{
			if (!_tcscmp(path, prev_volume))
				ChangedVolume = FALSE;
			else
				_tcscpy(prev_volume, path);
		}
		else if (GetFullPathName(params[loop], sizeof(path) / sizeof(TCHAR), path, &pszFilePart) != 0)
		{
			if (pszFilePart != NULL)
				*pszFilePart = _T('\0');
		}
		else
			_tcscpy(path, params[loop]);

		if (ChangedVolume && !stFlags.bBareFormat) {
			if (!PrintDirectoryHeader (params[loop], &stFlags)) {
				nErrorLevel = 1;
				goto cleanup;
			}
		}

		/* do the actual dir */
		if (DirList (params[loop], &stFlags))
		{
			nErrorLevel = 1;
			goto cleanup;
		}

		/* print the footer */
		PrintSummary(path,
			recurse_file_cnt,
			recurse_dir_cnt,
			recurse_bytes,
			&stFlags,
			TRUE);
	}

	ret = 0;

cleanup:
	freep(params);

	return ret;
}

#endif

/* EOF */

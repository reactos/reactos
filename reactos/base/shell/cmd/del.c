/*
*  DEL.C - del internal command.
*
*
*  History:
*
*    06/29/98 (Rob Lake rlake@cs.mun.ca)
*        rewrote del to support wildcards
*        added my name to the contributors
*
*    07/13/98 (Rob Lake)
*        fixed bug that caused del not to delete file with out
*        attribute. moved set, del, ren, and ver to there own files
*
*    27-Jul-1998 (John P Price <linux-guru@gcfl.net>)
*        added config.h include
*
*    09-Dec-1998 (Eric Kohl)
*        Fixed command line parsing bugs.
*
*    21-Jan-1999 (Eric Kohl)
*        Started major rewrite using a new structure.
*
*    03-Feb-1999 (Eric Kohl)
*        First working version.
*
*    30-Mar-1999 (Eric Kohl)
*        Added quiet ("/Q"), wipe ("/W") and zap ("/Z") option.
*
*    06-Nov-1999 (Eric Kohl)
*        Little fix to keep DEL quiet inside batch files.
*
*    28-Jan-2004 (Michael Fritscher <michael@fritscher.net>)
*        Added prompt ("/P"), yes ("/Y") and wipe("/W") option.
*
*    22-Jun-2005 (Brandon Turner <turnerb7@msu.edu>)
*        Added exclusive deletion "del * -abc.txt -text*.txt"
*
*    22-Jun-2005 (Brandon Turner <turnerb7@msu.edu>)
*        Implemented /A   example "del /A:H /A:-R *.exe -ping.exe"
*    
*    07-Aug-2005
*        Removed the exclusive deletion (see two comments above) because '-' is a valid file name character.
*        Optimized the recursive deletion in directories.
*        Preload some nice strings.
*/

#include <precomp.h>
#include "resource.h"

#ifdef INCLUDE_CMD_DEL


enum
{
	DEL_ATTRIBUTES = 0x001,   /* /A */
	DEL_NOTHING    = 0x004,   /* /N */
	DEL_PROMPT     = 0x008,   /* /P */
	DEL_QUIET      = 0x010,   /* /Q */
	DEL_SUBDIR     = 0x020,   /* /S */
	DEL_TOTAL      = 0x040,   /* /T */
	DEL_WIPE       = 0x080,   /* /W */
	DEL_EMPTYDIR   = 0x100,   /* /X : not implemented */
	DEL_YES        = 0x200,   /* /Y */
	DEL_FORCE      = 0x800    /* /F */
};

enum
{
	ATTR_ARCHIVE     = 0x001,   /* /A:A */
	ATTR_HIDDEN      = 0x002,   /* /A:H */
	ATTR_SYSTEM      = 0x004,   /* /A:S */
	ATTR_READ_ONLY   = 0x008,   /* /A:R */
	ATTR_N_ARCHIVE   = 0x010,   /* /A:-A */
	ATTR_N_HIDDEN    = 0x020,   /* /A:-H */
	ATTR_N_SYSTEM    = 0x040,   /* /A:-S */
	ATTR_N_READ_ONLY = 0x080    /* /A:-R */
};

static TCHAR szDeleteWipe[RC_STRING_MAX_SIZE];
static TCHAR szDelHelp2[RC_STRING_MAX_SIZE];
static TCHAR szDelHelp3[RC_STRING_MAX_SIZE];
static TCHAR szDelHelp4[RC_STRING_MAX_SIZE];
static TCHAR szDelError5[RC_STRING_MAX_SIZE];
static TCHAR szDelError6[RC_STRING_MAX_SIZE];
static TCHAR szDelError7[RC_STRING_MAX_SIZE];
static TCHAR CMDPath[MAX_PATH];

static BOOLEAN StringsLoaded = FALSE;

static VOID LoadStrings(VOID)
{
        LoadString( CMD_ModuleHandle, STRING_DELETE_WIPE, szDeleteWipe, RC_STRING_MAX_SIZE);
        LoadString( CMD_ModuleHandle, STRING_DEL_HELP2, szDelHelp2, RC_STRING_MAX_SIZE);
        LoadString( CMD_ModuleHandle, STRING_DEL_HELP3, szDelHelp3, RC_STRING_MAX_SIZE);
        LoadString( CMD_ModuleHandle, STRING_DEL_HELP4, szDelHelp4, RC_STRING_MAX_SIZE);
        LoadString( CMD_ModuleHandle, STRING_DEL_ERROR5, szDelError5, RC_STRING_MAX_SIZE);
        LoadString( CMD_ModuleHandle, STRING_DEL_ERROR6, szDelError6, RC_STRING_MAX_SIZE);
        LoadString( CMD_ModuleHandle, STRING_DEL_ERROR7, szDelError7, RC_STRING_MAX_SIZE);
        GetModuleFileName(NULL, CMDPath, MAX_PATH);
        StringsLoaded = TRUE;
}

static BOOL
RemoveFile (LPTSTR lpFileName, DWORD dwFlags, WIN32_FIND_DATA* f)
{
	/*This function is called by CommandDelete and
	does the actual process of deleting the single 
	file*/
		if(CheckCtrlBreak(BREAK_INPUT))
			return 1;

        /*check to see if it is read only and if this is done based on /A
          if it is done by file name, access is denied. However, if it is done 
          using the /A switch you must un-read only the file and allow it to be
          deleted*/
        if((dwFlags & DEL_ATTRIBUTES) || (dwFlags & DEL_FORCE))
        {
                if(f->dwFileAttributes & FILE_ATTRIBUTE_READONLY)
		{
			/*setting file to normal, not saving old attrs first
			  because the file is going to be deleted anyways
			  so the only thing that matters is that it isnt
			  read only.*/
                        SetFileAttributes(lpFileName,FILE_ATTRIBUTE_NORMAL);
                }
        }

        if (dwFlags & DEL_WIPE)
        {

	        HANDLE file;
	        DWORD temp;
#define BufferSize 65536
	        BYTE buffer[BufferSize];
	        LONGLONG i;
	        LARGE_INTEGER FileSize;

	        FileSize.u.HighPart = f->nFileSizeHigh;
                FileSize.u.LowPart = f->nFileSizeLow;

	        for(i = 0; i < BufferSize; i++)
	        {
		        buffer[i]=rand() % 256;
	        }
	        file = CreateFile (lpFileName, GENERIC_WRITE, 0, NULL, OPEN_EXISTING,  FILE_FLAG_WRITE_THROUGH, NULL);
                if (file != INVALID_HANDLE_VALUE)
                {
                        for(i = 0; i < (FileSize.QuadPart - BufferSize); i += BufferSize)
		        {
			        WriteFile (file, buffer, BufferSize, &temp, NULL);
			        ConOutPrintf (_T("%I64d%% %s\r"),(i * (LONGLONG)100)/FileSize.QuadPart,szDeleteWipe);
		        }
		        WriteFile (file, buffer, (DWORD)(FileSize.QuadPart - i), &temp, NULL);
		        ConOutPrintf (_T("100%% %s\n"),szDeleteWipe);
		        CloseHandle (file);
                }
        }

	return DeleteFile (lpFileName);
}


static DWORD
DeleteFiles(LPTSTR FileName, DWORD* dwFlags, DWORD dwAttrFlags)
{
        TCHAR szFullPath[MAX_PATH];
        TCHAR szFileName[MAX_PATH];
        LPTSTR pFilePart;
        HANDLE hFile;
        WIN32_FIND_DATA f;
        BOOL bExclusion;
        INT res;
        DWORD dwFiles = 0;

        _tcscpy(szFileName, FileName);

        if(_tcschr (szFileName, _T('*')) == NULL && 
	   IsExistingDirectory (szFileName))
        {
	        /* If it doesnt have a \ at the end already then on needs to be added */
		if(szFileName[_tcslen(szFileName) -  1] != _T('\\'))
                        _tcscat (szFileName, _T("\\"));
                /* Add a wildcard after the \ */
                _tcscat (szFileName, _T("*"));
        }

	if(!_tcscmp (szFileName, _T("*")) ||
           !_tcscmp (szFileName, _T("*.*")) ||
           (szFileName[_tcslen(szFileName) -  2] == _T('\\') && szFileName[_tcslen(szFileName) -  1] == _T('*')))
        {
                /* well, the user wants to delete everything but if they didnt yes DEL_YES, DEL_QUIET, or DEL_PROMPT
	           then we are going to want to make sure that in fact they want to do that.  */

		if (!((*dwFlags & DEL_YES) || (*dwFlags & DEL_QUIET) || (*dwFlags & DEL_PROMPT)))
	        {
        	        res = FilePromptYNA (szDelHelp2);
		        if ((res == PROMPT_NO) || (res == PROMPT_BREAK))
			        return 0x80000000;
		        if(res == PROMPT_ALL)
			        *dwFlags |= DEL_YES;
	        }
	}

        GetFullPathName (szFileName,
                         MAX_PATH,
                         szFullPath,
                         &pFilePart);

        hFile = FindFirstFile(szFullPath, &f);
        if (hFile != INVALID_HANDLE_VALUE)
        {
                do
                {
                        bExclusion = FALSE;

		        /*if it is going to be excluded by - no need to check attrs*/
		        if(*dwFlags & DEL_ATTRIBUTES && !bExclusion)
		        {

			        /*save if file attr check if user doesnt care about that attr anyways*/
			        if(dwAttrFlags & ATTR_ARCHIVE && !(f.dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE))
				        bExclusion = TRUE;
			        if(dwAttrFlags & ATTR_HIDDEN && !(f.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN))
				        bExclusion = TRUE;
			        if(dwAttrFlags & ATTR_SYSTEM && !(f.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM))
				        bExclusion = TRUE;
			        if(dwAttrFlags & ATTR_READ_ONLY && !(f.dwFileAttributes & FILE_ATTRIBUTE_READONLY))
			                bExclusion = TRUE;
		                if(dwAttrFlags & ATTR_N_ARCHIVE && (f.dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE))
			                bExclusion = TRUE;
		                if(dwAttrFlags & ATTR_N_HIDDEN && (f.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN))
			                bExclusion = TRUE;
		                if(dwAttrFlags & ATTR_N_SYSTEM && (f.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM))
			                bExclusion = TRUE;
		                if(dwAttrFlags & ATTR_N_READ_ONLY && (f.dwFileAttributes & FILE_ATTRIBUTE_READONLY))
			                bExclusion = TRUE;
	                }

	                if(bExclusion)
		                continue;

		        /* ignore directories */
		        if (f.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		                continue;


		        _tcscpy (pFilePart, f.cFileName);

		        /* We cant delete ourselves */
		        if(!_tcscmp (CMDPath,szFullPath))
			        continue;


#ifdef _DEBUG
		        ConErrPrintf(_T("Full filename: %s\n"), szFullPath);
#endif

		        /* ask for deleting */
		        if (*dwFlags & DEL_PROMPT)
		        {
		                ConErrPrintf(szDelError5, szFullPath);

		                res = FilePromptYN (szDelError6);

		                if ((res == PROMPT_NO) || (res == PROMPT_BREAK))
		                {
								 nErrorLevel = 0;
			                continue;
		                }
	                }

	                /*user cant ask it to be quiet and tell you what it did*/
	                if (!(*dwFlags & DEL_QUIET) && !(*dwFlags & DEL_TOTAL))
	                {
		                ConErrPrintf(szDelError7, szFullPath);
	                }

	                /* delete the file */
	                if(*dwFlags & DEL_NOTHING)
		                continue;

	                if(RemoveFile (szFullPath, *dwFlags, &f))
		                dwFiles++;
		        else
                        {		
		                ErrorMessage (GetLastError(), _T(""));
//                                FindClose(hFile);
//                                return -1;
	                }
                }
                while (FindNextFile (hFile, &f));
	        FindClose (hFile);
        }
        return dwFiles;
}


static DWORD 
ProcessDirectory(LPTSTR FileName, DWORD* dwFlags, DWORD dwAttrFlags)
{
        TCHAR szFullPath[MAX_PATH];
        LPTSTR pFilePart;
        LPTSTR pSearchPart;
        HANDLE hFile;
        WIN32_FIND_DATA f;
        DWORD dwFiles = 0;

        GetFullPathName (FileName,
                         MAX_PATH,
                         szFullPath,
                         &pFilePart);

        dwFiles = DeleteFiles(szFullPath, dwFlags, dwAttrFlags);
        if (dwFiles & 0x80000000)
                return dwFiles;

        if (*dwFlags & DEL_SUBDIR)
        {
	        /* Get just the file name */
	        pSearchPart = _tcsrchr(FileName,_T('\\'));
	        if(pSearchPart != NULL)
		        pSearchPart++;
	        else
		        pSearchPart = FileName;

	        /* Get the full path to the file */
	        GetFullPathName (FileName,MAX_PATH,szFullPath,NULL);

	        /* strip the filename off of it */
                pFilePart = _tcsrchr(szFullPath, _T('\\'));
                if (pFilePart == NULL)
                {
                        pFilePart = szFullPath;
                }
                else
                {
                        pFilePart++;
                }

                _tcscpy(pFilePart, _T("*"));

                hFile = FindFirstFile(szFullPath, &f);
                if (hFile != INVALID_HANDLE_VALUE)
                {
                        do
                        {
       		                if (!(f.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ||
                                    !_tcscmp(f.cFileName, _T(".")) ||
                                    !_tcscmp(f.cFileName, _T("..")))
		                        continue;

                                _tcscpy(pFilePart, f.cFileName);
                                _tcscat(pFilePart, _T("\\"));
                                _tcscat(pFilePart, pSearchPart);

                                dwFiles +=ProcessDirectory(szFullPath, dwFlags, dwAttrFlags);
                                if (dwFiles & 0x80000000)
                                {
                                    break;
                                }
                        }
                        while (FindNextFile (hFile, &f));
	                FindClose (hFile);
                }
        }
        return dwFiles;
}        



INT CommandDelete (LPTSTR cmd, LPTSTR param)
{
	/*cmd is the command that was given, in this case it will always be "del" or "delete"
	param is whatever is given after the command*/

	LPTSTR *arg = NULL;
	INT args;
	INT i;
	INT   nEvalArgs = 0; /* nunber of evaluated arguments */
	DWORD dwFlags = 0;
	DWORD dwAttrFlags = 0;
	DWORD dwFiles = 0;
	LONG ch;
	TCHAR szOrginalArg[MAX_PATH];

	/*checks the first two chars of param to see if it is /?
	this however allows the following command to not show help
	"del frog.txt /?" */

        if (!StringsLoaded)
        {
                LoadStrings();
        }

	if (!_tcsncmp (param, _T("/?"), 2))
	{
		ConOutResPaging(TRUE,STRING_DEL_HELP1);
		return 0;
	}

	nErrorLevel = 0;

	arg = split (param, &args, FALSE);

	if (args == 0)
	{
		/* only command given */
		error_req_param_missing ();
		freep (arg);
		return 1;
	}
	/* check for options anywhere in command line */
	for (i = 0; i < args; i++)
	{
		if (*arg[i] == _T('/'))
		{
			/*found a command, but check to make sure it has something after it*/
			if (_tcslen (arg[i]) >= 2)
			{
				ch = _totupper (arg[i][1]);
				if (ch == _T('N'))
				{
					dwFlags |= DEL_NOTHING;
				}
				else if (ch == _T('P'))
				{
					dwFlags |= DEL_PROMPT;
				}
				else if (ch == _T('Q'))
				{
					dwFlags |= DEL_QUIET;
				}
				else if (ch == _T('F'))
				{
					dwFlags |= DEL_FORCE;
				}
				else if (ch == _T('S'))
				{
					dwFlags |= DEL_SUBDIR;
				}
				else if (ch == _T('T'))
				{
					dwFlags |= DEL_TOTAL;
				}
				else if (ch == _T('W'))
				{
					dwFlags |= DEL_WIPE;
				}
				else if (ch == _T('Y'))
				{
					dwFlags |= DEL_YES;
				}
				else if (ch == _T('A'))
				{

					dwFlags |= DEL_ATTRIBUTES;
					/*the proper syntax for /A has a min of 4 chars
					i.e. /A:R or /A:-H */
					if (_tcslen (arg[i]) < 4)
					{
						error_invalid_parameter_format(arg[i]);
						return 0;
					}
					ch = _totupper (arg[i][3]);
					if (_tcslen (arg[i]) == 4)
					{						
						if(ch == _T('A'))
						{
							dwAttrFlags |= ATTR_ARCHIVE;
						}
						if(ch == _T('H'))
						{
							dwAttrFlags |= ATTR_HIDDEN;
						}
						if(ch == _T('S'))
						{
							dwAttrFlags |= ATTR_SYSTEM;
						}
						if(ch == _T('R'))
						{
							dwAttrFlags |= ATTR_READ_ONLY;
						}
					}
					if (_tcslen (arg[i]) == 5)
					{
						if(ch == _T('-'))
						{
							ch = _totupper (arg[i][4]);
							if(ch == _T('A'))
							{
								dwAttrFlags |= ATTR_N_ARCHIVE;
							}
							if(ch == _T('H'))
							{
								dwAttrFlags |= ATTR_N_HIDDEN;
							}
							if(ch == _T('S'))
							{
								dwAttrFlags |= ATTR_N_SYSTEM;
							}
							if(ch == _T('R'))
							{
								dwAttrFlags |= ATTR_N_READ_ONLY;
							}
						}
					}
				}
			}

			nEvalArgs++;
		}
	}

	/* there are only options on the command line --> error!!!
	there is the same number of args as there is flags, so none of the args were filenames*/
	if (args == nEvalArgs)
	{
		error_req_param_missing ();
		freep (arg);
		return 1;
	}

	/* keep quiet within batch files */
	if (bc != NULL)
		dwFlags |= DEL_QUIET;

	/* check for filenames anywhere in command line */
	for (i = 0; i < args && !(dwFiles & 0x80000000); i++)
	{

                /*this checks to see if it isnt a flag, if it isnt, we assume it is a file name*/
		if((*arg[i] == _T('/')) || (*arg[i] == _T('-')))
			continue;

		/* We want to make a copies of the argument */
		if(_tcslen(arg[i]) == 2 && arg[i][1] == _T(':'))
		{
			/* Check for C: D: ... */
			GetRootPath(arg[i],szOrginalArg,MAX_PATH);
		}
		else
		{
			_tcscpy(szOrginalArg,arg[i]);
		}
                dwFiles += ProcessDirectory(szOrginalArg, &dwFlags, dwAttrFlags);

        }
	
	freep (arg);

	/*Based on MS cmd, we only tell what files are being deleted when /S is used */
	if (dwFlags & DEL_TOTAL)
	{
                dwFiles &= 0x7fffffff;
		if (dwFiles < 2)
		{
                        ConOutPrintf(szDelHelp3, dwFiles);
		}
		else
		{
			ConOutPrintf(szDelHelp4, dwFiles);
		}
	}

	return 0;
}


#endif

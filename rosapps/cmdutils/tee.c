/* 
 * TEE.C - external command.
 *
 * clone from 4nt tee command
 *
 * 01 Sep 1999 - Dr.F <dfaustus@freemail.it>
 *     started 
 *
 *
 */


#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <malloc.h>



#define TEE_BUFFER_SIZE 8192

/*these are function that emulate the ones used in cmd*/

/*many of them are just copied in this file from their
original location*/

VOID ConOutPuts (LPTSTR szText)
{
	DWORD dwWritten;

	WriteFile (GetStdHandle (STD_OUTPUT_HANDLE), szText, _tcslen(szText), &dwWritten, NULL);
#if 0
	WriteFile (GetStdHandle (STD_OUTPUT_HANDLE), "\x0a\x0d", 2, &dwWritten, NULL);
#endif
	WriteFile (GetStdHandle (STD_OUTPUT_HANDLE), "\n", 1, &dwWritten, NULL);
}


VOID ConErrPrintf (LPTSTR szFormat, ...)
{
	DWORD dwWritten;
	TCHAR szOut[4096];
	va_list arg_ptr;

	va_start (arg_ptr, szFormat);
	_vstprintf (szOut, szFormat, arg_ptr);
	va_end (arg_ptr);

	WriteFile (GetStdHandle (STD_ERROR_HANDLE), szOut, _tcslen(szOut), &dwWritten, NULL);
}



VOID error_sfile_not_found (LPTSTR f)
{
	ConErrPrintf (_T("Error opening file") _T(" - %s\n"), f);
}




VOID ConErrPuts (LPTSTR szText)
{
	ConErrPrintf("%s\n",szText );
}


INT main (int argc,char **p)
{
	/*reading/writing buffer*/
	TCHAR buff[TEE_BUFFER_SIZE];

	/*handle for file and console*/
	HANDLE hConsoleIn,hConsoleOut;
	
	/*bytes written by WriteFile and ReadFile*/
	DWORD dwRead,dwWritten;


	BOOL bRet,bAppend=FALSE;


	/*command line parsing stuff*/
	LPTSTR tmp;
	INT i;
	BOOL bQuote;

	/*file list implementation*/	
	LPTSTR *files;
	INT iFileCounter=0;
	HANDLE *hFile;

	/*used to remove '"' (if any)*/
	INT add;

	DWORD dw;


	if (argc < 2)
		return 1;

	if (_tcsncmp (p[1], _T("/?"), 2) == 0)
	{
		ConOutPuts (_T("Copy standard input to both standard output and a file.\n"
		               "\n"
		               "TEE [/A] file...\n"
		               "\n"
		               "  file  One or more files that will receive output.\n"
		               "  /A    Append output to files.\n"));
		return 0;
	}

	files = malloc(sizeof(LPTSTR)*argc);
	hFile = malloc(sizeof(HANDLE)*argc);

	hConsoleIn=GetStdHandle(STD_INPUT_HANDLE);
	hConsoleOut=GetStdHandle(STD_OUTPUT_HANDLE);

	/*parse command line for /a and file name(s)*/
	for(i=1;i <argc;i++)
	{
		bQuote=FALSE;
		add=0;

		if(_tcsnicmp(p[i],_T("/a"),2) == 0)
		{
			bAppend = TRUE;
			continue;
		}

		/*remove quote if any*/
		if (p[i][0] == _T('"'))
		{
			tmp = _tcschr (p[i]+1, _T('"'));
			if (tmp != 0)
			{
				add = 1;
				*tmp= _T('\0');
			}
		}

		/*add filename to array of filename*/
/*
		if(  iFileCounter >= sizeof(files) / sizeof(*files)  )
		{
			ConErrPrintf("too many files, maximum is %d\n",sizeof(files) / sizeof(*files));			
			return 1;
		}
		*/

		files[iFileCounter++]= p[i]+add;
	}

	/*open file(s)*/
	for(i=0;i<iFileCounter;i++)
	{	
		//l=0;
		hFile[i] = CreateFile(files[i],GENERIC_WRITE,
			0,NULL,
			OPEN_ALWAYS,
			FILE_ATTRIBUTE_NORMAL,NULL);

		if (hFile[i] == INVALID_HANDLE_VALUE)
		{
			error_sfile_not_found (files[i]);

			for(i=0;i<iFileCounter;i++)
				CloseHandle (hFile[i]);

			free (files);
			free (hFile);

			return 1;
		}

		/*set append mode*/
		if (bAppend)
		{
			if (GetFileType (hFile[i]) == FILE_TYPE_DISK)
			{
				dw = SetFilePointer (hFile[i],0,NULL,FILE_END);
				if (dw == 0xFFFFFFFF)
				{
					ConErrPrintf(_T("error moving to end of file %s"),files[i]);

					for(i=0;i<iFileCounter;i++)
						CloseHandle (hFile[i]);

					free (files);
					free (hFile);

					return 1;
				}

				ConErrPrintf(_T("SetFilePointer() = %d\n"),dw);
			}
		}
	}

	/*read and write*/
	do
	{
		bRet = ReadFile(hConsoleIn,buff,sizeof(buff),&dwRead,NULL);

		if (dwRead>0 && bRet)
		{
			for(i=0;i<iFileCounter;i++)
				WriteFile(hFile[i],buff,dwRead,&dwWritten,NULL);

			WriteFile(hConsoleOut,buff,dwRead,&dwWritten,NULL);
		}
	} while(dwRead>0 && bRet);

	for(i=0;i<iFileCounter;i++)
		CloseHandle (hFile[i]);

	free (files);
	free (hFile);

	return 0;
}

/* EOF */
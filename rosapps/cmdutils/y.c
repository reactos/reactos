/*
 * Y.C - Y external command.
 *
 * clone from 4nt y command
 *
 * 02 Oct 1999 (Paolo Pantaleo)
 *     started
 *
 *
 */

#define BUFF_SIZE 4096

#include <windows.h>
#include <stdio.h>
#include <tchar.h>

static
VOID ConErrPrintf (LPTSTR szFormat, ...)
{
	DWORD dwWritten;
	TCHAR szOut[BUFF_SIZE];
	va_list arg_ptr;

	va_start (arg_ptr, szFormat);
	_vstprintf (szOut, szFormat, arg_ptr);
	va_end (arg_ptr);

	WriteFile (GetStdHandle (STD_ERROR_HANDLE), szOut, _tcslen(szOut), &dwWritten, NULL);
}


static
VOID ConOutPuts (LPTSTR szText)
{
	DWORD dwWritten;

	WriteFile (GetStdHandle (STD_OUTPUT_HANDLE), szText, _tcslen(szText), &dwWritten, NULL);
	WriteFile (GetStdHandle (STD_OUTPUT_HANDLE), "\n", 1, &dwWritten, NULL);
}


int main (int argc, char **argv)
{
	INT i;
	HANDLE hFind;
	HANDLE hConsoleIn, hConsoleOut, hFile;
	char buff[BUFF_SIZE];
	DWORD dwRead,dwWritten;
	BOOL bRet;
	WIN32_FIND_DATA FindData;

	hConsoleIn = GetStdHandle(STD_INPUT_HANDLE);
	hConsoleOut = GetStdHandle(STD_OUTPUT_HANDLE);

	if (_tcsncmp (argv[1], _T("/?"), 2) == 0)
	{
		ConOutPuts(_T("copy stdin to stdout and then files to stdout\n"
		              "\n"
		              "Y [files]\n"
		              "\n"
		              "files         files to copy to stdout"));
		return 0;
	}

	/*stdin to stdout*/
	do
	{
		bRet = ReadFile(hConsoleIn,buff,sizeof(buff),&dwRead,NULL);

		if (dwRead>0 && bRet)
			WriteFile(hConsoleOut,buff,dwRead,&dwWritten,NULL);

	} while(dwRead>0 && bRet);


	/*files to stdout*/
	Sleep(0);

	for (i = 1; i < argc; i++)
	{
		hFind=FindFirstFile(argv[i],&FindData);
		
		if (hFind==INVALID_HANDLE_VALUE)
		{
			ConErrPrintf("File not found - %s\n",argv[i]);
			continue;
		}

		do
		{
			hFile = CreateFile(FindData.cFileName,
				GENERIC_READ,
				FILE_SHARE_READ,NULL,
				OPEN_EXISTING,
				FILE_ATTRIBUTE_NORMAL,NULL);

			if(hFile == INVALID_HANDLE_VALUE)
			{
				ConErrPrintf("File not found - %s\n",FindData.cFileName);
				continue;
			}

			do
			{
				bRet = ReadFile(hFile,buff,sizeof(buff),&dwRead,NULL);

				if (dwRead>0 && bRet)
					WriteFile(hConsoleOut,buff,dwRead,&dwWritten,NULL);
			
			} while(dwRead>0 && bRet);

			CloseHandle(hFile);

		}
		while(FindNextFile(hFind,&FindData));

		FindClose(hFile);
	}

	return 0;
}

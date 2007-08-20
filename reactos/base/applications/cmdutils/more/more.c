/* $Id$
 * 
 * MORE.C - external command.
 *
 * clone from 4nt more command
 *
 * 26 Sep 1999 - Paolo Pantaleo <paolopan@freemail.it>
 *     started
 * Oct 2003 - Timothy Schepens <tischepe at fastmail dot fm>
 *     use window size instead of buffer size.
 */

#include <windows.h>
#include <malloc.h>
#include <tchar.h>


DWORD len;
LPTSTR msg = _T("--- continue ---");


/*handle for file and console*/
HANDLE hStdIn;
HANDLE hStdOut;
HANDLE hStdErr;
HANDLE hKeyboard;


static VOID
GetScreenSize (PSHORT maxx, PSHORT maxy)
{
	CONSOLE_SCREEN_BUFFER_INFO csbi;

	GetConsoleScreenBufferInfo (hStdOut, &csbi);
		*maxx = (csbi.srWindow.Right - csbi.srWindow.Left) + 1;
		*maxy = (csbi.srWindow.Bottom  - csbi.srWindow.Top) - 4;

}


static
VOID ConOutPuts (LPTSTR szText)
{
	DWORD dwWritten;

	WriteFile (GetStdHandle (STD_OUTPUT_HANDLE), szText, _tcslen(szText), &dwWritten, NULL);
	WriteFile (GetStdHandle (STD_OUTPUT_HANDLE), "\n", 1, &dwWritten, NULL);
}


static VOID
ConInKey (VOID)
{
	INPUT_RECORD ir;
	DWORD dwRead;

	do
	{
		ReadConsoleInput (hKeyboard, &ir, 1, &dwRead);
		if ((ir.EventType == KEY_EVENT) &&
			(ir.Event.KeyEvent.bKeyDown == TRUE))
			return;
	}
	while (TRUE);
}


static VOID
WaitForKey (VOID)
{
	DWORD dwWritten;

	WriteFile (hStdErr,msg , len, &dwWritten, NULL);

	ConInKey();

	WriteFile (hStdErr, _T("\n"), 1, &dwWritten, NULL);

//	FlushConsoleInputBuffer (hConsoleIn);
}


//INT CommandMore (LPTSTR cmd, LPTSTR param)
int main (int argc, char **argv)
{
	SHORT maxx,maxy;
	SHORT line_count=0,ch_count=0;
	DWORD i, last;
	HANDLE hFile = INVALID_HANDLE_VALUE;
	TCHAR szFullPath[MAX_PATH];

	/*reading/writing buffer*/
	TCHAR *buff;

	/*bytes written by WriteFile and ReadFile*/
	DWORD dwRead,dwWritten;

	/*ReadFile() return value*/
	BOOL bRet;

	len = _tcslen (msg);
	hStdIn = GetStdHandle(STD_INPUT_HANDLE);
	hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
	hStdErr = GetStdHandle(STD_ERROR_HANDLE);

	if (argc > 1 && _tcsncmp (argv[1], _T("/?"), 2) == 0)
	{
		ConOutPuts(_T("Help text still missing!!"));
		return 0;
	}

	hKeyboard = CreateFile (_T("CONIN$"), GENERIC_READ,
	                        0,NULL,OPEN_ALWAYS,0,0);

	GetScreenSize(&maxx,&maxy);

	buff=malloc(4096);

	FlushConsoleInputBuffer (hKeyboard);

	if(argc > 1)
	{
		GetFullPathName(argv[1], MAX_PATH, szFullPath, NULL);
		hFile = CreateFile (szFullPath, GENERIC_READ,
	                        0,NULL,OPEN_ALWAYS,0,0);
        
		if (hFile == INVALID_HANDLE_VALUE)
		{
			ConOutPuts(_T("The file could not be opened"));
			return 0;
		}
	}
	else
	{
		hFile = hStdIn;
	}

	do
	{
		bRet = ReadFile(hFile,buff,4096,&dwRead,NULL);

		for(last=i=0;i<dwRead && bRet;i++)
		{
			ch_count++;
			if(buff[i] == _T('\n') || ch_count == maxx)
			{
				ch_count=0;
				line_count++;
				if (line_count == maxy)
				{
					line_count = 0;
					WriteFile(hStdOut,&buff[last], i-last+1, &dwWritten, NULL);
					last=i+1;
					FlushFileBuffers (hStdOut);
					WaitForKey ();
				}
			}
		}
		if (last<dwRead && bRet)
			WriteFile(hStdOut,&buff[last], dwRead-last, &dwWritten, NULL);

	}
	while(dwRead>0 && bRet);

	free (buff);
	CloseHandle (hKeyboard);
	if (hFile != hStdIn)
		CloseHandle (hFile);

	return 0;
}

/* EOF */

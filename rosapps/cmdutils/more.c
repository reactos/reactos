/* 
 * MORE.C - external command.
 *
 * clone from 4nt more command
 *
 * 26 Sep 1999 - Dr.F <dfaustus@freemail.it>
 *     started 
 */

#include <windows.h>
#include <malloc.h>
#include <tchar.h>


DWORD len;
LPTSTR msg = "--- continue ---";


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

	if (maxx)
		*maxx = csbi.dwSize.X;
	if (maxy)
		*maxy = csbi.dwSize.Y;
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
	INT i;

	/*reading/writing buffer*/
	TCHAR *buff;

	/*bytes written by WriteFile and ReadFile*/
	DWORD dwRead,dwWritten;

	/*ReadFile() return value*/
	BOOL bRet;

	len = _tcslen (msg);
	hStdIn = GetStdHandle(STD_INPUT_HANDLE);
	hKeyboard = CreateFile ("CONIN$", GENERIC_READ,
	                        0,NULL,OPEN_ALWAYS,0,0);
	hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
	hStdErr = GetStdHandle(STD_ERROR_HANDLE);

	GetScreenSize(&maxx,&maxy);

	buff=malloc(maxx);

	FlushConsoleInputBuffer (hKeyboard);

	do
	{
		bRet = ReadFile(hStdIn,buff,1,&dwRead,NULL);

		if (dwRead>0 && bRet)
			WriteFile(hStdOut,buff,dwRead,&dwWritten,NULL);

		for(i=0;i<dwRead;i++)
		{
			ch_count++;
			if(buff[i] == _T('\x0a') || ch_count == maxx)
			{
				ch_count=0;
				line_count++;
				if (line_count == maxy-1)
				{
					line_count = 0;
					FlushFileBuffers (hStdOut);
					WaitForKey ();
				}
			}
		}
	}
	while(dwRead>0 && bRet);

	free (buff);
	CloseHandle (hKeyboard);

	return 0;
}

/* EOF */
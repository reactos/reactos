/* $Id: alive.c,v 1.1 2001/03/18 20:20:13 ea Exp $
 *
 */
#include <windows.h>
#include <stdlib.h>

HANDLE	StandardOutput = INVALID_HANDLE_VALUE;
WCHAR	Message [80];
DWORD	CharactersToWrite = 0;
DWORD	WrittenCharacters = 0;
INT	d = 0, h = 0, m = 0, s = 0;

int
main (int argc, char * argv [])
{
	StandardOutput = GetStdHandle (STD_OUTPUT_HANDLE);
	if (INVALID_HANDLE_VALUE == StandardOutput)
	{
		return (EXIT_FAILURE);
	}
	while (TRUE)
	{
		/* Prepare the message and update it */
		CharactersToWrite =
			wsprintfW (
				Message,
				L"Alive for %dd %dh %d' %d\"   \r",
				d, h, m, s
				);
		WriteConsoleW (
			StandardOutput,
			Message,
			CharactersToWrite,
			& WrittenCharacters,
			NULL
			);
		/* suspend the execution for 1s */
		Sleep (1000);
		/* increment seconds */
		++ s;
		if (60 == s) { s = 0; ++ m; }
		if (60 == m) { m = 0; ++ h; }
		if (24 == h) { h = 0; ++ d; }
	}
	return (EXIT_SUCCESS);
}

/* EOF */

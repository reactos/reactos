/* $Id: alive.c,v 1.2 2001/03/26 21:30:20 ea Exp $
 *
 */
#include <windows.h>
#include <stdlib.h>

HANDLE	StandardOutput = INVALID_HANDLE_VALUE;
CHAR	Message [80];
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
			wsprintf (
				Message,
				"Alive for %dd %dh %d' %d\"   \r",
				d, h, m, s
				);
		WriteConsole (
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

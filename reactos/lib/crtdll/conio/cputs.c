/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/crtdll/conio/cputs.c
 * PURPOSE:     Writes a character to stdout
 * PROGRAMER:   Boudewijn Dekker
 * UPDATE HISTORY:
 *              28/12/98: Created
 */
#include <windows.h>
#include <conio.h>
#include <string.h>

int     cputs(const char *_str)
{
	int len = strlen(_str);
	int written = 0;
	if ( !WriteFile(GetStdHandle(STD_OUTPUT_HANDLE),_str,len,&written,NULL)) 
		return -1;
	return 0;
}
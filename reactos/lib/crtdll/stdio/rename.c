#include <windows.h>
#include <stdio.h>
#include <io.h> 



int rename(const char *old, const char *new)
{
 	if ( !MoveFile(old,new) )
		return -1;

	return 0;
}



#include <windows.h>
#include <crtdll/stdio.h>
#include <crtdll/io.h> 


int rename(const char *old_, const char *new_)
{
 	if ( !MoveFile(old_,new_) )
		return -1;

	return 0;
}



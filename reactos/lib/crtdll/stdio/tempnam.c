#include <windows.h>
#include <crtdll/stdio.h>
#include <crtdll/stdlib.h>


char *_tempnam(const char *dir,const char *prefix )
{
	char *TempFileName;
	TempFileName = malloc(MAX_PATH);
	GetTempFileName(
 		dir,	
    		prefix,	
    		98,	
    		TempFileName 	
   	);
	return TempFileName;
}

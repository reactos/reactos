#include <windows.h>
#include <crtdll/stdio.h>
#include <crtdll/stdlib.h>


char *_tempnam(const char *dir,const char *prefix )
{
	char *TempFileName = malloc(MAX_PATH);
	char *d;
	if ( dir == NULL )
		d = getenv("TMP");
	else 
		d = (char *)dir;

	if ( GetTempFileNameA(d, prefix, 0, TempFileName ) == 0 ) {
		free(TempFileName);
		return NULL;
	}

	return TempFileName;
}

#include <windows.h>
#include <crtdll/stdio.h>
#include <crtdll/string.h>



char * tmpnam(char *s)
{
	char PathName[MAX_PATH];
	static char static_buf[MAX_PATH];
   	GetTempPath(MAX_PATH,PathName);
   	GetTempFileNameA(PathName,	"ARI",007,static_buf);
	strcpy(s,static_buf);
	return s;
}

#include <windows.h>
#include <stdio.h>
#include <string.h>

char *
_tmpnam(char *s);

char *
tmpnam(char *s)
{
	return _tmpnam(s);
}


char *
_tmpnam(char *s)
{
	char PathName[MAX_PATH];
	static char static_buf[MAX_PATH];
   	GetTempPath(MAX_PATH,PathName);
   	GetTempFileNameA(PathName,	"ARI",007,static_buf);
	strcpy(s,static_buf);
	return s;
}

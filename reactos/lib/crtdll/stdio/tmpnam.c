#include <windows.h>
#include <msvcrt/stdio.h>
#include <msvcrt/string.h>


char * tmpnam(char *s)
{
	char PathName[MAX_PATH];
	static char static_buf[MAX_PATH];

    GetTempPathA(MAX_PATH, PathName);
   	GetTempFileNameA(PathName,	"ARI",007,static_buf);
	strcpy(s,static_buf);

    return s;
}

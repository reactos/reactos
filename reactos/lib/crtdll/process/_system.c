#include <crtdll/process.h>
#include <windows.h>
#include <crtdll/stdlib.h>
#include <crtdll/string.h>

int system(const char *command)
{
	char CmdLine[MAX_PATH];
	char *comspec = getenv("COMSPEC");
	if ( comspec == NULL )
		comspec = "cmd.exe";
	strcpy(CmdLine,comspec);
	strcat(CmdLine," /C ");
	if ( !WinExec(CmdLine,SW_SHOWNORMAL) < 31 )
		return -1;

	return 0;
}

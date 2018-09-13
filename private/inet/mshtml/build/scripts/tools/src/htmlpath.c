

#include <direct.h>
#include <stdlib.h>
#include <stdio.h>
#include <windows.h>
#include <conio.h>
#include <process.h>


void main( int argc, char *argv[] )
{	
	char achBuf[MAX_PATH];
	char *p;
	int setup;

	// Assume we are not running setup
	setup = 0;
		
	// Default : System version
	GetSystemDirectoryA(achBuf, MAX_PATH);
	strcat(achBuf, "\\mshtml.dll");	

	// Check if we have a second parameter
	if (argc >= 2)
	{
		if (strcmp(argv[1],"setup") != 0)
		{
			// Use second parameter for path
			strcpy(achBuf, argv[1]);
		}
		else
		{
			//otherwise for 'HTMLPath setup' register system and set flag
			setup = 1;
		}
	}
		 	
	printf("Setting HTML engine path: %s\n",achBuf);

	//Register Pad to this path
	RegSetValueA(HKEY_CLASSES_ROOT, 
            "CLSID\\{25336920-03F9-11cf-8FD0-00AA00686F13}\\InprocServer32", 
            REG_SZ,
            achBuf,
            0);
	
	// if flag is set above
	if (setup == 1)
	{
		//_getcwd(achBuf, MAX_PATH);
		
		strcpy(achBuf, argv[0]);
				
		p=strrchr(achBuf, '\\');
		if (p>0)
		{
			p[0] = '\0';
		}
		strcat(achBuf, "\\setup.exe");
		printf("Launching %s\n",achBuf);
		_execl( achBuf,"setup.exe", "/ua", NULL );
	}
	return;
}

#include <io.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libc/file.h>

FILE *
_popen (const char *cm, const char *md) /* program name, pipe mode */
{
  FILE *pf;
  HANDLE hReadPipe, hWritePipe;
  HANDLE SpawnedProcess;
  STARTUPINFO StartupInfo;
  PROCESS_INFORMATION ProcessInformation;
  if ( !CreatePipe(&hReadPipe,&hWritePipe,NULL,1024,))
		return NULL;	

  StartupInfo.cb = sizeof(STARTUPINFO);
  if ( md == "r" ) {
	StartupInfo.hStdOutput = hWritePipe;
  }
  else if ( md == "w" ) {
	StartupInfo.hStdInput = hReadPipe;
  }
	
  SpawnedProcess = CreateProcess("cmd.exe",cm,NULL,NULL,TRUE,CREATE_NEW_CONSOLE,NULL,NULL,&StartupInfo,&ProcessInformation );


  if ( *md == 'r' ) {
	pf =  _fdopen( __fileno_alloc(hReadPipe,  _fmode) , "r" );
  }
  else {
	pf =  _fdopen( __fileno_alloc(hWritePipe, _fmode) , "w" );
  }

  pf->name_to_remove = SpawnedProcess; 

  return pf;
	

}
FILE*	_wpopen (const wchar_t* szPipeName, const wchar_t* szMode)
{
 	return NULL;
}

int
pclose (FILE *pp)
{
	
 	fclose(pp);
	if (!TerminateProcess(pp->name_to_remove,0))
		return -1;
	return 0;
}

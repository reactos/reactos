
#include <crtdll/io.h>
#include <crtdll/errno.h>
#include <crtdll/stdio.h>
#include <crtdll/stdlib.h>
#include <crtdll/string.h>
#include <crtdll/internal/file.h>

FILE *
_popen (const char *cm, const char *md) /* program name, pipe mode */
{
  FILE *pf;
  HANDLE hReadPipe, hWritePipe;
  HANDLE SpawnedProcess;
  STARTUPINFO StartupInfo;
  PROCESS_INFORMATION ProcessInformation;

  // fixme CreatePipe

//  if ( !CreatePipe(&hReadPipe,&hWritePipe,NULL,1024))
//		return NULL;	

  StartupInfo.cb = sizeof(STARTUPINFO);
  if ( md == "r" ) {
	StartupInfo.hStdOutput = hWritePipe;
  }
  else if ( md == "w" ) {
	StartupInfo.hStdInput = hReadPipe;
  }
	
  SpawnedProcess = CreateProcessA("cmd.exe",(char *)cm,NULL,NULL,TRUE,CREATE_NEW_CONSOLE,NULL,NULL,&StartupInfo,&ProcessInformation );


  if ( *md == 'r' ) {
	pf =  _fdopen( __fileno_alloc(hReadPipe,  _fmode) , "r" );
  }
  else {
	pf =  _fdopen( __fileno_alloc(hWritePipe, _fmode) , "w" );
  }

  pf->_name_to_remove = SpawnedProcess; 

  return pf;
	

}


int
_pclose (FILE *pp)
{
	
 	fclose(pp);
	printf("Terminate Process\n");
//	if (!TerminateProcess(pp->_name_to_remove,0))
//		return -1;
	return 0;
}


#include <windows.h>
#include <msvcrt/io.h>
#include <msvcrt/errno.h>
#include <msvcrt/stdio.h>
#include <msvcrt/stdlib.h>
#include <msvcrt/string.h>
#include <msvcrt/internal/file.h>


FILE *_popen (const char *cm, const char *md) /* program name, pipe mode */
{
  FILE *pf;
  HANDLE hReadPipe, hWritePipe;
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
	
  if (CreateProcessA("cmd.exe",(char *)cm,NULL,NULL,TRUE,
                     CREATE_NEW_CONSOLE,NULL,NULL,
                     &StartupInfo,
                     &ProcessInformation) == FALSE)
    return NULL;


  if ( *md == 'r' ) {
	pf =  _fdopen( __fileno_alloc(hReadPipe,  _fmode) , "r" );
  }
  else {
	pf =  _fdopen( __fileno_alloc(hWritePipe, _fmode) , "w" );
  }

  pf->_name_to_remove = ProcessInformation.hProcess;

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

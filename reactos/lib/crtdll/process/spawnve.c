#include <process.h>
#include <string.h>
#include <windows.h>
#include <stdio.h>


int _p_overlay = 2;

int _spawnve(int mode, const char *path,const char *const argv[],const char *const envp[])
{

  char ApplicationName[MAX_PATH];
  char CommandLine[1024];
  PROCESS_INFORMATION ProcessInformation;
  STARTUPINFO StartupInfo;
  
  int i = 0;
  CommandLine[0] = 0;
  while(argv[i] != NULL ) {
	strcat(CommandLine,argv[i]);
	strcat(CommandLine," ");
  	i++; 
  }
  strcpy(ApplicationName,argv[0]);
 
  fflush(stdout); /* just in case */
  StartupInfo.cb = sizeof(STARTUPINFO);
  StartupInfo.lpReserved= NULL;
  StartupInfo.dwFlags = 0;


//  if ( CreateProcessA(ApplicationName,CommandLine,NULL,NULL,TRUE,CREATE_NEW_CONSOLE|NORMAL_PRIORITY_CLASS,NULL,*envp,&StartupInfo,&ProcessInformation) ) {
//	errno = GetLastError();
//	return -1;
//  }

  
  if (mode == P_OVERLAY)
    _exit(i);

// _P_NOWAIT or _P_NOWAITO 
  return ProcessInformation.hProcess;
}

#include <crtdll/process.h>
#include <crtdll/string.h>
#include <windows.h>
#include <crtdll/stdio.h>


int _p_overlay = 2;

int _spawnve(int nMode, const char* szPath, char* const* szaArgv, char* const* szaEnv)
{

  char ApplicationName[MAX_PATH];
  char CommandLine[1024];
  PROCESS_INFORMATION ProcessInformation;
  STARTUPINFO StartupInfo;
  
  int i = 0;
  CommandLine[0] = 0;
  while(szaArgv[i] != NULL ) {
	strcat(CommandLine,szaArgv[i]);
	strcat(CommandLine," ");
  	i++; 
  }
  strcpy(ApplicationName,szaArgv[0]);
 
  fflush(stdout); /* just in case */
  StartupInfo.cb = sizeof(STARTUPINFO);
  StartupInfo.lpReserved= NULL;
  StartupInfo.dwFlags = 0;


  if ( CreateProcessA(ApplicationName,CommandLine,NULL,NULL,TRUE,CREATE_NEW_CONSOLE|NORMAL_PRIORITY_CLASS,NULL,*szaEnv,&StartupInfo,&ProcessInformation) ) {
	return -1;
  }

  
 // if (nMode == P_OVERLAY)
 //   _exit(i);

// _P_NOWAIT or _P_NOWAITO 
  return (int )ProcessInformation.hProcess;
}

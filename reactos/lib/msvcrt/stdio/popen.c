
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
  STARTUPINFOA StartupInfo;
  PROCESS_INFORMATION ProcessInformation;

  // fixme CreatePipe

//  if ( !CreatePipe(&hReadPipe,&hWritePipe,NULL,1024))
//		return NULL;	

  StartupInfo.cb = sizeof(STARTUPINFOA);
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

  if ( *md == 'r' )
    {
      pf = _fdopen(__fileno_alloc(hReadPipe,  _fmode) , "r");
    }
  else
    {
      pf = _fdopen( __fileno_alloc(hWritePipe, _fmode) , "w");
    }

  pf->_name_to_remove = ProcessInformation.hProcess;

  return pf;
}


int _pclose (FILE *pp)
{
  fclose(pp);
  printf("Terminate Process\n");
//  if (!TerminateProcess(pp->_name_to_remove,0))
//    return -1;
  return 0;
}


FILE *_wpopen (const wchar_t *cm, const wchar_t *md) /* program name, pipe mode */
{
  FILE *pf;
  HANDLE hReadPipe, hWritePipe;
  STARTUPINFOW StartupInfo;
  PROCESS_INFORMATION ProcessInformation;

  // fixme CreatePipe

//  if ( !CreatePipe(&hReadPipe,&hWritePipe,NULL,1024))
//		return NULL;	

  StartupInfo.cb = sizeof(STARTUPINFOW);
  if (*md == L'r')
  {
	StartupInfo.hStdOutput = hWritePipe;
  }
  else if (*md == L'w')
  {
	StartupInfo.hStdInput = hReadPipe;
  }
	
  if (CreateProcessW(L"cmd.exe",(wchar_t *)cm,NULL,NULL,TRUE,
                     CREATE_NEW_CONSOLE,NULL,NULL,
                     &StartupInfo,
                     &ProcessInformation) == FALSE)
    return NULL;

  if (*md == L'r')
    {
      pf = _wfdopen(__fileno_alloc(hReadPipe,  _fmode) , L"r");
    }
  else
    {
      pf = _wfdopen( __fileno_alloc(hWritePipe, _fmode) , L"w");
    }

  pf->_name_to_remove = ProcessInformation.hProcess;

  return pf;
}

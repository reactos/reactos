/* $Id: popen.c,v 1.5 2003/07/11 21:58:09 royce Exp $ */
#include <windows.h>
#include <msvcrt/io.h>
#include <msvcrt/errno.h>
#include <msvcrt/stdio.h>
#include <msvcrt/stdlib.h>
#include <msvcrt/string.h>
#include <msvcrt/internal/file.h>
#define NDEBUG
#include <msvcrt/msvcrtdbg.h>


/*
 * @implemented
 */
FILE *_popen (const char *cm, const char *md) /* program name, pipe mode */
{
  char *szCmdLine=NULL;
  char *szComSpec=NULL;
  char *s;
  FILE *pf;
  HANDLE hReadPipe, hWritePipe;
  BOOL result;
  STARTUPINFOA StartupInfo;
  PROCESS_INFORMATION ProcessInformation;
  SECURITY_ATTRIBUTES sa = {sizeof(SECURITY_ATTRIBUTES), NULL, TRUE};

  DPRINT("_popen('%s', '%s')\n", cm, md);

  if (cm == NULL)
    return NULL;

  szComSpec = getenv("COMSPEC");

  if (szComSpec == NULL)
  {
    szComSpec = strdup("cmd.exe");
    if (szComSpec == NULL)
      return NULL;
  }

  s = max(strrchr(szComSpec, '\\'), strrchr(szComSpec, '/'));
  if (s == NULL)
    s = szComSpec;
  else
    s++;

  szCmdLine = malloc(strlen(s) + 4 + strlen(cm) + 1);
  if (szCmdLine == NULL)
  {
    free (szComSpec);
    return NULL;
  }
  
  strcpy(szCmdLine, s);
  s = strrchr(szCmdLine, '.');
  if (s)
    *s = 0;
  strcat(szCmdLine, " /C ");
  strcat(szCmdLine, cm);

  if ( !CreatePipe(&hReadPipe,&hWritePipe,&sa,1024))
  {
    free (szComSpec);
    free (szCmdLine);
    return NULL;
  }

  memset(&StartupInfo, 0, sizeof(STARTUPINFOA));
  StartupInfo.cb = sizeof(STARTUPINFOA);

  if (*md == 'r' ) {
	StartupInfo.hStdOutput = hWritePipe;
	StartupInfo.dwFlags |= STARTF_USESTDHANDLES;
  }
  else if ( *md == 'w' ) {
	StartupInfo.hStdInput = hReadPipe;
	StartupInfo.dwFlags |= STARTF_USESTDHANDLES;
  }
	
  result = CreateProcessA(szComSpec,
	                  szCmdLine,
			  NULL,
			  NULL,
			  TRUE,
			  0,
			  NULL,
			  NULL,
			  &StartupInfo,
			  &ProcessInformation);
  free (szComSpec);
  free (szCmdLine);

  if (result == FALSE)
  {
    CloseHandle(hReadPipe);
    CloseHandle(hWritePipe);
    return NULL;
  }

  CloseHandle(ProcessInformation.hThread);

  if ( *md == 'r' )
    {
      pf = _fdopen(__fileno_alloc(hReadPipe,  _fmode) , "r");
      CloseHandle(hWritePipe);
    }
  else
    {
      pf = _fdopen( __fileno_alloc(hWritePipe, _fmode) , "w");
      CloseHandle(hReadPipe);
    }

  pf->_name_to_remove = ProcessInformation.hProcess;

  return pf;
}


/*
 * @implemented
 */
int _pclose (FILE *pp)
{
  fclose(pp);
  if (!TerminateProcess(pp->_name_to_remove,0))
    return -1;
  return 0;
}


/*
 * @implemented
 */
FILE *_wpopen (const wchar_t *cm, const wchar_t *md) /* program name, pipe mode */
{
  wchar_t *szCmdLine=NULL;
  wchar_t *szComSpec=NULL;
  wchar_t *s;
  FILE *pf;
  HANDLE hReadPipe, hWritePipe;
  BOOL result;
  STARTUPINFOW StartupInfo;
  PROCESS_INFORMATION ProcessInformation;
  SECURITY_ATTRIBUTES sa = {sizeof(SECURITY_ATTRIBUTES), NULL, TRUE};

  DPRINT("_wpopen('%S', '%S')\n", cm, md);

  if (cm == NULL)
    return NULL;

  szComSpec = _wgetenv(L"COMSPEC");

  if (szComSpec == NULL)
  {
    szComSpec = _wcsdup(L"cmd.exe");
    if (szComSpec == NULL)
      return NULL;
  }

  s = max(wcsrchr(szComSpec, L'\\'), wcsrchr(szComSpec, L'/'));
  if (s == NULL)
    s = szComSpec;
  else
    s++;

  szCmdLine = malloc((wcslen(s) + 4 + wcslen(cm) + 1) * sizeof(wchar_t));
  if (szCmdLine == NULL)
  {
    free (szComSpec);
    return NULL;
  }
  
  wcscpy(szCmdLine, s);
  s = wcsrchr(szCmdLine, L'.');
  if (s)
    *s = 0;
  wcscat(szCmdLine, L" /C ");
  wcscat(szCmdLine, cm);

  if ( !CreatePipe(&hReadPipe,&hWritePipe,&sa,1024))
  {
    free (szComSpec);
    free (szCmdLine);
    return NULL;
  }

  memset(&StartupInfo, 0, sizeof(STARTUPINFOW));
  StartupInfo.cb = sizeof(STARTUPINFOW);

  if (*md == L'r' ) {
	StartupInfo.hStdOutput = hWritePipe;
	StartupInfo.dwFlags |= STARTF_USESTDHANDLES;
  }
  else if ( *md == L'w' ) {
	StartupInfo.hStdInput = hReadPipe;
	StartupInfo.dwFlags |= STARTF_USESTDHANDLES;
  }
	
  result = CreateProcessW(szComSpec,
	                  szCmdLine,
			  NULL,
			  NULL,
			  TRUE,
			  0,
			  NULL,
			  NULL,
			  &StartupInfo,
			  &ProcessInformation);
  free (szComSpec);
  free (szCmdLine);

  if (result == FALSE)
  {
    CloseHandle(hReadPipe);
    CloseHandle(hWritePipe);
    return NULL;
  }

  CloseHandle(ProcessInformation.hThread);

  if ( *md == L'r' )
    {
      pf = _wfdopen(__fileno_alloc(hReadPipe,  _fmode) , L"r");
      CloseHandle(hWritePipe);
    }
  else
    {
      pf = _wfdopen( __fileno_alloc(hWritePipe, _fmode) , L"w");
      CloseHandle(hReadPipe);
    }

  pf->_name_to_remove = ProcessInformation.hProcess;

  return pf;
}

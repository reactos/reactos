/* Copyright (C) 1998 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1996 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */

#include <windows.h>

#include <msvcrt/stdio.h>
#include <msvcrt/stdlib.h>
#include <msvcrt/string.h>
#include <msvcrt/errno.h>
//#include <msvcrt/limits.h>
#include <msvcrt/process.h>
#include <msvcrt/ctype.h>
#include <msvcrt/io.h>

#define NDEBUG
#include <msvcrt/msvcrtdbg.h>


#ifndef F_OK
 #define F_OK	0x01
#endif
#ifndef R_OK
 #define R_OK	0x02
#endif
#ifndef W_OK
 #define W_OK	0x04
#endif
#ifndef X_OK
 #define X_OK	0x08
#endif
#ifndef D_OK
 #define D_OK	0x10
#endif

// information about crtdll file handles is not passed to child
int _fileinfo_dll = 0;

static int
direct_exec_tail(const char *program, const char *args,
		 const char * envp,
		PROCESS_INFORMATION *ProcessInformation)
{

	static STARTUPINFO StartupInfo;

	StartupInfo.cb = sizeof(STARTUPINFO);
	StartupInfo.lpReserved= NULL;
	StartupInfo.dwFlags = 0 /*STARTF_USESTDHANDLES*/;
	StartupInfo.wShowWindow = SW_SHOWDEFAULT; 
	StartupInfo.lpReserved2 = NULL;
	StartupInfo.cbReserved2 = 0; 
	StartupInfo.hStdInput = _get_osfhandle(0);
	StartupInfo.hStdOutput = _get_osfhandle(1);
	StartupInfo.hStdError = _get_osfhandle(2);


	if (! CreateProcessA((char *)program,(char *)args,NULL,NULL,TRUE,0,(LPVOID)envp,NULL,&StartupInfo,ProcessInformation) ) 
	{
	  __set_errno( GetLastError() );
          return -1;
	}

	return (int)ProcessInformation->hProcess;
}

static int vdm_exec(const char *program, char **argv, char **envp,
	PROCESS_INFORMATION *ProcessInformation)
{
	static char args[1024];
	int i = 0;
	args[0] = 0;

	strcpy(args,"vdm.exe ");
	while(argv[i] != NULL ) {
        strcat(args,argv[i]);
        strcat(args," ");
        i++; 
	}
  
	return direct_exec_tail(program,args,envp,ProcessInformation);
}

static int go32_exec(const char *program, char **argv, char **envp,
	PROCESS_INFORMATION *ProcessInformation)
{
	char * penvblock, * ptr;
	char * args;
	int i, len, result;

	for (i = 0, len = 0; envp[i]; i++) {
	  len += strlen(envp[i]) + 1;
	}
	penvblock = ptr = (char*)malloc(len + 1);
	if (penvblock == NULL)
	  return -1;

	for(i = 0, *ptr = 0; envp[i]; i++) {
	   strcpy(ptr, envp[i]);
	   ptr += strlen(envp[i]) + 1;
	}
	*ptr = 0;

	for(i = 0, len = 0; argv[i]; i++) {
	  len += strlen(argv[i]) + 1;
	}
	
	args = (char*) malloc(len + 1);
	if (args == NULL)
	{
	  free(penvblock);
	  return -1;
	}

	for(i = 0, *args = 0; argv[i]; i++) {
          strcat(args,argv[i]);
	  if (argv[i+1] != NULL) {
            strcat(args," ");
	  }
	}
  
	result = direct_exec_tail(program,args,(const char*)penvblock,ProcessInformation);
	free(args);
	free(penvblock);
	return result;
}

int
command_exec(const char *program, char **argv, char **envp,
	PROCESS_INFORMATION *ProcessInformation)
{
 	static char args[1024];
	int i = 0;



	args[0] = 0;

	strcpy(args,"cmd.exe  /c ");
	while(argv[i] != NULL ) {
        strcat(args,argv[i]);
        strcat(args," ");
        i++; 
	}
  
	return direct_exec_tail(program,args,envp,ProcessInformation);

}

static int script_exec(const char *program, char **argv, char **envp,
	PROCESS_INFORMATION *ProcessInformation)
{
	return 0;
}


/* Note: the following list is not supposed to mention *every*
   possible extension of an executable file.  It only mentions
   those extensions that can be *omitted* when you invoke the
   executable from one of the shells used on MSDOS.  */
static struct {
  const char *extension;
  int (*interp)(const char *, char **, char **,
	PROCESS_INFORMATION *);
} interpreters[] = {
	{ ".com", vdm_exec },
	{ ".exe", go32_exec },
	{ ".dll", go32_exec },
	{ ".cmd", command_exec },
	{ ".bat", command_exec },
	{ ".btm", command_exec },
	{ ".sh",  script_exec },  /* for compatibility with ms_sh */
	{ ".ksh", script_exec },
	{ ".pl", script_exec },   /* Perl */
	{ ".sed", script_exec },
	{ "",     go32_exec },
	{ 0,      script_exec },  /* every extension not mentioned above calls it */
	{ 0,      0 },
};

/* This is the index into the above array of the interpreter
   which is called when the program filename has no extension.  */
#define INTERP_NO_EXT (sizeof(interpreters)/sizeof(interpreters[0]) - 3)

/*-------------------------------------------------*/




int _spawnve(int mode, const char *path, char *const argv[], char *const envp[])
{
  /* This is the one that does the work! */
  PROCESS_INFORMATION ProcessInformation;
  union { char *const *x; char **p; } u;
  int i = -1;
  char **argvp;
  char **envpp;
  char rpath[FILENAME_MAX], *rp, *rd=0;
  int e = errno;
  int is_dir = 0;
  int found = 0;
  DWORD ExitCode;

  DPRINT("_spawnve('%s')\n", path);

  if (path == 0 || argv[0] == 0)
  {
    errno = EINVAL;
    DPRINT("??\n");
    return -1;
  }
  if (strlen(path) > FILENAME_MAX - 1)
  {
    errno = ENAMETOOLONG;
    DPRINT("??\n");
    return -1;
  }

  u.x = argv; argvp = u.p;
  u.x = envp; envpp = u.p;

  fflush(stdout); /* just in case */
  for (rp=rpath; *path; *rp++ = *path++)
  {
    if (*path == '.')
      rd = rp;
    if (*path == '\\' || *path == '/')
      rd = 0;
  }
  *rp = 0;

  /* If LFN is supported on the volume where rpath resides, we
     might have something like foo.bar.exe or even foo.exe.com.
     If so, look for RPATH.ext before even trying RPATH itself. */
  if (!rd)
  {
    for (i=0; interpreters[i].extension; i++)
    {
      strcpy(rp, interpreters[i].extension);
      if (_access(rpath, F_OK) == 0 && !(is_dir = (_access(rpath, D_OK) == 0)))
      {
	found = 1;
	break;
      }
    }
  }

  if (!found)
  {
    const char *rpath_ext;

    if (rd)
    {
      i = 0;
      rpath_ext = rd;
    }
    else
    {
      i = INTERP_NO_EXT;
      rpath_ext = "";
    }
    for ( ; interpreters[i].extension; i++)
      if (_stricmp(rpath_ext, interpreters[i].extension) == 0
	  && _access(rpath, F_OK) == 0
	  && !(is_dir = (_access(rpath, D_OK) == 0)))
      {
	found = 1;
        break;
      }
  }
  if (!found)
  {
    DPRINT("??\n");
    errno = is_dir ? EISDIR : ENOENT;
    return -1;
  }
  errno = e;
  i = interpreters[i].interp(rpath, argvp, envpp, &ProcessInformation);
  if (mode == P_OVERLAY)
    exit(i);
  if (mode == P_WAIT)
  {
    WaitForSingleObject(ProcessInformation.hProcess,INFINITE);
    GetExitCodeProcess(ProcessInformation.hProcess,&ExitCode);
    i = (int)ExitCode;
    CloseHandle(ProcessInformation.hThread);
    CloseHandle(ProcessInformation.hProcess);
  }
  else
  {
     CloseHandle(ProcessInformation.hThread);
  }
  return i;
}




const char * find_exec(char * path,char *rpath)
{
 char *rp, *rd=0;
 int i;
 int is_dir = 0;
 int found = 0;
  if (path == 0 )
    return 0;
  if (strlen(path) > FILENAME_MAX - 1)
    return path;

  /* copy path in rpath */
  for (rd=path,rp=rpath; *rd; *rp++ = *rd++)
    ;
  *rp = 0;
  /* try first with the name as is */
  for (i=0; interpreters[i].extension; i++)
  {
    strcpy(rp, interpreters[i].extension);
    if (_access(rpath, F_OK) == 0 && !(is_dir = (_access(rpath, D_OK) == 0)))
    {
	found = 1;
	break;
    }
  }

  if (!found)
  {
    /* search in the PATH */
    char winpath[MAX_PATH];
    if( GetEnvironmentVariableA("PATH",winpath,MAX_PATH))
    {
     char *ep=winpath;
      while( *ep)
      {
        if(*ep == ';') ep++;
        rp=rpath;
        for ( ; *ep && (*ep != ';') ; *rp++ = *ep++)
	  ;
        *rp++='/';
        for (rd=path ; *rd ; *rp++ = *rd++)
	  ;

        for (i=0; interpreters[i].extension; i++)
        {
          strcpy(rp, interpreters[i].extension);
          if (_access(rpath, F_OK) == 0 && !(is_dir = (_access(rpath, D_OK) == 0)))
          {
	    found = 1;
	    break;
          }
        }
	if (found) break;
      }
    }
  }
  if (!found)
    return path;

  return rpath;
}

int _spawnvpe(int nMode, const char* szPath, char* const* szaArgv, char* const* szaEnv)
{
 char rpath[FILENAME_MAX];

  return _spawnve(nMode, find_exec((char*)szPath,rpath), szaArgv, szaEnv);

}

/* Copyright (C) 1998 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1996 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */

#include <windows.h>

#include <crtdll/stdio.h>
#include <crtdll/stdlib.h>
#include <crtdll/string.h>
#include <crtdll/errno.h>
//#include <crtdll/limits.h>
#include <crtdll/process.h>
#include <crtdll/ctype.h>
#include <crtdll/io.h>


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
		 char * const envp[])
{

	static PROCESS_INFORMATION ProcessInformation;
	static STARTUPINFO StartupInfo;

	StartupInfo.cb = sizeof(STARTUPINFO);
	StartupInfo.lpReserved= NULL;
	StartupInfo.dwFlags = 0;
	StartupInfo.wShowWindow = SW_SHOWDEFAULT; 
	StartupInfo.lpReserved2 = NULL;
	StartupInfo.cbReserved2 = 0; 


	if ( CreateProcessA((char *)program,(char *)args,NULL,NULL,FALSE,0,(char **)envp,NULL,&StartupInfo,&ProcessInformation) ) {
        return -1;
	}

	return ProcessInformation.dwProcessId;
}

static int vdm_exec(const char *program, char **argv, char **envp)
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
  
	return direct_exec_tail(program,args,envp);
}

static int go32_exec(const char *program, char **argv, char **envp)
{


	static char args[1024];
	int i = 0;


	args[0] = 0;

	while(argv[i] != NULL ) {
        strcat(args,envp[i]);
        strcat(args," ");
        i++; 
	}
    printf("%s \n %s\n",args, GetEnvironmentStrings());
	args[0] = 0;
    i = 0;
	while(argv[i] != NULL ) {
        strcat(args,argv[i]);
        strcat(args," ");
        i++; 
	}
  
	return direct_exec_tail(program,args,envp);
}

int
command_exec(const char *program, char **argv, char **envp)
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
  
	return direct_exec_tail(program,args,envp);

}

static int script_exec(const char *program, char **argv, char **envp)
{
	return 0;
}


/* Note: the following list is not supposed to mention *every*
   possible extension of an executable file.  It only mentions
   those extensions that can be *omitted* when you invoke the
   executable from one of the shells used on MSDOS.  */
static struct {
  const char *extension;
  int (*interp)(const char *, char **, char **);
} interpreters[] = {
	{ ".com", vdm_exec },
	{ ".exe", go32_exec },
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
  union { char *const *x; char **p; } u;
  int i = -1;
  char **argvp;
  char **envpp;
  char rpath[FILENAME_MAX], *rp, *rd=0;
  int e = errno;
  int is_dir = 0;
  int found = 0;

  if (path == 0 || argv[0] == 0)
  {
    errno = EINVAL;
    return -1;
  }
  if (strlen(path) > FILENAME_MAX - 1)
  {
    errno = ENAMETOOLONG;
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
    errno = is_dir ? EISDIR : ENOENT;
    return -1;
  }
  errno = e;
  i = interpreters[i].interp(rpath, argvp, envpp);
  if (mode == P_OVERLAY)
    exit(i);
  return i;
}






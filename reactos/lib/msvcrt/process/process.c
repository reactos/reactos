/* $Id: process.c,v 1.4 2003/03/23 15:18:01 hbirr Exp $ */
#include <msvcrt/process.h>
#include <msvcrt/stdlib.h>
#include <msvcrt/string.h>
#include <msvcrt/errno.h>
#include <msvcrt/internal/file.h>

#define NDEBUG
#include <msvcrt/msvcrtdbg.h>

extern int maxfno;

char const* ext[] =
{
    "",
    ".bat",
    ".cmd",
    ".com",
    ".exe"
};

const char* find_exec(const char* path, char* rpath)
{
    char *rp;
    const char *rd; 
    int i, found = 0;

    DPRINT("find_exec('%s', %x)\n", path, rpath);

    if (path == NULL)
    {
	return NULL;
    }
    if (strlen(path) > FILENAME_MAX - 1)
    {
	return path;
    }
    /* copy path in rpath */
    for (rd = path, rp = rpath; *rd; *rp++ = *rd++);
    *rp = 0;
    /* try first with the name as is */
    for (i = 0; i < sizeof(ext) / sizeof(*ext); i++)
    {
	strcpy(rp, ext[i]);

	DPRINT("trying '%s'\n", rpath);

	if (_access(rpath, F_OK) == 0 && _access(rpath, D_OK) != 0)
	{
	    found = 1;
	    break;
	}
    }
    if (!found)
    {
	char* env = getenv("PATH");
	if (env)
	{
	    char* ep = env;
	    while (*ep && !found)
	    {
	       if (*ep == ';') ep++;
	       rp=rpath;
	       for (; *ep && (*ep != ';'); *rp++ = *ep++);
	       if (rp > rpath)
	       {
		  rp--;
		  if (*rp != '/' && *rp != '\\')
		  {
		     *++rp = '\\';
		  }
		  rp++;
	       }
	       for (rd=path; *rd; *rp++ = *rd++);
	       for (i = 0; i < sizeof(ext) / sizeof(*ext); i++)
	       {
		  strcpy(rp, ext[i]);

		  DPRINT("trying '%s'\n", rpath);

		  if (_access(rpath, F_OK) == 0 && _access(rpath, D_OK) != 0)
		  {
	             found = 1;
	             break;
		  }
	       }
	    }
	    free(env);
	}
    }
    
    return found ? rpath : path;
}

static char* 
argvtos(char* const* argv, char delim)
{
    int i, len;
    char *ptr, *str;

    if (argv == NULL)
	return NULL;

    for (i = 0, len = 0; argv[i]; i++) 
    {
	len += strlen(argv[i]) + 1;
    }

    str = ptr = (char*) malloc(len + 1);
    if (str == NULL)
	return NULL;

    for(i = 0; argv[i]; i++) 
    {
	len = strlen(argv[i]);
	memcpy(ptr, argv[i], len);
	ptr += len;
	*ptr++ = delim;
    }
    *ptr = 0;

    return str;
}

static char* 
valisttos(const char* arg0, va_list alist, char delim)
{
    va_list alist2 = alist;
    char *ptr, *str;
    int len;

    if (arg0 == NULL)
	return NULL;

    ptr = (char*)arg0;
    len = 0;
    do
    {
	len += strlen(ptr) + 1;
	ptr = va_arg(alist, char*);
    }
    while(ptr != NULL);

    str = (char*) malloc(len + 1);
    if (str == NULL)
	return NULL;

    ptr = str;
    do
    {
	len = strlen(arg0);
	memcpy(ptr, arg0, len);
	ptr += len;
	*ptr++ = delim;
	arg0 = va_arg(alist2, char*);
    }
    while(arg0 != NULL);
    *ptr = 0;

    return str;
}

static int
do_spawn(int mode, const char* cmdname, const char* args, const char* envp)
{
    STARTUPINFO StartupInfo;
    PROCESS_INFORMATION ProcessInformation;
    char* fmode;
    HANDLE* hFile;
    int i, last;
    BOOL bResult;
    DWORD dwExitCode;
    DWORD dwError;

    DPRINT("do_spawn('%s')\n", cmdname);

    if (mode != _P_NOWAIT && mode != _P_NOWAITO && mode != _P_WAIT && mode != _P_DETACH && mode != _P_OVERLAY)
    {
       errno = EINVAL;
       return -1;
    }

    if (0 != _access(cmdname, F_OK))
    {
	errno = ENOENT;
	return -1;
    }
    if (0 == _access(cmdname, D_OK))
    {
	errno = EISDIR;
	return -1;
    }

    memset (&StartupInfo, 0, sizeof(STARTUPINFO));
    StartupInfo.cb = sizeof(STARTUPINFO);

    for (last = i = 0; i < maxfno; i++)
    {
	if ((void*)-1 != _get_osfhandle(i))
	{
	    last = i + 1;
	}
    }

    if (last)
    {
	StartupInfo.cbReserved2 = sizeof(ULONG) + last * (sizeof(char) + sizeof(HANDLE));
	StartupInfo.lpReserved2 = malloc(StartupInfo.cbReserved2);
	if (StartupInfo.lpReserved2 == NULL)
	{
	    errno = ENOMEM;
	    return -1;
	} 

	*(DWORD*)StartupInfo.lpReserved2 = last;
	fmode = (char*)(StartupInfo.lpReserved2 + sizeof(ULONG));
	hFile = (HANDLE*)(StartupInfo.lpReserved2 + sizeof(ULONG) + last * sizeof(char));
	for (i = 0; i < last; i++)
	{
	    int _mode = __fileno_getmode(i);
	    HANDLE h = _get_osfhandle(i);
	    /* FIXME: The test of console handles (((ULONG)Handle) & 0x10000003) == 0x3) 
	     *        is possible wrong 
	     */
	    if ((((ULONG)h) & 0x10000003) == 0x3 || _mode & _O_NOINHERIT || (i < 3 && mode == _P_DETACH))
	    {
		*hFile = INVALID_HANDLE_VALUE;
		*fmode = 0;
	    }
	    else
	    {
		DWORD dwFlags;
		BOOL bFlag;
		bFlag = GetHandleInformation(h, &dwFlags);
		if (bFlag && (dwFlags & HANDLE_FLAG_INHERIT))
		{
	            *hFile = h;
	            *fmode = (_O_ACCMODE & _mode) | (((_O_TEXT | _O_BINARY) & _mode) >> 8);
		}
		else
		{
		    *hFile = INVALID_HANDLE_VALUE;
		    *fmode = 0;
		}
	    }
	    fmode++;
	    hFile++;
	}
    }

    bResult = CreateProcessA((char *)cmdname,
		             (char *)args,
			     NULL,
			     NULL,
			     TRUE,
			     mode == _P_DETACH ? DETACHED_PROCESS : 0,
			     (LPVOID)envp,
			     NULL,
			     &StartupInfo,
			     &ProcessInformation);
    if (StartupInfo.lpReserved2)
    {
        free(StartupInfo.lpReserved2);
    }

    if (!bResult) 
    {
	dwError = GetLastError();
	DPRINT("%x\n", dwError);
	__set_errno(dwError);
	return -1;
    }
    CloseHandle(ProcessInformation.hThread);
    switch(mode)
    {
	case _P_OVERLAY:
	    _exit(0);
	case _P_WAIT:
	    WaitForSingleObject(ProcessInformation.hProcess, INFINITE);
	    GetExitCodeProcess(ProcessInformation.hProcess, &dwExitCode);
	    CloseHandle(ProcessInformation.hProcess);
	    return (int)dwExitCode;
	case _P_DETACH:
	    CloseHandle(ProcessInformation.hProcess);
	    return 0;
    }
    return (int)ProcessInformation.hProcess;
}

int _spawnl(int mode, const char *cmdname, const char* arg0, ...)
{
    va_list argp;
    char* args;
    int ret = -1;

    DPRINT("_spawnl('%s')\n", cmdname);

    va_start(argp, arg0);
    args = valisttos(arg0, argp, ' ');

    if (args)
    {
	ret = do_spawn(mode, cmdname, args, NULL);
	free(args);
    }
    return ret;
}

int _spawnv(int mode, const char *cmdname, char* const* argv)
{
    char* args;
    int ret = -1;

    DPRINT("_spawnv('%s')\n", cmdname);

    args = argvtos(argv, ' ');

    if (args)
    {
	ret = do_spawn(mode, cmdname, args, NULL);
	free(args);
    }
    return ret;
}

int _spawnle(int mode, const char *cmdname, const char* arg0, ... /*, NULL, const char* const* envp*/)
{
    va_list argp;
    char* args;
    char* envs;
    char* const* ptr;
    int ret = -1;

    DPRINT("_spawnle('%s')\n", cmdname);

    va_start(argp, arg0);
    args = valisttos(arg0, argp, ' ');
    do
    {
	ptr = (char* const*)va_arg(argp, char*);
    }
    while (ptr != NULL);
    envs = argvtos(ptr, 0);
    if (args)
    {
	ret = do_spawn(mode, cmdname, args, envs);
	free(args);
    }
    if (envs)
    {
	free(envs);
    }
    return ret;
    
}

int _spawnve(int mode, const char *cmdname, char* const* argv, char* const* envp)
{
    char *args;
    char *envs;
    int ret = -1;

    DPRINT("_spawnve('%s')\n", cmdname);

    args = argvtos(argv, ' ');
    envs = argvtos(envp, 0);

    if (args)
    {
	ret = do_spawn(mode, cmdname, args, envs);
	free(args);
    }
    if (envs)
    {
	free(envs);
    }
    return ret;
}

int _spawnvp(int mode, const char* cmdname, char* const* argv)
{
    char pathname[FILENAME_MAX];
  
    DPRINT("_spawnvp('%s')\n", cmdname);

    return _spawnv(mode, find_exec(cmdname, pathname), argv);
}

int _spawnlp(int mode, const char* cmdname, const char* arg0, .../*, NULL*/)
{
    va_list argp;
    char* args;
    int ret = -1;
    char pathname[FILENAME_MAX];

    DPRINT("_spawnlp('%s')\n", cmdname);

    va_start(argp, arg0);
    args = valisttos(arg0, argp, ' ');
    if (args)
    {
	ret = do_spawn(mode, find_exec(cmdname, pathname), args, NULL);
	free(args);
    }
    return ret;
}


int _spawnlpe(int mode, const char* cmdname, const char* arg0, .../*, NULL, const char* const* envp*/)
{
    va_list argp;
    char* args;
    char* envs;
    char* const * ptr;
    int ret = -1;
    char pathname[FILENAME_MAX];

    DPRINT("_spawnlpe('%s')\n", cmdname);

    va_start(argp, arg0);
    args = valisttos(arg0, argp, ' ');
    do
    {
	ptr = (char* const*)va_arg(argp, char*);
    }
    while (ptr != NULL);
    envs = argvtos(ptr, 0);
    if (args)
    {
	ret = do_spawn(mode, find_exec(cmdname, pathname), args, envs);
	free(args);
    }
    if (envs)
    {
	free(envs);
    }
    return ret;
}

int _spawnvpe(int mode, const char* cmdname, char* const* argv, char* const* envp)
{
    char pathname[FILENAME_MAX];
  
    DPRINT("_spawnvpe('%s')\n", cmdname);

    return _spawnve(mode, find_exec(cmdname, pathname), argv, envp);
}

int _execl(const char* cmdname, const char* arg0, ...)
{
    char* args;
    va_list argp;
    int ret = -1;

    DPRINT("_execl('%s')\n", cmdname);

    va_start(argp, arg0);
    args = valisttos(arg0, argp, ' ');

    if (args)
    {
	ret = do_spawn(P_OVERLAY, cmdname, args, NULL);
	free(args);
    }
    return ret;
}

int _execv(const char* cmdname, char* const* argv)
{
    DPRINT("_execv('%s')\n", cmdname);
    return _spawnv(P_OVERLAY, cmdname, argv);
}

int _execle(const char* cmdname, const char* arg0, ... /*, NULL, char* const* envp */)
{
    va_list argp;
    char* args;
    char* envs;
    char* const* ptr;
    int ret = -1;

    DPRINT("_execle('%s')\n", cmdname);

    va_start(argp, arg0);
    args = valisttos(arg0, argp, ' ');
    do
    {
	ptr = (char* const*)va_arg(argp, char*);
    }
    while (ptr != NULL);
    envs = argvtos((char**)ptr, 0);
    if (args)
    {
	ret = do_spawn(P_OVERLAY, cmdname, args, envs);
	free(args);
    }
    if (envs)
    {
	free(envs);
    }
    return ret;
}

int _execve(const char* cmdname, char* const* argv, char* const* envp)
{
    DPRINT("_execve('%s')\n", cmdname);
    return _spawnve(P_OVERLAY, cmdname, argv, envp);
}

int _execlp(const char* cmdname, const char* arg0, ...)
{
    char* args;
    va_list argp;
    int ret = -1;
    char pathname[FILENAME_MAX];

    DPRINT("_execlp('%s')\n", cmdname);

    va_start(argp, arg0);
    args = valisttos(arg0, argp, ' ');

    if (args)
    {
	ret = do_spawn(P_OVERLAY, find_exec(cmdname, pathname), args, NULL);
	free(args);
    }
    return ret;
}

int _execvp(const char* cmdname, char* const* argv)
{
    DPRINT("_execvp('%s')\n", cmdname);
    return _spawnvp(P_OVERLAY, cmdname, argv);
}

int _execlpe(const char* cmdname, const char* arg0, ... /*, NULL, char* const* envp */)
{
    va_list argp;
    char* args;
    char* envs;
    char* const* ptr;
    int ret = -1;
    char pathname[FILENAME_MAX];

    DPRINT("_execlpe('%s')\n", cmdname);

    va_start(argp, arg0);
    args = valisttos(arg0, argp, ' ');
    do
    {
	ptr = (char* const*)va_arg(argp, char*);
    }
    while (ptr != NULL);
    envs = argvtos(ptr, 0);
    if (args)
    {
	ret = do_spawn(P_OVERLAY, find_exec(cmdname, pathname), args, envs);
	free(args);
    }
    if (envs)
    {
	free(envs);
    }
    return ret;
}

int _execvpe(const char* cmdname, char* const* argv, char* const* envp)
{
    DPRINT("_execvpe('%s')\n", cmdname);
    return _spawnvpe(P_OVERLAY, cmdname, argv, envp);
}

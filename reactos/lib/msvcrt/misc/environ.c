/* $Id: environ.c,v 1.8 2004/05/27 11:49:48 hbirr Exp $
 *
 * dllmain.c
 *
 * ReactOS MSVCRT.DLL Compatibility Library
 */

#include <windows.h>
#include <msvcrt/internal/tls.h>
#include <msvcrt/stdlib.h>
#include <msvcrt/string.h>

#define NDEBUG
#include <msvcrt/msvcrtdbg.h>


unsigned int _osver = 0;
unsigned int _winminor = 0;
unsigned int _winmajor = 0;
unsigned int _winver = 0;

char *_acmdln = NULL;       /* pointer to ascii command line */
unsigned _envvar_count;	    /* number of environment vars within current environment */
#undef _environ
char **_environ = NULL;     /* pointer to environment block */
char ***_environ_dll = &_environ;/* pointer to environment block */
char **__initenv = NULL;
#undef _pgmptr 
char *_pgmptr = NULL;       /* pointer to program name */
int __app_type = 0; //_UNKNOWN_APP; /* application type */
int __mb_cur_max = 1;

int _commode = _IOCOMMIT;


/*
 * @implemented
 */
int *__p__commode(void) // not exported by NTDLL
{
   return &_commode;
}

int BlockEnvToEnviron(void)
{
    char * ptr, * ptr2;
    int i, count, len, size;

    DPRINT("BlockEnvToEnviron()\n");

    ptr2 = ptr = (char*)GetEnvironmentStringsA();
    if (ptr == NULL) {
	return -1;
    }

    size = 0;
    count = 0;
    while (*ptr2) {
	len = strlen(ptr2);
	if (*ptr2 != '=') {
	    count++;
	    size += len + 1;
	}
	ptr2 += len + 1;
    }

    if (count != _envvar_count) {
	if (_environ && _environ != __initenv) {
	    free(_environ[0]);
	    _environ = realloc(_environ, (count + 1) * sizeof(char*));
	} else {
	    _environ = malloc((count + 1) * sizeof(char*));
	}
	if (_environ == NULL) {
	    FreeEnvironmentStringsA(ptr);
	    _envvar_count = 0;
	    return -1;
	}
	_environ[0] = NULL;
    }
    if (_environ[0] != NULL) {
	free(_environ[0]);
    }

    _environ[0] = malloc(size);
    if (_environ[0] == NULL) {
	FreeEnvironmentStringsA(ptr);
	free(_environ);
	_envvar_count = 0;
	return -1;
    }

    ptr2 = ptr;
    i = 0;
    while (*ptr2 && i < count) {
	len = strlen(ptr2);
        /* skip current directory of the form "=C:=C:\directory\" */
	if (*ptr2 != '=') {
	    memcpy(_environ[i], ptr2, len + 1);
	    i++;
	    if (i < count) {
                _environ[i] = _environ[i - 1] + len + 1; 
	    }
	}
	ptr2 += len + 1;
    }
    _environ[i] = NULL;
    _envvar_count = count;
    if (__initenv == NULL) 
    {
       __initenv = _environ;
    }
    FreeEnvironmentStringsA(ptr);
    return 0;
}

/*
 * @implemented
 */
void __set_app_type(int app_type)
{
    __app_type = app_type;
}

/*
 * @implemented
 */
char **__p__acmdln(void)
{
    return &_acmdln;
}

/*
 * @implemented
 */
char ***__p__environ(void)
{
    return _environ_dll;
}

/*
 * @implemented
 */
char ***__p___initenv(void)
{
    return &__initenv;
}

/*
 * @implemented
 */
int *__p___mb_cur_max(void)
{
    return &__mb_cur_max;
}

/*
 * @implemented
 */
unsigned int *__p__osver(void)
{
    return &_osver;
}

/*
 * @implemented
 */
char **__p__pgmptr(void)
{
    return &_pgmptr;
}

/*
 * @implemented
 */
unsigned int *__p__winmajor(void)
{
    return &_winmajor;
}

/*
 * @implemented
 */
unsigned int *__p__winminor(void)
{
    return &_winminor;
}

/*
 * @implemented
 */
unsigned int *__p__winver(void)
{
    return &_winver;
}

/* EOF */

/* $Id: environ.c,v 1.2 2003/04/24 16:34:26 hbirr Exp $
 *
 * dllmain.c
 *
 * ReactOS MSVCRT.DLL Compatibility Library
 */

#include <windows.h>
#include <msvcrt/internal/tls.h>
#include <msvcrt/stdlib.h>

#define NDEBUG
#include <msvcrt/msvcrtdbg.h>


unsigned int _osver = 0;
unsigned int _winminor = 0;
unsigned int _winmajor = 0;
unsigned int _winver = 0;

char *_acmdln = NULL;       /* pointer to ascii command line */
#undef _environ
char **_environ = NULL;     /* pointer to environment block */
char ***_environ_dll = &_environ;/* pointer to environment block */
char **__initenv = NULL;
char *_pgmptr = NULL;       /* pointer to program name */
int __app_type = 0; //_UNKNOWN_APP; /* application type */
int __mb_cur_max = 1;

int _commode = _IOCOMMIT;


int *__p__commode(void) // not exported by NTDLL
{
   return &_commode;
}

int BlockEnvToEnviron(void)
{
    char * ptr, * ptr2;
    int i, len;

    DPRINT("BlockEnvToEnviron()\n");

    if (_environ) {
        FreeEnvironmentStringsA(_environ[0]);
	if (__initenv == _environ) {
	    __initenv[0] == NULL;
	} else {
	    free(_environ);
	}
    }
    _environ = NULL;
    ptr2 = ptr = (char*)GetEnvironmentStringsA();
    if (ptr == NULL) {
        DPRINT("GetEnvironmentStringsA() returnd NULL\n");
        return -1;
    }
    len = 0;
    while (*ptr2) {
        len++;
        while (*ptr2++);
    }
    _environ = malloc((len + 1) * sizeof(char*));
    if (_environ == NULL) {
        FreeEnvironmentStringsA(ptr);
        return -1;
    }
    for (i = 0; i < len && *ptr; i++) {
        _environ[i] = ptr;
        while (*ptr++);
    }
    _environ[i] = NULL;
    if (__initenv == NULL)
    {
       __initenv = _environ;
    }
    return 0;
}

void __set_app_type(int app_type)
{
    __app_type = app_type;
}

char **__p__acmdln(void)
{
    return &_acmdln;
}

char ***__p__environ(void)
{
    return _environ_dll;
}

char ***__p___initenv(void)
{
    return &__initenv;
}

int *__p___mb_cur_max(void)
{
    return &__mb_cur_max;
}

unsigned int *__p__osver(void)
{
    return &_osver;
}

char **__p__pgmptr(void)
{
    return &_pgmptr;
}

unsigned int *__p__winmajor(void)
{
    return &_winmajor;
}

unsigned int *__p__winminor(void)
{
    return &_winminor;
}

unsigned int *__p__winver(void)
{
    return &_winver;
}

/* EOF */

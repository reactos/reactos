/* $Id: environ.c,v 1.7 2004/02/21 08:02:49 tamlin Exp $
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
    int i, count;

    DPRINT("BlockEnvToEnviron()\n");

    if (_environ && _environ != __initenv) {
        FreeEnvironmentStringsA(_environ[0]);
        free(_environ);
    }
    _environ = NULL;
    ptr2 = ptr = (char*)GetEnvironmentStringsA();
    if (ptr == NULL) {
        DPRINT("GetEnvironmentStringsA() returnd NULL\n");
        return -1;
    }
    count = 0;
    while (*ptr2) {
        /* skip current directory of the form "=C:=C:\directory\" */
        if (*ptr2 != '=') {
            count++;
        }
        ptr2 += strlen(ptr2) + 1;
    }
    _environ = malloc((count + 1) * sizeof(char*));
    if (_environ == NULL) {
        FreeEnvironmentStringsA(ptr);
        return -1;
    }
    i = 0;
    while (i < count && *ptr) {
        if (*ptr != '=') {
            _environ[i] = ptr;
            ++i;
        }
        ptr += strlen(ptr) + 1;
    }
    _environ[i] = NULL;
    if (__initenv == NULL)
    {
       __initenv = _environ;
    }
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

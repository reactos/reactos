/* $Id: environ.c,v 1.4 2003/07/11 21:58:09 royce Exp $
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
    int i, len;

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
    len = 0;
    while (*ptr2) {
        len++;
        ptr2 += strlen(ptr2) + 1;
    }
    _environ = malloc((len + 1) * sizeof(char*));
    if (_environ == NULL) {
        FreeEnvironmentStringsA(ptr);
        return -1;
    }
    for (i = 0; i < len && *ptr; i++) {
        _environ[i] = ptr;
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

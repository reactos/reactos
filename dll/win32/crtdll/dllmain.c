/*
 * dllmain.c
 *
 * ReactOS CRTDLL.DLL Compatibility Library
 *
 *  THIS SOFTWARE IS NOT COPYRIGHTED
 *
 *  This source code is offered for use in the public domain. You may
 *  use, modify or distribute it freely.
 *
 *  This code is distributed in the hope that it will be useful but
 *  WITHOUT ANY WARRANTY. ALL WARRENTIES, EXPRESS OR IMPLIED ARE HEREBY
 *  DISCLAMED. This includes but is not limited to warrenties of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#include "precomp.h"
#include <mbctype.h>
#include <sys/stat.h>
#include <internal/wine/msvcrt.h>

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(crtdll);

/* from msvcrt */
extern void __getmainargs( int *argc, char ***argv, char ***envp,
                           int expand_wildcards, int *new_mode );

/* EXTERNAL PROTOTYPES ********************************************************/

extern int BlockEnvToEnvironA(void);
extern int BlockEnvToEnvironW(void);
extern void FreeEnvironment(char **environment);
extern void _atexit_cleanup(void);

extern unsigned int _osver;
extern unsigned int _winminor;
extern unsigned int _winmajor;
extern unsigned int _winver;

unsigned int CRTDLL__basemajor_dll = 0;
unsigned int CRTDLL__baseminor_dll = 0;
unsigned int CRTDLL__baseversion_dll = 0;
unsigned int CRTDLL__cpumode_dll = 0;
unsigned int CRTDLL__osmajor_dll = 0;
unsigned int CRTDLL__osminor_dll = 0;
unsigned int CRTDLL__osmode_dll = 0;
unsigned int CRTDLL__osversion_dll = 0;
int _fileinfo_dll;

extern char* _acmdln;        /* pointer to ascii command line */
extern wchar_t* _wcmdln;     /* pointer to wide character command line */
#undef _environ
extern char** _environ;      /* pointer to environment block */
extern char** __initenv;     /* pointer to initial environment block */
extern wchar_t** _wenviron;  /* pointer to environment block */
extern wchar_t** __winitenv; /* pointer to initial environment block */

/* dev_t is a short in crtdll but an unsigned int in msvcrt */
typedef short crtdll_dev_t;

struct crtdll_stat
{
  crtdll_dev_t   st_dev;
  _ino_t         st_ino;
  unsigned short st_mode;
  short          st_nlink;
  short          st_uid;
  short          st_gid;
  crtdll_dev_t   st_rdev;
  _off_t         st_size;
  time_t         st_atime;
  time_t         st_mtime;
  time_t         st_ctime;
};

/* convert struct _stat from crtdll format to msvcrt format */
static void convert_struct_stat( struct crtdll_stat *dst, const struct _stat *src )
{
    dst->st_dev   = src->st_dev;
    dst->st_ino   = src->st_ino;
    dst->st_mode  = src->st_mode;
    dst->st_nlink = src->st_nlink;
    dst->st_uid   = src->st_uid;
    dst->st_gid   = src->st_gid;
    dst->st_rdev  = src->st_rdev;
    dst->st_size  = src->st_size;
    dst->st_atime = src->st_atime;
    dst->st_mtime = src->st_mtime;
    dst->st_ctime = src->st_ctime;
}

/* LIBRARY ENTRY POINT ********************************************************/

BOOL
WINAPI
DllMain(PVOID hinstDll, ULONG dwReason, PVOID reserved)
{
    DWORD version;
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        version = GetVersion();

        /* initialize version info */
        CRTDLL__basemajor_dll   = (version >> 24) & 0xFF;
        CRTDLL__baseminor_dll   = (version >> 16) & 0xFF;
        CRTDLL__baseversion_dll = (version >> 16);
        CRTDLL__cpumode_dll     = 1; /* FIXME */
        CRTDLL__osmajor_dll     = (version >>8) & 0xFF;
        CRTDLL__osminor_dll     = (version & 0xFF);
        CRTDLL__osmode_dll      = 1; /* FIXME */
        CRTDLL__osversion_dll   = (version & 0xFFFF);

        _winmajor = (_osver >> 8) & 0xFF;
        _winminor = _osver & 0xFF;
        _winver = (_winmajor << 8) + _winminor;
        _osver = (_osver >> 16) & 0xFFFF;

        /* create tls stuff */
        if (!msvcrt_init_tls())
          return FALSE;

        if (BlockEnvToEnvironA() < 0)
            return FALSE;

        if (BlockEnvToEnvironW() < 0)
        {
            FreeEnvironment(_environ);
            return FALSE;
        }

        _acmdln = _strdup(GetCommandLineA());
        _wcmdln = _wcsdup(GetCommandLineW());

        /* Initialization of the WINE code */
        msvcrt_init_mt_locks();
        //if(!msvcrt_init_locale()) {
            //msvcrt_free_mt_locks();
           // msvcrt_free_tls_mem();
            //return FALSE;
        //}
        //msvcrt_init_math();
        msvcrt_init_io();
        //msvcrt_init_console();
        //msvcrt_init_args();
        //msvcrt_init_signals();
        _setmbcp(_MB_CP_LOCALE);
        TRACE("Attach done\n");
        break;
    case DLL_THREAD_ATTACH:
        break;

    case DLL_THREAD_DETACH:
        msvcrt_free_tls_mem();
        break;

    case DLL_PROCESS_DETACH:
        TRACE("Detach\n");
        /* Deinit of the WINE code */
        msvcrt_free_io();
        if (reserved) break;
        msvcrt_free_mt_locks();
        //msvcrt_free_console();
        //msvcrt_free_args();
        //msvcrt_free_signals();
        msvcrt_free_tls_mem();
        if (!msvcrt_free_tls())
          return FALSE;
        //MSVCRT__free_locale(MSVCRT_locale);

        if (__winitenv && __winitenv != _wenviron)
            FreeEnvironment((char**)__winitenv);
        if (_wenviron)
            FreeEnvironment((char**)_wenviron);

        if (__initenv && __initenv != _environ)
            FreeEnvironment(__initenv);
        if (_environ)
            FreeEnvironment(_environ);

        TRACE("Detach done\n");
        break;
    }

    return TRUE;
}


/*********************************************************************
 *                  __GetMainArgs  (CRTDLL.@)
 */
void __GetMainArgs( int *argc, char ***argv, char ***envp, int expand_wildcards )
{
    int new_mode = 0;
    __getmainargs( argc, argv, envp, expand_wildcards, &new_mode );
}


/*********************************************************************
 *    _fstat (CRTDLL.@)
 */
int CRTDLL__fstat(int fd, struct crtdll_stat* buf)
{
    extern int _fstat(int,struct _stat*);
    struct _stat st;
    int ret;

    if (!(ret = _fstat( fd, &st ))) convert_struct_stat( buf, &st );
    return ret;
}


/*********************************************************************
 *    _stat (CRTDLL.@)
 */
int CRTDLL__stat(const char* path, struct crtdll_stat * buf)
{
    extern int _stat(const char*,struct _stat*);
    struct _stat st;
    int ret;

    if (!(ret = _stat( path, &st ))) convert_struct_stat( buf, &st );
    return ret;
}


/*********************************************************************
 *    _strdec (CRTDLL.@)
 */
char *_strdec(const char *str1, const char *str2)
{
    return (char *)(str2 - 1);
}


/*********************************************************************
 *    _strinc (CRTDLL.@)
 */
char *_strinc(const char *str)
{
    return (char *)(str + 1);
}


/*********************************************************************
 *    _strncnt (CRTDLL.@)
 */
size_t _strncnt(const char *str, size_t maxlen)
{
    size_t len = strlen(str);
    return (len > maxlen) ? maxlen : len;
}


/*********************************************************************
 *    _strnextc (CRTDLL.@)
 */
unsigned int _strnextc(const char *str)
{
    return (unsigned int)str[0];
}


/*********************************************************************
 *    _strninc (CRTDLL.@)
 */
char *_strninc(const char *str, size_t len)
{
    return (char *)(str + len);
}


/*********************************************************************
 *    _strspnp (CRTDLL.@)
 */
char *_strspnp( const char *str1, const char *str2)
{
    str1 += strspn( str1, str2 );
    return *str1 ? (char*)str1 : NULL;
}

/* EOF */

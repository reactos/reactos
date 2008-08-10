/* $Id: dllmain.c 12852 2005-01-06 13:58:04Z mf $
 *
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

#include <precomp.h>
#include <internal/wine/msvcrt.h>
#include <sys/stat.h>
#include <locale.h>
#include <mbctype.h>

#include "wine/debug.h"
WINE_DEFAULT_DEBUG_CHANNEL(crtdll);


/* EXTERNAL PROTOTYPES ********************************************************/

extern int BlockEnvToEnvironA(void);
extern int BlockEnvToEnvironW(void);
extern void FreeEnvironment(char **environment);
extern void _atexit_cleanup(void);

extern unsigned int _osver;
extern unsigned int _winminor;
extern unsigned int _winmajor;
extern unsigned int _winver;

unsigned int CRTDLL__basemajor_dll;
unsigned int CRTDLL__baseminor_dll;
unsigned int CRTDLL__baseversion_dll;
unsigned int CRTDLL__cpumode_dll;
unsigned int CRTDLL__osmajor_dll;
unsigned int CRTDLL__osminor_dll;
unsigned int CRTDLL__osmode_dll;
unsigned int CRTDLL__osversion_dll;

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

/* from msvcrt */
extern void __getmainargs( int *argc, char ***argv, char ***envp,
                           int expand_wildcards, int *new_mode );

/* LIBRARY GLOBAL VARIABLES ***************************************************/

HANDLE hHeap = NULL;        /* handle for heap */


/* LIBRARY ENTRY POINT ********************************************************/

BOOL
STDCALL
DllMain(PVOID hinstDll, ULONG dwReason, PVOID reserved)
{
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH://1
        /* initialize version info */
        //TRACE("Attach %d\n", nAttachCount);

        _osver = GetVersion();

        CRTDLL__basemajor_dll   = (_osver >> 24) & 0xFF;
        CRTDLL__baseminor_dll   = (_osver >> 16) & 0xFF;
        CRTDLL__baseversion_dll = (_osver >> 16);
        CRTDLL__cpumode_dll     = 1; /* FIXME */
        CRTDLL__osmajor_dll     = (_osver >>8) & 0xFF;
        CRTDLL__osminor_dll     = (_osver & 0xFF);
        CRTDLL__osmode_dll      = 1; /* FIXME */
        CRTDLL__osversion_dll   = (_osver & 0xFFFF);

        _winmajor = (_osver >> 8) & 0xFF;
        _winminor = _osver & 0xFF;
        _winver = (_winmajor << 8) + _winminor;
        _osver = (_osver >> 16) & 0xFFFF;
        hHeap = HeapCreate(0, 100000, 0);
        if (hHeap == NULL)
            return FALSE;

        /* create tls stuff */
        if (!CreateThreadData())
            return FALSE;

        if (BlockEnvToEnvironA() < 0)
            return FALSE;

        if (BlockEnvToEnvironW() < 0)
        {
            FreeEnvironment((char**)_wenviron);
            return FALSE;
        }

        _acmdln = _strdup(GetCommandLineA());
        _wcmdln = _wcsdup(GetCommandLineW());

        /* FIXME: more initializations... */

        /* FIXME: Initialization of the WINE code */
        msvcrt_init_mt_locks();
        msvcrt_init_io();
        setlocale(0, "C");
        //_setmbcp(_MB_CP_LOCALE);

        TRACE("Attach done\n");
        break;

    case DLL_THREAD_ATTACH://2
        break;

    case DLL_THREAD_DETACH://4
        FreeThreadData(NULL);
        break;

    case DLL_PROCESS_DETACH://0
        //TRACE("Detach %d\n", nAttachCount);
        /* Deinit of the WINE code */
        msvcrt_free_io();
        msvcrt_free_mt_locks();
        _atexit_cleanup();

        /* destroy tls stuff */
        DestroyThreadData();

	if (__winitenv && __winitenv != _wenviron)
            FreeEnvironment((char**)__winitenv);
        if (_wenviron)
            FreeEnvironment((char**)_wenviron);

	if (__initenv && __initenv != _environ)
            FreeEnvironment(__initenv);
        if (_environ)
            FreeEnvironment(_environ);

        /* destroy heap */
        HeapDestroy(hHeap);

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

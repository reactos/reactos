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
 *  WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 *  DISCLAMED. This includes but is not limited to warranties of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#include "precomp.h"
#include <mbctype.h>
#include <sys/stat.h>
#include <internal/wine/msvcrt.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <mbstring.h>

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(crtdll);

/* from msvcrt */
extern void __getmainargs( int *argc, char ***argv, char ***envp,
                           int expand_wildcards, int *new_mode );

/* EXTERNAL PROTOTYPES ********************************************************/

unsigned int CRTDLL__basemajor_dll = 0;
unsigned int CRTDLL__baseminor_dll = 0;
unsigned int CRTDLL__baseversion_dll = 0;
unsigned int CRTDLL__cpumode_dll = 0;
unsigned int CRTDLL__osmajor_dll = 0;
unsigned int CRTDLL__osminor_dll = 0;
unsigned int CRTDLL__osmode_dll = 0;
unsigned int CRTDLL__osversion_dll = 0;
int _fileinfo_dll;

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

/*********************************************************************
 *    _mbsstr (CRTDLL.@)
 */
unsigned char* CRTDLL__mbsstr(const unsigned char* Str, const unsigned char* Substr)
{
    if (!*Str && !*Substr)
        return NULL;
    return (unsigned char*)strstr((const char*)Str, (const char*)Substr);
}

/*********************************************************************
 *    sprintf (CRTDLL.@)
 */
int CRTDLL_sprintf(char* buffer, const char* format, ...)
{
    va_list args;
    int ret;

    /* Access both buffers to emulate CRTDLL exception behavior */
    (void)*(volatile char*)buffer;
    (void)*(volatile char*)format;

    va_start(args, format);
    ret = vsprintf(buffer, format, args);
    va_end(args);
    return ret;
}

/*********************************************************************
 *    strtoul (CRTDLL.@)
 */
unsigned long CRTDLL_strtoul(const char* nptr, char** endptr, int base)
{
    /* Access nptr to emulate CRTDLL exception behavior */
    (void)*(volatile char*)nptr;
    return strtoul(nptr, endptr, base);
}

/*********************************************************************
 *    wcstoul (CRTDLL.@)
 */
unsigned long CRTDLL_wcstoul(const wchar_t* nptr, wchar_t** endptr, int base)
{
    /* Access nptr to emulate CRTDLL exception behavior */
    (void)*(volatile wchar_t*)nptr;
    return wcstoul(nptr, endptr, base);
}

/*********************************************************************
 *    system (CRTDLL.@)
 */
int CRTDLL_system(const char* command)
{
    if (command != NULL)
    {
        errno = 0;
    }
    return system(command);
}

/*********************************************************************
 *    _mbsnbcat (CRTDLL.@)
 */
unsigned char* CRTDLL__mbsnbcat(unsigned char* dest, const unsigned char* src, size_t n)
{
    if (n)
    {
        /* Access both buffers to emulate CRTDLL exception behavior */
        (void)*(volatile char*)dest;
        (void)*(volatile char*)src;
    }

    return _mbsnbcat(dest, src, n);
}

/*********************************************************************
 *    _mbsncat (CRTDLL.@)
 */
unsigned char* CRTDLL__mbsncat(unsigned char* dest, const unsigned char* src, size_t n)
{
    if (n)
    {
        /* Access both buffers to emulate CRTDLL exception behavior */
        (void)*(volatile char*)dest;
        (void)*(volatile char*)src;
    }

    return _mbsncat(dest, src, n);
}

/*********************************************************************
 *    _vsnprintf (CRTDLL.@)
 */
int CRTDLL__vsnprintf(char* buffer, size_t count, const char* format, va_list argptr)
{
    (void)*(volatile char*)format;
    if (count)
    {
        (void)*(volatile char*)buffer;
    }
    int ret = _vsnprintf(buffer, count, format, argptr);
    if (ret > (int)count)
        ret = -1;
    errno = 0;;
    return ret;
}

/*********************************************************************
 *    _snprintf (CRTDLL.@)
 */
int CRTDLL__snprintf(char* buffer, size_t count, const char* format, ...)
{
    va_list args;
    int ret;
    va_start(args, format);
    ret = CRTDLL__vsnprintf(buffer, count, format, args);
    va_end(args);
    return ret;
}

/*********************************************************************
 *    _vsnwprintf (CRTDLL.@)
 */
int CRTDLL__vsnwprintf(wchar_t* buffer, size_t count, const wchar_t* format, va_list argptr)
{
    (void)*(volatile wchar_t*)format;
    int ret = _vsnwprintf(buffer, count, format, argptr);
    if ((ret > (int)count) || !buffer)
        ret = -1;
    errno = 0;;
    return ret;
}


/* Dummy for rand_s, not used. */
BOOLEAN
WINAPI
SystemFunction036(PVOID pbBuffer, ULONG dwLen)
{
    assert(FALSE);
    return 0;
}


/* EOF */

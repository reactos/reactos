/*
 * msvcrt.dll dll data items
 *
 * Copyright 2000 Jon Griffiths
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <fcntl.h>
#include <math.h>
#include "msvcrt.h"
#include <winnls.h>
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msvcrt);

static WCHAR **initial_wargv;
static int initial_argc;
int MSVCRT___argc = 0;
static int wargc_expand;
unsigned int MSVCRT__commode = 0;
int MSVCRT__fmode = 0;
unsigned int MSVCRT__osver = 0;
unsigned int MSVCRT__osplatform = 0;
unsigned int MSVCRT__winmajor = 0;
unsigned int MSVCRT__winminor = 0;
unsigned int MSVCRT__winver = 0;
#ifdef _CRTDLL
unsigned int CRTDLL__basemajor_dll = 0;
unsigned int CRTDLL__baseminor_dll = 0;
unsigned int CRTDLL__baseversion_dll = 0;
unsigned int CRTDLL__cpumode_dll = 1;
unsigned int CRTDLL__osmode_dll = 1;
#endif
unsigned int MSVCRT___setlc_active = 0;
unsigned int MSVCRT___unguarded_readlc_active = 0;
double MSVCRT__HUGE = 0;
char **MSVCRT___argv = NULL;
wchar_t **MSVCRT___wargv = NULL;
static wchar_t **wargv_expand;
char *MSVCRT__acmdln = NULL;
wchar_t *MSVCRT__wcmdln = NULL;
char **MSVCRT__environ = NULL;
wchar_t **MSVCRT__wenviron = NULL;
char **MSVCRT___initenv = NULL;
wchar_t **MSVCRT___winitenv = NULL;
int MSVCRT_app_type = 0;
char* MSVCRT__pgmptr = NULL;
WCHAR* MSVCRT__wpgmptr = NULL;

static char **build_argv( WCHAR **wargv )
{
    int argc;
    char *p, **argv;
    DWORD total = 0;

    for (argc = 0; wargv[argc]; argc++)
        total += WideCharToMultiByte( CP_ACP, 0, wargv[argc], -1, NULL, 0, NULL, NULL );

    argv = HeapAlloc( GetProcessHeap(), 0, total + (argc + 1) * sizeof(*argv) );
    p = (char *)(argv + argc + 1);
    for (argc = 0; wargv[argc]; argc++)
    {
        DWORD reslen = WideCharToMultiByte( CP_ACP, 0, wargv[argc], -1, p, total, NULL, NULL );
        argv[argc] = p;
        p += reslen;
        total -= reslen;
    }
    argv[argc] = NULL;
    return argv;
}

static WCHAR **cmdline_to_argv( const WCHAR *src, int *ret_argc )
{
    WCHAR **argv, *arg, *dst;
    int argc, in_quotes = 0, bcount = 0, len = wcslen(src) + 1;

    argc = 2 + len / 2;
    argv = HeapAlloc( GetProcessHeap(), 0, argc * sizeof(*argv) + len * sizeof(WCHAR) );
    arg = dst = (WCHAR *)(argv + argc);
    argc = 0;
    while (*src)
    {
        if ((*src == ' ' || *src == '\t') && !in_quotes)
        {
            /* skip the remaining spaces */
            while (*src == ' ' || *src == '\t') src++;
            if (!*src) break;
            /* close the argument and copy it */
            *dst++ = 0;
            argv[argc++] = arg;
            /* start with a new argument */
            arg = dst;
            bcount = 0;
        }
        else if (*src == '\\')
        {
            *dst++ = *src++;
            bcount++;
        }
        else if (*src == '"')
        {
            if ((bcount & 1) == 0)
            {
                /* Preceded by an even number of '\', this is half that
                 * number of '\', plus a '"' which we discard.
                 */
                dst -= bcount / 2;
                src++;
                if (in_quotes && *src == '"') *dst++ = *src++;
                else in_quotes = !in_quotes;
            }
            else
            {
                /* Preceded by an odd number of '\', this is half that
                 * number of '\' followed by a '"'
                 */
                dst -= bcount / 2 + 1;
                *dst++ = *src++;
            }
            bcount = 0;
        }
        else  /* a regular character */
        {
            *dst++ = *src++;
            bcount = 0;
        }
    }
    *dst = 0;
    argv[argc++] = arg;
    argv[argc] = NULL;
    *ret_argc = argc;
    return argv;
}

typedef void (CDECL *_INITTERMFUN)(void);
typedef int (CDECL *_INITTERM_E_FN)(void);

/***********************************************************************
 *		__p___argc (MSVCRT.@)
 */
int* CDECL __p___argc(void) { return &MSVCRT___argc; }

/***********************************************************************
 *		__p__commode (MSVCRT.@)
 */
unsigned int* CDECL __p__commode(void) { return &MSVCRT__commode; }


/***********************************************************************
 *              __p__pgmptr (MSVCRT.@)
 */
char** CDECL __p__pgmptr(void) { return &MSVCRT__pgmptr; }

/***********************************************************************
 *              __p__wpgmptr (MSVCRT.@)
 */
WCHAR** CDECL __p__wpgmptr(void) { return &MSVCRT__wpgmptr; }

/***********************************************************************
 *              _get_pgmptr (MSVCRT.@)
 */
int CDECL _get_pgmptr(char** p)
{
  if (!MSVCRT_CHECK_PMT(p)) return EINVAL;

  *p = MSVCRT__pgmptr;
  return 0;
}

/***********************************************************************
 *              _get_wpgmptr (MSVCRT.@)
 */
int CDECL _get_wpgmptr(WCHAR** p)
{
  if (!MSVCRT_CHECK_PMT(p)) return EINVAL;
  *p = MSVCRT__wpgmptr;
  return 0;
}

/***********************************************************************
 *		__p__fmode (MSVCRT.@)
 */
int* CDECL __p__fmode(void) { return &MSVCRT__fmode; }

/***********************************************************************
 *              _set_fmode (MSVCRT.@)
 */
int CDECL _set_fmode(int mode)
{
    /* TODO: support _O_WTEXT */
    if(!MSVCRT_CHECK_PMT(mode==_O_TEXT || mode==_O_BINARY))
        return EINVAL;

    MSVCRT__fmode = mode;
    return 0;
}

/***********************************************************************
 *              _get_fmode (MSVCRT.@)
 */
int CDECL _get_fmode(int *mode)
{
    if(!MSVCRT_CHECK_PMT(mode))
        return EINVAL;

    *mode = MSVCRT__fmode;
    return 0;
}

/***********************************************************************
 *		__p__osver (MSVCRT.@)
 */
unsigned int* CDECL __p__osver(void) { return &MSVCRT__osver; }

/***********************************************************************
 *		__p__winmajor (MSVCRT.@)
 */
unsigned int* CDECL __p__winmajor(void) { return &MSVCRT__winmajor; }

/***********************************************************************
 *		__p__winminor (MSVCRT.@)
 */
unsigned int* CDECL __p__winminor(void) { return &MSVCRT__winminor; }

/***********************************************************************
 *		__p__winver (MSVCRT.@)
 */
unsigned int* CDECL __p__winver(void) { return &MSVCRT__winver; }

/*********************************************************************
 *		__p__acmdln (MSVCRT.@)
 */
char** CDECL __p__acmdln(void) { return &MSVCRT__acmdln; }

/*********************************************************************
 *		__p__wcmdln (MSVCRT.@)
 */
wchar_t** CDECL __p__wcmdln(void) { return &MSVCRT__wcmdln; }

/*********************************************************************
 *		__p___argv (MSVCRT.@)
 */
char*** CDECL __p___argv(void) { return &MSVCRT___argv; }

/*********************************************************************
 *		__p___wargv (MSVCRT.@)
 */
wchar_t*** CDECL __p___wargv(void) { return &MSVCRT___wargv; }

/*********************************************************************
 *		__p__environ (MSVCRT.@)
 */
char*** CDECL __p__environ(void)
{
  return &MSVCRT__environ;
}

/*********************************************************************
 *		__p__wenviron (MSVCRT.@)
 */
wchar_t*** CDECL __p__wenviron(void)
{
  return &MSVCRT__wenviron;
}

/*********************************************************************
 *		__p___initenv (MSVCRT.@)
 */
char*** CDECL __p___initenv(void) { return &MSVCRT___initenv; }

/*********************************************************************
 *		__p___winitenv (MSVCRT.@)
 */
wchar_t*** CDECL __p___winitenv(void) { return &MSVCRT___winitenv; }

/*********************************************************************
 *		_get_osplatform (MSVCRT.@)
 */
int CDECL _get_osplatform(int *pValue)
{
    if (!MSVCRT_CHECK_PMT(pValue != NULL)) return EINVAL;
    *pValue = MSVCRT__osplatform;
    return 0;
}

/* INTERNAL: Create a wide string from an ascii string */
wchar_t *msvcrt_wstrdupa(const char *str)
{
  const unsigned int len = strlen(str) + 1 ;
  wchar_t *wstr = malloc(len* sizeof (wchar_t));
  if (!wstr)
    return NULL;
   MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED,str,len,wstr,len);
  return wstr;
}

/*********************************************************************
 *		___unguarded_readlc_active_add_func (MSVCRT.@)
 */
unsigned int * CDECL ___unguarded_readlc_active_add_func(void)
{
  return &MSVCRT___unguarded_readlc_active;
}

/*********************************************************************
 *		___setlc_active_func (MSVCRT.@)
 */
unsigned int CDECL ___setlc_active_func(void)
{
  return MSVCRT___setlc_active;
}

/* INTERNAL: Since we can't rely on Winelib startup code calling w/getmainargs,
 * we initialise data values during DLL loading. When called by a native
 * program we simply return the data we've already initialised. This also means
 * you can call multiple times without leaking
 */
void msvcrt_init_args(void)
{
  OSVERSIONINFOW osvi;

  MSVCRT__acmdln = _strdup( GetCommandLineA() );
  MSVCRT__wcmdln = _wcsdup( GetCommandLineW() );
  initial_wargv  = cmdline_to_argv( GetCommandLineW(), &initial_argc );
  MSVCRT___argc  = initial_argc;
  MSVCRT___wargv = initial_wargv;
  MSVCRT___argv  = build_argv( initial_wargv );

  TRACE("got %s, wide = %s argc=%d\n", debugstr_a(MSVCRT__acmdln),
        debugstr_w(MSVCRT__wcmdln),MSVCRT___argc);

  osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOW);
  GetVersionExW( &osvi );
  MSVCRT__winver     = (osvi.dwMajorVersion << 8) | osvi.dwMinorVersion;
  MSVCRT__winmajor   = osvi.dwMajorVersion;
  MSVCRT__winminor   = osvi.dwMinorVersion;
  MSVCRT__osver      = osvi.dwBuildNumber;
  MSVCRT__osplatform = osvi.dwPlatformId;
  TRACE( "winver %08x winmajor %08x winminor %08x osver %08x\n",
          MSVCRT__winver, MSVCRT__winmajor, MSVCRT__winminor, MSVCRT__osver);
#ifdef _CRTDLL
  CRTDLL__baseversion_dll = (GetVersion() >> 16);
  CRTDLL__basemajor_dll   = CRTDLL__baseversion_dll >> 8;
  CRTDLL__baseminor_dll   = CRTDLL__baseversion_dll & 0xff;
#endif

  MSVCRT__HUGE = HUGE_VAL;
  MSVCRT___setlc_active = 0;
  MSVCRT___unguarded_readlc_active = 0;
  MSVCRT__fmode = _O_TEXT;

  env_init(FALSE, FALSE);

  MSVCRT__pgmptr = HeapAlloc(GetProcessHeap(), 0, MAX_PATH);
  if (MSVCRT__pgmptr)
  {
    if (!GetModuleFileNameA(0, MSVCRT__pgmptr, MAX_PATH))
      MSVCRT__pgmptr[0] = '\0';
    else
      MSVCRT__pgmptr[MAX_PATH - 1] = '\0';
  }

  MSVCRT__wpgmptr = HeapAlloc(GetProcessHeap(), 0, MAX_PATH * sizeof(WCHAR));
  if (MSVCRT__wpgmptr)
  {
    if (!GetModuleFileNameW(0, MSVCRT__wpgmptr, MAX_PATH))
      MSVCRT__wpgmptr[0] = '\0';
    else
      MSVCRT__wpgmptr[MAX_PATH - 1] = '\0';
  }
}

/* INTERNAL: free memory used by args */
void msvcrt_free_args(void)
{
  /* FIXME: more things to free */
  HeapFree(GetProcessHeap(), 0, MSVCRT___argv);
  HeapFree(GetProcessHeap(), 0, MSVCRT__pgmptr);
  HeapFree(GetProcessHeap(), 0, MSVCRT__wpgmptr);
  HeapFree(GetProcessHeap(), 0, wargv_expand);
}

static int build_expanded_wargv(int *argc, wchar_t **argv)
{
    int i, size=0, args_no=0, path_len;
    BOOL is_expandable;
    HANDLE h;

    args_no = 0;
    for(i=0; i < initial_argc; i++) {
        WIN32_FIND_DATAW data;
        int len = 0;

        is_expandable = FALSE;
        for(path_len = wcslen(initial_wargv[i])-1; path_len>=0; path_len--) {
            if(initial_wargv[i][path_len]=='*' || initial_wargv[i][path_len]=='?')
                is_expandable = TRUE;
            else if(initial_wargv[i][path_len]=='\\' || initial_wargv[i][path_len]=='/')
                break;
        }
        path_len++;

        if(is_expandable)
            h = FindFirstFileW(initial_wargv[i], &data);
        else
            h = INVALID_HANDLE_VALUE;

        if(h != INVALID_HANDLE_VALUE) {
            do {
                if(data.cFileName[0]=='.' && (data.cFileName[1]=='\0' ||
                            (data.cFileName[1]=='.' && data.cFileName[2]=='\0')))
                    continue;

                len = wcslen(data.cFileName)+1;
                if(argv) {
                    argv[args_no] = (wchar_t*)(argv+*argc+1)+size;
                    memcpy(argv[args_no], initial_wargv[i], path_len*sizeof(wchar_t));
                    memcpy(argv[args_no]+path_len, data.cFileName, len*sizeof(wchar_t));
                }
                args_no++;
                size += len+path_len;
            }while(FindNextFileW(h, &data));
            FindClose(h);
        }

        if(!len) {
            len = wcslen(initial_wargv[i])+1;
            if(argv) {
                argv[args_no] = (wchar_t*)(argv+*argc+1)+size;
                memcpy(argv[args_no], initial_wargv[i], len*sizeof(wchar_t));
            }
            args_no++;
            size += len;
        }
    }

    if(argv)
        argv[args_no] = NULL;
    size *= sizeof(wchar_t);
    size += (args_no+1)*sizeof(wchar_t*);
    *argc = args_no;
    return size;
}

/*********************************************************************
 *		__wgetmainargs (MSVCRT.@)
 */
int CDECL __wgetmainargs(int *argc, wchar_t** *wargv, wchar_t** *wenvp,
                          int expand_wildcards, int *new_mode)
{
    TRACE("(%p,%p,%p,%d,%p).\n", argc, wargv, wenvp, expand_wildcards, new_mode);

    if (expand_wildcards) {
        HeapFree(GetProcessHeap(), 0, wargv_expand);
        wargv_expand = HeapAlloc(GetProcessHeap(), 0,
                build_expanded_wargv(&wargc_expand, NULL));
        if (wargv_expand) {
            build_expanded_wargv(&wargc_expand, wargv_expand);

            MSVCRT___argc = wargc_expand;
            MSVCRT___wargv = wargv_expand;
        }else {
            expand_wildcards = 0;
        }
    }
    if (!expand_wildcards) {
        MSVCRT___argc = initial_argc;
        MSVCRT___wargv = initial_wargv;
    }

    env_init(TRUE, FALSE);

    *argc = MSVCRT___argc;
    *wargv = MSVCRT___wargv;
    *wenvp = MSVCRT__wenviron;
    if (new_mode)
        _set_new_mode( *new_mode );
    return 0;
}

/*********************************************************************
 *		__getmainargs (MSVCRT.@)
 */
int CDECL __getmainargs(int *argc, char** *argv, char** *envp,
                         int expand_wildcards, int *new_mode)
{
    TRACE("(%p,%p,%p,%d,%p).\n", argc, argv, envp, expand_wildcards, new_mode);

    if (expand_wildcards) {
        HeapFree(GetProcessHeap(), 0, wargv_expand);
        wargv_expand = HeapAlloc(GetProcessHeap(), 0,
                build_expanded_wargv(&wargc_expand, NULL));
        if (wargv_expand) {
            build_expanded_wargv(&wargc_expand, wargv_expand);

            MSVCRT___argc = wargc_expand;
            MSVCRT___argv = build_argv( wargv_expand );
        }else {
            expand_wildcards = 0;
        }
    }
    if (!expand_wildcards) {
        MSVCRT___argc = initial_argc;
        MSVCRT___argv = build_argv( initial_wargv );
    }

    *argc = MSVCRT___argc;
    *argv = MSVCRT___argv;
    *envp = MSVCRT__environ;

    if (new_mode)
        _set_new_mode( *new_mode );
    return 0;
}

#ifdef _CRTDLL
/*********************************************************************
 *                  __GetMainArgs  (CRTDLL.@)
 */
void CDECL __GetMainArgs( int *argc, char ***argv, char ***envp, int expand_wildcards )
{
    int new_mode = 0;
    __getmainargs( argc, argv, envp, expand_wildcards, &new_mode );
}
#endif

/*********************************************************************
 *		_initterm (MSVCRT.@)
 */
void CDECL _initterm(_INITTERMFUN *start,_INITTERMFUN *end)
{
  _INITTERMFUN* current = start;

  TRACE("(%p,%p)\n",start,end);
  while (current<end)
  {
    if (*current)
    {
      TRACE("Call init function %p\n",*current);
      (**current)();
      TRACE("returned\n");
    }
    current++;
  }
}

/*********************************************************************
 *  _initterm_e (MSVCRT.@)
 *
 * call an array of application initialization functions and report the return value
 */
int CDECL _initterm_e(_INITTERM_E_FN *table, _INITTERM_E_FN *end)
{
    int res = 0;

    TRACE("(%p, %p)\n", table, end);

    while (!res && table < end) {
        if (*table) {
            TRACE("calling %p\n", **table);
            res = (**table)();
            if (res)
                TRACE("function %p failed: %#x\n", *table, res);

        }
        table++;
    }
    return res;
}

/*********************************************************************
 *		__set_app_type (MSVCRT.@)
 */
void CDECL __set_app_type(int app_type)
{
  TRACE("(%d) %s application\n", app_type, app_type == 2 ? "Gui" : "Console");
  MSVCRT_app_type = app_type;
}

#if _MSVCR_VER>=140

/*********************************************************************
 *		_configure_narrow_argv (UCRTBASE.@)
 */
int CDECL _configure_narrow_argv(int mode)
{
  TRACE("(%d)\n", mode);
  return 0;
}

/*********************************************************************
 *		_initialize_narrow_environment (UCRTBASE.@)
 */
int CDECL _initialize_narrow_environment(void)
{
    TRACE("\n");
    return env_init(FALSE, FALSE);
}

/*********************************************************************
 *		_get_initial_narrow_environment (UCRTBASE.@)
 */
char** CDECL _get_initial_narrow_environment(void)
{
    TRACE("\n");
    _initialize_narrow_environment();
    return MSVCRT___initenv;
}

/*********************************************************************
 *		_configure_wide_argv (UCRTBASE.@)
 */
int CDECL _configure_wide_argv(int mode)
{
  WARN("(%d) stub\n", mode);
  return 0;
}

/*********************************************************************
 *		_initialize_wide_environment (UCRTBASE.@)
 */
int CDECL _initialize_wide_environment(void)
{
    TRACE("\n");
    return env_init(TRUE, FALSE);
}

/*********************************************************************
 *		_get_initial_wide_environment (UCRTBASE.@)
 */
wchar_t** CDECL _get_initial_wide_environment(void)
{
    TRACE("\n");
    _initialize_wide_environment();
    return MSVCRT___winitenv;
}

/*********************************************************************
 *		_get_narrow_winmain_command_line (UCRTBASE.@)
 */
char* CDECL _get_narrow_winmain_command_line(void)
{
  static char *narrow_command_line;
  char *s;

  if (narrow_command_line)
      return narrow_command_line;

  s = GetCommandLineA();
  while (*s && *s != ' ' && *s != '\t')
  {
      if (*s++ == '"')
      {
          while (*s && *s++ != '"')
              ;
      }
  }

  while (*s == ' ' || *s == '\t')
      s++;

  return narrow_command_line = s;
}

/*********************************************************************
 *		_get_wide_winmain_command_line (UCRTBASE.@)
 */
wchar_t* CDECL _get_wide_winmain_command_line(void)
{
  static wchar_t *wide_command_line;
  wchar_t *s;

  if (wide_command_line)
      return wide_command_line;

  s = GetCommandLineW();
  while (*s && *s != ' ' && *s != '\t')
  {
      if (*s++ == '"')
      {
          while (*s && *s++ != '"')
              ;
      }
  }

  while (*s == ' ' || *s == '\t')
      s++;

  return wide_command_line = s;
}

#endif /* _MSVCR_VER>=140 */

/*********************************************************************
 *    _get_winmajor (MSVCRT.@)
 */
int CDECL _get_winmajor(int* value)
{
    if (!MSVCRT_CHECK_PMT(value != NULL)) return EINVAL;
    *value = MSVCRT__winmajor;
    return 0;
}

/*********************************************************************
 *    _get_winminor (MSVCRT.@)
 */
int CDECL _get_winminor(int* value)
{
    if (!MSVCRT_CHECK_PMT(value != NULL)) return EINVAL;
    *value = MSVCRT__winminor;
    return 0;
}

/*********************************************************************
 *    _get_osver (MSVCRT.@)
 */
int CDECL _get_osver(int* value)
{
    if (!MSVCRT_CHECK_PMT(value != NULL)) return EINVAL;
    *value = MSVCRT__osver;
    return 0;
}

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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"
#include "wine/port.h"

#include <math.h>
#include "msvcrt.h"

#include "stdlib.h"
#include "string.h"

//#include "wine/library.h"
#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msvcrt);

unsigned int MSVCRT___argc;
unsigned int MSVCRT_basemajor;/* FIXME: */
unsigned int MSVCRT_baseminor;/* FIXME: */
unsigned int MSVCRT_baseversion; /* FIXME: */
unsigned int MSVCRT__commode;
unsigned int MSVCRT__fmode;
unsigned int MSVCRT_osmajor;/* FIXME: */
unsigned int MSVCRT_osminor;/* FIXME: */
unsigned int MSVCRT_osmode;/* FIXME: */
unsigned int MSVCRT__osver;
unsigned int MSVCRT_osversion; /* FIXME: */
unsigned int MSVCRT__winmajor;
unsigned int MSVCRT__winminor;
unsigned int MSVCRT__winver;
unsigned int MSVCRT__sys_nerr; /* FIXME: not accessible from Winelib apps */
char**       MSVCRT__sys_errlist; /* FIXME: not accessible from Winelib apps */
unsigned int MSVCRT___setlc_active;
unsigned int MSVCRT___unguarded_readlc_active;
double MSVCRT__HUGE;
char **MSVCRT___argv;
MSVCRT_wchar_t **MSVCRT___wargv;
char *MSVCRT__acmdln;
MSVCRT_wchar_t *MSVCRT__wcmdln;
char **MSVCRT__environ = 0;
MSVCRT_wchar_t **MSVCRT__wenviron = 0;
char **MSVCRT___initenv = 0;
MSVCRT_wchar_t **MSVCRT___winitenv = 0;
int MSVCRT_timezone;
int MSVCRT_app_type;
char* MSVCRT__pgmptr = 0;
WCHAR* MSVCRT__wpgmptr = 0;

/* Get a snapshot of the current environment
 * and construct the __p__environ array
 *
 * The pointer returned from GetEnvironmentStrings may get invalid when
 * some other module cause a reallocation of the env-variable block
 *
 * blk is an array of pointers to environment strings, ending with a NULL
 * and after that the actual copy of the environment strings, ending in a \0
 */
char ** msvcrt_SnapshotOfEnvironmentA(char **blk)
{
  char* environ_strings = GetEnvironmentStringsA();
  int count = 1, len = 1, i = 0; /* keep space for the trailing NULLS */
  char *ptr;

  for (ptr = environ_strings; *ptr; ptr += strlen(ptr) + 1)
  {
    count++;
    len += strlen(ptr) + 1;
  }
  if (blk)
      blk = HeapReAlloc( GetProcessHeap(), 0, blk, count* sizeof(char*) + len );
  else
    blk = HeapAlloc(GetProcessHeap(), 0, count* sizeof(char*) + len );

  if (blk)
    {
      if (count)
	{
	  memcpy(&blk[count],environ_strings,len);
	  for (ptr = (char*) &blk[count]; *ptr; ptr += strlen(ptr) + 1)
	    {
	      blk[i++] = ptr;
	    }
	}
      blk[i] = NULL;
    }
  FreeEnvironmentStringsA(environ_strings);
  return blk;
}

MSVCRT_wchar_t ** msvcrt_SnapshotOfEnvironmentW(MSVCRT_wchar_t **wblk)
{
  MSVCRT_wchar_t* wenviron_strings = GetEnvironmentStringsW();
  int count = 1, len = 1, i = 0; /* keep space for the trailing NULLS */
  MSVCRT_wchar_t *wptr;

  for (wptr = wenviron_strings; *wptr; wptr += strlenW(wptr) + 1)
  {
    count++;
    len += strlenW(wptr) + 1;
  }
  if (wblk)
      wblk = HeapReAlloc( GetProcessHeap(), 0, wblk, count* sizeof(MSVCRT_wchar_t*) + len * sizeof(MSVCRT_wchar_t));
  else
    wblk = HeapAlloc(GetProcessHeap(), 0, count* sizeof(MSVCRT_wchar_t*) + len * sizeof(MSVCRT_wchar_t));
  if (wblk)
    {
      if (count)
	{
	  memcpy(&wblk[count],wenviron_strings,len * sizeof(MSVCRT_wchar_t));
	  for (wptr = (MSVCRT_wchar_t*)&wblk[count]; *wptr; wptr += strlenW(wptr) + 1)
	    {
	      wblk[i++] = wptr;
	    }
	}
      wblk[i] = NULL;
    }
  FreeEnvironmentStringsW(wenviron_strings);
  return wblk;
}

typedef void (*_INITTERMFUN)(void);
#ifndef __REACTOS__
/***********************************************************************
 *		__p___argc (MSVCRT.@)
 */
int* __p___argc(void) { return &MSVCRT___argc; }

/***********************************************************************
 *		__p__commode (MSVCRT.@)
 */
unsigned int* __p__commode(void) { return &MSVCRT__commode; }

/***********************************************************************
 *              __p__pgmptr (MSVCRT.@)
 */
char** __p__pgmptr(void) { return &MSVCRT__pgmptr; }

/***********************************************************************
 *              __p__wpgmptr (MSVCRT.@)
 */
WCHAR** __p__wpgmptr(void) { return &MSVCRT__wpgmptr; }

/***********************************************************************
 *		__p__fmode (MSVCRT.@)
 */
unsigned int* __p__fmode(void) { return &MSVCRT__fmode; }

/***********************************************************************
 *		__p__osver (MSVCRT.@)
 */
unsigned int* __p__osver(void) { return &MSVCRT__osver; }

/***********************************************************************
 *		__p__winmajor (MSVCRT.@)
 */
unsigned int* __p__winmajor(void) { return &MSVCRT__winmajor; }

/***********************************************************************
 *		__p__winminor (MSVCRT.@)
 */
unsigned int* __p__winminor(void) { return &MSVCRT__winminor; }

/***********************************************************************
 *		__p__winver (MSVCRT.@)
 */
unsigned int* __p__winver(void) { return &MSVCRT__winver; }

/*********************************************************************
 *		__p__acmdln (MSVCRT.@)
 */
char** __p__acmdln(void) { return &MSVCRT__acmdln; }

/*********************************************************************
 *		__p__wcmdln (MSVCRT.@)
 */
MSVCRT_wchar_t** __p__wcmdln(void) { return &MSVCRT__wcmdln; }

/*********************************************************************
 *		__p___argv (MSVCRT.@)
 */
char*** __p___argv(void) { return &MSVCRT___argv; }

/*********************************************************************
 *		__p___wargv (MSVCRT.@)
 */
MSVCRT_wchar_t*** __p___wargv(void) { return &MSVCRT___wargv; }

/*********************************************************************
 *		__p__environ (MSVCRT.@)
 */
char*** __p__environ(void)
{
  if (!MSVCRT__environ)
    MSVCRT__environ = msvcrt_SnapshotOfEnvironmentA(NULL);
  return &MSVCRT__environ;
}

/*********************************************************************
 *		__p__wenviron (MSVCRT.@)
 */
MSVCRT_wchar_t*** __p__wenviron(void)
{
  if (!MSVCRT__wenviron)
    MSVCRT__wenviron = msvcrt_SnapshotOfEnvironmentW(NULL);
  return &MSVCRT__wenviron;
}

/*********************************************************************
 *		__p___initenv (MSVCRT.@)
 */
char*** __p___initenv(void) { return &MSVCRT___initenv; }

/*********************************************************************
 *		__p___winitenv (MSVCRT.@)
 */
MSVCRT_wchar_t*** __p___winitenv(void) { return &MSVCRT___winitenv; }

/*********************************************************************
 *		__p__timezone (MSVCRT.@)
 */
int* __p__timezone(void) { return &MSVCRT_timezone; }
#endif
/* INTERNAL: Create a wide string from an ascii string */
static MSVCRT_wchar_t *wstrdupa(const char *str)
{
  const size_t len = strlen(str) + 1 ;
  MSVCRT_wchar_t *wstr = malloc(len* sizeof (MSVCRT_wchar_t));
  if (!wstr)
    return NULL;
   MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED,str,len,wstr,len);
  return wstr;
}

/* INTERNAL: Since we can't rely on Winelib startup code calling w/getmainargs,
 * we initialise data values during DLL loading. When called by a native
 * program we simply return the data we've already initialised. This also means
 * you can call multiple times without leaking
 */
void msvcrt_init_args(void)
{
  DWORD version;

  MSVCRT__acmdln = _strdup( GetCommandLineA() );
  MSVCRT__wcmdln = wstrdupa(MSVCRT__acmdln);
  //MSVCRT___argc = __wine_main_argc;
  //MSVCRT___argv = __wine_main_argv;
  //MSVCRT___wargv = __wine_main_wargv;

  TRACE("got '%s', wide = %s argc=%d\n", MSVCRT__acmdln,
        debugstr_w(MSVCRT__wcmdln),MSVCRT___argc);

  version = GetVersion();
  MSVCRT__osver       = version >> 16;
  MSVCRT__winminor    = version & 0xFF;
  MSVCRT__winmajor    = (version>>8) & 0xFF;
  MSVCRT_baseversion = version >> 16;
  MSVCRT__winver     = ((version >> 8) & 0xFF) + ((version & 0xFF) << 8);
  MSVCRT_baseminor   = (version >> 16) & 0xFF;
  MSVCRT_basemajor   = (version >> 24) & 0xFF;
  MSVCRT_osversion   = version & 0xFFFF;
  MSVCRT_osminor     = version & 0xFF;
  MSVCRT_osmajor     = (version>>8) & 0xFF;
  MSVCRT__sys_nerr   = 43;
  MSVCRT__HUGE = HUGE_VAL;
  MSVCRT___setlc_active = 0;
  MSVCRT___unguarded_readlc_active = 0;
  MSVCRT_timezone = 0;

  MSVCRT___initenv= msvcrt_SnapshotOfEnvironmentA(NULL);
  MSVCRT___winitenv= msvcrt_SnapshotOfEnvironmentW(NULL);

  MSVCRT__pgmptr = HeapAlloc(GetProcessHeap(), 0, MAX_PATH);
  if (MSVCRT__pgmptr)
    GetModuleFileNameA(0, MSVCRT__pgmptr, MAX_PATH);

  MSVCRT__wpgmptr = HeapAlloc(GetProcessHeap(), 0, MAX_PATH * sizeof(WCHAR));
  if (MSVCRT__wpgmptr)
    GetModuleFileNameW(0, MSVCRT__wpgmptr, MAX_PATH);
}


/* INTERNAL: free memory used by args */
void msvcrt_free_args(void)
{
  /* FIXME: more things to free */
  if (MSVCRT___initenv) HeapFree(GetProcessHeap(), 0, MSVCRT___initenv);
  if (MSVCRT___winitenv) HeapFree(GetProcessHeap(), 0, MSVCRT___winitenv);
  if (MSVCRT__environ) HeapFree(GetProcessHeap(), 0, MSVCRT__environ);
  if (MSVCRT__wenviron) HeapFree(GetProcessHeap(), 0, MSVCRT__wenviron);
  if (MSVCRT__pgmptr) HeapFree(GetProcessHeap(), 0, MSVCRT__pgmptr);
  if (MSVCRT__wpgmptr) HeapFree(GetProcessHeap(), 0, MSVCRT__wpgmptr);
}
#ifndef __REACTOS__
/*********************************************************************
 *		__getmainargs (MSVCRT.@)
 */
void __getmainargs(int *argc, char** *argv, char** *envp,
                                  int expand_wildcards, int *new_mode)
{
  TRACE("(%p,%p,%p,%d,%p).\n", argc, argv, envp, expand_wildcards, new_mode);
  *argc = MSVCRT___argc;
  *argv = MSVCRT___argv;
  *envp = MSVCRT___initenv;
  if (new_mode)
    MSVCRT__set_new_mode( *new_mode );
}
#endif
/*********************************************************************
 *		__wgetmainargs (MSVCRT.@)
 */
void __wgetmainargs(int *argc, MSVCRT_wchar_t** *wargv, MSVCRT_wchar_t** *wenvp,
                    int expand_wildcards, int *new_mode)
{
  TRACE("(%p,%p,%p,%d,%p).\n", argc, wargv, wenvp, expand_wildcards, new_mode);
  *argc = MSVCRT___argc;
  *wargv = MSVCRT___wargv;
  *wenvp = MSVCRT___winitenv;
  if (new_mode)
    MSVCRT__set_new_mode( *new_mode );
}
#ifndef __REACTOS__
/*********************************************************************
 *		_initterm (MSVCRT.@)
 */
unsigned int _initterm(_INITTERMFUN *start,_INITTERMFUN *end)
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
  return 0;
}
#endif
/*********************************************************************
 *		__set_app_type (MSVCRT.@)
 */
void MSVCRT___set_app_type(int app_type)
{
  TRACE("(%d) %s application\n", app_type, app_type == 2 ? "Gui" : "Console");
  MSVCRT_app_type = app_type;
}

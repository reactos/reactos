/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the w64 mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER within this package.
 */

#ifndef _INC_INTERNAL
#define _INC_INTERNAL

#include <crtdefs.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <limits.h>
#include <windows.h>

#pragma pack(push,_CRT_PACKING)

  typedef void (__cdecl *_PVFV)(void);
  typedef int (__cdecl *_PIFV)(void);
  typedef void (__cdecl *_PVFI)(int);

#if defined (SPECIAL_CRTEXE) && defined (_DLL)
  extern int _commode;
#else
  _CRTIMP extern int _commode;
#endif

#define __IOINFO_TM_ANSI 0
#define __IOINFO_TM_UTF8 1
#define __IOINFO_TM_UTF16LE 2

  typedef struct {
    intptr_t osfhnd;
    char osfile;
    char pipech;
    int lockinitflag;
    CRITICAL_SECTION lock;
    char textmode : 7;
    char unicode : 1;
    char pipech2[2];
  } ioinfo;

#define IOINFO_ARRAY_ELTS (1 << 5)

#define _pioinfo(i) (__pioinfo[(i) >> 5] + ((i) & (IOINFO_ARRAY_ELTS - 1)))
#define _osfile(i) (_pioinfo(i)->osfile)
#define _pipech2(i) (_pioinfo(i)->pipech2)
#define _textmode(i) (_pioinfo(i)->textmode)
#define _tm_unicode(i) (_pioinfo(i)->unicode)
#define _pioinfo_safe(i) ((((i) != -1) && ((i) != -2)) ? _pioinfo(i) : &__badioinfo)
#define _osfhnd_safe(i) (_pioinfo_safe(i)->osfhnd)
#define _osfile_safe(i) (_pioinfo_safe(i)->osfile)
#define _pipech_safe(i) (_pioinfo_safe(i)->pipech)
#define _pipech2_safe(i) (_pioinfo_safe(i)->pipech2)
#define _textmode_safe(i) (_pioinfo_safe(i)->textmode)
#define _tm_unicode_safe(i) (_pioinfo_safe(i)->unicode)

#ifndef __badioinfo
  extern ioinfo ** _imp____badioinfo[];
#define __badioinfo (*_imp____badioinfo)
#endif

#ifndef __pioinfo
  extern ioinfo ** _imp____pioinfo[];
#define __pioinfo (*_imp____pioinfo)
#endif

#define _NO_CONSOLE_FILENO (intptr_t)-2

#ifndef _FILE_DEFINED
#define _FILE_DEFINED
  struct _iobuf {
    char *_ptr;
    int _cnt;
    char *_base;
    int _flag;
    int _file;
    int _charbuf;
    int _bufsiz;
    char *_tmpfname;
  };
  typedef struct _iobuf FILE;
#endif

#if !defined (_FILEX_DEFINED) && defined (_WINDOWS_)
#define _FILEX_DEFINED
  typedef struct {
    FILE f;
    CRITICAL_SECTION lock;
  } _FILEX;
#endif

  extern int _dowildcard;
  extern int _newmode;

#ifndef __winitenv
extern wchar_t ***_imp____winitenv;
#define __winitenv (*_imp____winitenv)
#endif

#ifndef __initenv
extern char ***_imp____initenv;
#define __initenv (*_imp____initenv)
#endif

#ifndef _acmdln
extern char **_imp___acmdln;
#define _acmdln (*_imp___acmdln)
/*  _CRTIMP extern char *_acmdln; */
#endif

#ifndef _wcmdln
extern char **_imp___wcmdln;
#define _wcmdln (*_imp___wcmdln)
/*  __CRTIMP extern wchar_t *_wcmdln; */
#endif

  _CRTIMP void __cdecl _amsg_exit(int);

  int __CRTDECL _setargv(void);
  int __CRTDECL __setargv(void);
  int __CRTDECL _wsetargv(void);
  int __CRTDECL __wsetargv(void);

  int __CRTDECL main(int _Argc, char **_Argv, char **_Env);
  int __CRTDECL wmain(int _Argc, wchar_t **_Argv, wchar_t **_Env);

#ifndef _STARTUP_INFO_DEFINED
#define _STARTUP_INFO_DEFINED
  typedef struct {
    int newmode;
  } _startupinfo;
#endif

  _CRTIMP int __cdecl __getmainargs(int * _Argc, char *** _Argv, char ***_Env, int _DoWildCard, _startupinfo *_StartInfo);
  _CRTIMP int __cdecl __wgetmainargs(int * _Argc, wchar_t ***_Argv, wchar_t ***_Env, int _DoWildCard, _startupinfo *_StartInfo);

#define _CONSOLE_APP 1
#define _GUI_APP 2

  typedef enum __enative_startup_state {
    __uninitialized, __initializing, __initialized
  } __enative_startup_state;

  extern volatile __enative_startup_state __native_startup_state;
  extern volatile void *__native_startup_lock;

  extern volatile unsigned int __native_dllmain_reason;
  extern volatile unsigned int __native_vcclrit_reason;

  _CRTIMP void __cdecl __set_app_type (int);

  typedef LONG NTSTATUS;

#include <crtdbg.h>
#include <errno.h>

  void * __cdecl _encode_pointer(void *);
  void * __cdecl _encoded_null();
  void * __cdecl _decode_pointer(void *);

  BOOL __cdecl _ValidateImageBase (PBYTE pImageBase);
  PIMAGE_SECTION_HEADER __cdecl _FindPESection (PBYTE pImageBase, DWORD_PTR rva);
  BOOL __cdecl _IsNonwritableInCurrentImage (PBYTE pTarget);

  extern int __globallocalestatus;

#ifdef __cplusplus
}
#endif

#pragma pack(pop)
#endif

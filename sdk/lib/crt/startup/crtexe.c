/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the w64 mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER.PD within this package.
 */

#define SPECIAL_CRTEXE

#include <oscalls.h>
#include <internal.h>
#include <process.h>
#include <signal.h>
#include <math.h>
#include <stdlib.h>
#include <tchar.h>
#include <sect_attribs.h>
#include <locale.h>
#ifdef _MBCS
#include <mbstring.h>
#endif

/* Special handling for ARM & ARM64, __winitenv & __initenv aren't present there. */
#if !defined(__arm__) && !defined(__aarch64__)
_CRTIMP extern wchar_t** __winitenv;
_CRTIMP extern char** __initenv;
#endif

/* Hack, for bug in ld.  Will be removed soon.  */
#if defined(__GNUC__)
#define __ImageBase __MINGW_LSYMBOL(_image_base__)
#endif

/* This symbol is defined by ld.  */
extern IMAGE_DOS_HEADER __ImageBase;

extern void __cdecl _fpreset (void);
#define SPACECHAR _T(' ')
#define DQUOTECHAR _T('\"')

extern int _dowildcard;

extern _CRTIMP void __cdecl _initterm(_PVFV *, _PVFV *);

static int __cdecl check_managed_app (void);

extern _CRTALLOC(".CRT$XIA") _PIFV __xi_a[];
extern _CRTALLOC(".CRT$XIZ") _PIFV __xi_z[];
extern _CRTALLOC(".CRT$XCA") _PVFV __xc_a[];
extern _CRTALLOC(".CRT$XCZ") _PVFV __xc_z[];

/* TLS initialization hook.  */
extern const PIMAGE_TLS_CALLBACK __dyn_tls_init_callback;

extern _PVFV *__onexitbegin;
extern _PVFV *__onexitend;

extern int mingw_app_type;

HINSTANCE __mingw_winmain_hInstance;
_TCHAR *__mingw_winmain_lpCmdLine;
DWORD __mingw_winmain_nShowCmd;

static int argc;
extern void __main(void);
#ifdef WPRFLAG
static wchar_t **argv;
static wchar_t **envp;
#else
static char **argv;
static char **envp;
#endif

static int argret;
static int mainret=0;
static int managedapp;
static int has_cctor = 0;
static _startupinfo startinfo;
extern LPTOP_LEVEL_EXCEPTION_FILTER __mingw_oldexcpt_handler;

extern void _pei386_runtime_relocator (void);
long CALLBACK _gnu_exception_handler (EXCEPTION_POINTERS * exception_data);
#ifdef WPRFLAG
static void duplicate_ppstrings (int ac, wchar_t ***av);
#else
static void duplicate_ppstrings (int ac, char ***av);
#endif

static int __cdecl pre_c_init (void);
static void __cdecl pre_cpp_init (void);
static void __cdecl __mingw_prepare_except_for_msvcr80_and_higher (void);
_CRTALLOC(".CRT$XIAA") _PIFV mingw_pcinit = pre_c_init;
_CRTALLOC(".CRT$XCAA") _PVFV mingw_pcppinit = pre_cpp_init;

extern int _MINGW_INSTALL_DEBUG_MATHERR;

#ifdef __GNUC__
extern void __do_global_dtors(void);
#endif

static int __cdecl
pre_c_init (void)
{
  managedapp = check_managed_app ();
  if (mingw_app_type)
    __set_app_type(_GUI_APP);
  else
    __set_app_type (_CONSOLE_APP);
  __onexitbegin = __onexitend = (_PVFV *)(-1);

#ifdef WPRFLAG
  _wsetargv();
#else
  _setargv();
#endif
  if (_MINGW_INSTALL_DEBUG_MATHERR == 1)
    {
      __setusermatherr (_matherr);
    }
#if !defined(__clang__) && (!defined(_M_ARM64) || (_MSC_VER < 1930)) /* FIXME: CORE-14042 */
  if (__globallocalestatus == -1)
    {
    }
#endif
  return 0;
}

static void __cdecl
pre_cpp_init (void)
{
  startinfo.newmode = _newmode;

#ifdef WPRFLAG
  argret = __wgetmainargs(&argc,&argv,&envp,_dowildcard,&startinfo);
#else
  argret = __getmainargs(&argc,&argv,&envp,_dowildcard,&startinfo);
#endif
}

static int __cdecl __tmainCRTStartup (void);

int __cdecl WinMainCRTStartup (void);

int __cdecl WinMainCRTStartup (void)
{
  int ret = 255;
#ifdef __SEH__
  asm ("\t.l_startw:\n"
    "\t.seh_handler __C_specific_handler, @except\n"
    "\t.seh_handlerdata\n"
    "\t.long 1\n"
    "\t.rva .l_startw, .l_endw, _gnu_exception_handler ,.l_endw\n"
    "\t.text"
    );
#endif
  mingw_app_type = 1;
  __security_init_cookie ();
  ret = __tmainCRTStartup ();
#ifdef __SEH__
  asm ("\tnop\n"
    "\t.l_endw: nop\n");
#endif
  return ret;
}

int __cdecl mainCRTStartup (void);
BOOL crt_process_init(void);

#ifdef _WIN64
int __mingw_init_ehandler (void);
#endif

int __cdecl mainCRTStartup (void)
{
  int ret = 255;
#ifndef _DLL
  if (!crt_process_init())
  {
      return -1;
  }
#endif
#ifdef __SEH__
  asm ("\t.l_start:\n"
    "\t.seh_handler __C_specific_handler, @except\n"
    "\t.seh_handlerdata\n"
    "\t.long 1\n"
    "\t.rva .l_start, .l_end, _gnu_exception_handler ,.l_end\n"
    "\t.text"
    );
#endif
  mingw_app_type = 0;
  __security_init_cookie ();
  ret = __tmainCRTStartup ();
#ifdef __SEH__
  asm ("\tnop\n"
    "\t.l_end: nop\n");
#endif
  return ret;
}

static
__declspec(noinline)
int __cdecl
__tmainCRTStartup (void)
{
  _TCHAR *lpszCommandLine = NULL;
  STARTUPINFO StartupInfo;
  WINBOOL inDoubleQuote = FALSE;
  memset (&StartupInfo, 0, sizeof (STARTUPINFO));

#ifndef _WIN64
  /* We need to make sure that this function is build with frame-pointer
     and that we align the stack to 16 bytes for the sake of SSE ops in main
     or in functions inlined into main.  */
  lpszCommandLine = (_TCHAR *) _alloca (32);
  memset (lpszCommandLine, 0xcc, 32);
#ifdef __GNUC__
  asm  __volatile__  ("andl $-16, %%esp" : : : "%esp");
#endif
#endif

  if (mingw_app_type)
    GetStartupInfo (&StartupInfo);
  {
    void *lock_free = NULL;
    void *fiberid = ((PNT_TIB)NtCurrentTeb())->StackBase;
    int nested = FALSE;
    while((lock_free = InterlockedCompareExchangePointer ((volatile PVOID *) &__native_startup_lock,
							  fiberid, 0)) != 0)
      {
	if (lock_free == fiberid)
	  {
	    nested = TRUE;
	    break;
	  }
	Sleep(1000);
      }
    if (__native_startup_state == __initializing)
      {
	_amsg_exit (31);
      }
    else if (__native_startup_state == __uninitialized)
      {
	__native_startup_state = __initializing;
	_initterm ((_PVFV *)(void *)__xi_a, (_PVFV *)(void *) __xi_z);
      }
    else
      has_cctor = 1;

    if (__native_startup_state == __initializing)
      {
	_initterm (__xc_a, __xc_z);
	__native_startup_state = __initialized;
      }
    _ASSERTE(__native_startup_state == __initialized);
    if (! nested)
      (VOID)InterlockedExchangePointer ((volatile PVOID *) &__native_startup_lock, 0);

    if (__dyn_tls_init_callback != NULL)
      __dyn_tls_init_callback (NULL, DLL_THREAD_ATTACH, NULL);

    _pei386_runtime_relocator ();
    __mingw_oldexcpt_handler = SetUnhandledExceptionFilter (_gnu_exception_handler);
#ifdef _WIN64
    __mingw_init_ehandler ();
#endif
    __mingw_prepare_except_for_msvcr80_and_higher ();

    _fpreset ();

    if (mingw_app_type)
      {
#ifdef WPRFLAG
	lpszCommandLine = (_TCHAR *) _wcmdln;
#else
	lpszCommandLine = (char *) _acmdln;
#endif
	while (*lpszCommandLine > SPACECHAR || (*lpszCommandLine && inDoubleQuote))
	  {
	    if (*lpszCommandLine == DQUOTECHAR)
	      inDoubleQuote = !inDoubleQuote;
#ifdef _MBCS
	    if (_ismbblead (*lpszCommandLine))
	      {
		if (lpszCommandLine) /* FIXME: Why this check? Should I check for *lpszCommandLine != 0 too? */
		  lpszCommandLine++;
	      }
#endif
	    ++lpszCommandLine;
	  }
	while (*lpszCommandLine && (*lpszCommandLine <= SPACECHAR))
	  lpszCommandLine++;

	__mingw_winmain_hInstance = (HINSTANCE) &__ImageBase;
	__mingw_winmain_lpCmdLine = lpszCommandLine;
	__mingw_winmain_nShowCmd = StartupInfo.dwFlags & STARTF_USESHOWWINDOW ?
				    StartupInfo.wShowWindow : SW_SHOWDEFAULT;
      }
    duplicate_ppstrings (argc, &argv);
    __main ();
#ifdef WPRFLAG
#if !defined(__arm__) && !defined(__aarch64__)
    __winitenv = envp;
#endif
    /* C++ initialization.
       gcc inserts this call automatically for a function called main, but not for wmain.  */
    mainret = wmain (argc, argv, envp);
#else
#if !defined(__arm__) && !defined(__aarch64__)
    __initenv = envp;
#endif
    mainret = main (argc, argv, envp);
#endif

#ifdef __GNUC__
    __do_global_dtors();
#endif

    if (!managedapp)
      exit (mainret);

    if (has_cctor == 0)
      _cexit ();
  }
  return mainret;
}

extern int mingw_initltsdrot_force;
extern int mingw_initltsdyn_force;
extern int mingw_initltssuo_force;
extern int mingw_initcharmax;

static int __cdecl
check_managed_app (void)
{
  PIMAGE_DOS_HEADER pDOSHeader;
  PIMAGE_NT_HEADERS pPEHeader;
  PIMAGE_OPTIONAL_HEADER32 pNTHeader32;
  PIMAGE_OPTIONAL_HEADER64 pNTHeader64;

  /* Force to be linked.  */
  mingw_initltsdrot_force=1;
  mingw_initltsdyn_force=1;
  mingw_initltssuo_force=1;
  mingw_initcharmax=1;

  pDOSHeader = (PIMAGE_DOS_HEADER) &__ImageBase;
  if (pDOSHeader->e_magic != IMAGE_DOS_SIGNATURE)
    return 0;

  pPEHeader = (PIMAGE_NT_HEADERS)((char *)pDOSHeader + pDOSHeader->e_lfanew);
  if (pPEHeader->Signature != IMAGE_NT_SIGNATURE)
    return 0;

  pNTHeader32 = (PIMAGE_OPTIONAL_HEADER32) &pPEHeader->OptionalHeader;
  switch (pNTHeader32->Magic)
    {
    case IMAGE_NT_OPTIONAL_HDR32_MAGIC:
      if (pNTHeader32->NumberOfRvaAndSizes <= IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR)
	return 0;
      return !! pNTHeader32->DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR].VirtualAddress;
    case IMAGE_NT_OPTIONAL_HDR64_MAGIC:
      pNTHeader64 = (PIMAGE_OPTIONAL_HEADER64)pNTHeader32;
      if (pNTHeader64->NumberOfRvaAndSizes <= IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR)
	return 0;
      return !! pNTHeader64->DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR].VirtualAddress;
    }
  return 0;
}

#ifdef WPRFLAG
static size_t wbytelen(const wchar_t *p)
{
	size_t ret = 1;
	while (*p!=0) {
		ret++,++p;
	}
	return ret*2;
}
static void duplicate_ppstrings (int ac, wchar_t ***av)
{
	wchar_t **avl;
	int i;
	wchar_t **n = (wchar_t **) malloc (sizeof (wchar_t *) * (ac + 1));

	avl=*av;
	for (i=0; i < ac; i++)
	  {
		size_t l = wbytelen (avl[i]);
		n[i] = (wchar_t *) malloc (l);
		memcpy (n[i], avl[i], l);
	  }
	n[i] = NULL;
	*av = n;
}
#else
static void duplicate_ppstrings (int ac, char ***av)
{
	char **avl;
	int i;
	char **n = (char **) malloc (sizeof (char *) * (ac + 1));

	avl=*av;
	for (i=0; i < ac; i++)
	  {
		size_t l = strlen (avl[i]) + 1;
		n[i] = (char *) malloc (l);
		memcpy (n[i], avl[i], l);
	  }
	n[i] = NULL;
	*av = n;
}
#endif

#ifdef __MINGW_SHOW_INVALID_PARAMETER_EXCEPTION
#define __UNUSED_PARAM_1(x) x
#else
#define __UNUSED_PARAM_1	__UNUSED_PARAM
#endif
static void __cdecl
__mingw_invalidParameterHandler (const wchar_t * __UNUSED_PARAM_1(expression),
				 const wchar_t * __UNUSED_PARAM_1(function),
				 const wchar_t * __UNUSED_PARAM_1(file),
				 unsigned int    __UNUSED_PARAM_1(line),
				 uintptr_t __UNUSED_PARAM(pReserved))
{
#ifdef __MINGW_SHOW_INVALID_PARAMETER_EXCEPTION
  wprintf(L"Invalid parameter detected in function %s. File: %s Line: %u\n", function, file, line);
  wprintf(L"Expression: %s\n", expression);
#endif
}

HANDLE __mingw_get_msvcrt_handle(void);

static void __cdecl
__mingw_prepare_except_for_msvcr80_and_higher (void)
{
  _invalid_parameter_handler (*fIPH)(_invalid_parameter_handler) = NULL;

  fIPH = (void*)GetProcAddress (__mingw_get_msvcrt_handle(), "_set_invalid_parameter_handler");
  if (fIPH)
    (*fIPH)(__mingw_invalidParameterHandler);
}

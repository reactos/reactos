/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the w64 mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER within this package.
 */

#undef CRTDLL
#ifndef _DLL
#define _DLL
#endif

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

#ifndef __winitenv
extern wchar_t ***_imp____winitenv;
#define __winitenv (*_imp____winitenv)
#endif

#ifndef __initenv
extern char ***_imp____initenv;
#define __initenv (*_imp____initenv)
#endif

/* Hack, for bug in ld.  Will be removed soon.  */
#define __ImageBase _image_base__
/* This symbol is defined by ld.  */
extern IMAGE_DOS_HEADER __ImageBase;

extern void _fpreset (void);
#define SPACECHAR _T(' ')
#define DQUOTECHAR _T('\"')

__declspec(dllimport) void __setusermatherr(int (__cdecl *)(struct _exception *));

extern int *_imp___fmode;
extern int *_imp___commode;

#undef _fmode
extern int _fmode;
extern int *_imp___commode;
#define _commode (*_imp___commode)
extern int _dowildcard;

int _MINGW_INSTALL_DEBUG_MATHERR __attribute__((weak)) = 0;
extern int __defaultmatherr;
extern _CRTIMP void __cdecl _initterm(_PVFV *, _PVFV *);

static int __cdecl check_managed_app (void);

extern _CRTALLOC(".CRT$XIA") _PIFV __xi_a[];
extern _CRTALLOC(".CRT$XIZ") _PIFV __xi_z[];
extern _CRTALLOC(".CRT$XCA") _PVFV __xc_a[];
extern _CRTALLOC(".CRT$XCZ") _PVFV __xc_z[];

extern const PIMAGE_TLS_CALLBACK __dyn_tls_init_callback;

extern _PVFV *__onexitbegin;
extern _PVFV *__onexitend;

extern int mingw_app_type;

static int argc;
#ifdef WPRFLAG
extern void __main(void);
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

extern void _pei386_runtime_relocator (void);
static CALLBACK long _gnu_exception_handler (EXCEPTION_POINTERS * exception_data);
static LONG __mingw_vex(EXCEPTION_POINTERS * exception_data);
#ifdef WPRFLAG
static void duplicate_ppstrings (int ac, wchar_t ***av);
#else
static void duplicate_ppstrings (int ac, char ***av);
#endif

static int __cdecl pre_c_init (void);
static void __cdecl pre_cpp_init (void);

_CRTALLOC(".CRT$XIAA") _PIFV mingw_pcinit = pre_c_init;
_CRTALLOC(".CRT$XCAA") _PVFV mingw_pcppinit = pre_cpp_init;

static int __cdecl
pre_c_init (void)
{
  managedapp = check_managed_app ();
  if (mingw_app_type)
    __set_app_type(_GUI_APP);
  else
    __set_app_type (_CONSOLE_APP);
  __onexitbegin = __onexitend = (_PVFV *) _encode_pointer ((_PVFV *)(-1));

  *_imp___fmode = _fmode;
  *_imp___commode = _commode;

#ifdef WPRFLAG
  _wsetargv();
#else
  _setargv();
#endif
  if (_MINGW_INSTALL_DEBUG_MATHERR)
    {
      if (! __defaultmatherr)
	{
	  __setusermatherr (_matherr);
	  __defaultmatherr = 1;
	}
    }

  if (__globallocalestatus == -1)
    {
    }
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

static int __tmainCRTStartup (void);

int WinMainCRTStartup (void)
{
  mingw_app_type = 1;
  __security_init_cookie ();
  return __tmainCRTStartup ();
}

int mainCRTStartup (void)
{
  mingw_app_type = 0;
  __security_init_cookie ();
  return __tmainCRTStartup ();
}


__declspec(noinline) int
__tmainCRTStartup (void)
{
  _TCHAR *lpszCommandLine = NULL;
  STARTUPINFO StartupInfo;
  BOOL inDoubleQuote = FALSE;
  memset (&StartupInfo, 0, sizeof (STARTUPINFO));
  
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
    
    if (__dyn_tls_init_callback != NULL && _IsNonwritableInCurrentImage ((PBYTE) &__dyn_tls_init_callback))
      __dyn_tls_init_callback (NULL, DLL_THREAD_ATTACH, NULL);
    
    _pei386_runtime_relocator ();
    
    #ifdef _WIN64
    __asm__ __volatile__ (
	"xorq %rax,%rax\n\t"
	"decq %rax\n\t"
	"movq %rax,%gs:0" "\n");
    #else
    __asm__ __volatile__ (
	"xorl %eax,%eax\n\t"
	"decl %eax\n\t"
	"movl %eax,%fs:0" "\n");
    #endif
    AddVectoredExceptionHandler (0, (PVECTORED_EXCEPTION_HANDLER)__mingw_vex);
    SetUnhandledExceptionFilter (_gnu_exception_handler);
    
    _fpreset ();

    if (mingw_app_type)
      {
#ifdef WPRFLAG
    lpszCommandLine = (_TCHAR *) _wcmdln;
#else
    lpszCommandLine = (char *) _acmdln;
#endif
    while (*lpszCommandLine > SPACECHAR || (*lpszCommandLine&&inDoubleQuote))
      {
	if (*lpszCommandLine == DQUOTECHAR)
	  inDoubleQuote = !inDoubleQuote;
#ifdef _MBCS
	if (_ismbblead (*lpszCommandLine))
	  {
	    if (lpszCommandLine)
	      lpszCommandLine++;
	  }
#endif
	++lpszCommandLine;
      }
    while (*lpszCommandLine && (*lpszCommandLine <= SPACECHAR))
      lpszCommandLine++;

#ifdef WPRFLAG
    /* C++ initialization.
       gcc inserts this call automatically for a function called main, but not for wmain.  */
    __main ();
    mainret = wmain (
    	(int) (StartupInfo.dwFlags & STARTF_USESHOWWINDOW ? StartupInfo.wShowWindow : SW_SHOWDEFAULT),
    	(wchar_t **) lpszCommandLine, (wchar_t **) (HINSTANCE) &__ImageBase);
#else
    mainret = main (
    	(int) (StartupInfo.dwFlags & STARTF_USESHOWWINDOW ? StartupInfo.wShowWindow : SW_SHOWDEFAULT),
    	(char **) lpszCommandLine, (char **) (HINSTANCE) &__ImageBase);
#endif
    }
  else
    {
    duplicate_ppstrings (argc, &argv);
#ifdef WPRFLAG
    __winitenv = envp;
    /* C++ initialization.
       gcc inserts this call automatically for a function called main, but not for wmain.  */
    __main ();
    mainret = wmain (argc, argv, envp);
#else
    __initenv = envp;
    mainret = main (argc, argv, envp);
#endif
    }
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

static CALLBACK long
_gnu_exception_handler (EXCEPTION_POINTERS * exception_data)
{
  void (*old_handler) (int);
  long action = EXCEPTION_CONTINUE_SEARCH;
  int reset_fpu = 0;

  switch (exception_data->ExceptionRecord->ExceptionCode)
    {
    case EXCEPTION_ACCESS_VIOLATION:
      /* test if the user has set SIGSEGV */
      old_handler = signal (SIGSEGV, SIG_DFL);
      if (old_handler == SIG_IGN)
	{
	  /* this is undefined if the signal was raised by anything other
	     than raise ().  */
	  signal (SIGSEGV, SIG_IGN);
	  action = EXCEPTION_CONTINUE_EXECUTION;
	}
      else if (old_handler != SIG_DFL)
	{
	  /* This means 'old' is a user defined function. Call it */
	  (*old_handler) (SIGSEGV);
	  action = EXCEPTION_CONTINUE_EXECUTION;
	}
      break;

    case EXCEPTION_ILLEGAL_INSTRUCTION:
    case EXCEPTION_PRIV_INSTRUCTION:
      /* test if the user has set SIGILL */
      old_handler = signal (SIGILL, SIG_DFL);
      if (old_handler == SIG_IGN)
	{
	  /* this is undefined if the signal was raised by anything other
	     than raise ().  */
	  signal (SIGILL, SIG_IGN);
	  action = EXCEPTION_CONTINUE_EXECUTION;
	}
      else if (old_handler != SIG_DFL)
	{
	  /* This means 'old' is a user defined function. Call it */
	  (*old_handler) (SIGILL);
	  action = EXCEPTION_CONTINUE_EXECUTION;
	}
      break;

    case EXCEPTION_FLT_INVALID_OPERATION:
    case EXCEPTION_FLT_DIVIDE_BY_ZERO:
    case EXCEPTION_FLT_DENORMAL_OPERAND:
    case EXCEPTION_FLT_OVERFLOW:
    case EXCEPTION_FLT_UNDERFLOW:
    case EXCEPTION_FLT_INEXACT_RESULT:
      reset_fpu = 1;
      /* fall through. */

    case EXCEPTION_INT_DIVIDE_BY_ZERO:
      /* test if the user has set SIGFPE */
      old_handler = signal (SIGFPE, SIG_DFL);
      if (old_handler == SIG_IGN)
	{
	  signal (SIGFPE, SIG_IGN);
	  if (reset_fpu)
	    _fpreset ();
	  action = EXCEPTION_CONTINUE_EXECUTION;
	}
      else if (old_handler != SIG_DFL)
	{
	  /* This means 'old' is a user defined function. Call it */
	  (*old_handler) (SIGFPE);
	  action = EXCEPTION_CONTINUE_EXECUTION;
	}
      break;

    default:
      break;
    }
  return action;
}

static LONG __mingw_vex(EXCEPTION_POINTERS * exception_data)
{
  /* TODO this is not chainablem, therefore need rewrite. Disabled the ill code. */
  #if 0
  #ifdef _WIN64
  __asm__ __volatile__ (
      "movq %gs:0,%rax" "\n\t"
      "orq %rax,%rax\n\t"
      "jz l1\n\t"
      "jmp *8(%rax)\n\r"
      "l1:\n\t"
      "nop\n");
#else
  __asm__ __volatile__ (
      "movl %fs:0,%eax" "\n\t"
      "orl %eax,%eax\n\t"
      "jz l1\n\t"
      "jmp *4(%eax)\n\r"
      "l1:\n\t"
      "nop\n");
#endif
#endif
  return _gnu_exception_handler(exception_data);
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
		int l = wbytelen (avl[i]);
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
		int l = strlen (avl[i]) + 1;
		n[i] = (char *) malloc (l);
		memcpy (n[i], avl[i], l);
	  }
	n[i] = NULL;
	*av = n;
}
#endif

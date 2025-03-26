/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the w64 mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER.PD within this package.
 */
#define _NTSYSTEM_
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#define WIN32_NO_STATUS
#include <stdlib.h>	/* abort () */
//#include <windows.h>
#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#undef  WIN32_NO_STATUS
#include <ntstatus.h>	/* STATUS macros */
#ifdef _WIN64
#include <intrin.h>
#endif

#ifdef _WIN64
#define DEFAULT_SECURITY_COOKIE 0x00002B992DDFA232ll
#else
#define DEFAULT_SECURITY_COOKIE 0xBB40E64E
#endif

/* Externals.  */

typedef LONG NTSTATUS;	/* same as in ntdef.h / winternl.h */

#define UNW_FLAG_NHANDLER 0x00

typedef union
{
  unsigned __int64 ft_scalar;
  FILETIME ft_struct;
} FT;

#ifndef _MSC_VER
static EXCEPTION_RECORD GS_ExceptionRecord;
static CONTEXT GS_ContextRecord;

static const EXCEPTION_POINTERS GS_ExceptionPointers = {
  &GS_ExceptionRecord,&GS_ContextRecord
};
#endif

DECLSPEC_SELECTANY UINT_PTR __security_cookie = DEFAULT_SECURITY_COOKIE;
DECLSPEC_SELECTANY UINT_PTR __security_cookie_complement = ~(DEFAULT_SECURITY_COOKIE);

void __cdecl __security_init_cookie (void);

void __cdecl
__security_init_cookie (void)
{
  UINT_PTR cookie;
  FT systime = { 0, };
  LARGE_INTEGER perfctr;

  if (__security_cookie != DEFAULT_SECURITY_COOKIE)
    {
      __security_cookie_complement = ~__security_cookie;
      return;
    }

  GetSystemTimeAsFileTime (&systime.ft_struct);
#ifdef _WIN64
  cookie = systime.ft_scalar;
#else
  cookie = systime.ft_struct.dwLowDateTime;
  cookie ^= systime.ft_struct.dwHighDateTime;
#endif

  cookie ^= GetCurrentProcessId ();
  cookie ^= GetCurrentThreadId ();
  cookie ^= GetTickCount ();

  QueryPerformanceCounter (&perfctr);
#ifdef _WIN64
  cookie ^= perfctr.QuadPart;
#else
  cookie ^= perfctr.LowPart;
  cookie ^= perfctr.HighPart;
#endif

#ifdef _WIN64
  cookie &= 0x0000ffffffffffffll;
#endif

  if (cookie == DEFAULT_SECURITY_COOKIE)
    cookie = DEFAULT_SECURITY_COOKIE + 1;
  __security_cookie = cookie;
  __security_cookie_complement = ~cookie;
}


#if defined(__GNUC__) /* wrap msvc intrinsics onto gcc builtins */
#undef  _ReturnAddress
#undef  _AddressOfReturnAddress
#define _ReturnAddress()		__builtin_return_address(0)
#define _AddressOfReturnAddress()	__builtin_frame_address (0)
#endif /* __GNUC__ */

__declspec(noreturn) void __cdecl __report_gsfailure (ULONG_PTR);

#ifndef _MSC_VER
__declspec(noreturn) void __cdecl
__report_gsfailure (ULONG_PTR StackCookie)
{
  volatile UINT_PTR cookie[2] __MINGW_ATTRIB_UNUSED;
#ifdef _WIN64
  ULONG64 controlPC, imgBase, establisherFrame;
  PRUNTIME_FUNCTION fctEntry;
  PVOID hndData;

  RtlCaptureContext (&GS_ContextRecord);
  controlPC = GS_ContextRecord.Rip;
  fctEntry = RtlLookupFunctionEntry(controlPC, &imgBase, NULL);
  if (fctEntry != NULL)
    {
      RtlVirtualUnwind (UNW_FLAG_NHANDLER, imgBase, controlPC, fctEntry,
			&GS_ContextRecord, &hndData, &establisherFrame, NULL);
    }
  else
#endif /* _WIN64 */
    {
#ifdef _WIN64
      GS_ContextRecord.Rip = (ULONGLONG) _ReturnAddress();
      GS_ContextRecord.Rsp = (ULONGLONG) _AddressOfReturnAddress() + 8;
#else
      GS_ContextRecord.Eip = (DWORD) _ReturnAddress();
      GS_ContextRecord.Esp = (DWORD) _AddressOfReturnAddress() + 4;
#endif /* _WIN64 */
    }

#ifdef _WIN64
  GS_ExceptionRecord.ExceptionAddress = (PVOID) GS_ContextRecord.Rip;
  GS_ContextRecord.Rcx = StackCookie;
#else
  GS_ExceptionRecord.ExceptionAddress = (PVOID) GS_ContextRecord.Eip;
  GS_ContextRecord.Ecx = StackCookie;
#endif /* _WIN64 */
  GS_ExceptionRecord.ExceptionCode = STATUS_STACK_BUFFER_OVERRUN;
  GS_ExceptionRecord.ExceptionFlags = EXCEPTION_NONCONTINUABLE;
  cookie[0] = __security_cookie;
  cookie[1] = __security_cookie_complement;
  SetUnhandledExceptionFilter (NULL);
  UnhandledExceptionFilter ((EXCEPTION_POINTERS *) &GS_ExceptionPointers);
  TerminateProcess (GetCurrentProcess (), STATUS_STACK_BUFFER_OVERRUN);
  abort();
}
#endif /* !_MSC_VER */

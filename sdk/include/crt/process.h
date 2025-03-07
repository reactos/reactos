/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the w64 mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER within this package.
 */
#ifndef _INC_PROCESS
#define _INC_PROCESS

#include <corecrt.h>

/* Includes a definition of _pid_t and pid_t */
#include <sys/types.h>

#ifndef _POSIX_

#ifdef __cplusplus
extern "C" {
#endif

#define _P_WAIT 0
#define _P_NOWAIT 1
#define _OLD_P_OVERLAY 2
#define _P_NOWAITO 3
#define _P_DETACH 4
#define _P_OVERLAY 2

#define _WAIT_CHILD 0
#define _WAIT_GRANDCHILD 1

  _CRTIMP
  uintptr_t
  __cdecl
  _beginthread(
    _In_ void(__cdecl *_StartAddress) (void *),
    _In_ unsigned _StackSize,
    _In_opt_ void *_ArgList);

  _CRTIMP void __cdecl _endthread(void);

  _CRTIMP
  uintptr_t
  __cdecl
  _beginthreadex(
    _In_opt_ void *_Security,
    _In_ unsigned _StackSize,
    _In_ unsigned(__stdcall *_StartAddress) (void *),
    _In_opt_ void *_ArgList,
    _In_ unsigned _InitFlag,
    _Out_opt_ unsigned *_ThrdAddr);

  _CRTIMP void __cdecl _endthreadex(_In_ unsigned _Retval);

#ifndef _CRT_TERMINATE_DEFINED
#define _CRT_TERMINATE_DEFINED
  __declspec(noreturn) void __cdecl exit(_In_ int _Code);
  _CRTIMP __declspec(noreturn) void __cdecl _exit(_In_ int _Code);
#if !defined __NO_ISOCEXT /* extern stub in static libmingwex.a */
  /* C99 function name */
  __declspec(noreturn) void __cdecl _Exit(int); /* Declare to get noreturn attribute.  */
  __CRT_INLINE void __cdecl _Exit(int status)
  {  _exit(status); }
#endif
#if __MINGW_GNUC_PREREQ(4,4)
#pragma push_macro("abort")
#undef abort
#endif
  __declspec(noreturn) void __cdecl abort(void);
#if __MINGW_GNUC_PREREQ(4,4)
#pragma pop_macro("abort")
#endif
#endif

  __analysis_noreturn _CRTIMP void __cdecl _cexit(void);
  __analysis_noreturn _CRTIMP void __cdecl _c_exit(void);
  _CRTIMP int __cdecl _getpid(void);

  _CRTIMP
  intptr_t
  __cdecl
  _cwait(
    _Out_opt_ int *_TermStat,
    _In_ intptr_t _ProcHandle,
    _In_ int _Action);

  _CRTIMP
  intptr_t
  __cdecl
  _execl(
    _In_z_ const char *_Filename,
    _In_z_ const char *_ArgList,
    ...);

  _CRTIMP
  intptr_t
  __cdecl
  _execle(
    _In_z_ const char *_Filename,
    _In_z_ const char *_ArgList,
    ...);

  _CRTIMP
  intptr_t
  __cdecl
  _execlp(
    _In_z_ const char *_Filename,
    _In_z_ const char *_ArgList,
    ...);

  _CRTIMP
  intptr_t
  __cdecl
  _execlpe(
    _In_z_ const char *_Filename,
    _In_z_ const char *_ArgList,
    ...);

  _CRTIMP
  intptr_t
  __cdecl
  _execv(
    _In_z_ const char *_Filename,
    _In_z_ const char *const *_ArgList);

  _CRTIMP
  intptr_t
  __cdecl
  _execve(
    _In_z_ const char *_Filename,
    _In_z_ const char *const *_ArgList,
    _In_opt_z_ const char *const *_Env);

  _CRTIMP
  intptr_t
  __cdecl
  _execvp(
    _In_z_ const char *_Filename,
    _In_z_ const char *const *_ArgList);

  _CRTIMP
  intptr_t
  __cdecl
  _execvpe(
    _In_z_ const char *_Filename,
    _In_z_ const char *const *_ArgList,
    _In_opt_z_ const char *const *_Env);

  _CRTIMP
  intptr_t
  __cdecl
  _spawnl(
    _In_ int _Mode,
    _In_z_ const char *_Filename,
    _In_z_ const char *_ArgList,
    ...);

  _CRTIMP
  intptr_t
  __cdecl
  _spawnle(
    _In_ int _Mode,
    _In_z_ const char *_Filename,
    _In_z_ const char *_ArgList,
    ...);

  _CRTIMP
  intptr_t
  __cdecl
  _spawnlp(
    _In_ int _Mode,
    _In_z_ const char *_Filename,
    _In_z_ const char *_ArgList,
    ...);

  _CRTIMP
  intptr_t
  __cdecl
  _spawnlpe(
    _In_ int _Mode,
    _In_z_ const char *_Filename,
    _In_z_ const char *_ArgList,
    ...);

  _CRTIMP
  intptr_t
  __cdecl
  _spawnv(
    _In_ int _Mode,
    _In_z_ const char *_Filename,
    _In_z_ const char *const *_ArgList);

  _CRTIMP
  intptr_t
  __cdecl
  _spawnve(
    _In_ int _Mode,
    _In_z_ const char *_Filename,
    _In_z_ const char *const *_ArgList,
    _In_opt_z_ const char *const *_Env);

  _CRTIMP
  intptr_t
  __cdecl
  _spawnvp(
    _In_ int _Mode,
    _In_z_ const char *_Filename,
    _In_z_ const char *const *_ArgList);

  _CRTIMP
  intptr_t
  __cdecl
  _spawnvpe(
    _In_ int _Mode,
    _In_z_ const char *_Filename,
    _In_z_ const char *const *_ArgList,
    _In_opt_z_ const char *const *_Env);

#ifndef _CRT_SYSTEM_DEFINED
#define _CRT_SYSTEM_DEFINED
  int __cdecl system(_In_opt_z_ const char *_Command);
#endif

#ifndef _WPROCESS_DEFINED
#define _WPROCESS_DEFINED

  _CRTIMP
  intptr_t
  __cdecl
  _wexecl(
    _In_z_ const wchar_t *_Filename,
    _In_z_ const wchar_t *_ArgList,
    ...);

  _CRTIMP
  intptr_t
  __cdecl
  _wexecle(
    _In_z_ const wchar_t *_Filename,
    _In_z_ const wchar_t *_ArgList,
    ...);

  _CRTIMP
  intptr_t
  __cdecl
  _wexeclp(
    _In_z_ const wchar_t *_Filename,
    _In_z_ const wchar_t *_ArgList,
    ...);

  _CRTIMP
  intptr_t
  __cdecl
  _wexeclpe(
    _In_z_ const wchar_t *_Filename,
    _In_z_ const wchar_t *_ArgList,
    ...);

  _CRTIMP
  intptr_t
  __cdecl
  _wexecv(
    _In_z_ const wchar_t *_Filename,
    _In_z_ const wchar_t *const *_ArgList);

  _CRTIMP
  intptr_t
  __cdecl
  _wexecve(
    _In_z_ const wchar_t *_Filename,
    _In_z_ const wchar_t *const *_ArgList,
    _In_opt_z_ const wchar_t *const *_Env);

  _CRTIMP
  intptr_t
  __cdecl
  _wexecvp(
    _In_z_ const wchar_t *_Filename,
    _In_z_ const wchar_t *const *_ArgList);

  _CRTIMP
  intptr_t
  __cdecl
  _wexecvpe(
    _In_z_ const wchar_t *_Filename,
    _In_z_ const wchar_t *const *_ArgList,
    _In_opt_z_ const wchar_t *const *_Env);

  _CRTIMP
  intptr_t
  __cdecl
  _wspawnl(
    _In_ int _Mode,
    _In_z_ const wchar_t *_Filename,
    _In_z_ const wchar_t *_ArgList,
    ...);

  _CRTIMP
  intptr_t
  __cdecl
  _wspawnle(
    _In_ int _Mode,
    _In_z_ const wchar_t *_Filename,
    _In_z_ const wchar_t *_ArgList,
    ...);

  _CRTIMP
  intptr_t
  __cdecl
  _wspawnlp(
    _In_ int _Mode,
    _In_z_ const wchar_t *_Filename,
    _In_z_ const wchar_t *_ArgList,
    ...);

  _CRTIMP
  intptr_t
  __cdecl
  _wspawnlpe(
    _In_ int _Mode,
    _In_z_ const wchar_t *_Filename,
    _In_z_ const wchar_t *_ArgList,
    ...);

  _CRTIMP
  intptr_t
  __cdecl
  _wspawnv(
    _In_ int _Mode,
    _In_z_ const wchar_t *_Filename,
    _In_z_ const wchar_t *const *_ArgList);

  _CRTIMP
  intptr_t
  __cdecl
  _wspawnve(
    _In_ int _Mode,
    _In_z_ const wchar_t *_Filename,
    _In_z_ const wchar_t *const *_ArgList,
    _In_opt_z_ const wchar_t *const *_Env);

  _CRTIMP
  intptr_t
  __cdecl
  _wspawnvp(
    _In_ int _Mode,
    _In_z_ const wchar_t *_Filename,
    _In_z_ const wchar_t *const *_ArgList);

  _CRTIMP
  intptr_t
  __cdecl
  _wspawnvpe(
    _In_ int _Mode,
    _In_z_ const wchar_t *_Filename,
    _In_z_ const wchar_t *const *_ArgList,
    _In_opt_z_ const wchar_t *const *_Env);

#ifndef _CRT_WSYSTEM_DEFINED
#define _CRT_WSYSTEM_DEFINED
  _CRTIMP int __cdecl _wsystem(_In_opt_z_ const wchar_t *_Command);
#endif /* _CRT_WSYSTEM_DEFINED */

#endif /* _WPROCESS_DEFINED */

  void __cdecl __security_init_cookie(void);
#if (defined(_X86_) && !defined(__x86_64))
  void __fastcall __security_check_cookie(uintptr_t _StackCookie);
  __declspec(noreturn) void __cdecl __report_gsfailure(void);
#else
  void __cdecl __security_check_cookie(uintptr_t _StackCookie);
  __declspec(noreturn) void __cdecl __report_gsfailure(uintptr_t _StackCookie);
#endif
  extern uintptr_t __security_cookie;

  intptr_t __cdecl _loaddll(_In_z_ char *_Filename);
  int __cdecl _unloaddll(_In_ intptr_t _Handle);
  int (__cdecl *__cdecl _getdllprocaddr(_In_ intptr_t _Handle, _In_opt_z_ char *_ProcedureName, _In_ intptr_t _Ordinal))(void);

#ifdef _DECL_DLLMAIN

#ifdef _WIN32

  WINBOOL
  WINAPI
  DllMain(
    _In_ HANDLE _HDllHandle,
    _In_ DWORD _Reason,
    _In_opt_ LPVOID _Reserved);

  WINBOOL
  WINAPI
  _CRT_INIT(
    _In_ HANDLE _HDllHandle,
    _In_ DWORD _Reason,
    _In_opt_ LPVOID _Reserved);

  WINBOOL
  WINAPI
  _wCRT_INIT(
    _In_ HANDLE _HDllHandle,
    _In_ DWORD _Reason,
    _In_opt_ LPVOID _Reserved);

  extern WINBOOL (WINAPI *const _pRawDllMain)(HANDLE,DWORD,LPVOID);

#else /* _WIN32 */

  int
  __stdcall
  DllMain(
    _In_ void *_HDllHandle,
    _In_ unsigned _Reason,
    _In_opt_ void *_Reserved);

  int
  __stdcall
  _CRT_INIT(
    _In_ void *_HDllHandle,
    _In_ unsigned _Reason,
    _In_opt_ void *_Reserved);

  int
  __stdcall
  _wCRT_INIT(
    _In_ void *_HDllHandle,
    _In_ unsigned _Reason,
    _In_opt_ void *_Reserved);

  extern int (__stdcall *const _pRawDllMain)(void *,unsigned,void *);

#endif /* _WIN32 */

#endif /* _DECL_DLLMAIN */

#ifndef NO_OLDNAMES

#define P_WAIT _P_WAIT
#define P_NOWAIT _P_NOWAIT
#define P_OVERLAY _P_OVERLAY
#define OLD_P_OVERLAY _OLD_P_OVERLAY
#define P_NOWAITO _P_NOWAITO
#define P_DETACH _P_DETACH
#define WAIT_CHILD _WAIT_CHILD
#define WAIT_GRANDCHILD _WAIT_GRANDCHILD

  _CRTIMP
  intptr_t
  __cdecl
  cwait(
    _Out_opt_ int *_TermStat,
    _In_ intptr_t _ProcHandle,
    _In_ int _Action);

#ifdef __GNUC__

  _CRTIMP
  int
  __cdecl
  execl(
    _In_z_ const char *_Filename,
    _In_z_ const char *_ArgList,
    ...);

  _CRTIMP
  int
  __cdecl
  execle(
    _In_z_ const char *_Filename,
    _In_z_ const char *_ArgList,
    ...);

  _CRTIMP
  int
  __cdecl
  execlp(
    _In_z_ const char *_Filename,
    _In_z_ const char *_ArgList,
    ...);

  _CRTIMP
  int
  __cdecl
  execlpe(
    _In_z_ const char *_Filename,
    _In_z_ const char *_ArgList,
    ...);

#else /* __GNUC__ */

  _CRTIMP
  intptr_t
  __cdecl
  execl(
    _In_z_ const char *_Filename,
    _In_z_ const char *_ArgList,
    ...);

  _CRTIMP
  intptr_t
  __cdecl
  execle(
    _In_z_ const char *_Filename,
    _In_z_ const char *_ArgList,
    ...);

  _CRTIMP
  intptr_t
  __cdecl
  execlp(
    _In_z_ const char *_Filename,
    _In_z_ const char *_ArgList,
    ...);

  _CRTIMP
  intptr_t
  __cdecl
  execlpe(
    _In_z_ const char *_Filename,
    _In_z_ const char *_ArgList,
    ...);

#endif /* __GNUC__ */

  _CRTIMP
  intptr_t
  __cdecl
  spawnl(
    _In_ int,
    _In_z_ const char *_Filename,
    _In_z_ const char *_ArgList,
    ...);

  _CRTIMP
  intptr_t
  __cdecl
  spawnle(
    _In_ int,
    _In_z_ const char *_Filename,
    _In_z_ const char *_ArgList,
    ...);

  _CRTIMP
  intptr_t
  __cdecl
  spawnlp(
    _In_ int,
    _In_z_ const char *_Filename,
    _In_z_ const char *_ArgList,
    ...);

  _CRTIMP
  intptr_t
  __cdecl
  spawnlpe(
    _In_ int,
    _In_z_ const char *_Filename,
    _In_z_ const char *_ArgList,
    ...);

  _CRTIMP int __cdecl getpid(void);

#ifdef __GNUC__

  /* Those methods are predefined by gcc builtins to return int. So to prevent
     stupid warnings, define them in POSIX way.  This is save, because those
     methods do not return in success case, so that the return value is not
     really dependent to its scalar width.  */

  _CRTIMP
  int
  __cdecl
  execv(
    _In_z_ const char *_Filename,
    _In_z_ char *const _ArgList[]);

  _CRTIMP
  int
  __cdecl
  execve(
    _In_z_ const char *_Filename,
    _In_z_ char *const _ArgList[],
    _In_opt_z_ char *const _Env[]);

  _CRTIMP
  int
  __cdecl
  execvp(
    _In_z_ const char *_Filename,
    _In_z_ char *const _ArgList[]);

  _CRTIMP
  int
  __cdecl
  execvpe(
    _In_z_ const char *_Filename,
    _In_z_ char *const _ArgList[],
    _In_opt_z_ char *const _Env[]);

#else /* __GNUC__ */

  _CRTIMP
  intptr_t
  __cdecl
  execv(
    _In_z_ const char *_Filename,
    _In_z_ char *const _ArgList[]);

  _CRTIMP
  intptr_t
  __cdecl
  execve(
    _In_z_ const char *_Filename,
    _In_z_ char *const _ArgList[],
    _In_opt_z_ char *const _Env[]);

  _CRTIMP
  intptr_t
  __cdecl
  execvp(
    _In_z_ const char *_Filename,
    _In_z_ char *const _ArgList[]);

  _CRTIMP
  intptr_t
  __cdecl
  execvpe(
    _In_z_ const char *_Filename,
    _In_z_ char *const _ArgList[],
    _In_opt_z_ char *const _Env[]);

#endif /* __GNUC__ */

  _CRTIMP
  intptr_t
  __cdecl
  spawnv(
    _In_ int,
    _In_z_ const char *_Filename,
    _In_z_ char *const _ArgList[]);

  _CRTIMP
  intptr_t
  __cdecl
  spawnve(
    _In_ int,
    _In_z_ const char *_Filename,
    _In_z_ char *const _ArgList[],
    _In_opt_z_ char *const _Env[]);

  _CRTIMP
  intptr_t
  __cdecl
  spawnvp(
    _In_ int,
    _In_z_ const char *_Filename,
    _In_z_ char *const _ArgList[]);

  _CRTIMP
  intptr_t
  __cdecl
  spawnvpe(
    _In_ int,
    _In_z_ const char *_Filename,
    _In_z_ char *const _ArgList[],
    _In_opt_z_ char *const _Env[]);

#endif /* NO_OLDNAMES */

#ifdef __cplusplus
}
#endif

#endif /* _POSIX_ */

#endif /* _INC_PROCESS */

/*
 * Process definitions
 *
 * Derived from the mingw header written by Colin Peters.
 * Modified for Wine use by Jon Griffiths and Francois Gouget.
 * This file is in the public domain.
 */
#ifndef __WINE_PROCESS_H
#define __WINE_PROCESS_H
#ifndef __WINE_USE_MSVCRT
#define __WINE_USE_MSVCRT
#endif

#ifndef MSVCRT
# ifdef USE_MSVCRT_PREFIX
#  define MSVCRT(x)    MSVCRT_##x
# else
#  define MSVCRT(x)    x
# endif
#endif

#ifndef MSVCRT_WCHAR_T_DEFINED
#define MSVCRT_WCHAR_T_DEFINED
#ifndef __cplusplus
typedef unsigned short MSVCRT(wchar_t);
#endif
#endif

/* Process creation flags */
#define _P_WAIT    0
#define _P_NOWAIT  1
#define _P_OVERLAY 2
#define _P_NOWAITO 3
#define _P_DETACH  4

#define _WAIT_CHILD      0
#define _WAIT_GRANDCHILD 1

#ifndef __stdcall
# ifdef __i386__
#  ifdef __GNUC__
#   define __stdcall __attribute__((__stdcall__))
#  elif defined(_MSC_VER)
    /* Nothing needs to be done. __stdcall already exists */
#  else
#   error You need to define __stdcall for your compiler
#  endif
# else  /* __i386__ */
#  define __stdcall
# endif  /* __i386__ */
#endif /* __stdcall */

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*_beginthread_start_routine_t)(void *);
typedef unsigned int (__stdcall *_beginthreadex_start_routine_t)(void *);

unsigned long _beginthread(_beginthread_start_routine_t,unsigned int,void*);
unsigned long _beginthreadex(void*,unsigned int,_beginthreadex_start_routine_t,void*,unsigned int,unsigned int*);
int         _cwait(int*,int,int);
void        _endthread(void);
void        _endthreadex(unsigned int);
int         _execl(const char*,const char*,...);
int         _execle(const char*,const char*,...);
int         _execlp(const char*,const char*,...);
int         _execlpe(const char*,const char*,...);
int         _execv(const char*,char* const *);
int         _execve(const char*,char* const *,const char* const *);
int         _execvp(const char*,char* const *);
int         _execvpe(const char*,char* const *,const char* const *);
int         _getpid(void);
int         _spawnl(int,const char*,const char*,...);
int         _spawnle(int,const char*,const char*,...);
int         _spawnlp(int,const char*,const char*,...);
int         _spawnlpe(int,const char*,const char*,...);
int         _spawnv(int,const char*,const char* const *);
int         _spawnve(int,const char*,const char* const *,const char* const *);
int         _spawnvp(int,const char*,const char* const *);
int         _spawnvpe(int,const char*,const char* const *,const char* const *);

void        MSVCRT(_c_exit)(void);
void        MSVCRT(_cexit)(void);
void        MSVCRT(_exit)(int);
void        MSVCRT(abort)(void);
void        MSVCRT(exit)(int);
int         MSVCRT(system)(const char*);

#ifndef MSVCRT_WPROCESS_DEFINED
#define MSVCRT_WPROCESS_DEFINED
int         _wexecl(const MSVCRT(wchar_t)*,const MSVCRT(wchar_t)*,...);
int         _wexecle(const MSVCRT(wchar_t)*,const MSVCRT(wchar_t)*,...);
int         _wexeclp(const MSVCRT(wchar_t)*,const MSVCRT(wchar_t)*,...);
int         _wexeclpe(const MSVCRT(wchar_t)*,const MSVCRT(wchar_t)*,...);
int         _wexecv(const MSVCRT(wchar_t)*,const MSVCRT(wchar_t)* const *);
int         _wexecve(const MSVCRT(wchar_t)*,const MSVCRT(wchar_t)* const *,const MSVCRT(wchar_t)* const *);
int         _wexecvp(const MSVCRT(wchar_t)*,const MSVCRT(wchar_t)* const *);
int         _wexecvpe(const MSVCRT(wchar_t)*,const MSVCRT(wchar_t)* const *,const MSVCRT(wchar_t)* const *);
int         _wspawnl(int,const MSVCRT(wchar_t)*,const MSVCRT(wchar_t)*,...);
int         _wspawnle(int,const MSVCRT(wchar_t)*,const MSVCRT(wchar_t)*,...);
int         _wspawnlp(int,const MSVCRT(wchar_t)*,const MSVCRT(wchar_t)*,...);
int         _wspawnlpe(int,const MSVCRT(wchar_t)*,const MSVCRT(wchar_t)*,...);
int         _wspawnv(int,const MSVCRT(wchar_t)*,const MSVCRT(wchar_t)* const *);
int         _wspawnve(int,const MSVCRT(wchar_t)*,const MSVCRT(wchar_t)* const *,const MSVCRT(wchar_t)* const *);
int         _wspawnvp(int,const MSVCRT(wchar_t)*,const MSVCRT(wchar_t)* const *);
int         _wspawnvpe(int,const MSVCRT(wchar_t)*,const MSVCRT(wchar_t)* const *,const MSVCRT(wchar_t)* const *);
int         _wsystem(const MSVCRT(wchar_t)*);
#endif /* MSVCRT_WPROCESS_DEFINED */

#ifdef __cplusplus
}
#endif


#ifndef USE_MSVCRT_PREFIX
#define P_WAIT          _P_WAIT
#define P_NOWAIT        _P_NOWAIT
#define P_OVERLAY       _P_OVERLAY
#define P_NOWAITO       _P_NOWAITO
#define P_DETACH        _P_DETACH

#define WAIT_CHILD      _WAIT_CHILD
#define WAIT_GRANDCHILD _WAIT_GRANDCHILD

static inline int cwait(int *status, int pid, int action) { return _cwait(status, pid, action); }
static inline int getpid(void) { return _getpid(); }
static inline int execv(const char* name, char* const* argv) { return _execv(name, argv); }
static inline int execve(const char* name, char* const* argv, const char* const* envv) { return _execve(name, argv, envv); }
static inline int execvp(const char* name, char* const* argv) { return _execvp(name, argv); }
static inline int execvpe(const char* name, char* const* argv, const char* const* envv) { return _execvpe(name, argv, envv); }
static inline int spawnv(int flags, const char* name, const char* const* argv) { return _spawnv(flags, name, argv); }
static inline int spawnve(int flags, const char* name, const char* const* argv, const char* const* envv) { return _spawnve(flags, name, argv, envv); }
static inline int spawnvp(int flags, const char* name, const char* const* argv) { return _spawnvp(flags, name, argv); }
static inline int spawnvpe(int flags, const char* name, const char* const* argv, const char* const* envv) { return _spawnvpe(flags, name, argv, envv); }

#ifdef __GNUC__
extern int execl(const char*,const char*,...) __attribute__((alias("_execl")));
extern int execle(const char*,const char*,...) __attribute__((alias("_execle")));
extern int execlp(const char*,const char*,...) __attribute__((alias("_execlp")));
extern int execlpe(const char*,const char*,...) __attribute__((alias("_execlpe")));
extern int spawnl(int,const char*,const char*,...) __attribute__((alias("_spawnl")));
extern int spawnle(int,const char*,const char*,...) __attribute__((alias("_spawnle")));
extern int spawnlp(int,const char*,const char*,...) __attribute__((alias("_spawnlp")));
extern int spawnlpe(int,const char*,const char*,...) __attribute__((alias("_spawnlpe")));
#else
#define execl    _execl
#define execle   _execle
#define execlp   _execlp
#define execlpe  _execlpe
#define spawnl   _spawnl
#define spawnle  _spawnle
#define spawnlp  _spawnlp
#define spawnlpe _spawnlpe
#endif  /* __GNUC__ */

#endif /* USE_MSVCRT_PREFIX */

#endif /* __WINE_PROCESS_H */

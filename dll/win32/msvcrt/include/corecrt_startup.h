/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the Wine project.
 */

#ifndef _INC_CORECRT_STARTUP
#define _INC_CORECRT_STARTUP

#include <corecrt.h>

typedef enum _crt_argv_mode
{
    _crt_argv_no_arguments,
    _crt_argv_unexpanded_arguments,
    _crt_argv_expanded_arguments
} _crt_argv_mode;

typedef enum _crt_app_type
{
    _crt_unknown_app,
    _crt_console_app,
    _crt_gui_app
} _crt_app_type;

typedef void (__cdecl *_PVFV)(void);
typedef int (__cdecl *_PIFV)(void);
typedef void (__cdecl *_PVFI)(int);

typedef struct _onexit_table_t {
    _PVFV *_first;
    _PVFV *_last;
    _PVFV *_end;
} _onexit_table_t;

#ifndef _CRT_ONEXIT_T_DEFINED
#define _CRT_ONEXIT_T_DEFINED
typedef int (__cdecl *_onexit_t)(void);
#endif

struct _exception;
typedef int (__cdecl *_UserMathErrorFunctionPointer)(struct _exception *);

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _UCRT
_ACRTIMP void __cdecl __getmainargs(int *, char ***, char ***, int, int *);
_ACRTIMP void __cdecl __wgetmainargs(int *, wchar_t ***, wchar_t ***, int, int *);
#define _set_app_type __set_app_type
#endif /* _UCRT */

_ACRTIMP void      __cdecl __setusermatherr(_UserMathErrorFunctionPointer);
_ACRTIMP errno_t   __cdecl _configure_narrow_argv(_crt_argv_mode);
_ACRTIMP errno_t   __cdecl _configure_wide_argv(_crt_argv_mode);
_ACRTIMP int       __cdecl _crt_at_quick_exit(_PVFV);
_ACRTIMP int       __cdecl _crt_atexit(_PVFV);
_ACRTIMP int       __cdecl _execute_onexit_table(_onexit_table_t*);
_ACRTIMP char    **__cdecl _get_initial_narrow_environment(void);
_ACRTIMP wchar_t **__cdecl _get_initial_wide_environment(void);
_ACRTIMP char*     __cdecl _get_narrow_winmain_command_line(void);
_ACRTIMP wchar_t*  __cdecl _get_wide_winmain_command_line(void);
_ACRTIMP int       __cdecl _initialize_narrow_environment(void);
_ACRTIMP int       __cdecl _initialize_onexit_table(_onexit_table_t*);
_ACRTIMP int       __cdecl _initialize_wide_environment(void);
_ACRTIMP int       __cdecl _register_onexit_function(_onexit_table_t*,_onexit_t);
_ACRTIMP void      __cdecl _set_app_type(_crt_app_type);

#ifdef __cplusplus
}
#endif

#endif /* _INC_CORECRT_STARTUP */

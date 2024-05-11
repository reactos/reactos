//
// argv_data.cpp
//
//      Copyright (c) Microsoft Corporation.  All rights reserved.
//
// This file defines the global data that stores the command line with which the
// program was executed, along with the parsed arguments (if the arguments were
// parsed), and the accessors for the global data.
//
#include <corecrt_internal.h>



extern "C" {


// Note:  In general, either the narrow or wide string variables will be set,
// but not both.  These get initialized by the CRT startup sequence before any
// user code is executed.  There are cases where any or all of the pointers may
// be null during execution.  Do not assume that they are non-null.

int       __argc   = 0;       // The number of arguments in __argv or __wargv
char**    __argv   = nullptr; // The arguments as narrow strings
wchar_t** __wargv  = nullptr; // The arguments as wide strings
char*     _pgmptr  = nullptr; // The name of the program as a narrow string
wchar_t*  _wpgmptr = nullptr; // The name of the program as a wide string
char*     _acmdln  = nullptr; // The raw command line as a narrow string
wchar_t*  _wcmdln  = nullptr; // The raw command line as a wide string

_BEGIN_SECURE_CRT_DEPRECATION_DISABLE

int*       __cdecl __p___argc()   { return &__argc;   }
char***    __cdecl __p___argv()   { return &__argv;   }
wchar_t*** __cdecl __p___wargv()  { return &__wargv;  }
char**     __cdecl __p__pgmptr()  { return &_pgmptr;  }
wchar_t**  __cdecl __p__wpgmptr() { return &_wpgmptr; }
char**     __cdecl __p__acmdln()  { return &_acmdln;  }
wchar_t**  __cdecl __p__wcmdln()  { return &_wcmdln;  }

errno_t __cdecl _get_wpgmptr(wchar_t** const result)
{
    _VALIDATE_RETURN_ERRCODE(result   != nullptr, EINVAL);
    _VALIDATE_RETURN_ERRCODE(_wpgmptr != nullptr, EINVAL);

    *result = _wpgmptr;
    return 0;
}

errno_t __cdecl _get_pgmptr(char** const result)
{
    _VALIDATE_RETURN_ERRCODE(result  != nullptr, EINVAL);
    _VALIDATE_RETURN_ERRCODE(_pgmptr != nullptr, EINVAL);
    *result = _pgmptr;
    return 0;
}

_END_SECURE_CRT_DEPRECATION_DISABLE



bool __cdecl __acrt_initialize_command_line()
{
    _acmdln = GetCommandLineA();
    _wcmdln = GetCommandLineW();
    return true;
}

bool __cdecl __acrt_uninitialize_command_line(bool const /* terminating */)
{
    return true;
}



} // extern "C"

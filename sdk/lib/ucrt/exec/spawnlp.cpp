//
// spawnlp.cpp
//
//      Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Defines the -lp and -lpe flavors of the _exec() and _spawn() functions.  See
// the comments in spawnv.cpp for details of the various flavors of these
// functions.
//
#include <corecrt_internal.h>
#include <corecrt_internal_traits.h>
#include <stddef.h>
#include <process.h>
#include <stdarg.h>



template <typename Character>
static intptr_t __cdecl common_spawnlp(
    bool             const pass_environment,
    int              const mode,
    Character const* const file_name,
    Character const* const arguments,
    va_list                varargs
    ) throw()
{
    typedef __crt_char_traits<Character> traits;

    _VALIDATE_RETURN(file_name    != nullptr, EINVAL, -1);
    _VALIDATE_RETURN(file_name[0] != '\0',    EINVAL, -1);
    _VALIDATE_RETURN(arguments    != nullptr, EINVAL, -1);
    _VALIDATE_RETURN(arguments[0] != '\0',    EINVAL, -1);

    Character* arguments_buffer[64];
    Character** const captured_arguments = traits::capture_argv(
        &varargs,
        arguments,
        &arguments_buffer[0],
        64);

    _VALIDATE_RETURN_NOEXC(captured_arguments != nullptr, ENOMEM, -1);

    __crt_unique_heap_ptr<Character*> const captured_arguments_cleanup(
        captured_arguments == arguments_buffer ? nullptr : captured_arguments);

    Character const* const* const environment = pass_environment
        ? va_arg(varargs, Character const* const*)
        : nullptr;

    return traits::tspawnvpe(mode, file_name, captured_arguments, environment);
}



extern "C" intptr_t _execlp(
    char const* const file_name,
    char const* const arguments,
    ...)
{
    va_list varargs;
    va_start(varargs, arguments);
    return common_spawnlp(false, _P_OVERLAY, file_name, arguments, varargs);
}

extern "C" intptr_t _execlpe(
    char const* const file_name,
    char const* const arguments,
    ...)
{
    va_list varargs;
    va_start(varargs, arguments);
    return common_spawnlp(true, _P_OVERLAY, file_name, arguments, varargs);
}

extern "C" intptr_t _spawnlp(
    int         const mode,
    char const* const file_name,
    char const* const arguments,
    ...)
{
    va_list varargs;
    va_start(varargs, arguments);
    return common_spawnlp(false, mode, file_name, arguments, varargs);
}

extern "C" intptr_t _spawnlpe(
    int         const mode,
    char const* const file_name,
    char const* const arguments,
    ...)
{
    va_list varargs;
    va_start(varargs, arguments);
    return common_spawnlp(true, mode, file_name, arguments, varargs);
}
   


extern "C" intptr_t _wexeclp(
    wchar_t const* const file_name,
    wchar_t const* const arguments,
    ...)
{
    va_list varargs;
    va_start(varargs, arguments);
    return common_spawnlp(false, _P_OVERLAY, file_name, arguments, varargs);
}

extern "C" intptr_t _wexeclpe(
    wchar_t const* const file_name,
    wchar_t const* const arguments,
    ...)
{
    va_list varargs;
    va_start(varargs, arguments);
    return common_spawnlp(true, _P_OVERLAY, file_name, arguments, varargs);
}

extern "C" intptr_t _wspawnlp(
    int            const mode,
    wchar_t const* const file_name,
    wchar_t const* const arguments,
    ...)
{
    va_list varargs;
    va_start(varargs, arguments);
    return common_spawnlp(false, mode, file_name, arguments, varargs);
}

extern "C" intptr_t _wspawnlpe(
    int            const mode,
    wchar_t const* const file_name,
    wchar_t const* const arguments,
    ...)
{
    va_list varargs;
    va_start(varargs, arguments);
    return common_spawnlp(true, mode, file_name, arguments, varargs);
}
    
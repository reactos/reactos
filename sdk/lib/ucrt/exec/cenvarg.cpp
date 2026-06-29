//
// cenvarg.cpp
//
//      Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Defines the _cenvarg() and _capture_argv functions, which transform argument
// vectors and environments for use by the _exec() and _spawn() functions.
//
#include <corecrt_internal.h>
#include <errno.h>
#include <corecrt_internal_traits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma warning(disable:__WARNING_POSTCONDITION_NULLTERMINATION_VIOLATION) // 26036


// Converts a main()-style argv arguments vector into a command line.  On success,
// returns a pointer to the newly constructed arguments block; the caller is
// responsible for freeing the string.  On failure, returns null and sets errno.
template <typename Character>
static errno_t __cdecl construct_command_line(
    Character const* const* const argv,
    Character**             const command_line_result
    ) throw()
{
    typedef __crt_char_traits<Character> traits;

    *command_line_result = nullptr;

    // Compute the number of bytes required to store the arguments in argv in a
    // command line string (including spaces between arguments and a terminator):
    size_t const command_line_count = [&]
    {
        size_t n = 0;
        for (Character const* const* it = argv; *it; n += traits::tcslen(*it++) + 1) { }

        // If there were no arguments, return 1 so that we can return an empty
        // string:
        return __max(n, 1);
    }();

    __crt_unique_heap_ptr<Character> command_line(_calloc_crt_t(Character, command_line_count));
    if (!command_line)
    {
        __acrt_errno_map_os_error(ERROR_NOT_ENOUGH_MEMORY);
        return errno = ENOMEM;
    }

    Character const* const* source_it = argv;
    Character*              result_it = command_line.get();

    // If there are no arguments, just return the empty string:
    if (*source_it == nullptr)
    {
        *command_line_result = command_line.detach();
        return 0;
    }

    // Copy the arguments, separated by spaces:
    while (*source_it != nullptr)
    {
        _ERRCHECK(traits::tcscpy_s(result_it, command_line_count - (result_it - command_line.get()), *source_it));
        result_it += traits::tcslen(*source_it);
        *result_it++ = ' ';
        ++source_it;
    }

    // Replace the last space with a terminator:
    result_it[-1] = '\0';

    *command_line_result = command_line.detach();
    return 0;
}



// Converts a main()-style envp environment vector into an environment block in
// the form required by the CreateProcess API.  On success, returns a pointer to
// the newly constructed environment block; the caller is responsible for freeing
// the block.  On failure, returns null and sets errno.
template <typename Character>
static errno_t __cdecl construct_environment_block(
    _In_opt_z_ Character const* const*      const   envp,
    _Outptr_result_maybenull_ Character**   const   environment_block_result
    ) throw()
{
    typedef __crt_char_traits<Character> traits;

    *environment_block_result = nullptr;

    // If envp is null, we will use the current environment of this process as
    // the environment for the new process.  No action is required in this case
    // because simply passing a null environment pointer to CreateProcess will
    // do the right thing.
    if (envp == nullptr)
        return 0;

    // Get the value of the SystemRoot environment variable, if it is defined,
    // and compute the number of characters required to store it in the
    // envrionment block:
    Character const system_root_name[] = { 'S', 'y', 's', 't', 'e', 'm', 'R', 'o', 'o', 't', '\0' };

    __crt_unique_heap_ptr<Character> system_root_value;
    if (_ERRCHECK_EINVAL(traits::tdupenv_s_crt(system_root_value.get_address_of(), nullptr, system_root_name)) != 0)
        return errno;

    size_t const system_root_value_count = system_root_value
        ? traits::tcslen(system_root_value.get()) + 1
        : 0;

    size_t const system_root_count = _countof(system_root_name) + system_root_value_count;

    // Compute the number of characters required to hold the environment
    // strings provided by the user:
    size_t const envp_count = [&]
    {
        size_t n = 2; // Account for double null terminator
        for (auto it = envp; *it != nullptr; n += traits::tcslen(*it++) + 1) { }
        return n;
    }();

    // Get the current environment from the OS so that we can get the current
    // directory strings (those starting with '=') and append them to the user-
    // provided environment.
    __crt_unique_heap_ptr<Character> const os_environment(traits::get_environment_from_os());
    if (!os_environment)
        return EINVAL;

    // Find the first shell environment variable:
    Character* const first_cwd = [&]
    {
        Character* it = os_environment.get();
        while (*it != '=')
            it += traits::tcslen(it) + 1;
        return it;
    }();

    // Find the end of the shell environment variables (assume they are contiguous):
    Character* const last_cwd = [&]
    {
        Character* it = first_cwd;
        while (it[0] == '=' && it[1] != '\0' && it[2] == ':' && it[3] == '=')
            it += 4 + traits::tcslen(it + 4) + 1;
        return it;
    }();

    size_t const cwd_count = last_cwd - first_cwd;


    // Check to see if the SystemRoot is already defined in the environment:
    bool const system_root_defined_in_environment = [&]
    {
        for (auto it = envp; *it != nullptr; ++it)
        {
            if (traits::tcsnicmp(*it, system_root_name, traits::tcslen(system_root_name)) == 0)
                return true;
        }

        return false;
    }();

    // Allocate storage for the new environment:
    size_t const environment_block_count = system_root_defined_in_environment
        ? envp_count + cwd_count
        : envp_count + cwd_count + system_root_count;
    
    __crt_unique_heap_ptr<Character> environment_block(_calloc_crt_t(Character, environment_block_count));
    if (!environment_block)
    {
        __acrt_errno_map_os_error(ERROR_OUTOFMEMORY);
        return errno = ENOMEM;
    }

    // Build the environment block by concatenating the environment strings with
    // null characters between them, and with a double null terminator.
    Character* result_it            = environment_block.get();
    size_t     remaining_characters = environment_block_count;

    // Copy the cwd strings into the new environment:
    if (cwd_count != 0)
    {
        memcpy(result_it, first_cwd, cwd_count * sizeof(Character));
        result_it            += cwd_count;
        remaining_characters -= cwd_count;
    }

    // Copy the environment strings from envp into the new environment:
    for (auto it = envp; *it != nullptr; ++it)
    {
        _ERRCHECK(traits::tcscpy_s(result_it, remaining_characters, *it));
        
        size_t const count_copied = traits::tcslen(*it) + 1;
        result_it            += count_copied;
        remaining_characters -= count_copied;
    }

    // Copy the SystemRoot into the new environment:
    if (!system_root_defined_in_environment)
    {
        static Character const equal_sign[] = { '=', '\0' };

        _ERRCHECK(traits::tcscpy_s(result_it, system_root_count, system_root_name));
        _ERRCHECK(traits::tcscat_s(result_it, system_root_count, equal_sign));
        if (system_root_value)
        {
            _ERRCHECK(traits::tcscat_s(result_it, system_root_count, system_root_value.get()));
        }
        result_it += system_root_count;
    }

    // Null-terminate the environment block and return it.  If the environment
    // block is empty, it requires two null terminators:
    if (result_it == environment_block.get())
        *result_it++ = '\0';

    *result_it = '\0';

    *environment_block_result = environment_block.detach();
    return 0;
}



// Converts a main()-style argv arguments vector and envp environment vector into
// a command line and an environment block, for use in the _exec and _spawn
// functions.  On success, returns 0 and sets the two result argumetns to point
// to the newly created command line and environment block.  The caller is
// responsible for freeing these blocks.  On failure, returns -1 and sets errno.
template <typename Character>
_Success_(return == 0)
_Ret_range_(-1, 0)
static int __cdecl common_pack_argv_and_envp(
    _In_z_                    Character const* const* const argv,
    _In_opt_z_                Character const* const* const envp,
    _Outptr_result_maybenull_ Character**             const command_line_result,
    _Outptr_result_maybenull_ Character**             const environment_block_result
    ) throw()
{
    typedef __crt_char_traits<Character> traits;

    __crt_unique_heap_ptr<Character> command_line;
    if (construct_command_line(argv, command_line.get_address_of()) != 0)
        return -1;
    
    __crt_unique_heap_ptr<Character> environment_block;
    if (construct_environment_block(envp, environment_block.get_address_of()) != 0)
        return -1;

    *command_line_result      = command_line.detach();
    *environment_block_result = environment_block.detach();
    return 0;
}

extern "C" int __cdecl __acrt_pack_narrow_command_line_and_environment(
    char const* const* const argv,
    char const* const* const envp,
    char**             const command_line_result,
    char**             const environment_block_result
    )
{
    return common_pack_argv_and_envp(argv, envp, command_line_result, environment_block_result);
}

extern "C" int __cdecl __acrt_pack_wide_command_line_and_environment(
    wchar_t const* const* const argv,
    wchar_t const* const* const envp,
    wchar_t**             const command_line_result,
    wchar_t**             const environment_block_result
    )
{
    return common_pack_argv_and_envp(argv, envp, command_line_result, environment_block_result);
}



// Creates an argv array for the _exec and _spawn functions.  This function walks
// the provided varargs list, copying the char* or wchar_t* pointers into an
// array.  The caller_array is used first; if it is too small to fit all of the
// arguments, an array is dynamically allocated.  A pointer to the argv array is
// returned to the caller.  If the returned pointer is not 'caller_array', the
// caller must free the array.  On failure, nullptr is returned and errno is set.
template <typename Character>
_Success_(return != 0)
static Character** __cdecl common_capture_argv(
    _In_ va_list*                                   const   arglist,
    _In_z_ Character const*                         const   first_argument,
    _When_(return == caller_array, _Post_z_)
    _Out_writes_(caller_array_count) Character**    const   caller_array,
    _In_ size_t                                     const   caller_array_count
    ) throw()
{
    Character** argv       = caller_array;
    size_t      argv_count = caller_array_count;

    __crt_unique_heap_ptr<Character*> local_array;

    size_t i = 0;
    Character* next_argument = const_cast<Character*>(first_argument);
    for (;;)
    {
        if (i >= argv_count)
        {
            _VALIDATE_RETURN_NOEXC(SIZE_MAX / 2 > argv_count, ENOMEM, nullptr);

            // If we have run out of room in the caller-provided array, allocate
            // an array on the heap and copy the contents of the caller-provided
            // array:
            if (argv == caller_array)
            {
                local_array = _calloc_crt_t(Character*, argv_count * 2);
                _VALIDATE_RETURN_NOEXC(local_array.get() != nullptr, ENOMEM, nullptr);

                _ERRCHECK(memcpy_s(local_array.get(), argv_count * 2, caller_array, caller_array_count));

                argv = local_array.get();
            }
            // Otherwise, we have run out of room in a dynamically allocated
            // array.  We need to reallocate:
            else
            {
                __crt_unique_heap_ptr<Character*> new_array(_recalloc_crt_t(Character*, local_array.get(), argv_count * 2));
                _VALIDATE_RETURN_NOEXC(new_array.get() != nullptr, ENOMEM, nullptr);

                local_array.detach();
                local_array.attach(new_array.detach());

                argv = local_array.get();
            }

            argv_count *= 2;
        }

        argv[i++] = next_argument;
        if (!next_argument)
            break;

#pragma warning(suppress:__WARNING_INCORRECT_ANNOTATION) // 26007 Possibly incorrect single element annotation on arglist
        next_argument = va_arg(*arglist, Character*);
    }

    // At this point, we have succeeded; either local_array is null, or argv is
    // local_array.  In either case, we detach so that we can transfer ownership
    // to the caller:
    local_array.detach();
    return argv;
}

extern "C" char** __acrt_capture_narrow_argv(
    va_list*    const arglist,
    char const* const first_argument,
    char**      const caller_array,
    size_t      const caller_array_count
    )
{
    return common_capture_argv(arglist, first_argument, caller_array, caller_array_count);
}

extern "C" wchar_t** __acrt_capture_wide_argv(
    va_list*       const arglist,
    wchar_t const* const first_argument,
    wchar_t**      const caller_array,
    size_t         const caller_array_count
    )
{
    return common_capture_argv(arglist, first_argument, caller_array, caller_array_count);
}

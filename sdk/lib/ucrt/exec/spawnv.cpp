//
// spawnv.cpp
//
//      Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Defines the -v and -ve flavors of the _exec() and _spawn() functions.
//
// There are many varieties of the _exec() and _spawn() functions.  A high level
// summary of the behavioral differences is as follows:
//
// All of these functions execute a new process.  The _spawn() functions all take
// a 'mode' parameter, which specifies the way in which the process is started.
// The 'mode' must be one of the _P_-prefixed modes from <process.h>.  The _exec()
// family of functions execute a new process, then call _exit() to terminate the
// calling process.  Each _exit() function is equivalent to the corresponding
// _spawn() function with the _P_OVERLAY mode.
//
// There are eight variants each of _exec() and _spawn(), suffixed with -l, -le,
// -lp, -lpe, -v, -ve, -vp, and -vpe.
//
// If a function has 'e' in its suffix, it accepts an environment with which to
// create the new process.  If a function does not have 'e' in its suffix, it
// does not accept an environment and instead uses the environment of the calling
// process.  A call to an 'e' function with a null environment argument has the
// same effect as calling the non-'e'-suffixed equivalent.
//
// Each _exec() or _spawn() function has either an 'l' or 'v' suffix.  These have
// equivalent functionality; they differ only in how they accept their arguments.
// The 'l'-suffixed functions accept the command-line arguments and the environment
// as varargs.  There must be at least one command line argument (conventionally
// the name of the program to be executed).  If the function accepts an environment
// (if it is 'e'-suffixed), then there must be a null pointer between the last
// argument and the first environment variable.  The arguments are terminated by a
// null pointer.
//
// The 'v'-suffixed functions accept a pair of pointers:  one to the argv vector
// and one to the envp vector.  Each is an array of pointers terminated by a null
// pointer, similar to how arguments and the environment are passed to main().
//
// Finally, if a function has a 'p' in its suffix, and if the provided executable
// file name is just a file name (and does not contain any path component), the
// %PATH% is searched for an executable with the given name.  If a function does
// not have a 'p' suffix, then the environment is not searched; the executable
// must be found simply by passing its name to CreateProcess.
//
// All functions return -1 and set errno on failure.  On success, the _exec()
// functions do not return.  On success, the _spawn() functions return different
// things, depending on the provided mode.  See the CreateProcess invocation
// logic in this file for details.
//
// Note that the only supported modes are wait and overlay.
//
// These functions may set errno to one of the following values:
// * E2BIG:    Failed in argument or environment processing because the argument
//             list or environment is too large.
// * EACCESS:  Locking or sharing violation on a file
// * EMFILE:   Too many files open
// * ENOENT:   Failed to find the program (no such file or directory)
// * ENOEXEC:  Failed in a call to exec() due to a bad executable format
// * ENOMEM:   Failed to allocate memory required for the spawn operation
// * EINVAL:   Invalid mode argumnt or process state for spawn (note that most
//             invalid arguments cause the invalid parameter handler to be
//             invoked).
//
#include <corecrt_internal.h>
#include <corecrt_internal_lowio.h>
#include <corecrt_internal_traits.h>
#include <errno.h>
#include <io.h>
#include <mbstring.h>
#include <process.h>
#include <stdlib.h>
#include <string.h>



// Accumulates the inheritable file handles into *data, in the structure expected
// by the spawnee (see the lowio initialization code for the logic that decodes
// this data structure).  On success, *data and *size have the handle data array
// and the size of the handle data, and true is returned.  The caller must free
// *data.  On failure, false is returned and errno is set.
static bool accumulate_inheritable_handles(
    BYTE**  const data,
    size_t* const size,
    bool    const include_std_handles
    ) throw()
{
    return __acrt_lock_and_call(__acrt_lowio_index_lock, [&]() -> bool
    {
        *data = nullptr;
        *size = 0;

        // Count the number of handles to be inherited:
        size_t handle_count = 0;
        for (handle_count = _nhandle; handle_count != 0 && _osfile(handle_count - 1) != 0; --handle_count)
        {
        }

        size_t const max_handle_count = (USHRT_MAX - sizeof(int)) / (sizeof(char) + sizeof(intptr_t));
        _VALIDATE_RETURN_NOEXC(handle_count < max_handle_count, ENOMEM, false);

        size_t const handle_data_header_size  = sizeof(int);
        size_t const handle_data_element_size = sizeof(char) + sizeof(intptr_t);

        unsigned short const handle_data_size = static_cast<unsigned short>(
            handle_data_header_size +
            handle_count * handle_data_element_size);

        __crt_unique_heap_ptr<BYTE> handle_data(_calloc_crt_t(BYTE, handle_data_size));
        _VALIDATE_RETURN_NOEXC(handle_data.get() != nullptr, ENOMEM, false);

        // Set the handle count in the data:
        *reinterpret_cast<int*>(handle_data.get()) = static_cast<int>(handle_count);

        auto const first_flags  = reinterpret_cast<char*>(handle_data.get() + sizeof(int));
        auto const first_handle = reinterpret_cast<intptr_t UNALIGNED*>(first_flags + (handle_count * sizeof(char)));

        // Copy the handle data:
        auto flags_it  = first_flags;
        auto handle_it = first_handle;
        for (size_t i = 0; i != handle_count; ++i, ++flags_it, ++handle_it)
        {
            __crt_lowio_handle_data* const pio = _pioinfo(i);
            if ((pio->osfile & FNOINHERIT) == 0)
            {
                *flags_it  = pio->osfile;
                *handle_it = pio->osfhnd;
            }
            else
            {
                *flags_it  = 0;
                *handle_it = reinterpret_cast<intptr_t>(INVALID_HANDLE_VALUE);
            }
        }

        // Exclude the first three handles (stdin, stdout, stderr) if asked:
        if (!include_std_handles)
        {
            flags_it = first_flags;
            handle_it = first_handle;
            for (size_t i = 0; i != __min(handle_count, 3); ++i, ++flags_it, ++handle_it)
            {
                *flags_it = 0;
                *handle_it = reinterpret_cast<intptr_t>(INVALID_HANDLE_VALUE);
            }
        }

        *data = handle_data.detach();
        *size = handle_data_size;
        return true;
    });
}



static bool __cdecl should_create_unicode_environment(char)    throw() { return false; }
static bool __cdecl should_create_unicode_environment(wchar_t) throw() { return true;  }



// Spawns a child process.  The mode must be one of the _P-modes from <process.h>.
// The return value depends on the mode:
// * _P_OVERLAY:  On success, calls _exit() and does not return. Returns -1 on failure.
// * _P_WAIT:     Returns (termination_code << 8 + result_code)
// * _P_DETACH:   Returns 0 on success; -1 on failure
// * Others:      Returns a handle to the process. The caller must close the handle.
template <typename Character>
static intptr_t __cdecl execute_command(
    int                     const mode,
    Character const*        const file_name,
    Character const* const* const arguments,
    Character const* const* const environment
    ) throw()
{
    typedef __crt_char_traits<Character> traits;

    _VALIDATE_RETURN(file_name != nullptr, EINVAL, -1);
    _VALIDATE_RETURN(arguments != nullptr, EINVAL, -1);

    _VALIDATE_CLEAR_OSSERR_RETURN(mode >= 0 && mode <= _P_DETACH, EINVAL, -1);

    __crt_unique_heap_ptr<Character> command_line;
    __crt_unique_heap_ptr<Character> environment_block;
    if (traits::pack_command_line_and_environment(
            arguments,
            environment,
            command_line.get_address_of(),
            environment_block.get_address_of()
        ) == -1)
    {
        return -1;
    }

    __crt_unique_heap_ptr<BYTE> handle_data;
    size_t                      handle_data_size;
    if (!accumulate_inheritable_handles(handle_data.get_address_of(), &handle_data_size, mode != _P_DETACH))
        return -1;

    DWORD creation_flags = 0;
    if (mode == _P_DETACH)
        creation_flags |= DETACHED_PROCESS;

    if (should_create_unicode_environment(Character()))
        creation_flags |= CREATE_UNICODE_ENVIRONMENT;

    _doserrno = 0;

    STARTUPINFOW startup_info = { };
    startup_info.cb          = sizeof(startup_info);
    startup_info.cbReserved2 = static_cast<WORD>(handle_data_size);
    startup_info.lpReserved2 = handle_data.get();

    PROCESS_INFORMATION process_info;
    BOOL const create_process_status = traits::create_process(
        const_cast<Character*>(file_name),
        command_line.get(),
        nullptr,
        nullptr,
        TRUE,
        creation_flags,
        environment_block.get(),
        nullptr,
        &startup_info,
        &process_info);

    __crt_unique_handle process_handle(process_info.hProcess);
    __crt_unique_handle thread_handle(process_info.hThread);

    if (!create_process_status)
    {
        __acrt_errno_map_os_error(GetLastError());
        return -1;
    }

    if (mode == _P_OVERLAY)
    {
        // Destroy ourselves:
        _exit(0);
    }
    else if (mode == _P_WAIT)
    {
        WaitForSingleObject(process_info.hProcess, static_cast<DWORD>(-1));

        // Return the termination code and exit code.  Note that we return
        // the full exit code.
        DWORD exit_code;
        if (0 != GetExitCodeProcess(process_info.hProcess, &exit_code))
        {
            return static_cast<int>(exit_code);
        }
        else
        {
            __acrt_errno_map_os_error(GetLastError());
            return -1;
        }
    }
    else if (mode == _P_DETACH)
    {
        /* like totally detached asynchronous spawn, dude,
            close process handle, return 0 for success */
        return 0;
    }
    else
    {
        // Asynchronous spawn:  return process handle:
        return reinterpret_cast<intptr_t>(process_handle.detach());
    }
}



template <typename Character>
static intptr_t __cdecl common_spawnv(
    int                     const mode,
    Character const*        const file_name,
    Character const* const* const arguments,
    Character const* const* const environment
    ) throw()
{
    typedef __crt_char_traits<Character> traits;

    _VALIDATE_RETURN(file_name       != nullptr, EINVAL, -1);
    _VALIDATE_RETURN(file_name[0]    != '\0',    EINVAL, -1);
    _VALIDATE_RETURN(arguments       != nullptr, EINVAL, -1);
    _VALIDATE_RETURN(arguments[0]    != nullptr, EINVAL, -1);
    _VALIDATE_RETURN(arguments[0][0] != '\0',    EINVAL, -1);

    Character const* const final_backslash = traits::tcsrchr(file_name, '\\');
    Character const* const final_slash     = traits::tcsrchr(file_name, '/');

    Character const* mutated_file_name = file_name;
    Character const* end_of_directory  = final_backslash;
    if (!final_slash)
    {
        if (!final_backslash)
        {
            Character const* const final_colon = traits::tcsrchr(file_name, ':');
            if (final_colon)
            {
                end_of_directory = final_colon;
            }
            else
            {
                // The path is a file name only.  We force it to be a relative
                // path name.
                size_t const file_name_size = traits::tcslen(file_name) + 3;
                __crt_unique_heap_ptr<Character> buffer(_calloc_crt_t(Character, file_name_size));
                if (!buffer)
                    return -1;

                static Character const dot_slash[] = { '.', '\\', '\0' };
                _ERRCHECK(traits::tcscpy_s(buffer.get(), file_name_size, dot_slash));
                _ERRCHECK(traits::tcscat_s(buffer.get(), file_name_size, file_name));

                mutated_file_name = buffer.detach();
                end_of_directory  = mutated_file_name + 2; // Adjust for ".\"
            }
        }
    }
    else if (!final_backslash || final_slash > final_backslash)
    {
        end_of_directory = final_slash;
    }

    // If we allocated a file name above, make sure we clean it up:
    __crt_unique_heap_ptr<Character const> const mutated_file_name_cleanup(file_name == mutated_file_name
        ? nullptr
        : mutated_file_name);

    if (traits::tcsrchr(end_of_directory, '.'))
    {
        // If an extension was provided, just invoke the path:
        if (traits::taccess_s(mutated_file_name, 0) == 0)
        {
            return execute_command(mode, mutated_file_name, arguments, environment);
        }
    }
    else
    {
        // If no extension was provided, try known executable extensions:
        size_t const buffer_size = traits::tcslen(mutated_file_name) + 5;
        __crt_unique_heap_ptr<Character> const buffer(_calloc_crt_t(Character, buffer_size));
        if (!buffer)
            return -1;

        _ERRCHECK(traits::tcscpy_s(buffer.get(), buffer_size, mutated_file_name));
        Character* extension_buffer = buffer.get() + buffer_size - 5;

        typedef Character const extension_type[5];
        static extension_type const extensions[4] =
        {
            { '.', 'c', 'o', 'm', '\0' },
            { '.', 'e', 'x', 'e', '\0' },
            { '.', 'b', 'a', 't', '\0' },
            { '.', 'c', 'm', 'd', '\0' }
        };

        errno_t const saved_errno = errno;

        extension_type const* const first_extension = extensions;
        extension_type const* const last_extension  = first_extension + _countof(extensions);
        for (auto it = first_extension; it != last_extension; ++it)
        {
            _ERRCHECK(traits::tcscpy_s(extension_buffer, 5, *it));

            if (traits::taccess_s(buffer.get(), 0) == 0)
            {
                errno = saved_errno;
                return execute_command(mode, buffer.get(), arguments, environment);
            }
        }
    }

    return -1;
}



extern "C" intptr_t __cdecl _execv(
    char const*        const file_name,
    char const* const* const arguments
    )
{
    return common_spawnv(_P_OVERLAY, file_name, arguments, static_cast<char const* const*>(nullptr));
}

extern "C" intptr_t __cdecl _execve(
    char const*        const file_name,
    char const* const* const arguments,
    char const* const* const environment
    )
{
    return common_spawnv(_P_OVERLAY, file_name, arguments, environment);
}

extern "C" intptr_t __cdecl _spawnv(
    int                const mode,
    char const*        const file_name,
    char const* const* const arguments
    )
{
    return common_spawnv(mode, file_name, arguments, static_cast<char const* const*>(nullptr));
}

extern "C" intptr_t __cdecl _spawnve(
    int                const mode,
    char const*        const file_name,
    char const* const* const arguments,
    char const* const* const environment
    )
{
    return common_spawnv(mode, file_name, arguments, environment);
}



extern "C" intptr_t __cdecl _wexecv(
    wchar_t const*        const file_name,
    wchar_t const* const* const arguments
    )
{
    return common_spawnv(_P_OVERLAY, file_name, arguments, static_cast<wchar_t const* const*>(nullptr));
}

extern "C" intptr_t __cdecl _wexecve(
    wchar_t const*        const file_name,
    wchar_t const* const* const arguments,
    wchar_t const* const* const environment
    )
{
    return common_spawnv(_P_OVERLAY, file_name, arguments, environment);
}

extern "C" intptr_t __cdecl _wspawnv(
    int                   const mode,
    wchar_t const*        const file_name,
    wchar_t const* const* const arguments
    )
{
    return common_spawnv(mode, file_name, arguments, static_cast<wchar_t const* const*>(nullptr));
}

extern "C" intptr_t __cdecl _wspawnve(
    int                   const mode,
    wchar_t const*        const file_name,
    wchar_t const* const* const arguments,
    wchar_t const* const* const environment
    )
{
    return common_spawnv(mode, file_name, arguments, environment);
}

//
// system.cpp
//
//      Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Defines the system() family of functions, which execute a command via the
// shell.
//
#include <corecrt_internal.h>
#include <corecrt_internal_traits.h>
#include <process.h>
#include <io.h>
#include <stdlib.h>
#include <errno.h>



// Executes a command via the shell.  If the command is null, attempts to execute
// the command processor specified by the %COMSPEC% environment variable.  If 
// that fails, attempts to execute cmd.exe.
template <typename Character>
static int __cdecl common_system(Character const* const command) throw()
{
    typedef __crt_char_traits<Character> traits;

    static Character const comspec_name[] = { 'C', 'O', 'M', 'S', 'P', 'E', 'C', '\0' };
    static Character const cmd_exe[]      = { 'c', 'm', 'd', '.', 'e', 'x', 'e', '\0' };
    static Character const slash_c[]      = { '/', 'c', '\0' };

    __crt_unique_heap_ptr<Character> comspec_value;
    _ERRCHECK_EINVAL(traits::tdupenv_s_crt(comspec_value.get_address_of(), nullptr, comspec_name));

    // If the command is null, return TRUE only if %COMSPEC% is set and the file
    // to which it points exists.
    if (!command)
    {
        if (!comspec_value)
            return 0;

        return traits::taccess_s(comspec_value.get(), 0) == 0;
    }

    _ASSERTE(command[0] != '\0');

    Character const* arguments[4] =
    {
        comspec_value.get(),
        slash_c,
        command,
        nullptr
    };

    if (comspec_value)
    {
        errno_t const saved_errno = errno;
        errno = 0;

        int const result = static_cast<int>(traits::tspawnve(_P_WAIT, arguments[0], arguments, nullptr));
        if (result != -1)
        {
            errno = saved_errno;
            return result;
        }

        if (errno != ENOENT && errno != EACCES)
        {
            return result;
        }

        // If the error wasn't one of those two errors, try again with cmd.exe...
        errno = saved_errno;
    }

   arguments[0] = cmd_exe;
   return static_cast<int>(traits::tspawnvpe(_P_WAIT, arguments[0], arguments, nullptr));
}

extern "C" int __cdecl system(char const* const command)
{
    return common_system(command);
}

extern "C" int __cdecl _wsystem(wchar_t const* const command)
{
    return common_system(command);
}

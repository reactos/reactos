/*
 * PROJECT:     ReactOS msvcrt
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of system / _wsystem
 * COPYRIGHT:   Copyright (c) Microsoft Corporation.  All rights reserved.
 * COPYRIGHT:   Copyright 2025 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <process.h>
#include <io.h>
#include <stdlib.h>
#include <errno.h>

extern "C" int _cdecl _access_s(const char* filename, int mode);
extern "C" int _cdecl _waccess_s(const wchar_t* filename, int mode);

int _cdecl _taccess_s(const char* filename, int mode)
{
    return _access_s(filename, mode);
}

int _cdecl _taccess_s(const wchar_t* filename, int mode)
{
    return _waccess_s(filename, mode);
}

char* __cdecl _tgetenv(_In_z_ char const* _VarName)
{
    return getenv(_VarName);
}

wchar_t* __cdecl _tgetenv(_In_z_ wchar_t const* _VarName)
{
    return _wgetenv(_VarName);
}

intptr_t __cdecl _tspawnve(int flags, const char* name, const char* const* argv,
                        const char* const* envv)
{
    return _spawnve(flags, name, argv, envv);
}

intptr_t __cdecl _tspawnve(int flags, const wchar_t* name, const wchar_t* const* argv,
                        const wchar_t* const* envv)
{
    return _wspawnve(flags, name, argv, envv);
}

intptr_t __cdecl _tspawnvpe(int flags, const char* name, const char* const* argv,
                         const char* const* envv)
{
    return _spawnvpe(flags, name, argv, envv);
}

intptr_t __cdecl _tspawnvpe(int flags, const wchar_t* name, const wchar_t* const* argv,
                         const wchar_t* const* envv)
{
    return _wspawnvpe(flags, name, argv, envv);
}

template <typename Character>
static int __cdecl common_system(Character const* const command) throw()
{
    static Character const comspec_name[] = { 'C', 'O', 'M', 'S', 'P', 'E', 'C', '\0' }; // "COMSPEC"
    static Character const cmd_exe[]      = { 'c', 'm', 'd', '.', 'e', 'x', 'e', '\0' }; // "cmd.exe"
    static Character const slash_c[]      = { '/', 'c', '\0' }; // "/c"

    Character const * comspec_value = _tgetenv(comspec_name);

    // If the command is null, return TRUE only if %COMSPEC% is set and the file
    // to which it points exists.
    if (!command)
    {
        if (!comspec_value)
            return 0;

        return _taccess_s(comspec_value, 0) == 0;
    }

    Character const* arguments[4] =
    {
        comspec_value,
        slash_c,
        command,
        nullptr
    };

    if (comspec_value)
    {
        errno_t const saved_errno = errno;
        errno = 0;

        int const result = static_cast<int>(_tspawnve(_P_WAIT, arguments[0], arguments, nullptr));
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
   return static_cast<int>(_tspawnvpe(_P_WAIT, arguments[0], arguments, nullptr));

    return 0;
}

extern "C" int __cdecl system(char const* const command)
{
    return common_system(command);
}

extern "C" int __cdecl _wsystem(wchar_t const* const command)
{
    return common_system(command);
}

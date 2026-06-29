//
// setenv.cpp
//
//      Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Internal functions for setting or removing variables from an environment. The
// logic for manipulating the environment data structures is split across this
// file and environment_initialization.cpp.
//
#include <corecrt_internal.h>
#include <corecrt_internal_traits.h>
#include <limits.h>
#include <stdlib.h>



static char**&    __cdecl get_environment(char)    throw() { return _environ_table.value(); }
static wchar_t**& __cdecl get_environment(wchar_t) throw() { return _wenviron_table.value(); }

static wchar_t**& __cdecl get_other_environment(char)    throw() { return _wenviron_table.value(); }
static char**&    __cdecl get_other_environment(wchar_t) throw() { return _environ_table.value(); }

static char**&    __cdecl get_initial_environment(char)    throw() { return __dcrt_initial_narrow_environment; }
static wchar_t**& __cdecl get_initial_environment(wchar_t) throw() { return __dcrt_initial_wide_environment;   }



// Makes a copy of the provided environment and returns the copy.  The caller is
// responsible for freeing the returned array (using the CRT free).  Returns
// nullptr on failure; terminates the process on allocation failure.
template <typename Character>
static Character** __cdecl copy_environment(Character** const old_environment) throw()
{
    typedef __crt_char_traits<Character> traits;

    if (!old_environment)
    {
        return nullptr;
    }

    // Count the number of environment variables:
    size_t entry_count = 0;
    for (Character** it = old_environment; *it; ++it)
    {
        ++entry_count;
    }

    // We need one pointer for each string, plus one null pointer at the end:
    __crt_unique_heap_ptr<Character*> new_environment(_calloc_crt_t(Character*, entry_count + 1));
    if (!new_environment)
    {
        abort();
    }

    Character** old_it = old_environment;
    Character** new_it = new_environment.get();
    for (; *old_it; ++old_it, ++new_it)
    {
        size_t const required_count = traits::tcslen(*old_it) + 1;
        *new_it = _calloc_crt_t(Character, required_count).detach();
        if (!*new_it)
        {
            abort();
        }

        _ERRCHECK(traits::tcscpy_s(*new_it, required_count, *old_it));
    }

    return new_environment.detach();
}



// If the current environment is the initial environment, this function clones
// the current environment so that it is not the initial environment.  This
// should be called any time that we are about to modify the current environment
// but we do not know whether the current environment is the initial environment.
template <typename Character>
static void __cdecl ensure_current_environment_is_not_initial_environment_nolock() throw()
{
    if (get_environment(Character()) == get_initial_environment(Character()))
    {
        get_environment(Character()) = copy_environment(get_environment(Character()));
    }
}



// Finds an environment variable in the specified environment.  If a variable
// with the given name is found, its index in the environment is returned.  If
// no such environment is found, the total number of environment variables is
// returned, multiplied by -1.  Note that a return value of 0 may indicate
// either that the variable was found at index 0 or there are zero variables
// in the environment.  Be sure to check for this case.
template <typename Character>
static ptrdiff_t __cdecl find_in_environment_nolock(
    Character const* const name,
    size_t           const length
    ) throw()
{
    typedef __crt_char_traits<Character> traits;

    Character** const environment = get_environment(Character());

    Character** it = nullptr;
    for (it = environment; *it; ++it)
    {
        // See if the first 'length' characters match:
        if (traits::tcsnicoll(name, *it, length) != 0)
        {
            continue;
        }

        // Ensure that the next character of the environment is an '=' or '\0':
        if ((*it)[length] != '=' && (*it)[length] != '\0')
        {
            continue;
        }

        // Otherwise, this entry matched; return its index in the environment:
        return static_cast<ptrdiff_t>(it - environment);
    }

    // No entry matched; return the total number of strings, multiplied by -1:
    return -static_cast<ptrdiff_t>(it - environment);
}


/***
*int __dcrt_set_variable_in_narrow_environment(option) - add/replace/remove variable in environment
*
*Purpose:
*       option should be of the form "option=value".  If a string with the
*       given option part already exists, it is replaced with the given
*       string; otherwise the given string is added to the environment.
*       If the string is of the form "option=", then the string is
*       removed from the environment, if it exists.  If the string has
*       no equals sign, error is returned.
*
*Entry:
*       TCHAR **poption - pointer to option string to set in the environment list.
*           should be of the form "option=value".
*           This function takes ownership of this pointer in the success case.
*       int primary - Only the primary call to _crt[w]setenv needs to
*           create new copies or set the OS environment.
*           1 indicates that this is the primary call.
*
*Exit:
*       returns 0 if OK, -1 if fails.
*       If *poption is non-null on exit, we did not free it, and the caller should
*       If *poption is null on exit, we did free it, and the caller should not.
*
*Exceptions:
*
*Warnings:
*       This code will not work if variables are removed from the environment
*       by deleting them from environ[].  Use _putenv("option=") to remove a
*       variable.
*
*       The option argument will be taken ownership of by this code and may be freed!
*
*******************************************************************************/
template <typename Character>
static int __cdecl common_set_variable_in_environment_nolock(
    Character* const option,
    int        const is_top_level_call
    ) throw()
{
    typedef __crt_char_traits<Character> traits;

    // Check that the option string is valid first.  Find the '=' and verify
    // that '=' is not the first character in the string:
    _VALIDATE_RETURN_NOEXC(option != nullptr, EINVAL, -1);
    __crt_unique_heap_ptr<Character> owned_option(option);

    Character* const equal_sign = traits::tcschr(option, '=');
    _VALIDATE_RETURN_NOEXC(equal_sign != nullptr && equal_sign != option, EINVAL, -1);

    // Internal consistency check:  The environment string should never use
    // buffers larger than _MAX_ENV.  See also the SetEnvironmentVariable SDK
    // function.
    _ASSERTE(equal_sign - option                       < _MAX_ENV);
    _ASSERTE(traits::tcsnlen(equal_sign + 1, _MAX_ENV) < _MAX_ENV);

    // If the character following '=' is the terminator, we are removing the
    // environment variable.  Otherwise, we are adding or updating the variable:
    bool const is_removal = *(equal_sign + 1) == '\0';

    // At program startup, the initial environment (__dcrt_initial_narrow_environment), which is passed
    // to main(), is backed by the same environment arrays as the global
    // environment used by getenv, setenv, et al.  We cannot modify thie initial
    // environment, so we make a copy of it the first time we need to make any
    // modifications to the global environment:
    ensure_current_environment_is_not_initial_environment_nolock<Character>();

    // If the required environment does not exist, see if the other environment
    // exists; if it does, convert it to create the required environment.  These
    // functions will reenter this function once for each environment variable;
    // we use the top-level call flag to stop recursion.
    if (!get_environment(Character()))
    {
        if (is_top_level_call && get_other_environment(Character()))
        {
            _VALIDATE_RETURN_NOEXC(traits::get_or_create_environment_nolock() != nullptr, EINVAL, -1);

            // The call to get_or_create_environment() may have initialized the
            // current environment to the same environment that is the initial
            // environment.  Re-check and make a new copy of the environment to
            // modify if necessary.
            ensure_current_environment_is_not_initial_environment_nolock<Character>();
        }
        else
        {
            // If the environment doesn't exist and the requested operation is a
            // removal, there is nothing to do (there is nothing to remove):
            if (is_removal)
            {
                return 0;
            }

            // Create a new environment for each environment that does not exist.
            // Just start each off as an empty environment:
            if (!_environ_table.value())
            {
                _environ_table.value() = _calloc_crt_t(char*, 1).detach();
            }

            if (!_environ_table.value())
            {
                return -1;
            }

            if (!_wenviron_table.value())
            {
                _wenviron_table.value() = _calloc_crt_t(wchar_t*, 1).detach();
            }

            if (!_wenviron_table.value())
            {
                return -1;
            }
        }
    }

    // At this point, either [1] only one environment exists, or [2] both of the
    // environments exist and are in-sync.  The only way they can get out of sync
    // is if there are conversion problems.  For example, if the user sets two
    // Unicode environment variables, FOO1 and FOO2, and the conversion of these
    // to multibyte yields FOO? and FOO?, then these environment blocks will
    // differ.
    Character** const environment = get_environment(Character());
    if (!environment)
    {
        _ASSERTE(("CRT logic error in setenv", 0));
        return -1;
    }

    // Try to find the option in the environment...
    ptrdiff_t const option_index = find_in_environment_nolock(option, equal_sign - option);

    // ... if the string is already in the environment, we free up the original
    // string, then install the new string or shrink the environment:
    if (option_index >= 0 && environment[0])
    {
        _free_crt(environment[option_index]);

        // If this is a removal, shrink the environment:
        if (is_removal)
        {
            // Shift all of the entries down by one element:
            size_t i = static_cast<size_t>(option_index);
            for (; environment[i]; ++i)
            {
                environment[i] = environment[i + 1];
            }

            // Shrink the environment memory block.  At this point, i is the
            // number of elements remaining in the environment.  This realloc
            // should never fail, since we are shrinking the block, but it is
            // best to be careful.  If it does fail, it doesn't matter.
            Character** new_environment = _recalloc_crt_t(Character*, environment, i).detach();
            if (new_environment)
            {
                get_environment(Character()) = new_environment;
            }
        }
        // If this is a replacement, replace the variable:
        else
        {
            environment[option_index] = owned_option.detach();
        }
    }
    // Otherwise, the string is not in the environment:
    else
    {
        // If this is a removal, it is a no-op:  the variable does not exist.
        if (is_removal)
        {
            return 0;
        }
        // Otherwise, we need to append the string to the environment table, and
        // we must grow the table to do this:
        else
        {
            size_t const environment_count = static_cast<size_t>(-option_index);
            if (environment_count + 2 < environment_count)
            {
                return -1;
            }

            if (environment_count + 2 >= SIZE_MAX / sizeof(Character*))
            {
                return -1;
            }

            Character** const new_environment = _recalloc_crt_t(Character*, environment, environment_count + 2).detach();
            if (!new_environment)
            {
                return -1;
            }

            new_environment[environment_count]     = owned_option.detach();
            new_environment[environment_count + 1] = nullptr;

            get_environment(Character()) = new_environment;
        }
    }

    // Update the operating system environment.  Do not give an error if this
    // fails since the failure will not affect the user code unless it is making
    // direct calls to the operating system.  We only need to do this for one of
    // the environments; the operating system synchronizes with the other
    // environment automatically.
    if (is_top_level_call)
    {
        size_t const count = traits::tcslen(option) + 2;
        __crt_unique_heap_ptr<Character> const buffer(_calloc_crt_t(Character, count));
        if (!buffer)
        {
            return 0;
        }

        Character* const name = buffer.get();
        _ERRCHECK(traits::tcscpy_s(name, count, option));

        Character* const value = name + (equal_sign - option) + 1;
        *(value - 1) = '\0'; // Overwrite the '=' with a null terminator

        if (traits::set_environment_variable(name, is_removal ? nullptr : value) == 0)
        {
            errno = EILSEQ;
            return -1;
        }
    }

    return 0;
}

extern "C" int __cdecl __dcrt_set_variable_in_narrow_environment_nolock(
    char* const option,
    int   const is_top_level_call
    )
{
    return common_set_variable_in_environment_nolock(option, is_top_level_call);
}

extern "C" int __cdecl __dcrt_set_variable_in_wide_environment_nolock(
    wchar_t* const option,
    int      const is_top_level_call
    )
{
    return common_set_variable_in_environment_nolock(option, is_top_level_call);
}

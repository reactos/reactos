//
// environment_initialization.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// Defines functions for initializing and uninitializing the global environments
// and for constructing and destroying environments.  The logic for manipulating
// the environment data structures is split across this file and the setenv.cpp
// file.
//
#include <corecrt_internal_traits.h>
#include <stdlib.h>
#include <string.h>



// The global environment data.  The initial environments store the pointer to
// the environment that is passed to main or wmain.  This is used only to
// ensure that we do not modify that environment block after we pass it to
// user code.  The _environ_table and _wenviron_table hold the current CRT environment.
// Their names cannot change; they are publicly documented.
extern "C"
{
    char**    __dcrt_initial_narrow_environment = nullptr;
    wchar_t** __dcrt_initial_wide_environment   = nullptr;

    __crt_state_management::dual_state_global<char**>    _environ_table;
    __crt_state_management::dual_state_global<wchar_t**> _wenviron_table;

    char***    __cdecl __p__environ()  { return &_environ_table.value();  }
    wchar_t*** __cdecl __p__wenviron() { return &_wenviron_table.value(); }
}



_Ret_opt_z_
static char**&    get_environment_nolock(char)    throw() { return _environ_table.value();  }

_Ret_opt_z_
static wchar_t**& get_environment_nolock(wchar_t) throw() { return _wenviron_table.value(); }

_Ret_opt_z_
static char**&    __cdecl get_initial_environment(char)    throw() { return __dcrt_initial_narrow_environment; }

_Ret_opt_z_
static wchar_t**& __cdecl get_initial_environment(wchar_t) throw() { return __dcrt_initial_wide_environment;   }

static __crt_state_management::dual_state_global<char**>&    get_dual_state_environment_nolock(char)    throw() { return _environ_table;  }
static __crt_state_management::dual_state_global<wchar_t**>& get_dual_state_environment_nolock(wchar_t) throw() { return _wenviron_table; }



// Counts the number of environment variables in the provided 'environment_block',
// excluding those that start with '=' (these are drive letter settings).
template <typename Character>
static size_t const count_variables_in_environment_block(Character* const environment_block) throw()
{
    typedef __crt_char_traits<Character> traits;

    // Count the number of variables in the environment block, ignoring drive
    // letter settings, which begin with '=':
    size_t count = 0;

    Character* it = environment_block;
    while (*it != '\0')
    {
        if (*it != '=')
            ++count;

        // This advances the iterator to the next string:
        it += traits::tcslen(it) + 1;
    }

    return count;
}



//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// Environment Create and Free (These do not modify any global data)
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// Frees the environment pointed-to by 'environment'.  This function ensures that
// each of the environment strings is freed and that the array itself is freed.
// This function requires that 'environment' is either nullptr or that it points
// to a valid environment (i.e., one composed of a sequence of zero or more non-
// null pointers terminated by a null pointer).
template <typename Character>
static void free_environment(Character** const environment) throw()
{
    if (!environment)
        return;

    for (Character** it = environment; *it; ++it)
        _free_crt(*it);

    _free_crt(environment);
}



// Creates a new environment, populating it with the strings from the provided
// 'environment_block', which must be a double-null-terminated sequence of
// environment variable strings of the form "name=value".  Variables beginning
// with '=' are ignored (these are drive letter settings).  Returns the newly
// created environment on succes; returns nullptr on failure.
template <typename Character>
static Character** const create_environment(Character* const environment_block) throw()
{
    typedef __crt_char_traits<Character> traits;

    size_t const variable_count = count_variables_in_environment_block(environment_block);

    __crt_unique_heap_ptr<Character*> environment(_calloc_crt_t(Character*, variable_count + 1));
    if (!environment)
        return nullptr;

    Character*  source_it = environment_block;
    Character** result_it = environment.get();

    while (*source_it != '\0')
    {
        size_t const required_count = traits::tcslen(source_it) + 1;

        // Don't copy drive letter settings, which start with '=':
        if (*source_it != '=')
        {
            __crt_unique_heap_ptr<Character> variable(_calloc_crt_t(Character, required_count));
            if (!variable)
            {
                free_environment(environment.detach());
                return nullptr;
            }

            _ERRCHECK(traits::tcscpy_s(variable.get(), required_count, source_it));
            *result_it++ = variable.detach();
        }

        // This advances the iterator to the next string:
        source_it += required_count;
    }

    // The sequence of pointers is already null-terminated; return it:
    return environment.detach();
}



//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// Environment Initialize and Uninitialize
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// In the initialize function below, we need to ensure that we've initialized
// the mbc table before we start performing character transformations.
static void pre_initialize(char)    throw() { __acrt_initialize_multibyte(); }
static void pre_initialize(wchar_t) throw() { /* no-op */                    }



// Gets the current environment from the operating system and initializes the
// CRT environment from that environment.  Returns 0 on success; -1 on failure.
// If this function returns successfully, the global environment pointer for
// the requested environment will be non-null and valid.
template <typename Character>
static int __cdecl common_initialize_environment_nolock() throw()
{
    typedef __crt_char_traits<Character> traits;

    // We only initialize the environment once.  Once the environment has been
    // initialized, all updates and modifications go through the other functions
    // that manipulate the environment.
    if (get_environment_nolock(Character()))
        return 0;

    pre_initialize(Character());

    __crt_unique_heap_ptr<Character> const os_environment(traits::get_environment_from_os());
    if (!os_environment)
        return -1;

    __crt_unique_heap_ptr<Character*> crt_environment(create_environment(os_environment.get()));
    if (!crt_environment)
        return -1;

    get_initial_environment(Character()) = crt_environment.get();
    get_dual_state_environment_nolock(Character()).initialize(crt_environment.detach());
    return 0;
}

extern "C" int __cdecl _initialize_narrow_environment()
{
    return common_initialize_environment_nolock<char>();
}

extern "C" int __cdecl _initialize_wide_environment()
{
    return common_initialize_environment_nolock<wchar_t>();
}



// Frees the global wide and narrow environments and returns.
template <typename Character>
static void __cdecl uninitialize_environment_internal(Character**& environment) throw()
{
    if (environment == get_initial_environment(Character()))
    {
        return;
    }

    free_environment(environment);
}

extern "C" void __cdecl __dcrt_uninitialize_environments_nolock()
{
    _environ_table .uninitialize(uninitialize_environment_internal<char>);
    _wenviron_table.uninitialize(uninitialize_environment_internal<wchar_t>);

    free_environment(__dcrt_initial_narrow_environment);
    free_environment(__dcrt_initial_wide_environment);
}



//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// Environment Get-Or-Create
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// These functions help with synchronization between the narrow and wide
// environments.  If the requested environment has not yet been initialized but
// the other environment has been initialized, the other environment is cloned
// to create the requested environment.  Note that if the other environment has
// not yet been initialized, these functions do not do anything.

// Gets the other environment and copies each of the environment variables from
// it into the requested environment.  Returns 0 on success; -1 on failure.
template <typename Character>
static int __cdecl initialize_environment_by_cloning_nolock() throw()
{
    typedef __crt_char_traits<Character> traits;
    typedef typename traits::other_char_type other_char_type;

    other_char_type** const other_environment = get_environment_nolock(other_char_type());
    if (!other_environment)
        return -1;

    for (other_char_type** it = other_environment; *it; ++it)
    {
        size_t const required_count = __crt_compute_required_transform_buffer_count(CP_ACP, *it);
        if (required_count == 0)
            return -1;

        __crt_unique_heap_ptr<Character> buffer(_calloc_crt_t(Character, required_count));
        if (!buffer)
            return -1;

        size_t const actual_count = __crt_transform_string(CP_ACP, *it, buffer.get(), required_count);
        if (actual_count == 0)
            return -1;

        // Ignore a failed attempt to set a variable; continue with the rest...
        traits::set_variable_in_environment_nolock(buffer.detach(), 0);
    }

    return 0;
}



// If the requested environment exists, this function returns it unmodified.  If
// the requested environment does not exist but the other environment does, the
// other environment is cloned to create the requested environment, and the new
// requested environment is returned.  Otherwise, nullptr is returned.
template <typename Character>
_Deref_ret_opt_z_
static Character** __cdecl common_get_or_create_environment_nolock() throw()
{
    typedef __crt_char_traits<Character> traits;
    typedef typename traits::other_char_type other_char_type;

    // Check to see if the required environment already exists:
    Character** const existing_environment = get_environment_nolock(Character());
    if (existing_environment)
        return existing_environment;

    // Check to see if the other environment exists.  We will only initialize
    // the environment here if the other environment was already initialized.
    other_char_type** const other_environment = get_environment_nolock(other_char_type());
    if (!other_environment)
        return nullptr;

    if (common_initialize_environment_nolock<Character>() != 0)
    {
        if (initialize_environment_by_cloning_nolock<Character>() != 0)
        {
            return nullptr;
        }
    }

    return get_environment_nolock(Character());
}

extern "C" char** __cdecl __dcrt_get_or_create_narrow_environment_nolock()
{
    return common_get_or_create_environment_nolock<char>();
}

extern "C" wchar_t** __cdecl __dcrt_get_or_create_wide_environment_nolock()
{
    return common_get_or_create_environment_nolock<wchar_t>();
}

template <typename Character>
static Character** __cdecl common_get_initial_environment() throw()
{
    Character**& initial_environment = get_initial_environment(Character());
    if (!initial_environment)
    {
        initial_environment = common_get_or_create_environment_nolock<Character>();
    }

    return initial_environment;
}

extern "C" char** __cdecl _get_initial_narrow_environment()
{
    return common_get_initial_environment<char>();
}

extern "C" wchar_t** __cdecl _get_initial_wide_environment()
{
    return common_get_initial_environment<wchar_t>();
}

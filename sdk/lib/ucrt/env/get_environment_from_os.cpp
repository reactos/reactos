//
// get_environment_from_os.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// Defines a pair of functions to get the environment strings from the operating
// system.  These functions copy the environment strings into a CRT-allocated
// buffer and return a pointer to that buffer.  The caller is responsible for
// freeing the returned buffer (via _crt_free).
//
#include <corecrt_internal.h>
#include <corecrt_internal_traits.h>
#include <stdlib.h>



namespace
{
    struct environment_strings_traits
    {
        typedef wchar_t* type;

        static bool close(_In_ type p) throw()
        {
            FreeEnvironmentStringsW(p);
            return true;
        }

        static type get_invalid_value() throw()
        {
            return nullptr;
        }
    };

    typedef __crt_unique_handle_t<environment_strings_traits> environment_strings_handle;
}



static wchar_t const* __cdecl find_end_of_double_null_terminated_sequence(wchar_t const* const first) throw()
{
    wchar_t const* last = first;
    for (; *last != '\0'; last += wcslen(last) + 1)
    {
    }

    return last + 1; // Return the pointer one-past-the-end of the double null terminator
}



extern "C" wchar_t* __cdecl __dcrt_get_wide_environment_from_os() throw()
{
    environment_strings_handle const environment(GetEnvironmentStringsW());
    if (!environment)
        return nullptr;

    // Find the
    wchar_t const* const first = environment.get();
    wchar_t const* const last = find_end_of_double_null_terminated_sequence(first);

    size_t const required_count = last - first;

    __crt_unique_heap_ptr<wchar_t> buffer(_malloc_crt_t(wchar_t, required_count));
    if (!buffer)
        return nullptr;

    // Note that the multiplication here cannot overflow:
    memcpy(buffer.get(), environment.get(), required_count * sizeof(wchar_t));
    return buffer.detach();
}



extern "C" char* __cdecl __dcrt_get_narrow_environment_from_os() throw()
{
    // Note that we call GetEnvironmentStringsW and convert to multibyte.  The
    // GetEnvironmentStringsA function returns the environment in the OEM code
    // page, but we need the strings in ANSI.
    environment_strings_handle const environment(GetEnvironmentStringsW());
    if (!environment.is_valid())
        return nullptr;

    wchar_t const* const first = environment.get();
    wchar_t const* const last = find_end_of_double_null_terminated_sequence(first);

    size_t const required_wide_count = last - first;
#pragma warning(suppress:__WARNING_W2A_BEST_FIT) // 38021 Prefast recommends WC_NO_BEST_FIT_CHARS.
    size_t const required_narrow_count = static_cast<size_t>(__acrt_WideCharToMultiByte(
        CP_ACP,
        0,
        environment.get(),
        static_cast<int>(required_wide_count),
        nullptr,
        0,
        nullptr,
        nullptr));

    if (required_narrow_count == 0)
        return nullptr;

    __crt_unique_heap_ptr<char> buffer(_malloc_crt_t(char, required_narrow_count));
    if (!buffer)
        return nullptr;

#pragma warning(suppress:__WARNING_W2A_BEST_FIT) // 38021 Prefast recommends WC_NO_BEST_FIT_CHARS.
    int const conversion_result = __acrt_WideCharToMultiByte(
        CP_ACP,
        0,
        environment.get(),
        static_cast<int>(required_wide_count),
        buffer.get(),
        static_cast<int>(required_narrow_count),
        nullptr,
        nullptr);

    if (conversion_result == 0)
        return nullptr;

    return buffer.detach();
}

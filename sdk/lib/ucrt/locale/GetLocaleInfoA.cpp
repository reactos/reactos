//
// GetLocaleInfoA.cpp
//
//      Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Defines the CRT-internal GetLocaleInfoA implementation.
//
#include <corecrt_internal.h>



// Wraps a call to GetLocaleInfoEx and translates the result into a narrow string.
_Success_(return > 0)
static int __cdecl InternalGetLocaleInfoA(
                                _locale_t   const   locale,
                                PCWSTR      const   locale_name,
                                LCTYPE      const   locale_type,
    _Out_writes_z_(result_size) char*       const   result,
                                int         const   result_size
    )
{
    _LocaleUpdate locale_update(locale);

    int const code_page = locale_update.GetLocaleT()->locinfo->_public._locale_lc_codepage;

    int const buffer_size = __acrt_GetLocaleInfoEx(locale_name, locale_type, nullptr, 0);
    if (buffer_size == 0)
        return 0;

    __crt_scoped_stack_ptr<wchar_t> const buffer(_malloca_crt_t(wchar_t, buffer_size));
    if (buffer.get() == nullptr)
        return 0;

    if (__acrt_GetLocaleInfoEx(locale_name, locale_type, buffer.get(), buffer_size) == 0)
        return 0;

    // Convert from wide to narrow strings:
    return __acrt_WideCharToMultiByte(
        code_page,
        0,
        buffer.get(),
        -1,
        result_size != 0 ? result : nullptr,
        result_size,
        nullptr,
        nullptr);
}



// Gets locale information appropriate for use in the setlocale initialization
// functions.  In particular, wide locale strings can be converted to narrow
// strings or numeric values depending on the value of the lc_type parameter.
//
// The void_result must be reinterpretable as a pointer to one of the following
// types, depending on the value of LC_TYPE:
//
//  * LC_STR_TYPE:   char*
//  * LC_WSTR_TYPE:  wchar_t*
//  * LC_INT_TYPE:   unsigned char
//
// For the first two cases, where a pointer to a pointer is passed, if the
// function succeeds, the caller is responsible for freeing the pointed-to
// buffer.
//
// Returns 0 on success; -1 on failure.
//
// Future optimization:  When converting a large number of wide strings to
// multibyte, we do not need to query the size of the result.  We can convert
// them one after another into a large character buffer.
int __cdecl __acrt_GetLocaleInfoA(
    _locale_t      const locale,
    int            const lc_type,
    wchar_t const* const locale_name,
    LCTYPE         const locale_type,
    void*          const void_result
    )
{
    *static_cast<void**>(void_result) = nullptr;

    if (lc_type == LC_STR_TYPE)
    {
        char** const char_result = static_cast<char**>(void_result);

        int const local_buffer_size = 128;
        char local_buffer[local_buffer_size];

        int const local_length = InternalGetLocaleInfoA(
            locale, locale_name, locale_type, local_buffer, local_buffer_size);

        if (local_length != 0)
        {
            *char_result = _calloc_crt_t(char, local_length).detach();
            if (*char_result == nullptr)
                return -1;

            _ERRCHECK(strncpy_s(*char_result, local_length, local_buffer, local_length - 1));
            return 0;
        }

        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
            return -1;

        // If the buffer size was too small, compute the required size and use a
        // dynamically allocated buffer:
        int const required_length = InternalGetLocaleInfoA(
            locale, locale_name, locale_type, nullptr, 0);

        if (required_length == 0)
            return -1;

        __crt_unique_heap_ptr<char> dynamic_buffer(_calloc_crt_t(char, required_length));
        if (dynamic_buffer.get() == nullptr)
            return -1;

        int const actual_length = InternalGetLocaleInfoA(
            locale, locale_name, locale_type, dynamic_buffer.get(), required_length);

        if (actual_length == 0)
            return -1;

        *char_result = dynamic_buffer.detach();
        return 0;
    }
    else if (lc_type == LC_WSTR_TYPE)
    {
        wchar_t** const wchar_result = static_cast<wchar_t**>(void_result);

        int const required_length = __acrt_GetLocaleInfoEx(locale_name, locale_type, nullptr, 0);
        if (required_length == 0)
            return -1;

        __crt_unique_heap_ptr<wchar_t> dynamic_buffer(_calloc_crt_t(wchar_t, required_length));
        if (dynamic_buffer.get() == nullptr)
            return -1;

        int const actual_length = __acrt_GetLocaleInfoEx(
            locale_name, locale_type, dynamic_buffer.get(), required_length);

        if (actual_length == 0)
            return -1;

        *wchar_result = dynamic_buffer.detach();
        return 0;
    }
    else if (lc_type == LC_INT_TYPE)
    {
        unsigned char* const uchar_result = static_cast<unsigned char*>(void_result);

        DWORD value = 0;
        int const actual_length = __acrt_GetLocaleInfoEx(
            locale_name,
            locale_type | LOCALE_RETURN_NUMBER,
            reinterpret_cast<wchar_t*>(&value),
            sizeof(value) / sizeof(wchar_t));

        if (actual_length == 0)
            return -1;

        *uchar_result = static_cast<unsigned char>(value);
        return 0;
    }

    return -1;
}

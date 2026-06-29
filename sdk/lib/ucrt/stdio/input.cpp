//
// input.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// The core formatted input functions; all of the scanf-style functions call one
// of these functions.
//
#define _ALLOW_OLD_VALIDATE_MACROS
#include <corecrt_internal_stdio_input.h>

using namespace __crt_stdio_input;


// Enclaves do not have a file system, but they do allow in-memory operations
// from stdio.
#ifndef _UCRT_ENCLAVE_BUILD

template <typename Character>
static int __cdecl common_vfscanf(
    unsigned __int64 const options,
    FILE*            const stream,
    Character const* const format,
    _locale_t        const locale,
    va_list          const arglist
    ) throw()
{
    typedef input_processor<
        Character,
        stream_input_adapter<Character>
    > processor_type;

    _VALIDATE_RETURN(stream != nullptr, EINVAL, EOF);
    _VALIDATE_RETURN(format != nullptr, EINVAL, EOF);

    return __acrt_lock_stream_and_call(stream, [&]()
    {
        _LocaleUpdate locale_update(locale);

        processor_type processor(
            stream_input_adapter<Character>(stream),
            options,
            format,
            locale_update.GetLocaleT(),
            arglist);

        return processor.process();
    });
}

extern "C" int __cdecl __stdio_common_vfscanf(
    unsigned __int64 const options,
    FILE*            const stream,
    char const*      const format,
    _locale_t        const locale,
    va_list          const arglist
    )
{
    return common_vfscanf(options, stream, format, locale, arglist);
}

extern "C" int __cdecl __stdio_common_vfwscanf(
    unsigned __int64 const options,
    FILE*            const stream,
    wchar_t const*   const format,
    _locale_t        const locale,
    va_list          const arglist
    )
{
    return common_vfscanf(options, stream, format, locale, arglist);
}

#endif /* _UCRT_ENCLAVE_BUILD */


template <typename Character>
static int __cdecl common_vsscanf(
    unsigned __int64 const options,
    Character const* const buffer,
    size_t           const buffer_count,
    Character const* const format,
    _locale_t        const locale,
    va_list          const arglist
    ) throw()
{
    typedef __acrt_stdio_char_traits<Character> char_traits;

    typedef input_processor<
        Character,
        string_input_adapter<Character>
    > processor_type;

    _VALIDATE_RETURN(buffer != nullptr, EINVAL, EOF);
    _VALIDATE_RETURN(format != nullptr, EINVAL, EOF);

    // The number of elements to consider when scanning is the lesser of [1] the
    // specified number of elements in the buffer or [2] the length of the string
    // in the buffer.
    size_t const buffer_count_for_stream = char_traits::tcsnlen(buffer, buffer_count);

    _LocaleUpdate locale_update(locale);

    processor_type processor(
        string_input_adapter<Character>(buffer, buffer_count_for_stream),
        options,
        format,
        locale_update.GetLocaleT(),
        arglist);

    return processor.process();
}

extern "C" int __cdecl __stdio_common_vsscanf(
    unsigned __int64 const options,
    char const*      const buffer,
    size_t           const buffer_count,
    char const*      const format,
    _locale_t        const locale,
    va_list          const arglist
    )
{
    return common_vsscanf(options, buffer, buffer_count, format, locale, arglist);
}

extern "C" int __cdecl __stdio_common_vswscanf(
    unsigned __int64 const options,
    wchar_t const*   const buffer,
    size_t           const buffer_count,
    wchar_t const*   const format,
    _locale_t        const locale,
    va_list          const arglist
    )
{
    return common_vsscanf(options, buffer, buffer_count, format, locale, arglist);
}

//
// cscanf.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// The core formatted input functions for direct console I/O.
//
#define _ALLOW_OLD_VALIDATE_MACROS
#include <corecrt_internal_stdio_input.h>

using namespace __crt_stdio_input;

template <typename Character>
static int __cdecl common_cscanf(
    unsigned __int64 const options,
    Character const* const format,
    _locale_t        const locale,
    va_list          const arglist
    ) throw()
{
    typedef input_processor<
        Character,
        console_input_adapter<Character>
    > processor_type;

    _LocaleUpdate locale_update(locale);

    processor_type processor(
        console_input_adapter<Character>(),
        options,
        format,
        locale_update.GetLocaleT(),
        arglist);

    return processor.process();
}

extern "C" int __cdecl __conio_common_vcscanf(
    unsigned __int64 const options,
    char const*      const format,
    _locale_t        const locale,
    va_list          const arglist
    )
{
    return common_cscanf(options, format, locale, arglist);
}

extern "C" int __cdecl __conio_common_vcwscanf(
    unsigned __int64 const options,
    wchar_t const*   const format,
    _locale_t        const locale,
    va_list          const arglist
    )
{
    return common_cscanf(options, format, locale, arglist);
}

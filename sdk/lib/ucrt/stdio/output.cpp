//
// output.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// The standard _output functions, which perform formatted output to a stream.
//
#include <corecrt_internal_stdio_output.h>



using namespace __crt_stdio_output;


// Enclaves do not have a file system, but they do allow in-memory operations
// from stdio.
#ifndef _UCRT_ENCLAVE_BUILD

template <template <typename, typename> class Base, typename Character>
static int __cdecl common_vfprintf(
    unsigned __int64   const options,
    FILE*              const stream,
    Character const*   const format,
    __crt_cached_ptd_host&   ptd,
    va_list            const arglist
    ) throw()
{
    typedef output_processor<
        Character,
        stream_output_adapter<Character>,
        Base<Character, stream_output_adapter<Character>>
    > processor_type;

    _UCRT_VALIDATE_RETURN(ptd, stream != nullptr, EINVAL, -1);
    _UCRT_VALIDATE_RETURN(ptd, format != nullptr, EINVAL, -1);

    return __acrt_lock_stream_and_call(stream, [&]() -> int
    {
        __acrt_stdio_temporary_buffering_guard const buffering(stream, ptd);

        processor_type processor(
            stream_output_adapter<Character>(stream),
            options,
            format,
            ptd,
            arglist);

        return processor.process();
    });
}

extern "C" int __cdecl __stdio_common_vfprintf(
    unsigned __int64 const options,
    FILE*            const stream,
    char const*      const format,
    _locale_t        const locale,
    va_list          const arglist
    )
{
    __crt_cached_ptd_host ptd(locale);
    return common_vfprintf<standard_base>(options, stream, format, ptd, arglist);
}

extern "C" int __cdecl __stdio_common_vfwprintf(
    unsigned __int64 const options,
    FILE*            const stream,
    wchar_t const*   const format,
    _locale_t        const locale,
    va_list          const arglist
    )
{
    __crt_cached_ptd_host ptd(locale);
    return common_vfprintf<standard_base>(options, stream, format, ptd, arglist);
}

extern "C" int __cdecl __stdio_common_vfprintf_s(
    unsigned __int64 const options,
    FILE*            const stream,
    char const*      const format,
    _locale_t        const locale,
    va_list          const arglist
    )
{
    __crt_cached_ptd_host ptd(locale);
    return common_vfprintf<format_validation_base>(options, stream, format, ptd, arglist);
}

extern "C" int __cdecl __stdio_common_vfwprintf_s(
    unsigned __int64 const options,
    FILE*            const stream,
    wchar_t const*   const format,
    _locale_t        const locale,
    va_list          const arglist
    )
{
    __crt_cached_ptd_host ptd(locale);
    return common_vfprintf<format_validation_base>(options, stream, format, ptd, arglist);
}

extern "C" int __cdecl __stdio_common_vfprintf_p(
    unsigned __int64 const options,
    FILE*            const stream,
    char const*      const format,
    _locale_t        const locale,
    va_list          const arglist
    )
{
    __crt_cached_ptd_host ptd(locale);
    return common_vfprintf<positional_parameter_base>(options, stream, format, ptd, arglist);
}

extern "C" int __cdecl __stdio_common_vfwprintf_p(
    unsigned __int64 const options,
    FILE*            const stream,
    wchar_t const*   const format,
    _locale_t        const locale,
    va_list          const arglist
    )
{
    __crt_cached_ptd_host ptd(locale);
    return common_vfprintf<positional_parameter_base>(options, stream, format, ptd, arglist);
}

#endif /* _UCRT_ENCLAVE_BUILD */


template <template <typename, typename> class Base, typename Character>
_Success_(return >= 0)
static int __cdecl common_vsprintf(
                                    unsigned __int64   const options,
    _Out_writes_z_(buffer_count)    Character*         const buffer,
                                    size_t             const buffer_count,
                                    Character const*   const format,
                                    __crt_cached_ptd_host&   ptd,
                                    va_list            const arglist
    ) throw()
{
    typedef __acrt_stdio_char_traits<Character> char_traits;

    typedef output_processor<
        Character,
        string_output_adapter<Character>,
        Base<Character, string_output_adapter<Character>>
    > processor_type;

    _UCRT_VALIDATE_RETURN(ptd, format != nullptr,                      EINVAL, -1);
    _UCRT_VALIDATE_RETURN(ptd, buffer_count == 0 || buffer != nullptr, EINVAL, -1);

    string_output_adapter_context<Character> context{};
    context._buffer         = buffer;
    context._buffer_count   = buffer_count;
    context._buffer_used    = 0;

    // For the C Standard snprintf functions, we continue formatting even after
    // the buffer is full so that we can return the number of characters that
    // are required to complete the format operation.  For all other sprintf
    // functions that have a buffer count, if no buffer was provided then we
    // do the same.
    context._continue_count =
        (options & _CRT_INTERNAL_PRINTF_STANDARD_SNPRINTF_BEHAVIOR) != 0 ||
        buffer == nullptr;

    processor_type processor(
        string_output_adapter<Character>(&context),
        options,
        format,
        ptd,
        arglist);

    int const result = processor.process();

    if (buffer == nullptr)
    {
        return result;
    }

    // Otherwise, we formatted data into the buffer and need to terminate it:
    if (options & _CRT_INTERNAL_PRINTF_LEGACY_VSPRINTF_NULL_TERMINATION)
    {
        if (buffer_count == 0 && result != 0)
        {
            return -1;
        }
        else if (context._buffer_used != buffer_count)
        {
            buffer[context._buffer_used] = '\0';
        }
        else if (result >= 0 && static_cast<size_t>(result) > buffer_count)
        {
            return -1;
        }
    }
    else if (options & _CRT_INTERNAL_PRINTF_STANDARD_SNPRINTF_BEHAVIOR)
    {
        if (buffer_count == 0)
        {
            // No-op
        }
        else if (result < 0)
        {
            buffer[0] = '\0';
        }
        else if (context._buffer_used == buffer_count)
        {
            buffer[buffer_count - 1] = '\0';
        }
        else
        {
            buffer[context._buffer_used] = '\0';
        }
    }
    else
    {
        if (buffer_count == 0)
        {
            return -1;
        }
        else if (context._buffer_used == buffer_count)
        {
            buffer[buffer_count - 1] = '\0';
            return -2;
        }
        else
        {
            buffer[context._buffer_used] = '\0';
        }
    }

#pragma warning(suppress:__WARNING_POSTCONDITION_NULLTERMINATION_VIOLATION) // 26036 needs work
    return result;
}

extern "C" int __cdecl __stdio_common_vsprintf(
    unsigned __int64 const options,
    char*            const buffer,
    size_t           const buffer_count,
    char const*      const format,
    _locale_t        const locale,
    va_list          const arglist
    )
{
    __crt_cached_ptd_host ptd(locale);
    return common_vsprintf<standard_base>(options, buffer, buffer_count, format, ptd, arglist);
}

extern "C" int __cdecl __stdio_common_vswprintf(
    unsigned __int64 const options,
    wchar_t*         const buffer,
    size_t           const buffer_count,
    wchar_t const*   const format,
    _locale_t        const locale,
    va_list          const arglist
    )
{
    __crt_cached_ptd_host ptd(locale);
    return common_vsprintf<standard_base>(options, buffer, buffer_count, format, ptd, arglist);
}

template <typename Character>
_Success_(return >= 0)
static int __cdecl common_vsprintf_s(
                                    unsigned __int64   const options,
    _Out_writes_z_(buffer_count)    Character*         const buffer,
                                    size_t             const buffer_count,
                                    Character const*   const format,
                                    __crt_cached_ptd_host&   ptd,
                                    va_list            const arglist
    ) throw()
{
    _UCRT_VALIDATE_RETURN(ptd, format != nullptr,                     EINVAL, -1);
    _UCRT_VALIDATE_RETURN(ptd, buffer != nullptr && buffer_count > 0, EINVAL, -1);

    int const result = common_vsprintf<format_validation_base>(options, buffer, buffer_count, format, ptd, arglist);
    if (result < 0)
    {
        buffer[0] = 0;
        _SECURECRT__FILL_STRING(buffer, buffer_count, 1);
    }

    if (result == -2)
    {
        _UCRT_VALIDATE_RETURN(ptd, ("Buffer too small", 0), ERANGE, -1);
    }
    else if (result >= 0)
    {
        _SECURECRT__FILL_STRING(buffer, buffer_count, result + 1);
    }

    return result;
}

extern "C" int __cdecl __stdio_common_vsprintf_s(
    unsigned __int64 const options,
    char*            const buffer,
    size_t           const buffer_count,
    char const*      const format,
    _locale_t        const locale,
    va_list          const arglist
    )
{
    __crt_cached_ptd_host ptd(locale);
    return common_vsprintf_s(options, buffer, buffer_count, format, ptd, arglist);
}

extern "C" int __cdecl __stdio_common_vswprintf_s(
    unsigned __int64 const options,
    wchar_t*         const buffer,
    size_t           const buffer_count,
    wchar_t const*   const format,
    _locale_t        const locale,
    va_list          const arglist
    )
{
    __crt_cached_ptd_host ptd(locale);
    return common_vsprintf_s(options, buffer, buffer_count, format, ptd, arglist);
}

template <typename Character>
_Success_(return >= 0)
static int __cdecl common_vsnprintf_s(
                                    unsigned __int64   const options,
    _Out_writes_z_(buffer_count)    Character*         const buffer,
                                    size_t             const buffer_count,
                                    size_t             const max_count,
                                    Character const*   const format,
                                    __crt_cached_ptd_host&   ptd,
                                    va_list            const arglist
    ) throw()
{
    _UCRT_VALIDATE_RETURN(ptd, format != nullptr, EINVAL, -1);

    if (max_count == 0 && buffer == nullptr && buffer_count == 0)
        return 0; // No work to do

    _UCRT_VALIDATE_RETURN(ptd, buffer != nullptr && buffer_count > 0, EINVAL, -1);

    int result = -1;
    {
        auto errno_restore_point = ptd.get_errno().create_guard();
        errno_restore_point.disable();

        if (buffer_count > max_count)
        {
            result = common_vsprintf<format_validation_base>(options, buffer, max_count + 1, format, ptd, arglist);

            if (result == -2)
            {
                // The string has been truncated; return -1:
                _SECURECRT__FILL_STRING(buffer, buffer_count, max_count + 1);
                if (ptd.get_errno().check(ERANGE))
                {
                    errno_restore_point.enable();
                }

                return -1;
            }
        }
        else
        {
            result = common_vsprintf<format_validation_base>(options, buffer, buffer_count, format, ptd, arglist);
            buffer[buffer_count - 1] = 0;

            // We allow truncation if count == _TRUNCATE
            if (result == -2 && max_count == _TRUNCATE)
            {
                if (ptd.get_errno().check(ERANGE))
                {
                    errno_restore_point.enable();
                }

                return -1;
            }
        }
    }

    if (result < 0)
    {
        buffer[0] = 0;
        _SECURECRT__FILL_STRING(buffer, buffer_count, 1);
        if (result == -2)
        {
            _UCRT_VALIDATE_RETURN(ptd, ("Buffer too small", 0), ERANGE, -1);
        }

        return -1;
    }

    _SECURECRT__FILL_STRING(buffer, buffer_count, result + 1);

    return result < 0 ? -1 : result;
}

extern "C" int __cdecl __stdio_common_vsnprintf_s(
    unsigned __int64 const options,
    char*            const buffer,
    size_t           const buffer_count,
    size_t           const max_count,
    char const*      const format,
    _locale_t        const locale,
    va_list          const arglist
    )
{
    __crt_cached_ptd_host ptd(locale);
    return common_vsnprintf_s(options, buffer, buffer_count, max_count, format, ptd, arglist);
}

extern "C" int __cdecl __stdio_common_vsnwprintf_s(
    unsigned __int64 const options,
    wchar_t*         const buffer,
    size_t           const buffer_count,
    size_t           const max_count,
    wchar_t const*   const format,
    _locale_t        const locale,
    va_list          const arglist
    )
{
    __crt_cached_ptd_host ptd(locale);
    return common_vsnprintf_s(options, buffer, buffer_count, max_count, format, ptd, arglist);
}

extern "C" int __cdecl __stdio_common_vsprintf_p(
    unsigned __int64 const options,
    char*            const buffer,
    size_t           const buffer_count,
    char const*      const format,
    _locale_t        const locale,
    va_list          const arglist
    )
{
    __crt_cached_ptd_host ptd(locale);
    return common_vsprintf<positional_parameter_base>(options, buffer, buffer_count, format, ptd, arglist);
}

extern "C" int __cdecl __stdio_common_vswprintf_p(
    unsigned __int64 const options,
    wchar_t*         const buffer,
    size_t           const buffer_count,
    wchar_t const*   const format,
    _locale_t        const locale,
    va_list          const arglist
    )
{
    __crt_cached_ptd_host ptd(locale);
    return common_vsprintf<positional_parameter_base>(options, buffer, buffer_count, format, ptd, arglist);
}

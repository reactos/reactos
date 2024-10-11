//
// freopen.cpp
//
//      Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Defines the freopen() family of functions, which is used to close a stream
// and associate it with a new file (typically used for redirecting the
// standard streams).
//
#include <corecrt_internal_stdio.h>



// Reopens a stream to a new file.  If the 'stream' is open, it is closed.  The
// new file, named by 'file_name', is then opened, and 'stream' is associated
// with that file.  The provided 'mode' and 'share_flag' are used when opening
// the new file.  The resulting FILE* is returned via 'result'.
//
// Returns 0 on success; returns an error code on failure.
template <typename Character>
static errno_t __cdecl common_freopen(
    FILE**             const result,
    Character const*   const file_name,
    Character const*   const mode,
    __crt_stdio_stream const stream,
    int                const share_flag
    ) throw()
{
    typedef __acrt_stdio_char_traits<Character> stdio_traits;

    _VALIDATE_RETURN_ERRCODE(result != nullptr, EINVAL);
    *result = nullptr;

    // C11 7.21.5.4/3:  "If filename is a null pointer, the freopen function
    // attempts to change the mode of the stream to that specified by mode, as
    // if the name of the file currently associated with the stream had been
    // used. It is implementation-defined which changes of mode are permitted
    // (if any), and under what circumstances."
    //
    // In our implementation, we do not currently support changing the mode
    // in this way.  In the future, we might consider use of ReOpenFile to
    // implement support for changing the mode.
    _VALIDATE_RETURN_ERRCODE_NOEXC(file_name != nullptr, EBADF);

    _VALIDATE_RETURN_ERRCODE(mode      != nullptr, EINVAL);
    _VALIDATE_RETURN_ERRCODE(stream.valid()      , EINVAL);

    // Just as in the common_fsopen function, we do not hard-validate empty
    // 'file_name' strings in this function:
    _VALIDATE_RETURN_ERRCODE_NOEXC(*file_name != 0, EINVAL);

    errno_t return_value = 0;

    _lock_file(stream.public_stream());
    __try
    {
        // If the stream is in use, try to close it, ignoring possible errors:
        if (stream.is_in_use())
            _fclose_nolock(stream.public_stream());

        stream->_ptr  = nullptr;
        stream->_base = nullptr;
        stream->_cnt  = 0;
        stream.unset_flags(-1);

        // We may have called fclose above, which will deallocate the stream.
        // We still hold the lock on the stream, though, so we can just reset
        // the allocated flag to retain ownership.
        stream.set_flags(_IOALLOCATED);

        *result = stdio_traits::open_file(file_name, mode, share_flag, stream.public_stream());
        if (*result == nullptr)
        {
            stream.unset_flags(_IOALLOCATED);
            return_value = errno;
        }
    }
    __finally
    {
        _unlock_file(stream.public_stream());
    }
    __endtry

    return return_value;
}



extern "C" FILE* __cdecl freopen(
    char const* const file_name,
    char const* const mode,
    FILE*       const public_stream
    )
{
    FILE* result_stream = nullptr;
    common_freopen(&result_stream, file_name, mode, __crt_stdio_stream(public_stream), _SH_DENYNO);
    return result_stream;
}

extern "C" errno_t __cdecl freopen_s(
    FILE**      const result,
    char const* const file_name,
    char const* const mode,
    FILE*       const public_stream
    )
{
    return common_freopen(result, file_name, mode, __crt_stdio_stream(public_stream), _SH_SECURE);
}

extern "C" FILE* __cdecl _wfreopen(
    wchar_t const* const file_name,
    wchar_t const* const mode,
    FILE*          const public_stream
    )
{
    FILE* result_stream = nullptr;
    common_freopen(&result_stream, file_name, mode, __crt_stdio_stream(public_stream), _SH_DENYNO);
    return result_stream;
}

extern "C" errno_t __cdecl _wfreopen_s(
    FILE**         const result,
    wchar_t const* const file_name,
    wchar_t const* const mode,
    FILE*          const public_stream
    )
{
    return common_freopen(result, file_name, mode, __crt_stdio_stream(public_stream), _SH_SECURE);
}

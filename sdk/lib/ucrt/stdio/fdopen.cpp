//
// fdopen.cpp
//
//      Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Defines functions that open a stdio stream over an existing lowio file handle.
//
#include <corecrt_internal_stdio.h>



// Opens a lowio file handle as a stdio stream.  This associates a stream with
// the specified file handle, thus allowing buffering and use with the stdio
// functions.  The mode must be specified and must be compatible with the mode
// with which the file was opened in lowio.
//
// Returns the new FILE* associated with the file handle on success; returns
// nullptr on failure.
template <typename Character>
static FILE* __cdecl common_fdopen(
    int              const fh,
    Character const* const mode
    ) throw()
{
    _VALIDATE_RETURN(mode != nullptr, EINVAL, nullptr);

    _CHECK_FH_RETURN(fh, EBADF, nullptr);
    _VALIDATE_RETURN(fh >= 0 && (unsigned)fh < (unsigned)_nhandle, EBADF, nullptr);
    _VALIDATE_RETURN(_osfile(fh) & FOPEN, EBADF, nullptr);

    __acrt_stdio_stream_mode const parsed_mode = __acrt_stdio_parse_mode(mode);
    if (!parsed_mode._success)
        return nullptr;

    __crt_stdio_stream stream = __acrt_stdio_allocate_stream();
    if (!stream.valid())
    {
        errno = EMFILE;
        return nullptr;
    }

    __try
    {
        // Ensure that streams get flushed during pre-termination:
        #ifndef CRTDLL
        ++_cflush;
        #endif

        stream.set_flags(parsed_mode._stdio_mode);
        stream->_file = fh;
    }
    __finally
    {
        stream.unlock();
    }

    return stream.public_stream();
}



extern "C" FILE* __cdecl _fdopen(int const fh, char const* const mode)
{
    return common_fdopen(fh, mode);
}

extern "C" FILE* __cdecl _wfdopen(int const fh, wchar_t const* const mode)
{
    return common_fdopen(fh, mode);
}

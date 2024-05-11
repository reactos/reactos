//
// openfile.cpp
//
//      Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Functions that open a file as a stdio stream, with mode and share flag.
//
#include <corecrt_internal_stdio.h>
#include <sys/stat.h>



// Opens a file with a string mode and file sharing flag.  This function defines
// the common logic shared by other stdio open functions.  It parses the mode
// string, per the rules described in the above parser, then opens the file via
// the lowio library.
//
// Returns the new FILE* on success; returns nullptr on failure.
template <typename Character>
static FILE* __cdecl common_openfile(
    Character const*   const file_name,
    Character const*   const mode,
    int                const share_flag,
    __crt_stdio_stream const stream
    ) throw()
{
    typedef __acrt_stdio_char_traits<Character> stdio_traits;

    _ASSERTE(file_name != nullptr);
    _ASSERTE(mode      != nullptr);
    _ASSERTE(stream.valid());

    __acrt_stdio_stream_mode const parsed_mode = __acrt_stdio_parse_mode(mode);
    if (!parsed_mode._success)
        return nullptr;

    int fh;
    if (stdio_traits::tsopen_s(&fh, file_name, parsed_mode._lowio_mode, share_flag, _S_IREAD | _S_IWRITE) != 0)
        return nullptr;

    // Ensure that streams get flushed during pre-termination:
    #ifndef CRTDLL
    _cflush++;
    #endif

    // Initialize the stream:
    stream.set_flags(parsed_mode._stdio_mode);
    stream->_cnt      = 0;
    stream->_tmpfname = nullptr;
    stream->_base     = nullptr;
    stream->_ptr      = nullptr;
    stream->_file     = fh;

    return stream.public_stream();
}



extern "C" FILE* __cdecl _openfile(
    char const* const file_name,
    char const* const mode,
    int         const share_flag,
    FILE*       const public_stream
    )
{
    return common_openfile(file_name, mode, share_flag, __crt_stdio_stream(public_stream));
}

extern "C" FILE* __cdecl _wopenfile(
    wchar_t const* const file_name,
    wchar_t const* const mode,
    int            const share_flag,
    FILE*          const public_stream
    )
{
    return common_openfile(file_name, mode, share_flag, __crt_stdio_stream(public_stream));
}
